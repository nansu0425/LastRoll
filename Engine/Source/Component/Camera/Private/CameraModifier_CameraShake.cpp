#include "pch.h"
#include "Component/Camera/Public/CameraModifier_CameraShake.h"
#include "Component/Camera/Public/CameraComponent.h"
#include "Actor/Public/PlayerCameraManager.h"
#include "Global/CoreTypes.h"
#include <cmath>
#include <random>

IMPLEMENT_CLASS(UCameraModifier_CameraShake, UCameraModifier)

UCameraModifier_CameraShake::UCameraModifier_CameraShake()
	: ShakeDuration(1.0f)
	, ShakeTimeRemaining(0.0f)
	, LocationAmplitude(5.0f)
	, RotationAmplitude(2.0f)
	, Pattern(ECameraShakePattern::Perlin)
	, bIsShaking(false)
	, ShakeTime(0.0f)
	, Frequency(10.0f)
	, PerlinOffset(FVector::ZeroVector())
	, LastLocationOffset(FVector::ZeroVector())
	, LastRotationOffset(FVector::ZeroVector())
{
	// Camera shake has high priority (processed late in modifier chain)
	SetPriority(200);

	// Fast blend in/out for responsive shake
	SetAlphaInTime(0.1f);
	SetAlphaOutTime(0.2f);

	// Generate random Perlin offset seed
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> dis(0.0f, 1000.0f);
	PerlinOffset = FVector(dis(gen), dis(gen), dis(gen));
}

UCameraModifier_CameraShake::~UCameraModifier_CameraShake()
{
}

void UCameraModifier_CameraShake::Initialize(APlayerCameraManager* InOwner)
{
	Super::Initialize(InOwner);
}

void UCameraModifier_CameraShake::StartShake(
	float InDuration,
	float InLocationAmplitude,
	float InRotationAmplitude,
	ECameraShakePattern InPattern,
	float InFrequency)
{
	// Validate parameters
	ShakeDuration = (InDuration > 0.0f) ? InDuration : 1.0f;
	LocationAmplitude = std::max(0.0f, InLocationAmplitude);
	RotationAmplitude = std::max(0.0f, InRotationAmplitude);
	Pattern = InPattern;
	Frequency = std::max(0.1f, InFrequency);

	// Reset shake state
	ShakeTimeRemaining = ShakeDuration;
	ShakeTime = 0.0f;
	bIsShaking = true;
	LastLocationOffset = FVector::ZeroVector();
	LastRotationOffset = FVector::ZeroVector();

	// Enable modifier (will start blending in)
	EnableModifier();

	UE_LOG_DEBUG("CameraShake: Started shake (Duration: %.2f, LocAmp: %.2f, RotAmp: %.2f)",
		ShakeDuration, LocationAmplitude, RotationAmplitude);
}

void UCameraModifier_CameraShake::StopShake()
{
	bIsShaking = false;
	ShakeTimeRemaining = 0.0f;

	// Disable modifier (will start blending out)
	DisableModifier(false);

	UE_LOG_DEBUG("CameraShake: Stopped shake");
}

bool UCameraModifier_CameraShake::ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV)
{
	if (!bIsShaking)
	{
		return false;
	}

	// Update shake time
	ShakeTime += DeltaTime;
	ShakeTimeRemaining -= DeltaTime;

	// Check if shake has finished
	if (ShakeTimeRemaining <= 0.0f)
	{
		StopShake();
		return false;
	}

	// Calculate decay alpha (fade out shake intensity over time)
	// Uses smoothstep for natural decay curve
	float DecayAlpha = ShakeTimeRemaining / ShakeDuration;
	DecayAlpha = SmoothStep(DecayAlpha); // Smooth decay curve

	// Evaluate shake offsets
	FVector LocationOffset = FVector::ZeroVector();
	FVector RotationOffset = FVector::ZeroVector();
	EvaluateShake(LocationOffset, RotationOffset, ShakeTime, DecayAlpha);

	// Apply Alpha blend weight
	float BlendWeight = GetAlpha();
	LocationOffset *= BlendWeight;
	RotationOffset *= BlendWeight;

	// Apply location offset (world space)
	InOutPOV.Location += LocationOffset;

	// Apply rotation offset (convert degrees to quaternion and compose)
	// Rotation offset is in Euler angles (degrees)
	FQuaternion RotationDelta = FQuaternion::Identity();
	
	// Convert Euler angles (degrees) to radians
	FVector RotationRad = FVector::GetDegreeToRadian(RotationOffset);
	
	// Create quaternion from Euler angles (YXZ order: Yaw-Pitch-Roll)
	// Note: FutureEngine uses Z-up, so rotations are applied accordingly
	float CosYaw = std::cos(RotationRad.Z * 0.5f);
	float SinYaw = std::sin(RotationRad.Z * 0.5f);
	float CosPitch = std::cos(RotationRad.Y * 0.5f);
	float SinPitch = std::sin(RotationRad.Y * 0.5f);
	float CosRoll = std::cos(RotationRad.X * 0.5f);
	float SinRoll = std::sin(RotationRad.X * 0.5f);

	RotationDelta.X = CosYaw * SinPitch * CosRoll + SinYaw * CosPitch * SinRoll;
	RotationDelta.Y = SinYaw * CosPitch * CosRoll - CosYaw * SinPitch * SinRoll;
	RotationDelta.Z = CosYaw * CosPitch * SinRoll - SinYaw * SinPitch * CosRoll;
	RotationDelta.W = CosYaw * CosPitch * CosRoll + SinYaw * SinPitch * SinRoll;
	RotationDelta.Normalize();

	// Compose with existing rotation
	InOutPOV.Rotation = RotationDelta * InOutPOV.Rotation;
	InOutPOV.Rotation.Normalize();

	return true;
}

