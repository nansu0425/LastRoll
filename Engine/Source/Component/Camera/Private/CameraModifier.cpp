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
	, Priority(127)  // 기본 우선순위 (중간 값)
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
			// 시간에 걸쳐 블렌드 인
			Alpha += DeltaTime / AlphaInTime;
			if (Alpha >= 1.0f)
			{
				Alpha = 1.0f;
				BlendMode = ECameraModifierBlendMode::Active;
			}
		}
		else
		{
			// 즉시 블렌드 인
			Alpha = 1.0f;
			BlendMode = ECameraModifierBlendMode::Active;
		}
	}
	else if (BlendMode == ECameraModifierBlendMode::BlendingOut)
	{
		if (AlphaOutTime > 0.0f)
		{
			// 시간에 걸쳐 블렌드 아웃
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
			// 즉시 블렌드 아웃
			Alpha = 0.0f;
			BlendMode = ECameraModifierBlendMode::Disabled;
			bDisabled = true;
		}
	}
	// ECameraModifierBlendMode::Active: Alpha는 1.0 유지
}

bool UCameraModifier::ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV)
{
	// 베이스 구현: 수정 없음
	return false;
}

void UCameraModifier::EnableModifier()
{
	bDisabled = false;

	if (BlendMode == ECameraModifierBlendMode::Disabled)
	{
		// 블렌드 인 시작
		BlendMode = ECameraModifierBlendMode::BlendingIn;
		Alpha = 0.0f;
	}
	else if (BlendMode == ECameraModifierBlendMode::BlendingOut)
	{
		// 방향 반전: 현재 알파에서 블렌드 인 시작
		BlendMode = ECameraModifierBlendMode::BlendingIn;
	}
	// 이미 BlendingIn 또는 Active면 아무것도 하지 않음
}

void UCameraModifier::DisableModifier(bool bImmediate)
{
	if (bImmediate)
	{
		// 즉시 비활성화
		bDisabled = true;
		Alpha = 0.0f;
		BlendMode = ECameraModifierBlendMode::Disabled;
	}
	else
	{
		// 시간에 걸쳐 블렌드 아웃
		if (BlendMode != ECameraModifierBlendMode::Disabled)
		{
			BlendMode = ECameraModifierBlendMode::BlendingOut;
		}
	}
}
