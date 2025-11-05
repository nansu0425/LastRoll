#include "pch.h"

#include "Game/Actor/Public/Player.h"

#include "Actor/Public/PlayerCameraManager.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Component/Public/ScriptComponent.h"
#include "Component/Shape/Public/SphereComponent.h"
#include "Core/Public/NewObject.h"
#include "Level/Public/World.h"
#include "Editor/Public/EditorEngine.h"

IMPLEMENT_CLASS(APlayer, AActor)

APlayer::APlayer()
{
    // ScriptComponent 생성
    PlayerScriptComponent = CreateDefaultSubobject<UScriptComponent>();

    // DetectionCollider 생성 (Enemy 탐지용)
    DetectionCollider = CreateDefaultSubobject<USphereComponent>();

    PhysicsCollider = CreateDefaultSubobject<USphereComponent>();

	bCanEverTick = true; // Tick 활성화
}

UClass* APlayer::GetDefaultRootComponent()
{
    return UStaticMeshComponent::StaticClass();
}

void APlayer::InitializeComponents()
{
    AActor::InitializeComponents();

    auto StaticMeshComponent = Cast<UStaticMeshComponent>(GetRootComponent());

    StaticMeshComponent->SetStaticMesh("Data/Dice.obj");  // 임시로 Dice 사용, 나중에 Player 메시로 변경

    // DetectionCollider를 RootComponent에 부착
    DetectionCollider->AttachToComponent(StaticMeshComponent);
    PhysicsCollider->AttachToComponent(StaticMeshComponent);

    // DetectionCollider 설정
    DetectionCollider->SetSphereRadius(30.0f);  // Enemy 탐지 범위 30 유닛 (Homing Projectile 감지 범위)
    DetectionCollider->SetGenerateOverlapEvents(true);  // Overlap 이벤트 활성화
    DetectionCollider->SetBlockComponent(false);  // Block 비활성화 (Overlap만 사용)

    PhysicsCollider->SetSphereRadius(1.15f);
    PhysicsCollider->SetGenerateOverlapEvents(false);
    PhysicsCollider->SetBlockComponent(true);

    PlayerScriptComponent->SetScriptPath("Player.lua");
}

void APlayer::BeginPlay()
{
    AActor::BeginPlay();

    // Create PlayerCameraManager when player spawns (similar to Unreal Engine's PlayerController)
    // Use SpawnActor to properly register with Level for automatic cleanup
    if (!PlayerCameraManager && GWorld)
    {
        AActor* SpawnedActor = GWorld->SpawnActor(APlayerCameraManager::StaticClass());
        PlayerCameraManager = Cast<APlayerCameraManager>(SpawnedActor);

        if (PlayerCameraManager)
        {
            PlayerCameraManager->SetName("PlayerCameraManager");
            UE_LOG("APlayer: Created PlayerCameraManager");
        }
    }
}

void APlayer::EndPlay()
{
    // Destroy PlayerCameraManager if it exists
    // It's an Actor spawned via SpawnActor, so use DestroyActor for proper cleanup
    if (PlayerCameraManager && GWorld)
    {
        GWorld->DestroyActor(PlayerCameraManager);
        PlayerCameraManager = nullptr;
    }

    AActor::EndPlay();
}
