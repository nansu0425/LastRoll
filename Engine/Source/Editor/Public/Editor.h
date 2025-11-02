#pragma once
#include "Core/Public/Object.h"
#include "Editor/Public/Gizmo.h"
#include "Editor/Public/Grid.h"
#include "Editor/Public/ObjectPicker.h"
#include "Editor/Public/BatchLines.h"
#include "editor/Public/Camera.h"
#include "Global/Function.h"

class UPrimitiveComponent;
class UUUIDTextComponent;
class FViewportClient;
class UCamera;
class ULevel;
class USplitterWidget;
struct FRay;

class UEditor : public UObject
{
	DECLARE_CLASS(UEditor, UObject)

public:
	UEditor();
	~UEditor();

	void Update();
	void RenderEditorGeometry();
	void Collect2DRender(UCamera* InCamera, const D3D11_VIEWPORT& InViewport);
	void RenderGizmo(UCamera* InCamera, const D3D11_VIEWPORT& InViewport);
	void RenderGizmoForHitProxy(UCamera* InCamera, const D3D11_VIEWPORT& InViewport);

	void SelectActor(AActor* InActor);
	void SelectComponent(UActorComponent* InComponent);
	void SelectActorAndComponent(AActor* InActor, UActorComponent* InComponent);
	void FocusOnSelectedActor();

	// Alt + Drag 복사 기능
	UActorComponent* DuplicateComponent(UActorComponent* InSourceComponent, AActor* InParentActor);
	AActor* DuplicateActor(AActor* InSourceActor);

	// Getter & Setter
	EViewModeIndex GetViewMode() const { return CurrentViewMode; }
	AActor* GetSelectedActor() const { return SelectedActor; }
	UActorComponent* GetSelectedComponent() const { return SelectedComponent; }
	bool IsPilotMode() const { return bIsPilotMode; }
	AActor* GetPilotedActor() const { return PilotedActor; }
	UBatchLines* GetBatchLines() { return &BatchLines; }
	UGizmo* GetGizmo() { return &Gizmo; }

	/**
	 * @brief 현재 활성화된 World(GWorld)에 해당하는 선택된 Actor 반환
	 * PIE 모드면 PIESelectedActor, Editor 모드면 SelectedActor 반환
	 */
	AActor* GetSelectedActorForCurrentWorld() const;

	/**
	 * @brief 현재 활성화된 World(GWorld)에 해당하는 선택된 Component 반환
	 * PIE 모드면 PIESelectedComponent, Editor 모드면 SelectedComponent 반환
	 */
	UActorComponent* GetSelectedComponentForCurrentWorld() const;

	/**
	 * @brief PIE World의 Actor 선택 (아웃라이너에서 PIE World Actor 선택 시 사용)
	 */
	void SelectPIEActor(AActor* InActor);
	void SelectPIEActorAndComponent(AActor* InActor, UActorComponent* InComponent);
	void SelectPIEComponent(UActorComponent* InComponent);

	/**
	 * @brief PIE 선택 상태 초기화 (PIE 시작/종료 시 호출)
	 */
	void ClearPIESelection();

	void SetViewMode(EViewModeIndex InNewViewMode) { CurrentViewMode = InNewViewMode; }

	// Pilot Mode Public Interface
	void RequestExitPilotMode();

private:
	UObjectPicker ObjectPicker;

	// Editor World 선택 상태
	AActor* SelectedActor = nullptr; // 선택된 액터 (Editor World)
	UActorComponent* SelectedComponent = nullptr; // 선택된 컴포넌트 (Editor World)

	// PIE World 선택 상태
	AActor* PIESelectedActor = nullptr; // 선택된 액터 (PIE World)
	UActorComponent* PIESelectedComponent = nullptr; // 선택된 컴포넌트 (PIE World)

	// 선택 타입 (Actor vs Component)
	bool bIsActorSelected = true; // true: Actor 선택 (Root Component), false: Component 선택
	bool bIsPIEActorSelected = true; // PIE World용 선택 타입

	// Alt + 드래그 복사 모드
	bool bIsInCopyMode = false; // Alt 키 누른 상태로 드래그 시작 시 true
	AActor* CopiedActor = nullptr; // 복사된 Actor (복사 모드 시)
	UActorComponent* CopiedComponent = nullptr; // 복사된 Component (복사 모드 시)

	UCamera* Camera;
	UGizmo Gizmo;
	UBatchLines BatchLines;

	EViewModeIndex CurrentViewMode = EViewModeIndex::VMI_BlinnPhong;

	int32 ActiveViewportIndex = 0;

	// 드래그 중 뷰포트 고정 처리를 위한 트래킹
	int32 LockedViewportIndexForDrag = -1;
	bool bWasRightMouseDown = false;

	// 최소 스케일 값 설정
	static constexpr float MIN_SCALE_VALUE = 0.01f;

	// Camera focus animation
	bool bIsCameraAnimating = false;
	float CameraAnimationTime = 0.0f;
	ECameraType AnimatingCameraType = ECameraType::ECT_Perspective;
	TArray<FVector> CameraStartLocation;
	TArray<FVector> CameraStartRotation;
	TArray<FVector> CameraTargetLocation;
	TArray<FVector> CameraTargetRotation;
	TArray<float> OrthoZoomStart;
	TArray<float> OrthoZoomTarget;
	static constexpr float CAMERA_ANIMATION_DURATION = 0.3f;

	// Pilot Mode
	bool bIsPilotMode = false;
	AActor* PilotedActor = nullptr;
	int32 PilotModeViewportIndex = -1;
	FVector PilotModeStartCameraLocation;
	FVector PilotModeStartCameraRotation;
	FVector PilotModeFixedGizmoLocation;

	void UpdateBatchLines();
	void ProcessMouseInput();
	
	// 모든 기즈모 드래그 함수가 ActiveCamera를 받도록 통일
	FVector GetGizmoDragLocation(UCamera* InActiveCamera, FRay& WorldRay);
	FQuaternion GetGizmoDragRotation(UCamera* InActiveCamera, FRay& WorldRay);
	FVector GetGizmoDragScale(UCamera* InActiveCamera, FRay& WorldRay);

	// Focus Target Calculation
	bool GetComponentFocusTarget(UActorComponent* Component, FVector& OutCenter, float& OutRadius);
	bool GetActorFocusTarget(AActor* Actor, FVector& OutCenter, float& OutRadius);

	void UpdateCameraAnimation();
	
	void TogglePilotMode();
	void UpdatePilotMode();
	void ExitPilotMode();
};
