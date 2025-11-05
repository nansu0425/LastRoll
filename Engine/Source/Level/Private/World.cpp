#include "pch.h"
#include "Level/Public/World.h"
#include "Level/Public/Level.h"
#include "Actor/Public/AmbientLight.h"
#include "Actor/Public/PlayerCameraManager.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Component/Public/ActorComponent.h"
#include "Game/Actor/Public/Player.h"
#include "Utility/Public/JsonSerializer.h"
#include "Manager/Config/Public/ConfigManager.h"
#include "Manager/Path/Public/PathManager.h"
#include "Editor/Public/Editor.h"

IMPLEMENT_CLASS(UWorld, UObject)

UWorld::UWorld()
	: WorldType(EWorldType::Editor)
	, bBegunPlay(false)
{
}

UWorld::UWorld(EWorldType InWorldType)
	: WorldType(InWorldType)
	, bBegunPlay(false)
{
}

UWorld::~UWorld()
{
	EndPlay();
	if (Level)
	{
		ULevel* CurrentLevel = Level;
		SafeDelete(CurrentLevel); // 내부 Clean up은 Level의 소멸자에서 수행
		Level = nullptr;
	}
}

void UWorld::BeginPlay()
{
	if (bBegunPlay)
	{
		return;
	}

	if (!Level)
	{
		UE_LOG_ERROR("World: BeginPlay 호출 전에 로드된 Level이 없습니다.");
		return;
	}

	Level->Init();
	bBegunPlay = true;
}

bool UWorld::EndPlay()
{
	if (!Level || !bBegunPlay)
	{
		bBegunPlay = false;
		return false;
	}

	FlushPendingDestroy();
	// Level EndPlay
	bBegunPlay = false;
	return true;
}

void UWorld::Tick(float DeltaTimes)
{
	if (!Level || !bBegunPlay)
	{
		return;
	}

	// 스폰 / 삭제 처리
	FlushPendingDestroy();

	// TODO: 현재 임시로 OCtree 업데이트 처리
	Level->UpdateOctree();

	if (WorldType == EWorldType::Editor )
	{
		// Use index-based loop to safely handle actors spawned/destroyed during Tick
		auto& LevelActors = Level->GetLevelActors();
		for (int32 i = 0; i < LevelActors.size(); ++i)
		{
			AActor* Actor = LevelActors[i];
			if(Actor && Actor->CanTickInEditor() && Actor->CanTick())
			{
				Actor->Tick(DeltaTimes);
			}

			if (Actor && Actor->IsPendingDestroy())
			{
				DestroyActor(Actor);
			}
		}
	}

	if (WorldType == EWorldType::Game || WorldType == EWorldType::PIE)
	{
		auto& LevelActors = Level->GetLevelActors();
		for (int32 i = 0; i < LevelActors.size(); ++i)
		{
			if(LevelActors[i]->CanTick())
			{
				LevelActors[i]->Tick(DeltaTimes);
			}

			if (LevelActors[i]->IsPendingDestroy())
			{
				DestroyActor(LevelActors[i]);
			}
		}

		// Update camera manager (Game/PIE only) - Get from player
		APlayerCameraManager* CameraManager = GetCameraManager();
		if (CameraManager)
		{
			CameraManager->UpdateCamera(DeltaTimes);
		}
	}
	// 충돌 감지 업데이트
	UpdateCollisions();
}

ULevel* UWorld::GetLevel() const
{
	return Level;
}

/**
* @brief 지정된 경로에서 Level을 로드하고 현재 Level로 전환합니다.
* @param InLevelFilePath 로드할 Level 파일 경로
* @return 로드 성공 여부
* @note FilePath는 최종 확정된 경로여야 합니다. EditorEngine을 통해 호출됩니다.
*/
bool UWorld::LoadLevel(path InLevelFilePath)
{
	JSON LevelJson;
	ULevel* NewLevel = nullptr;

	try
	{
		FString LevelNameString = InLevelFilePath.stem().string();
		NewLevel = NewObject<ULevel>(this);
		NewLevel->SetName(LevelNameString);

		if (!FJsonSerializer::LoadJsonFromFile(LevelJson, InLevelFilePath.string()))
		{
			UE_LOG_ERROR("World: Level JSON 로드에 실패했습니다: %s", InLevelFilePath.string().c_str());
			SafeDelete(NewLevel);
			return false;
		}

		NewLevel->SetOuter(this);
		SwitchToLevel(NewLevel);
		NewLevel->Serialize(true, LevelJson);

		UConfigManager::GetInstance().SetLastUsedLevelPath(InLevelFilePath.string());
		BeginPlay();
	}
	catch (const exception& Exception)
	{
		UE_LOG_ERROR("World: Level 로드 중 예외 발생: %s", Exception.what());
		SafeDelete(NewLevel);
		CreateNewLevel();
		BeginPlay();
		return false;
	}


	return true;
}

