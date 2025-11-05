# Camera Shake Preset + Trigger System 구현 계획

## 프로젝트 개요

**목표:** FutureEngine에 Unreal Engine 스타일의 재사용 가능한 Camera Shake 시스템 구축

**철학:**
- Designer-friendly (코드 없이 설정 가능)
- 재사용성 (Preset 기반)
- 분리된 관심사 (Shake 설정 ↔ 트리거 로직)
- 시각적 편집 환경

**작성일:** 2025-01-XX
**버전:** 1.0
**상태:** Planning

---

## 시스템 아키텍처

```
┌─────────────────────────────────────────────────────────┐
│                  Camera Shake System                     │
├─────────────────────────────────────────────────────────┤
│                                                           │
│  ┌──────────────────┐      ┌────────────────────────┐  │
│  │ Preset Data      │──────│ Preset Manager         │  │
│  │ (JSON Files)     │      │ (Singleton)            │  │
│  └──────────────────┘      └────────────────────────┘  │
│           │                          │                   │
│           │                          ▼                   │
│           │              ┌────────────────────────┐     │
│           │              │ PlayerCameraManager    │     │
│           │              │ ::PlayCameraShake      │     │
│           │              │   Preset()             │     │
│           │              └────────────────────────┘     │
│           │                          ▲                   │
│           │                          │                   │
│           │              ┌────────────────────────┐     │
│           └──────────────│ Preset Editor Window   │     │
│                          │ (ImGui UI)             │     │
│                          └────────────────────────┘     │
│                                      ▲                   │
│                                      │                   │
│                          ┌────────────────────────┐     │
│                          │ CameraShakeTrigger     │     │
│                          │ Component              │     │
│                          └────────────────────────┘     │
└─────────────────────────────────────────────────────────┘
```

---

## 파일 구조

```
Engine/Source/
├── Global/Public/
│   └── CameraShakeTypes.h              (NEW) - Preset 데이터 구조체
│
├── Manager/Camera/
│   ├── Public/
│   │   └── CameraShakePresetManager.h  (NEW) - Preset 관리 싱글톤
│   └── Private/
│       └── CameraShakePresetManager.cpp (NEW)
│
├── Component/Camera/
│   ├── Public/
│   │   └── CameraShakeTriggerComponent.h (NEW) - 트리거 컴포넌트
│   └── Private/
│       └── CameraShakeTriggerComponent.cpp (NEW)
│
├── Actor/Public/
│   └── PlayerCameraManager.h           (MODIFY) - PlayCameraShakePreset() 추가
│
├── Editor/Public/
│   └── CameraShakePresetDetailPanel.h  (NEW) - Preset 편집 패널
│
└── Render/UI/Window/
    ├── Public/
    │   └── CameraShakePresetEditorWindow.h (NEW) - Preset 에디터 윈도우
    └── Private/
        └── CameraShakePresetEditorWindow.cpp (NEW)

Engine/Data/
└── CameraShakePresets.json             (NEW) - Preset 저장 파일
```

---

## 구현 단계

### Phase 1: Preset 데이터 구조 & Manager (기반)
**예상 시간:** 1-2시간
**우선순위:** 🔴 High

#### 1.1 FCameraShakePresetData 구조체 생성
**파일:** `Engine/Source/Global/Public/CameraShakeTypes.h`

**작업:**
- [ ] FCameraShakePresetData 구조체 정의
  - FName PresetName
  - float Duration, LocationAmplitude, RotationAmplitude
  - ECameraShakePattern Pattern
  - float Frequency
  - bool bUseDecayCurve
  - FCubicBezierCurve DecayCurve
- [ ] Serialize() 메서드 구현 (JSON 직렬화)
- [ ] 기본 생성자 및 초기값 설정

**구현 예시:**
```cpp
struct FCameraShakePresetData
{
    FName PresetName;
    float Duration = 1.0f;
    float LocationAmplitude = 5.0f;
    float RotationAmplitude = 2.0f;
    ECameraShakePattern Pattern = ECameraShakePattern::Perlin;
    float Frequency = 10.0f;
    bool bUseDecayCurve = false;
    FCubicBezierCurve DecayCurve;

    void Serialize(bool bIsLoading, JSON& InOutHandle);
};
```

