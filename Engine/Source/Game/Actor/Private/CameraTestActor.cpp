#include "pch.h"
#include "Game/Actor/Public/CameraTestActor.h"
#include "Component/Camera/Public/CameraComponent.h"
#include "Component/Camera/Public/CameraModifier_CameraShake.h"
#include "Actor/Public/PlayerCameraManager.h"
#include "Level/Public/World.h"
#include "Editor/Public/EditorEngine.h"

IMPLEMENT_CLASS(ACameraTestActor, AActor)

ACameraTestActor::ACameraTestActor()
	: TestTimer(0.0f)
	, CurrentTestPhase(0)
{
	SetName("CameraTestActor");
	
	// Create camera component using CreateDefaultSubobject (APlayer/AEnemy pattern)
	CameraComponent = CreateDefaultSubobject<UCameraComponent>();
	
	bCanEverTick = true; // Enable Tick
}

ACameraTestActor::~ACameraTestActor()
{
}

UClass* ACameraTestActor::GetDefaultRootComponent()
{
	// Use SceneComponent as root (simple transform-only component)
	return USceneComponent::StaticClass();
}

void ACameraTestActor::InitializeComponents()
{
	AActor::InitializeComponents();
	
	// Get root component
	USceneComponent* RootComp = GetRootComponent();
	
	if (!RootComp)
	{
		UE_LOG_ERROR("ACameraTestActor::InitializeComponents - RootComponent is null!");
		return;
	}
	
	// Attach CameraComponent to root (following APlayer/AEnemy pattern)
	if (CameraComponent)
	{
		CameraComponent->AttachToComponent(RootComp);
		
		// Set camera parameters
		CameraComponent->SetFieldOfView(90.0f);
		CameraComponent->SetAspectRatio(16.0f / 9.0f);
		CameraComponent->SetNearClipPlane(1.0f);
		CameraComponent->SetFarClipPlane(10000.0f);
		
		UE_LOG_DEBUG("ACameraTestActor: CameraComponent initialized (FOV=90, Aspect=16:9)");
	}
	else
	{
		UE_LOG_ERROR("ACameraTestActor::InitializeComponents - CameraComponent is null!");
	}
}

void ACameraTestActor::BeginPlay()
{
	Super::BeginPlay();
	
	UE_LOG_SUCCESS("=== CameraTestActor: BeginPlay ===");
	UE_LOG("Position: (%.2f, %.2f, %.2f)", 
		GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z);
	
	// Verify CameraComponent
	if (CameraComponent)
	{
		UE_LOG_SUCCESS("? CameraComponent created successfully");
		UE_LOG("  FOV: %.2f, AspectRatio: %.2f", 
			CameraComponent->GetFieldOfView(), 
			CameraComponent->GetAspectRatio());
	}
	else
	{
		UE_LOG_ERROR("? CameraComponent is null!");
	}
	
	// Get World and CameraManager
	if (!GWorld)
	{
		UE_LOG_ERROR("? GWorld is null!");
		return;
	}
	
	APlayerCameraManager* CameraManager = GWorld->GetCameraManager();
	if (!CameraManager)
	{
		UE_LOG_WARNING("CameraManager not set in World - creating one");
		CameraManager = NewObject<APlayerCameraManager>(GWorld);
		GWorld->SetCameraManager(CameraManager);
	}
	
	if (CameraManager)
	{
		UE_LOG_SUCCESS("? CameraManager found/created");
		
		// Set this actor as view target
		CameraManager->SetViewTarget(this, 0.0f);
		UE_LOG_SUCCESS("? Set as ViewTarget");
		
		// Verify ViewTarget
		AActor* CurrentTarget = CameraManager->GetViewTarget();
		if (CurrentTarget == this)
		{
			UE_LOG_SUCCESS("? ViewTarget verified: %s", GetName().ToString().c_str());
		}
		else
		{
			UE_LOG_ERROR("? ViewTarget mismatch!");
		}
	}
}

void ACameraTestActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	RunAutomatedTests(DeltaTime);
}

void ACameraTestActor::TestCameraShake_Explosion()
{
	if (!GWorld)
	{
		UE_LOG_ERROR("TestCameraShake_Explosion: GWorld is null");
		return;
	}
	
	APlayerCameraManager* CameraManager = GWorld->GetCameraManager();
	if (!CameraManager)
	{
		UE_LOG_ERROR("TestCameraShake_Explosion: CameraManager not found");
		return;
	}
	
	UE_LOG_INFO("=== Testing Camera Shake: Explosion ===");
	
	// Find or create CameraShake modifier
	UCameraModifier_CameraShake* Shake = Cast<UCameraModifier_CameraShake>(
		CameraManager->FindCameraModifierByClass(UCameraModifier_CameraShake::StaticClass())
	);
	
	if (!Shake)
	{
		UE_LOG("Creating CameraShake modifier...");
		Shake = Cast<UCameraModifier_CameraShake>(
			CameraManager->AddCameraModifier(UCameraModifier_CameraShake::StaticClass())
		);
	}
	
	if (Shake)
	{
		UE_LOG_SUCCESS("? CameraShake modifier found/created");
		Shake->StartShake(1.5f, 20.0f, 8.0f, ECameraShakePattern::Perlin);
		UE_LOG_SUCCESS("? Explosion shake started (1.5s, LocAmp=20, RotAmp=8, Perlin)");
	}
	else
	{
		UE_LOG_ERROR("? Failed to create CameraShake modifier");
	}
}