/**
* @brief 현재 Level을 지정된 경로에 저장합니다.
* @param InLevelFilePath 저장할 파일 경로
* @return 저장 성공 여부
* @note FilePath는 최종 확정된 경로여야 합니다. EditorEngine을 통해 호출됩니다.
*/
bool UWorld::SaveCurrentLevel(path InLevelFilePath) const
{
	if (!Level)
	{
		UE_LOG_ERROR("World: 저장할 Level이 없습니다.");
		return false;
	}

	if(WorldType != EWorldType::Editor && WorldType != EWorldType::EditorPreview)
	{
		UE_LOG_ERROR("World: 게임 또는 PIE 모드에서는 Level 저장이 허용되지 않습니다.");
		return false;
	}

	try
	{
		JSON LevelJson;
		Level->Serialize(false, LevelJson);

		if (!FJsonSerializer::SaveJsonToFile(LevelJson, InLevelFilePath.string()))
		{
			UE_LOG_ERROR("World: Level 저장에 실패했습니다: %s", InLevelFilePath.string().c_str());
			return false;
		}

	}
	catch (const exception& Exception)
	{
		UE_LOG_ERROR("World: Level 저장 중 예외 발생: %s", Exception.what());
		return false;
	}

	return true;
}

AActor* UWorld::SpawnActor(UClass* InActorClass, JSON* ActorJsonData)
{
	if (!Level)
	{
		UE_LOG_ERROR("World: Actor를 Spawn할 수 있는 Level이 없습니다.");
		return nullptr;
	}

	return Level->SpawnActorToLevel(InActorClass, ActorJsonData);
}

/**
* @brief 지정된 Actor를 월드에서 삭제합니다. 실제 삭제는 안전한 시점에 이루어집니다.
* @param InActor 삭제할 Actor
* @return 삭제 요청이 성공적으로 접수되었는지 여부
*/
bool UWorld::DestroyActor(AActor* InActor)
{
	if (!Level)
	{
		UE_LOG_ERROR("World: Level이 없어 Actor 삭제를 수행할 수 없습니다.");
		return false;
	}

	if (!InActor)
	{
		UE_LOG_ERROR("World: DestroyActor에 null 포인터가 전달되었습니다.");
		return false;
	}

	if (std::find(PendingDestroyActors.begin(), PendingDestroyActors.end(), InActor) != PendingDestroyActors.end())
	{
		UE_LOG_ERROR("World: 이미 삭제 대기 중인 액터에 대한 중복 삭제 요청입니다.");
		return false; // 이미 삭제 대기 중인 액터
	}

	PendingDestroyActors.push_back(InActor);
	return true;
}

EWorldType UWorld::GetWorldType() const
{
	return WorldType;
}

void UWorld::SetWorldType(EWorldType InWorldType)
{
	WorldType = InWorldType;
}

/**
 * @brief 삭제 대기 중인 Actor들을 실제로 삭제합니다.
 * @note 이 함수는 Tick 루프 내에서 안전한 시점에 호출되어야 합니다.
 */
void UWorld::FlushPendingDestroy()
{
	if (PendingDestroyActors.empty() || !Level)
	{
		return;
	}

	TArray<AActor*> ActorsToProcess = PendingDestroyActors;
	PendingDestroyActors.clear();
	UE_LOG("World: %zu개의 Actor를 삭제합니다.", ActorsToProcess.size());
	for (AActor* ActorToDestroy : ActorsToProcess)
	{
		if (!Level->DestroyActor(ActorToDestroy))
		{
			UE_LOG_ERROR("World: Actor 삭제에 실패했습니다: %s", ActorToDestroy->GetName().ToString().c_str());
		}
	}
}

