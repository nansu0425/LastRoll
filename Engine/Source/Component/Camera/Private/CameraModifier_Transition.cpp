#include "pch.h"
#include "Component/Camera/Public/CameraModifier_Transition.h"
#include "Actor/Public/PlayerCameraManager.h"
#include "Global/Function.h"

IMPLEMENT_CLASS(UCameraModifier_Transition, UCameraModifier)

UCameraModifier_Transition::UCameraModifier_Transition()
	: TransitionDuration(0.0f)
	, TransitionTimeRemaining(0.0f)
	, TransitionTime(0.0f)
	, bIsTransitioning(false)
	, TimingCurve(FCubicBezierCurve::CreateLinear())
	, bUseTimingCurve(true)
{
	// Set default priority (higher than camera shake)
	Priority = 128;
}

UCameraModifier_Transition::~UCameraModifier_Transition()
{
}

void UCameraModifier_Transition::Initialize(APlayerCameraManager* InOwner)
{
	Super::Initialize(InOwner);
}

void UCameraModifier_Transition::StartTransition(
	const FMinimalViewInfo& InFromPOV,
	const FMinimalViewInfo& InToPOV,
	float InDuration,
	const FCubicBezierCurve& InTimingCurve)
{
	if (InDuration <= 0.0f)
	{
		UE_LOG_ERROR("UCameraModifier_Transition::StartTransition - Duration must be > 0 (given: %f)", InDuration);
		return;
	}

	StartPOV = InFromPOV;
	TargetPOV = InToPOV;
	TransitionDuration = InDuration;
	TransitionTimeRemaining = InDuration;
	TransitionTime = 0.0f;
	TimingCurve = InTimingCurve;
	bUseTimingCurve = true;
	bIsTransitioning = true;

	// CRITICAL: Enable modifier so Alpha becomes 1.0 and ModifyCamera() gets called
	EnableModifier();

	UE_LOG_INFO("Camera transition started: Duration=%.2fs", InDuration);
	UE_LOG_INFO("  StartPOV: Loc=(%.1f, %.1f, %.1f), FOV=%.1f",
		StartPOV.Location.X, StartPOV.Location.Y, StartPOV.Location.Z, StartPOV.FOV);
	UE_LOG_INFO("  TargetPOV: Loc=(%.1f, %.1f, %.1f), FOV=%.1f",
		TargetPOV.Location.X, TargetPOV.Location.Y, TargetPOV.Location.Z, TargetPOV.FOV);
}

void UCameraModifier_Transition::StartTransitionLinear(
	const FMinimalViewInfo& InFromPOV,
	const FMinimalViewInfo& InToPOV,
	float InDuration)
{
	if (InDuration <= 0.0f)
	{
		UE_LOG_ERROR("UCameraModifier_Transition::StartTransitionLinear - Duration must be > 0 (given: %f)", InDuration);
		return;
	}

	StartPOV = InFromPOV;
	TargetPOV = InToPOV;
	TransitionDuration = InDuration;
	TransitionTimeRemaining = InDuration;
	TransitionTime = 0.0f;
	bUseTimingCurve = false;
	bIsTransitioning = true;

	// CRITICAL: Enable modifier so Alpha becomes 1.0 and ModifyCamera() gets called
	EnableModifier();

	UE_LOG_INFO("Camera transition started (linear): Duration=%.2fs", InDuration);
}

void UCameraModifier_Transition::StopTransition()
{
	if (bIsTransitioning)
	{
		bIsTransitioning = false;
		TransitionTimeRemaining = 0.0f;
		TransitionTime = 0.0f;

		// Disable modifier when transition stops
		DisableModifier(false);  // false = instant disable

		UE_LOG_INFO("Camera transition stopped");
	}
}

void UCameraModifier_Transition::SetTimingCurve(const FCubicBezierCurve& InCurve)
{
	TimingCurve = InCurve;
}

bool UCameraModifier_Transition::ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV)
{
	if (!bIsTransitioning)
		return false;

	// Debug: Log that ModifyCamera is being called
	static int32 FrameCount = 0;
	if (FrameCount % 30 == 0)  // Log every 30 frames to avoid spam
	{
		UE_LOG_DEBUG("UCameraModifier_Transition::ModifyCamera() called - Frame %d", FrameCount);
	}
	FrameCount++;

	// Update transition time
	TransitionTimeRemaining -= DeltaTime;
	TransitionTime += DeltaTime;

	if (TransitionTimeRemaining <= 0.0f)
	{
		// Transition completed - snap to target
		InOutPOV.Location = TargetPOV.Location;
		InOutPOV.Rotation = TargetPOV.Rotation;
		InOutPOV.FOV = TargetPOV.FOV;
		InOutPOV.AspectRatio = TargetPOV.AspectRatio;
		InOutPOV.NearClipPlane = TargetPOV.NearClipPlane;
		InOutPOV.FarClipPlane = TargetPOV.FarClipPlane;

		bIsTransitioning = false;
		TransitionTimeRemaining = 0.0f;

		// Disable modifier when transition completes
		DisableModifier(false);  // false = instant disable

		UE_LOG_INFO("Camera transition completed");
		return true;
	}

	// Compute normalized time [0, 1]
	float NormalizedTime = TransitionTime / TransitionDuration;

	// Clamp to [0, 1] for safety
	NormalizedTime = Clamp(NormalizedTime, 0.0f, 1.0f);

	// Compute blend alpha from timing curve
	float BlendAlpha = ComputeBlendAlpha(NormalizedTime);

	// Debug: Log interpolation progress every 30 frames
	static int32 InterpFrameCount = 0;
	if (InterpFrameCount % 30 == 0)
	{
		UE_LOG_DEBUG("  Transition progress: Time=%.2f/%.2f, Alpha=%.3f",
			TransitionTime, TransitionDuration, BlendAlpha);
	}
	InterpFrameCount++;

	// Interpolate POV
	InOutPOV.Location = Lerp(StartPOV.Location, TargetPOV.Location, BlendAlpha);

	// IMPORTANT: Use Quaternion Slerp for smooth rotation
	InOutPOV.Rotation = FQuaternion::Slerp(StartPOV.Rotation, TargetPOV.Rotation, BlendAlpha);

	InOutPOV.FOV = Lerp(StartPOV.FOV, TargetPOV.FOV, BlendAlpha);
	InOutPOV.AspectRatio = Lerp(StartPOV.AspectRatio, TargetPOV.AspectRatio, BlendAlpha);
	InOutPOV.NearClipPlane = Lerp(StartPOV.NearClipPlane, TargetPOV.NearClipPlane, BlendAlpha);
	InOutPOV.FarClipPlane = Lerp(StartPOV.FarClipPlane, TargetPOV.FarClipPlane, BlendAlpha);

	return true;
}

float UCameraModifier_Transition::ComputeBlendAlpha(float NormalizedTime) const
{
	if (bUseTimingCurve)
	{
		// Sample Y from Bezier curve based on X (normalized time)
		return TimingCurve.SampleY(NormalizedTime);
	}
	else
	{
		// Linear interpolation
		return NormalizedTime;
	}
}