void ACameraTestActor::TestCameraShake_Hit()
{
	if (!GWorld)
	{
		UE_LOG_ERROR("TestCameraShake_Hit: GWorld is null");
		return;
	}
	
	APlayerCameraManager* CameraManager = GWorld->GetCameraManager();
	if (!CameraManager)
	{
		UE_LOG_ERROR("TestCameraShake_Hit: CameraManager not found");
		return;
	}
	
	UE_LOG_INFO("=== Testing Camera Shake: Hit ===");
	
	UCameraModifier_CameraShake* Shake = Cast<UCameraModifier_CameraShake>(
		CameraManager->FindCameraModifierByClass(UCameraModifier_CameraShake::StaticClass())
	);
	
	if (!Shake)
	{
		Shake = Cast<UCameraModifier_CameraShake>(
			CameraManager->AddCameraModifier(UCameraModifier_CameraShake::StaticClass())
		);
	}
	
	if (Shake)
	{
		UE_LOG_SUCCESS("? CameraShake modifier found/created");
		Shake->StartShake(0.3f, 15.0f, 10.0f, ECameraShakePattern::Random);
		UE_LOG_SUCCESS("? Hit shake started (0.3s, LocAmp=15, RotAmp=10, Random)");
	}
	else
	{
		UE_LOG_ERROR("? Failed to create CameraShake modifier");
	}
}

void ACameraTestActor::TestCameraShake_Earthquake()
{
	if (!GWorld)
	{
		UE_LOG_ERROR("TestCameraShake_Earthquake: GWorld is null");
		return;
	}
	
	APlayerCameraManager* CameraManager = GWorld->GetCameraManager();
	if (!CameraManager)
	{
		UE_LOG_ERROR("TestCameraShake_Earthquake: CameraManager not found");
		return;
	}
	
	UE_LOG_INFO("=== Testing Camera Shake: Earthquake ===");
	
	UCameraModifier_CameraShake* Shake = Cast<UCameraModifier_CameraShake>(
		CameraManager->FindCameraModifierByClass(UCameraModifier_CameraShake::StaticClass())
	);
	
	if (!Shake)
	{
		Shake = Cast<UCameraModifier_CameraShake>(
			CameraManager->AddCameraModifier(UCameraModifier_CameraShake::StaticClass())
		);
	}
	
	if (Shake)
	{
		UE_LOG_SUCCESS("? CameraShake modifier found/created");
		Shake->StartShake(8.0f, 25.0f, 12.0f, ECameraShakePattern::Perlin);
		UE_LOG_SUCCESS("? Earthquake shake started (8.0s, LocAmp=25, RotAmp=12, Perlin)");
	}
	else
	{
		UE_LOG_ERROR("? Failed to create CameraShake modifier");
	}
}

void ACameraTestActor::TestViewTargetSwitch()
{
	if (!GWorld)
	{
		UE_LOG_ERROR("TestViewTargetSwitch: GWorld is null");
		return;
	}
	
	APlayerCameraManager* CameraManager = GWorld->GetCameraManager();
	if (!CameraManager)
	{
		UE_LOG_ERROR("TestViewTargetSwitch: CameraManager not found");
		return;
	}
	
	UE_LOG_INFO("=== Testing ViewTarget Switch ===");
	
	// Get current view target
	AActor* CurrentTarget = CameraManager->GetViewTarget();
	UE_LOG("Current ViewTarget: %s", 
		CurrentTarget ? CurrentTarget->GetName().ToString().c_str() : "null");
	
	// Switch back to this actor with blend time
	if (CurrentTarget != this)
	{
		CameraManager->SetViewTarget(this, 2.0f);
		UE_LOG_SUCCESS("? ViewTarget switch initiated (2s blend)");
	}
	else
	{
		UE_LOG("Already viewing this actor");
	}
}

void ACameraTestActor::RunAutomatedTests(float DeltaTime)
{
	TestTimer += DeltaTime;
	
	// Run tests in sequence every 5 seconds
	constexpr float TestInterval = 5.0f;
	
	if (TestTimer >= TestInterval)
	{
		TestTimer = 0.0f;
		CurrentTestPhase = (CurrentTestPhase + 1) % 4;
		
		switch (CurrentTestPhase)
		{
		case 0:
			UE_LOG_SYSTEM("=== Automated Test Phase 0: Idle ===");
			break;
			
		case 1:
			UE_LOG_SYSTEM("=== Automated Test Phase 1: Explosion Shake ===");
			TestCameraShake_Explosion();
			break;
			
		case 2:
			UE_LOG_SYSTEM("=== Automated Test Phase 2: Hit Shake ===");
			TestCameraShake_Hit();
			break;
			
		case 3:
			UE_LOG_SYSTEM("=== Automated Test Phase 3: Earthquake Shake ===");
			TestCameraShake_Earthquake();
			break;
		}
	}
}

void ACameraTestActor::LogTestResult(const char* TestName, bool bPassed)
{
	if (bPassed)
	{
		UE_LOG_SUCCESS("? Test Passed: %s", TestName);
	}
	else
	{
		UE_LOG_ERROR("? Test Failed: %s", TestName);
	}
}