#### 1.2 UCameraShakePresetManager 싱글톤 생성
**파일:** `Engine/Source/Manager/Camera/Public/CameraShakePresetManager.h`

**작업:**
- [ ] DECLARE_SINGLETON_CLASS 매크로 적용
- [ ] TMap<FName, FCameraShakePresetData> Presets 멤버
- [ ] API 구현:
  - [ ] LoadPresetsFromFile(const FString& FilePath)
  - [ ] SavePresetsToFile(const FString& FilePath)
  - [ ] AddPreset(const FCameraShakePresetData& Preset)
  - [ ] RemovePreset(FName PresetName)
  - [ ] FCameraShakePresetData* FindPreset(FName PresetName)
  - [ ] TArray<FName> GetAllPresetNames()
- [ ] 기본 Preset 초기화 (생성자에서 "Default" Preset 추가)

**API 사용 예시:**
```cpp
// Preset 추가
UCameraShakePresetManager& Manager = UCameraShakePresetManager::GetInstance();
FCameraShakePresetData ExplosionPreset;
ExplosionPreset.PresetName = FName("Explosion");
ExplosionPreset.Duration = 2.0f;
ExplosionPreset.LocationAmplitude = 30.0f;
Manager.AddPreset(ExplosionPreset);

// Preset 저장
Manager.SavePresetsToFile("Engine/Data/CameraShakePresets.json");

// Preset 로드
Manager.LoadPresetsFromFile("Engine/Data/CameraShakePresets.json");

// Preset 찾기
FCameraShakePresetData* Preset = Manager.FindPreset(FName("Explosion"));
```

---

### Phase 2: PlayerCameraManager 통합 (핵심 실행)
**예상 시간:** 30분 - 1시간
**우선순위:** 🔴 High

#### 2.1 PlayCameraShakePreset() 메서드 추가
**파일:** `Engine/Source/Actor/Public/PlayerCameraManager.h` (MODIFY)

**작업:**
- [ ] 헤더에 함수 선언 추가
- [ ] .cpp에 구현

**선언:**
```cpp
/**
 * @brief Preset 이름으로 Camera Shake 실행
 *
 * PresetManager에서 지정된 이름의 Preset을 찾아서 Camera Shake를 시작합니다.
 * Preset이 존재하지 않거나 CameraShake Modifier가 없으면 실패합니다.
 *
 * @param PresetName 실행할 Preset 이름 (예: "Explosion", "Collision")
 * @return true면 Shake 시작 성공, false면 실패
 */
bool PlayCameraShakePreset(FName PresetName);
```

**구현 로직:**
1. CameraShake Modifier 찾기 (`FindCameraModifierByClass`)
2. PresetManager에서 Preset 로드 (`FindPreset`)
3. Preset 없으면 UE_LOG_ERROR 반환
4. `bUseDecayCurve` 체크
5. `StartShakeWithCurve()` 또는 `StartShake()` 호출

**사용 예시:**
```cpp
// 코드에서 직접 호출
APlayerCameraManager* CamMgr = GetPlayerCameraManager();
if (CamMgr)
{
    CamMgr->PlayCameraShakePreset(FName("Explosion"));
}

// 충돌 이벤트에서 호출
void AExplosiveBarrel::OnDestroyed(AActor* DestroyedActor)
{
    APlayerCameraManager* CamMgr = GetWorld()->GetPlayerCameraManager();
    if (CamMgr)
    {
        CamMgr->PlayCameraShakePreset(FName("Explosion"));
    }
}
```

---

### Phase 3: Preset Editor UI (가장 중요!)
**예상 시간:** 3-4시간
**우선순위:** 🔴 High

#### 3.1 FCameraShakePresetDetailPanel 생성
**파일:** `Engine/Source/Editor/Public/CameraShakePresetDetailPanel.h`

**작업:**
- [ ] 기존 FCameraShakeDetailPanel 복사하여 수정
- [ ] Preset 이름 편집 필드 추가
- [ ] Preset 데이터 직접 편집 (FCameraShakePresetData&)
- [ ] Test 버튼 추가 (PIE 모드 체크)

