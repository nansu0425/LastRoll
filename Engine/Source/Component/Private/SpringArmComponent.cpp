#include "pch.h"
#include "Component/Public/SpringArmComponent.h"
#include "Utility/Public/JsonSerializer.h"
#include <json.hpp>
IMPLEMENT_CLASS(USpringArmComponent, UActorComponent)


USpringArmComponent::USpringArmComponent()
{
}

void USpringArmComponent::BeginPlay()
{

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
		FJsonSerializer::ReadFloat(InOutHandle, "TargtArmLength", TargtArmLength, 300);
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
		InOutHandle["TargtArmLength"] = TargtArmLength;
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
