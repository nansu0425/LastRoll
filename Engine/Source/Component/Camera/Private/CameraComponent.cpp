#include "pch.h"
#include "Component/Camera/Public/CameraComponent.h"
#include "Global/CoreTypes.h"
#include <json.hpp>

using JSON = json::JSON;

IMPLEMENT_CLASS(UCameraComponent, USceneComponent)

UCameraComponent::UCameraComponent()
	: FieldOfView(90.0f)
	, AspectRatio(16.0f / 9.0f)
	, NearClipPlane(1.0f)
	, FarClipPlane(10000.0f)
	, bUsePerspectiveProjection(true)
	, OrthoWidth(1000.0f)
	, TargetAspectRatio(0.0f)
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
	OutPOV.TargetAspectRatio = TargetAspectRatio;

	// ===== PostProcessSettings 복사 =====
	OutPOV.PostProcessSettings = PostProcessSettings;
}

void UCameraComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	// 투영 파라미터 직렬화 (기존 TODO 구현)
	if (bInIsLoading)
	{
		if (InOutHandle.hasKey("FieldOfView"))
			FieldOfView = static_cast<float>(InOutHandle["FieldOfView"].ToFloat());
		if (InOutHandle.hasKey("AspectRatio"))
			AspectRatio = static_cast<float>(InOutHandle["AspectRatio"].ToFloat());
		if (InOutHandle.hasKey("NearClipPlane"))
			NearClipPlane = static_cast<float>(InOutHandle["NearClipPlane"].ToFloat());
		if (InOutHandle.hasKey("FarClipPlane"))
			FarClipPlane = static_cast<float>(InOutHandle["FarClipPlane"].ToFloat());
	}
	else
	{
		InOutHandle["FieldOfView"] = FieldOfView;
		InOutHandle["AspectRatio"] = AspectRatio;
		InOutHandle["NearClipPlane"] = NearClipPlane;
		InOutHandle["FarClipPlane"] = FarClipPlane;
	}

	// ===== PostProcessSettings 직렬화 위임 =====
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
