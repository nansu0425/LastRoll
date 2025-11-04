#pragma once
#include "Core/Public/Object.h"

class APlayerCameraManager; // 전방 선언
struct FMinimalViewInfo;    // 전방 선언

/**
 * @brief 카메라 모디파이어 블렌드 상태
 */
UENUM()
enum class ECameraModifierBlendMode : uint8
{
	Disabled,      // 모디파이어 비활성 (Alpha = 0.0)
	BlendingIn,    // Alpha 증가 중 (0.0 → 1.0)
	Active,        // Alpha가 1.0 (완전 활성)
	BlendingOut,   // Alpha 감소 중 (1.0 → 0.0)
	End
};
DECLARE_UINT8_ENUM_REFLECTION(ECameraModifierBlendMode)

/**
 * @brief 카메라 후처리 효과를 위한 베이스 클래스
 *
 * UCameraModifier는 다양한 카메라 효과 구현을 가능하게 합니다:
 * - 카메라 흔들림 (위치/회전 오프셋)
 * - 카메라 래그 (스프링 암 추적)
 * - FOV 전환 (줌 인/아웃)
 * - 시선 제약 (타겟 주시)
 * - 커스텀 카메라 동작
 *
 * 모디파이어는 APlayerCameraManager에 의해 우선순위 순서대로 처리됩니다.
 * 각 모디파이어는 AlphaInTime/AlphaOutTime을 사용하여 시간에 따라 블렌드됩니다.
 */
UCLASS()
class UCameraModifier : public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(UCameraModifier, UObject)

protected:
	// 소유자
	APlayerCameraManager* CameraOwner;  // 카메라 매니저 역참조

	// 블렌드 타이밍
	float AlphaInTime;                  // 블렌드 인 시간 (0.0 = 즉시)
	float AlphaOutTime;                 // 블렌드 아웃 시간 (0.0 = 즉시)
	float Alpha;                        // 현재 블렌드 가중치 [0.0, 1.0]

	// 상태
	ECameraModifierBlendMode BlendMode; // 현재 블렌드 상태
	bool bDisabled;                     // true면 이 모디파이어를 건너뜀
	uint8 Priority;                     // 높을수록 나중에 처리됨 (더 강한 영향)

public:
	UCameraModifier();
	virtual ~UCameraModifier() override;

	/**
	 * @brief 카메라 소유자로 모디파이어 초기화
	 *
	 * 모디파이어가 추가될 때 APlayerCameraManager가 호출합니다.
	 *
	 * @param InOwner 이 모디파이어를 소유하는 카메라 매니저
	 */
	virtual void Initialize(APlayerCameraManager* InOwner);

	/**
	 * @brief 블렌드 모드에 따라 블렌드 알파 업데이트
	 *
	 * 시간에 따라 자동 블렌드 인/아웃을 처리합니다. 매 프레임 카메라 매니저가 호출합니다.
	 *
	 * @param DeltaTime 마지막 프레임 이후 시간 (초)
	 */
	virtual void UpdateAlpha(float DeltaTime);

	/**
	 * @brief 메인 오버라이드 포인트: 카메라 POV 수정
	 *
	 * 카메라 효과를 적용하려면 이 메서드를 오버라이드하세요. 모디파이어는 우선순위 순서대로 처리됩니다.
	 * InOutPOV 필드(Location, Rotation, FOV 등)를 수정하여 효과를 적용합니다.
	 *
	 * @param DeltaTime 마지막 프레임 이후 시간 (초)
	 * @param InOutPOV 수정할 카메라 POV (입출력 파라미터)
	 * @return POV가 수정되었으면 true, 아니면 false
	 */
	virtual bool ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV);

	/**
	 * @brief 모디파이어 활성화 (블렌드 인 시작)
	 */
	virtual void EnableModifier();

	/**
	 * @brief 모디파이어 비활성화
	 *
	 * @param bImmediate true면 즉시 비활성화 (Alpha = 0.0). false면 AlphaOutTime에 걸쳐 블렌드 아웃.
	 */
	virtual void DisableModifier(bool bImmediate);

	// Getter
	bool IsDisabled() const { return bDisabled; }
	uint8 GetPriority() const { return Priority; }
	float GetAlpha() const { return Alpha; }
	APlayerCameraManager* GetCameraOwner() const { return CameraOwner; }

	// Setter
	void SetPriority(uint8 InPriority) { Priority = InPriority; }
	void SetAlphaInTime(float InTime) { AlphaInTime = InTime; }
	void SetAlphaOutTime(float InTime) { AlphaOutTime = InTime; }
};
