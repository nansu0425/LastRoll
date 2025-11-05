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
	
	// CreateDefaultSubobject를 사용하여 카메라 컴포넌트 생성 (APlayer/AEnemy 패턴)
	CameraComponent = CreateDefaultSubobject<UCameraComponent>();
	
	bCanEverTick = true; // Tick 활성화
}

ACameraTestActor::~ACameraTestActor()
{
}

UClass* ACameraTestActor::GetDefaultRootComponent()
{
	// 루트로 SceneComponent 사용 (단순 변환 전용 컴포넌트)
	return USceneComponent::StaticClass();
}

void ACameraTestActor::InitializeComponents()
{
	AActor::InitializeComponents();
	
	// 루트 컴포넌트 가져오기
	USceneComponent* RootComp = GetRootComponent();
	
	if (!RootComp)
	{
		UE_LOG_ERROR("ACameraTestActor::InitializeComponents - RootComponent가 null입니다!");
		return;
	}
	
	// CameraComponent를 루트에 부착 (APlayer/AEnemy 패턴 따르기)
	if (CameraComponent)
	{
		CameraComponent->AttachToComponent(RootComp);
		
		// 카메라 파라미터 설정
		CameraComponent->SetFieldOfView(90.0f);
		CameraComponent->SetAspectRatio(16.0f / 9.0f);
		CameraComponent->SetNearClipPlane(1.0f);
		CameraComponent->SetFarClipPlane(10000.0f);

		// ===== PostProcessSettings 설정 (Vignette 효과 테스트) =====
		FPostProcessSettings& PPSettings = CameraComponent->GetPostProcessSettings();

		// 강한 빨간색 비네트 효과
		PPSettings.bOverride_VignetteIntensity = true;
		PPSettings.VignetteIntensity = 0.8f;

		PPSettings.bOverride_VignetteColor = true;
		PPSettings.VignetteColor = FVector(1.0f, 0.0f, 0.0f);  // 빨간색

		UE_LOG_DEBUG("ACameraTestActor: CameraComponent 초기화됨 (FOV=90, Aspect=16:9)");
		UE_LOG_SUCCESS("ACameraTestActor: PostProcessSettings 설정됨 (Vignette: Intensity=0.8, Color=Red)");
	}
	else
	{
		UE_LOG_ERROR("ACameraTestActor::InitializeComponents - CameraComponent가 null입니다!");
	}
}

void ACameraTestActor::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG_SUCCESS("=== CameraTestActor: BeginPlay ===");
	UE_LOG("위치: (%.2f, %.2f, %.2f)", 
		GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z);
	
	// CameraComponent 검증
	if (CameraComponent)
	{
		UE_LOG_SUCCESS("✓ CameraComponent 생성 성공");
		UE_LOG("  FOV: %.2f, AspectRatio: %.2f", 
			CameraComponent->GetFieldOfView(), 
			CameraComponent->GetAspectRatio());
	}
	else
	{
		UE_LOG_ERROR("✗ CameraComponent가 null입니다!");
	}
	
	// World와 CameraManager 가져오기
	if (!GWorld)
	{
		UE_LOG_ERROR("✗ GWorld가 null입니다!");
		return;
	}
	
	APlayerCameraManager* CameraManager = GWorld->GetCameraManager();
	if (!CameraManager)
	{
		UE_LOG_WARNING("World에 CameraManager가 설정되지 않음 - 새로 생성");
		CameraManager = NewObject<APlayerCameraManager>(GWorld);
		GWorld->SetCameraManager(CameraManager);
	}
	
	if (CameraManager)
	{
		UE_LOG_SUCCESS("✓ CameraManager 찾음생성됨");
		
		// 이 액터를 뷰 타겟으로 설정
		CameraManager->SetViewTarget(this, 0.0f);
		UE_LOG_SUCCESS("✓ ViewTarget으로 설정됨");
		
		// ViewTarget 검증
		AActor* CurrentTarget = CameraManager->GetViewTarget();
		if (CurrentTarget == this)
		{
			UE_LOG_SUCCESS("✓ ViewTarget 확인됨: %s", GetName().ToString().c_str());
		}
		else
		{
			UE_LOG_ERROR("✗ ViewTarget 불일치!");
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
		UE_LOG_ERROR("TestCameraShake_Explosion: GWorld가 null입니다");
		return;
	}
	
	APlayerCameraManager* CameraManager = GWorld->GetCameraManager();
	if (!CameraManager)
	{
		UE_LOG_ERROR("TestCameraShake_Explosion: CameraManager를 찾을 수 없습니다");
		return;
	}
	
	UE_LOG_INFO("=== 카메라 흔들림 테스트: 폭발 ===");
	
	// CameraShake 모디파이어 찾기 또는 생성
	UCameraModifier_CameraShake* Shake = Cast<UCameraModifier_CameraShake>(
		CameraManager->FindCameraModifierByClass(UCameraModifier_CameraShake::StaticClass())
	);
	
	if (!Shake)
	{
		UE_LOG("CameraShake 모디파이어 생성 중...");
		Shake = Cast<UCameraModifier_CameraShake>(
			CameraManager->AddCameraModifier(UCameraModifier_CameraShake::StaticClass())
		);
	}
	
	if (Shake)
	{
		UE_LOG_SUCCESS("✓ CameraShake 모디파이어 찾음/생성됨");
		Shake->StartShake(1.5f, 20.0f, 8.0f, ECameraShakePattern::Perlin);
		UE_LOG_SUCCESS("✓ 폭발 흔들림 시작됨 (1.5초, 위치진폭=20, 회전진폭=8, Perlin)");
	}
	else
	{
		UE_LOG_ERROR("✗ CameraShake 모디파이어 생성 실패");
	}
}

