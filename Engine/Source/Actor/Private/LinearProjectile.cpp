#include "pch.h"
#include "Actor/Public/LinearProjectile.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Component/Shape/Public/SphereComponent.h"
#include "Component/Public/ScriptComponent.h"

IMPLEMENT_CLASS(ALinearProjectile, AActor)

ALinearProjectile::ALinearProjectile()
{
	// 추가 컴포넌트들을 생성자에서 생성
	// RootComponent는 InitializeComponents()에서 자동 생성됨

	// 1. SphereComponent 생성 (충돌 감지용)
	SphereCollider = CreateDefaultSubobject<USphereComponent>();

	// 2. ScriptComponent 생성 (Lua 스크립트용)
	ScriptComponent = CreateDefaultSubobject<UScriptComponent>();
}

UClass* ALinearProjectile::GetDefaultRootComponent()
{
	// RootComponent로 UStaticMeshComponent 사용 (렌더링용)
	return UStaticMeshComponent::StaticClass();
}

void ALinearProjectile::InitializeComponents()
{
	Super::InitializeComponents();

	// RootComponent는 이미 UStaticMeshComponent로 생성되어 있음
	UStaticMeshComponent* MeshComponent = Cast<UStaticMeshComponent>(GetRootComponent());

	// 1. 구형 메시 설정
	MeshComponent->SetStaticMesh("Data/Shapes/Sphere.obj");

	// 2. SphereCollider를 RootComponent에 부착
	SphereCollider->AttachToComponent(MeshComponent);

	// 3. SphereCollider 설정
	SphereCollider->SetSphereRadius(1.0f);  // 투사체 충돌 반지름
	SphereCollider->SetGenerateOverlapEvents(true);  // Overlap 이벤트 활성화
	SphereCollider->SetBlockComponent(false);  // Block 비활성화 (Overlap만 사용)

	// 4 RootComponent의 스케일 설정
	MeshComponent->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.5f));

	// 4. ScriptComponent 스크립트 경로 설정
	ScriptComponent->SetScriptPath("LinearProjectile.lua");
}
