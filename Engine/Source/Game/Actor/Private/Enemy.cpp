#include "pch.h"

#include "Game/Actor/Public/Enemy.h"

#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Component/Public/AudioComponent.h"
#include "Component/Public/ScriptComponent.h"
#include "Component/Shape/Public/SphereComponent.h"

IMPLEMENT_CLASS(AEnemy, AActor)

AEnemy::AEnemy()
{
    HitAudioComponent = CreateDefaultSubobject<UAudioComponent>();

    DeathAudioComponent = CreateDefaultSubobject<UAudioComponent>();
    
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

    HitAudioComponent->LoadSound("knight.wav");
    DeathAudioComponent->LoadSound("knight_death.wav");

    auto StaticMeshComponent = Cast<UStaticMeshComponent>(GetRootComponent());

    StaticMeshComponent->SetStaticMesh("Data/Knight.obj");

    // SphereCollider를 RootComponent에 부착
    SphereCollider->AttachToComponent(StaticMeshComponent);

    // SphereCollider 설정
    SphereCollider->SetSphereRadius(1.5f);  // Enemy 충돌 반지름 (메시 크기에 맞게 조정)
    SphereCollider->SetGenerateOverlapEvents(true);  // Overlap 이벤트 활성화
	SphereCollider->SetBlockComponent(true); // Block 활성화

    EnemyScriptComponent->SetScriptPath("EnemyA.lua");
}
