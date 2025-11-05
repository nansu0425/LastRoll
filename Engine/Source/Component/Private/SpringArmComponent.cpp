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

void USpringArmComponent::TickComponent(float DeltaTime)
{
	Super::TickComponent(DeltaTime);
	MarkAsDirty();
	FVector CurWorldLocation = GetWorldLocation();

	if (bLocationLag)
	{
		//이동지연보간
		FVector CurLerpMoveDis = FVector::LinearEXPLerpVt3(LagLocation, CurWorldLocation, LocationLagLinearLerpInterpolation, LocationLagSpeed) - LagLocation;
		LagLocation += CurLerpMoveDis;
		// UE_LOG("Lerp : %f,%f,%f , Lag : %f, %f, %f ", CurLerpMoveDis.X, CurLerpMoveDis.Y, CurLerpMoveDis.Z,
		// 	LagLocation.X, LagLocation.Y, LagLocation.Z);
		FVector FinalLagLocation = LagLocation;
	}
	else 
	{
		LagLocation = CurWorldLocation;
	}

	FQuaternion DesiredWorldRotation = GetWorldRotationAsQuaternion();
	if (bRotationLag) 
	{
		//회전지연보간
		LagRotation = FQuaternion::Slerp(LagRotation, DesiredWorldRotation, RotationLagSpeed);
	}
	else
	{
		LagRotation = DesiredWorldRotation;
	}

	for (USceneComponent* Child : AttachChildren)
	{
		Child->SetWorldLocation(LagLocation + GetSpringArmOffset(true));
		Child->SetWorldRotation(LagRotation);
	}
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

		WorldTransformMatrix *= FMatrix::TranslationMatrix(GetSpringArmOffset(false));
		bIsTransformDirty = false;
	}

	return WorldTransformMatrix;
}

const FMatrix& USpringArmComponent::GetWorldTransformMatrixInverse() const
{
	if (bIsTransformDirtyInverse)
	{
		if (AttachParent)
		{
			WorldTransformMatrixInverse *= AttachParent->GetWorldTransformMatrixInverse();
		}

		WorldTransformMatrixInverse = WorldTransformMatrix.Inverse();

		bIsTransformDirtyInverse = false;
	}

	return WorldTransformMatrixInverse;
}
FVector USpringArmComponent::GetWorldLocation() const
{
	return GetWorldTransformMatrix().GetLocation() - GetSpringArmOffset(false);
}


FVector USpringArmComponent::GetSpringArmOffset(bool bUseLagRot) const
{
	FVector CurTargetOffset = TargetOffset;
	FQuaternion CurQuat = GetWorldRotationAsQuaternion();
	FMatrix CurRotationMatrix = bUseLagRot ? LagRotation.ToRotationMatrix() : CurQuat.ToRotationMatrix();
	FVector Forward = CurQuat.GetForward();
	FVector TargetArmOffset = -Forward * TargetArmLength;
	FVector CurSocketOffset = (FMatrix::TranslationMatrix(SocketOffset) * CurRotationMatrix).GetLocation();
	return CurTargetOffset + CurSocketOffset + TargetArmOffset;
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
UObject* USpringArmComponent::Duplicate()
{
	USpringArmComponent* SpringArmComp = Cast<USpringArmComponent>(Super::Duplicate());

	SpringArmComp->TargetArmLength = TargetArmLength;
	SpringArmComp->SocketOffset = SocketOffset;
	SpringArmComp->TargetOffset = TargetOffset;
	SpringArmComp->bUsePawnControlRotation = bUsePawnControlRotation;
	SpringArmComp->bLocationLag = bLocationLag;
	SpringArmComp->LocationLagLinearLerpInterpolation = LocationLagLinearLerpInterpolation;
	SpringArmComp->LocationLagSpeed = LocationLagSpeed;
	SpringArmComp->bRotationLag = bRotationLag;
	SpringArmComp->RotationLagLinearLerpInterpolation = RotationLagLinearLerpInterpolation;
	SpringArmComp->RotationLagSpeed = RotationLagSpeed;
	SpringArmComp->bInHeritPitch = bInHeritPitch;
	SpringArmComp->bInHeritYaw = bInHeritYaw;
	SpringArmComp->bInHeritRoll = bInHeritRoll;
	SpringArmComp->LagLocation = GetWorldLocation();
	SpringArmComp->LagRotation = GetWorldRotationAsQuaternion();

	return SpringArmComp;
}

void USpringArmComponent::DuplicateSubObjects(UObject* DuplicatedObject)
{
	Super::DuplicateSubObjects(DuplicatedObject);
}