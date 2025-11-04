#pragma once
#include "Component/Public/SceneComponent.h"

struct FMinimalViewInfo; // Forward declaration

/**
 * @brief Game camera component that can be attached to actors
 *
 * UCameraComponent provides camera functionality for game actors (like player pawns, vehicles, etc.).
 * It stores projection parameters (FOV, aspect ratio, clip planes) and generates camera view data
 * used by APlayerCameraManager and camera modifiers.
 */
UCLASS()
class UCameraComponent : public USceneComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(UCameraComponent, USceneComponent)

private:
	// Projection Parameters
	float FieldOfView;                  // Vertical FOV (degrees), default: 90.0f
	float AspectRatio;                  // Width / Height, default: 16.0f/9.0f
	float NearClipPlane;                // Near Z, default: 1.0f
	float FarClipPlane;                 // Far Z, default: 10000.0f

	// Camera Type
	bool bUsePerspectiveProjection;     // true: perspective, false: ortho
	float OrthoWidth;                   // Orthographic width, default: 1000.0f

public:
	UCameraComponent();
	virtual ~UCameraComponent() override;

	/**
	 * @brief Get camera view information (POV)
	 *
	 * Fills FMinimalViewInfo with current camera transform (from SceneComponent hierarchy)
	 * and projection parameters. This POV is used by APlayerCameraManager as base camera data.
	 *
	 * @param OutPOV Output parameter filled with camera location, rotation, and projection settings
	 */
	void GetCameraView(FMinimalViewInfo& OutPOV) const;

	// Setters
	void SetFieldOfView(float InFOV) { FieldOfView = InFOV; }
	void SetAspectRatio(float InAspect) { AspectRatio = InAspect; }
	void SetNearClipPlane(float InNear) { NearClipPlane = InNear; }
	void SetFarClipPlane(float InFar) { FarClipPlane = InFar; }
	void SetProjectionType(bool bInUsePerspective) { bUsePerspectiveProjection = bInUsePerspective; }
	void SetOrthoWidth(float InWidth) { OrthoWidth = InWidth; }

	// Getters
	float GetFieldOfView() const { return FieldOfView; }
	float GetAspectRatio() const { return AspectRatio; }
	float GetNearClipPlane() const { return NearClipPlane; }
	float GetFarClipPlane() const { return FarClipPlane; }
	bool IsUsingPerspectiveProjection() const { return bUsePerspectiveProjection; }
	float GetOrthoWidth() const { return OrthoWidth; }

	// Serialization
	virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
};