void ACameraTestActor::TestCameraShake_Hit()
{
	if (!GWorld)
	{
		UE_LOG_ERROR("TestCameraShake_Hit: GWorld가 null입니다");
		return;
	}
	
	APlayerCameraManager* CameraManager = GWorld->GetCameraManager();
	if (!CameraManager)
	{
		UE_LOG_ERROR("TestCameraShake_Hit: CameraManager를 찾을 수 없습니다");
		return;
	}
	
	UE_LOG_INFO("=== 카메라 흔들림 테스트: 피격 ===");
	
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
		UE_LOG_SUCCESS("✓ CameraShake 모디파이어 찾음/생성됨");
		Shake->StartShake(0.3f, 15.0f, 10.0f, ECameraShakePattern::Random);
		UE_LOG_SUCCESS("✓ 피격 흔들림 시작됨 (0.3초, 위치진폭=15, 회전진폭=10, Random)");
	}
	else
	{
		UE_LOG_ERROR("✗ CameraShake 모디파이어 생성 실패");
	}
}

void ACameraTestActor::TestCameraShake_Earthquake()
{
	if (!GWorld)
	{
		UE_LOG_ERROR("TestCameraShake_Earthquake: GWorld가 null입니다");
		return;
	}
	
	APlayerCameraManager* CameraManager = GWorld->GetCameraManager();
	if (!CameraManager)
	{
		UE_LOG_ERROR("TestCameraShake_Earthquake: CameraManager를 찾을 수 없습니다");
		return;
	}
	
	UE_LOG_INFO("=== 카메라 흔들림 테스트: 지진 ===");
	
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
		UE_LOG_SUCCESS("✓ CameraShake 모디파이어 찾음/생성됨");
		Shake->StartShake(8.0f, 25.0f, 12.0f, ECameraShakePattern::Perlin);
		UE_LOG_SUCCESS("✓ 지진 흔들림 시작됨 (8.0초, 위치진폭=25, 회전진폭=12, Perlin)");
	}
	else
	{
		UE_LOG_ERROR("✗ CameraShake 모디파이어 생성 실패");
	}
}

void ACameraTestActor::TestViewTargetSwitch()
{
	if (!GWorld)
	{
		UE_LOG_ERROR("TestViewTargetSwitch: GWorld가 null입니다");
		return;
	}
	
	APlayerCameraManager* CameraManager = GWorld->GetCameraManager();
	if (!CameraManager)
	{
		UE_LOG_ERROR("TestViewTargetSwitch: CameraManager를 찾을 수 없습니다");
		return;
	}
	
	UE_LOG_INFO("=== ViewTarget 전환 테스트 ===");
	
	// 현재 뷰 타겟 가져오기
	AActor* CurrentTarget = CameraManager->GetViewTarget();
	UE_LOG("현재 ViewTarget: %s", 
		CurrentTarget ? CurrentTarget->GetName().ToString().c_str() : "null");
	
	// 블렌드 시간을 적용하여 이 액터로 다시 전환
	if (CurrentTarget != this)
	{
		CameraManager->SetViewTarget(this, 2.0f);
		UE_LOG_SUCCESS("✓ ViewTarget 전환 시작됨 (2초 블렌드)");
	}
	else
	{
		UE_LOG("이미 이 액터를 보고 있습니다");
	}
}