**UI 구성:**
```cpp
class FCameraShakePresetDetailPanel
{
public:
    /**
     * @brief Preset Detail Panel UI 렌더링
     *
     * @param Label 패널 고유 ID (ImGui ID로 사용)
     * @param Preset 편집할 Preset 데이터 (참조)
     * @param World 현재 World (PIE 모드 확인용)
     * @return true면 파라미터가 변경됨
     */
    bool Draw(const char* Label, FCameraShakePresetData& Preset, UWorld* World);

private:
    void DrawPresetName(FCameraShakePresetData& Preset);
    void DrawTestButton(FCameraShakePresetData& Preset, UWorld* World);

    // 기존 FCameraShakeDetailPanel 메서드 재사용
    FImGuiBezierEditor BezierEditor;
};
```

**패널 섹션:**
1. Preset Name (텍스트 입력)
2. Use Bezier Curve Decay (체크박스)
3. Bezier Curve Editor (기존 재사용)
4. Preset Buttons (Linear, EaseIn, EaseOut, EaseInOut, Bounce)
5. Curve Preview (실시간 그래프)
6. Shake Parameters (Duration, Amplitude, Pattern, Frequency)
7. **Test Shake 버튼** (PIE 모드에서만 활성화)

#### 3.2 UCameraShakePresetEditorWindow 생성
**파일:** `Engine/Source/Render/UI/Window/Public/CameraShakePresetEditorWindow.h`

**작업:**
- [ ] UUIWindow 상속
- [ ] 3-Panel 레이아웃 구현
- [ ] Toolbar 버튼 핸들러

**Window 레이아웃:**
```
┌──────────────────────────────────────────┐
│  [New] [Delete] [Save]  [Import] [Export]│  ← Toolbar
├───────────────┬──────────────────────────┤
│ Preset List   │  Preset Detail           │
│               │                          │
│ ▶ Explosion   │  Name: [Explosion____]   │
│   Collision   │  [Use Bezier Curve]      │
│   Earthquake  │  [Bezier Curve Editor]   │
│   Footstep    │  Duration: [2.0]         │
│               │  Location Amp: [30.0]    │
│               │  ...                     │
│               │  [Test Shake] (PIE only) │
└───────────────┴──────────────────────────┘
```

**클래스 구조:**
```cpp
class UCameraShakePresetEditorWindow : public UUIWindow
{
    DECLARE_CLASS(UCameraShakePresetEditorWindow, UUIWindow)

public:
    void Initialize() override;
    void OnPreRenderWindow(float MenuBarOffset) override;

private:
    void DrawToolbar();      // New/Delete/Save/Import/Export 버튼
    void DrawPresetList();   // 왼쪽 Preset 목록
    void DrawPresetDetail(); // 오른쪽 Detail Panel

    void OnNewPreset();      // New 버튼 핸들러
    void OnDeletePreset();   // Delete 버튼 핸들러
    void OnSave();           // Save 버튼 핸들러
    void OnImport();         // Import 버튼 핸들러
    void OnExport();         // Export 버튼 핸들러

    void RefreshPresetList(); // PresetManager에서 목록 갱신

private:
    FCameraShakePresetDetailPanel DetailPanel;
    TArray<FName> PresetNames;
    int32 SelectedPresetIndex = -1;
    FCameraShakePresetData* CurrentPreset = nullptr;
};
```

#### 3.3 UI Factory 통합
**파일:** `Engine/Source/Render/UI/Factory/Public/UIWindowFactory.h`

**작업:**
- [ ] CreateCameraShakePresetEditorWindow() 함수 추가
- [ ] MainMenuWindow에 메뉴 항목 추가

**메뉴 경로:** `Window > Camera Shake Preset Editor`

---

### Phase 4: Trigger Component (실전 사용)
**예상 시간:** 2-3시간
**우선순위:** 🟡 Medium

#### 4.1 ECameraShakeTriggerType Enum
**파일:** `Engine/Source/Global/Public/CameraShakeTypes.h` (추가)

```cpp
UENUM()
enum class ECameraShakeTriggerType : uint8
{
    OnActorHit,      // Actor 충돌 시
    OnComponentHit,  // Component 충돌 시
    OnTakeDamage,    // 데미지 받을 때
    OnDestroyed,     // 파괴될 때
    Manual,          // 수동 호출 (버튼/코드)
    Timer,           // 주기적으로
    End
};
DECLARE_UINT8_ENUM_REFLECTION(ECameraShakeTriggerType)
```

