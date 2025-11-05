#pragma once
#include "Actor/Public/Actor.h"
#include "Global/CoreTypes.h"

class UCameraComponent;   // 전방 선언
class UCameraModifier;    // 전방 선언

/**
 * @brief 카메라 블렌딩을 위한 뷰 타겟 구조체
 */
struct FViewTarget
{
	AActor* Target;                        // 타겟 액터 (null 가능)
	UCameraComponent* CameraComponent;     // 타겟의 카메라 컴포넌트 (null 가능)
	FMinimalViewInfo POV;                  // 현재 POV 데이터

	FViewTarget()
		: Target(nullptr)
		, CameraComponent(nullptr)
		, POV()
	{
	}
};

/**
 * @brief 플레이어 카메라 매니저 - 활성 카메라와 카메라 모디파이어 관리
 *
 * APlayerCameraManager는 게임/PIE 모드를 위한 중앙 카메라 시스템입니다. 다음을 수행합니다:
 * 1. 활성 뷰 타겟(카메라 컴포넌트가 있는 액터) 관리
 * 2. 카메라 모디파이어 체인(흔들림, 래그, FOV 전환 등) 처리
 * 3. 뷰 타겟 블렌딩(카메라 간 부드러운 전환) 처리
 * 4. 카메라 페이딩(색상으로 페이드 인/아웃) 처리
 * 5. 렌더링을 위한 최종 View/Projection 행렬 생성
 *
 * 업데이트 파이프라인 (매 프레임 호출):
 * UpdateViewTarget() → UpdateBlending() → ApplyCameraModifiers() → UpdateFading() → UpdateCameraConstants()
 */
UCLASS()
class APlayerCameraManager : public AActor
{
	GENERATED_BODY()
	DECLARE_CLASS(APlayerCameraManager, AActor)

private:
	// ===== 뷰 타겟 =====
	FViewTarget ViewTarget;             // 현재 뷰 타겟
	FViewTarget PendingViewTarget;      // 블렌드할 타겟 (전환 중)

	// ===== 블렌딩 =====
	float BlendTime;                    // 총 블렌드 지속 시간
	float BlendTimeRemaining;           // 블렌드 완료까지 남은 시간
	bool bIsBlending;                   // 뷰 타겟 전환 중일 때 true

	// ===== 카메라 페이딩 =====
	FVector FadeColor;                 // 페이드 오버레이 색상 (예: 검은색으로 페이드)
	float FadeAmount;                   // 목표 페이드 양 [0.0, 1.0]
	FVector2 FadeAlpha;                 // (현재, 목표) 페이드 알파
	float FadeTime;                     // 페이드 지속 시간
	float FadeTimeRemaining;            // 페이드 남은 시간
	bool bIsFading;                     // 페이드 중일 때 true

	// ===== 카메라 종횡비 =====
	float TargetAspectRatio;			// 뷰포트의 종횡비와는 다른 카메라가 목표로하는 종횡비 (레터박스에서 사용)
										// @todo: 변수를 카메라 컴포넌트로 옮겨야 할 수 있음
	
	// ===== 카메라 스타일 =====
	FName CameraStyle;                  // 현재 카메라 스타일 이름 (예: "Default", "ThirdPerson")

	// ===== 카메라 모디파이어 =====
	TArray<UCameraModifier*> ModifierList;  // 활성 카메라 모디파이어 (우선순위로 처리)

	// ===== 캐시된 결과 =====
	FMinimalViewInfo CachedPOV;         // 모디파이어 체인 후 최종 POV
	FCameraConstants CachedCameraConstants; // 최종 View/Projection 행렬

public:
	APlayerCameraManager();
	virtual ~APlayerCameraManager() override;

	// ===== 생명주기 =====
	virtual void BeginPlay() override;

	// ===== 뷰 타겟 관리 =====
	/**
	 * @brief 새 뷰 타겟 설정 (사용할 카메라)
	 *
	 * BlendTime > 0이면, 현재 카메라에서 새 카메라로 부드럽게 전환합니다.
	 * BlendTime = 0이면, 즉시 새 카메라로 전환합니다.
	 *
	 * @param NewTarget 뷰를 가져올 액터 (UCameraComponent가 있어야 하거나 액터 변환 사용)
	 * @param InBlendTime 블렌드 전환 지속 시간 (초). 0 = 즉시.
	 */
	void SetViewTarget(AActor* NewTarget, float InBlendTime = 0.0f);

	/**
	 * @brief 현재 뷰 타겟 액터 가져오기
	 */
	AActor* GetViewTarget() const { return ViewTarget.Target; }

	/**
	 * @brief 현재 뷰 타겟의 카메라 컴포넌트 가져오기
	 */
	UCameraComponent* GetViewTargetCamera() const { return ViewTarget.CameraComponent; }

