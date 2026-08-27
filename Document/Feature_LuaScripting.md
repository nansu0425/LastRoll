# Lua 스크립팅 시스템 — Sol2 런타임, 인스턴스 환경 분리, 핫 리로드

> 게임잼 주차(2025-10-31 ~ 11-06) 작업. `ScriptManager.cpp` 기준 약 70%가 본인 코드입니다 (파일별 지분은 [Contribution.md](Contribution.md) 참고). 이 시스템 위에서 Last Roll의 게임 로직(플레이어·적·투사체·게임 매니저)이 전부 Lua로 작성됐습니다.

## 문제

게임잼 7일 중 실제 게임플레이 구현 기간은 절반 정도였고, "수치 하나 바꿀 때마다 C++ 재빌드"로는 iteration이 불가능하다고 판단했습니다. 필요한 것은:

1. 게임 로직을 Lua로 작성하고, 엔진의 Actor/Component/입력/카메라 API를 스크립트에서 호출
2. 같은 스크립트를 쓰는 액터 여러 개가 **각자 독립된 상태**를 가질 것 (적 100마리가 HP를 공유하면 안 됨)
3. 스크립트 에러가 엔진 크래시로 이어지지 않을 것
4. 파일을 저장하면 실행 중인 에디터에 **즉시 반영**될 것 (핫 리로드)

관련 소스: [`ScriptManager.cpp`](../Engine/Source/Manager/Script/Private/ScriptManager.cpp) (바인딩·핫 리로드), [`ScriptComponent.cpp`](../Engine/Source/Component/Private/ScriptComponent.cpp) (인스턴스 환경·라이프사이클·coroutine)

## 설계 1 — 3단 environment 체인으로 인스턴스 상태 분리

Lua state는 프로세스에 하나지만, 스크립트 코드와 인스턴스 상태는 environment로 격리합니다.

```
lua globals            ─ 엔진 API (Vector, SpawnActor, IsKeyDown, ...)
  ▲ fallback (__index)
GlobalTable            ─ 스크립트 파일을 실행한 env — BeginPlay/Tick 등 함수 정의가 여기 담김
  ▲ fallback (__index)
InstanceEnv            ─ ScriptComponent 인스턴스마다 하나 — obj, Owner, self, 헬퍼 함수
```

스크립트 최상위에서 정의한 함수·변수는 전역이 아니라 자신의 env에 갇히므로 스크립트 간 이름 충돌이 없고, 인스턴스 데이터는 `InstanceEnv`에만 존재하므로 액터끼리 상태가 섞이지 않습니다.

여기서 Sol2의 함정을 하나 넘어야 했습니다. 캐싱한 함수에 `InstanceEnv.set_on(func)`으로 환경을 지정하는데, `set_on`은 함수 객체의 `_ENV` upvalue를 **파괴적으로 교체**합니다. 두 컴포넌트가 같은 함수 객체를 공유하면 마지막 `set_on`이 이겨서 첫 번째 액터가 두 번째 액터의 `obj`를 보게 됩니다. 그래서 컴포넌트마다 스크립트를 **별도 env에서 다시 실행해 독립된 함수 객체**를 만듭니다 ([`ScriptManager.cpp`](../Engine/Source/Manager/Script/Private/ScriptManager.cpp)의 `GetTable` 주석에 이 결정을 기록해 뒀습니다).

## 설계 2 — `obj` 메타테이블 프록시

스크립트에서 자기 액터를 다루는 관용구를 이렇게 만들고 싶었습니다.

```lua
obj.Location = obj.Location + dir * obj.Speed * dt   -- Location은 실제 액터 속성
obj.HP = obj.HP - damage                              -- HP는 스크립트가 임의로 붙인 속성
```

문제는 Sol2 usertype userdata에는 Lua에서 임의 필드를 추가할 수 없다는 것입니다. 그래서 `obj`를 usertype이 아니라 **메타테이블을 단 일반 테이블(프록시)** 로 만들었습니다 ([`ScriptComponent.cpp`](../Engine/Source/Component/Private/ScriptComponent.cpp)의 `SetInstanceTable`).

- `__index`: 테이블의 동적 필드를 먼저 찾고(`raw_get`), 없으면 `_actor`에 저장된 `AActor*`에서 Location/Rotation/UUID/Name을 읽음
- `__newindex`: Location/Rotation이면 C++ setter(`SetActorLocation` 등) 호출, 그 외에는 테이블에 저장 — 이 fallthrough가 `obj.HP = 100`을 가능하게 함
- 두 메타메서드 모두 액터 포인터를 **람다에 캡처하지 않고** 호출 시점마다 테이블에서 다시 꺼냅니다. 캡처했다면 액터 파괴 후 죽은 포인터를 잡고 있는 람다가 남습니다.

결과적으로 "엔진이 관리하는 속성"과 "게임플레이가 붙이는 속성"이 스크립트 입장에서는 구분 없이 한 이름 공간으로 보입니다.