#### 4.2 UCameraShakeTriggerComponent 생성
**파일:** `Engine/Source/Component/Camera/Public/CameraShakeTriggerComponent.h`

**작업:**
- [ ] UActorComponent 상속
- [ ] TriggerType별 이벤트 바인딩
- [ ] TickComponent (Timer 타입용)
- [ ] TriggerCameraShake() 메서드
- [ ] Serialize() 구현

**클래스 구조:**
```cpp
UCLASS()
class UCameraShakeTriggerComponent : public UActorComponent
{
    DECLARE_CLASS(UCameraShakeTriggerComponent, UActorComponent)

public:
    virtual void BeginPlay() override;
    virtual void EndPlay() override;
    virtual void TickComponent(float DeltaTime) override;
    virtual void Serialize(bool bInIsLoading, JSON& InOutHandle) override;

    // 수동 트리거
    void TriggerCameraShake();

private:
    // 이벤트 핸들러
    void OnActorHitHandler(AActor* SelfActor, AActor* OtherActor,
                           FVector NormalImpulse, const FHitResult& Hit);
    void OnTakeDamageHandler(AActor* DamagedActor, float Damage,
                             const UDamageType* DamageType,
                             AController* InstigatedBy, AActor* DamageCauser);
    void TimerTick(float DeltaTime);

public:
    // 설정 (Editor에서 편집 가능)
    ECameraShakeTriggerType TriggerType = ECameraShakeTriggerType::Manual;
    FName ShakePresetName;           // "Explosion"

    // Timer 설정
    float TimerInterval = 1.0f;      // Timer 타입일 때만 사용

    // 조건
    float MinImpulseMagnitude = 0.0f; // 최소 충격력 (Collision 타입)
    float MinDamage = 0.0f;           // 최소 데미지 (Damage 타입)

private:
    float TimerAccumulator = 0.0f;
};
```

**BeginPlay() 로직:**
```cpp
void UCameraShakeTriggerComponent::BeginPlay()
{
    Super::BeginPlay();

    AActor* Owner = GetOwner();
    if (!Owner) return;

    switch (TriggerType)
    {
    case ECameraShakeTriggerType::OnActorHit:
        Owner->OnActorHit.AddDynamic(this, &UCameraShakeTriggerComponent::OnActorHitHandler);
        break;

    case ECameraShakeTriggerType::OnTakeDamage:
        Owner->OnTakeAnyDamage.AddDynamic(this, &UCameraShakeTriggerComponent::OnTakeDamageHandler);
        break;

    case ECameraShakeTriggerType::Timer:
        bCanTick = true;
        break;

    // ... 기타 타입
    }
}
```

**TriggerCameraShake() 로직:**
```cpp
void UCameraShakeTriggerComponent::TriggerCameraShake()
{
    // PlayerCameraManager 찾기
    UWorld* World = GetWorld();
    if (!World) return;

    APlayerCameraManager* CamMgr = World->GetPlayerCameraManager();
    if (!CamMgr) return;

    // Preset 실행
    if (CamMgr->PlayCameraShakePreset(ShakePresetName))
    {
        UE_LOG("CameraShakeTrigger: Shake triggered (%s)",
               ShakePresetName.ToString().c_str());
    }
    else
    {
        UE_LOG_ERROR("CameraShakeTrigger: Failed to trigger shake (%s)",
                     ShakePresetName.ToString().c_str());
    }
}
```

#### 4.3 Detail Panel 통합
**파일:** `Engine/Source/Render/UI/Window/Private/DetailWindow.cpp` (MODIFY)

**작업:**
- [ ] UCameraShakeTriggerComponent 감지
- [ ] Component 설정 UI 렌더링

