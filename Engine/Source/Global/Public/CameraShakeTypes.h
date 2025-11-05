#pragma once

#include "Global/Types.h"
#include "Global/Public/BezierCurve.h"

// 전방 선언
enum class ECameraShakePattern : uint8;

namespace json { class JSON; }
using JSON = json::JSON;

/**
 * @brief Camera Shake Preset 데이터 구조체
 *
 * 재사용 가능한 Camera Shake 설정을 저장합니다.
 * Preset은 JSON 파일로 저장/로드되며, PresetManager가 관리합니다.
 *
 * 사용 예시:
 * ```cpp
 * FCameraShakePresetData ExplosionPreset;
 * ExplosionPreset.PresetName = FName("Explosion");
 * ExplosionPreset.Duration = 2.0f;
 * ExplosionPreset.LocationAmplitude = 30.0f;
 * ExplosionPreset.RotationAmplitude = 8.0f;
 * ExplosionPreset.Pattern = ECameraShakePattern::Perlin;
 * ExplosionPreset.bUseDecayCurve = true;
 * ExplosionPreset.DecayCurve = FCubicBezierCurve::CreateEaseOut();
 * ```
 */
struct FCameraShakePresetData
{
	/**
	 * @brief Preset 이름 (고유 식별자)
	 *
	 * 예: "Explosion", "Collision", "Earthquake"
	 */
	FName PresetName;

	/**
	 * @brief 흔들림 지속 시간 (초)
	 *
	 * 범위: 0.1 ~ 10.0
	 * 기본값: 1.0
	 */
	float Duration = 1.0f;

	/**
	 * @brief 위치 흔들림 최대 진폭
	 *
	 * 범위: 0.0 ~ 100.0
	 * 기본값: 5.0
	 */
	float LocationAmplitude = 5.0f;

	/**
	 * @brief 회전 흔들림 최대 진폭 (도 단위)
	 *
	 * 범위: 0.0 ~ 45.0
	 * 기본값: 2.0
	 */
	float RotationAmplitude = 2.0f;

	/**
	 * @brief 흔들림 패턴
	 *
	 * Sine: 사인파 (부드러운 진동)
	 * Perlin: Perlin 노이즈 (자연스러운 흔들림)
	 * Random: 랜덤 (불규칙한 흔들림)
	 *
	 * 기본값: Perlin
	 */
	ECameraShakePattern Pattern;

	/**
	 * @brief 주파수 (Hz) - Sine 패턴에서만 사용
	 *
	 * 범위: 0.1 ~ 50.0
	 * 기본값: 10.0
	 */
	float Frequency = 10.0f;

	/**
	 * @brief Bezier 곡선 감쇠 사용 여부
	 *
	 * true: DecayCurve 사용
	 * false: 기본 smoothstep 감쇠 사용
	 */
	bool bUseDecayCurve = false;

	/**
	 * @brief Bezier 곡선 감쇠 패턴
	 *
	 * bUseDecayCurve가 true일 때만 사용됩니다.
	 * 시간에 따른 진폭 감쇠를 제어합니다.
	 */
	FCubicBezierCurve DecayCurve;

	/**
	 * @brief 기본 생성자
	 *
	 * Pattern은 ECameraShakePattern include 후에만 초기화 가능
	 */
	FCameraShakePresetData();

	/**
	 * @brief JSON 직렬화
	 *
	 * @param bIsLoading true면 JSON에서 로드, false면 JSON으로 저장
	 * @param InOutHandle JSON 핸들
	 */
	void Serialize(bool bIsLoading, JSON& InOutHandle);
};
