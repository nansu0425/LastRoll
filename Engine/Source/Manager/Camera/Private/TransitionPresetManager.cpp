#include "pch.h"
#include "Manager/Camera/Public/TransitionPresetManager.h"
#include "Manager/Path/Public/PathManager.h"
#include <json.hpp>
#include <fstream>

IMPLEMENT_SINGLETON_CLASS(UTransitionPresetManager, UObject)

UTransitionPresetManager::UTransitionPresetManager()
{
}

UTransitionPresetManager::~UTransitionPresetManager()
{
}

void UTransitionPresetManager::Initialize()
{
	UE_LOG("TransitionPresetManager: Initializing");

	// CameraTransitionPresets.json 파일이 있는지 확인
	UPathManager& PathManager = UPathManager::GetInstance();
	path PresetFilePath = PathManager.GetEngineDataPath() / "CameraTransitionPresets.json";

	if (exists(PresetFilePath))
	{
		// 파일이 있으면 로드
		UE_LOG("TransitionPresetManager: Found existing preset file, loading...");
		if (LoadPresetsFromFile("CameraTransitionPresets.json"))
		{
			UE_LOG_SUCCESS("TransitionPresetManager: Initialized with %d presets from file", static_cast<int32>(PresetMap.size()));
			return;
		}
		else
		{
			UE_LOG_WARNING("TransitionPresetManager: Failed to load preset file, using default presets");
		}
	}

	// 파일이 없거나 로드 실패 시 기본 Preset 생성
	CreateDefaultPresets();

	UE_LOG_SUCCESS("TransitionPresetManager: Initialized with %d default presets", static_cast<int32>(PresetMap.size()));
}

void UTransitionPresetManager::CreateDefaultPresets()
{
	// QuickCut: Instant transition (0.1s, linear)
	{
		FTransitionPresetData Preset;
		Preset.PresetName = FName("QuickCut");
		Preset.Duration = 0.1f;
		Preset.bUseTimingCurve = false;
		AddPreset(Preset);
	}

	// SlowZoom: Slow zoom effect (2s, EaseInOut)
	{
		FTransitionPresetData Preset;
		Preset.PresetName = FName("SlowZoom");
		Preset.Duration = 2.0f;
		Preset.bUseTimingCurve = true;
		Preset.TimingCurve = FCubicBezierCurve::CreateEaseInOut();
		AddPreset(Preset);
	}

	// SmoothPan: Smooth camera pan (1.5s, EaseOut)
	{
		FTransitionPresetData Preset;
		Preset.PresetName = FName("SmoothPan");
		Preset.Duration = 1.5f;
		Preset.bUseTimingCurve = true;
		Preset.TimingCurve = FCubicBezierCurve::CreateEaseOut();
		AddPreset(Preset);
	}

	// Cinematic: Cinematic camera move (3s, EaseInOut)
	{
		FTransitionPresetData Preset;
		Preset.PresetName = FName("Cinematic");
		Preset.Duration = 3.0f;
		Preset.bUseTimingCurve = true;
		Preset.TimingCurve = FCubicBezierCurve::CreateEaseInOut();
		AddPreset(Preset);
	}

	// Bounce: Bouncy transition (1s, Bounce)
	{
		FTransitionPresetData Preset;
		Preset.PresetName = FName("Bounce");
		Preset.Duration = 1.0f;
		Preset.bUseTimingCurve = true;
		Preset.TimingCurve = FCubicBezierCurve::CreateBounce();
		AddPreset(Preset);
	}

	UE_LOG("TransitionPresetManager: Created default presets (QuickCut, SlowZoom, SmoothPan, Cinematic, Bounce)");
}

