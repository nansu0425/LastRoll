#include "pch.h"
#include "Render/UI/Window/Public/CameraShakeWindow.h"
#include "Component/Camera/Public/CameraModifier_CameraShake.h"
#include "Component/Camera/Public/CameraComponent.h"
#include "Actor/Public/PlayerCameraManager.h"
#include "Level/Public/World.h"
#include "Level/Public/Level.h"
#include "Manager/Time/Public/TimeManager.h"
#include "Editor/Public/EditorEngine.h"
#include "Core/Public/NewObject.h"
#include "ImGui/imgui.h"

IMPLEMENT_CLASS(UCameraShakeWindow, UUIWindow)

UCameraShakeWindow::UCameraShakeWindow()
{
	FUIWindowConfig Config;
	Config.WindowTitle = "Camera Shake Editor";
	Config.DefaultSize = ImVec2(400, 700);
	Config.DefaultPosition = ImVec2(100, 100);
	Config.MinSize = ImVec2(350, 600);
	Config.DockDirection = EUIDockDirection::None;  // 독립 윈도우
	Config.Priority = 30;
	Config.bResizable = true;
	Config.bMovable = true;
	Config.bCollapsible = true;

	Config.UpdateWindowFlags();
	SetConfig(Config);
}

void UCameraShakeWindow::Initialize()
{
	UE_LOG("CameraShakeWindow: Initialized");

	// 초기에 CameraShake 모디파이어 자동 검색
	FindAndSetCameraShake();
}

void UCameraShakeWindow::OnPreRenderWindow(float MenuBarOffset)
{
	// 기본 렌더링 (위젯들)
	Super::OnPreRenderWindow(MenuBarOffset);

	// 주기적으로 CameraShake 재검색 (1초마다)
	UTimeManager& TimeManager = UTimeManager::GetInstance();
	float DeltaTime = TimeManager.GetDeltaTime();
	RefreshTimer += DeltaTime;
	if (RefreshTimer >= RefreshInterval)
	{
		RefreshTimer = 0.0f;
		if (!CameraShake)
		{
			FindAndSetCameraShake();
		}
	}

	// CameraShake가 설정되어 있으면 Detail Panel 렌더링
	if (CameraShake)
	{
		// GWorld는 PIE 모드일 때 PIE World를, Editor 모드일 때 Editor World를 가리킴
		UWorld* World = GWorld;
		ImGui::BeginChild("CameraShakeDetail", ImVec2(0, 0), false);
		DetailPanel.Draw("CameraShakeDetailPanel", CameraShake, World);
		ImGui::EndChild();
	}
	else
	{
		// CameraShake를 찾지 못한 경우 메시지 표시
		ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "No Camera Shake Modifier Found");
		ImGui::Spacing();
		ImGui::Text("Please ensure:");
		ImGui::BulletText("A PlayerCameraManager exists in the level");
		ImGui::BulletText("UCameraModifier_CameraShake is added to the camera manager");
		ImGui::Spacing();

		if (ImGui::Button("Refresh", ImVec2(120, 0)))
		{
			FindAndSetCameraShake();
		}
	}
}

void UCameraShakeWindow::SetCameraShake(UCameraModifier_CameraShake* InCameraShake)
{
	CameraShake = InCameraShake;

	if (CameraShake)
	{
		UE_LOG("CameraShakeWindow: Camera Shake set successfully");
	}
	else
	{
		UE_LOG_WARNING("CameraShakeWindow: Camera Shake set to nullptr");
	}
}

bool UCameraShakeWindow::FindAndSetCameraShake()
{
	// 현재 월드 가져오기 (PIE 모드면 PIE World, Editor 모드면 Editor World)
	UWorld* World = GWorld;
	if (!World)
	{
		UE_LOG_WARNING("CameraShakeWindow: World not found");
		return false;
	}

	// 현재 레벨 가져오기
	ULevel* Level = World->GetLevel();
	if (!Level)
	{
		UE_LOG_WARNING("CameraShakeWindow: Level not found");
		return false;
	}

	// 레벨에서 PlayerCameraManager 찾기
	APlayerCameraManager* CameraManager = nullptr;
	const TArray<AActor*>& Actors = Level->GetLevelActors();
	for (AActor* Actor : Actors)
	{
		if (APlayerCameraManager* Manager = Cast<APlayerCameraManager>(Actor))
		{
			CameraManager = Manager;
			break;
		}
	}

	if (!CameraManager)
	{
		UE_LOG_WARNING("CameraShakeWindow: PlayerCameraManager not found in level");
		return false;
	}

	// ViewTarget이 없으면 임시 Camera Actor 생성
	if (!CameraManager->GetViewTarget())
	{
		if (!TempCameraActor)
		{
			// 임시 Camera Actor 생성
			TempCameraActor = World->SpawnActor(AActor::StaticClass());
			if (TempCameraActor)
			{
				TempCameraActor->SetName(FName("TempCameraForShake"));

				// CameraComponent 추가
				UCameraComponent* CameraComp = NewObject<UCameraComponent>(TempCameraActor);
				if (CameraComp)
				{
					CameraComp->SetFieldOfView(90.0f);
					CameraComp->SetAspectRatio(16.0f / 9.0f);
					TempCameraActor->SetRootComponent(CameraComp);

					UE_LOG("CameraShakeWindow: Temp Camera Actor created with CameraComponent");
				}
			}
		}

		// ViewTarget 설정
		if (TempCameraActor)
		{
			CameraManager->SetViewTarget(TempCameraActor, 0.0f);
			UE_LOG("CameraShakeWindow: ViewTarget set to Temp Camera Actor");
		}
	}

	// CameraManager에서 CameraShake 모디파이어 찾기
	UCameraModifier_CameraShake* ShakeModifier = Cast<UCameraModifier_CameraShake>(
		CameraManager->FindCameraModifierByClass(UCameraModifier_CameraShake::StaticClass())
	);

	if (!ShakeModifier)
	{
		// 없으면 생성
		UE_LOG("CameraShakeWindow: Creating new UCameraModifier_CameraShake");
		ShakeModifier = Cast<UCameraModifier_CameraShake>(
			CameraManager->AddCameraModifier(UCameraModifier_CameraShake::StaticClass())
		);
	}

	if (ShakeModifier)
	{
		SetCameraShake(ShakeModifier);
		UE_LOG("CameraShakeWindow: Camera Shake Modifier found and set");
		return true;
	}
	else
	{
		UE_LOG_ERROR("CameraShakeWindow: Failed to find or create Camera Shake Modifier");
		return false;
	}
}
