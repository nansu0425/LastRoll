#include "pch.h"
#include "Component/Camera/Public/CameraComponent.h"
#include "Global/CoreTypes.h"

IMPLEMENT_CLASS(UCameraComponent, USceneComponent)

UCameraComponent::UCameraComponent()
	: FieldOfView(90.0f)
	, AspectRatio(16.0f / 9.0f)
	, NearClipPlane(1.0f)
	, FarClipPlane(10000.0f)
	, bUsePerspectiveProjection(true)
	, OrthoWidth(1000.0f)
{
}

UCameraComponent::~UCameraComponent()
{
}

void UCameraComponent::GetCameraView(FMinimalViewInfo& OutPOV) const
{
	// 씬 컴포넌트 계층에서 월드 변환 가져오기
	OutPOV.Location = GetWorldLocation();
	OutPOV.Rotation = GetWorldRotationAsQuaternion();

	// 투영 설정 복사
	OutPOV.FOV = FieldOfView;
	OutPOV.AspectRatio = AspectRatio;
	OutPOV.NearClipPlane = NearClipPlane;
	OutPOV.FarClipPlane = FarClipPlane;
	OutPOV.OrthoWidth = OrthoWidth;
	OutPOV.bUsePerspectiveProjection = bUsePerspectiveProjection;
}

void UCameraComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	// TODO: 카메라 속성에 대한 JSON 직렬화 구현
	// 현재는 기본값 사용
	// JSON 직렬화가 필요할 때 완성될 예정
}
