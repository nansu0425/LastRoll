#pragma once
#include "Actor/Public/Actor.h"
#include "Global/CoreTypes.h"

class UCameraComponent;   // Forward declaration
class UCameraModifier;    // Forward declaration

/**
 * @brief View target structure for camera blending
 */
struct FViewTarget
{
	AActor* Target;                        // Target actor (can be null)
	UCameraComponent* CameraComponent;     // Target's camera component (can be null)
	FMinimalViewInfo POV;                  // Current POV data

	FViewTarget()
		: Target(nullptr)
		, CameraComponent(nullptr)
		, POV()
	{
	}
};

/**
 * @brief Player Camera Manager - manages active camera and camera modifiers
 *
 * APlayerCameraManager is the central camera system for game/PIE modes. It:
 * 1. Manages the active view target (actor with camera component)
 * 2. Processes camera modifier chain (shake, lag, FOV transitions, etc.)
 * 3. Handles view target blending (smooth transitions between cameras)
 * 4. Handles camera fading (fade to/from color)
 * 5. Produces final View/Projection matrices for rendering
 *
 * Update pipeline (called each frame):
 * UpdateViewTarget() → UpdateBlending() → ApplyCameraModifiers() → UpdateFading() → UpdateCameraConstants()
 */
UCLASS()
class APlayerCameraManager : public AActor
{
	GENERATED_BODY()
	DECLARE_CLASS(APlayerCameraManager, AActor)

private:
	// ===== View Target =====
	FViewTarget ViewTarget;             // Current view target
	FViewTarget PendingViewTarget;      // Target to blend to (during transition)

	// ===== Blending =====
	float BlendTime;                    // Total blend duration
	float BlendTimeRemaining;           // Time left to complete blend
	bool bIsBlending;                   // True during view target transition

	// ===== Camera Fading =====
	FVector4 FadeColor;                 // Fade overlay color (e.g., black for fade to black)
	float FadeAmount;                   // Target fade amount [0.0, 1.0]
	FVector2 FadeAlpha;                 // (Current, Target) fade alpha
	float FadeTime;                     // Fade duration
	float FadeTimeRemaining;            // Fade time left
	bool bIsFading;                     // True during fade

	// ===== Camera Style =====
	FName CameraStyle;                  // Current camera style name (e.g., "Default", "ThirdPerson")

	// ===== Camera Modifiers =====
	TArray<UCameraModifier*> ModifierList;  // Active camera modifiers (processed by priority)

	// ===== Cached Results =====
	FMinimalViewInfo CachedPOV;         // Final POV after modifier chain
	FCameraConstants CachedCameraConstants; // Final View/Projection matrices

public:
	APlayerCameraManager();
	virtual ~APlayerCameraManager() override;

	// ===== Lifecycle =====
	virtual void BeginPlay() override;

	// ===== View Target Management =====
	/**
	 * @brief Set new view target (camera to use)
	 *
	 * If BlendTime > 0, smoothly transitions from current camera to new camera.
	 * If BlendTime = 0, instantly switches to new camera.
	 *
	 * @param NewTarget Actor to view from (should have UCameraComponent, or use actor transform)
	 * @param InBlendTime Duration of blend transition (seconds). 0 = instant.
	 */
	void SetViewTarget(AActor* NewTarget, float InBlendTime = 0.0f);

	/**
	 * @brief Get current view target actor
	 */
	AActor* GetViewTarget() const { return ViewTarget.Target; }

	/**
	 * @brief Get current view target's camera component
	 */
	UCameraComponent* GetViewTargetCamera() const { return ViewTarget.CameraComponent; }

	// ===== Camera Modifier Management =====
	/**
	 * @brief Add camera modifier to processing chain
	 *
	 * Creates new instance of modifier class and adds to ModifierList.
	 * Modifier will be initialized and start processing on next UpdateCamera().
	 *
	 * @param ModifierClass Class of modifier to create (e.g., UCameraModifier_CameraShake::StaticClass())
	 * @return Created modifier instance (or null on failure)
	 */
	UCameraModifier* AddCameraModifier(UClass* ModifierClass);

	/**
	 * @brief Remove camera modifier from processing chain
	 *
	 * @param Modifier Modifier instance to remove
	 * @return true if removed successfully
	 */
	bool RemoveCameraModifier(UCameraModifier* Modifier);

	/**
	 * @brief Find camera modifier by class
	 *
	 * @param ModifierClass Class to search for
	 * @return First modifier of given class, or null if not found
	 */
	UCameraModifier* FindCameraModifierByClass(UClass* ModifierClass) const;

	/**
	 * @brief Remove all camera modifiers
	 */
	void ClearAllCameraModifiers();

	// ===== Camera Fade API =====
	/**
	 * @brief Start camera fade effect
	 *
	 * Fades screen to specified color over time. Common use cases:
	 * - Fade to black on death: StartCameraFade(0.0, 1.0, 1.0, FVector4(0,0,0,1))
	 * - Fade from black on spawn: StartCameraFade(1.0, 0.0, 1.0, FVector4(0,0,0,1))
	 *
	 * @param FromAlpha Starting fade alpha [0.0, 1.0]. 0 = no fade, 1 = full fade
	 * @param ToAlpha Target fade alpha [0.0, 1.0]
	 * @param Duration Fade duration (seconds)
	 * @param Color Fade overlay color
	 */
	void StartCameraFade(float FromAlpha, float ToAlpha, float Duration, FVector4 Color);

	/**
	 * @brief Stop camera fade immediately
	 */
	void StopCameraFade();

	// ===== Main Update Function =====
	/**
	 * @brief Update camera (called each frame by UWorld::Tick)
	 *
	 * Main update pipeline:
	 * 1. UpdateViewTarget() - Get base POV from view target's camera
	 * 2. UpdateBlending() - Blend between view targets if transitioning
	 * 3. ApplyCameraModifiers() - Process camera modifier chain (priority order)
	 * 4. UpdateFading() - Update screen fade effect
	 * 5. UpdateCameraConstants() - Convert final POV to View/Projection matrices
	 *
	 * @param DeltaTime Time since last frame (seconds)
	 */
	void UpdateCamera(float DeltaTime);

	// ===== Final Camera Access =====
	/**
	 * @brief Get final camera constants (View/Projection matrices)
	 *
	 * Used by URenderer to get camera matrices for rendering.
	 *
	 * @return Final camera constants after modifier chain processing
	 */
	const FCameraConstants& GetCameraConstants() const { return CachedCameraConstants; }

	/**
	 * @brief Get final camera POV
	 *
	 * @return Final POV after modifier chain processing
	 */
	const FMinimalViewInfo& GetCameraCachePOV() const { return CachedPOV; }

protected:
	// ===== Internal Update Pipeline =====
	/**
	 * @brief Step 1: Get base POV from current ViewTarget
	 */
	void UpdateViewTarget(float DeltaTime);

	/**
	 * @brief Step 2: Blend between ViewTargets if transitioning
	 */
	void UpdateBlending(float DeltaTime);

	/**
	 * @brief Step 3: Apply camera modifier chain (priority order)
	 */
	void ApplyCameraModifiers(float DeltaTime);

	/**
	 * @brief Step 4: Update screen fade effect
	 */
	void UpdateFading(float DeltaTime);

	/**
	 * @brief Step 5: Convert final POV to View/Projection matrices
	 */
	void UpdateCameraConstants();

	// ===== Serialization =====
public:
	virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
};
