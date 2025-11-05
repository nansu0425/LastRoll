#include "pch.h"
#include "Render/UI/Window/Public/CameraShakePresetEditorWindow.h"
#include "Manager/Camera/Public/CameraShakePresetManager.h"
#include "Global/Public/CameraShakeTypes.h"
#include "Component/Camera/Public/CameraModifier_CameraShake.h"
#include "Component/Camera/Public/CameraComponent.h"
#include "Actor/Public/PlayerCameraManager.h"
#include "Actor/Public/Actor.h"
#include "Level/Public/World.h"
#include "Level/Public/Level.h"
#include "Core/Public/NewObject.h"
#include "ImGui/imgui.h"
#include <cstring>

IMPLEMENT_CLASS(UCameraShakePresetEditorWindow, UUIWindow)

UCameraShakePresetEditorWindow::UCameraShakePresetEditorWindow()
	: SelectedPresetName(FName(""))
	, bShowAddDialog(false)
	, bShowRemoveDialog(false)
	, DefaultPresetsFilePath("Engine/Data/CameraShakePresets.json")
{
	FUIWindowConfig Config;
	Config.WindowTitle = "Camera Shake Preset Editor";
	Config.DefaultSize = ImVec2(900, 700);
	Config.DefaultPosition = ImVec2(200, 100);
	Config.MinSize = ImVec2(800, 600);
	Config.DockDirection = EUIDockDirection::None;  // 독립 윈도우
	Config.Priority = 25;
	Config.bResizable = true;
	Config.bMovable = true;
	Config.bCollapsible = true;

	Config.UpdateWindowFlags();
	SetConfig(Config);

	memset(NewPresetNameBuffer, 0, sizeof(NewPresetNameBuffer));
}

UCameraShakePresetEditorWindow::~UCameraShakePresetEditorWindow()
{
}

void UCameraShakePresetEditorWindow::Initialize()
{
	// PresetManager 초기화 (필요하다면)
	UCameraShakePresetManager& PresetManager = UCameraShakePresetManager::GetInstance();
	PresetManager.Initialize();

	UE_LOG("CameraShakePresetEditorWindow: Initialized");
}

void UCameraShakePresetEditorWindow::RenderWidget()
{
	Super::RenderWidget();
	UCameraShakePresetManager& PresetManager = UCameraShakePresetManager::GetInstance();

	// 레이아웃: 좌우 분할 + 하단 버튼
	float PanelWidth = ImGui::GetContentRegionAvail().x;
	float LeftPanelWidth = 250.0f;
	float RightPanelWidth = PanelWidth - LeftPanelWidth - 10.0f; // 간격 10px
	float TopPanelHeight = ImGui::GetContentRegionAvail().y - 50.0f; // 하단 버튼 50px

	// ===== 왼쪽 패널: Preset 목록 =====
	ImGui::BeginChild("PresetListPanel", ImVec2(LeftPanelWidth, TopPanelHeight), true);
	DrawPresetListPanel();
	ImGui::EndChild();

	ImGui::SameLine();

	// ===== 오른쪽 패널: Preset 상세 편집 =====
	ImGui::BeginChild("PresetDetailPanel", ImVec2(RightPanelWidth, TopPanelHeight), true);
	DrawPresetDetailPanel();
	ImGui::EndChild();

	// ===== 하단 패널: Save/Load/Test 버튼 =====
	ImGui::Separator();
	DrawBottomPanel();

	// ===== 다이얼로그 렌더링 =====
	if (bShowAddDialog)
		DrawAddPresetDialog();

	if (bShowRemoveDialog)
		DrawRemovePresetDialog();
}

void UCameraShakePresetEditorWindow::DrawPresetListPanel()
{
	UCameraShakePresetManager& PresetManager = UCameraShakePresetManager::GetInstance();

	ImGui::Text("Preset List:");
	ImGui::Separator();

	// Add New Preset 버튼
	if (ImGui::Button("Add New Preset", ImVec2(-1, 0)))
	{
		bShowAddDialog = true;
	}

	ImGui::Spacing();

	// Preset 목록
	TArray<FName> PresetNames = PresetManager.GetAllPresetNames();
	for (const FName& PresetName : PresetNames)
	{
		bool bIsSelected = (PresetName == SelectedPresetName);
		if (ImGui::Selectable(PresetName.ToString().c_str(), bIsSelected))
		{
			SelectedPresetName = PresetName;
		}
	}

	ImGui::Spacing();
	ImGui::Separator();

	// Remove Selected Preset 버튼
	if (SelectedPresetName.ToString().empty())
	{
		ImGui::BeginDisabled();
	}

	if (ImGui::Button("Remove Selected", ImVec2(-1, 0)))
	{
		bShowRemoveDialog = true;
	}

	if (SelectedPresetName.ToString().empty())
	{
		ImGui::EndDisabled();
	}
}

