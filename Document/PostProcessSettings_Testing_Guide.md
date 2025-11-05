# PostProcessSettings Testing Guide

**Phase 4 테스트 가이드**

이 문서는 PostProcessSettings 통합이 완료된 후 검증을 위한 테스트 절차를 안내합니다.

---

## 테스트 환경 설정

### 필수 사항

1. **빌드 완료**: Debug 또는 Develop 모드로 엔진 빌드 완료
2. **콘솔 창 활성화**: 디버그 로그 확인을 위해 콘솔 창이 보이도록 설정
3. **PIE 모드 사용**: Play-In-Editor 모드로 테스트 진행

### 디버그 로그 확인

VignettePass가 실행될 때마다 다음과 같은 로그가 출력됩니다:

```
VignettePass: BlendWeight=1.00, Override_I=1, Override_C=1, Intensity=0.50, Color=(1.00,0.00,0.00)
```

**로그 의미**:
- `BlendWeight`: PostProcessSettings의 전체 가중치
- `Override_I`: VignetteIntensity 오버라이드 활성화 여부 (0=비활성, 1=활성)
- `Override_C`: VignetteColor 오버라이드 활성화 여부
- `Intensity`: 실제 적용되는 비네트 강도
- `Color`: 실제 적용되는 비네트 색상 (R, G, B)

---

## 테스트 1: 기본 동작 확인

**목표**: PostProcessSettings가 카메라에서 렌더러로 정상 전달되는지 확인

### 단계

#### 1. 테스트 액터 생성

레벨에 카메라를 가진 액터를 배치하거나, 코드에서 생성:

```cpp
// BeginPlay() 또는 테스트 코드
AActor* CameraActor = World->SpawnActor(AActor::StaticClass());
UCameraComponent* Camera = NewObject<UCameraComponent>(CameraActor);
CameraActor->SetRootComponent(Camera);
```

#### 2. PostProcessSettings 설정

```cpp
FPostProcessSettings& PPSettings = Camera->GetPostProcessSettings();

// 강한 빨간색 비네트 효과
PPSettings.bOverride_VignetteIntensity = true;
PPSettings.VignetteIntensity = 0.8f;

PPSettings.bOverride_VignetteColor = true;
PPSettings.VignetteColor = FVector(1.0f, 0.0f, 0.0f);  // 빨간색
```

#### 3. CameraManager에 ViewTarget 설정

```cpp
APlayerCameraManager* CameraManager = World->GetCameraManager();
CameraManager->SetViewTarget(CameraActor);
```

#### 4. PIE 실행 및 확인

**예상 결과**:
- ✅ 화면 가장자리가 붉게 어두워짐
- ✅ 콘솔에 `VignettePass: BlendWeight=1.00, Override_I=1, Override_C=1, Intensity=0.80, Color=(1.00,0.00,0.00)` 로그 출력

**문제 발생 시 체크리스트**:
1. CameraManager의 ViewTarget이 올바르게 설정되었는지 확인
2. PIE 모드인지 확인 (Editor 모드는 별도 카메라 사용)
3. 콘솔 로그에서 Override 플래그가 1인지 확인

---

## 테스트 2: Override 플래그 동작

**목표**: Override 플래그가 제대로 작동하는지 확인

### 시나리오 A: Override 비활성화

```cpp
FPostProcessSettings& PPSettings = Camera->GetPostProcessSettings();
PPSettings.bOverride_VignetteIntensity = false;  // 비활성화
PPSettings.VignetteIntensity = 1.0f;  // 이 값은 무시됨
```

**예상 결과**:
- ✅ 비네트 효과 없음
- ✅ 콘솔 로그: `Override_I=0, Intensity=0.00`

### 시나리오 B: 부분 Override

```cpp
// Intensity만 활성화, Color는 비활성화
PPSettings.bOverride_VignetteIntensity = true;
PPSettings.VignetteIntensity = 0.5f;

PPSettings.bOverride_VignetteColor = false;  // 기본 검은색 사용
```

**예상 결과**:
- ✅ 검은색 비네트 효과 (중간 강도)
- ✅ 콘솔 로그: `Override_I=1, Override_C=0, Intensity=0.50, Color=(0.00,0.00,0.00)`

---

## 테스트 3: 카메라 블렌딩 (PostProcessSettings 보간)

**목표**: 두 카메라 간 블렌딩 시 PostProcessSettings도 부드럽게 보간되는지 확인

### 단계

#### 1. Camera A 생성 및 설정

```cpp
AActor* CameraActorA = World->SpawnActor(AActor::StaticClass());
UCameraComponent* CameraA = NewObject<UCameraComponent>(CameraActorA);
CameraActorA->SetRootComponent(CameraA);

// 비네트 효과 없음
CameraA->GetPostProcessSettings().bOverride_VignetteIntensity = true;
CameraA->GetPostProcessSettings().VignetteIntensity = 0.0f;
CameraA->GetPostProcessSettings().bOverride_VignetteColor = true;
CameraA->GetPostProcessSettings().VignetteColor = FVector(1.0f, 0.0f, 0.0f);  // 빨간색
```

