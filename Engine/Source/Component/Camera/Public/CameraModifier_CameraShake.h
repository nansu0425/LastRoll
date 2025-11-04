#pragma once
#include "Component/Camera/Public/CameraModifier.h"

/**
 * @brief Camera shake pattern type
 */
UENUM()
enum class ECameraShakePattern : uint8
{
	Sine,           // Smooth sinusoidal oscillation
	Perlin,         // Perlin noise for organic shake
	Random,         // Random jitter
	End
};
DECLARE_UINT8_ENUM_REFLECTION(ECameraShakePattern)

/**
 * @brief Camera shake modifier - adds procedural shake to camera
 *
 * Provides earthquake/explosion/impact effects by applying random or procedural
 * offsets to camera location and rotation. Shake intensity decays over time.
 *
 * Usage Example:
 * ```cpp
 * UCameraModifier_CameraShake* Shake = Cast<UCameraModifier_CameraShake>(
 *     CameraManager->AddCameraModifier(UCameraModifier_CameraShake::StaticClass())
 * );
 * if (Shake)
 * {
 *     Shake->StartShake(2.0f, 10.0f, 5.0f); // duration=2s, locationAmp=10, rotationAmp=5
 * }
 * ```
 */
UCLASS()
class UCameraModifier_CameraShake : public UCameraModifier
{
	GENERATED_BODY()
	DECLARE_CLASS(UCameraModifier_CameraShake, UCameraModifier)

private:
	// Shake Parameters
	float ShakeDuration;          // Total shake duration (seconds)
	float ShakeTimeRemaining;     // Time remaining for current shake
	float LocationAmplitude;      // Max location offset magnitude (world units)
	float RotationAmplitude;      // Max rotation offset magnitude (degrees)
	ECameraShakePattern Pattern;  // Shake pattern type

	// Shake State
	bool bIsShaking;              // Whether shake is currently active
	float ShakeTime;              // Accumulated time for pattern evaluation

	// Pattern Parameters
	float Frequency;              // Oscillation frequency (Hz)
	FVector PerlinOffset;         // Perlin noise offset seed
	FVector LastLocationOffset;   // Previous frame location offset (for smoothing)
	FVector LastRotationOffset;   // Previous frame rotation offset (for smoothing)

public:
	UCameraModifier_CameraShake();
	virtual ~UCameraModifier_CameraShake() override;

	/**
	 * @brief Start camera shake effect
	 *
	 * @param InDuration Shake duration (seconds). If 0, uses default (1.0s)
	 * @param InLocationAmplitude Location shake intensity (world units). 0 = no location shake
	 * @param InRotationAmplitude Rotation shake intensity (degrees). 0 = no rotation shake
	 * @param InPattern Shake pattern type. Default: Perlin
	 * @param InFrequency Shake frequency (Hz). Only used for Sine pattern. Default: 10.0
	 */
	void StartShake(
		float InDuration = 1.0f,
		float InLocationAmplitude = 5.0f,
		float InRotationAmplitude = 2.0f,
		ECameraShakePattern InPattern = ECameraShakePattern::Perlin,
		float InFrequency = 10.0f
	);

	/**
	 * @brief Stop shake immediately
	 */
	void StopShake();

	/**
	 * @brief Check if shake is currently active
	 */
	bool IsShaking() const { return bIsShaking; }

	// Overridden from UCameraModifier
	virtual void Initialize(APlayerCameraManager* InOwner) override;
	virtual bool ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV) override;

private:
	/**
	 * @brief Evaluate shake offset for current frame
	 *
	 * @param OutLocationOffset Output location offset (world space)
	 * @param OutRotationOffset Output rotation offset (degrees)
	 * @param CurrentTime Current shake time
	 * @param DecayAlpha Decay multiplier [0.0, 1.0]
	 */
	void EvaluateShake(
		FVector& OutLocationOffset,
		FVector& OutRotationOffset,
		float CurrentTime,
		float DecayAlpha
	);

	/**
	 * @brief Generate Perlin noise value
	 *
	 * @param X Input coordinate
	 * @return Noise value in range [-1.0, 1.0]
	 */
	float PerlinNoise1D(float X) const;

	/**
	 * @brief Smooth interpolation function (smoothstep)
	 */
	float SmoothStep(float t) const;
};
