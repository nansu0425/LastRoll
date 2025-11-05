#pragma once
#include "Component/Public/SceneComponent.h"

struct FMinimalViewInfo; // 전방 선언

/**
 * @brief 게임 카메라 컴포넌트 - 액터에 부착 가능
 *
 * UCameraComponent는 게임 액터(플레이어 폰, 차량 등)를 위한 카메라 기능을 제공합니다.
 * 투영 파라미터(FOV, 종횡비, 클립 평면)를 저장하고 APlayerCameraManager와
 * 카메라 모디파이어가 사용하는 카메라 뷰 데이터를 생성합니다.
 */
UCLASS()
class UCameraComponent : public USceneComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(UCameraComponent, USceneComponent)

private:
	// 투영 파라미터
	float FieldOfView;                  // 수직 FOV (도 단위), 기본값: 90.0f
	float AspectRatio;                  // 너비 / 높이, 기본값: 16.0f/9.0f
	float NearClipPlane;                // Near Z, 기본값: 1.0f
	float FarClipPlane;                 // Far Z, 기본값: 10000.0f
	float TargetAspectRatio;			// 목표 종횡비 (레터박스에 활용)

	// 카메라 타입
	bool bUsePerspectiveProjection;     // true: 원근 투영, false: 직교 투영
	float OrthoWidth;                   // 직교 투영 너비, 기본값: 1000.0f

	// ===== 후처리 설정 =====
	FPostProcessSettings PostProcessSettings;

public:
	UCameraComponent();
	virtual ~UCameraComponent() override;

	/**
	 * @brief 카메라 뷰 정보 가져오기 (POV)
	 *
	 * 현재 카메라 변환(SceneComponent 계층에서)과 투영 파라미터로
	 * FMinimalViewInfo를 채웁니다. 이 POV는 APlayerCameraManager가
	 * 기본 카메라 데이터로 사용합니다.
	 *
	 * @param OutPOV 카메라 위치, 회전, 투영 설정으로 채워질 출력 파라미터
	 */
	void GetCameraView(FMinimalViewInfo& OutPOV) const;

	// Setter
	void SetFieldOfView(float InFOV) { FieldOfView = InFOV; }
	void SetAspectRatio(float InAspect) { AspectRatio = InAspect; }
	void SetNearClipPlane(float InNear) { NearClipPlane = InNear; }
	void SetFarClipPlane(float InFar) { FarClipPlane = InFar; }
	void SetProjectionType(bool bInUsePerspective) { bUsePerspectiveProjection = bInUsePerspective; }
	void SetOrthoWidth(float InWidth) { OrthoWidth = InWidth; }
	void SetTargetAspectRatio(float InTargetAspect){ TargetAspectRatio = InTargetAspect; }

	// Getter
	float GetFieldOfView() const { return FieldOfView; }
	float GetAspectRatio() const { return AspectRatio; }
	float GetNearClipPlane() const { return NearClipPlane; }
	float GetFarClipPlane() const { return FarClipPlane; }
	bool IsUsingPerspectiveProjection() const { return bUsePerspectiveProjection; }
	float GetOrthoWidth() const { return OrthoWidth; }
	float GetTargetAspectRatio() const { return TargetAspectRatio; }

	// ===== PostProcessSettings Getter/Setter =====
	FPostProcessSettings& GetPostProcessSettings() { return PostProcessSettings; }
	const FPostProcessSettings& GetPostProcessSettings() const { return PostProcessSettings; }

	// 직렬화
	virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

	// 복제
	virtual UObject* Duplicate() override;
};