#### 2. Camera B 생성 및 설정

```cpp
AActor* CameraActorB = World->SpawnActor(AActor::StaticClass());
UCameraComponent* CameraB = NewObject<UCameraComponent>(CameraActorB);
CameraActorB->SetRootComponent(CameraB);

// 강한 비네트 효과
CameraB->GetPostProcessSettings().bOverride_VignetteIntensity = true;
CameraB->GetPostProcessSettings().VignetteIntensity = 1.0f;
CameraB->GetPostProcessSettings().bOverride_VignetteColor = true;
CameraB->GetPostProcessSettings().VignetteColor = FVector(1.0f, 0.0f, 0.0f);  // 빨간색
```

#### 3. 초기 ViewTarget 설정

```cpp
APlayerCameraManager* CameraManager = World->GetCameraManager();
CameraManager->SetViewTarget(CameraActorA);  // A로 시작
```

#### 4. 블렌딩 시작

```cpp
// 3초에 걸쳐 Camera B로 블렌드
CameraManager->SetViewTarget(CameraActorB, 3.0f);
```

**예상 결과**:
- ✅ 3초 동안 비네트 강도가 0.0 → 1.0으로 점진적으로 증가
- ✅ 화면이 서서히 어두워짐
- ✅ 콘솔 로그에서 Intensity 값이 점진적으로 증가하는 것 확인 (예: 0.00 → 0.33 → 0.67 → 1.00)

### 시나리오 B: 색상 블렌딩

```cpp
// Camera A: 빨간색 비네트
CameraA->GetPostProcessSettings().VignetteColor = FVector(1.0f, 0.0f, 0.0f);

// Camera B: 파란색 비네트
CameraB->GetPostProcessSettings().VignetteColor = FVector(0.0f, 0.0f, 1.0f);
```

**예상 결과**:
- ✅ 비네트 색상이 빨강 → 보라 → 파랑으로 점진적으로 변화
- ✅ 콘솔 로그에서 Color 값 변화 확인

---

## 테스트 4: 런타임 동적 변경

**목표**: 게임플레이 중 PostProcessSettings를 동적으로 변경할 수 있는지 확인

### 방법 A: Tick에서 펄스 효과

```cpp
void AMyTestActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 시간에 따라 비네트 강도 변경 (펄스 효과)
    float Time = UTimeManager::GetInstance().GetTotalTime();
    float Intensity = (sin(Time * 2.0f) + 1.0f) * 0.5f;  // [0.0, 1.0]

    UCameraComponent* Camera = GetComponentByClass<UCameraComponent>();
    if (Camera)
    {
        Camera->GetPostProcessSettings().bOverride_VignetteIntensity = true;
        Camera->GetPostProcessSettings().VignetteIntensity = Intensity;
    }
}
```

**예상 결과**:
- ✅ 비네트 효과가 주기적으로 강해졌다 약해짐 (펄스 효과)
- ✅ 콘솔 로그에서 Intensity 값이 계속 변하는 것 확인

### 방법 B: 입력 이벤트로 토글

```cpp
void AMyTestActor::OnKeyPress()
{
    UCameraComponent* Camera = GetComponentByClass<UCameraComponent>();
    if (Camera)
    {
        FPostProcessSettings& PP = Camera->GetPostProcessSettings();

        // 토글: 0.0 ↔ 1.0
        if (PP.VignetteIntensity < 0.5f)
        {
            PP.VignetteIntensity = 1.0f;
        }
        else
        {
            PP.VignetteIntensity = 0.0f;
        }

        PP.bOverride_VignetteIntensity = true;
    }
}
```

**예상 결과**:
- ✅ 키를 누를 때마다 비네트 효과가 ON/OFF 전환됨

---

## 테스트 5: 엣지 케이스

### 시나리오 A: 양쪽 모두 Override 없음

```cpp
// Camera A
CameraA->GetPostProcessSettings().bOverride_VignetteIntensity = false;

// Camera B
CameraB->GetPostProcessSettings().bOverride_VignetteIntensity = false;

// 블렌딩 시작
CameraManager->SetViewTarget(CameraB, 2.0f);
```

**예상 결과**:
- ✅ 비네트 효과 없음 (전체 블렌딩 동안 Intensity=0.0 유지)
- ✅ 콘솔 로그: `Override_I=0, Intensity=0.00`

### 시나리오 B: 한쪽만 Override

