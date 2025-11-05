#pragma once

class UScriptComponent;
class UStaticMeshComponent;
class USphereComponent;

UCLASS()
class AEnemy : public AActor
{
    GENERATED_BODY()
    DECLARE_CLASS(AEnemy, AActor)
public:
    AEnemy();

    virtual ~AEnemy() = default;

    virtual UClass* GetDefaultRootComponent() override;

    virtual void InitializeComponents() override;

protected:
    UAudioComponent* HitAudioComponent;
    UAudioComponent* DeathAudioComponent;
    UScriptComponent* EnemyScriptComponent = nullptr;
    USphereComponent* SphereCollider = nullptr;
};
