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
	// 카메라 흔들림은 높은 우선순위 (모디파이어 체인에서 나중에 처리)
	SetPriority(200);

	// 반응성 있는 흔들림을 위한 빠른 블렌드 인/아웃
	SetAlphaInTime(0.1f);
	SetAlphaOutTime(0.2f);

	// 랜덤 펄린 오프셋 시드 생성
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
	// 파라미터 검증
	ShakeDuration = (InDuration > 0.0f) ? InDuration : 1.0f;
	LocationAmplitude = std::max(0.0f, InLocationAmplitude);
	RotationAmplitude = std::max(0.0f, InRotationAmplitude);
	Pattern = InPattern;
	Frequency = std::max(0.1f, InFrequency);

	// 흔들림 상태 리셋
	ShakeTimeRemaining = ShakeDuration;
	ShakeTime = 0.0f;
	bIsShaking = true;
	LastLocationOffset = FVector::ZeroVector();
	LastRotationOffset = FVector::ZeroVector();

	// 모디파이어 활성화 (블렌드 인 시작)
	EnableModifier();

	UE_LOG_DEBUG("CameraShake: 흔들림 시작 (지속시간: %.2f, 위치진폭: %.2f, 회전진폭: %.2f)",
		ShakeDuration, LocationAmplitude, RotationAmplitude);
}

void UCameraModifier_CameraShake::StopShake()
{
	bIsShaking = false;
	ShakeTimeRemaining = 0.0f;

	// 모디파이어 비활성화 (블렌드 아웃 시작)
	DisableModifier(false);

	UE_LOG_DEBUG("CameraShake: 흔들림 중지");
}

bool UCameraModifier_CameraShake::ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV)
{
	if (!bIsShaking)
	{
		return false;
	}

	// 흔들림 시간 업데이트
	ShakeTime += DeltaTime;
	ShakeTimeRemaining -= DeltaTime;

	// 흔들림 종료 확인
	if (ShakeTimeRemaining <= 0.0f)
	{
		StopShake();
		return false;
	}

	// 감쇠 알파 계산 (시간에 따라 흔들림 강도 페이드 아웃)
	// 자연스러운 감쇠 곡선을 위해 smoothstep 사용
	float DecayAlpha = ShakeTimeRemaining / ShakeDuration;
	DecayAlpha = SmoothStep(DecayAlpha); // 부드러운 감쇠 곡선

	// 흔들림 오프셋 평가
	FVector LocationOffset = FVector::ZeroVector();
	FVector RotationOffset = FVector::ZeroVector();
	EvaluateShake(LocationOffset, RotationOffset, ShakeTime, DecayAlpha);

	// 알파 블렌드 가중치 적용
	float BlendWeight = GetAlpha();
	LocationOffset *= BlendWeight;
	RotationOffset *= BlendWeight;

	// 위치 오프셋 적용 (월드 공간)
	InOutPOV.Location += LocationOffset;

	// 회전 오프셋 적용 (도를 쿼터니언으로 변환하고 합성)
	// 회전 오프셋은 오일러 각도 (도 단위)
	FQuaternion RotationDelta = FQuaternion::Identity();
	
	// 오일러 각도 (도)를 라디안으로 변환
	FVector RotationRad = FVector::GetDegreeToRadian(RotationOffset);
	
	// 오일러 각도에서 쿼터니언 생성 (YXZ 순서: Yaw-Pitch-Roll)
	// 참고: FutureEngine은 Z-up을 사용하므로 회전을 적절히 적용
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

	// 기존 회전과 합성
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
		// 정현파 진동 (부드럽고 예측 가능)
		float Phase = CurrentTime * Frequency * 2.0f * PI;
		
		// 각 축에 다른 위상 오프셋 사용하여 다양성 추가
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
		// 펄린 노이즈 (유기적이고 자연스러운 흔들림)
		// 각 축에 다른 오프셋으로 펄린 노이즈 샘플링
		float Speed = Frequency * 0.5f; // 펄린 주파수 제어

		OutLocationOffset.X = PerlinNoise1D(PerlinOffset.X + CurrentTime * Speed) * LocationAmplitude;
		OutLocationOffset.Y = PerlinNoise1D(PerlinOffset.Y + CurrentTime * Speed) * LocationAmplitude;
		OutLocationOffset.Z = PerlinNoise1D(PerlinOffset.Z + CurrentTime * Speed) * LocationAmplitude;

		OutRotationOffset.X = PerlinNoise1D(PerlinOffset.X + CurrentTime * Speed + 10.0f) * RotationAmplitude;
		OutRotationOffset.Y = PerlinNoise1D(PerlinOffset.Y + CurrentTime * Speed + 20.0f) * RotationAmplitude;
		OutRotationOffset.Z = PerlinNoise1D(PerlinOffset.Z + CurrentTime * Speed + 30.0f) * RotationAmplitude;

		// 이전 프레임과 블렌드하여 부드러운 전환
		float SmoothFactor = 0.3f;
		OutLocationOffset = OutLocationOffset * (1.0f - SmoothFactor) + LastLocationOffset * SmoothFactor;
		OutRotationOffset = OutRotationOffset * (1.0f - SmoothFactor) + LastRotationOffset * SmoothFactor;

		LastLocationOffset = OutLocationOffset;
		LastRotationOffset = OutRotationOffset;
		break;
	}

	case ECameraShakePattern::Random:
	{
		// 랜덤 지터 (무질서하고 격렬한 흔들림)
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

	// 감쇠 적용 (시간에 따라 페이드 아웃)
	OutLocationOffset *= DecayAlpha;
	OutRotationOffset *= DecayAlpha;
}

float UCameraModifier_CameraShake::PerlinNoise1D(float X) const
{
	// 간단한 1D 펄린 노이즈 구현
	// 참고: Ken Perlin의 개선된 노이즈 (2002)
	
	int X0 = static_cast<int>(std::floor(X)) & 255;
	int X1 = (X0 + 1) & 255;
	
	float Fx = X - std::floor(X);
	
	// Smoothstep 보간
	float U = SmoothStep(Fx);
	
	// 의사 랜덤 기울기를 위한 간단한 해시 함수
	auto Hash = [](int I) -> float {
		I = (I << 13) ^ I;
		return (1.0f - ((I * (I * I * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f);
	};
	
	float G0 = Hash(X0);
	float G1 = Hash(X1);
	
	// 선형 보간
	return G0 + U * (G1 - G0);
}

float UCameraModifier_CameraShake::SmoothStep(float t) const
{
	// Smoothstep 함수: 3t^2 - 2t^3
	// 끝점에서 0인 도함수로 부드러운 보간 제공
	t = std::clamp(t, 0.0f, 1.0f);
	return t * t * (3.0f - 2.0f * t);
}
