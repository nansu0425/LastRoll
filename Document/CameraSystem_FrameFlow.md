# Camera System Frame Flow Documentation

**FutureEngine 카메라 시스템 - 한 프레임 처리 흐름**

이 문서는 게임/PIE 모드에서 카메라가 어떻게 계산되고, 렌더러로 전달되며, GPU에서 사용되는지를 설명합니다.

---

## 목차

1. [아키텍처 개요](#1-아키텍처-개요)
2. [한 프레임의 전체 흐름](#2-한-프레임의-전체-흐름)
3. [단계별 상세 설명](#3-단계별-상세-설명)
4. [코드 위치 레퍼런스](#4-코드-위치-레퍼런스)
5. [주요 데이터 구조](#5-주요-데이터-구조)
6. [디버깅 가이드](#6-디버깅-가이드)

---

## 1. 아키텍처 개요

### 1.1 시스템 구성 요소

```
┌─────────────────────────────────────────────────────────────────┐
│                        Game/PIE Frame                           │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  [UWorld::Tick]                                                 │
│       │                                                         │
│       ├─→ Actor Ticking                                         │
│       │                                                         │
│       └─→ [APlayerCameraManager::UpdateCamera]                  │
│              │                                                  │
│              ├─→ 1. UpdateViewTarget()                          │
│              │      (ViewTarget/CameraComponent에서 기본 POV)    │
│              │                                                  │
│              ├─→ 2. UpdateBlending()                            │
│              │      (ViewTarget 전환 블렌딩)                     │
│              │                                                  │
│              ├─→ 3. ApplyCameraModifiers()                      │
│              │      (모디파이어 체인: Shake, Lag, FOV 등)        │
│              │                                                  │
│              ├─→ 4. UpdateFading()                              │
│              │      (화면 페이드 효과)                           │
│              │                                                  │
│              └─→ 5. UpdateCameraConstants()                     │
│                     (FCameraConstants: View/Proj 행렬 생성)     │
│                                                                 │
│  ────────────────────────────────────────────────────────────  │
│                                                                 │
│  [URenderer::RenderLevel]                                       │
│       │                                                         │
│       ├─→ Camera Selection                                      │
│       │    if (PIE/Game) → CameraManager->GetCameraConstants()  │
│       │    else → EditorCamera->GetFViewProjConstants()         │
│       │                                                         │
│       ├─→ GPU Upload                                            │
│       │    UpdateConstantBufferData(ConstantBufferViewProj)     │
│       │    SetConstantBuffer(slot=1, VS)                        │
│       │                                                         │
│       └─→ Render Passes                                         │
│            (Shadow, Opaque, Transparent, PostProcess)           │
│                                                                 │
│  ────────────────────────────────────────────────────────────  │
│                                                                 │
│  [GPU - Vertex Shaders]                                         │
│       │                                                         │
│       └─→ cbuffer ViewProj : register(b1)                       │
│            {                                                    │
│                row_major float4x4 View;                         │
│                row_major float4x4 Projection;                   │
│                float3 ViewWorldLocation;                        │
│                float NearClip, FarClip;                         │
│            }                                                    │
│                                                                 │
│            // Vertex transform:                                 │
│            float4 worldPos = mul(float4(pos, 1), World);        │
│            float4 viewPos = mul(worldPos, View);                │
│            float4 clipPos = mul(viewPos, Projection);           │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 1.2 주요 클래스 역할

| 클래스 | 역할 | 위치 |
|--------|------|------|
| `APlayerCameraManager` | 카메라 시스템 총괄 관리자 | `Engine/Source/Actor/Public/PlayerCameraManager.h` |
| `UCameraModifier` | 카메라 후처리 효과 베이스 클래스 | `Engine/Source/Component/Camera/Public/CameraModifier.h` |
| `UCameraComponent` | 액터에 부착 가능한 카메라 컴포넌트 | `Engine/Source/Component/Camera/Public/CameraComponent.h` |
| `FMinimalViewInfo` | POV 데이터 (Location, Rotation, FOV 등) | `Engine/Source/Global/CoreTypes.h:34-119` |
| `FCameraConstants` | GPU 업로드용 View/Projection 행렬 | `Engine/Source/Global/CoreTypes.h:17-29` |
| `UWorld` | 카메라 매니저 소유 및 틱 호출 | `Engine/Source/Level/Public/World.h` |
| `URenderer` | 카메라 상수 수신 및 GPU 업로드 | `Engine/Source/Render/Renderer/Private/Renderer.cpp` |

---

## 2. 한 프레임의 전체 흐름

### 2.1 타임라인 (시간순)

```
Frame N 시작
│
├─[1] UWorld::Tick(DeltaTime)
│   │
│   ├─ Actor::Tick() 호출 (모든 액터)
│   │   └─ 게임 로직, ViewTarget 액터 위치/회전 업데이트
│   │
│   └─[2] CameraManager->UpdateCamera(DeltaTime)
│       │
│       ├─[2.1] UpdateViewTarget(DeltaTime)
│       │       → ViewTarget의 UCameraComponent 또는 Actor Transform에서
│       │         기본 POV 가져오기 → CachedPOV에 저장
│       │
│       ├─[2.2] UpdateBlending(DeltaTime)
│       │       → 블렌딩 중이면 ViewTarget ↔ PendingViewTarget 보간
│       │         → CachedPOV 업데이트
│       │
│       ├─[2.3] ApplyCameraModifiers(DeltaTime)
│       │       → ModifierList를 Priority로 정렬
│       │       → 각 Modifier::UpdateAlpha() 호출 (블렌드 상태 머신)
│       │       → 각 Modifier::ModifyCamera(DeltaTime, CachedPOV) 호출
│       │         예: CameraShake가 CachedPOV.Location += offset 수정
│       │
│       ├─[2.4] UpdateFading(DeltaTime)
│       │       → 화면 페이드 알파 업데이트 (렌더러에서 사용 예정)
│       │
│       └─[2.5] UpdateCameraConstants()
│               → CachedPOV.ToCameraConstants() 호출
│               → View 행렬 구성:
│                 - Translation = TranslationMatrixInverse(Location)
│                 - Rotation = Quaternion → Basis Vectors → Matrix
│                 - View = Translation * Rotation
│               → Projection 행렬 구성:
│                 - Perspective: FOV, AspectRatio, Near/FarClip
│                 - Orthographic: OrthoWidth, AspectRatio, Near/FarClip
│               → CachedCameraConstants에 저장
│
├─[3] URenderer::RenderFrame()
│   │
│   └─ for each Viewport:
│       │
│       ├─[3.1] URenderer::RenderLevel(Viewport, ViewportIndex)
│       │   │
│       │   ├─[3.1.1] 렌더링할 World 결정
│       │   │         WorldToRender = GEditor->GetWorldForViewport(ViewportIndex)
│       │   │
│       │   ├─[3.1.2] 카메라 선택
│       │   │         WorldType = WorldToRender->GetWorldType()
│       │   │         CameraManager = WorldToRender->GetCameraManager()
│       │   │
│       │   │         if (WorldType == PIE || Game) && CameraManager != nullptr:
│       │   │             ViewProj = CameraManager->GetCameraConstants()
│       │   │                        ↑
│       │   │                        └─ CachedCameraConstants 반환 (복사)
│       │   │         else:
│       │   │             ViewProj = EditorCamera->GetFViewProjConstants()
│       │   │
│       │   ├─[3.1.3] GPU 상수 버퍼 업로드
│       │   │         FRenderResourceFactory::UpdateConstantBufferData(
│       │   │             ConstantBufferViewProj,   // ID3D11Buffer*
│       │   │             ViewProj                  // FCameraConstants
│       │   │         )
│       │   │         → GPU 메모리에 복사 (D3D11 Map/Unmap)
│       │   │
│       │   │         Pipeline->SetConstantBuffer(
│       │   │             1,                // Slot b1
│       │   │             EShaderType::VS,  // Vertex Shader
│       │   │             ConstantBufferViewProj
│       │   │         )
│       │   │
│       │   ├─[3.1.4] RenderingContext 구성
│       │   │         - ViewProj 포인터
│       │   │         - EditorCamera (RenderPass 호환성)
│       │   │         - ViewMode, ShowFlags, ViewportRect
│       │   │
│       │   └─[3.1.5] Render Passes 실행
│       │             - ShadowMapPass
│       │             - OpaquePass
│       │             - TransparentPass
│       │             - PostProcessPass (FXAA, etc.)
│       │
│       └─[3.2] Present (SwapChain)
│
└─Frame N 종료
```

### 2.2 데이터 흐름

```
UCameraComponent (Actor에 부착)
    ↓ GetCameraView()
FMinimalViewInfo (POV: Location, Rotation, FOV, ...)
    ↓ UpdateViewTarget()
CachedPOV (PlayerCameraManager 내부)
    ↓ ModifyCamera() x N (모디파이어 체인)
CachedPOV (수정됨: Shake 오프셋, FOV 변경 등)
    ↓ ToCameraConstants()
FCameraConstants (View/Projection 행렬)
    ↓ GetCameraConstants()
Renderer (복사본 획득)
    ↓ UpdateConstantBufferData()
GPU 상수 버퍼 (VRAM)
    ↓ Vertex Shader (cbuffer b1)
최종 클립 공간 좌표 (화면 렌더링)
```

---

## 3. 단계별 상세 설명

### 3.1 Step 1: UpdateViewTarget()

**파일**: `Engine/Source/Actor/Private/PlayerCameraManager.cpp:244-266`

**목적**: ViewTarget으로부터 기본 카메라 POV 가져오기

**로직**:
```cpp
void APlayerCameraManager::UpdateViewTarget(float DeltaTime)
{
    if (ViewTarget.Target && ViewTarget.CameraComponent)
    {
        // Case 1: UCameraComponent 있음
        ViewTarget.CameraComponent->GetCameraView(ViewTarget.POV);
        // → POV.Location = CameraComponent->GetWorldLocation()
        // → POV.Rotation = CameraComponent->GetWorldRotationAsQuaternion()
        // → POV.FOV, AspectRatio, ClipPlanes 복사
    }
    else if (ViewTarget.Target)
    {
        // Case 2: CameraComponent 없음 → Actor Transform 사용
        ViewTarget.POV.Location = ViewTarget.Target->GetActorLocation();
        ViewTarget.POV.Rotation = ViewTarget.Target->GetActorRotation();
        // → FOV, AspectRatio는 기본값 유지
    }
    else
    {
        // Case 3: ViewTarget 없음 → 기본 카메라 위치
        ViewTarget.POV.Location = FVector(0, 0, 500);
        ViewTarget.POV.Rotation = FQuaternion::Identity();
    }

    // CachedPOV에 저장 (다음 단계에서 수정됨)
    CachedPOV = ViewTarget.POV;
}
```

**출력**: `CachedPOV` (FMinimalViewInfo)

---

### 3.2 Step 2: UpdateBlending()

**파일**: `Engine/Source/Actor/Private/PlayerCameraManager.cpp:268-312`

**목적**: 두 ViewTarget 간 부드러운 전환

**활성화 조건**: `bIsBlending == true` (SetViewTarget(Actor, BlendTime > 0) 호출 시)

**로직**:
```cpp
void APlayerCameraManager::UpdateBlending(float DeltaTime)
{
    BlendTimeRemaining -= DeltaTime;

    if (BlendTimeRemaining <= 0.0f)
    {
        // 블렌드 완료 → PendingViewTarget을 ViewTarget으로 승격
        ViewTarget = PendingViewTarget;
        bIsBlending = false;
    }
    else
    {
        // 블렌드 알파 계산
        float BlendAlpha = 1.0f - (BlendTimeRemaining / BlendTime);

        // PendingViewTarget POV 업데이트
        if (PendingViewTarget.CameraComponent)
            PendingViewTarget.CameraComponent->GetCameraView(PendingViewTarget.POV);
        else if (PendingViewTarget.Target)
        {
            PendingViewTarget.POV.Location = PendingViewTarget.Target->GetActorLocation();
            PendingViewTarget.POV.Rotation = PendingViewTarget.Target->GetActorRotation();
        }

        // POV 보간 (모든 필드)
        CachedPOV.Location = Lerp(ViewTarget.POV.Location, PendingViewTarget.POV.Location, BlendAlpha);
        CachedPOV.Rotation = Lerp(ViewTarget.POV.Rotation, PendingViewTarget.POV.Rotation, BlendAlpha);
        CachedPOV.Rotation.Normalize();  // Quaternion 정규화
        CachedPOV.FOV = Lerp(ViewTarget.POV.FOV, PendingViewTarget.POV.FOV, BlendAlpha);
        CachedPOV.AspectRatio = Lerp(...);
        CachedPOV.NearClipPlane = Lerp(...);
        CachedPOV.FarClipPlane = Lerp(...);
        CachedPOV.OrthoWidth = Lerp(...);
    }
}
```

**입력**: `ViewTarget.POV`, `PendingViewTarget.POV`, `BlendAlpha`
**출력**: `CachedPOV` (보간된 POV)

---

### 3.3 Step 3: ApplyCameraModifiers()

**파일**: `Engine/Source/Actor/Private/PlayerCameraManager.cpp:314-340`

**목적**: 카메라 후처리 효과 체인 적용

**로직**:
```cpp
void APlayerCameraManager::ApplyCameraModifiers(float DeltaTime)
{
    if (ModifierList.empty())
        return;

    // 1. 우선순위로 정렬 (낮은 순위 먼저)
    std::sort(ModifierList.begin(), ModifierList.end(),
        [](const UCameraModifier* A, const UCameraModifier* B) {
            return A->GetPriority() < B->GetPriority();
        });

    // 2. 각 모디파이어 적용
    for (UCameraModifier* Modifier : ModifierList)
    {
        if (Modifier && !Modifier->IsDisabled())
        {
            // 2.1 블렌드 알파 업데이트 (상태 머신)
            Modifier->UpdateAlpha(DeltaTime);
            // Disabled → BlendingIn → Active → BlendingOut → Disabled

            // 2.2 POV 수정 (Alpha > 0인 경우만)
            if (Modifier->GetAlpha() > 0.0f)
            {
                Modifier->ModifyCamera(DeltaTime, CachedPOV);
                // ↑ CachedPOV가 참조로 전달되어 수정됨

                // 예: UCameraModifier_CameraShake::ModifyCamera()
                //     CachedPOV.Location += ShakeOffset * Alpha;
                //     CachedPOV.Rotation = ApplyRotationOffset(...);
            }
        }
    }
}
```

**입력**: `CachedPOV` (블렌딩 후 POV)
**출력**: `CachedPOV` (모디파이어 체인 적용 후 최종 POV)

**모디파이어 예시**:
- Priority 50: `UCameraModifier_CameraLag` → 부드러운 추적
- Priority 100: `UCameraModifier_CameraShake` → 흔들림 추가
- Priority 150: `UCameraModifier_LookAt` → 특정 타겟 주시 (회전 오버라이드)

---

### 3.4 Step 4: UpdateFading()

**파일**: `Engine/Source/Actor/Private/PlayerCameraManager.cpp:342-362`

**목적**: 화면 페이드 효과 업데이트 (렌더러 통합 대기 중)

**로직**:
```cpp
void APlayerCameraManager::UpdateFading(float DeltaTime)
{
    FadeTimeRemaining -= DeltaTime;

    if (FadeTimeRemaining <= 0.0f)
    {
        FadeAlpha.X = FadeAlpha.Y;  // 목표 알파에 도달
        bIsFading = false;
    }
    else
    {
        float FadeBlendAlpha = 1.0f - (FadeTimeRemaining / FadeTime);
        FadeAlpha.X = Lerp(FadeAmount, FadeAlpha.Y, FadeBlendAlpha);
    }

    // TODO: URenderer 통합 (후처리 패스에서 FadeColor, FadeAlpha.X 사용)
}
```

**현재 상태**: 메타데이터만 업데이트, 렌더러 통합은 미구현

---

### 3.5 Step 5: UpdateCameraConstants()

**파일**: `Engine/Source/Actor/Private/PlayerCameraManager.cpp:364-368`

**목적**: 최종 POV를 GPU 업로드용 View/Projection 행렬로 변환

**로직**:
```cpp
void APlayerCameraManager::UpdateCameraConstants()
{
    // FMinimalViewInfo::ToCameraConstants() 호출
    CachedCameraConstants = CachedPOV.ToCameraConstants();
}
```

**`ToCameraConstants()` 상세** (`CoreTypes.h:62-118`):

```cpp
FCameraConstants FMinimalViewInfo::ToCameraConstants() const
{
    FCameraConstants Result;

    // ===== View 행렬 구성 (월드 → 뷰 공간) =====
    // 1. Translation (카메라 위치 역변환)
    FMatrix Translation = FMatrix::TranslationMatrixInverse(Location);

    // 2. Rotation (Quaternion → 회전 행렬)
    FVector WorldForward(1, 0, 0);  // X축 (엔진 좌표계)
    FVector WorldRight(0, 1, 0);    // Y축
    FVector WorldUp(0, 0, 1);       // Z축

    FVector Forward = Rotation.RotateVector(WorldForward);
    FVector Right = Rotation.RotateVector(WorldRight);
    FVector Up = Rotation.RotateVector(WorldUp);

    FMatrix RotationMatrix(Right, Up, Forward);
    RotationMatrix = RotationMatrix.Transpose();  // 역행렬

    Result.View = Translation * RotationMatrix;
    // 곱셈 순서: 먼저 Translation, 그 다음 Rotation (row-major)

    // ===== Projection 행렬 구성 =====
    if (bUsePerspectiveProjection)
    {
        // 원근 투영 (왼손 좌표계)
        float FovYRad = FOV * (PI / 180.0f);
        float f = 1.0f / tan(FovYRad / 2.0f);

        Result.Projection = FMatrix::Identity();
        Result.Projection.Data[0][0] = f / AspectRatio;
        Result.Projection.Data[1][1] = f;
        Result.Projection.Data[2][2] = FarClipPlane / (FarClipPlane - NearClipPlane);
        Result.Projection.Data[2][3] = 1.0f;
        Result.Projection.Data[3][2] = (-NearClipPlane * FarClipPlane) / (FarClipPlane - NearClipPlane);
        Result.Projection.Data[3][3] = 0.0f;
    }
    else
    {
        // 직교 투영
        float Width = OrthoWidth;
        float Height = OrthoWidth / AspectRatio;
        Result.Projection = FMatrix::CreateOrthoLH(
            -Width / 2.0f, Width / 2.0f,
            -Height / 2.0f, Height / 2.0f,
            NearClipPlane, FarClipPlane
        );
    }

    // 메타데이터 저장
    Result.ViewWorldLocation = Location;
    Result.NearClip = NearClipPlane;
    Result.FarClip = FarClipPlane;

    return Result;
}
```

**출력**: `CachedCameraConstants` (FCameraConstants: View, Projection, ViewWorldLocation, Near/FarClip)

---

### 3.6 Renderer: 카메라 선택

**파일**: `Engine/Source/Render/Renderer/Private/Renderer.cpp:948-977`

**목적**: 월드 타입에 따라 올바른 카메라 소스 선택

**로직**:
```cpp
void URenderer::RenderLevel(FViewport* InViewport, int32 ViewportIndex)
{
    // 1. 렌더링할 World 결정
    UWorld* WorldToRender = GEditor->GetWorldForViewport(ViewportIndex);
    if (!WorldToRender) { return; }

    // 2. 카메라 소스 결정
    FCameraConstants ViewProj;
    UCamera* EditorCamera = InViewport->GetViewportClient()->GetCamera();

    EWorldType WorldType = WorldToRender->GetWorldType();
    APlayerCameraManager* CameraManager = WorldToRender->GetCameraManager();

    if ((WorldType == EWorldType::Game || WorldType == EWorldType::PIE) && CameraManager != nullptr)
    {
        // PIE/Game 모드: PlayerCameraManager 사용
        ViewProj = CameraManager->GetCameraConstants();
        // ↑ CachedCameraConstants의 복사본 반환
    }
    else
    {
        // Editor 모드: EditorCamera 사용
        ViewProj = EditorCamera->GetFViewProjConstants();
    }

    // 참고: EditorCamera는 RenderingContext에 항상 포함
    // (많은 RenderPass가 빌보드 회전, 안개 계산 등을 위해 Camera 포인터 사용)

    // ...
}
```

**결정 테이블**:

| WorldType | CameraManager | 사용할 카메라 |
|-----------|---------------|---------------|
| `Editor` | N/A | EditorCamera |
| `EditorPreview` | N/A | EditorCamera |
| `PIE` | `nullptr` | EditorCamera (폴백) |
| `PIE` | 유효함 | **CameraManager** ✓ |
| `Game` | 유효함 | **CameraManager** ✓ |

---

### 3.7 Renderer: GPU 업로드

**파일**: `Engine/Source/Render/Renderer/Private/Renderer.cpp:979-982`

**목적**: FCameraConstants를 GPU 상수 버퍼에 업로드

**로직**:
```cpp
// CRITICAL: 카메라 선택 이후에 수행되어야 함
FRenderResourceFactory::UpdateConstantBufferData(ConstantBufferViewProj, ViewProj);
Pipeline->SetConstantBuffer(1, EShaderType::VS, ConstantBufferViewProj);
```

**`UpdateConstantBufferData()` 내부** (추정):
```cpp
void UpdateConstantBufferData(ID3D11Buffer* Buffer, const FCameraConstants& Data)
{
    D3D11_MAPPED_SUBRESOURCE MappedResource;
    DeviceContext->Map(Buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);

    memcpy(MappedResource.pData, &Data, sizeof(FCameraConstants));

    DeviceContext->Unmap(Buffer, 0);
}
```

**GPU 바인딩**:
- Slot: `b1` (register(b1))
- Shader Stage: Vertex Shader (`EShaderType::VS`)
- Buffer: `ConstantBufferViewProj` (ID3D11Buffer*)

---

### 3.8 GPU: Vertex Shader 사용

**파일**: `Engine/Asset/Shader/UberLit.hlsl` (예시)

**셰이더 코드**:
```hlsl
cbuffer ConstantBufferViewProj : register(b1)
{
    row_major float4x4 View;              // 64 bytes
    row_major float4x4 Projection;        // 64 bytes
    float3 ViewWorldLocation;             // 12 bytes
    float NearClip;                       // 4 bytes
    float FarClip;                        // 4 bytes
    float3 Padding;                       // 12 bytes (alignment)
};

struct VSInput
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float2 TexCoord : TEXCOORD;
};

struct PSInput
{
    float4 Position : SV_POSITION;
    float3 WorldPos : POSITION0;
    float3 Normal : NORMAL;
    float2 TexCoord : TEXCOORD;
};

PSInput VSMain(VSInput input)
{
    PSInput output;

    // Local → World
    float4 worldPos = mul(float4(input.Position, 1.0f), World);  // cbuffer b0

    // World → View
    float4 viewPos = mul(worldPos, View);  // cbuffer b1

    // View → Clip (Projection)
    float4 clipPos = mul(viewPos, Projection);  // cbuffer b1

    output.Position = clipPos;
    output.WorldPos = worldPos.xyz;
    output.Normal = mul(input.Normal, (float3x3)World);
    output.TexCoord = input.TexCoord;

    return output;
}
```

**변환 파이프라인**:
```
Local Space (Vertex Position)
    ↓ * World Matrix (b0)
World Space
    ↓ * View Matrix (b1)
View Space (Camera Space)
    ↓ * Projection Matrix (b1)
Clip Space
    ↓ Perspective Divide (GPU 자동)
NDC (Normalized Device Coordinates)
    ↓ Viewport Transform (GPU 자동)
Screen Space (픽셀)
```

---

## 4. 코드 위치 레퍼런스

### 4.1 핵심 파일

| 파일 | 핵심 내용 | 라인 |
|------|-----------|------|
| **PlayerCameraManager.cpp** | UpdateCamera() 메인 파이프라인 | 220-242 |
| | UpdateViewTarget() | 244-266 |
| | UpdateBlending() | 268-312 |
| | ApplyCameraModifiers() | 314-340 |
| | UpdateCameraConstants() | 364-368 |
| **CameraModifier.cpp** | UpdateAlpha() 상태 머신 | 27-76 |
| | EnableModifier() | 84-100 |
| | DisableModifier() | 102-119 |
| **CoreTypes.h** | FMinimalViewInfo 정의 | 34-59 |
| | ToCameraConstants() | 62-118 |
| | FCameraConstants 정의 | 17-29 |
| **World.cpp** | CameraManager->UpdateCamera() 호출 | 114-117 |
| **Renderer.cpp** | 카메라 선택 로직 | 957-977 |
| | GPU 상수 버퍼 업로드 | 979-982 |
| | RenderingContext 구성 | 1017-1025 |

### 4.2 관련 클래스

| 클래스 | 헤더 파일 | 구현 파일 |
|--------|-----------|-----------|
| `APlayerCameraManager` | `Actor/Public/PlayerCameraManager.h` | `Actor/Private/PlayerCameraManager.cpp` |
| `UCameraModifier` | `Component/Camera/Public/CameraModifier.h` | `Component/Camera/Private/CameraModifier.cpp` |
| `UCameraComponent` | `Component/Camera/Public/CameraComponent.h` | `Component/Camera/Private/CameraComponent.cpp` |
| `UWorld` | `Level/Public/World.h` | `Level/Private/World.cpp` |
| `URenderer` | `Render/Renderer/Public/Renderer.h` | `Render/Renderer/Private/Renderer.cpp` |

---

## 5. 주요 데이터 구조

### 5.1 FMinimalViewInfo

**정의**: `Engine/Source/Global/CoreTypes.h:34-119`

**용도**: 카메라 POV (Point of View) 데이터

**필드**:
```cpp
struct FMinimalViewInfo
{
    // 변환
    FVector Location;           // 월드 공간 카메라 위치
    FQuaternion Rotation;       // 카메라 회전 (쿼터니언)

    // 투영 파라미터
    float FOV;                  // 수직 시야각 (도 단위), 기본값: 90.0
    float AspectRatio;          // 너비 / 높이, 기본값: 16/9
    float NearClipPlane;        // Near Z, 기본값: 1.0
    float FarClipPlane;         // Far Z, 기본값: 10000.0
    float OrthoWidth;           // 직교 투영 너비, 기본값: 1000.0
    bool bUsePerspectiveProjection; // true: 원근, false: 직교

    // 메서드
    FCameraConstants ToCameraConstants() const;  // View/Proj 행렬 생성
};
```

---

### 5.2 FCameraConstants

**정의**: `Engine/Source/Global/CoreTypes.h:17-29`

**용도**: GPU 업로드용 카메라 행렬 및 메타데이터

**필드**:
```cpp
struct FCameraConstants
{
    FMatrix View;               // View 행렬 (월드 → 뷰 공간)
    FMatrix Projection;         // Projection 행렬 (뷰 → 클립 공간)
    FVector ViewWorldLocation;  // 카메라 월드 위치 (라이팅 계산용)
    float NearClip;             // Near 클립 평면
    float FarClip;              // Far 클립 평면
};
```

**GPU 레이아웃** (cbuffer b1):
```
Offset  | Field              | Size
--------+--------------------+------
0       | View               | 64 bytes
64      | Projection         | 64 bytes
128     | ViewWorldLocation  | 12 bytes
140     | NearClip           | 4 bytes
144     | FarClip            | 4 bytes
148     | Padding            | 12 bytes (alignment)
--------+--------------------+------
Total   |                    | 160 bytes
```

---

### 5.3 FViewTarget

**정의**: `Engine/Source/Actor/Public/PlayerCameraManager.h:11-23`

**용도**: 카메라 블렌딩을 위한 타겟 정보

**필드**:
```cpp
struct FViewTarget
{
    AActor* Target;                        // 타겟 액터 (카메라를 가진 액터)
    UCameraComponent* CameraComponent;     // 타겟의 카메라 컴포넌트 (optional)
    FMinimalViewInfo POV;                  // 현재 POV 스냅샷
};
```

**사용**:
- `ViewTarget`: 현재 활성 카메라
- `PendingViewTarget`: 블렌드할 타겟 (전환 중)

---

## 6. 디버깅 가이드

### 6.1 카메라 위치가 업데이트되지 않는 경우

**체크리스트**:

1. **CameraManager가 존재하는가?**
   ```cpp
   UWorld* World = ...;
   APlayerCameraManager* CameraManager = World->GetCameraManager();
   if (!CameraManager) { /* CameraManager를 생성해야 함 */ }
   ```

2. **ViewTarget이 설정되었는가?**
   ```cpp
   if (!CameraManager->GetViewTarget()) {
       // SetViewTarget(Actor) 호출 필요
   }
   ```

3. **World::Tick()이 호출되는가?**
   - WorldType이 `Game` 또는 `PIE`인지 확인
   - `bBegunPlay == true`인지 확인

4. **CameraManager->UpdateCamera()가 호출되는가?**
   - `World.cpp:114-117` 브레이크포인트 설정
   - DeltaTime이 0이 아닌지 확인

5. **Renderer가 올바른 카메라를 선택하는가?**
   - `Renderer.cpp:965-974` 브레이크포인트
   - `WorldType`과 `CameraManager` 값 확인
   - `ViewProj`가 EditorCamera가 아닌 CameraManager에서 왔는지 확인

6. **GPU 업로드가 카메라 선택 이후에 수행되는가?**
   - `Renderer.cpp:981` 확인
   - **CRITICAL**: 이 줄은 반드시 카메라 선택 (Line 968) 이후에 있어야 함

---

### 6.2 모디파이어가 작동하지 않는 경우

**체크리스트**:

1. **모디파이어가 추가되었는가?**
   ```cpp
   UCameraModifier* Modifier = CameraManager->AddCameraModifier(
       UCameraModifier_CameraShake::StaticClass()
   );
   if (!Modifier) { /* 클래스가 UCameraModifier 서브클래스인지 확인 */ }
   ```

2. **모디파이어가 활성화되었는가?**
   ```cpp
   Modifier->EnableModifier();  // 블렌드 인 시작
   ```

3. **모디파이어 Alpha가 > 0인가?**
   ```cpp
   float Alpha = Modifier->GetAlpha();  // 0.0이면 효과 없음
   ```

4. **ModifyCamera()가 호출되는가?**
   - `PlayerCameraManager.cpp:336` 브레이크포인트
   - `ModifierList`에 모디파이어가 있는지 확인
   - `IsDisabled() == false`인지 확인

5. **ModifyCamera() 구현이 CachedPOV를 수정하는가?**
   ```cpp
   bool UCameraModifier_CameraShake::ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV)
   {
       if (!bIsShaking) return false;

       // ✓ InOutPOV를 수정해야 함
       InOutPOV.Location += ShakeOffset * GetAlpha();

       return true;  // ✓ true 반환 필수
   }
   ```

---

### 6.3 블렌딩이 작동하지 않는 경우

**체크리스트**:

1. **SetViewTarget()에서 BlendTime > 0을 전달했는가?**
   ```cpp
   CameraManager->SetViewTarget(NewActor, 2.0f);  // 2초 블렌드
   ```

2. **bIsBlending == true인가?**
   ```cpp
   // PlayerCameraManager.cpp:226-229
   if (bIsBlending) {
       UpdateBlending(DeltaTime);  // 이 블록이 실행되는지 확인
   }
   ```

3. **BlendTimeRemaining > 0인가?**
   - `BlendTime`과 `BlendTimeRemaining` 값 확인
   - DeltaTime이 너무 크면 한 프레임에 블렌딩이 끝날 수 있음

4. **PendingViewTarget이 유효한가?**
   ```cpp
   if (!PendingViewTarget.Target) { /* 타겟이 삭제되었을 수 있음 */ }
   ```

---

### 6.4 로그 활용

**유용한 로그 위치**:

```cpp
// PlayerCameraManager.cpp
UE_LOG_DEBUG("ViewTarget POV: Location=(%.2f, %.2f, %.2f)",
    CachedPOV.Location.X, CachedPOV.Location.Y, CachedPOV.Location.Z);

// Renderer.cpp (Line 968 근처)
UE_LOG_DEBUG("Using CameraManager: ViewProj.View[3][0]=%.2f",
    ViewProj.View.Data[3][0]);
```

**로그 매크로** (`Global/Macro.h`):
- `UE_LOG()`: 일반 정보
- `UE_LOG_WARNING()`: 경고
- `UE_LOG_ERROR()`: 에러
- `UE_LOG_DEBUG()`: 디버그 정보

---

### 6.5 GPU 디버깅

**Visual Studio Graphics Debugger**:

1. `Debug` → `Graphics` → `Start Diagnostics`
2. PIE 모드 진입 후 `Print Screen` 키로 프레임 캡처
3. `Pipeline Stages` → `Vertex Shader`
4. Constant Buffer `b1` 확인:
   - View 행렬 값이 Identity가 아닌지
   - ViewWorldLocation이 기대한 위치인지
   - NearClip/FarClip이 합리적인지

---

## 7. 추가 참고사항

### 7.1 좌표계

**엔진 좌표계**: Z-up, X-forward, Y-right, Left-Handed

- **X축**: 전방 (Actor의 앞 방향)
- **Y축**: 우측
- **Z축**: 상단 (중력 반대 방향)

**카메라 좌표계** (DirectX 표준):
- **X축**: 우측
- **Y축**: 상단
- **Z축**: 전방 (깊이 방향, 화면 안쪽)

**변환**: `ToCameraConstants()`에서 Quaternion → Basis Vectors로 처리

---

### 7.2 행렬 곱셈 순서

**Row-Major, Row-Vector** 시스템:

```cpp
FVector4 ClipPos = LocalPos * World * View * Projection;
```

**HLSL**:
```hlsl
float4 clipPos = mul(mul(mul(localPos, World), View), Projection);
```

**주의**: `row_major` 키워드 필수!
```hlsl
cbuffer ConstantBufferViewProj : register(b1)
{
    row_major float4x4 View;        // ✓ 필수
    row_major float4x4 Projection;  // ✓ 필수
}
```

---

### 7.3 성능 고려사항

1. **ModifierList 정렬**:
   - `ApplyCameraModifiers()`에서 매 프레임 `std::sort()` 호출
   - 모디파이어가 많으면 성능 영향 가능
   - **최적화**: Priority가 변경되지 않으면 정렬 생략 (TODO)

2. **Quaternion Slerp**:
   - 현재 컴포넌트별 Lerp + Normalize 사용
   - **TODO**: `FQuaternion::Slerp()` 구현 시 더 부드러운 회전 블렌딩

3. **GPU 업로드**:
   - `UpdateConstantBufferData()`는 CPU → GPU 복사 (느림)
   - 매 프레임 호출되므로 `D3D11_MAP_WRITE_DISCARD` 사용 (오버헤드 최소화)

---

## 8. TODO 및 향후 개선사항

### 8.1 구현 대기 중

- [ ] **Camera Fade 렌더러 통합** (`PlayerCameraManager.cpp:360`)
  - FadeColor, FadeAlpha를 후처리 셰이더로 전달
  - 풀스크린 쿼드에 블렌딩

- [ ] **Quaternion Slerp** (`PlayerCameraManager.cpp:299`)
  - 구형 선형 보간으로 부드러운 회전 블렌딩

- [ ] **Modifier 정렬 최적화** (`PlayerCameraManager.cpp:320`)
  - Priority가 변경되지 않으면 정렬 생략

- [ ] **직렬화 구현** (`PlayerCameraManager.cpp:374`)
  - ViewTarget, ModifierList, Fade 상태 저장/로드
  - 현재는 런타임 전용

### 8.2 추가 기능 아이디어

- [ ] **Camera Stack**: 여러 CameraManager 스택 지원 (UI 카메라, 미니맵 카메라 등)
- [ ] **Camera Animation**: 키프레임 기반 카메라 애니메이션
- [ ] **Camera Shake Presets**: 지진, 폭발, 총격 등 사전 정의된 흔들림 패턴
- [ ] **Dynamic FOV**: 속도 기반 FOV 자동 조정 (레이싱 게임)
- [ ] **Cinematic Camera**: 영화 촬영용 카메라 (Depth of Field, Motion Blur)

---

## 9. 변경 이력

| 날짜 | 작성자 | 변경 내용 |
|------|--------|-----------|
| 2025-11-05 | Claude | 초기 문서 작성 |

---

**문서 끝**
