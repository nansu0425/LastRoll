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

	const virtual FMatrix& GetWorldTransformMatrix() const;
	const virtual FMatrix& GetWorldTransformMatrixInverse() const;
	FVector GetWorldLocation() const override;

	FVector GetSpringArmOffset() const;

	virtual UClass* GetSpecificWidgetClass() const override;

	void SetTargetArmLength(float InTargetArmLength) { MarkAsDirty(); TargetArmLength = InTargetArmLength; }
	float GetTargetArmLength() const { return TargetArmLength; }
	void SetSocketOffset(FVector InSocketOffset) { MarkAsDirty(); SocketOffset = InSocketOffset; }
	FVector GetSocketOffset() const { return SocketOffset; }
	void SetTargetOffset(FVector InTargetOffset) { MarkAsDirty(); TargetOffset = InTargetOffset; }
	FVector GetTargetOffset() const { return TargetOffset; }
	void SetbUsePawnControlRotation(bool InbUsePawnControlRotation) { MarkAsDirty(); bUsePawnControlRotation = InbUsePawnControlRotation; }
	bool GetbUsePawnControlRotation() const { return bUsePawnControlRotation; }
	void SetbLocationLag(bool InbLocationLag) { MarkAsDirty(); bLocationLag = InbLocationLag; }
	bool GetbLocationLag() const { return bLocationLag; }
	void SetLocationLagSpeed(float InLocationLagSpeed) { MarkAsDirty(); LocationLagSpeed = InLocationLagSpeed; }
	float GetLocationLagSpeed() const { return LocationLagSpeed; }
	void SetLocationLagLinearLerpInterpolation(float InLocationLagLinearLerpInterpolation) { MarkAsDirty(); LocationLagLinearLerpInterpolation = InLocationLagLinearLerpInterpolation; }
	float GetLocationLagLinearLerpInterpolation() const { return LocationLagLinearLerpInterpolation; }
	void SetbRotationLag(bool InbRotationLag) { MarkAsDirty(); bRotationLag = InbRotationLag; }
	bool GetbRotationLag() const { return bRotationLag; }
	void SetRotationLagLinearLerpInterpolation(float InRotationLagLinearLerpInterpolation) { MarkAsDirty(); RotationLagLinearLerpInterpolation = InRotationLagLinearLerpInterpolation; }
	float GetRotationLagLinearLerpInterpolation() const { return RotationLagLinearLerpInterpolation; }
	void SetRotationLagSpeed(float InRotationLagSpeed) { MarkAsDirty(); RotationLagSpeed = InRotationLagSpeed; }
	float GetRotationLagSpeed() const { return RotationLagSpeed; }
	void SetbInHeritPitch(bool InbInHeritPitch) { MarkAsDirty(); bInHeritPitch = InbInHeritPitch; }
	bool GetbInHeritPitch() const { return bInHeritPitch; }
	void SetbInHeritYaw(bool InbInHeritYaw) { MarkAsDirty(); bInHeritYaw = InbInHeritYaw; }
	bool GetbInHeritYaw() const { return bInHeritYaw; }
	void SetbInHeritRoll(bool InbInHeritRoll) { MarkAsDirty(); bInHeritRoll = InbInHeritRoll; }
	bool GetbInHeritRoll() const { return bInHeritRoll; }
private:
	float TargetArmLength = 3.0f; // -Forward 방향으로 멀어짐
	FVector SocketOffset; // 스프링암의 끝점 (부모회전축 기준)
	FVector TargetOffset; // 스프링암의 시작점 (월드기준)

	bool bUsePawnControlRotation = false;

	bool bLocationLag = true;
	float LocationLagLinearLerpInterpolation = 0.0f;
	float LocationLagSpeed = 5.0f;

	bool bRotationLag = true;
	float RotationLagLinearLerpInterpolation = 0.0f;
	float RotationLagSpeed = 5.0f;

	bool bInHeritPitch = true;
	bool bInHeritYaw = true;
	bool bInHeritRoll = true;

	// Lag 상태 저장용 변수
	FVector PrevWorldLocation;
	FVector LagLocation;
public:
	virtual UObject* Duplicate() override;

protected:
	virtual void DuplicateSubObjects(UObject* DuplicatedObject) override;

};