**Detail Panel UI:**
```cpp
if (UCameraShakeTriggerComponent* TriggerComp = Cast<UCameraShakeTriggerComponent>(Component))
{
    ImGui::Text("Camera Shake Trigger");
    ImGui::Separator();

    // TriggerType Combo Box
    const char* TriggerTypes[] = { "OnActorHit", "OnComponentHit", "OnTakeDamage",
                                    "OnDestroyed", "Manual", "Timer" };
    int32 CurrentType = static_cast<int32>(TriggerComp->TriggerType);
    if (ImGui::Combo("Trigger Type", &CurrentType, TriggerTypes, 6))
    {
        TriggerComp->TriggerType = static_cast<ECameraShakeTriggerType>(CurrentType);
    }

    // PresetName Combo Box
    UCameraShakePresetManager& PresetMgr = UCameraShakePresetManager::GetInstance();
    TArray<FName> PresetNames = PresetMgr.GetAllPresetNames();
    // ... Combo Box 렌더링

    // 조건 파라미터
    if (TriggerComp->TriggerType == ECameraShakeTriggerType::OnActorHit)
    {
        ImGui::DragFloat("Min Impulse Magnitude", &TriggerComp->MinImpulseMagnitude,
                        0.5f, 0.0f, 10000.0f);
    }

    if (TriggerComp->TriggerType == ECameraShakeTriggerType::Timer)
    {
        ImGui::DragFloat("Timer Interval", &TriggerComp->TimerInterval,
                        0.1f, 0.1f, 60.0f);
    }

    // Manual 타입일 때 Trigger 버튼
    if (TriggerComp->TriggerType == ECameraShakeTriggerType::Manual)
    {
        if (ImGui::Button("Trigger Shake"))
        {
            TriggerComp->TriggerCameraShake();
        }
    }
}
```

---

### Phase 5: 기본 Preset 생성 & 테스트
**예상 시간:** 1-2시간
**우선순위:** 🟢 Low

#### 5.1 기본 Preset 데이터 작성

**PresetManager 생성자에서 추가:**
```cpp
UCameraShakePresetManager::UCameraShakePresetManager()
{
    // Explosion Preset
    FCameraShakePresetData Explosion;
    Explosion.PresetName = FName("Explosion");
    Explosion.Duration = 2.0f;
    Explosion.LocationAmplitude = 30.0f;
    Explosion.RotationAmplitude = 8.0f;
    Explosion.Pattern = ECameraShakePattern::Perlin;
    Explosion.bUseDecayCurve = true;
    Explosion.DecayCurve = FCubicBezierCurve::CreateEaseOut();
    AddPreset(Explosion);

    // Collision Preset
    FCameraShakePresetData Collision;
    Collision.PresetName = FName("Collision");
    Collision.Duration = 0.5f;
    Collision.LocationAmplitude = 10.0f;
    Collision.RotationAmplitude = 3.0f;
    Collision.Pattern = ECameraShakePattern::Random;
    Collision.bUseDecayCurve = true;
    Collision.DecayCurve = FCubicBezierCurve::CreateLinear();
    AddPreset(Collision);

    // Earthquake Preset
    FCameraShakePresetData Earthquake;
    Earthquake.PresetName = FName("Earthquake");
    Earthquake.Duration = 5.0f;
    Earthquake.LocationAmplitude = 50.0f;
    Earthquake.RotationAmplitude = 5.0f;
    Earthquake.Pattern = ECameraShakePattern::Sine;
    Earthquake.Frequency = 5.0f;
    Earthquake.bUseDecayCurve = true;
    Earthquake.DecayCurve = FCubicBezierCurve::CreateEaseInOut();
    AddPreset(Earthquake);
}
```

#### 5.2 통합 테스트 시나리오

**테스트 1: Preset Editor UI**
- [ ] Window > Camera Shake Preset Editor 열기
- [ ] 기본 Preset 목록 확인 (Explosion, Collision, Earthquake)
- [ ] New 버튼으로 "TestShake" Preset 생성
- [ ] 파라미터 편집 (Duration: 1.5, LocationAmplitude: 20.0)
- [ ] Bezier Curve Editor로 곡선 조정
- [ ] PIE 모드 진입
- [ ] Test Shake 버튼 클릭 → 흔들림 확인
- [ ] Save 버튼 클릭
- [ ] `Engine/Data/CameraShakePresets.json` 파일 확인

**테스트 2: Manual Trigger**
- [ ] 레벨에 Cube Actor 추가
- [ ] CameraShakeTriggerComponent 추가 (Add Component)
- [ ] Detail Panel에서 설정:
  - TriggerType: Manual
  - PresetName: "Explosion"
