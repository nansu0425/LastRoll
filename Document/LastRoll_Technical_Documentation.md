# Last Roll - 기술 문서

## 목차
1. [게임 개요](#게임-개요)
2. [FutureEngine Delegate 시스템](#futureengine-delegate-시스템)
3. [FutureEngine Lua 스크립팅 시스템](#futureengine-lua-스크립팅-시스템)
4. [충돌 처리 시스템](#충돌-처리-시스템)
5. [게임 로직 아키텍처](#게임-로직-아키텍처)
6. [핵심 시스템 구현](#핵심-시스템-구현)

---

## 게임 개요

**Last Roll**은 FutureEngine으로 제작된 탑다운 슈팅 서바이벌 게임입니다.

### 게임 컨셉
- **플레이어**: 주사위(Dice)를 조작하며 굴러다니는 캐릭터
- **목표**: 몰려오는 적들(당구공, 체스말)을 공격하며 최대한 오래 생존
- **점수 시스템**: 생존 시간과 처치한 적의 수에 따라 점수 획득
- **레벨 시스템**: 경험치 획득으로 레벨업, 능력치 강화

### 기술 스택
- **엔진**: FutureEngine (커스텀 C++ 게임 엔진, DirectX 11)
- **스크립팅**: Lua (Sol2 바인딩)
- **아키텍처**: Actor-Component 모델 (Unreal Engine 스타일)
- **렌더링**: 탑다운 뷰, 3D 메쉬, 동적 조명

### 게임 씬 구성
씬 파일: [`Engine/Data/Scene/MyScene.Scene`](Engine/Data/Scene/MyScene.Scene)

**주요 액터들:**
1. **Managers (Actor ID: 147)**: 게임 관리 시스템
   - `GameManager.lua`: 게임 상태 관리, 점수/레벨 시스템
   - `UIManager.lua`: UI 렌더링 (데미지 텍스트, HP 바)
   - `EnemySpawner.lua`: 체스말 적 생성기
   - `EnemySpawnerB.lua`: 당구공 적 생성기

2. **조명 시스템**: 플레이어를 따라다니는 3색 조명
   - BlueLight, PinkLight, YellowLight

3. **타일 메쉬**: 체스판 형태의 바닥 타일들

4. **Height Fog**: 분위기를 위한 Exponential Height 포그

---

## FutureEngine Delegate 시스템

FutureEngine은 **Unreal Engine 스타일의 타입 안전 멀티캐스트 델리게이트 시스템**을 구현합니다.

### Delegate 구조

#### 1. 델리게이트 선언 매크로
```cpp
// 단일 바인딩 델리게이트
DECLARE_DELEGATE(FMyDelegate, ReturnType, Param1, Param2)

// 멀티캐스트 델리게이트 (여러 함수 바인딩 가능)
DECLARE_MULTICAST_DELEGATE(FMyMulticastDelegate, void, Param1, Param2)
```

#### 2. 실제 사용 예시
```cpp
// UPrimitiveComponent.h 충돌 이벤트 델리게이트
DECLARE_MULTICAST_DELEGATE(FOnComponentBeginOverlap, FOverlapInfo)
DECLARE_MULTICAST_DELEGATE(FOnComponentEndOverlap, FOverlapInfo)

class UPrimitiveComponent : public USceneComponent
{
public:
    // 충돌 시작/종료 델리게이트
    FOnComponentBeginOverlap OnComponentBeginOverlap;
    FOnComponentEndOverlap OnComponentEndOverlap;
};
```

#### 3. 델리게이트 바인딩 방법

델리게이트는 다양한 함수 타입을 바인딩할 수 있습니다:

```cpp
// 1. 정적 함수 바인딩
Delegate.AddStatic(&GlobalFunction);

// 2. 람다 바인딩
Delegate.AddLambda([](const FOverlapInfo& Info) {
    // 충돌 처리 로직
});

// 3. 약한 참조 람다 바인딩 (권장)
Delegate.AddWeakLambda(this, [this](const FOverlapInfo& Info) {
    // 객체가 파괴되면 자동으로 언바인딩됨
    HandleCollision(Info);
});

// 4. UObject 멤버 함수 바인딩
Delegate.AddUObject(this, &MyClass::MyMethod);

// 5. 공유 포인터 바인딩
Delegate.AddSP(sharedPtr, &MyClass::MyMethod);
```

#### 4. 델리게이트 실행

```cpp
// 멀티캐스트 델리게이트 실행 (모든 바인딩된 함수 호출)
FOverlapInfo Info;
Info.OverlappingComponent = OtherComponent;
OnComponentBeginOverlap.Broadcast(Info);
```

### Delegate 안전성 메커니즘

#### Weak Reference 시스템
델리게이트는 **약한 참조(Weak Reference)**를 사용하여 댕글링 포인터를 방지합니다:

```cpp
// AddWeakLambda 내부 구조 (개념적)
void AddWeakLambda(UObject* WeakRefObject, Lambda&& Func)
{
    // WeakRefObject를 약한 참조로 저장
    auto WeakPtr = MakeWeakObjectPtr(WeakRefObject);

    // 실행 시 객체 유효성 검사
    auto Wrapper = [WeakPtr, Func]() {
        if (WeakPtr.IsValid()) {
            Func();  // 객체가 살아있을 때만 실행
        }
    };

    InvocationList.Add(Wrapper);
}
```

**핵심 특징:**
- 바인딩된 객체가 파괴되면 자동으로 델리게이트 실행이 스킵됨
- 수동 언바인딩 불필요 (자동 메모리 관리)
- nullptr 검사를 내부에서 처리

---

## FutureEngine Lua 스크립팅 시스템

FutureEngine은 **Sol2 라이브러리**를 사용하여 C++과 Lua를 완벽하게 통합합니다.

### 핵심 아키텍처

#### 1. UScriptComponent: Lua 스크립트 컨테이너

모든 액터는 `UScriptComponent`를 추가하여 Lua 스크립트를 실행할 수 있습니다.

```cpp
class UScriptComponent : public UActorComponent
{
private:
    FString ScriptPath;              // 스크립트 파일 경로
    sol::environment InstanceEnv;    // 스크립트 인스턴스 환경

    // 캐싱된 함수 (성능 최적화)
    TMap<FString, sol::function> CachedFunctions;

    // 코루틴 관리
    TArray<CoroutineData> Coroutines;

public:
    void SetInstanceTable(sol::table GlobalTable);  // 스크립트 로드
    void CallLuaFunction(const char* FuncName, Args... args);
    sol::environment& GetEnv() { return InstanceEnv; }
};
```

#### 2. Environment Chain: 스크립트 격리 시스템

각 스크립트 인스턴스는 **독립된 환경(Environment)**을 가지며, 계층적 탐색을 통해 전역 API에 접근합니다:

```
InstanceEnv (인스턴스 전용 변수)
    ↓ (키를 찾지 못하면)
GlobalTable (스크립트 파일의 함수들)
    ↓ (여전히 못 찾으면)
lua.globals() (엔진 API + Lua 표준 라이브러리)
```

**장점:**
- 스크립트 인스턴스 간 변수 충돌 방지
- 같은 스크립트 파일을 여러 액터에서 재사용 가능
- 전역 API는 공유, 인스턴스 데이터는 격리

**구현 코드 (개념적):**
```cpp
void UScriptComponent::SetInstanceTable(sol::table GlobalTable)
{
    // 새로운 인스턴스 환경 생성
    InstanceEnv = sol::environment(ScriptManager.GetLuaState(),
                                    sol::create,
                                    ScriptManager.GetLuaState().globals());

    // GlobalTable을 메타테이블로 설정 (함수 상속)
    sol::table meta = InstanceEnv[sol::metatable_key];
    meta[sol::meta_function::index] = GlobalTable;

    // "obj" 프록시 테이블 설정 (아래 참조)
    SetupObjProxyTable();
}
```

#### 3. "obj" 프록시 테이블: 엔진 ↔ Lua 통신의 핵심

`obj` 테이블은 **메타테이블 마법**을 사용하여 액터 속성에 대한 이중 모드 접근을 제공합니다.

##### 읽기 모드 (`__index` 메타메서드)
```lua
local pos = obj.Location  -- C++ AActor::GetActorLocation() 호출
local rot = obj.Rotation  -- C++ AActor::GetActorRotation() 호출
local id = obj.UUID       -- C++ AActor::GetUUID() 호출
```

##### 쓰기 모드 (`__newindex` 메타메서드)
```lua
obj.Location = Vector(10, 20, 30)  -- C++ AActor::SetActorLocation() 호출
obj.Rotation = Quaternion(...)      -- C++ AActor::SetActorRotation() 호출
```

##### 동적 속성 (Lua 테이블에 저장)
```lua
obj.HP = 100         -- Lua 테이블에 저장 (C++와 무관)
obj.Speed = 15       -- 커스텀 게임 로직용 변수
obj.AttackTimer = 0  -- 프레임 간 유지되는 상태
```

##### 구현 메커니즘 (C++ 쪽)
```cpp
void SetupObjProxyTable()
{
    sol::table obj = InstanceEnv.create_named("obj");

    // 액터 포인터 저장 (약한 참조로 저장하면 더 안전)
    obj.raw_set("_actor", Owner);

    // 메타테이블 설정
    sol::table mt = lua.create_table();

    // READ: obj.Location 같은 접근 처리
    mt[sol::meta_function::index] = [](sol::table self, const std::string& key) -> sol::object {
        AActor* actor = self.raw_get<AActor*>("_actor");
        if (!actor) return sol::lua_nil;

        // C++ 속성 매핑
        if (key == "Location") return sol::make_object(lua, actor->GetActorLocation());
        if (key == "Rotation") return sol::make_object(lua, actor->GetActorRotation());
        if (key == "UUID") return sol::make_object(lua, actor->GetUUID());
        if (key == "Name") return sol::make_object(lua, actor->GetName());

        // Lua 테이블에 저장된 커스텀 속성 반환
        return self.raw_get<sol::object>(key);
    };

    // WRITE: obj.Location = Vector(...) 같은 대입 처리
    mt[sol::meta_function::new_index] = [](sol::table self, const std::string& key, sol::object value) {
        AActor* actor = self.raw_get<AActor*>("_actor");
        if (!actor) return;

        // C++ 속성 매핑
        if (key == "Location") {
            actor->SetActorLocation(value.as<FVector>());
            return;
        }
        if (key == "Rotation") {
            actor->SetActorRotation(value.as<FQuaternion>());
            return;
        }

        // 커스텀 속성은 Lua 테이블에 저장
        self.raw_set(key, value);
    };

    obj[sol::metatable_key] = mt;
}
```

**안전성 보장:**
- 액터 포인터는 **매 접근마다 재검색** (캡처하지 않음)
- nullptr 체크로 댕글링 포인터 방지
- 커스텀 속성은 Lua 테이블에 안전하게 격리

#### 4. Lua 바인딩 API

엔진은 다음 타입들을 Lua에 바인딩합니다:

##### 벡터 & 수학
```lua
-- 생성자
local v = Vector(x, y, z)
local v2 = Vector2(x, y)
local q = Quaternion(x, y, z, w)

-- 연산자
v1 + v2, v1 - v2, v1 * scalar, v1 / scalar

-- 메서드
v:Length(), v:Normalize(), v:Dot(v2), v:Cross(v2)
```

##### 액터 API
```lua
-- 속성
obj.UUID, obj.Location, obj.Rotation, obj.Name

-- 메서드
Owner:GetComponent("USphereComponent")
Owner:GetScriptComponent()
Owner:GetScriptComponentByName("Player.lua")
Owner:AxisRotation(axis, degrees)
Owner:SetCanTick(bool)
Owner:SetActorHiddenInGame(bool)
Owner:SetActorEnableCollision(bool)
```

##### 컴포넌트 API
```lua
-- 충돌 컴포넌트
comp.GenerateOverlapEvents = true
comp.BlockComponent = false
comp.SphereRadius = 10.0

comp:IsOverlappingActor(actor)
comp:IsOverlappingComponent(component)
comp:BindBeginOverlap(scriptComponent, luaFunction)
comp:BindEndOverlap(scriptComponent, luaFunction)
```

##### 입력 시스템
```lua
IsKeyDown(EKeyInput.W)
IsKeyDown(EKeyInput.MouseLeft)
IsKeyPressed(key)
IsKeyReleased(key)
GetMousePosition()  -- 스크린 좌표
GetMouseDelta()
```

##### 월드 & 스폰
```lua
SpawnActor()
SpawnActorByName("AStaticMeshActor")
SpawnActorFromScript("Enemy.lua")
FindActorByName("Player")
```

##### UI 렌더링
```lua
DrawText(text, screenPos, size, fontSize, color)
DrawGaugeBar(pos, size, percent, bgColor, gaugeColor)
WorldToScreenPos(worldPos)
ScreenToWorldPosition(screenPos, playerZ)
```

##### 시간 & 카메라
```lua
GetDeltaTime()
GetTime()
GetCamera()
camera.Location, camera.Rotation
```

#### 5. 코루틴 시스템

Lua 스크립트는 **비동기 대기**를 지원합니다:

```lua
function MyCoroutine()
    print("시작")
    coroutine.yield(WaitForSeconds(2.0))  -- 2초 대기

    print("2초 후")
    coroutine.yield(WaitUntil(function() return obj.HP < 50 end))  -- 조건 대기

    print("HP가 50 이하!")
    coroutine.yield(WaitTick())  -- 1프레임 대기

    print("다음 프레임")
end

StartCoroutine("MyCoroutine")
StopCoroutine("MyCoroutine")  -- 중간에 중단 가능
```

**내부 구조:**
```cpp
struct FWaitCondition {
    float WaitTime;
    EWaitType WaitType;  // Time, Lambda, None
    std::function<bool()> Lambda;
};

struct CoroutineData {
    sol::thread Thread;
    sol::coroutine Coroutine;
    FWaitCondition WaitCondition;
    FName FuncName;
};

// Tick에서 코루틴 업데이트
for (auto& coro : Coroutines) {
    if (coro.WaitType == Time) {
        coro.WaitTime -= DeltaTime;
        if (coro.WaitTime <= 0)
            ResumeCoroutine(coro);
    }
    else if (coro.WaitType == Lambda) {
        if (coro.Lambda())  // 조건 체크
            ResumeCoroutine(coro);
    }
}
```

#### 6. 핫 리로드 (Hot Reload)

개발 중 **스크립트 파일을 수정하면 자동으로 게임에 반영**됩니다 (런타임 중에도!):

**동작 방식:**
1. 0.5초마다 스크립트 파일의 수정 시간(timestamp) 체크
2. 변경 감지 시 파일 재컴파일
3. 해당 스크립트를 사용 중인 모든 `UScriptComponent` 찾기
4. 각 컴포넌트의 `OnScriptReloaded()` 호출하여 환경 업데이트

```cpp
void UScriptManager::HotReloadScripts()
{
    // 변경된 스크립트 탐지
    TArray<FString> ChangedScripts = GatherHotReloadTargets();

    for (const FString& ScriptPath : ChangedScripts) {
        // 재컴파일
        sol::table NewGlobalTable = LoadLuaScript(ScriptPath);

        // 이 스크립트를 사용하는 모든 컴포넌트 찾기
        TArray<UScriptComponent*>& Components = ScriptComponentRegistry[ScriptPath];

        for (UScriptComponent* Comp : Components) {
            // 환경 업데이트 (기존 인스턴스 데이터 유지)
            Comp->OnScriptReloaded(NewGlobalTable);
        }
    }
}
```

**핫 리로드의 장점:**
- 게임 재시작 없이 로직 수정 가능
- 빠른 반복 개발 (Rapid Iteration)
- 인스턴스 데이터 유지 (obj.HP 같은 변수는 그대로)

---

## 충돌 처리 시스템

Last Roll의 충돌 처리는 **C++ 충돌 감지 + Lua 이벤트 처리**의 하이브리드 구조입니다.

### 1. C++ 충돌 감지 시스템

#### UPrimitiveComponent: 충돌 가능한 컴포넌트
```cpp
class UPrimitiveComponent : public USceneComponent
{
public:
    // 충돌 델리게이트
    FOnComponentBeginOverlap OnComponentBeginOverlap;
    FOnComponentEndOverlap OnComponentEndOverlap;

    // 충돌 설정
    bool GenerateOverlapEvents = true;  // Overlap 이벤트 활성화
    bool BlockComponent = false;        // 물리적 충돌 (현재 미사용)

    // 현재 겹치는 컴포넌트 목록
    TArray<FOverlapInfo> OverlapInfos;

    // 충돌 업데이트 (매 프레임 호출)
    void UpdateOverlaps(const TArray<UPrimitiveComponent*>& AllComponents);
};
```

#### 충돌 감지 파이프라인
```
UWorld::Tick()
    ↓
각 UPrimitiveComponent::UpdateOverlaps(모든 컴포넌트)
    ↓
각 컴포넌트 쌍에 대해:
    CheckOverlapWith(OtherComponent)
        ↓
    CollisionDetection::CheckOverlap(Shape1, Shape2)
        ↓
    [새로운 충돌 발견]
        → OverlapInfos에 추가
        → OnComponentBeginOverlap.Broadcast(Info)
    [충돌 종료]
        → OverlapInfos에서 제거
        → OnComponentEndOverlap.Broadcast(Info)
```

#### 충돌 형상 타입
```cpp
// Engine/Source/Physics/
FAABB                // 축 정렬 바운딩 박스
FBoundingSphere      // 구체 (Last Roll에서 주로 사용)
FBoundingCapsule     // 캡슐

// 충돌 체크 함수들
namespace CollisionDetection {
    bool CheckOverlap(const FBoundingSphere& A, const FBoundingSphere& B);
    bool CheckOverlap(const FAABB& Box, const FBoundingSphere& Sphere);
    bool CheckOverlap(const FBoundingCapsule& A, const FBoundingCapsule& B);
}
```

### 2. Lua 델리게이트 바인딩

Lua 함수를 C++ 델리게이트에 바인딩하는 **마법의 래퍼**:

```cpp
// ScriptManager::RegisterCoreTypes() 내부
lua.new_usertype<UPrimitiveComponent>("PrimitiveComponent",
    "BindBeginOverlap",
    [](UPrimitiveComponent* PrimComp, UScriptComponent* ScriptComp, sol::function LuaFunc)
    {
        // Lua 함수를 C++ 람다로 감싸기
        auto Wrapper = [LuaFuncCaptured = std::move(LuaFunc)](const FOverlapInfo& Info)
        {
            AActor* OtherActor = Info.OverlappingComponent ?
                                 Info.OverlappingComponent->GetOwner() : nullptr;

            if (!OtherActor) return;

            // Lua 함수 호출
            sol::protected_function_result Result = LuaFuncCaptured(OtherActor);

            if (!Result.valid()) {
                sol::error Error = Result;
                UE_LOG_ERROR("Lua BindBeginOverlap Error: %s", Error.what());
            }
        };

        // C++ 델리게이트에 바인딩 (약한 참조 사용)
        PrimComp->OnComponentBeginOverlap.AddWeakLambda(ScriptComp, std::move(Wrapper));
    }
);
```

**핵심 포인트:**
1. Lua 함수를 `std::function`으로 캡처
2. C++ 델리게이트 래퍼로 감싸기
3. `AddWeakLambda`로 안전하게 바인딩 (스크립트 컴포넌트 파괴 시 자동 해제)
4. Lua 함수 실행 시 에러 처리 (`protected_function_result`)

### 3. Lua 측 충돌 처리 코드

#### Player.lua 충돌 감지 예시
```lua
function BeginPlay()
    -- Detection Collider 가져오기 (USphereComponent)
    local DetectionCollider = Owner:GetComponent("USphereComponent")

    if DetectionCollider then
        -- Lua 함수를 델리게이트에 바인딩
        DetectionCollider:BindBeginOverlap(self, OnDetectionBeginOverlap)
        DetectionCollider:BindEndOverlap(self, OnDetectionEndOverlap)
    end
end

-- 충돌 시작 이벤트
function OnDetectionBeginOverlap(OtherActor)
    -- Enemy 판별: TakeDamage 함수 존재 여부로 확인
    local ScriptComp = OtherActor:GetScriptComponent()
    if ScriptComp then
        local Env = ScriptComp:GetEnv()
        if Env["TakeDamage"] then
            -- Enemy 리스트에 추가
            table.insert(DetectedEnemies, OtherActor)
        end
    end
end

-- 충돌 종료 이벤트
function OnDetectionEndOverlap(OtherActor)
    for i, Enemy in ipairs(DetectedEnemies) do
        if Enemy.UUID == OtherActor.UUID then
            table.remove(DetectedEnemies, i)
            break
        end
    end
end
```

#### LinearProjectile.lua 충돌 처리
```lua
function OnBeginOverlap(OtherActor)
    -- 자기 자신과의 충돌 무시
    if OtherActor.UUID == obj.UUID then
        return
    end

    -- Enemy와 충돌 체크
    local ScriptComp = OtherActor:GetScriptComponent()
    if ScriptComp then
        local Env = ScriptComp:GetEnv()

        -- TakeDamage 함수가 있으면 Enemy로 판별
        if Env["TakeDamage"] then
            -- 죽은 Enemy는 무시
            if Env["obj"] and Env["obj"].IsDead then
                return
            end

            -- 데미지 처리
            Env["TakeDamage"](obj.Damage)

            -- 투사체 제거
            ReturnToPool()
        end
    end
end
```

### 4. 충돌 처리 패턴

#### 패턴 1: Duck Typing으로 타입 판별
```lua
-- Enemy 판별: TakeDamage 함수 존재 여부
if Env["TakeDamage"] then
    -- Enemy로 간주
end

-- Player 판별: TakeDamagePlayer 함수 존재 여부
if Env["TakeDamagePlayer"] then
    -- Player로 간주
end
```

#### 패턴 2: UUID로 액터 추적
```lua
-- HomingProjectile.lua: 타겟 추적
obj.TargetUUID = TargetActor.UUID  -- 생성 시 UUID 저장

-- Tick에서 유효성 검사
if obj.TargetActor.UUID ~= obj.TargetUUID then
    obj.TargetLost = true  -- 타겟이 바뀜
end
```

#### 패턴 3: 충돌 무시 조건
```lua
-- 자기 자신
if OtherActor.UUID == obj.UUID then return end

-- 특정 액터 (OrbitProjectile이 Player 무시)
if OtherActor.UUID == obj.CenterActor.UUID then return end

-- 죽은 Enemy
if Env["obj"] and Env["obj"].IsDead then return end
```

---

## 게임 로직 아키텍처

Last Roll의 게임 로직은 **완전히 Lua로 구현**되어 있으며, 모듈화된 설계를 따릅니다.

### 전역 데이터 테이블

게임 전체에서 공유하는 데이터를 `_G` (Lua 전역 테이블)에 저장합니다:

```lua
-- _G.PlayerData: 플레이어 관련 정보
_G.PlayerData = {
    PlayerPos = Vector(0,0,0),        -- 현재 위치
    PlayerEnv = nil,                  -- Player 스크립트 환경
    bPlayerAlive = false              -- 생존 여부
}

-- _G.GameData: 게임 상태
_G.GameData = {
    GameState = EGameState.Lobby,     -- 현재 게임 상태
    Score = 0,                        -- 점수
    EXP = 0,                          -- 경험치
    Level = 1,                        -- 레벨
    ManagerActor = nil,               -- 관리자 액터
    GMEnv = nil                       -- GameManager 환경
}

-- _G.UIData: UI 데이터
_G.UIData = {
    DamageTextList = {},              -- 데미지 텍스트 목록
    TrailTextList = {}                -- 트레일 텍스트 목록
}
```

### 게임 상태 머신 (GameManager.lua)

```lua
EGameState = {
    Lobby = 1,        -- 대기 화면 (W키 누르면 시작)
    Loading = 2,      -- 카운트다운 (3, 2, 1, 0)
    Playing = 3,      -- 게임 플레이 중
    EndSequence = 4,  -- 사망 애니메이션
    End = 5           -- 재시작 대기
}

function ChangeGameState(InGameState)
    if InGameState == EGameState.Loading then
        StartCoroutine("StartSequence")  -- 3초 카운트다운 코루틴
    elseif InGameState == EGameState.EndSequence then
        StartCoroutine("EndingSequence")  -- 사망 시퀀스 코루틴
    end
    _G.GameData.GameState = InGameState
end
```

### 점수 & 레벨 시스템

```lua
-- 레벨별 필요 경험치
local LevelEXP = {2, 5, 15, 30}
local MaxLevel = 5

function AddScore(Score)
    _G.GameData.Score = _G.GameData.Score + Score
end

function AddEXP(EXP)
    _G.GameData.EXP = _G.GameData.EXP + EXP

    -- 레벨업 체크
    if _G.GameData.Level < MaxLevel then
        if _G.GameData.EXP >= LevelEXP[_G.GameData.Level] then
            _G.GameData.EXP = _G.GameData.EXP - LevelEXP[_G.GameData.Level]
            _G.GameData.Level = _G.GameData.Level + 1
            LevelUp()  -- 플레이어 강화
        end
    end
end

function LevelUp()
    if _G.PlayerData.bPlayerAlive then
        -- 플레이어 스크립트의 LevelUp 함수 호출
        _G.PlayerData.PlayerEnv.LevelUp(_G.GameData.Level)
    end
end
```

### Actor Pool 시스템

메모리 효율과 성능 최적화를 위한 **오브젝트 풀링**:

```lua
-- ActorPool.lua (싱글톤 모듈)
local ActorPool = {}

-- 자료 구조
ActorPool.InactiveActors = {}  -- 타입별 비활성 액터 목록
ActorPool.ActorToNameMap = {}  -- UUID → 타입 매핑

function ActorPool:Get(ActorName)
    local Pool = self.InactiveActors[ActorName] or {}

    -- 풀에서 꺼내기 (재사용)
    local Actor = table.remove(Pool)

    if Actor then
        -- 재활성화
        Actor:SetCanTick(true)
        Actor:SetActorHiddenInGame(false)
        Actor:SetActorEnableCollision(true)
    else
        -- 새로 생성
        Actor = SpawnActorByName(ActorName)
        Actor:SetCanTick(true)
        Actor:SetActorHiddenInGame(false)
        Actor:SetActorEnableCollision(true)

        -- 매핑 등록
        self.ActorToNameMap[Actor.UUID] = ActorName
    end

    return Actor
end

function ActorPool:Return(Actor)
    local ActorName = self.ActorToNameMap[Actor.UUID]
    if not ActorName then return end

    -- 비활성화
    Actor:SetCanTick(false)
    Actor:SetActorHiddenInGame(true)
    Actor:SetActorEnableCollision(false)
    Actor:StopAllCoroutine()

    -- 풀에 반납
    table.insert(self.InactiveActors[ActorName], Actor)
end
```

**사용 예시:**
```lua
-- Enemy 생성
local Enemy = ActorPool:Get("AEnemy")
Enemy.Location = Vector(100, 100, 0)

-- 사용 후 반납 (제거 대신)
ActorPool:Return(Enemy)
```

---

## 핵심 시스템 구현

### 1. Player 시스템 (Player.lua)

플레이어는 **3가지 투사체 시스템**을 가집니다:

#### A. Linear Projectile (직선 투사체)
- **발사 방식**: 마우스 커서 방향으로 직선 발사
- **쿨타임**: 0.3초 (레벨업 시 감소)
- **자동 발사**: 쿨타임마다 자동 발사

```lua
function HandleAttack(dt)
    obj.AttackTimer = obj.AttackTimer - dt

    if obj.AttackTimer <= 0 then
        ShootProjectile()  -- 마우스 방향으로 발사
        obj.AttackTimer = AttackCooldown
    end
end

function ShootProjectile()
    -- 마우스 월드 위치 계산
    local MouseScreenPos = GetMousePosition()
    local MouseWorldPos2D = ScreenToWorldPosition(MouseScreenPos, obj.Location.z)

    -- 발사 방향
    local ShootDirection = Vector(
        MouseWorldPos2D.x - obj.Location.x,
        MouseWorldPos2D.y - obj.Location.y,
        0
    )
    ShootDirection:Normalize()

    -- 투사체 풀에서 가져오기
    local Projectile = ActorPool:Get("ALinearProjectile")

    if Projectile then
        Projectile.Location = obj.Location + ShootDirection * 2.0

        -- 투사체 설정
        local ProjScript = Projectile:GetScriptComponent()
        local Env = ProjScript:GetEnv()
        Env["Setup"](ShootDirection, LinearProjectileSpeed, ProjectileDamage, ProjectileRange)
    end
end
```

#### B. Homing Projectile (유도 투사체)
- **발사 방식**: 감지 범위 내 가장 가까운 적 자동 추적
- **쿨타임**: 1.0초
- **특징**:
  - 목표를 향해 부드럽게 유도
  - 적이 죽어도 마지막 위치까지 이동
  - 등가속도 운동으로 자연스러운 궤적

```lua
function HandleAutoTarget(dt)
    obj.AutoTargetTimer = obj.AutoTargetTimer - dt

    if obj.AutoTargetTimer <= 0 then
        local TargetEnemy = FindNearestEnemy()

        if TargetEnemy then
            ShootHomingProjectile(TargetEnemy)
            obj.AutoTargetTimer = AutoTargetCooldown
        end
    end
end

function FindNearestEnemy()
    local NearestEnemy = nil
    local MinDistance = math.huge

    for _, Enemy in ipairs(DetectedEnemies) do
        -- 죽은 적 제외
        local ScriptComp = Enemy:GetScriptComponent()
        local Env = ScriptComp:GetEnv()
        if Env["obj"].IsDead then goto continue end

        local Distance = (Enemy.Location - obj.Location):Length()

        if Distance < MinDistance then
            MinDistance = Distance
            NearestEnemy = Enemy
        end

        ::continue::
    end

    return NearestEnemy
end
```

#### HomingProjectile.lua: 유도 로직
```lua
function Tick(dt)
    -- 타겟 상태 업데이트
    UpdateTargetStatus()  -- 살아있는지 확인

    -- 타겟 방향 계산
    local ToTarget = obj.TargetLocation - obj.Location
    local DistanceToTarget = ToTarget:Length()

    -- 강제 충돌 체크 (tunneling 방지)
    if DistanceToTarget <= ForceCollisionDistance then
        HandleTargetCollision(obj.TargetActor)
        return
    end

    -- 등가속도 운동: v = v0 + a*t
    obj.ElapsedTime = obj.ElapsedTime + dt
    obj.Speed = InitialSpeed + obj.Acceleration * obj.ElapsedTime
    obj.Speed = math.min(obj.Speed, obj.DynamicMaxSpeed)

    -- 유도: Lerp로 부드러운 회전
    ToTarget:Normalize()
    obj.Direction = LerpVector(obj.Direction, ToTarget, RotationSpeed * dt)
    obj.Direction:Normalize()

    -- 이동
    obj.Location = obj.Location + obj.Direction * obj.Speed * dt
end
```

#### C. Orbit Projectile (궤도 투사체)
- **개수**: 3~5개 (레벨업 시 증가)
- **동작**: 플레이어 주위를 회전하며 적과 충돌
- **특징**: 사라지지 않고 지속적인 근접 방어

```lua
function SpawnOrbitProjectiles()
    ClearOrbitProjectiles()  -- 기존 제거

    for i = 1, OrbitCount do
        local Projectile = ActorPool:Get("AOrbitProjectile")

        -- 각도 균등 분배
        local InitialAngle = (360.0 / OrbitCount) * (i - 1)

        local ProjScript = Projectile:GetScriptComponent()
        local Env = ProjScript:GetEnv()
        Env["Setup"](Owner, InitialAngle, OrbitRadius, OrbitSpeed, OrbitDamage)

        table.insert(OrbitProjectiles, Projectile)
    end
end

-- 레벨업 시 동적 업데이트
function SetOrbitCount(NewCount)
    OrbitCount = NewCount
    SpawnOrbitProjectiles()  -- 재생성
end
```

#### OrbitProjectile.lua: 원운동 로직
```lua
function Tick(dt)
    -- 각도 업데이트
    obj.CurrentAngle = obj.CurrentAngle + obj.RotationSpeed * dt

    -- 각도를 0~360 범위로 유지
    if obj.CurrentAngle >= 360.0 then
        obj.CurrentAngle = obj.CurrentAngle - 360.0
    end

    -- 원운동 위치 계산
    local CenterPos = obj.CenterActor.Location
    local AngleRad = math.rad(obj.CurrentAngle)

    obj.Location = Vector(
        CenterPos.x + obj.Radius * math.cos(AngleRad),
        CenterPos.y + obj.Radius * math.sin(AngleRad),
        CenterPos.z
    )
end
```

#### 플레이어 이동 & 카메라
```lua
function Move(dt)
    local MoveDir = Vector(0,0,0)

    -- WASD 입력
    if IsKeyDown(EKeyInput.W) then
        MoveDir.x = MoveDir.x + 1
        Owner:AxisRotation(Vector(0,1,0), 500 * dt)  -- 주사위 회전
    end
    if IsKeyDown(EKeyInput.A) then
        MoveDir.y = MoveDir.y - 1
        Owner:AxisRotation(Vector(1,0,0), 500 * dt)
    end
    if IsKeyDown(EKeyInput.S) then
        MoveDir.x = MoveDir.x - 1
        Owner:AxisRotation(Vector(0,1,0), -500 * dt)
    end
    if IsKeyDown(EKeyInput.D) then
        MoveDir.y = MoveDir.y + 1
        Owner:AxisRotation(Vector(1,0,0), -500 * dt)
    end

    MoveDir:Normalize()
    obj.Location = obj.Location + MoveDir * obj.Speed * dt
    _G.PlayerData.PlayerPos = obj.Location

    -- 카메라 따라가기
    TopCamera()
end

function TopCamera()
    local TargetPos = obj.Location
    GetCamera().Location = TargetPos + Vector(-5, 0, 25)  -- 탑다운 뷰
    GetCamera().Rotation = Vector(0, -70, 0)  -- 아래를 내려다봄
end
```

#### 레벨업 강화
```lua
function LevelUp(CurLevel)
    -- 데미지 증가
    ProjectileDamage = ProjectileDamage + 2

    -- 공격 속도 증가
    AttackCooldown = AttackCooldown - 0.02

    if CurLevel == 3 then
        SetOrbitCount(4)            -- Orbit 개수 증가
        LinearProjectileSpeed = 55  -- 투사체 속도 증가
    elseif CurLevel == 5 then
        SetOrbitCount(5)
        SetOrbitSpeed(220)
        SetOrbitRadius(12.0)
        SetOrbitDamage(8)
        LinearProjectileSpeed = 60
    end
end
```

### 2. Enemy 시스템

Last Roll은 **2가지 타입의 적**을 가집니다:

#### A. EnemyA (체스말) - 홉 이동

체스 나이트처럼 점프하며 이동하는 적:

```lua
-- 상태 머신
obj.MoveState = "Idle"  -- Idle → Lifting → Rotate → Moving → Landing

-- Idle: 다음 이동 대기
if obj.MoveState == "Idle" then
    obj.MoveTimer = obj.MoveTimer - dt

    -- 공격 거리 밖에 있으면 이동 시작
    if CurrentPlayerDis > AttackDis and obj.MoveTimer <= 0 then
        obj.MoveState = "Lifting"
        obj.StartMovePos = obj.Location

        -- 목표 위치 계산 (플레이어 방향)
        local DirNorm = DirToPlayer
        DirNorm:Normalize()
        obj.TargetMovePos = obj.Location + DirNorm * obj.StepDistance
    end
end

-- Lifting: 위로 떠오르기
elseif obj.MoveState == "Lifting" then
    obj.MoveAnimTimer = obj.MoveAnimTimer + dt
    local AnimTime = obj.MoveAnimDuration * 0.3
    local Alpha = obj.MoveAnimTimer / AnimTime

    if Alpha >= 1.0 then
        newZ = obj.GroundZ + obj.LiftHeight
        obj.MoveState = "Rotate"
        obj.MoveAnimTimer = 0.0
    else
        -- EaseOutQuad로 부드러운 상승
        newZ = Lerp(obj.GroundZ, obj.GroundZ + obj.LiftHeight, -Alpha * (Alpha - 2))
    end

    obj.Location = Vector(obj.StartMovePos.x, obj.StartMovePos.y, newZ)
end

-- Rotate: 플레이어 방향으로 회전
elseif obj.MoveState == "Rotate" then
    local CurForwardXY = Util.GetForwardFromDegreeZ(obj.CurRotZ)
    local ToPlayerDegree = Util.Rotate2DDegree(CurForwardXY, obj.ToPlayerDirXY)
    local CurRotDegree = math.random(500, 1000) * dt * Util.Sign(ToPlayerDegree)

    if math.abs(ToPlayerDegree) <= math.abs(CurRotDegree) then
        Owner:AxisRotation(Vector(0,0,1), ToPlayerDegree)
        obj.CurRotZ = obj.CurRotZ + ToPlayerDegree
        obj.MoveState = "Moving"
    else
        Owner:AxisRotation(Vector(0,0,1), CurRotDegree)
        obj.CurRotZ = obj.CurRotZ + CurRotDegree
    end
end

-- Moving: 수평 이동
elseif obj.MoveState == "Moving" then
    obj.MoveAnimTimer = obj.MoveAnimTimer + dt
    local AnimTime = obj.MoveAnimDuration * 0.4
    local Alpha = obj.MoveAnimTimer / AnimTime

    if Alpha >= 1.0 then
        newX = obj.TargetMovePos.x
        newY = obj.TargetMovePos.y
        obj.MoveState = "Landing"
        obj.MoveAnimTimer = 0.0
    else
        newX = Lerp(obj.StartMovePos.x, obj.TargetMovePos.x, Alpha)
        newY = Lerp(obj.StartMovePos.y, obj.TargetMovePos.y, Alpha)
    end

    obj.Location = Vector(newX, newY, obj.GroundZ + obj.LiftHeight)
end

-- Landing: 착지
elseif obj.MoveState == "Landing" then
    obj.MoveAnimTimer = obj.MoveAnimTimer + dt
    local AnimTime = obj.MoveAnimDuration * 0.3
    local Alpha = obj.MoveAnimTimer / AnimTime

    if Alpha >= 1.0 then
        obj.Location = obj.TargetMovePos
        obj.MoveState = "Idle"
        obj.MoveTimer = obj.MoveInterval

        -- 착지 시 플레이어와 가까우면 공격
        if DisToPlayer < 3.5 then
            _G.PlayerData.PlayerEnv.TakeDamagePlayer(obj.Dmg)
        end
    else
        -- EaseInQuad로 부드러운 착지
        local newZ = Lerp(obj.GroundZ + obj.LiftHeight, obj.GroundZ, Alpha * Alpha)
        obj.Location = Vector(obj.TargetMovePos.x, obj.TargetMovePos.y, newZ)
    end
end
```

#### B. EnemyB (당구공) - 직선 돌진

플레이어를 향해 굴러가며 돌진하는 적:

```lua
-- 상태 머신
obj.MoveState = "Idle"  -- Idle → Moving → Decelerating

-- Idle: 새로운 목표 설정
if obj.MoveState == "Idle" then
    -- 공격 거리 밖에 있으면 돌진 시작
    if CurrentPlayerDis > obj.AttackDistance then
        obj.TargetPosition = PlayerPos
        obj.MoveDirection = DirToPlayer
        obj.MoveDirection:Normalize()
        obj.MoveState = "Moving"
        obj.LastPlayerDistance = CurrentPlayerDis
    end
end

-- Moving: 가속하며 이동
elseif obj.MoveState == "Moving" then
    -- Lerp로 부드러운 가속
    obj.CurrentSpeed = Lerp(obj.CurrentSpeed, obj.MaxSpeed, dt * obj.Acceleration)
    obj.Location = obj.Location + obj.MoveDirection * obj.CurrentSpeed * dt

    -- 굴러가는 애니메이션
    if obj.CurrentSpeed > 0.01 then
        local RollAxis = Vector(-obj.MoveDirection.y, obj.MoveDirection.x, 0)
        local RollAngle = obj.CurrentSpeed * obj.RollSpeedFactor * dt
        Owner:AxisRotation(RollAxis, RollAngle)
    end

    -- 플레이어를 지나쳐서 멀어지면 감속 시작
    if CurrentPlayerDis > obj.LastPlayerDistance and
       CurrentPlayerDis > obj.MaxChaseDistance then
        obj.MoveState = "Decelerating"
    end

    obj.LastPlayerDistance = CurrentPlayerDis
end

-- Decelerating: 감속
elseif obj.MoveState == "Decelerating" then
    obj.CurrentSpeed = Lerp(obj.CurrentSpeed, 0.0, dt * obj.Deceleration)
    obj.Location = obj.Location + obj.MoveDirection * obj.CurrentSpeed * dt

    -- 계속 굴러가는 애니메이션
    if obj.CurrentSpeed > 0.01 then
        local RollAxis = Vector(-obj.MoveDirection.y, obj.MoveDirection.x, 0)
        local RollAngle = obj.CurrentSpeed * obj.RollSpeedFactor * dt
        Owner:AxisRotation(RollAxis, RollAngle)
    end

    -- 완전히 멈추면 Idle로
    if obj.CurrentSpeed < obj.MinStopSpeed then
        obj.CurrentSpeed = 0.0
        obj.MoveState = "Idle"
    end
end
```

#### 공격 로직 (상태와 분리)
```lua
-- 쿨타임 감소
if obj.CurrentAttackDelay > 0 then
    obj.CurrentAttackDelay = obj.CurrentAttackDelay - dt
end

-- 쿨타운이 0이고 사거리 내에 있으면 (이동 상태와 무관하게) 공격
if obj.CurrentAttackDelay <= 0 and CurrentPlayerDis <= obj.AttackDistance then
    Attack()  -- 플레이어에게 데미지
    obj.CurrentAttackDelay = AttackDelay
end
```

#### 사망 애니메이션
```lua
function Die()
    obj.IsDead = true
    obj.IsDying = true
    obj.MoveState = "Idle"

    -- 튕겨 올라가는 초기 속도
    local UpwardForce = Vector(0, 0, obj.DeathInitialFlySpeed)
    local OutwardForce = obj.KnockbackDir * (obj.DeathInitialFlySpeed * 0.5)
    obj.DeathFlightVelocity = UpwardForce + OutwardForce

    -- 랜덤 회전
    obj.DeathSpinSpeed = Random(360, 720)
    obj.DeathSpinAxis = Vector(Random(-1, 1), Random(-1, 1), Random(-1, 1))
    obj.DeathSpinAxis:Normalize()
end

-- Tick에서 사망 애니메이션 처리
if obj.IsDying then
    -- 중력 적용
    obj.DeathFlightVelocity.z = obj.DeathFlightVelocity.z - obj.DeathGravity * dt
    obj.Location = obj.Location + obj.DeathFlightVelocity * dt

    -- 회전
    local SpinAngleDelta = obj.DeathSpinSpeed * dt
    Owner:AxisRotation(obj.DeathSpinAxis, SpinAngleDelta)

    -- 충분히 떨어지면 풀에 반납
    CheckCanReturnToPool()
    return
end
```

#### HomingProjectile 추적 시스템
Enemy가 죽어도 **타게팅된 HomingProjectile이 모두 충돌할 때까지 대기**:

```lua
-- Enemy.lua
function RegisterProjectile(ProjectileUUID)
    table.insert(obj.TargetingProjectiles, ProjectileUUID)
end

function UnregisterProjectile(ProjectileUUID)
    for i, UUID in ipairs(obj.TargetingProjectiles) do
        if UUID == ProjectileUUID then
            table.remove(obj.TargetingProjectiles, i)
            CheckCanReturnToPool()  -- 모두 충돌했는지 확인
            return
        end
    end
end

function CheckCanReturnToPool()
    -- 죽지 않았거나, 죽음 애니메이션 중이 아니면 반환 불가
    if not obj.IsDead or not obj.IsDying then return end

    -- 아직 추적 중인 투사체가 있으면 대기
    if #obj.TargetingProjectiles > 0 then return end

    -- 지면에서 충분히 멀어지면 반환
    if obj.Location.z < (obj.GroundZ - 200.0) then
        ReturnToPool()
    end
end
```

#### 재배치 시스템 (너무 멀어지면)
```lua
-- 플레이어와 너무 멀어지면 플레이어 근처로 텔레포트
if CurrentPlayerDis > obj.MaxDistanceBeforeRelocate then
    -- 플레이어 주변 랜덤 위치
    local RandomAngle = math.rad(Random(0, 360))
    local RandomDist = Random(obj.RelocateRadius, obj.RelocateRadius * 1.2)

    local OffsetX = math.cos(RandomAngle) * RandomDist
    local OffsetY = math.sin(RandomAngle) * RandomDist

    obj.Location = Vector(PlayerPos.x + OffsetX, PlayerPos.y + OffsetY, obj.GroundZ)
    obj.HP = obj.MaxHP  -- HP 회복
end
```

### 3. EnemySpawner 시스템

```lua
-- EnemySpawner.lua
local SpawnInterval = 2.0
local SpawnRadius = 80.0
local MinSpawnRadius = 70.0

function Tick(dt)
    -- 게임 플레이 중에만 스폰
    if _G.GameData.GameState ~= EGameState.Playing then
        return
    end

    SpawnTimer = SpawnTimer + dt

    if SpawnTimer >= SpawnInterval then
        SpawnFromPool()
        SpawnTimer = SpawnTimer - SpawnInterval
    end
end

function SpawnFromPool()
    local PlayerPos = _G.PlayerData.PlayerPos

    -- 플레이어 주변 원형 범위에 랜덤 스폰
    local Angle = Random(0, math.pi * 2)
    local Distance = Random(MinSpawnRadius, SpawnRadius)

    local SpawnX = PlayerPos.x + math.cos(Angle) * Distance
    local SpawnY = PlayerPos.y + math.sin(Angle) * Distance

    local SpawnedActor = ActorPool:Get("AEnemy")
    SpawnedActor.Location = Vector(SpawnX, SpawnY, PlayerPos.z)

    -- 점점 빨라지는 스폰
    SpawnInterval = SpawnInterval - 0.03
    SpawnInterval = math.max(SpawnInterval, 1.4)  -- 최소 1.4초
end
```

### 4. UI 시스템

#### 데미지 텍스트 (떠오르는 숫자)
```lua
-- Util.lua
function Util.MakeDamageText(InDamage, InWorldPos, InColor)
    local DamageText = {
        Damage = InDamage,
        WorldPos = InWorldPos + Vector(2,0,0),
        LifeTime = 1.0,
        Color = InColor
    }
    table.insert(_G.UIData.DamageTextList, DamageText)
end

-- UIManager.lua
function DamageTextUIUpdate(dt)
    for i = #_G.UIData.DamageTextList, 1, -1 do
        local Text = _G.UIData.DamageTextList[i]
        Text.LifeTime = Text.LifeTime - dt

        if Text.LifeTime <= 0 then
            table.remove(_G.UIData.DamageTextList, i)
        else
            -- 월드 좌표를 스크린 좌표로 변환
            DrawText(
                tostring(Text.Damage),
                WorldToScreenPos(Text.WorldPos),
                Vector2(80,30),
                30,
                Vector4(Text.Color.x, Text.Color.y, Text.Color.z, Text.LifeTime)
            )

            -- 오른쪽으로 이동
            Text.WorldPos = Text.WorldPos + Vector(1 * dt, 0, 0)
        end
    end
end
```

#### HP 바 렌더링
```lua
-- Util.lua
function Util.RenderHPBar(WorldPos, Size, HPPer)
    DrawGaugeBar(
        WorldToScreenPos(WorldPos + Vector(1,0,0)),
        Size,
        HPPer,
        Vector4(0.2,0.2,0.2,1.0),  -- 배경색 (어두운 회색)
        Vector4(1.0, 0.2, 0.2, 1.0) -- 게이지색 (빨간색)
    )
end

-- Player.lua / Enemy.lua
function Tick(dt)
    local HPPer = obj.HP / obj.MaxHP
    Util.RenderHPBar(obj.Location, Vector2(70, 20), HPPer)
end
```

#### 게임 상태별 UI
```lua
-- GameManager.lua
function Tick(dt)
    local ScreenCenter = GetViewportCenter()

    if _G.GameData.GameState == EGameState.Lobby then
        DrawText("To Start Press W", ScreenCenter, Vector2(500,100), 50, Vector4(0.5,1,1,1))

    elseif _G.GameData.GameState == EGameState.Loading then
        DrawText(LoadingText, ScreenCenter, Vector2(200,200), 70, Vector4(0.5,1,1,1))

    elseif _G.GameData.GameState == EGameState.Playing then
        -- 점수
        DrawText("Score " .. tostring(_G.GameData.Score),
                 Vector2(100,100), Vector2(400,100), 40, Vector4(0,1,0,1))

        -- 레벨 & 경험치 게이지
        DrawText(GetLevelText(), Vector2(ScreenCenter.x - 430, 100),
                 Vector2(200,70), 40, Vector4(0,1,0,1))
        DrawGaugeBar(Vector2(ScreenCenter.x, 100), Vector2(700, 40),
                     GetCurEXPPer(),
                     Vector4(0.2,0.2,0.2,1.0),
                     Vector4(0.0, 1.0, 0.0, 1.0))

    elseif _G.GameData.GameState == EGameState.End then
        DrawText("Restart Press W", ScreenCenter, Vector2(500,100), 50, Vector4(0.5,1,1,1))
    end
end
```

### 5. 조명 시스템

플레이어를 부드럽게 따라다니는 3색 조명:

```lua
-- GameManager.lua
function LightMove(dt)
    -- 플레이어 위치를 향해 Lerp
    local LightToPlayer = _G.PlayerData.PlayerPos - CurLightPos
    LightToPlayer.z = 0
    CurLightPos = CurLightPos + LightToPlayer * dt * 1.3

    SetLightPos()
end

function SetLightPos()
    -- 각 조명을 오프셋만큼 이동
    BlueLight.Location = CurLightPos + BlueLightOffset
    PinkLight.Location = CurLightPos + PinkLightOffset
    YellowLight.Location = CurLightPos + YellowLightOffset
end
```

---

## 정리 및 핵심 특징

### 아키텍처 설계 원칙

1. **C++ 엔진 + Lua 게임 로직 분리**
   - C++: 충돌 감지, 렌더링, 입력 처리
   - Lua: 게임 로직, AI, 상태 머신

2. **타입 안전 델리게이트 시스템**
   - 멀티캐스트 지원
   - 약한 참조로 안전성 보장
   - Lua 함수 바인딩 가능

3. **Environment Chain 격리**
   - 스크립트 인스턴스 간 변수 충돌 방지
   - 전역 API 공유
   - 핫 리로드 지원

4. **오브젝트 풀링**
   - 메모리 할당 최소화
   - 생성/삭제 오버헤드 제거
   - 수백 개의 적/투사체 동시 관리

5. **Duck Typing으로 타입 판별**
   - 런타임 함수 존재 여부로 타입 확인
   - 유연한 충돌 처리
   - 엔진 수정 없이 새로운 타입 추가 가능

### 성능 최적화

- **함수 캐싱**: BeginPlay, Tick 등 자주 호출되는 함수 미리 컴파일
- **ActorPool**: 적/투사체 재사용으로 할당 오버헤드 제거
- **약한 참조 델리게이트**: 수동 언바인딩 불필요
- **Z축 분리 거리 계산**: 탑다운 게임에서 Z축 무시로 계산 최적화

### 개발자 경험 (DX)

- **핫 리로드**: 게임 재시작 없이 스크립트 수정 반영
- **Lua 디버깅**: protected_function으로 에러 메시지 출력
- **코루틴**: 비동기 로직을 동기 코드처럼 작성
- **전역 테이블**: 모듈 간 데이터 공유 용이

---

## 결론

Last Roll은 FutureEngine의 강력한 아키텍처를 활용하여 **완전히 Lua로 구현된 게임 로직**을 가진 탑다운 슈팅 게임입니다.

**핵심 기술:**
- 타입 안전 멀티캐스트 델리게이트 시스템
- Sol2 기반 Lua 바인딩 및 Environment Chain
- "obj" 프록시 테이블을 통한 이중 모드 속성 접근
- 오브젝트 풀링을 통한 메모리 최적화
- Duck Typing 기반 충돌 처리

이 시스템은 **C++의 성능**과 **Lua의 유연성**을 완벽하게 결합하여, 빠른 프로토타이핑과 안정적인 실행을 동시에 달성합니다.
