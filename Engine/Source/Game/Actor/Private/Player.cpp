#include "pch.h"

#include "Game/Actor/Public/Player.h"

#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Component/Public/ScriptComponent.h"
#include "Component/Shape/Public/SphereComponent.h"

IMPLEMENT_CLASS(APlayer, AActor)

APlayer::APlayer()
{
    // ScriptComponent 생성
    PlayerScriptComponent = CreateDefaultSubobject<UScriptComponent>();

    // DetectionCollider 생성 (Enemy 탐지용)
    DetectionCollider = CreateDefaultSubobject<USphereComponent>();

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

    // DetectionCollider 설정
    DetectionCollider->SetSphereRadius(20.0f);  // Enemy 탐지 범위 20 유닛
    DetectionCollider->SetGenerateOverlapEvents(true);  // Overlap 이벤트 활성화
    DetectionCollider->SetBlockComponent(false);  // Block 비활성화 (Overlap만 사용)

    PlayerScriptComponent->SetScriptPath("Player.lua");
}
