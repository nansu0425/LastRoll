# PostProcessSettings Integration Plan

**카메라 시스템과 후처리 효과 통합 작업 계획**

이 문서는 Unreal Engine의 PostProcessSettings 구조를 참고하여, FutureEngine의 카메라 시스템에 후처리 설정을 통합하는 작업 계획입니다.

---

## 목차

1. [작업 개요](#1-작업-개요)
2. [현재 상황 분석](#2-현재-상황-분석)
3. [목표 아키텍처](#3-목표-아키텍처)
4. [구현 단계](#4-구현-단계)
5. [코드 변경 사항](#5-코드-변경-사항)
6. [테스트 계획](#6-테스트-계획)
7. [향후 확장](#7-향후-확장)

---

## 1. 작업 개요

### 1.1 목적

VignettePass를 포함한 모든 후처리 효과의 설정을 카메라 시스템을 통해 Renderer에 전달할 수 있도록 구조화합니다.

### 1.2 설계 원칙

1. **Unreal Engine 스타일**: UE의 FPostProcessSettings 구조 참고
2. **확장성**: 향후 다른 후처리 효과(Bloom, DOF, Color Grading 등) 추가 용이
3. **Override 시스템**: 각 설정별로 활성화/비활성화 가능
4. **기존 흐름 유지**: 현재 카메라 시스템 파이프라인을 최대한 유지

### 1.3 참고: Unreal Engine의 구조

```cpp
// Unreal Engine 예시
struct FPostProcessSettings
{
    // Vignette
    uint8 bOverride_VignetteIntensity : 1;
    float VignetteIntensity;
    FLinearColor VignetteColor;

    // Bloom
    uint8 bOverride_BloomIntensity : 1;
    float BloomIntensity;

    // Depth of Field
    uint8 bOverride_DepthOfFieldFocalDistance : 1;
    float DepthOfFieldFocalDistance;

    // ... 수십 개의 설정
};
```

---

## 2. 현재 상황 분석

### 2.1 현재 데이터 흐름

```
UCameraComponent
    ↓ GetCameraView()
FMinimalViewInfo (Location, Rotation, FOV, AspectRatio, ClipPlanes만)
    ↓ UpdateViewTarget()
CachedPOV (PlayerCameraManager)
    ↓ ModifyCamera() 체인
CachedPOV (수정됨)
    ↓ ToCameraConstants()
FCameraConstants (View, Projection 행렬만)
    ↓ GetCameraConstants()
Renderer
    ↓
VignettePass::UpdateConstants()
    → 하드코딩된 값 사용 ❌
```

### 2.2 문제점

**파일**: `Engine/Source/Render/RenderPass/Private/VignettePass.cpp:23-32`

```cpp
void FVignettePass::UpdateConstants()
{
    FVignetteConstants VignetteConstants = {};
    VignetteConstants.VignetteColor = FVector(0.0f, 1.0f, 1.0f);  // ❌ 하드코딩
    VignetteConstants.VignetteIntensity = 0.5f;                   // ❌ 하드코딩

    FRenderResourceFactory::UpdateConstantBufferData(VignetteConstantBuffer.Get(), VignetteConstants);
    Pipeline->SetConstantBuffer(0, EShaderType::PS, VignetteConstantBuffer.Get());
}
```

**문제**:
- VignetteColor, VignetteIntensity가 하드코딩되어 있음
- 카메라 설정으로 제어할 수 없음
- 런타임에 변경 불가능

### 2.3 현재 구조

| 클래스/구조체 | 역할 | PostProcess 관련 |
|---------------|------|------------------|
| `UCameraComponent` | 카메라 컴포넌트 | ❌ 없음 |
| `FMinimalViewInfo` | POV 데이터 | ❌ 없음 |
| `FCameraConstants` | View/Projection 행렬 | ❌ 없음 |
| `VignettePass` | Vignette 렌더링 | ✓ 자체 상수 (하드코딩) |

---

## 3. 목표 아키텍처

### 3.1 목표 데이터 흐름

```
UCameraComponent
    ├─ 투영 파라미터 (FOV, AspectRatio, ...)
    └─ PostProcessSettings (VignetteIntensity, VignetteColor, ...)
        ↓ GetCameraView()
FMinimalViewInfo
    ├─ Location, Rotation, FOV, ...
    └─ PostProcessSettings ✓
        ↓ UpdateViewTarget()
CachedPOV (PlayerCameraManager)
    ↓ ModifyCamera() 체인 (PostProcessSettings도 수정 가능!)
CachedPOV (수정됨)
    └─ PostProcessSettings (수정됨)
        ↓ ToCameraConstants()
FCameraConstants
    ├─ View, Projection 행렬
    └─ PostProcessSettings ✓ (또는 별도 전달)
        ↓ GetCameraConstants() / GetPostProcessSettings()
Renderer
    ↓
VignettePass::UpdateConstants(PostProcessSettings)
    → PostProcessSettings로부터 값 가져오기 ✓
```

### 3.2 새로운 구조

| 클래스/구조체 | 역할 | PostProcess 관련 |
|---------------|------|------------------|
| `FPostProcessSettings` | **새로 추가**: 모든 후처리 설정 | ✓ Vignette, Bloom, DOF 등 |
| `UCameraComponent` | 카메라 컴포넌트 | ✓ PostProcessSettings 멤버 |
| `FMinimalViewInfo` | POV 데이터 | ✓ PostProcessSettings 멤버 |
| `FCameraConstants` | View/Projection 행렬 | ✓ PostProcessSettings 멤버 (또는 별도) |
| `APlayerCameraManager` | 카메라 관리자 | ✓ PostProcessSettings Getter |
| `VignettePass` | Vignette 렌더링 | ✓ PostProcessSettings로부터 읽기 |

---

## 4. 구현 단계

### Phase 1: FPostProcessSettings 구조체 정의

**목적**: 모든 후처리 설정을 담을 구조체 생성

**작업**:
1. `CoreTypes.h`에 `FPostProcessSettings` 구조체 추가
2. Vignette 설정 포함 (확장 가능하도록 설계)
3. Override 플래그 시스템 구현
4. **BlendWeight 필드 추가** (향후 Volume/Modifier 블렌딩용)
5. **개선된 Lerp 메서드** (A→B 직관적 보간)
6. **Serialize() 메서드 추가** (위임 구조)

**예상 소요 시간**: 30분

---

### Phase 2A: 카메라 시스템 기본 통합

**목적**: UCameraComponent와 FMinimalViewInfo에 PostProcessSettings 추가

**작업**:
1. `FMinimalViewInfo`에 `PostProcessSettings` 멤버 추가
2. `UCameraComponent`에 `PostProcessSettings` 멤버 추가
3. `UCameraComponent::GetCameraView()`에서 PostProcessSettings 복사
4. `UCameraComponent` 직렬화를 **위임 구조**로 수정
5. `FCameraConstants`에 `PostProcessSettings` 추가
6. `FMinimalViewInfo::ToCameraConstants()`에서 PostProcessSettings 복사

**예상 소요 시간**: 30분

---

### Phase 2B: 간단 테스트 (CameraManager 없이)

**목적**: 카메라→렌더러 직통 연결 검증

**작업**:
1. `VignettePass::UpdateConstants()`에 PostProcessSettings 파라미터 추가
2. `URenderer::RenderLevel()`에서 **임시로** ViewportClient의 ActiveCamera에서 PostProcessSettings 직접 읽기
3. VignettePass에 전달
4. PIE 모드에서 카메라의 Vignette 설정이 화면에 반영되는지 확인

**검증 포인트**:
- ✅ UCameraComponent에서 설정한 VignetteIntensity/Color가 화면에 나타남
- ✅ Override 플래그 동작 확인

**예상 소요 시간**: 30분

---

### Phase 3: PlayerCameraManager 통합

**목적**: CameraManager가 PostProcessSettings를 처리하도록 수정

**작업**:
1. `APlayerCameraManager::UpdateBlending()`에서 **개선된 Lerp**로 PostProcessSettings 보간
2. `APlayerCameraManager::GetPostProcessSettings()` Getter 추가
3. `URenderer::RenderLevel()`를 수정하여 **CameraManager에서** PostProcessSettings 가져오도록 변경
4. 모디파이어가 PostProcessSettings를 수정할 수 있는지 확인

**검증 포인트**:
- ✅ CameraManager를 통해 PostProcessSettings 전달됨
- ✅ 카메라 블렌딩 시 PostProcessSettings도 보간됨

**예상 소요 시간**: 45분

---

### Phase 4: 최종 테스트 및 디버깅

**목적**: 전체 통합 검증 및 디버깅 로그 추가

**작업**:
1. 카메라 블렌딩 + PostProcessSettings 보간 테스트
2. 런타임에 PostProcessSettings 변경 테스트
3. VignettePass에 **디버깅 로그** 추가 (BlendWeight, Override 상태 출력)
4. 엣지 케이스 테스트 (Override 없음, 블렌딩 중단 등)

**예상 소요 시간**: 30분

---

### Phase 5: 문서화

**목적**: 새로운 시스템 사용법 문서화

**작업**:
1. `CameraSystem_FrameFlow.md` 업데이트 (PostProcessSettings 흐름 추가)
2. 사용 예제 작성
3. API 레퍼런스 주석 추가

**예상 소요 시간**: 30분

---

## 5. 코드 변경 사항

### 5.1 Phase 1: FPostProcessSettings 정의 (개선된 버전)

**파일**: `Engine/Source/Global/CoreTypes.h`

**위치**: FMinimalViewInfo 위에 추가 (Line 32 근처)

```cpp
/**
 * @brief 후처리 효과 설정
 *
 * Unreal Engine의 FPostProcessSettings를 참고한 구조입니다.
 * 각 설정은 Override 플래그를 가지며, 활성화된 설정만 적용됩니다.
 */
struct FPostProcessSettings
{
	// ===== 소스 전체 가중치 (향후 Volume/Modifier 블렌딩용) =====
	float BlendWeight;  // 이 설정 전체의 가중치 [0.0, 1.0]

	// ===== Vignette (비네트) =====
	bool bOverride_VignetteIntensity;   // true면 VignetteIntensity 사용
	float VignetteIntensity;            // 비네트 강도 [0.0, 1.0]

	bool bOverride_VignetteColor;       // true면 VignetteColor 사용
	FVector VignetteColor;              // 비네트 색상 (RGB)

	// ===== Bloom (블룸) - 향후 확장 =====
	// bool bOverride_BloomIntensity;
	// float BloomIntensity;

	// ===== Depth of Field (피사계 심도) - 향후 확장 =====
	// bool bOverride_DepthOfFieldFocalDistance;
	// float DepthOfFieldFocalDistance;

	// 기본 생성자
	FPostProcessSettings()
		: BlendWeight(1.0f)
		, bOverride_VignetteIntensity(false)
		, VignetteIntensity(0.0f)
		, bOverride_VignetteColor(false)
		, VignetteColor(0.0f, 0.0f, 0.0f)
	{
	}

	/**
	 * @brief A에서 B로 선형 보간 (개선된 버전)
	 *
	 * 카메라 블렌딩 시 사용됩니다.
	 * 둘 중 하나라도 Override면 보간하며, Override 없는 쪽은 0(기본값)으로 간주합니다.
	 *
	 * @param A 시작 설정
	 * @param B 목표 설정
	 * @param Alpha 보간 계수 [0.0, 1.0]
	 * @return A→B로 보간된 PostProcessSettings
	 */
	static FPostProcessSettings Lerp(const FPostProcessSettings& A, const FPostProcessSettings& B, float Alpha);

	/**
	 * @brief JSON 직렬화 (위임 방식)
	 *
	 * CameraComponent 등에서 PostProcessSettings를 블랙박스로 취급하도록 합니다.
	 *
	 * @param bInIsLoading true: 로드, false: 저장
	 * @param InOutHandle JSON 핸들
	 */
	void Serialize(bool bInIsLoading, JSON& InOutHandle);
};
```

**Lerp 구현** (CoreTypes.h에 inline 함수로):

```cpp
// 개선된 A→B 직관적 보간
inline FPostProcessSettings FPostProcessSettings::Lerp(const FPostProcessSettings& A, const FPostProcessSettings& B, float Alpha)
{
	FPostProcessSettings Result = A;  // 일단 A 복사

	// BlendWeight 보간
	Result.BlendWeight = A.BlendWeight * (1.0f - Alpha) + B.BlendWeight * Alpha;

	// VignetteIntensity - 둘 중 하나라도 Override면 보간
	if (A.bOverride_VignetteIntensity || B.bOverride_VignetteIntensity)
	{
		Result.bOverride_VignetteIntensity = true;
		float IA = A.bOverride_VignetteIntensity ? A.VignetteIntensity : 0.0f;
		float IB = B.bOverride_VignetteIntensity ? B.VignetteIntensity : 0.0f;
		Result.VignetteIntensity = IA * (1.0f - Alpha) + IB * Alpha;
	}

	// VignetteColor - 둘 중 하나라도 Override면 보간
	if (A.bOverride_VignetteColor || B.bOverride_VignetteColor)
	{
		Result.bOverride_VignetteColor = true;
		FVector CA = A.bOverride_VignetteColor ? A.VignetteColor : FVector(0, 0, 0);
		FVector CB = B.bOverride_VignetteColor ? B.VignetteColor : FVector(0, 0, 0);
		Result.VignetteColor = CA * (1.0f - Alpha) + CB * Alpha;
	}

	return Result;
}
```

**Serialize 구현** (CoreTypes.cpp 또는 별도 .cpp 파일):

```cpp
void FPostProcessSettings::Serialize(bool bInIsLoading, JSON& InOutHandle)
{
	if (bInIsLoading)
	{
		// BlendWeight
		if (InOutHandle.hasKey("BlendWeight"))
			BlendWeight = InOutHandle["BlendWeight"].ToFloat();

		// VignetteIntensity
		if (InOutHandle.hasKey("VignetteIntensity"))
		{
			bOverride_VignetteIntensity = true;
			VignetteIntensity = InOutHandle["VignetteIntensity"].ToFloat();
		}

		// VignetteColor
		if (InOutHandle.hasKey("VignetteColor"))
		{
			bOverride_VignetteColor = true;
			JSON ColorJSON = InOutHandle["VignetteColor"];
			VignetteColor = FVector(
				ColorJSON["X"].ToFloat(),
				ColorJSON["Y"].ToFloat(),
				ColorJSON["Z"].ToFloat()
			);
		}
	}
	else
	{
		// BlendWeight (항상 저장)
		InOutHandle["BlendWeight"] = BlendWeight;

		// VignetteIntensity (Override된 경우만)
		if (bOverride_VignetteIntensity)
			InOutHandle["VignetteIntensity"] = VignetteIntensity;

		// VignetteColor (Override된 경우만)
		if (bOverride_VignetteColor)
		{
			JSON ColorJSON;
			ColorJSON["X"] = VignetteColor.X;
			ColorJSON["Y"] = VignetteColor.Y;
			ColorJSON["Z"] = VignetteColor.Z;
			InOutHandle["VignetteColor"] = ColorJSON;
		}
	}
}
```

---

### 5.2 Phase 2: FMinimalViewInfo 수정

**파일**: `Engine/Source/Global/CoreTypes.h`

**변경 전** (Line 35-60):
```cpp
struct FMinimalViewInfo
{
	// 변환
	FVector Location;
	FQuaternion Rotation;

	// 투영 파라미터
	float FOV;
	float AspectRatio;
	float NearClipPlane;
	float FarClipPlane;
	float OrthoWidth;
	bool bUsePerspectiveProjection;

	// 기본 생성자
	FMinimalViewInfo()
		: Location(FVector::ZeroVector())
		, Rotation(FQuaternion::Identity())
		, FOV(90.0f)
		, AspectRatio(16.0f / 9.0f)
		, NearClipPlane(1.0f)
		, FarClipPlane(10000.0f)
		, OrthoWidth(1000.0f)
		, bUsePerspectiveProjection(true)
	{
	}
	// ...
};
```

**변경 후**:
```cpp
struct FMinimalViewInfo
{
	// 변환
	FVector Location;
	FQuaternion Rotation;

	// 투영 파라미터
	float FOV;
	float AspectRatio;
	float NearClipPlane;
	float FarClipPlane;
	float OrthoWidth;
	bool bUsePerspectiveProjection;

	// ===== 후처리 설정 (새로 추가) =====
	FPostProcessSettings PostProcessSettings;

	// 기본 생성자
	FMinimalViewInfo()
		: Location(FVector::ZeroVector())
		, Rotation(FQuaternion::Identity())
		, FOV(90.0f)
		, AspectRatio(16.0f / 9.0f)
		, NearClipPlane(1.0f)
		, FarClipPlane(10000.0f)
		, OrthoWidth(1000.0f)
		, bUsePerspectiveProjection(true)
		, PostProcessSettings()  // 기본값 사용
	{
	}

	// FCameraConstants로 변환
	FCameraConstants ToCameraConstants() const;
};
```

---

### 5.3 Phase 2: UCameraComponent 수정

**파일**: `Engine/Source/Component/Camera/Public/CameraComponent.h`

**추가 멤버** (Line 28 이후):
```cpp
private:
	// 투영 파라미터
	float FieldOfView;
	float AspectRatio;
	float NearClipPlane;
	float FarClipPlane;
	bool bUsePerspectiveProjection;
	float OrthoWidth;

	// ===== 후처리 설정 (새로 추가) =====
	FPostProcessSettings PostProcessSettings;

public:
	// ... 기존 메서드 ...

	// ===== PostProcessSettings Getter/Setter (새로 추가) =====
	FPostProcessSettings& GetPostProcessSettings() { return PostProcessSettings; }
	const FPostProcessSettings& GetPostProcessSettings() const { return PostProcessSettings; }
```

**파일**: `Engine/Source/Component/Camera/Private/CameraComponent.cpp`

**GetCameraView() 수정**:

```cpp
void UCameraComponent::GetCameraView(FMinimalViewInfo& OutPOV) const
{
	// 기존 코드
	OutPOV.Location = GetWorldLocation();
	OutPOV.Rotation = GetWorldRotationAsQuaternion();
	OutPOV.FOV = FieldOfView;
	OutPOV.AspectRatio = AspectRatio;
	OutPOV.NearClipPlane = NearClipPlane;
	OutPOV.FarClipPlane = FarClipPlane;
	OutPOV.bUsePerspectiveProjection = bUsePerspectiveProjection;
	OutPOV.OrthoWidth = OrthoWidth;

	// ===== 새로 추가: PostProcessSettings 복사 =====
	OutPOV.PostProcessSettings = PostProcessSettings;
}
```

**Serialize() 수정** (위임 구조):

```cpp
void UCameraComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	// 투영 파라미터 직렬화 (기존 코드)
	// ...

	// ===== PostProcessSettings 직렬화 위임 (새로 추가) =====
	if (bInIsLoading)
	{
		if (InOutHandle.hasKey("PostProcess"))
		{
			PostProcessSettings.Serialize(bInIsLoading, InOutHandle["PostProcess"]);
		}
	}
	else
	{
		JSON PPHandle;
		PostProcessSettings.Serialize(bInIsLoading, PPHandle);
		InOutHandle["PostProcess"] = PPHandle;
	}
}
```

**개선 포인트**:
- ✅ CameraComponent는 PostProcessSettings를 블랙박스로 취급
- ✅ PostProcessSettings 확장 시 CameraComponent 코드 수정 불필요
- ✅ 직렬화 로직이 FPostProcessSettings에 캡슐화됨

---

### 5.4 Phase 3: PlayerCameraManager 수정

**파일**: `Engine/Source/Actor/Private/PlayerCameraManager.cpp`

**UpdateBlending() 수정** (Line 268-312, PostProcessSettings 보간 추가):

```cpp
void APlayerCameraManager::UpdateBlending(float DeltaTime)
{
	BlendTimeRemaining -= DeltaTime;

	if (BlendTimeRemaining <= 0.0f)
	{
		// 블렌드 완료
		ViewTarget = PendingViewTarget;
		bIsBlending = false;
		BlendTimeRemaining = 0.0f;
	}
	else
	{
		float BlendAlpha = 1.0f - (BlendTimeRemaining / BlendTime);

		// 대기 중인 타겟 POV 업데이트
		if (PendingViewTarget.CameraComponent)
		{
			PendingViewTarget.CameraComponent->GetCameraView(PendingViewTarget.POV);
		}
		else if (PendingViewTarget.Target)
		{
			PendingViewTarget.POV.Location = PendingViewTarget.Target->GetActorLocation();
			PendingViewTarget.POV.Rotation = PendingViewTarget.Target->GetActorRotation();
		}

		// POV 보간 (기존 코드)
		CachedPOV.Location = Lerp(ViewTarget.POV.Location, PendingViewTarget.POV.Location, BlendAlpha);
		CachedPOV.Rotation.X = Lerp(ViewTarget.POV.Rotation.X, PendingViewTarget.POV.Rotation.X, BlendAlpha);
		CachedPOV.Rotation.Y = Lerp(ViewTarget.POV.Rotation.Y, PendingViewTarget.POV.Rotation.Y, BlendAlpha);
		CachedPOV.Rotation.Z = Lerp(ViewTarget.POV.Rotation.Z, PendingViewTarget.POV.Rotation.Z, BlendAlpha);
		CachedPOV.Rotation.W = Lerp(ViewTarget.POV.Rotation.W, PendingViewTarget.POV.Rotation.W, BlendAlpha);
		CachedPOV.Rotation.Normalize();
		CachedPOV.FOV = Lerp(ViewTarget.POV.FOV, PendingViewTarget.POV.FOV, BlendAlpha);
		CachedPOV.AspectRatio = Lerp(ViewTarget.POV.AspectRatio, PendingViewTarget.POV.AspectRatio, BlendAlpha);
		CachedPOV.NearClipPlane = Lerp(ViewTarget.POV.NearClipPlane, PendingViewTarget.POV.NearClipPlane, BlendAlpha);
		CachedPOV.FarClipPlane = Lerp(ViewTarget.POV.FarClipPlane, PendingViewTarget.POV.FarClipPlane, BlendAlpha);
		CachedPOV.OrthoWidth = Lerp(ViewTarget.POV.OrthoWidth, PendingViewTarget.POV.OrthoWidth, BlendAlpha);

		// ===== PostProcessSettings 보간 (새로 추가) =====
		CachedPOV.PostProcessSettings = FPostProcessSettings::Lerp(
			ViewTarget.POV.PostProcessSettings,
			PendingViewTarget.POV.PostProcessSettings,
			BlendAlpha
		);
	}
}
```

**Getter 추가** (`PlayerCameraManager.h`):

```cpp
public:
	// ===== 최종 카메라 접근 =====
	const FCameraConstants& GetCameraConstants() const { return CachedCameraConstants; }
	const FMinimalViewInfo& GetCameraCachePOV() const { return CachedPOV; }

	// ===== PostProcessSettings 접근 (새로 추가) =====
	const FPostProcessSettings& GetPostProcessSettings() const { return CachedPOV.PostProcessSettings; }
```

---

### 5.5 Phase 4: Renderer 통합 - Option A (권장)

**방법 A: FCameraConstants에 PostProcessSettings 추가**

**장점**: 한 곳에서 모든 카메라 데이터 관리
**단점**: FCameraConstants가 커짐

**파일**: `Engine/Source/Global/CoreTypes.h`

```cpp
struct FCameraConstants
{
	FCameraConstants() : NearClip(0), FarClip(0)
	{
		View = FMatrix::Identity();
		Projection = FMatrix::Identity();
	}

	FMatrix View;
	FMatrix Projection;
	FVector ViewWorldLocation;
	float NearClip;
	float FarClip;

	// ===== 후처리 설정 (새로 추가) =====
	FPostProcessSettings PostProcessSettings;
};
```

**ToCameraConstants() 수정**:

```cpp
FCameraConstants FMinimalViewInfo::ToCameraConstants() const
{
	FCameraConstants Result;

	// View 행렬 구성 (기존 코드)
	// ...

	// Projection 행렬 구성 (기존 코드)
	// ...

	Result.ViewWorldLocation = Location;
	Result.NearClip = NearClipPlane;
	Result.FarClip = FarClipPlane;

	// ===== PostProcessSettings 복사 (새로 추가) =====
	Result.PostProcessSettings = PostProcessSettings;

	return Result;
}
```

---

### 5.6 Phase 4: VignettePass 수정

**파일**: `Engine/Source/Render/RenderPass/Public/VignettePass.h`

**변경**:
```cpp
class FVignettePass : public FPostProcessPass
{
public:
	FVignettePass(UPipeline* InPipeline, UDeviceResources* InDeviceResources);
	virtual ~FVignettePass();

protected:
	// ===== 변경: PostProcessSettings를 인자로 받도록 수정 =====
	void UpdateConstants(const FPostProcessSettings& InPostProcessSettings);

private:
	Microsoft::WRL::ComPtr<ID3D11Buffer> VignetteConstantBuffer = nullptr;
};
```

**파일**: `Engine/Source/Render/RenderPass/Private/VignettePass.cpp`

**변경 전**:
```cpp
void FVignettePass::UpdateConstants()
{
	FVignetteConstants VignetteConstants = {};
	VignetteConstants.VignetteColor = FVector(0.0f, 1.0f, 1.0f);  // ❌ 하드코딩
	VignetteConstants.VignetteIntensity = 0.5f;                   // ❌ 하드코딩

	FRenderResourceFactory::UpdateConstantBufferData(VignetteConstantBuffer.Get(), VignetteConstants);
	Pipeline->SetConstantBuffer(0, EShaderType::PS, VignetteConstantBuffer.Get());
}
```

**변경 후** (디버깅 로그 포함):
```cpp
void FVignettePass::UpdateConstants(const FPostProcessSettings& InPostProcessSettings)
{
	FVignetteConstants VignetteConstants = {};

	// ===== PostProcessSettings로부터 값 가져오기 =====
	if (InPostProcessSettings.bOverride_VignetteIntensity)
	{
		VignetteConstants.VignetteIntensity = InPostProcessSettings.VignetteIntensity;
	}
	else
	{
		VignetteConstants.VignetteIntensity = 0.0f;  // 기본값 (효과 없음)
	}

	if (InPostProcessSettings.bOverride_VignetteColor)
	{
		VignetteConstants.VignetteColor = InPostProcessSettings.VignetteColor;
	}
	else
	{
		VignetteConstants.VignetteColor = FVector(0.0f, 0.0f, 0.0f);  // 기본값 (검은색)
	}

	// ===== 디버깅 로그 (카메라 소스 추적용) =====
	UE_LOG_DEBUG("VignettePass: BlendWeight=%.2f, Override_I=%d, Intensity=%.2f, Color=(%.2f,%.2f,%.2f)",
		InPostProcessSettings.BlendWeight,
		InPostProcessSettings.bOverride_VignetteIntensity,
		VignetteConstants.VignetteIntensity,
		VignetteConstants.VignetteColor.X,
		VignetteConstants.VignetteColor.Y,
		VignetteConstants.VignetteColor.Z);

	FRenderResourceFactory::UpdateConstantBufferData(VignetteConstantBuffer.Get(), VignetteConstants);
	Pipeline->SetConstantBuffer(0, EShaderType::PS, VignetteConstantBuffer.Get());
}
```

**개선 포인트**:
- ✅ BlendWeight, Override 상태까지 로그 출력
- ✅ 나중에 multi-camera 디버깅 시 유용

---

### 5.7 Phase 4: Renderer 호출 수정

**파일**: `Engine/Source/Render/Renderer/Private/Renderer.cpp`

**위치**: `RenderLevel()` 함수에서 VignettePass 호출 부분

**변경 전** (예상):
```cpp
// 후처리 패스
VignettePass->Render(Context);
```

**변경 후**:
```cpp
// ===== PostProcessSettings 가져오기 =====
const FPostProcessSettings* PostProcessSettings = nullptr;
if ((WorldType == EWorldType::Game || WorldType == EWorldType::PIE) && CameraManager != nullptr)
{
	PostProcessSettings = &CameraManager->GetPostProcessSettings();
}
else
{
	// Editor 모드는 기본 PostProcessSettings 사용 (또는 EditorCamera에서 가져오기)
	static FPostProcessSettings DefaultSettings;  // 정적 변수로 생성
	PostProcessSettings = &DefaultSettings;
}

// 후처리 패스 (PostProcessSettings 전달)
VignettePass->UpdateConstants(*PostProcessSettings);
VignettePass->Render(Context);
```

**대안**: VignettePass::Render()가 PostProcessSettings를 받도록 수정

```cpp
// VignettePass.h
virtual void Render(const FRenderingContext& Context, const FPostProcessSettings& InPostProcessSettings);

// VignettePass.cpp
void FVignettePass::Render(const FRenderingContext& Context, const FPostProcessSettings& InPostProcessSettings)
{
	UpdateConstants(InPostProcessSettings);

	// 기존 렌더링 코드...
	Pipeline->SetVertexShader(VertexShader.Get());
	Pipeline->SetPixelShader(PixelShader.Get());
	// ...
}
```

---

## 6. 테스트 계획

### 6.1 기본 동작 테스트

**목표**: PostProcessSettings가 카메라에서 Renderer로 제대로 전달되는지 확인

**테스트 시나리오**:

1. **Actor 생성**:
   ```cpp
   // BeginPlay 또는 테스트 코드
   AActor* CameraActor = World->SpawnActor(AActor::StaticClass());
   UCameraComponent* Camera = NewObject<UCameraComponent>(CameraActor);
   CameraActor->SetRootComponent(Camera);
   ```

2. **PostProcessSettings 설정**:
   ```cpp
   FPostProcessSettings& PPSettings = Camera->GetPostProcessSettings();
   PPSettings.bOverride_VignetteIntensity = true;
   PPSettings.VignetteIntensity = 0.8f;  // 강한 비네트
   PPSettings.bOverride_VignetteColor = true;
   PPSettings.VignetteColor = FVector(1.0f, 0.0f, 0.0f);  // 빨간색 비네트
   ```

3. **CameraManager 설정**:
   ```cpp
   APlayerCameraManager* CameraManager = World->GetCameraManager();
   CameraManager->SetViewTarget(CameraActor);
   ```

4. **PIE 실행**:
   - PIE 모드 진입
   - 화면 가장자리에 빨간색 비네트 효과 확인

**예상 결과**:
- ✓ 화면 가장자리가 붉게 어두워짐
- ✓ VignetteIntensity 0.8에 해당하는 강도

---

### 6.2 Override 플래그 테스트

**목표**: Override 플래그가 제대로 작동하는지 확인

**테스트 시나리오 1**: Override 없음
```cpp
FPostProcessSettings& PPSettings = Camera->GetPostProcessSettings();
PPSettings.bOverride_VignetteIntensity = false;  // Override 비활성화
PPSettings.VignetteIntensity = 1.0f;  // 이 값은 무시되어야 함
```

**예상 결과**:
- ✓ 비네트 효과 없음 (VignetteIntensity = 0.0)

**테스트 시나리오 2**: Override 활성화
```cpp
PPSettings.bOverride_VignetteIntensity = true;  // Override 활성화
PPSettings.VignetteIntensity = 0.3f;
```

**예상 결과**:
- ✓ 약한 비네트 효과 (VignetteIntensity = 0.3)

---

### 6.3 카메라 블렌딩 테스트 (선택적)

**목표**: 두 카메라 간 블렌딩 시 PostProcessSettings도 보간되는지 확인

**테스트 시나리오**:

1. **Camera A 설정**:
   ```cpp
   UCameraComponent* CameraA = ...;
   CameraA->GetPostProcessSettings().bOverride_VignetteIntensity = true;
   CameraA->GetPostProcessSettings().VignetteIntensity = 0.0f;  // 효과 없음
   ```

2. **Camera B 설정**:
   ```cpp
   UCameraComponent* CameraB = ...;
   CameraB->GetPostProcessSettings().bOverride_VignetteIntensity = true;
   CameraB->GetPostProcessSettings().VignetteIntensity = 1.0f;  // 강한 효과
   ```

3. **블렌딩 시작**:
   ```cpp
   CameraManager->SetViewTarget(CameraB, 2.0f);  // 2초 블렌드
   ```

**예상 결과**:
- ✓ 2초에 걸쳐 VignetteIntensity가 0.0 → 1.0으로 부드럽게 증가
- ✓ 화면이 점진적으로 어두워짐

---

### 6.4 런타임 변경 테스트

**목표**: 런타임에 PostProcessSettings를 변경할 수 있는지 확인

**테스트 코드** (Tick 또는 입력 핸들러):
```cpp
void AMyTestActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 시간에 따라 VignetteIntensity 변경 (펄스 효과)
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
- ✓ 비네트 효과가 주기적으로 강해졌다 약해짐 (펄스)

---

### 6.5 디버깅 체크리스트

**PostProcessSettings가 전달되지 않는 경우**:

1. **UCameraComponent::GetCameraView() 확인**:
   - PostProcessSettings 복사 코드가 있는지 확인
   - 브레이크포인트 설정: `OutPOV.PostProcessSettings = PostProcessSettings;`

2. **PlayerCameraManager::UpdateViewTarget() 확인**:
   - `CachedPOV.PostProcessSettings`에 값이 설정되는지 확인

3. **Renderer::RenderLevel() 확인**:
   - `CameraManager->GetPostProcessSettings()` 호출 여부
   - PostProcessSettings가 VignettePass로 전달되는지 확인

4. **VignettePass::UpdateConstants() 확인**:
   - `InPostProcessSettings.bOverride_VignetteIntensity` 값 확인
   - `VignetteConstants.VignetteIntensity` 값 확인
   - GPU 상수 버퍼 업로드 확인

5. **로그 추가**:
   ```cpp
   // VignettePass.cpp
   UE_LOG_DEBUG("Vignette: Override=%d, Intensity=%.2f, Color=(%.2f, %.2f, %.2f)",
       InPostProcessSettings.bOverride_VignetteIntensity,
       VignetteConstants.VignetteIntensity,
       VignetteConstants.VignetteColor.X,
       VignetteConstants.VignetteColor.Y,
       VignetteConstants.VignetteColor.Z);
   ```

---

## 7. 향후 확장

### 7.1 추가 후처리 효과

**Bloom (블룸)**:

```cpp
struct FPostProcessSettings
{
	// Vignette
	bool bOverride_VignetteIntensity;
	float VignetteIntensity;
	// ...

	// ===== Bloom (새로 추가) =====
	bool bOverride_BloomIntensity;
	float BloomIntensity;          // 블룸 강도 [0.0, ...]

	bool bOverride_BloomThreshold;
	float BloomThreshold;          // 밝기 임계값 [0.0, 1.0]

	bool bOverride_BloomSize;
	float BloomSize;               // 블룸 크기 (텍스처 다운스케일 레벨)
};
```

**Depth of Field (피사계 심도)**:

```cpp
struct FPostProcessSettings
{
	// ...

	// ===== Depth of Field (새로 추가) =====
	bool bOverride_DepthOfFieldFocalDistance;
	float DepthOfFieldFocalDistance;  // 초점 거리 (월드 단위)

	bool bOverride_DepthOfFieldFocalRegion;
	float DepthOfFieldFocalRegion;    // 초점 영역 크기

	bool bOverride_DepthOfFieldNearTransitionRegion;
	float DepthOfFieldNearTransitionRegion;  // Near 블러 전환 범위

	bool bOverride_DepthOfFieldFarTransitionRegion;
	float DepthOfFieldFarTransitionRegion;   // Far 블러 전환 범위

	bool bOverride_DepthOfFieldScale;
	float DepthOfFieldScale;          // 블러 강도
};
```

**Color Grading (색보정)**:

```cpp
struct FPostProcessSettings
{
	// ...

	// ===== Color Grading (새로 추가) =====
	bool bOverride_ColorGain;
	FVector ColorGain;            // RGB 게인 (곱셈)

	bool bOverride_ColorGamma;
	FVector ColorGamma;           // RGB 감마

	bool bOverride_ColorSaturation;
	float ColorSaturation;        // 채도 [0.0, 2.0]

	bool bOverride_ColorContrast;
	float ColorContrast;          // 대비 [0.0, 2.0]
};
```

---

### 7.2 UCameraModifier로 PostProcessSettings 제어

**예시**: 데미지 받았을 때 빨간색 비네트

```cpp
// UCameraModifier_DamageEffect.h
class UCameraModifier_DamageEffect : public UCameraModifier
{
	// ...

	virtual bool ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV) override;

	void TriggerDamage(float DamageAmount);

private:
	float DamageIntensity;  // 현재 데미지 강도 [0.0, 1.0]
	float DecayRate;        // 감쇠 속도
};

// UCameraModifier_DamageEffect.cpp
bool UCameraModifier_DamageEffect::ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV)
{
	if (DamageIntensity <= 0.0f)
		return false;

	// 데미지 강도 감쇠
	DamageIntensity -= DecayRate * DeltaTime;
	DamageIntensity = FMath::Max(DamageIntensity, 0.0f);

	// ===== PostProcessSettings 수정 =====
	InOutPOV.PostProcessSettings.bOverride_VignetteIntensity = true;
	InOutPOV.PostProcessSettings.VignetteIntensity = DamageIntensity;

	InOutPOV.PostProcessSettings.bOverride_VignetteColor = true;
	InOutPOV.PostProcessSettings.VignetteColor = FVector(1.0f, 0.0f, 0.0f);  // 빨간색

	return true;
}

void UCameraModifier_DamageEffect::TriggerDamage(float DamageAmount)
{
	DamageIntensity = FMath::Clamp(DamageAmount / 100.0f, 0.0f, 1.0f);
	EnableModifier();  // 모디파이어 활성화
}
```

**사용법**:
```cpp
// 게임 코드
UCameraModifier_DamageEffect* DamageModifier = Cast<UCameraModifier_DamageEffect>(
	CameraManager->AddCameraModifier(UCameraModifier_DamageEffect::StaticClass())
);

// 플레이어가 데미지를 받았을 때
DamageModifier->TriggerDamage(50.0f);  // 50 데미지 → 빨간색 비네트
```

---

### 7.3 Post Process Volume (향후)

Unreal Engine처럼 공간 기반 PostProcessSettings:

```cpp
class APostProcessVolume : public AActor
{
	FPostProcessSettings Settings;
	float Priority;           // 우선순위
	float BlendRadius;        // 블렌딩 반경
	float BlendWeight;        // 블렌딩 가중치
	bool bUnbound;            // 전역 적용 여부

	// ...
};
```

**로직**:
1. CameraManager가 현재 카메라 위치에서 겹치는 모든 PostProcessVolume 찾기
2. Priority와 BlendWeight로 PostProcessSettings 블렌딩
3. 최종 PostProcessSettings를 Renderer에 전달

---

## 8. 변경 이력

| 날짜 | 작성자 | 변경 내용 |
|------|--------|-----------|
| 2025-11-05 | Claude | 초기 작업 계획 작성 |

---

**문서 끝**

## 요약

**총 구현 시간**: 약 3.5시간

**핵심 변경 사항**:
1. `FPostProcessSettings` 구조체 추가 (Vignette 설정 포함)
2. `FMinimalViewInfo`, `UCameraComponent`에 PostProcessSettings 추가
3. `VignettePass`가 PostProcessSettings로부터 값을 읽도록 수정
4. Renderer가 CameraManager로부터 PostProcessSettings를 받아 VignettePass에 전달

**확장성**:
- Bloom, DOF, Color Grading 등 추가 후처리 효과 쉽게 추가 가능
- UCameraModifier로 PostProcessSettings 동적 제어 가능
- 향후 PostProcessVolume 시스템 구현 가능