void ACameraTestActor::RunAutomatedTests(float DeltaTime)
{
	static float ElapsedTime = 0.0f;
	ElapsedTime += DeltaTime;
	
	TestTimer += DeltaTime;

	CameraComponent->SetTargetAspectRatio(1.0f + 1.0f * sin(ElapsedTime * 2 * PI));
	
	// 5초마다 테스트를 순서대로 실행
	constexpr float TestInterval = 5.0f;

	if (TestTimer >= TestInterval)
	{
		TestTimer = 0.0f;
		CurrentTestPhase = (CurrentTestPhase + 1) % 5;  // 페이즈 수 증가 (0-4)

		switch (CurrentTestPhase)
		{
		case 0:
			UE_LOG_SYSTEM("=== 자동화 테스트 페이즈 0: Vignette OFF ===");
			if (CameraComponent)
			{
				FPostProcessSettings& PP = CameraComponent->GetPostProcessSettings();
				PP.bOverride_VignetteIntensity = false;  // 효과 비활성화
				UE_LOG_SUCCESS("✓ Vignette 효과 비활성화");
			}
			break;

		case 1:
			UE_LOG_SYSTEM("=== 자동화 테스트 페이즈 1: Vignette 빨간색 (강함) ===");
			if (CameraComponent)
			{
				FPostProcessSettings& PP = CameraComponent->GetPostProcessSettings();
				PP.bOverride_VignetteIntensity = true;
				PP.VignetteIntensity = 0.8f;
				PP.bOverride_VignetteColor = true;
				PP.VignetteColor = FVector(1.0f, 0.0f, 0.0f);  // 빨간색
				UE_LOG_SUCCESS("✓ 빨간색 비네트 (Intensity=0.8)");
			}
			break;

		case 2:
			UE_LOG_SYSTEM("=== 자동화 테스트 페이즈 2: Vignette 파란색 (약함) ===");
			if (CameraComponent)
			{
				FPostProcessSettings& PP = CameraComponent->GetPostProcessSettings();
				PP.bOverride_VignetteIntensity = true;
				PP.VignetteIntensity = 0.4f;
				PP.bOverride_VignetteColor = true;
				PP.VignetteColor = FVector(0.0f, 0.0f, 1.0f);  // 파란색
				UE_LOG_SUCCESS("✓ 파란색 비네트 (Intensity=0.4)");
			}
			break;

		case 3:
			UE_LOG_SYSTEM("=== 자동화 테스트 페이즈 3: Vignette 녹색 (중간) ===");
			if (CameraComponent)
			{
				FPostProcessSettings& PP = CameraComponent->GetPostProcessSettings();
				PP.bOverride_VignetteIntensity = true;
				PP.VignetteIntensity = 0.6f;
				PP.bOverride_VignetteColor = true;
				PP.VignetteColor = FVector(0.0f, 1.0f, 0.0f);  // 녹색
				UE_LOG_SUCCESS("✓ 녹색 비네트 (Intensity=0.6)");
			}
			break;

		case 4:
			UE_LOG_SYSTEM("=== 자동화 테스트 페이즈 4: Vignette 검은색 (최대) ===");
			if (CameraComponent)
			{
				FPostProcessSettings& PP = CameraComponent->GetPostProcessSettings();
				PP.bOverride_VignetteIntensity = true;
				PP.VignetteIntensity = 1.0f;
				PP.bOverride_VignetteColor = true;
				PP.VignetteColor = FVector(0.0f, 0.0f, 0.0f);  // 검은색
				UE_LOG_SUCCESS("✓ 검은색 비네트 (Intensity=1.0)");
			}
			break;
		}
	}
}

void ACameraTestActor::LogTestResult(const char* TestName, bool bPassed)
{
	if (bPassed)
	{
		UE_LOG_SUCCESS("✓ 테스트 통과: %s", TestName);
	}
	else
	{
		UE_LOG_ERROR("✗ 테스트 실패: %s", TestName);
	}
}
