#include "pch.h"

#include "Game/Actor/Public/EightBallEnemy.h"

#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Component/Public/AudioComponent.h"
#include "Component/Public/ScriptComponent.h"
#include "Component/Shape/Public/SphereComponent.h"

IMPLEMENT_CLASS(AEightBallEnemy, AEnemy)

void AEightBallEnemy::InitializeComponents()
{
    AEnemy::InitializeComponents();
    
    HitAudioComponent->LoadSound("knight.wav");
    DeathAudioComponent->LoadSound("knight_death.wav");

    auto StaticMeshComponent = Cast<UStaticMeshComponent>(GetRootComponent());

    StaticMeshComponent->SetStaticMesh("Data/EightBall.obj");

    EnemyScriptComponent->SetScriptPath("EnemyB.lua");

    SphereCollider->SetSphereRadius(3.0f);
}