	// ===== 카메라 모디파이어 관리 =====
	/**
	 * @brief 처리 체인에 카메라 모디파이어 추가
	 *
	 * 모디파이어 클래스의 새 인스턴스를 생성하고 ModifierList에 추가합니다.
	 * 모디파이어는 초기화되고 다음 UpdateCamera()에서 처리를 시작합니다.
	 *
	 * @param ModifierClass 생성할 모디파이어 클래스 (예: UCameraModifier_CameraShake::StaticClass())
	 * @return 생성된 모디파이어 인스턴스 (실패 시 null)
	 */
	UCameraModifier* AddCameraModifier(UClass* ModifierClass);

	/**
	 * @brief 처리 체인에서 카메라 모디파이어 제거
	 *
	 * @param Modifier 제거할 모디파이어 인스턴스
	 * @return 성공적으로 제거되면 true
	 */
	bool RemoveCameraModifier(UCameraModifier* Modifier);

	/**
	 * @brief 클래스로 카메라 모디파이어 찾기
	 *
	 * @param ModifierClass 찾을 클래스
	 * @return 주어진 클래스의 첫 번째 모디파이어, 없으면 null
	 */
	UCameraModifier* FindCameraModifierByClass(UClass* ModifierClass) const;

	/**
	 * @brief 모든 카메라 모디파이어 제거
	 */
	void ClearAllCameraModifiers();

	// ===== 카메라 페이드 API =====
	/**
	 * @brief 카메라 페이드 효과 시작
	 *
	 * 시간에 걸쳐 화면을 지정된 색상으로 페이드합니다. 일반적인 사용 예:
	 * - 사망 시 검an색으로 페이드: StartCameraFade(0.0, 1.0, 1.0, FVector4(0,0,0,1))
	 * - 스폰 시 검은색에서 페이드: StartCameraFade(1.0, 0.0, 1.0, FVector4(0,0,0,1))
	 *
	 * @param FromAlpha 시작 페이드 알파 [0.0, 1.0]. 0 = 페이드 없음, 1 = 전체 페이드
	 * @param ToAlpha 목표 페이드 알파 [0.0, 1.0]
	 * @param Duration 페이드 지속 시간 (초)
	 * @param Color 페이드 오버레이 색상
	 */
	void StartCameraFade(float FromAlpha, float ToAlpha, float Duration, FVector4 Color);

	/**
	 * @brief 카메라 페이드 즉시 중지
	 */
	void StopCameraFade();

	// ===== 메인 업데이트 함수 =====
	/**
	 * @brief 카메라 업데이트 (매 프레임 UWorld::Tick이 호출)
	 *
	 * 메인 업데이트 파이프라인:
	 * 1. UpdateViewTarget() - 뷰 타겟의 카메라에서 기본 POV 가져오기
	 * 2. UpdateBlending() - 전환 중이면 뷰 타겟 간 블렌드
	 * 3. ApplyCameraModifiers() - 카메라 모디파이어 체인 처리 (우선순위 순서)
	 * 4. UpdateFading() - 화면 페이드 효과 업데이트
	 * 5. UpdateCameraConstants() - 최종 POV를 View/Projection 행렬로 변환
	 *
	 * @param DeltaTime 마지막 프레임 이후 시간 (초)
	 */
	void UpdateCamera(float DeltaTime);

	// ===== 최종 카메라 접근 =====
	/**
	 * @brief 최종 카메라 상수 가져오기 (View/Projection 행렬)
	 *
	 * URenderer가 렌더링을 위해 카메라 행렬을 가져올 때 사용합니다.
	 *
	 * @return 모디파이어 체인 처리 후 최종 카메라 상수
	 */
	const FCameraConstants& GetCameraConstants() const { return CachedCameraConstants; }

	/**
	 * @brief 최종 카메라 POV 가져오기
	 *
	 * @return 모디파이어 체인 처리 후 최종 POV
	 */
	const FMinimalViewInfo& GetCameraCachePOV() const { return CachedPOV; }

protected:
	// ===== 내부 업데이트 파이프라인 =====
	/**
	 * @brief 1단계: 현재 ViewTarget에서 기본 POV 가져오기
	 */
	void UpdateViewTarget(float DeltaTime);

	/**
	 * @brief 2단계: 전환 중이면 ViewTarget 간 블렌드
	 */
	void UpdateBlending(float DeltaTime);

	/**
	 * @brief 3단계: 카메라 모디파이어 체인 적용 (우선순위 순서)
	 */
	void ApplyCameraModifiers(float DeltaTime);

	/**
	 * @brief 4단계: 화면 페이드 효과 업데이트
	 */
	void UpdateFading(float DeltaTime);

	/**
	 * @brief 5단계: 최종 POV를 View/Projection 행렬로 변환
	 */
	void UpdateCameraConstants();

	// ===== 직렬화 =====
public:
	virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
};
