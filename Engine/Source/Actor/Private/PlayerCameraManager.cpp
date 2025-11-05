#include "pch.h"
#include "Actor/Public/PlayerCameraManager.h"
#include "Component/Camera/Public/CameraComponent.h"
#include "Component/Camera/Public/CameraModifier.h"
#include "Global/Function.h"

IMPLEMENT_CLASS(APlayerCameraManager, AActor)

APlayerCameraManager::APlayerCameraManager()
	: BlendTime(0.0f)
	, BlendTimeRemaining(0.0f)
	, bIsBlending(false)
	, FadeColor(0.0f, 0.0f, 0.0f)
	, FadeAmount(0.0f)
	, FadeAlpha(0.0f, 0.0f)
	, FadeTime(0.0f)
	, FadeTimeRemaining(0.0f)
	, bIsFading(false)
	, CameraStyle(FName("Default"))
{
	bCanEverTick = true;
	bTickInEditor = false; // 게임/PIE 모드에서만 틱
}

APlayerCameraManager::~APlayerCameraManager()
{
	// 모디파이어 정리
	for (UCameraModifier* Modifier : ModifierList)
	{
		if (Modifier)
		{
			delete Modifier;
		}
	}
	ModifierList.clear();
}

void APlayerCameraManager::BeginPlay()
{
	Super::BeginPlay();

	// 기본 카메라 초기화
	if (!ViewTarget.Target)
	{
		// 기본 카메라 위치 사용
		ViewTarget.POV.Location = FVector(0, 0, 500);
		ViewTarget.POV.Rotation = FQuaternion::Identity();
	}
}

void APlayerCameraManager::SetViewTarget(AActor* NewTarget, float InBlendTime)
{
	if (!NewTarget)
	{
		UE_LOG_WARNING("APlayerCameraManager::SetViewTarget - NewTarget이 null입니다");
		return;
	}

	if (InBlendTime > 0.0f)
	{
		// 새 타겟으로 블렌딩 시작
		PendingViewTarget.Target = NewTarget;

		// 새 타겟에서 카메라 컴포넌트 찾기
		PendingViewTarget.CameraComponent = Cast<UCameraComponent>(
			NewTarget->GetComponentByClass(UCameraComponent::StaticClass())
		);

		// 대기 중인 타겟에서 초기 POV 가져오기
		if (PendingViewTarget.CameraComponent)
		{
			PendingViewTarget.CameraComponent->GetCameraView(PendingViewTarget.POV);
		}
		else
		{
			// 카메라 컴포넌트 없음: 액터 변환 사용
			PendingViewTarget.POV.Location = NewTarget->GetActorLocation();
			PendingViewTarget.POV.Rotation = NewTarget->GetActorRotation();
		}

		BlendTime = InBlendTime;
		BlendTimeRemaining = InBlendTime;
		bIsBlending = true;

		UE_LOG_DEBUG("APlayerCameraManager: 새 뷰 타겟으로 블렌드 시작 (%.2f초)", InBlendTime);
	}
	else
	{
		// 새 타겟으로 즉시 전환
		ViewTarget.Target = NewTarget;
		ViewTarget.CameraComponent = Cast<UCameraComponent>(
			NewTarget->GetComponentByClass(UCameraComponent::StaticClass())
		);

		if (ViewTarget.CameraComponent)
		{
			ViewTarget.CameraComponent->GetCameraView(ViewTarget.POV);
		}
		else
		{
			ViewTarget.POV.Location = NewTarget->GetActorLocation();
			ViewTarget.POV.Rotation = NewTarget->GetActorRotation();
		}

		bIsBlending = false;

		UE_LOG_DEBUG("APlayerCameraManager: 새 뷰 타겟으로 즉시 전환");
	}
}