- [ ] "Trigger Shake" 버튼 클릭
- [ ] 흔들림 확인

**테스트 3: Collision Trigger**
- [ ] Physics 활성화된 FallingBox Actor 생성
- [ ] CameraShakeTriggerComponent 추가
- [ ] 설정:
  - TriggerType: OnActorHit
  - PresetName: "Collision"
  - MinImpulseMagnitude: 100.0
- [ ] Ground Actor 추가
- [ ] PIE 실행
- [ ] FallingBox가 Ground에 충돌 시 흔들림 자동 실행 확인

**테스트 4: Timer Trigger**
- [ ] Actor에 CameraShakeTriggerComponent 추가
- [ ] 설정:
  - TriggerType: Timer
  - PresetName: "Earthquake"
  - TimerInterval: 3.0
- [ ] PIE 실행
- [ ] 3초마다 흔들림 확인 (최소 3회 관찰)

**테스트 5: 여러 Preset 동시 사용**
- [ ] 여러 Actor에 다른 Preset 할당
- [ ] 동시 트리거 시 Shake 중첩 동작 확인
- [ ] 성능 확인 (프레임 드롭 없음)

**테스트 6: Preset 저장/로드**
- [ ] Preset Editor에서 새 Preset 생성
- [ ] Save 버튼
- [ ] Engine 재시작
- [ ] Preset Editor 열기
- [ ] 저장한 Preset 로드 확인

---

## 예상 작업 시간

| Phase | 작업 내용 | 예상 시간 | 우선순위 |
|-------|----------|----------|----------|
| **Phase 1** | Preset Data & Manager | 1-2시간 | 🔴 High |
| **Phase 2** | PlayerCameraManager 통합 | 0.5-1시간 | 🔴 High |
| **Phase 3** | Preset Editor UI | 3-4시간 | 🔴 High |
| **Phase 4** | Trigger Component | 2-3시간 | 🟡 Medium |
| **Phase 5** | 기본 Preset & 테스트 | 1-2시간 | 🟢 Low |
| **합계** | | **8-12시간** | |

---

## 마일스톤

### Milestone 1: 기본 Preset 시스템 (Phase 1-2 완료)
**완료 기준:**
- ✅ Preset JSON 저장/로드 가능
- ✅ PresetManager API 동작
- ✅ PlayerCameraManager에서 Preset 실행 가능

**결과:** Console에서 `PlayCameraShakePreset("Explosion")` 호출하면 흔들림 동작

**데모:**
```cpp
APlayerCameraManager* CamMgr = GetPlayerCameraManager();
CamMgr->PlayCameraShakePreset(FName("Explosion"));
```

### Milestone 2: Editor UI 완성 (Phase 3 완료)
**완료 기준:**
- ✅ Preset Editor Window 열기 가능
- ✅ Preset 생성/편집/삭제 가능
- ✅ Bezier Curve Editor 동작
- ✅ Test Shake 버튼으로 즉시 테스트 가능

**결과:** Designer가 코드 없이 Preset 생성 및 테스트 가능

**워크플로우:**
1. Window > Camera Shake Preset Editor
2. New → "TestShake" 입력
3. 파라미터 조정
4. PIE 모드 → Test Shake
5. Save

### Milestone 3: Trigger 시스템 완성 (Phase 4 완료)
**완료 기준:**
- ✅ CameraShakeTriggerComponent 동작
- ✅ Detail Panel에서 설정 가능
- ✅ Collision/Damage/Timer 트리거 동작

**결과:** Actor에 Component 추가만으로 이벤트 기반 Shake 가능

**사용 예:**
- Explosion Barrel → OnDestroyed → "Explosion" Preset
- Vehicle → OnActorHit → "Collision" Preset
- Environment → Timer (5초) → "Earthquake" Preset

### Milestone 4: 프로덕션 레디 (Phase 5 완료)
**완료 기준:**
- ✅ 기본 Preset 제공 (Explosion, Collision, Earthquake)
- ✅ 모든 테스트 시나리오 통과
- ✅ 문서화 완료 (CLAUDE.md 업데이트)

**결과:** 실제 게임 개발에 즉시 사용 가능

---

## 테스트 계획

