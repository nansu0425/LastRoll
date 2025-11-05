#pragma once
#include "Core/Public/Object.h"
#include "Global/Public/TransitionTypes.h"

/**
 * @brief Transition Preset Manager Singleton
 *
 * Manages a library of reusable transition presets.
 * Presets are loaded from/saved to JSON files.
 *
 * Default File: Engine/Data/CameraTransitionPresets.json
 */
UCLASS()
class UTransitionPresetManager : public UObject
{
	GENERATED_BODY()
	DECLARE_SINGLETON_CLASS(UTransitionPresetManager, UObject)

private:
	TMap<FName, FTransitionPresetData> PresetMap;

public:
	/**
	 * @brief Initialize manager (load default presets)
	 */
	void Initialize();

	/**
	 * @brief Load presets from JSON file
	 *
	 * @param FilePath Absolute or relative path to preset file
	 * @return true if successful
	 */
	bool LoadPresetsFromFile(const FString& FilePath);

	/**
	 * @brief Save presets to JSON file
	 *
	 * @param FilePath Absolute or relative path to preset file
	 * @return true if successful
	 */
	bool SavePresetsToFile(const FString& FilePath);

	/**
	 * @brief Find preset by name
	 *
	 * @param PresetName Preset identifier
	 * @return Pointer to preset data, or nullptr if not found
	 */
	FTransitionPresetData* FindPreset(FName PresetName);

	/**
	 * @brief Add or update preset
	 *
	 * @param Preset Preset data to add
	 * @note If preset with same name exists, it will be replaced
	 */
	void AddPreset(const FTransitionPresetData& Preset);

	/**
	 * @brief Remove preset by name
	 *
	 * @param PresetName Preset to remove
	 * @return true if preset was found and removed
	 */
	bool RemovePreset(FName PresetName);

	/**
	 * @brief Get all preset names
	 *
	 * @return Array of preset names
	 */
	TArray<FName> GetAllPresetNames() const;

	/**
	 * @brief Clear all presets
	 */
	void ClearAllPresets();

	/**
	 * @brief Create default presets (QuickCut, SlowZoom, etc.)
	 */
	void CreateDefaultPresets();
};
