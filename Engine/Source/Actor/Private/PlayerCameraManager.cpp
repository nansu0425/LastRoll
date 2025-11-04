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
	, FadeColor(0.0f, 0.0f, 0.0f, 1.0f)
	, FadeAmount(0.0f)
	, FadeAlpha(0.0f, 0.0f)
	, FadeTime(0.0f)
	, FadeTimeRemaining(0.0f)
	, bIsFading(false)
	, CameraStyle(FName("Default"))
{
	bCanEverTick = true;
	bTickInEditor = false; // Only tick in game/PIE modes
}

APlayerCameraManager::~APlayerCameraManager()
{
	// Clean up modifiers
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

	// Initialize default camera
	if (!ViewTarget.Target)
	{
		// Use default camera position
		ViewTarget.POV.Location = FVector(0, 0, 500);
		ViewTarget.POV.Rotation = FQuaternion::Identity();
	}
}

void APlayerCameraManager::SetViewTarget(AActor* NewTarget, float InBlendTime)
{
	if (!NewTarget)
	{
		UE_LOG_WARNING("APlayerCameraManager::SetViewTarget - NewTarget is null");
		return;
	}

	if (InBlendTime > 0.0f)
	{
		// Start blending to new target
		PendingViewTarget.Target = NewTarget;

		// Find camera component on new target
		PendingViewTarget.CameraComponent = Cast<UCameraComponent>(
			NewTarget->GetComponentByClass(UCameraComponent::StaticClass())
		);

		// Get initial POV from pending target
		if (PendingViewTarget.CameraComponent)
		{
			PendingViewTarget.CameraComponent->GetCameraView(PendingViewTarget.POV);
		}
		else
		{
			// No camera component: use actor transform
			PendingViewTarget.POV.Location = NewTarget->GetActorLocation();
			PendingViewTarget.POV.Rotation = NewTarget->GetActorRotation();
		}

		BlendTime = InBlendTime;
		BlendTimeRemaining = InBlendTime;
		bIsBlending = true;

		UE_LOG_DEBUG("APlayerCameraManager: Starting blend to new view target (%.2f seconds)", InBlendTime);
	}
	else
	{
		// Instant switch to new target
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

		UE_LOG_DEBUG("APlayerCameraManager: Instant switch to new view target");
	}
}

UCameraModifier* APlayerCameraManager::AddCameraModifier(UClass* ModifierClass)
{
	if (!ModifierClass)
	{
		UE_LOG_ERROR("APlayerCameraManager::AddCameraModifier - ModifierClass is null");
		return nullptr;
	}

	// Verify that ModifierClass is a subclass of UCameraModifier
	if (!ModifierClass->IsChildOf(UCameraModifier::StaticClass()))
	{
		UE_LOG_ERROR("APlayerCameraManager::AddCameraModifier - %s is not a subclass of UCameraModifier", 
			ModifierClass->GetName().ToString().c_str());
		return nullptr;
	}

	// Create new modifier instance using NewObject
	UCameraModifier* NewModifier = Cast<UCameraModifier>(NewObject(ModifierClass, this));
	
	if (!NewModifier)
	{
		UE_LOG_ERROR("APlayerCameraManager::AddCameraModifier - Failed to create modifier instance");
		return nullptr;
	}

	// Initialize modifier
	NewModifier->Initialize(this);

	// Add to list
	ModifierList.push_back(NewModifier);

	UE_LOG_DEBUG("APlayerCameraManager: Added camera modifier (Class: %s, Priority: %d)",
		ModifierClass->GetName().ToString().c_str(), NewModifier->GetPriority());

	return NewModifier;
}

