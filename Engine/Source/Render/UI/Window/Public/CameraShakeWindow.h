#pragma once
#include "UIWindow.h"
#include "Editor/Public/CameraShakeDetailPanel.h"

class UCameraModifier_CameraShake;
class APlayerCameraManager;

/**
 * @brief Camera Shake 전용 에디터 윈도우
 *
 * UCameraModifier_CameraShake의 파라미터를 편집할 수 있는 독립 윈도우입니다.
 * FCameraShakeDetailPanel을 사용하여 Bezier 곡선 에디터와 모든 shake 파라미터를 제공합니다.
 *
 * 사용 방법:
 * - 에디터에서 Window > Camera Shake Editor 메뉴로 열기
 * - PlayerCameraManager에서 CameraShake 모디파이어를 자동으로 찾아서 표시
 */
class UCameraShakeWindow : public UUIWindow
{
	DECLARE_CLASS(UCameraShakeWindow, UUIWindow)

public:
	UCameraShakeWindow();
	~UCameraShakeWindow() override = default;

	void Initialize() override;
	void OnPreRenderWindow(float MenuBarOffset) override;

	/**
	 * @brief Camera Shake 모디파이어 설정 (외부에서 설정 가능)
	 *
	 * @param InCameraShake 표시할 Camera Shake 모디파이어
	 */
	void SetCameraShake(UCameraModifier_CameraShake* InCameraShake);

	/**
	 * @brief 현재 레벨의 PlayerCameraManager에서 자동으로 CameraShake를 찾아서 설정
	 *
	 * @return true면 CameraShake를 찾았음, false면 찾지 못함
	 */
	bool FindAndSetCameraShake();

private:
	FCameraShakeDetailPanel DetailPanel;
	UCameraModifier_CameraShake* CameraShake = nullptr;

	// 자동 새로고침 설정
	float RefreshTimer = 0.0f;
	float RefreshInterval = 1.0f;  // 1초마다 CameraShake 재검색

	// ViewTarget으로 사용할 임시 Camera Actor (자동 생성)
	AActor* TempCameraActor = nullptr;
};