bool UTransitionPresetManager::LoadPresetsFromFile(const FString& FilePath)
{
	try
	{
		// 절대 경로 구성 (EngineDataPath 사용)
		UPathManager& PathManager = UPathManager::GetInstance();
		path FullPath = PathManager.GetEngineDataPath() / FilePath;

		UE_LOG("TransitionPresetManager: Loading from %s", FullPath.string().c_str());

		// 파일 읽기
		std::ifstream File(FullPath.string());
		if (!File.is_open())
		{
			UE_LOG_ERROR("TransitionPresetManager: Failed to open file for reading: %s", FullPath.string().c_str());
			return false;
		}

		FString FileContent((std::istreambuf_iterator<char>(File)), std::istreambuf_iterator<char>());
		File.close();

		// JSON 파싱
		json::JSON RootJSON = json::JSON::Load(FileContent);

		if (!RootJSON.hasKey("Presets"))
		{
			UE_LOG_ERROR("TransitionPresetManager: Invalid JSON format (missing 'Presets' key)");
			return false;
		}

		// 기존 Preset 클리어
		ClearAllPresets();

		// Preset 배열 로드
		auto& PresetArray = RootJSON["Presets"];
		int32 LoadedCount = 0;

		for (size_t i = 0; i < PresetArray.length(); ++i)
		{
			FTransitionPresetData Preset;
			Preset.Serialize(true, PresetArray[static_cast<uint32>(i)]);
			AddPreset(Preset);
			LoadedCount++;
		}

		UE_LOG_SUCCESS("TransitionPresetManager: Loaded %d presets from %s", LoadedCount, FullPath.string().c_str());
		return true;
	}
	catch (const std::exception& e)
	{
		UE_LOG_ERROR("TransitionPresetManager: Exception while loading presets: %s", e.what());
		return false;
	}
}

bool UTransitionPresetManager::SavePresetsToFile(const FString& FilePath)
{
	try
	{
		// 절대 경로 구성 (EngineDataPath 사용)
		UPathManager& PathManager = UPathManager::GetInstance();
		path FullPath = PathManager.GetEngineDataPath() / FilePath;

		UE_LOG("TransitionPresetManager: Saving to %s", FullPath.string().c_str());

		// JSON 구성
		json::JSON RootJSON;
		json::JSON PresetArray = json::Array();

		for (const auto& Pair : PresetMap)
		{
			json::JSON PresetJSON;
			FTransitionPresetData Preset = Pair.second;
			Preset.Serialize(false, PresetJSON);
			PresetArray.append(PresetJSON);
		}

		RootJSON["Presets"] = PresetArray;

		// 파일 쓰기
		std::ofstream File(FullPath.string());
		if (!File.is_open())
		{
			UE_LOG_ERROR("TransitionPresetManager: Failed to open file for writing: %s", FullPath.string().c_str());
			return false;
		}

		File << RootJSON.dump(4);  // 4 spaces indentation for readability
		File.close();

		UE_LOG_SUCCESS("TransitionPresetManager: Saved %d presets to %s", static_cast<int32>(PresetMap.size()), FullPath.string().c_str());
		return true;
	}
	catch (const std::exception& e)
	{
		UE_LOG_ERROR("TransitionPresetManager: Exception while saving presets: %s", e.what());
		return false;
	}
}

FTransitionPresetData* UTransitionPresetManager::FindPreset(FName PresetName)
{
	auto It = PresetMap.find(PresetName);
	if (It != PresetMap.end())
	{
		return &It->second;
	}
	return nullptr;
}

void UTransitionPresetManager::AddPreset(const FTransitionPresetData& Preset)
{
	PresetMap[Preset.PresetName] = Preset;
	UE_LOG("TransitionPresetManager: Added preset '%s'", Preset.PresetName.ToString().c_str());
}

bool UTransitionPresetManager::RemovePreset(FName PresetName)
{
	auto It = PresetMap.find(PresetName);
	if (It != PresetMap.end())
	{
		PresetMap.erase(It);
		UE_LOG("TransitionPresetManager: Removed preset '%s'", PresetName.ToString().c_str());
		return true;
	}
	return false;
}

TArray<FName> UTransitionPresetManager::GetAllPresetNames() const
{
	TArray<FName> Names;
	for (const auto& Pair : PresetMap)
	{
		Names.push_back(Pair.first);
	}
	return Names;
}

void UTransitionPresetManager::ClearAllPresets()
{
	PresetMap.clear();
	UE_LOG("TransitionPresetManager: Cleared all presets");
}