void UCameraShakePresetEditorWindow::DrawPresetDetailPanel()
{
	UCameraShakePresetManager& PresetManager = UCameraShakePresetManager::GetInstance();

	if (SelectedPresetName.ToString().empty())
	{
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No Preset Selected");
		ImGui::Text("Select a preset from the list to edit.");
		return;
	}

	// 선택된 Preset 찾기
	FCameraShakePresetData* Preset = PresetManager.FindPreset(SelectedPresetName);
	if (!Preset)
	{
		ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Preset not found: %s", SelectedPresetName.ToString().c_str());
		return;
	}

	// Detail Panel 렌더링
	ImGui::Text("Editing: %s", SelectedPresetName.ToString().c_str());
	ImGui::Separator();

	if (DetailPanel.Draw("PresetDetailPanel", Preset))
	{
		// Preset 데이터 변경됨 (자동으로 PresetManager에 반영됨)
		UE_LOG("CameraShakePresetEditorWindow: Preset '%s' modified", SelectedPresetName.ToString().c_str());
	}
}

void UCameraShakePresetEditorWindow::DrawBottomPanel()
{
	UCameraShakePresetManager& PresetManager = UCameraShakePresetManager::GetInstance();

	// Save/Load 버튼
	if (ImGui::Button("Save Presets to File", ImVec2(200, 0)))
	{
		if (PresetManager.SavePresetsToFile(DefaultPresetsFilePath))
		{
			UE_LOG_SUCCESS("CameraShakePresetEditorWindow: Presets saved to %s", DefaultPresetsFilePath.c_str());
		}
		else
		{
			UE_LOG_ERROR("CameraShakePresetEditorWindow: Failed to save presets");
		}
	}

	ImGui::SameLine();

	if (ImGui::Button("Load Presets from File", ImVec2(200, 0)))
	{
		if (PresetManager.LoadPresetsFromFile(DefaultPresetsFilePath))
		{
			UE_LOG_SUCCESS("CameraShakePresetEditorWindow: Presets loaded from %s", DefaultPresetsFilePath.c_str());
			// 선택 초기화 (로드 후 목록이 변경될 수 있음)
			SelectedPresetName = FName("");
		}
		else
		{
			UE_LOG_ERROR("CameraShakePresetEditorWindow: Failed to load presets");
		}
	}

	ImGui::SameLine();

	// Test Preset 버튼 (PIE 모드에서만 활성화)
	UWorld* World = GWorld;
	bool bIsPIE = (World && World->GetWorldType() == EWorldType::PIE);

	if (!bIsPIE || SelectedPresetName.ToString().empty())
	{
		ImGui::BeginDisabled();
	}

	if (ImGui::Button("Test Preset (PIE)", ImVec2(200, 0)))
	{
		// PlayerCameraManager 찾기 (Player가 자동으로 생성함)
		if (World)
		{
			APlayerCameraManager* CameraManager = World->GetCameraManager();
			if (CameraManager)
			{
				// Preset 재생
				if (CameraManager->PlayCameraShakePreset(SelectedPresetName))
				{
					UE_LOG_SUCCESS("CameraShakePresetEditorWindow: Testing Preset '%s'", SelectedPresetName.ToString().c_str());
				}
			}
			else
			{
				UE_LOG_ERROR("CameraShakePresetEditorWindow: PlayerCameraManager not found. Please spawn a Player actor in PIE mode.");
			}
		}
	}

	if (!bIsPIE || SelectedPresetName.ToString().empty())
	{
		ImGui::EndDisabled();
	}

	// PIE 모드가 아니면 경고 메시지
	if (!bIsPIE)
	{
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "(PIE mode required for testing)");
	}
}