UCameraModifier* APlayerCameraManager::AddCameraModifier(UClass* ModifierClass)
{
	if (!ModifierClass)
	{
		UE_LOG_ERROR("APlayerCameraManager::AddCameraModifier - ModifierClass가 null입니다");
		return nullptr;
	}

	// ModifierClass가 UCameraModifier의 서브클래스인지 확인
	if (!ModifierClass->IsChildOf(UCameraModifier::StaticClass()))
	{
		UE_LOG_ERROR("APlayerCameraManager::AddCameraModifier - %s는 UCameraModifier의 서브클래스가 아닙니다", 
			ModifierClass->GetName().ToString().c_str());
		return nullptr;
	}

	// NewObject를 사용하여 새 모디파이어 인스턴스 생성
	UCameraModifier* NewModifier = Cast<UCameraModifier>(NewObject(ModifierClass, this));
	
	if (!NewModifier)
	{
		UE_LOG_ERROR("APlayerCameraManager::AddCameraModifier - 모디파이어 인스턴스 생성 실패");
		return nullptr;
	}

	// 모디파이어 초기화
	NewModifier->Initialize(this);

	// 리스트에 추가
	ModifierList.push_back(NewModifier);

	UE_LOG_DEBUG("APlayerCameraManager: 카메라 모디파이어 추가됨 (클래스: %s, 우선순위: %d)",
		ModifierClass->GetName().ToString().c_str(), NewModifier->GetPriority());

	return NewModifier;
}

bool APlayerCameraManager::RemoveCameraModifier(UCameraModifier* Modifier)
{
	if (!Modifier)
		return false;

	// 찾아서 제거
	for (auto It = ModifierList.begin(); It != ModifierList.end(); ++It)
	{
		if (*It == Modifier)
		{
			delete Modifier;
			ModifierList.erase(It);
			UE_LOG_DEBUG("APlayerCameraManager: 카메라 모디파이어 제거됨");
			return true;
		}
	}

	return false;
}

UCameraModifier* APlayerCameraManager::FindCameraModifierByClass(UClass* ModifierClass) const
{
	if (!ModifierClass)
		return nullptr;

	for (UCameraModifier* Modifier : ModifierList)
	{
		if (Modifier && Modifier->GetClass() == ModifierClass)
		{
			return Modifier;
		}
	}

	return nullptr;
}

void APlayerCameraManager::ClearAllCameraModifiers()
{
	for (UCameraModifier* Modifier : ModifierList)
	{
		if (Modifier)
		{
			delete Modifier;
		}
	}
	ModifierList.clear();

	UE_LOG_DEBUG("APlayerCameraManager: 모든 카메라 모디파이어 제거됨");
}

void APlayerCameraManager::StartCameraFade(float FromAlpha, float ToAlpha, float Duration, FVector Color)
{
	FadeColor = Color;
	FadeAmount = FromAlpha;
	FadeAlpha = FVector2(FromAlpha, ToAlpha);
	FadeTime = Duration;
	FadeTimeRemaining = Duration;
	bIsFading = true;

	UE_LOG_DEBUG("APlayerCameraManager: 카메라 페이드 시작 (%.2f→%.2f, %.2f초)", 
		FromAlpha, ToAlpha, Duration);
}

void APlayerCameraManager::StopCameraFade()
{
	bIsFading = false;
	FadeTimeRemaining = 0.0f;
	FadeAlpha = FVector2(0.0f, 0.0f);

	UE_LOG_DEBUG("APlayerCameraManager: 카메라 페이드 중지");
}

void APlayerCameraManager::UpdateCamera(float DeltaTime)
{
	// 1단계: 현재 ViewTarget에서 POV 업데이트
	UpdateViewTarget(DeltaTime);

	// 2단계: 블렌딩 중이면 ViewTarget 간 보간
	if (bIsBlending)
	{
		UpdateBlending(DeltaTime);
	}

	// 3단계: 카메라 모디파이어 체인 적용
	ApplyCameraModifiers(DeltaTime);

	// 4단계: 페이딩 업데이트
	if (bIsFading)
	{
		UpdateFading(DeltaTime);
	}

	// 5단계: 최종 POV를 카메라 상수(View/Projection 행렬)로 변환
	UpdateCameraConstants();
}

