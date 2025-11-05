#include "pch.h"
#include "Manager/Camera/Public/CameraShakePresetManager.h"
#include "Component/Camera/Public/CameraModifier_CameraShake.h"
#include "Manager/Path/Public/PathManager.h"
#include <json.hpp>
#include <fstream>

IMPLEMENT_SINGLETON_CLASS(UCameraShakePresetManager, UObject)

UCameraShakePresetManager::UCameraShakePresetManager()
{
}

UCameraShakePresetManager::~UCameraShakePresetManager()
{
}

void UCameraShakePresetManager::Initialize()
{
	UE_LOG("CameraShakePresetManager: Initializing");

	// 기본 Preset 생성
	CreateDefaultPresets();

	UE_LOG_SUCCESS("CameraShakePresetManager: Initialized with %d presets", static_cast<int32>(Presets.size()));
}

void UCameraShakePresetManager::CreateDefaultPresets()
{
	// Explosion Preset
	FCameraShakePresetData Explosion;
	Explosion.PresetName = FName("Explosion");
	Explosion.Duration = 2.0f;
	Explosion.LocationAmplitude = 30.0f;
	Explosion.RotationAmplitude = 8.0f;
	Explosion.Pattern = ECameraShakePattern::Perlin;
	Explosion.Frequency = 10.0f;
	Explosion.bUseDecayCurve = true;
	Explosion.DecayCurve = FCubicBezierCurve::CreateEaseOut();
	AddPreset(Explosion);

	// Collision Preset
	FCameraShakePresetData Collision;
	Collision.PresetName = FName("Collision");
	Collision.Duration = 0.5f;
	Collision.LocationAmplitude = 10.0f;
	Collision.RotationAmplitude = 3.0f;
	Collision.Pattern = ECameraShakePattern::Random;
	Collision.Frequency = 10.0f;
	Collision.bUseDecayCurve = true;
	Collision.DecayCurve = FCubicBezierCurve::CreateLinear();
	AddPreset(Collision);

	// Earthquake Preset
	FCameraShakePresetData Earthquake;
	Earthquake.PresetName = FName("Earthquake");
	Earthquake.Duration = 5.0f;
	Earthquake.LocationAmplitude = 50.0f;
	Earthquake.RotationAmplitude = 5.0f;
	Earthquake.Pattern = ECameraShakePattern::Sine;
	Earthquake.Frequency = 5.0f;
	Earthquake.bUseDecayCurve = true;
	Earthquake.DecayCurve = FCubicBezierCurve::CreateEaseInOut();
	AddPreset(Earthquake);

	UE_LOG("CameraShakePresetManager: Created default presets (Explosion, Collision, Earthquake)");
}

bool UCameraShakePresetManager::LoadPresetsFromFile(const FString& FilePath)
{
	try
	{
		// 절대 경로 구성 (EngineDataPath 사용)
		UPathManager& PathManager = UPathManager::GetInstance();
		path FullPath = PathManager.GetEngineDataPath() / FilePath;

		UE_LOG("CameraShakePresetManager: Loading from %s", FullPath.string().c_str());

		// 파일 읽기
		std::ifstream File(FullPath.string());
		if (!File.is_open())
		{
			UE_LOG_ERROR("CameraShakePresetManager: Failed to open file for reading: %s", FullPath.string().c_str());
			return false;
		}

		FString FileContent((std::istreambuf_iterator<char>(File)), std::istreambuf_iterator<char>());
		File.close();

		// JSON 파싱
		json::JSON RootJSON = json::JSON::Load(FileContent);

		if (!RootJSON.hasKey("Presets"))
		{
			UE_LOG_ERROR("CameraShakePresetManager: Invalid JSON format (missing 'Presets' key)");
			return false;
		}

		// 기존 Preset 클리어
		ClearAllPresets();

		// Preset 배열 로드
		auto& PresetArray = RootJSON["Presets"];
		int32 LoadedCount = 0;

		for (size_t i = 0; i < PresetArray.length(); ++i)
		{
			FCameraShakePresetData Preset;
			Preset.Serialize(true, PresetArray[static_cast<uint32>(i)]);
			AddPreset(Preset);
			LoadedCount++;
		}

		UE_LOG_SUCCESS("CameraShakePresetManager: Loaded %d presets from %s", LoadedCount, FullPath.string().c_str());
		return true;
	}
	catch (const std::exception& e)
	{
		UE_LOG_ERROR("CameraShakePresetManager: Exception while loading presets: %s", e.what());
		return false;
	}
}