void UCameraShakePresetEditorWindow::DrawAddPresetDialog()
{
	ImGui::OpenPopup("Add New Preset");

	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	if (ImGui::BeginPopupModal("Add New Preset", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Enter new preset name:");
		ImGui::SetNextItemWidth(300);
		ImGui::InputText("##NewPresetName", NewPresetNameBuffer, sizeof(NewPresetNameBuffer));

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// OK 버튼
		if (ImGui::Button("Create", ImVec2(120, 0)))
		{
			if (strlen(NewPresetNameBuffer) > 0)
			{
				UCameraShakePresetManager& PresetManager = UCameraShakePresetManager::GetInstance();

				// 새 Preset 생성
				FCameraShakePresetData NewPreset;
				NewPreset.PresetName = FName(NewPresetNameBuffer);
				NewPreset.Duration = 1.0f;
				NewPreset.LocationAmplitude = 5.0f;
				NewPreset.RotationAmplitude = 2.0f;
				NewPreset.Pattern = ECameraShakePattern::Perlin;
				NewPreset.Frequency = 10.0f;
				NewPreset.bUseDecayCurve = false;

				PresetManager.AddPreset(NewPreset);

				// 새로 생성된 Preset 선택
				SelectedPresetName = NewPreset.PresetName;

				UE_LOG_SUCCESS("CameraShakePresetEditorWindow: Preset '%s' created", NewPresetNameBuffer);

				// 다이얼로그 닫기
				memset(NewPresetNameBuffer, 0, sizeof(NewPresetNameBuffer));
				bShowAddDialog = false;
				ImGui::CloseCurrentPopup();
			}
		}

		ImGui::SameLine();

		// Cancel 버튼
		if (ImGui::Button("Cancel", ImVec2(120, 0)))
		{
			memset(NewPresetNameBuffer, 0, sizeof(NewPresetNameBuffer));
			bShowAddDialog = false;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

void UCameraShakePresetEditorWindow::DrawRemovePresetDialog()
{
	ImGui::OpenPopup("Remove Preset");

	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	if (ImGui::BeginPopupModal("Remove Preset", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Are you sure you want to remove this preset?");
		ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s", SelectedPresetName.ToString().c_str());

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// Yes 버튼
		if (ImGui::Button("Yes, Remove", ImVec2(120, 0)))
		{
			UCameraShakePresetManager& PresetManager = UCameraShakePresetManager::GetInstance();

			if (PresetManager.RemovePreset(SelectedPresetName))
			{
				UE_LOG_SUCCESS("CameraShakePresetEditorWindow: Preset '%s' removed", SelectedPresetName.ToString().c_str());
				SelectedPresetName = FName("");  // 선택 초기화
			}

			bShowRemoveDialog = false;
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();

		// Cancel 버튼
		if (ImGui::Button("Cancel", ImVec2(120, 0)))
		{
			bShowRemoveDialog = false;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

//APlayerCameraManager* UCameraShakePresetEditorWindow::FindCameraManager(UWorld* World)
//{
//	if (!World)
//		return nullptr;
//
//	ULevel* Level = World->GetLevel();
//	if (!Level)
//		return nullptr;
//
//	// 기존 PlayerCameraManager 찾기 (Player가 자동으로 생성함)
//	const TArray<AActor*>& Actors = Level->GetLevelActors();
//	for (AActor* Actor : Actors)
//	{
//		if (APlayerCameraManager* CameraManager = Cast<APlayerCameraManager>(Actor))
//		{
//			// ViewTarget이 없으면 임시로 생성
//			if (!CameraManager->GetViewTarget())
//			{
//				// 임시 카메라 액터 생성
//				if (!TempCameraActor)
//				{
//					TempCameraActor = World->SpawnActor(AActor::StaticClass());
//					if (TempCameraActor)
//					{
//						UCameraComponent* CameraComp = NewObject<UCameraComponent>(TempCameraActor);
//						CameraComp->SetFieldOfView(90.0f);
//						TempCameraActor->SetRootComponent(CameraComp);
//					}
//				}
//
//				// ViewTarget 설정
//				if (TempCameraActor)
//				{
//					CameraManager->SetViewTarget(TempCameraActor, 0.0f);
//					UE_LOG("CameraShakePresetEditorWindow: Created temp ViewTarget for PlayerCameraManager");
//				}
//			}
//			return CameraManager;
//		}
//	}
//
//	// PlayerCameraManager가 없으면 Player도 없는 것
//	UE_LOG_WARNING("CameraShakePresetEditorWindow: PlayerCameraManager not found. Please spawn a Player actor in PIE mode.");
//	return nullptr;
//}
