#pragma once
#include "Actor/Public/Actor.h"

class USphereComponent;
class UScriptComponent;
class UStaticMeshComponent;
class UPointLightComponent;

UCLASS()
class AOrbitProjectile : public AActor
{
	GENERATED_BODY()
	DECLARE_CLASS(AOrbitProjectile, AActor)

public:
	AOrbitProjectile();
	virtual ~AOrbitProjectile() override = default;

	virtual UClass* GetDefaultRootComponent() override;
	virtual void InitializeComponents() override;

private:
	USphereComponent* SphereCollider = nullptr;
	UScriptComponent* ScriptComponent = nullptr;
	UPointLightComponent* PointLight = nullptr;
};