bool UCameraShakePresetManager::SavePresetsToFile(const FString& FilePath)
{
	try
	{
		// 절대 경로 구성 (EngineDataPath 사용)
		UPathManager& PathManager = UPathManager::GetInstance();
		path FullPath = PathManager.GetEngineDataPath() / FilePath;

		UE_LOG("CameraShakePresetManager: Saving to %s", FullPath.string().c_str());

		// JSON 루트 생성
		json::JSON RootJSON;
		json::JSON PresetArray = json::Array();

		// 모든 Preset 직렬화
		for (auto& Pair : Presets)
		{
			json::JSON PresetJSON;
			Pair.second.Serialize(false, PresetJSON);
			PresetArray.append(PresetJSON);
		}

		RootJSON["Presets"] = PresetArray;

		// 파일 쓰기
		std::ofstream File(FullPath.string().c_str());
		if (!File.is_open())
		{
			UE_LOG_ERROR("CameraShakePresetManager: Failed to open file for writing: %s", FullPath.string().c_str());
			return false;
		}

		File << RootJSON.dump(4); // 들여쓰기 4칸
		File.close();

		UE_LOG_SUCCESS("CameraShakePresetManager: Saved %d presets to %s", static_cast<int32>(Presets.size()), FullPath.string().c_str());
		return true;
	}
	catch (const std::exception& e)
	{
		UE_LOG_ERROR("CameraShakePresetManager: Exception while saving presets: %s", e.what());
		return false;
	}
}

void UCameraShakePresetManager::AddPreset(const FCameraShakePresetData& Preset)
{
	if (Preset.PresetName.ToString().empty())
	{
		UE_LOG_WARNING("CameraShakePresetManager: Cannot add preset with empty name");
		return;
	}

	// 기존 Preset이 있으면 업데이트
	auto It = Presets.find(Preset.PresetName);
	if (It != Presets.end())
	{
		UE_LOG("CameraShakePresetManager: Updating existing preset '%s'", Preset.PresetName.ToString().c_str());
	}
	else
	{
		UE_LOG("CameraShakePresetManager: Adding new preset '%s'", Preset.PresetName.ToString().c_str());
	}

	Presets[Preset.PresetName] = Preset;
}

bool UCameraShakePresetManager::RemovePreset(FName PresetName)
{
	auto It = Presets.find(PresetName);
	if (It != Presets.end())
	{
		Presets.erase(It);
		UE_LOG("CameraShakePresetManager: Removed preset '%s'", PresetName.ToString().c_str());
		return true;
	}

	UE_LOG_WARNING("CameraShakePresetManager: Preset '%s' not found for removal", PresetName.ToString().c_str());
	return false;
}

FCameraShakePresetData* UCameraShakePresetManager::FindPreset(FName PresetName)
{
	auto It = Presets.find(PresetName);
	if (It != Presets.end())
	{
		return &(It->second);
	}

	return nullptr;
}

TArray<FName> UCameraShakePresetManager::GetAllPresetNames() const
{
	TArray<FName> Names;
	Names.reserve(Presets.size());

	for (const auto& Pair : Presets)
	{
		Names.push_back(Pair.first);
	}

	return Names;
}

void UCameraShakePresetManager::ClearAllPresets()
{
	int32 Count = static_cast<int32>(Presets.size());
	Presets.clear();
	UE_LOG("CameraShakePresetManager: Cleared all %d presets", Count);
}
