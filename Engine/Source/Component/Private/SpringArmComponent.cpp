#include "pch.h"
#include "Component/Public/SpringArmComponent.h"
#include "Utility/Public/JsonSerializer.h"
#include "Render/UI/Widget/Public/SpringArmComponentWidget.h"
#include <json.hpp>
IMPLEMENT_CLASS(USpringArmComponent, USceneComponent)

USpringArmComponent::USpringArmComponent()
{
}

void USpringArmComponent::BeginPlay()
{

}

FVector USpringArmComponent::GetSpringArmOffset() const
{
	FVector CurTargetOffset = TargetOffset;
	FQuaternion CurQuat = GetWorldRotationAsQuaternion();
	FMatrix CurRotationMatrix = CurQuat.ToRotationMatrix();
	FVector Forward = CurQuat.GetForward();
	FVector TargetArmOffset = -Forward * TargetArmLength;
	FVector CurSocketOffset = (FMatrix::TranslationMatrix(SocketOffset) * CurRotationMatrix).GetLocation();
	return CurTargetOffset + CurSocketOffset + TargetArmOffset;
}





const FMatrix& USpringArmComponent::GetWorldTransformMatrix() const
{ 
	if (bIsTransformDirty)
	{
		WorldTransformMatrix = FMatrix::GetModelMatrix(RelativeLocation, RelativeRotation, RelativeScale3D);

		if (AttachParent)
		{
			WorldTransformMatrix *= AttachParent->GetWorldTransformMatrix();
		}

		WorldTransformMatrix *= FMatrix::CreateTranslation(GetSpringArmOffset());
		bIsTransformDirty = false;
	}

	return WorldTransformMatrix;
}
const FMatrix& USpringArmComponent::GetWorldTransformMatrixInverse() const
{ 
	if (bIsTransformDirtyInverse)
	{
		WorldTransformMatrixInverse = GetWorldTransformMatrix().Inverse();
	}


	return WorldTransformMatrixInverse;
}

FVector USpringArmComponent::GetWorldLocation() const
{ 
	return GetWorldTransformMatrix().GetLocation();
}
FVector USpringArmComponent::GetWorldRotation() const
{
	return GetWorldRotationAsQuaternion().ToEuler();
}
FQuaternion USpringArmComponent::GetWorldRotationAsQuaternion() const
{
	if (AttachParent)
	{
		return RelativeRotation * AttachParent->GetWorldRotationAsQuaternion();
	}
	return RelativeRotation;
}

void USpringArmComponent::SetWorldLocation(const FVector& NewLocation)
{
	if (AttachParent)
	{
		const FMatrix ParentWorldMatrixInverse = AttachParent->GetWorldTransformMatrixInverse();
		SetRelativeLocation(ParentWorldMatrixInverse.TransformPosition(NewLocation));
	}
	else
	{
		SetRelativeLocation(NewLocation);
	}
}
void USpringArmComponent::SetWorldRotation(const FQuaternion& NewRotation)
{
	if (AttachParent)
	{
		FQuaternion ParentWorldRotationQuat = AttachParent->GetWorldRotationAsQuaternion();
		SetRelativeRotation(NewRotation * ParentWorldRotationQuat.Inverse());
	}
	else
	{
		SetRelativeRotation(NewRotation);
	}
}
void USpringArmComponent::TickComponent(float DeltaTime)
{
	Super::TickComponent(DeltaTime);
}
void USpringArmComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	// 불러오기
	if (bInIsLoading)
	{
		FJsonSerializer::ReadFloat(InOutHandle, "TargetArmLength", TargetArmLength, 3);
		FJsonSerializer::ReadVector(InOutHandle, "SocketOffset", SocketOffset);
		FJsonSerializer::ReadVector(InOutHandle, "TargetOffset", TargetOffset);
		FJsonSerializer::ReadBool(InOutHandle, "bUsePawnControlRotation", bUsePawnControlRotation);
		FJsonSerializer::ReadBool(InOutHandle, "bLocationLag", bLocationLag, true);
		FJsonSerializer::ReadFloat(InOutHandle, "LocationLagSpeed", LocationLagSpeed, 5);
		FJsonSerializer::ReadBool(InOutHandle, "bRotationLag", bRotationLag, true);
		FJsonSerializer::ReadFloat(InOutHandle, "RotationLagSpeed", RotationLagSpeed, 5);
		FJsonSerializer::ReadBool(InOutHandle, "bInHeritPitch", bInHeritPitch, true);
		FJsonSerializer::ReadBool(InOutHandle, "bInHeritYaw", bInHeritYaw, true);
		FJsonSerializer::ReadBool(InOutHandle, "bInHeritRoll", bInHeritRoll, true);
	}
	// 저장
	else
	{
		InOutHandle["TargetArmLength"] = TargetArmLength;
		InOutHandle["SocketOffset"] = FJsonSerializer::VectorToJson(SocketOffset);
		InOutHandle["TargetOffset"] = FJsonSerializer::VectorToJson(TargetOffset);
		InOutHandle["bUsePawnControlRotation"] = bUsePawnControlRotation;
		InOutHandle["bLocationLag"] = bLocationLag;
		InOutHandle["LocationLagSpeed"] = LocationLagSpeed;
		InOutHandle["bRotationLag"] = bRotationLag;
		InOutHandle["RotationLagSpeed"] = RotationLagSpeed;
		InOutHandle["bInHeritPitch"] = bInHeritPitch;
		InOutHandle["bInHeritYaw"] = bInHeritYaw;
		InOutHandle["bInHeritRoll"] = bInHeritRoll;

	}
}

UClass* USpringArmComponent::GetSpecificWidgetClass() const
{
	return USpringArmComponentWidget::StaticClass();
}