bool APlayerCameraManager::RemoveCameraModifier(UCameraModifier* Modifier)
{
	if (!Modifier)
		return false;

	// Find and remove
	for (auto It = ModifierList.begin(); It != ModifierList.end(); ++It)
	{
		if (*It == Modifier)
		{
			delete Modifier;
			ModifierList.erase(It);
			UE_LOG_DEBUG("APlayerCameraManager: Removed camera modifier");
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

	UE_LOG_DEBUG("APlayerCameraManager: Cleared all camera modifiers");
}

void APlayerCameraManager::StartCameraFade(float FromAlpha, float ToAlpha, float Duration, FVector4 Color)
{
	FadeColor = Color;
	FadeAmount = FromAlpha;
	FadeAlpha.X = FromAlpha;
	FadeAlpha.Y = ToAlpha;
	FadeTime = Duration;
	FadeTimeRemaining = Duration;
	bIsFading = true;

	UE_LOG_DEBUG("APlayerCameraManager: Started camera fade (%.2f → %.2f over %.2f seconds)",
		FromAlpha, ToAlpha, Duration);
}

void APlayerCameraManager::StopCameraFade()
{
	bIsFading = false;
	FadeTimeRemaining = 0.0f;
	FadeAlpha.X = 0.0f;
	FadeAlpha.Y = 0.0f;

	UE_LOG_DEBUG("APlayerCameraManager: Stopped camera fade");
}

void APlayerCameraManager::UpdateCamera(float DeltaTime)
{
	// ===== Step 1: Get base POV from ViewTarget =====
	UpdateViewTarget(DeltaTime);

	// ===== Step 2: Blend between ViewTargets if transitioning =====
	if (bIsBlending)
	{
		UpdateBlending(DeltaTime);
	}

	// ===== Step 3: Apply camera modifier chain =====
	ApplyCameraModifiers(DeltaTime);

	// ===== Step 4: Update screen fade =====
	if (bIsFading)
	{
		UpdateFading(DeltaTime);
	}

	// ===== Step 5: Convert final POV to View/Projection matrices =====
	UpdateCameraConstants();
}

void APlayerCameraManager::UpdateViewTarget(float DeltaTime)
{
	if (ViewTarget.CameraComponent)
	{
		// Get POV from camera component
		ViewTarget.CameraComponent->GetCameraView(ViewTarget.POV);
	}
	else if (ViewTarget.Target)
	{
		// No camera component: use actor transform with default projection
		ViewTarget.POV.Location = ViewTarget.Target->GetActorLocation();
		ViewTarget.POV.Rotation = ViewTarget.Target->GetActorRotation();
		// Keep existing FOV, aspect, etc.
	}
	else
	{
		// No view target: use default camera
		ViewTarget.POV.Location = FVector(0, 0, 500);
		ViewTarget.POV.Rotation = FQuaternion::Identity();
	}

	// Store in cached POV
	CachedPOV = ViewTarget.POV;
}

void APlayerCameraManager::UpdateBlending(float DeltaTime)
{
	BlendTimeRemaining -= DeltaTime;

	if (BlendTimeRemaining <= 0.0f)
	{
		// Blend complete
		ViewTarget = PendingViewTarget;
		bIsBlending = false;
		BlendTimeRemaining = 0.0f;

		UE_LOG_DEBUG("APlayerCameraManager: Blend complete");
	}
	else
	{
		// Lerp between current and pending
		float BlendAlpha = 1.0f - (BlendTimeRemaining / BlendTime);

		// Update pending target POV
		if (PendingViewTarget.CameraComponent)
		{
			PendingViewTarget.CameraComponent->GetCameraView(PendingViewTarget.POV);
		}
		else if (PendingViewTarget.Target)
		{
			PendingViewTarget.POV.Location = PendingViewTarget.Target->GetActorLocation();
			PendingViewTarget.POV.Rotation = PendingViewTarget.Target->GetActorRotation();
		}

		// Interpolate POV
		CachedPOV.Location = Lerp(ViewTarget.POV.Location, PendingViewTarget.POV.Location, BlendAlpha);
		// TODO: Implement proper quaternion slerp for smooth rotation blending
		// For now, use simple linear interpolation
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
	}
}

void APlayerCameraManager::ApplyCameraModifiers(float DeltaTime)
{
	if (ModifierList.empty())
		return;

	// Sort modifiers by priority (ascending: low priority first)
	std::sort(ModifierList.begin(), ModifierList.end(),
		[](const UCameraModifier* A, const UCameraModifier* B) {
			return A->GetPriority() < B->GetPriority();
		});

	// Apply each modifier in order
	for (UCameraModifier* Modifier : ModifierList)
	{
		if (Modifier && !Modifier->IsDisabled())
		{
			// Update blend alpha
			Modifier->UpdateAlpha(DeltaTime);

			// Apply modifier to POV
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
		// Fade complete
		FadeAlpha.X = FadeAlpha.Y;
		FadeTimeRemaining = 0.0f;
		bIsFading = false;
	}
	else
	{
		// Interpolate fade alpha
		float FadeBlendAlpha = 1.0f - (FadeTimeRemaining / FadeTime);
		FadeAlpha.X = Lerp(FadeAmount, FadeAlpha.Y, FadeBlendAlpha);
	}

	// TODO: Integrate with URenderer for post-process fade overlay
	// For now, fade data is available via GetFadeAlpha() for renderer to use
}

void APlayerCameraManager::UpdateCameraConstants()
{
	// Convert final POV to camera constants (View/Projection matrices)
	CachedCameraConstants = CachedPOV.ToCameraConstants();
}

void APlayerCameraManager::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	// TODO: Implement JSON serialization for camera manager
	// ViewTarget, modifiers, and fade state are runtime state
	// that should be set up in BeginPlay or gameplay code
}
