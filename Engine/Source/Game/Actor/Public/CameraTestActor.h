#pragma once
#include "Actor/Public/Actor.h"

/**
 * @brief Camera system test actor for PIE mode
 * 
 * This actor is used to test PlayerCameraManager features:
 * - UCameraComponent integration
 * - Camera modifiers (CameraShake)
 * - ViewTarget switching
 * - Modifier priority system
 */
UCLASS()
class ACameraTestActor : public AActor
{
	GENERATED_BODY()
	DECLARE_CLASS(ACameraTestActor, AActor)

private:
	class UCameraComponent* CameraComponent;
	
	// Test state
	float TestTimer;
	int32 CurrentTestPhase;

public:
	ACameraTestActor();
	virtual ~ACameraTestActor() override;

	virtual UClass* GetDefaultRootComponent() override;
	virtual void InitializeComponents() override;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// Manual test triggers (called via console or input)
	void TestCameraShake_Explosion();
	void TestCameraShake_Hit();
	void TestCameraShake_Earthquake();
	void TestViewTargetSwitch();
	
	UCameraComponent* GetCameraComponent() const { return CameraComponent; }

private:
	// Automated test sequence
	void RunAutomatedTests(float DeltaTime);
	void LogTestResult(const char* TestName, bool bPassed);
};
