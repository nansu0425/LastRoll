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
	// Get world transform from scene component hierarchy
	OutPOV.Location = GetWorldLocation();
	OutPOV.Rotation = GetWorldRotationAsQuaternion();

	// Copy projection settings
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

	// TODO: Implement JSON serialization for camera properties
	// For now, use default values
	// This will be completed when JSON serialization is needed
}
