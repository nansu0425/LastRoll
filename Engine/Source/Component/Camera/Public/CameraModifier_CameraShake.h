#pragma once
#include "Component/Camera/Public/CameraModifier.h"
#include "Global/Public/BezierCurve.h"

/**
 * @brief 카메라 흔들림 패턴 타입
 */
UENUM()
enum class ECameraShakePattern : uint8
{
	Sine,           // 부드러운 정현파 진동
	Perlin,         // 유기적인 흔들림을 위한 펄린 노이즈
	Random,         // 랜덤 지터
	End
};
DECLARE_UINT8_ENUM_REFLECTION(ECameraShakePattern)

/**
 * @brief 카메라 흔들림 모디파이어 - 카메라에 절차적 흔들림 추가
 *
 * 지진/폭발/충격 등의 효과를 구현하기 위해 카메라 위치와 회전에
 * 랜덤 또는 절차적 오프셋을 적용합니다. 흔들림 강도는 시간에 따라 감쇠됩니다.
 *
 * 사용 예시:
 * ```cpp
 * UCameraModifier_CameraShake* Shake = Cast<UCameraModifier_CameraShake>(
 *     CameraManager->AddCameraModifier(UCameraModifier_CameraShake::StaticClass())
 * );
 * if (Shake)
 * {
 *     Shake->StartShake(2.0f, 10.0f, 5.0f); // 지속시간=2초, 위치진폭=10, 회전진폭=5도
 * }
 * ```
 */
UCLASS()
class UCameraModifier_CameraShake : public UCameraModifier
{
	GENERATED_BODY()
	DECLARE_CLASS(UCameraModifier_CameraShake, UCameraModifier)

private:
	// 흔들림 파라미터
	float ShakeDuration;          // 총 흔들림 지속 시간 (초)
	float ShakeTimeRemaining;     // 현재 흔들림의 남은 시간
	float LocationAmplitude;      // 최대 위치 오프셋 크기 (월드 단위)
	float RotationAmplitude;      // 최대 회전 오프셋 크기 (도)
	ECameraShakePattern Pattern;  // 흔들림 패턴 타입

	// 흔들림 상태
	bool bIsShaking;              // 현재 흔들림이 활성화되어 있는지
	float ShakeTime;              // 패턴 평가를 위한 누적 시간

	// 패턴 파라미터
	float Frequency;              // 진동 주파수 (Hz)
	FVector PerlinOffset;         // 펄린 노이즈 오프셋 시드
	FVector LastLocationOffset;   // 이전 프레임 위치 오프셋 (스무딩용)
	FVector LastRotationOffset;   // 이전 프레임 회전 오프셋 (스무딩용)

	// 베지어 곡선 기반 감쇠
	FCubicBezierCurve DecayCurve; // 시간에 따른 감쇠 곡선 (X=시간비율, Y=진폭배율)
	bool bUseDecayCurve;          // true면 DecayCurve 사용, false면 기본 감쇠 사용

public:
	UCameraModifier_CameraShake();
	virtual ~UCameraModifier_CameraShake() override;

	/**
	 * @brief 카메라 흔들림 효과 시작
	 *
	 * @param InDuration 흔들림 지속 시간 (초). 0이면 기본값 사용 (1.0초)
	 * @param InLocationAmplitude 위치 흔들림 강도 (월드 단위). 0 = 위치 흔들림 없음
	 * @param InRotationAmplitude 회전 흔들림 강도 (도). 0 = 회전 흔들림 없음
	 * @param InPattern 흔들림 패턴 타입. 기본값: Perlin
	 * @param InFrequency 흔들림 주파수 (Hz). Sine 패턴에만 사용됨. 기본값: 10.0
	 */
	void StartShake(
		float InDuration = 1.0f,
		float InLocationAmplitude = 5.0f,
		float InRotationAmplitude = 2.0f,
		ECameraShakePattern InPattern = ECameraShakePattern::Perlin,
		float InFrequency = 10.0f
	);

	/**
	 * @brief Bezier 곡선 기반 감쇠를 사용하여 카메라 흔들림 시작
	 *
	 * @param InDuration 흔들림 지속 시간 (초)
	 * @param InLocationAmplitude 위치 흔들림 강도 (월드 단위)
	 * @param InRotationAmplitude 회전 흔들림 강도 (도)
	 * @param InPattern 흔들림 패턴 타입
	 * @param InFrequency 흔들림 주파수 (Hz). Sine 패턴에만 사용됨
	 * @param InDecayCurve 감쇠 곡선 (X=정규화된 시간 [0,1], Y=진폭 배율 [0,1])
	 */
	void StartShakeWithCurve(
		float InDuration,
		float InLocationAmplitude,
		float InRotationAmplitude,
		ECameraShakePattern InPattern,
		float InFrequency,
		const FCubicBezierCurve& InDecayCurve
	);

	/**
	 * @brief 흔들림 즉시 중지
	 */
	void StopShake();

	/**
	 * @brief 현재 흔들림이 활성화되어 있는지 확인
	 */
	bool IsShaking() const { return bIsShaking; }

	// ===== Bezier Curve Getter/Setter =====

	/**
	 * @brief 현재 감쇠 곡선 가져오기
	 */
	const FCubicBezierCurve& GetDecayCurve() const { return DecayCurve; }

	/**
	 * @brief 감쇠 곡선 설정
	 * @param InCurve 새로운 감쇠 곡선
	 */
	void SetDecayCurve(const FCubicBezierCurve& InCurve);

	/**
	 * @brief Bezier 곡선 감쇠 사용 여부
	 */
	bool IsUsingDecayCurve() const { return bUseDecayCurve; }

	/**
	 * @brief Bezier 곡선 감쇠 사용 여부 설정
	 * @param bInUse true면 Bezier 곡선 사용, false면 기본 감쇠 사용
	 */
	void SetUseDecayCurve(bool bInUse) { bUseDecayCurve = bInUse; }

	// UCameraModifier에서 오버라이드
	virtual void Initialize(APlayerCameraManager* InOwner) override;
	virtual bool ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV) override;

private:
	/**
	 * @brief 현재 프레임의 흔들림 오프셋 평가
	 *
	 * @param OutLocationOffset 출력 위치 오프셋 (월드 공간)
	 * @param OutRotationOffset 출력 회전 오프셋 (도)
	 * @param CurrentTime 현재 흔들림 시간
	 * @param DecayAlpha 감쇠 배율 [0.0, 1.0]
	 */
	void EvaluateShake(
		FVector& OutLocationOffset,
		FVector& OutRotationOffset,
		float CurrentTime,
		float DecayAlpha
	);

	/**
	 * @brief 펄린 노이즈 값 생성
	 *
	 * @param X 입력 좌표
	 * @return [-1.0, 1.0] 범위의 노이즈 값
	 */
	float PerlinNoise1D(float X) const;

	/**
	 * @brief 부드러운 보간 함수 (smoothstep)
	 */
	float SmoothStep(float t) const;
};