/**
 * @brief 모든 PrimitiveComponent의 충돌을 업데이트합니다.
 */
void UWorld::UpdateCollisions()
{
	if (!Level)
	{
		return;
	}

	// 모든 PrimitiveComponent 수집
	TArray<UPrimitiveComponent*> AllPrimitives;
	for (AActor* Actor : Level->GetLevelActors())
	{
		if (!Actor)
		{
			continue;
		}

		for (UActorComponent* Comp : Actor->GetOwnedComponents())
		{
			if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(Comp))
			{
				if (PrimComp->GetGenerateOverlapEvents())
				{
					AllPrimitives.push_back(PrimComp);
				}
			}
		}
	}

	// 각 컴포넌트의 충돌 업데이트
	for (UPrimitiveComponent* PrimComp : AllPrimitives)
	{
		PrimComp->UpdateOverlaps(AllPrimitives);
	}
}

/**
 * @brief 현재 Level을 새 Level로 전환합니다. 기존 Level은 소멸됩니다.
 * @param InNewLevel 새로 전환할 Level
 * @note 이전 Level의 안전한 종료 및 메모리 해제를 여기에서 책입집니다.
 */
void UWorld::SwitchToLevel(ULevel* InNewLevel)
{
	EndPlay();

	// CRITICAL: Level 전환 전에 Editor의 선택 상태를 클리어
	// 이전 Level의 Actor를 가리키는 댕글링 포인터 방지
	if (GEditor)
	{
		GEditor->GetEditorModule()->SelectActor(nullptr);
		GEditor->GetEditorModule()->SelectComponent(nullptr);
	}

	if (Level)
	{
		ULevel* OldLevel = Level;
		SafeDelete(OldLevel);
		Level = nullptr;
	}

	Level = InNewLevel;
	PendingDestroyActors.clear();
	bBegunPlay = false;
}

UObject* UWorld::Duplicate()
{
	UWorld* World = Cast<UWorld>(Super::Duplicate());
	return World;
}

void UWorld::DuplicateSubObjects(UObject* DuplicatedObject)
{
	Super::DuplicateSubObjects(DuplicatedObject);
	UWorld* World = Cast<UWorld>(DuplicatedObject);
	World->Level = Cast<ULevel>(Level->Duplicate());
	// PIE World의 Level에 Outer 설정 (메모리 추적을 위해)
	if (World->Level)
	{
		World->Level->SetOuter(World);
	}
}

void UWorld::CreateNewLevel(const FName& InLevelName)
{
	ULevel* NewLevel = NewObject<ULevel>();
	NewLevel->SetName(InLevelName);
	NewLevel->SetOuter(this);
	SwitchToLevel(NewLevel);

	// 기본 AmbientLight 추가
	AActor* SpawnedActor = SpawnActor(AAmbientLight::StaticClass());
	if (AAmbientLight* AmbientLight = Cast<AAmbientLight>(SpawnedActor))
	{
		AmbientLight->SetActorLocation(FVector(0.0f, 0.0f, 0.0f));
		AmbientLight->SetName("AmbientLight");
	}

	BeginPlay();
}

/**
 * @brief Gets the first APlayer actor found in the level
 * @return Pointer to the first APlayer, or nullptr if not found
 */
APlayer* UWorld::GetFirstPlayerActor()
{
	if (!Level)
	{
		return nullptr;
	}

	// Find first player actor in level
	for (AActor* Actor : Level->GetLevelActors())
	{
		if (APlayer* Player = Cast<APlayer>(Actor))
		{
			return Player;
		}
	}

	return nullptr;
}

/**
 * @brief Gets the PlayerCameraManager from the first player actor
 * @return Pointer to the PlayerCameraManager, or nullptr if no player found
 * @note This replaces the old UWorld-owned CameraManager with player-owned pattern
 */
APlayerCameraManager* UWorld::GetCameraManager()
{
	APlayer* Player = GetFirstPlayerActor();
	if (Player)
	{
		return Player->GetPlayerCameraManager();
	}

	return nullptr;
}
