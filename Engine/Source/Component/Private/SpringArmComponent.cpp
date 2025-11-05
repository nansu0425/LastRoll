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
	PrevWorldLocation = GetWorldLocation();
}

void USpringArmComponent::TickComponent(float DeltaTime)
{
	Super::TickComponent(DeltaTime);
	bIsTransformDirty = true;
	bIsTransformDirtyInverse = true;
	FVector CurWorldLocation = GetWorldLocation();
	FVector CurLerpMoveDis = FVector::LinearEXPLerpVt3(LagLocation, CurWorldLocation, LocationLagLinearLerpInterpolation, LocationLagSpeed) - LagLocation;
	LagLocation += CurLerpMoveDis;
	UE_LOG("Lerp : %f,%f,%f , Lag : %f, %f, %f " , CurLerpMoveDis.X, CurLerpMoveDis.Y, CurLerpMoveDis.Z,
		LagLocation.X,LagLocation.Y,LagLocation.Z);
	for (USceneComponent* Child : AttachChildren)
	{
		//부모가 이동하면 따라오는 자동 이동으로 인해 현재위치에서 빼야함
		Child->bIsTransformDirty = true;
		Child->bIsTransformDirtyInverse = true;

		Child->SetWorldLocation(LagLocation + GetSpringArmOffset());

	}
	PrevWorldLocation = CurWorldLocation;
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

		WorldTransformMatrix *= FMatrix::TranslationMatrix(GetSpringArmOffset());
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
	return GetWorldTransformMatrix().GetLocation() - GetSpringArmOffset();
}


//크기 적용 필요
FVector USpringArmComponent::GetSpringArmOffset() const
{
	FVector CurTargetOffset = TargetOffset;
	FQuaternion CurQuat = GetWorldRotationAsQuaternion();
	FMatrix CurRotationMatrix = CurQuat.ToRotationMatrix();
	//FMatrix CurScaleMatrix = FMatrix::ScaleMatrix(GetWorldScale3D()); 오류나는데 시간없어서 넘김
	FVector Forward = CurQuat.GetForward();
	FVector TargetArmOffset = -Forward * TargetArmLength;
	//FVector CurSocketOffset = (FMatrix::TranslationMatrix(SocketOffset) * CurScaleMatrix * CurRotationMatrix).GetLocation();
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

	return SpringArmComp;
}

void USpringArmComponent::DuplicateSubObjects(UObject* DuplicatedObject)
{
	Super::DuplicateSubObjects(DuplicatedObject);
}