void UCameraModifier_CameraShake::EvaluateShake(
	FVector& OutLocationOffset,
	FVector& OutRotationOffset,
	float CurrentTime,
	float DecayAlpha)
{
	OutLocationOffset = FVector::ZeroVector();
	OutRotationOffset = FVector::ZeroVector();

	switch (Pattern)
	{
	case ECameraShakePattern::Sine:
	{
		// Sinusoidal oscillation (smooth, predictable)
		float Phase = CurrentTime * Frequency * 2.0f * PI;
		
		// Use different phase offsets for each axis for variety
		OutLocationOffset.X = std::sin(Phase) * LocationAmplitude;
		OutLocationOffset.Y = std::sin(Phase + 2.0f) * LocationAmplitude;
		OutLocationOffset.Z = std::sin(Phase + 4.0f) * LocationAmplitude;

		OutRotationOffset.X = std::sin(Phase + 1.0f) * RotationAmplitude;
		OutRotationOffset.Y = std::sin(Phase + 3.0f) * RotationAmplitude;
		OutRotationOffset.Z = std::sin(Phase + 5.0f) * RotationAmplitude;
		break;
	}

	case ECameraShakePattern::Perlin:
	{
		// Perlin noise (organic, natural-looking shake)
		// Sample Perlin noise at different offsets for each axis
		float Speed = Frequency * 0.5f; // Perlin frequency control

		OutLocationOffset.X = PerlinNoise1D(PerlinOffset.X + CurrentTime * Speed) * LocationAmplitude;
		OutLocationOffset.Y = PerlinNoise1D(PerlinOffset.Y + CurrentTime * Speed) * LocationAmplitude;
		OutLocationOffset.Z = PerlinNoise1D(PerlinOffset.Z + CurrentTime * Speed) * LocationAmplitude;

		OutRotationOffset.X = PerlinNoise1D(PerlinOffset.X + CurrentTime * Speed + 10.0f) * RotationAmplitude;
		OutRotationOffset.Y = PerlinNoise1D(PerlinOffset.Y + CurrentTime * Speed + 20.0f) * RotationAmplitude;
		OutRotationOffset.Z = PerlinNoise1D(PerlinOffset.Z + CurrentTime * Speed + 30.0f) * RotationAmplitude;

		// Smooth transition by blending with previous frame
		float SmoothFactor = 0.3f;
		OutLocationOffset = OutLocationOffset * (1.0f - SmoothFactor) + LastLocationOffset * SmoothFactor;
		OutRotationOffset = OutRotationOffset * (1.0f - SmoothFactor) + LastRotationOffset * SmoothFactor;

		LastLocationOffset = OutLocationOffset;
		LastRotationOffset = OutRotationOffset;
		break;
	}

	case ECameraShakePattern::Random:
	{
		// Random jitter (chaotic, harsh shake)
		static std::random_device rd;
		static std::mt19937 gen(rd());
		std::uniform_real_distribution<float> dis(-1.0f, 1.0f);

		OutLocationOffset.X = dis(gen) * LocationAmplitude;
		OutLocationOffset.Y = dis(gen) * LocationAmplitude;
		OutLocationOffset.Z = dis(gen) * LocationAmplitude;

		OutRotationOffset.X = dis(gen) * RotationAmplitude;
		OutRotationOffset.Y = dis(gen) * RotationAmplitude;
		OutRotationOffset.Z = dis(gen) * RotationAmplitude;
		break;
	}

	default:
		break;
	}

	// Apply decay (fade out over time)
	OutLocationOffset *= DecayAlpha;
	OutRotationOffset *= DecayAlpha;
}

float UCameraModifier_CameraShake::PerlinNoise1D(float X) const
{
	// Simple 1D Perlin noise implementation
	// Reference: Ken Perlin's improved noise (2002)
	
	int X0 = static_cast<int>(std::floor(X)) & 255;
	int X1 = (X0 + 1) & 255;
	
	float Fx = X - std::floor(X);
	
	// Smoothstep interpolation
	float U = SmoothStep(Fx);
	
	// Simple hash function for pseudo-random gradients
	auto Hash = [](int I) -> float {
		I = (I << 13) ^ I;
		return (1.0f - ((I * (I * I * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f);
	};
	
	float G0 = Hash(X0);
	float G1 = Hash(X1);
	
	// Linear interpolation
	return G0 + U * (G1 - G0);
}

float UCameraModifier_CameraShake::SmoothStep(float t) const
{
	// Smoothstep function: 3t^2 - 2t^3
	// Provides smooth interpolation with zero derivative at endpoints
	t = std::clamp(t, 0.0f, 1.0f);
	return t * t * (3.0f - 2.0f * t);
}
