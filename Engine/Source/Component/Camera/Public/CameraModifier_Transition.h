#pragma once
#include "Component/Camera/Public/CameraModifier.h"
#include "Global/Public/BezierCurve.h"

/**
 * @brief Camera Transition Modifier - Smooth camera POV interpolation
 *
 * Provides smooth transitions between camera states using Bezier curve-based timing.
 * Interpolates Location, Rotation (Slerp), and FOV over a specified duration.
 *
 * Usage Example:
 * ```cpp
 * UCameraModifier_Transition* Transition = Cast<UCameraModifier_Transition>(
 *     CameraManager->AddCameraModifier(UCameraModifier_Transition::StaticClass())
 * );
 *
 * FMinimalViewInfo CurrentPOV = CameraManager->GetCameraCachePOV();
 * FMinimalViewInfo TargetPOV;
 * TargetPOV.Location = FVector(100, 200, 300);
 * TargetPOV.Rotation = FQuaternion::FromEuler(FVector(0, 45, 0));
 * TargetPOV.FOV = 60.0f;
 *
 * FCubicBezierCurve Curve = FCubicBezierCurve::CreateEaseInOut();
 * Transition->StartTransition(CurrentPOV, TargetPOV, 2.0f, Curve);
 * ```
 */
UCLASS()
class UCameraModifier_Transition : public UCameraModifier
{
	GENERATED_BODY()
	DECLARE_CLASS(UCameraModifier_Transition, UCameraModifier)

private:
	// ===== Transition State =====
	float TransitionDuration;         // Total transition duration (seconds)
	float TransitionTimeRemaining;    // Time remaining until completion
	float TransitionTime;             // Elapsed time since start
	bool bIsTransitioning;            // Is transition currently active?

	// ===== POV States =====
	FMinimalViewInfo StartPOV;        // Starting camera state
	FMinimalViewInfo TargetPOV;       // Target camera state

	// ===== Timing Curve =====
	FCubicBezierCurve TimingCurve;    // Bezier curve for timing control
	                                  // X = Normalized time [0,1]
	                                  // Y = Blend alpha [0,1]
	bool bUseTimingCurve;             // If false, use linear interpolation

public:
	UCameraModifier_Transition();
	virtual ~UCameraModifier_Transition() override;

	/**
	 * @brief Start a camera transition
	 *
	 * @param InFromPOV Starting camera state (usually current camera)
	 * @param InToPOV Target camera state
	 * @param InDuration Transition duration in seconds (must be > 0)
	 * @param InTimingCurve Bezier curve controlling interpolation timing
	 */
	void StartTransition(
		const FMinimalViewInfo& InFromPOV,
		const FMinimalViewInfo& InToPOV,
		float InDuration,
		const FCubicBezierCurve& InTimingCurve
	);

	/**
	 * @brief Start a transition with linear timing (no curve)
	 */
	void StartTransitionLinear(
		const FMinimalViewInfo& InFromPOV,
		const FMinimalViewInfo& InToPOV,
		float InDuration
	);

	/**
	 * @brief Stop the current transition immediately
	 *
	 * Camera will snap to target state.
	 */
	void StopTransition();

	/**
	 * @brief Check if a transition is currently active
	 */
	bool IsTransitioning() const { return bIsTransitioning; }

	/**
	 * @brief Get remaining transition time
	 */
	float GetTimeRemaining() const { return TransitionTimeRemaining; }

	/**
	 * @brief Get transition progress [0, 1]
	 */
	float GetProgress() const
	{
		return bIsTransitioning ? (TransitionTime / TransitionDuration) : 0.0f;
	}

	// ===== Bezier Curve Access =====

	/**
	 * @brief Get the current timing curve
	 */
	const FCubicBezierCurve& GetTimingCurve() const { return TimingCurve; }

	/**
	 * @brief Set the timing curve
	 *
	 * @param InCurve New timing curve
	 * @note Only affects new transitions, does not modify active transition
	 */
	void SetTimingCurve(const FCubicBezierCurve& InCurve);

	/**
	 * @brief Check if using Bezier curve timing
	 */
	bool IsUsingTimingCurve() const { return bUseTimingCurve; }

	/**
	 * @brief Enable/disable Bezier curve timing
	 *
	 * @param bInUse true = use TimingCurve, false = linear interpolation
	 */
	void SetUseTimingCurve(bool bInUse) { bUseTimingCurve = bInUse; }

	// ===== UCameraModifier Overrides =====

	virtual void Initialize(APlayerCameraManager* InOwner) override;
	virtual bool ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV) override;

private:
	/**
	 * @brief Compute blend alpha from normalized time
	 *
	 * @param NormalizedTime Time ratio [0, 1]
	 * @return Blend alpha [0, 1]
	 */
	float ComputeBlendAlpha(float NormalizedTime) const;
};
