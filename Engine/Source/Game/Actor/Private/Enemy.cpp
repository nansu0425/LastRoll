#include "pch.h"

#include "Game/Actor/Public/Enemy.h"

#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Component/Public/ScriptComponent.h"
#include "Component/Shape/Public/SphereComponent.h"

IMPLEMENT_CLASS(AEnemy, AActor)

AEnemy::AEnemy()
{
    // ScriptComponent 생성
    EnemyScriptComponent = CreateDefaultSubobject<UScriptComponent>();

    // SphereComponent 생성 (충돌 감지용)
    SphereCollider = CreateDefaultSubobject<USphereComponent>();

	bCanEverTick = true; // Tick 활성화
}

UClass* AEnemy::GetDefaultRootComponent()
{
    return UStaticMeshComponent::StaticClass();
}

void AEnemy::InitializeComponents()
{
    AActor::InitializeComponents();

    auto StaticMeshComponent = Cast<UStaticMeshComponent>(GetRootComponent());

    StaticMeshComponent->SetStaticMesh("Data/Knight.obj");

    // SphereCollider를 RootComponent에 부착
    SphereCollider->AttachToComponent(StaticMeshComponent);

    // SphereCollider 설정
    SphereCollider->SetSphereRadius(0.4f);  // Enemy 충돌 반지름
    SphereCollider->SetGenerateOverlapEvents(true);  // Overlap 이벤트 활성화
    SphereCollider->SetBlockComponent(false);  // Block 비활성화 (Overlap만 사용)

    EnemyScriptComponent->SetScriptPath("EnemyA.lua");

    SphereCollider->AttachToComponent(GetRootComponent());
    SphereCollider->SetSphereRadius(1.5f);
    SphereCollider->SetBlockComponent(true);
}
