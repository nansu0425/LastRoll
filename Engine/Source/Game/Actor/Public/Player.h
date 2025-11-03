#pragma once

class UScriptComponent;
class UStaticMeshComponent;
class USphereComponent;

UCLASS()
class APlayer : public AActor
{
    GENERATED_BODY()
    DECLARE_CLASS(APlayer, AActor)

public:
    APlayer();

    virtual ~APlayer() = default;

    virtual UClass* GetDefaultRootComponent() override;

    virtual void InitializeComponents() override;

private:
    UScriptComponent* PlayerScriptComponent = nullptr;
    USphereComponent* DetectionCollider = nullptr;  // Enemy 탐지용
    USphereComponent* PhysicsCollider = nullptr;
};
