#include "pch.h"
#include "Actor/Public/OrbitProjectile.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Component/Mesh/Public/StaticMesh.h"
#include "Component/Shape/Public/SphereComponent.h"
#include "Component/Public/ScriptComponent.h"
#include "Texture/Public/Material.h"

IMPLEMENT_CLASS(AOrbitProjectile, AActor)

AOrbitProjectile::AOrbitProjectile()
{
	// 추가 컴포넌트들을 생성자에서 생성
	// RootComponent는 InitializeComponents()에서 자동 생성됨

	// 1. SphereComponent 생성 (충돌 감지용)
	SphereCollider = CreateDefaultSubobject<USphereComponent>();

	// 2. ScriptComponent 생성 (Lua 스크립트용)
	ScriptComponent = CreateDefaultSubobject<UScriptComponent>();
}

UClass* AOrbitProjectile::GetDefaultRootComponent()
{
	// RootComponent로 UStaticMeshComponent 사용 (렌더링용)
	return UStaticMeshComponent::StaticClass();
}

void AOrbitProjectile::InitializeComponents()
{
	Super::InitializeComponents();

	// RootComponent는 이미 UStaticMeshComponent로 생성되어 있음
	UStaticMeshComponent* MeshComponent = Cast<UStaticMeshComponent>(GetRootComponent());

	// 1. 메시 설정
	MeshComponent->SetStaticMesh("Data/sun/sun.obj");

	// 2. SphereCollider를 RootComponent에 부착
	SphereCollider->AttachToComponent(MeshComponent);

	// 3. SphereCollider 설정
	SphereCollider->SetSphereRadius(10.5f);  // 투사체 충돌 반지름
	SphereCollider->SetGenerateOverlapEvents(true);  // Overlap 이벤트 활성화
	SphereCollider->SetBlockComponent(false);  // Block 비활성화 (Overlap만 사용)

	// 4. RootComponent의 스케일 설정
	MeshComponent->SetRelativeScale3D(FVector(0.1f, 0.1f, 0.1f));

	// 5. Emissive Material 설정 (빛나는 효과)
	// 모든 Material 슬롯에 Emissive 적용 (메시가 여러 섹션을 가질 수 있음)
	UStaticMesh* StaticMesh = MeshComponent->GetStaticMesh();
	if (StaticMesh)
	{
		FVector EmissiveColor = FVector(1.0f, 0.8f, 0.3f);  // 밝은 주황빛

		// StaticMesh의 모든 Material 슬롯에 대해 처리
		int32 NumMaterials = StaticMesh->GetNumMaterials();
		for (int32 i = 0; i < NumMaterials; ++i)
		{
			UMaterial* OriginalMaterial = MeshComponent->GetMaterial(i);
			if (OriginalMaterial)
			{
				// Material 복제 (Duplicate)
				UMaterial* OverrideMaterial = Cast<UMaterial>(OriginalMaterial->Duplicate());
				if (OverrideMaterial)
				{
					// Emissive 색상 설정
					OverrideMaterial->SetEmissiveColor(EmissiveColor);

					// Override Material로 설정
					MeshComponent->SetMaterial(i, OverrideMaterial);
				}
			}
		}
	}

	// 6. ScriptComponent 스크립트 경로 설정
	ScriptComponent->SetScriptPath("OrbitProjectile.lua");
}
