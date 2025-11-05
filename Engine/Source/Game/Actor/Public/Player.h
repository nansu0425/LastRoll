#pragma once

class UScriptComponent;
class UStaticMeshComponent;
class USphereComponent;
class APlayerCameraManager;
class ATopDownCameraActor;

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

    virtual void BeginPlay() override;

    virtual void EndPlay() override;

    // Camera Manager Access
    APlayerCameraManager* GetPlayerCameraManager() const { return PlayerCameraManager; }

    // Camera Actor Access
    ATopDownCameraActor* GetCameraActor() const { return CameraActor; }

private:
    UScriptComponent* PlayerScriptComponent = nullptr;
    USphereComponent* DetectionCollider = nullptr;  // Enemy 탐지용
    USphereComponent* PhysicsCollider = nullptr;

    // Player owns the camera manager (similar to PlayerController in Unreal Engine)
    APlayerCameraManager* PlayerCameraManager = nullptr;

    // Top-down camera actor (독립적인 카메라 액터)
    ATopDownCameraActor* CameraActor = nullptr;
};