void APlayerCameraManager::UpdateViewTarget(float DeltaTime)
{
	if (ViewTarget.Target && ViewTarget.CameraComponent)
	{
		// ViewTarget에 카메라 컴포넌트가 있으면: POV 가져오기
		ViewTarget.CameraComponent->GetCameraView(ViewTarget.POV);
	}
	else if (ViewTarget.Target)
	{
		// 카메라 컴포넌트 없으면: 액터 변환 사용
		ViewTarget.POV.Location = ViewTarget.Target->GetActorLocation();
		ViewTarget.POV.Rotation = ViewTarget.Target->GetActorRotation();
	}
	else
	{
		// 뷰 타겟 없음: 기본 카메라 사용
		ViewTarget.POV.Location = FVector(0, 0, 500);
		ViewTarget.POV.Rotation = FQuaternion::Identity();
	}

	ViewTarget.POV.OverlayColor = FVector4(FadeColor, FadeAmount);

	// 캐시된 POV에 저장
	CachedPOV = ViewTarget.POV;
}

void APlayerCameraManager::UpdateBlending(float DeltaTime)
{
	BlendTimeRemaining -= DeltaTime;

	if (BlendTimeRemaining <= 0.0f)
	{
		// 블렌드 완료
		ViewTarget = PendingViewTarget;
		bIsBlending = false;
		BlendTimeRemaining = 0.0f;

		UE_LOG_DEBUG("APlayerCameraManager: 블렌드 완료");
	}
	else
	{
		// 현재와 대기 중인 것 사이에서 선형 보간
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

		// POV 보간
		CachedPOV.Location = Lerp(ViewTarget.POV.Location, PendingViewTarget.POV.Location, BlendAlpha);
		// TODO: 부드러운 회전 블렌딩을 위한 적절한 쿼터니언 slerp 구현
		// 현재는 단순 선형 보간 사용
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

		// ===== PostProcessSettings 보간 =====
		CachedPOV.PostProcessSettings = FPostProcessSettings::Lerp(
			ViewTarget.POV.PostProcessSettings,
			PendingViewTarget.POV.PostProcessSettings,
			BlendAlpha
		);
	}
}

void APlayerCameraManager::ApplyCameraModifiers(float DeltaTime)
{
	if (ModifierList.empty())
		return;

	// 우선순위로 모디파이어 정렬 (오름차순: 낮은 우선순위 먼저)
	std::sort(ModifierList.begin(), ModifierList.end(),
		[](const UCameraModifier* A, const UCameraModifier* B) {
			return A->GetPriority() < B->GetPriority();
		});

	// 각 모디파이어를 순서대로 적용
	for (UCameraModifier* Modifier : ModifierList)
	{
		if (Modifier && !Modifier->IsDisabled())
		{
			// 블렌드 알파 업데이트
			Modifier->UpdateAlpha(DeltaTime);

			// POV에 모디파이어 적용
			if (Modifier->GetAlpha() > 0.0f)
			{
				Modifier->ModifyCamera(DeltaTime, CachedPOV);
			}
		}
	}
}

void APlayerCameraManager::UpdateFading(float DeltaTime)
{
	FadeTimeRemaining -= DeltaTime;

	if (FadeTimeRemaining <= 0.0f)
	{
		// 페이드 완료
		FadeAmount = FadeAlpha.Y;
		FadeTimeRemaining = 0.0f;
		bIsFading = false;
	}
	else
	{
		// 페이드 알파 보간
		float FadeBlendAlpha = 1.0f - (FadeTimeRemaining / FadeTime);
		FadeAmount = Lerp(FadeAlpha.X, FadeAlpha.Y, FadeBlendAlpha);
	}

	UE_LOG("FadeAmount: %f", FadeAmount);

	// TODO: 후처리 페이드 오버레이를 위해 URenderer와 통합
	// 현재는 렌더러가 사용할 수 있도록 페이드 데이터를 GetFadeAlpha()로 제공
}

void APlayerCameraManager::UpdateCameraConstants()
{
	// 최종 POV를 카메라 상수로 변환 (View/Projection 행렬)
	CachedCameraConstants = CachedPOV.ToCameraConstants();
}

void APlayerCameraManager::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	// TODO: 카메라 매니저에 대한 JSON 직렬화 구현
	// ViewTarget, 모디파이어, 페이드 상태는 런타임 상태이므로
	// BeginPlay 또는 게임플레이 코드에서 설정되어야 합니다
}
