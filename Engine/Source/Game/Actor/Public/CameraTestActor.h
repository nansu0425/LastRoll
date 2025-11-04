#pragma once
#include "Actor/Public/Actor.h"

/**
 * @brief PIE 모드용 카메라 시스템 테스트 액터
 * 
 * 이 액터는 PlayerCameraManager 기능을 테스트하는 데 사용됩니다:
 * - UCameraComponent 통합
 * - 카메라 모디파이어 (CameraShake)
 * - ViewTarget 전환
 * - 모디파이어 우선순위 시스템
 */
UCLASS()
class ACameraTestActor : public AActor
{
	GENERATED_BODY()
	DECLARE_CLASS(ACameraTestActor, AActor)

private:
	class UCameraComponent* CameraComponent;
	
	// 테스트 상태
	float TestTimer;
	int32 CurrentTestPhase;

public:
	ACameraTestActor();
	virtual ~ACameraTestActor() override;

	virtual UClass* GetDefaultRootComponent() override;
	virtual void InitializeComponents() override;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// 수동 테스트 트리거 (콘솔 또는 입력을 통해 호출)
	void TestCameraShake_Explosion();
	void TestCameraShake_Hit();
	void TestCameraShake_Earthquake();
	void TestViewTargetSwitch();
	
	UCameraComponent* GetCameraComponent() const { return CameraComponent; }

private:
	// 자동화 테스트 시퀀스
	void RunAutomatedTests(float DeltaTime);
	void LogTestResult(const char* TestName, bool bPassed);
};
