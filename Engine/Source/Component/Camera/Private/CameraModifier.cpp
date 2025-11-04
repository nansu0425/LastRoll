#include "pch.h"
#include "Component/Camera/Public/CameraModifier.h"
#include "Global/CoreTypes.h"

IMPLEMENT_CLASS(UCameraModifier, UObject)

UCameraModifier::UCameraModifier()
	: CameraOwner(nullptr)
	, AlphaInTime(0.2f)
	, AlphaOutTime(0.2f)
	, Alpha(0.0f)
	, BlendMode(ECameraModifierBlendMode::Disabled)
	, bDisabled(true)
	, Priority(127)  // Default priority (middle value)
{
}

UCameraModifier::~UCameraModifier()
{
}

void UCameraModifier::Initialize(APlayerCameraManager* InOwner)
{
	CameraOwner = InOwner;
}

void UCameraModifier::UpdateAlpha(float DeltaTime)
{
	if (BlendMode == ECameraModifierBlendMode::Disabled)
	{
		Alpha = 0.0f;
		return;
	}

	if (BlendMode == ECameraModifierBlendMode::BlendingIn)
	{
		if (AlphaInTime > 0.0f)
		{
			// Blend in over time
			Alpha += DeltaTime / AlphaInTime;
			if (Alpha >= 1.0f)
			{
				Alpha = 1.0f;
				BlendMode = ECameraModifierBlendMode::Active;
			}
		}
		else
		{
			// Instant blend in
			Alpha = 1.0f;
			BlendMode = ECameraModifierBlendMode::Active;
		}
	}
	else if (BlendMode == ECameraModifierBlendMode::BlendingOut)
	{
		if (AlphaOutTime > 0.0f)
		{
			// Blend out over time
			Alpha -= DeltaTime / AlphaOutTime;
			if (Alpha <= 0.0f)
			{
				Alpha = 0.0f;
				BlendMode = ECameraModifierBlendMode::Disabled;
				bDisabled = true;
			}
		}
		else
		{
			// Instant blend out
			Alpha = 0.0f;
			BlendMode = ECameraModifierBlendMode::Disabled;
			bDisabled = true;
		}
	}
	// ECameraModifierBlendMode::Active: Alpha stays at 1.0
}

bool UCameraModifier::ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV)
{
	// Base implementation: no modification
	return false;
}

void UCameraModifier::EnableModifier()
{
	bDisabled = false;

	if (BlendMode == ECameraModifierBlendMode::Disabled)
	{
		// Start blending in
		BlendMode = ECameraModifierBlendMode::BlendingIn;
		Alpha = 0.0f;
	}
	else if (BlendMode == ECameraModifierBlendMode::BlendingOut)
	{
		// Reverse direction: start blending in from current alpha
		BlendMode = ECameraModifierBlendMode::BlendingIn;
	}
	// If already BlendingIn or Active, do nothing
}

void UCameraModifier::DisableModifier(bool bImmediate)
{
	if (bImmediate)
	{
		// Immediate disable
		bDisabled = true;
		Alpha = 0.0f;
		BlendMode = ECameraModifierBlendMode::Disabled;
	}
	else
	{
		// Blend out over time
		if (BlendMode != ECameraModifierBlendMode::Disabled)
		{
			BlendMode = ECameraModifierBlendMode::BlendingOut;
		}
	}
}