### 단위 테스트
```cpp
// PresetManager 테스트
void TestPresetManager()
{
    UCameraShakePresetManager& Manager = UCameraShakePresetManager::GetInstance();

    // AddPreset
    FCameraShakePresetData TestPreset;
    TestPreset.PresetName = FName("TestPreset");
    Manager.AddPreset(TestPreset);

    // GetAllPresetNames
    TArray<FName> Names = Manager.GetAllPresetNames();
    assert(Names.Contains(FName("TestPreset")));

    // FindPreset
    FCameraShakePresetData* Found = Manager.FindPreset(FName("TestPreset"));
    assert(Found != nullptr);
    assert(Found->PresetName == FName("TestPreset"));

    // RemovePreset
    Manager.RemovePreset(FName("TestPreset"));
    Found = Manager.FindPreset(FName("TestPreset"));
    assert(Found == nullptr);
}

// SavePresetsToFile / LoadPresetsFromFile
void TestSerialization()
{
    UCameraShakePresetManager& Manager = UCameraShakePresetManager::GetInstance();

    // 저장
    Manager.SavePresetsToFile("Engine/Data/Test_CameraShakePresets.json");

    // 로드
    Manager.LoadPresetsFromFile("Engine/Data/Test_CameraShakePresets.json");

    // 검증
    TArray<FName> Names = Manager.GetAllPresetNames();
    // ... 검증 로직
}
```

### 통합 테스트
**End-to-End 워크플로우:**
1. Editor에서 "TestShake" Preset 생성
2. 파라미터 편집 (Duration: 2.0, Amplitude: 20.0)
3. EaseOut 곡선 적용
4. Test Shake 버튼 클릭 → 흔들림 확인
5. Save → `CameraShakePresets.json` 생성
6. Engine 재시작
7. Preset Editor 열기 → "TestShake" 로드 확인
8. Actor에 Trigger Component 추가
9. "TestShake" Preset 선택
10. PIE 실행 → 트리거 동작 확인

### 성능 테스트
- [ ] 100개 Preset 로드 시간 측정 (< 100ms 목표)
- [ ] Preset Editor UI 프레임 드롭 확인 (60fps 유지)
- [ ] 동시에 10개 Shake 실행 시 성능 (프레임 드롭 < 5%)

---

## Unreal Engine과의 비교

### 유사점
| 항목 | Unreal Engine | FutureEngine (제안) |
|------|---------------|---------------------|
| **재사용성** | UCameraShake Asset | JSON Preset |
| **참조 방식** | TSubclassOf<UCameraShake> | FName PresetName |
| **실행 함수** | StartCameraShake(Class) | PlayCameraShakePreset(Name) |
| **트리거** | Blueprint 이벤트 | CameraShakeTriggerComponent |
| **편집 UI** | Blueprint Editor | Preset Editor Window |
| **중앙 관리** | PlayerCameraManager | PlayerCameraManager |

### 차이점
| 항목 | Unreal Engine | FutureEngine (제안) | 이유 |
|------|---------------|---------------------|------|
| **저장 형식** | .uasset (UObject) | .json | Asset 시스템 없음 |
| **참조 타입** | TSubclassOf | FName | 간소화된 RTTI |
| **편집 환경** | Blueprint Editor | ImGui Window | Blueprint 없음 |
| **Curve 편집** | Curve Asset (별도) | 내장 Bezier Editor | 더 편리함 |

---

## 알려진 제약사항 & 향후 개선

### 현재 제약사항
1. **Blueprint 없음** - C++ Component만 사용 가능 (FutureEngine 특성)
2. **Asset 시스템 없음** - JSON 파일로 대체
3. **Hot Reload 없음** - Preset 변경 시 재시작 필요 (향후 개선 가능)
4. **AnimSequence 지원 없음** - UE의 SequenceCameraShake 미지원

### 향후 개선 사항
- [ ] **Preset Hot Reload** - 파일 감시 + 자동 리로드
- [ ] **Preset Import/Export** - 개별 Preset .json 파일
- [ ] **Advanced Patterns** - Noise, Wave, Animation Curve
- [ ] **Preset Preview Animation** - Test 전 시각화 (그래프 애니메이션)
- [ ] **Multi-Camera Shake** - 여러 카메라 동시 흔들림
- [ ] **Shake Blending** - 여러 Shake의 가중치 블렌딩
- [ ] **Distance Attenuation** - 거리에 따른 Shake 강도 감쇠