## 설계 3 — 라이프사이클 바인딩과 에러 격리

- `UScriptComponent`가 `BeginPlay` / `Tick(dt)` / `EndPlay` / `OnBeginOverlap` / `OnEndOverlap` 5개 함수를 로드 시점에 찾아 **캐싱**하고 `set_on`까지 미리 적용해 둡니다. 매 프레임 Tick 호출은 이 빠른 경로만 탑니다.
- `SOL_ALL_SAFETIES_ON`으로 `sol::function`이 전부 `protected_function`이 되어, **모든 Lua 호출이 pcall을 경유**합니다. 스크립트 런타임 에러는 엔진 콘솔에 로그로 남고 프레임은 계속 진행됩니다 — 게임잼 중 팀원의 스크립트 오타가 엔진을 죽이는 일이 없도록 한 장치입니다.
- Coroutine 지원: `StartCoroutine("Fn")`이 `sol::thread`를 만들고, 스크립트는 `coroutine.yield(WaitForSeconds(1.0))` 패턴으로 대기 조건을 yield합니다. C++ 쪽이 매 Tick 조건을 평가해 재개합니다. GC가 thread를 수거하지 않도록 reference 앵커를 잡아두고, Tick 순회 중 컨테이너가 변하지 않도록 신규 coroutine은 pending 큐에 넣었다가 다음 Tick에 시작합니다. 게임 시작 카운트다운("3, 2, 1")이 이 coroutine으로 구현돼 있습니다.

## 설계 4 — 충돌 이벤트를 Lua로

Lua가 콜백 함수를 직접 넘기는 방식입니다.

```lua
-- Player.lua
local sphere = Owner:GetComponent("USphereComponent")
sphere:BindBeginOverlap(self, OnDetectionBeginOverlap)
```

C++ 쪽은 넘겨받은 `sol::function`을 캡처한 wrapper 람다를 엔진 델리게이트에 등록하는데, 이때 `AddWeakLambda(ScriptComponent, ...)`로 **ScriptComponent를 weak owner로 지정**합니다. 컴포넌트가 파괴되면 델리게이트 엔트리가 자동 무효화되므로, 파괴된 객체의 Lua 함수가 호출되는 사고를 델리게이트 계층에서 차단합니다. 콜백 실행도 protected call이라 충돌 핸들러의 에러 역시 로그로 격리됩니다.

## 설계 5 — 핫 리로드

- **감지**: 에디터 모드에서 0.5초 간격으로 로드된 스크립트들의 `last_write_time`을 폴링합니다. 소스(`Engine/Data/Scripts/`)가 바뀌면 실행 폴더(`Build/.../Data/Scripts/`)로 복사한 뒤 리로드 대상으로 표시합니다.
- **재바인딩**: 스크립트를 새 env에서 다시 실행하고, 그 스크립트를 쓰는 모든 `ScriptComponent`(registry로 추적)에 새 GlobalTable을 전달해 InstanceEnv와 함수 캐시를 재구성합니다.
- **실패 시**: 컴파일 에러는 로그로 남고 해당 스크립트의 콜백들은 정지하지만, **registry 등록은 유지**되므로 파일을 고쳐 저장하면 다음 폴링에서 자동으로 다시 로드됩니다. 엔진을 재시작할 필요가 없습니다.

## 한계

- 핫 리로드가 **인스턴스 상태를 보존하지 않습니다**. InstanceEnv를 새로 만들기 때문에 `obj.HP` 같은 동적 속성이 초기화되고, `BeginPlay`에서 건 overlap 바인딩도 재실행되지 않습니다. 게임잼에서는 "리로드 후 PIE 재시작"으로 운용해 문제가 되지 않았지만, 상태 마이그레이션이 없는 것은 명확한 미완성입니다.
- 핫 리로드는 에디터 모드 한정이며, `require`로 로드한 모듈(`Util.lua` 등)은 `package.loaded` 캐시 때문에 리로드 대상이 아닙니다.
- 컴파일된 chunk 캐시가 없어 같은 스크립트의 인스턴스가 N개면 파일을 N번 읽고 컴파일합니다. 투사체처럼 대량 스폰되는 액터는 Lua 쪽 오브젝트 풀(`ActorPool.lua`)로 우회했습니다.
- Lua에 넘어가는 엔진 객체가 전부 raw pointer라 수명 추적이 없습니다. `obj` 프록시의 재조회 패턴과 `AddWeakLambda`로 주요 경로는 방어했지만, 스크립트가 액터 포인터를 테이블에 보관하면 dangling을 감지할 수 없습니다.
- 타입 바인딩(`RegisterCoreTypes`)이 1,000줄 단일 함수로 자랐고, 컴포넌트 타입을 추가할 때마다 다운캐스트 분기를 손으로 늘려야 합니다. 게임잼 속도 우선의 대가로 남은 빚입니다.
