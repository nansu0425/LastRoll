#include "pch.h"

#include "Game/Actor/Public/Enemy.h"

#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Component/Public/ScriptComponent.h"

IMPLEMENT_CLASS(AEnemy, AActor)

AEnemy::AEnemy()
{
    EnemyScriptComponent = CreateDefaultSubobject<UScriptComponent>();
}

UClass* AEnemy::GetDefaultRootComponent()
{
    return UStaticMeshComponent::StaticClass();
}

void AEnemy::InitializeComponents()
{
    AActor::InitializeComponents();
    
    auto StaticMeshComponent = Cast<UStaticMeshComponent>(GetRootComponent());
    
    StaticMeshComponent->SetStaticMesh("Data/Apple.obj");
    
    EnemyScriptComponent->SetScriptPath("Enemy.lua");
}