---

## 참고 자료

### Unreal Engine Documentation
- [Camera Shake Documentation](https://docs.unrealengine.com/en-US/camera-shake/)
- [Matinee Camera Shake](https://docs.unrealengine.com/4.27/en-US/AnimatingObjects/Matinee/UserGuide/CameraShake/)
- [UCameraShakeBase API](https://docs.unrealengine.com/5.0/en-US/API/Runtime/Engine/Camera/UCameraShakeBase/)

### FutureEngine Codebase
- `Engine/Source/Component/Camera/Public/CameraModifier_CameraShake.h` - 기존 Shake 구현
- `Engine/Source/Editor/Public/CameraShakeDetailPanel.h` - 기존 Detail Panel (재사용 가능)
- `Engine/Source/Global/Public/BezierCurve.h` - Bezier Curve 시스템
- `Engine/Source/Actor/Public/PlayerCameraManager.h` - Camera Manager

### 관련 문서
- `Document/BezierCurveEditor_CameraShake_Implementation_Plan.md` - Bezier Editor 구현 계획
- `Document/PlayerCameraManager_Implementation_Plan.md` - PlayerCameraManager 설계
- `Document/CameraShake_Usage_Guide.md` - Camera Shake 사용 가이드
- `CLAUDE.md` - 엔진 전체 가이드 (업데이트 필요)

---

## 완료 기준 (Definition of Done)

각 Phase 완료 시 확인 항목:

### 코드 품질
- [ ] 모든 코드가 컴파일 성공
- [ ] 경고(Warning) 없음
- [ ] 모든 public 함수에 Doxygen 주석 작성
- [ ] UE Coding Standard 준수
  - [ ] 탭 인덴트 (spaces 아님)
  - [ ] PascalCase (클래스/함수)
  - [ ] camelCase (변수)
  - [ ] U/A/F/E 접두사 준수

### 기능
- [ ] 모든 API 동작 확인
- [ ] 에러 처리 구현 (nullptr 체크, UE_LOG)
- [ ] 메모리 누수 없음
- [ ] 테스트 시나리오 통과

### 문서화
- [ ] 헤더 파일 주석 작성
- [ ] 사용 예시 코드 작성
- [ ] CLAUDE.md 업데이트 (Phase 3, 4 완료 시)
  - [ ] Camera Shake Preset 시스템 섹션 추가
  - [ ] Preset Editor 사용법
  - [ ] Trigger Component 사용법
  - [ ] 코드 예시

---

## 버전 히스토리

| 버전 | 날짜 | 작성자 | 변경 내용 |
|------|------|--------|----------|
| 1.0 | 2025-01-XX | Claude | 초안 작성 |

---

## 작업 진행 상황

### Phase 1: Preset Data & Manager
- [ ] FCameraShakePresetData 구조체 생성
- [ ] UCameraShakePresetManager 생성
- [ ] Serialize 구현
- [ ] 컴파일 테스트
- [ ] 단위 테스트

### Phase 2: PlayerCameraManager 통합
- [ ] PlayCameraShakePreset() 함수 선언
- [ ] 함수 구현
- [ ] 컴파일 테스트
- [ ] 런타임 테스트

### Phase 3: Preset Editor UI
- [ ] FCameraShakePresetDetailPanel 생성
- [ ] UCameraShakePresetEditorWindow 생성
- [ ] UI Factory 통합
- [ ] MainMenu 통합
- [ ] 컴파일 테스트
- [ ] UI 테스트

### Phase 4: Trigger Component
- [ ] ECameraShakeTriggerType Enum
- [ ] UCameraShakeTriggerComponent 생성
- [ ] Detail Panel 통합
- [ ] 컴파일 테스트
- [ ] 트리거 테스트

### Phase 5: 기본 Preset & 테스트
- [ ] 기본 Preset 작성
- [ ] 통합 테스트 시나리오
- [ ] JSON 파일 검증
- [ ] 문서화

---

## 연락처 & 질문

이 문서에 대한 질문이나 제안 사항이 있으면:
- GitHub Issues
- 프로젝트 Wiki
- 개발팀 채널

---

**문서 끝**
