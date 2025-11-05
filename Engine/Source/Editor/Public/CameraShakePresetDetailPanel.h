#pragma once

#include "Global/Public/CameraShakeTypes.h"
#include "ImGui/ImGuiBezierEditor.h"

class UWorld;

/**
 * @brief FCameraShakePresetData의 상세 편집 패널
 *
 * ImGui를 사용하여 Camera Shake Preset의 모든 파라미터를 편집할 수 있습니다:
 * - Preset 이름 편집
 * - Bezier 곡선 에디터 (감쇠 곡선 편집)
 * - Preset 버튼 (Linear, EaseIn, EaseOut, EaseInOut)
 * - 흔들림 파라미터 (Duration, Amplitude, Pattern, Frequency)
 * - 실시간 프리뷰 (현재 곡선 값 표시)
 *
 * 사용 예시:
 * ```cpp
 * FCameraShakePresetDetailPanel DetailPanel;
 * FCameraShakePresetData* Preset = PresetManager.FindPreset(FName("Explosion"));
 * if (DetailPanel.Draw("PresetDetail", Preset))
 * {
 *     // Preset 데이터가 변경됨
 * }
 * ```
 */
class FCameraShakePresetDetailPanel
{
public:
	FCameraShakePresetDetailPanel();
	~FCameraShakePresetDetailPanel();

	/**
	 * @brief Detail Panel UI 렌더링
	 *
	 * @param Label 패널 고유 ID (ImGui ID로 사용)
	 * @param Preset 편집할 Preset 데이터
	 * @return true면 파라미터가 변경됨
	 */
	bool Draw(const char* Label, FCameraShakePresetData* Preset);

private:
	/**
	 * @brief Preset 이름 편집 섹션
	 *
	 * @param Preset 편집할 Preset
	 * @return true면 이름이 변경됨
	 */
	bool DrawPresetName(FCameraShakePresetData* Preset);

	/**
	 * @brief Bezier 곡선 에디터 섹션
	 *
	 * @param Preset 편집할 Preset
	 * @return true면 곡선이 변경됨
	 */
	bool DrawBezierEditor(FCameraShakePresetData* Preset);

	/**
	 * @brief Preset 버튼 섹션 (Linear, EaseIn, EaseOut 등)
	 *
	 * @param Preset 편집할 Preset
	 * @return true면 Preset이 적용됨
	 */
	bool DrawPresetButtons(FCameraShakePresetData* Preset);

	/**
	 * @brief 프리뷰 그래프 섹션 (곡선 시각화)
	 *
	 * @param Preset 편집할 Preset
	 */
	void DrawPreviewGraph(FCameraShakePresetData* Preset);

	/**
	 * @brief 흔들림 파라미터 섹션 (Duration, Amplitude 등)
	 *
	 * @param Preset 편집할 Preset
	 * @return true면 파라미터가 변경됨
	 */
	bool DrawShakeParameters(FCameraShakePresetData* Preset);

private:
	// Bezier 에디터 인스턴스
	FImGuiBezierEditor BezierEditor;

	// 에디터 상태
	bool bShowPreview;          // 프리뷰 그래프 표시 여부
	float PreviewSampleX;       // 프리뷰 샘플링 X 좌표 [0, 1]

	// 임시 편집 버퍼 (텍스트 입력용)
	char TempPresetName[128];   // Preset 이름 임시 버퍼
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
