#pragma once

#include "Component/Camera/Public/CameraModifier_CameraShake.h"
#include "ImGui/ImGuiBezierEditor.h"

class UWorld;

/**
 * @brief UCameraModifier_CameraShake의 상세 설정 패널
 *
 * ImGui를 사용하여 Camera Shake 모디파이어의 모든 파라미터를 편집할 수 있습니다:
 * - Bezier 곡선 에디터 (감쇠 곡선 편집)
 * - Preset 버튼 (Linear, EaseIn, EaseOut, EaseInOut, Bounce)
 * - 흔들림 파라미터 (Duration, Amplitude, Pattern, Frequency)
 * - 실시간 프리뷰 (현재 곡선 값 표시)
 * - 테스트 버튼 (흔들림 즉시 시작/중지)
 *
 * 사용 예시:
 * ```cpp
 * FCameraShakeDetailPanel Panel;
 * if (UCameraModifier_CameraShake* Shake = GetSelectedCameraShake())
 * {
 *     Panel.Draw("CameraShakeDetail", Shake);
 * }
 * ```
 */
class FCameraShakeDetailPanel
{
public:
	FCameraShakeDetailPanel();
	~FCameraShakeDetailPanel();

	/**
	 * @brief Detail Panel UI 렌더링
	 *
	 * @param Label 패널 고유 ID (ImGui ID로 사용)
	 * @param CameraShake 편집할 Camera Shake 모디파이어
	 * @param World 현재 World (PIE 모드 확인용)
	 * @return true면 파라미터가 변경됨
	 */
	bool Draw(const char* Label, UCameraModifier_CameraShake* CameraShake, UWorld* World);

private:
	/**
	 * @brief Bezier 곡선 에디터 섹션
	 *
	 * @param CameraShake 편집할 Camera Shake
	 * @return true면 곡선이 변경됨
	 */
	bool DrawBezierEditor(UCameraModifier_CameraShake* CameraShake);

	/**
	 * @brief Preset 버튼 섹션
	 *
	 * @param CameraShake 편집할 Camera Shake
	 * @return true면 preset이 선택됨
	 */
	bool DrawPresetButtons(UCameraModifier_CameraShake* CameraShake);

	/**
	 * @brief 곡선 프리뷰 그래프
	 *
	 * @param CameraShake 편집할 Camera Shake
	 */
	void DrawCurvePreview(UCameraModifier_CameraShake* CameraShake);

	/**
	 * @brief 흔들림 파라미터 편집 섹션
	 *
	 * @param CameraShake 편집할 Camera Shake
	 * @return true면 파라미터가 변경됨
	 */
	bool DrawShakeParameters(UCameraModifier_CameraShake* CameraShake);

	/**
	 * @brief 테스트 컨트롤 섹션 (Start/Stop 버튼)
	 *
	 * @param CameraShake 편집할 Camera Shake
	 * @param World 현재 World (PIE 모드 확인용)
	 */
	void DrawTestControls(UCameraModifier_CameraShake* CameraShake, UWorld* World);

private:
	// Bezier 에디터 인스턴스
	FImGuiBezierEditor BezierEditor;

	// 에디터 상태
	bool bShowPreview;          // 프리뷰 그래프 표시 여부
	float PreviewSampleX;       // 프리뷰 샘플링 X 좌표 [0, 1]

	// 임시 파라미터 (UI 편집용)
	float TempDuration;
	float TempLocationAmplitude;
	float TempRotationAmplitude;
	int32 TempPattern;          // ECameraShakePattern as int
	float TempFrequency;
	bool bTempUseDecayCurve;

	// UI 스타일
	float PresetButtonWidth;
	float PreviewGraphHeight;
};
