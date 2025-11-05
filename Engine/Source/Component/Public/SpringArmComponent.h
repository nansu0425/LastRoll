#pragma once
#include "Component/Public/SceneComponent.h"

UCLASS()
class USpringArmComponent : public USceneComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(USpringArmComponent, USceneComponent)
public:
	USpringArmComponent();

	void BeginPlay() override;
	void TickComponent(float DeltaTime) override;
	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;


	FVector GetSpringArmOffset() const;


	//World Transform 값 관련 함수를 오버라이딩 해서 SpringArm Option값들을 반영한다.
	const virtual FMatrix& GetWorldTransformMatrix() const;
	const virtual FMatrix& GetWorldTransformMatrixInverse() const;

	virtual FVector GetWorldLocation() const;
	virtual FVector GetWorldRotation() const;
	virtual FQuaternion GetWorldRotationAsQuaternion() const;

	virtual void SetWorldLocation(const FVector& NewLocation);
	virtual void SetWorldRotation(const FQuaternion& NewRotation);

	virtual UClass* GetSpecificWidgetClass() const override;

	void SetTargetArmLength(float InTargetArmLength) { 		MarkAsDirty();TargetArmLength = InTargetArmLength; }
	float GetTargetArmLength() const { return TargetArmLength; }
	void SetSocketOffset(FVector InSocketOffset) { 	MarkAsDirty(); SocketOffset = InSocketOffset; }
	FVector GetSocketOffset() const { return SocketOffset; }
	void SetTargetOffset(FVector InTargetOffset) { 	MarkAsDirty(); TargetOffset = InTargetOffset; }
	FVector GetTargetOffset() const { return TargetOffset; }
	void SetbUsePawnControlRotation(bool InbUsePawnControlRotation) { 	MarkAsDirty(); bUsePawnControlRotation = InbUsePawnControlRotation; }
	bool GetbUsePawnControlRotation() const { return bUsePawnControlRotation; }
	void SetbLocationLag(bool InbLocationLag) { 	MarkAsDirty(); bLocationLag = InbLocationLag; }
	bool GetbLocationLag() const { return bLocationLag; }
	void SetLocationLagSpeed(float InLocationLagSpeed) { 	MarkAsDirty(); LocationLagSpeed = InLocationLagSpeed; }
	float GetLocationLagSpeed() const { return LocationLagSpeed; }
	void SetbRotationLag(bool InbRotationLag) { 	MarkAsDirty(); bRotationLag = InbRotationLag; }
	bool GetbRotationLag() const { return bRotationLag; }
	void SetRotationLagSpeed(float InRotationLagSpeed) { 	MarkAsDirty(); RotationLagSpeed = InRotationLagSpeed; }
	float GetRotationLagSpeed() const { return RotationLagSpeed; }
	void SetbInHeritPitch(bool InbInHeritPitch) { 	MarkAsDirty(); bInHeritPitch = InbInHeritPitch; }
	bool GetbInHeritPitch() const { return bInHeritPitch; }
	void SetbInHeritYaw(bool InbInHeritYaw) { 	MarkAsDirty(); bInHeritYaw = InbInHeritYaw; }
	bool GetbInHeritYaw() const { return bInHeritYaw; }
	void SetbInHeritRoll(bool InbInHeritRoll) { 	MarkAsDirty(); bInHeritRoll = InbInHeritRoll; }
	bool GetbInHeritRoll() const { return bInHeritRoll; }
private:
	float TargetArmLength = 3; //-Forward 방향으로 멀어짐 (o)
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