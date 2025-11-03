#pragma once

#include "Actor/Public/Actor.h"

class UStaticMeshComponent;
class USphereComponent;
class UScriptComponent;

UCLASS()
class AHomingProjectile : public AActor
{
	GENERATED_BODY()
	DECLARE_CLASS(AHomingProjectile, AActor)

public:
	AHomingProjectile();

	virtual UClass* GetDefaultRootComponent() override;
	virtual void InitializeComponents() override;

private:
	// RootComponent는 자동 생성되므로 멤버 변수 불필요
	// UStaticMeshComponent* MeshComponent (자동 생성됨)

	USphereComponent* SphereCollider = nullptr;
	UScriptComponent* ScriptComponent = nullptr;
};
