#include "pch.h"
#include "Actor/Public/HomingProjectile.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Component/Shape/Public/SphereComponent.h"
#include "Component/Public/ScriptComponent.h"

IMPLEMENT_CLASS(AHomingProjectile, AActor)

AHomingProjectile::AHomingProjectile()
{
	// 추가 컴포넌트들을 생성자에서 생성
	// RootComponent는 InitializeComponents()에서 자동 생성됨

	// 1. SphereComponent 생성 (충돌 감지용)
	SphereCollider = CreateDefaultSubobject<USphereComponent>();

	// 2. ScriptComponent 생성 (Lua 스크립트용)
	ScriptComponent = CreateDefaultSubobject<UScriptComponent>();
}

UClass* AHomingProjectile::GetDefaultRootComponent()
{
	// RootComponent로 UStaticMeshComponent 사용 (렌더링용)
	return UStaticMeshComponent::StaticClass();
}

void AHomingProjectile::InitializeComponents()
{
	Super::InitializeComponents();

	// RootComponent는 이미 UStaticMeshComponent로 생성되어 있음
	UStaticMeshComponent* MeshComponent = Cast<UStaticMeshComponent>(GetRootComponent());

	// 1. 구형 메시 설정
	MeshComponent->SetStaticMesh("Data/snow_and_rocks_material/snow_and_rocks_material.obj");

	// 2. SphereCollider를 RootComponent에 부착
	SphereCollider->AttachToComponent(MeshComponent);

	// 3. SphereCollider 설정
	SphereCollider->SetRelativeLocation(FVector(0.0f, 0.0f, 0.5f));
	SphereCollider->SetSphereRadius(0.6f);  // 투사체 충돌 반지름 (작게 조정)
	SphereCollider->SetGenerateOverlapEvents(true);  // Overlap 이벤트 활성화a
	SphereCollider->SetBlockComponent(false);  // Block 비활성화 (Overlap만 사용)

	// 4 RootComponent의 스케일 설정
	MeshComponent->SetRelativeScale3D(FVector(4.0f, 4.0f, 4.0f));

	// 5. ScriptComponent 스크립트 경로 설정
	ScriptComponent->SetScriptPath("HomingProjectile.lua");
}
