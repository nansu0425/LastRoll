#pragma once
#include "Component/Public/SceneComponent.h"

UCLASS()
class USpringArmComponent : public UActorComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(USpringArmComponent, UActorComponent)
public:
	USpringArmComponent();

	void BeginPlay() override;
	void TickComponent(float DeltaTime) override;
	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

private:
	float TargtArmLength = 300;
	FVector SocketOffset; //스프링암의 끝점 (부모회전축 기준)
	FVector TargetOffset; //스프링암의 시작점 (월드기준)
	bool bUsePawnControlRotation = false;
	bool bLocationLag = true;
	float LocationLagSpeed = 5;
	bool bRotationLag = true;
	float RotationLagSpeed = 5;
	bool bInHeritPitch = true;
	bool bInHeritYaw = true;
	bool bInHeritRoll = true;



};