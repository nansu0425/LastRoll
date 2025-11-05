#pragma once

#include "Render/UI/Window/Public/UIWindow.h"
#include "Editor/Public/CameraShakePresetDetailPanel.h"

/**
 * @brief Camera Shake Preset 에디터 윈도우
 *
 * UCameraShakePresetManager에 등록된 Preset들을 시각적으로 편집하고 관리합니다.
 *
 * 기능:
 * - Preset 목록 표시 (왼쪽 패널)
 * - Preset 추가/제거/이름 변경
 * - Preset 상세 편집 (오른쪽 패널: Duration, Amplitude, Pattern, Bezier Curve 등)
 * - Preset 파일 저장/로드 (JSON)
 * - PIE 모드에서 실시간 테스트
 *
 * 사용 예시:
 * ```cpp
 * // UIWindowFactory에서 생성
 * UCameraShakePresetEditorWindow* PresetEditor =
 *     UUIWindowFactory::CreateCameraShakePresetEditorWindow();
 *
 * // 또는 MainMenu에서 "Tools > Camera Shake Presets" 메뉴로 열기
 * ```
 */
UCLASS()
class UCameraShakePresetEditorWindow : public UUIWindow
{
	GENERATED_BODY()
	DECLARE_CLASS(UCameraShakePresetEditorWindow, UUIWindow)

public:
	UCameraShakePresetEditorWindow();
	virtual ~UCameraShakePresetEditorWindow() override;

	// UUIWindow 오버라이드
	virtual void Initialize() override;
	virtual void OnPreRenderWindow(float MenuBarOffset) override;

private:
	/**
	 * @brief 왼쪽 패널: Preset 목록 및 관리 버튼
	 */
	void DrawPresetListPanel();

	/**
	 * @brief 오른쪽 패널: 선택된 Preset 상세 편집
	 */
	void DrawPresetDetailPanel();

	/**
	 * @brief 하단 패널: 파일 입출력 및 테스트 버튼
	 */
	void DrawBottomPanel();

	/**
	 * @brief Add New Preset 다이얼로그
	 */
	void DrawAddPresetDialog();

	/**
	 * @brief Remove Preset 확인 다이얼로그
	 */
	void DrawRemovePresetDialog();

	/**
	 * @brief PlayerCameraManager 찾기 또는 생성
	 * @param World 대상 월드
	 * @return PlayerCameraManager 인스턴스 (nullptr 가능)
	 */
	APlayerCameraManager* FindOrCreateCameraManager(UWorld* World);

private:
	// Detail Panel 인스턴스
	FCameraShakePresetDetailPanel DetailPanel;

	// 선택된 Preset
	FName SelectedPresetName;

	// UI 상태
	bool bShowAddDialog;            // Add Preset 다이얼로그 표시 여부
	bool bShowRemoveDialog;         // Remove Preset 확인 다이얼로그 표시 여부
	char NewPresetNameBuffer[128];  // 새 Preset 이름 입력 버퍼

	// 파일 경로
	FString DefaultPresetsFilePath; // 기본 Preset 파일 경로 ("Engine/Data/CameraShakePresets.json")

	// 임시 카메라 액터 (ViewTarget 설정용)
	AActor* TempCameraActor = nullptr;
};
