#pragma once
#include "Core/Public/Object.h"

class APlayerCameraManager; // Forward declaration
struct FMinimalViewInfo;    // Forward declaration

/**
 * @brief Camera modifier blend state
 */
UENUM()
enum class ECameraModifierBlendMode : uint8
{
	Disabled,      // Modifier inactive (Alpha = 0.0)
	BlendingIn,    // Alpha increasing (0.0 → 1.0)
	Active,        // Alpha at 1.0 (fully active)
	BlendingOut,   // Alpha decreasing (1.0 → 0.0)
	End
};
DECLARE_UINT8_ENUM_REFLECTION(ECameraModifierBlendMode)

/**
 * @brief Base class for camera post-processing effects
 *
 * UCameraModifier allows implementation of various camera effects:
 * - Camera shake (location/rotation offsets)
 * - Camera lag (spring-arm follow)
 * - FOV transitions (zoom in/out)
 * - Look-at constraints (aim at target)
 * - Custom camera behaviors
 *
 * Modifiers are processed in priority order (low to high) by APlayerCameraManager.
 * Each modifier can blend in/out over time using AlphaInTime/AlphaOutTime.
 */
UCLASS()
class UCameraModifier : public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(UCameraModifier, UObject)

protected:
	// Owner
	APlayerCameraManager* CameraOwner;  // Back-reference to camera manager

	// Blend Timing
	float AlphaInTime;                  // Time to blend in (0.0 = instant)
	float AlphaOutTime;                 // Time to blend out (0.0 = instant)
	float Alpha;                        // Current blend weight [0.0, 1.0]

	// State
	ECameraModifierBlendMode BlendMode; // Current blend state
	bool bDisabled;                     // If true, skip this modifier
	uint8 Priority;                     // Higher = processed later (stronger influence)

public:
	UCameraModifier();
	virtual ~UCameraModifier() override;

	/**
	 * @brief Initialize modifier with camera owner
	 *
	 * Called by APlayerCameraManager when modifier is added.
	 *
	 * @param InOwner Camera manager that owns this modifier
	 */
	virtual void Initialize(APlayerCameraManager* InOwner);

	/**
	 * @brief Update blend alpha based on blend mode
	 *
	 * Handles automatic blending in/out over time. Called each frame by camera manager.
	 *
	 * @param DeltaTime Time since last frame (seconds)
	 */
	virtual void UpdateAlpha(float DeltaTime);

	/**
	 * @brief Main override point: Modify camera POV
	 *
	 * Override this method to apply your camera effect. Modifiers are processed in priority order.
	 * Modify InOutPOV fields (Location, Rotation, FOV, etc.) to apply effect.
	 *
	 * @param DeltaTime Time since last frame (seconds)
	 * @param InOutPOV Camera POV to modify (input/output parameter)
	 * @return true if POV was modified, false otherwise
	 */
	virtual bool ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV);

	/**
	 * @brief Enable modifier (start blending in)
	 */
	virtual void EnableModifier();

	/**
	 * @brief Disable modifier
	 *
	 * @param bImmediate If true, disable immediately (Alpha = 0.0). If false, blend out over AlphaOutTime.
	 */
	virtual void DisableModifier(bool bImmediate);

	// Getters
	bool IsDisabled() const { return bDisabled; }
	uint8 GetPriority() const { return Priority; }
	float GetAlpha() const { return Alpha; }
	APlayerCameraManager* GetCameraOwner() const { return CameraOwner; }

	// Setters
	void SetPriority(uint8 InPriority) { Priority = InPriority; }
	void SetAlphaInTime(float InTime) { AlphaInTime = InTime; }
	void SetAlphaOutTime(float InTime) { AlphaOutTime = InTime; }
};