```cpp
// Camera A: Override 없음
CameraA->GetPostProcessSettings().bOverride_VignetteIntensity = false;

// Camera B: Override 활성화
CameraB->GetPostProcessSettings().bOverride_VignetteIntensity = true;
CameraB->GetPostProcessSettings().VignetteIntensity = 1.0f;

// 블렌딩 시작
CameraManager->SetViewTarget(CameraB, 2.0f);
```

**예상 결과**:
- ✅ A(0.0) → B(1.0)로 보간됨
- ✅ FPostProcessSettings::Lerp()의 개선된 로직 덕분에 "둘 중 하나라도 Override면 보간" 동작
- ✅ 콘솔 로그에서 Intensity가 0.0 → 1.0으로 증가

### 시나리오 C: 블렌딩 중단

```cpp
// Camera A → B 블렌딩 시작
CameraManager->SetViewTarget(CameraB, 3.0f);

// 1.5초 후 강제로 다른 카메라로 전환
// (Tick에서 시간 체크 후 실행)
CameraManager->SetViewTarget(CameraC, 0.0f);  // 즉시 전환
```

**예상 결과**:
- ✅ 블렌딩이 중단되고 CameraC의 설정이 즉시 적용됨
- ✅ 콘솔 로그에서 값이 갑자기 변하는 것 확인

---

## 디버깅 체크리스트

### PostProcessSettings가 전달되지 않는 경우

#### 1. UCameraComponent::GetCameraView() 확인

**브레이크포인트 위치**: `CameraComponent.cpp:527`
```cpp
OutPOV.PostProcessSettings = PostProcessSettings;
```

**확인 사항**:
- PostProcessSettings 값이 올바르게 설정되어 있는지
- OutPOV에 값이 복사되는지

#### 2. PlayerCameraManager::UpdateViewTarget() 확인

**브레이크포인트 위치**: `PlayerCameraManager.cpp:265`
```cpp
CachedPOV = ViewTarget.POV;
```

**확인 사항**:
- CachedPOV.PostProcessSettings에 값이 설정되는지

#### 3. PlayerCameraManager::UpdateBlending() 확인 (블렌딩 중)

**브레이크포인트 위치**: `PlayerCameraManager.cpp:313`
```cpp
CachedPOV.PostProcessSettings = FPostProcessSettings::Lerp(...);
```

**확인 사항**:
- BlendAlpha 값이 0.0 → 1.0으로 증가하는지
- Lerp 결과가 제대로 보간되는지

#### 4. Renderer::RenderLevel() 확인

**브레이크포인트 위치**: Renderer가 CameraManager에서 PostProcessSettings를 가져오는 부분

**확인 사항**:
- CameraManager가 nullptr이 아닌지
- PostProcessSettings가 FRenderingContext에 전달되는지

#### 5. VignettePass::UpdateConstants() 확인

**브레이크포인트 위치**: `VignettePass.cpp:26`
```cpp
const FPostProcessSettings& PPSettings = Context.PostProcessSettings;
```

**확인 사항**:
- Context.PostProcessSettings 값 확인
- Override 플래그 확인
- VignetteConstants에 올바른 값이 설정되는지

#### 6. 콘솔 로그 분석

```
VignettePass: BlendWeight=1.00, Override_I=1, Override_C=1, Intensity=0.50, Color=(1.00,0.00,0.00)
```

**로그가 출력되지 않는 경우**:
- VignettePass가 실행되지 않고 있음
- Renderer의 후처리 파이프라인 확인 필요

**로그에서 Override=0인 경우**:
- 카메라의 PostProcessSettings 설정 확인
- bOverride 플래그가 true로 설정되었는지 확인

**로그에서 값이 항상 0인 경우**:
- PostProcessSettings가 전달 경로 어딘가에서 초기화되고 있음
- 위의 브레이크포인트로 단계별 추적 필요

---

## 성공 기준

### Phase 4 통과 조건

- ✅ **테스트 1**: 카메라의 PostProcessSettings가 화면에 정상 반영됨
- ✅ **테스트 2**: Override 플래그가 예상대로 동작함
- ✅ **테스트 3**: 카메라 블렌딩 시 PostProcessSettings도 부드럽게 보간됨
- ✅ **테스트 4**: 런타임에 PostProcessSettings 변경이 즉시 반영됨
- ✅ **테스트 5**: 엣지 케이스에서도 안정적으로 동작함
- ✅ **디버그 로그**: 콘솔에 정상적인 디버그 로그 출력됨

---

## 다음 단계

Phase 4 테스트가 모두 통과하면:

1. **Phase 5: 문서화** 진행
   - `CameraSystem_FrameFlow.md` 업데이트
   - 사용 예제 작성
   - API 레퍼런스 주석 추가

2. **선택적 확장**:
   - UCameraModifier로 PostProcessSettings 동적 제어
   - 추가 후처리 효과 (Bloom, DOF, Color Grading)
   - PostProcessVolume 시스템

---

**문서 작성일**: 2025-11-05
**작성자**: Claude
**버전**: 1.0
