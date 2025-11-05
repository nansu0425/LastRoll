#pragma once

#include "Core/Public/Object.h"
#include "Global/Public/CameraShakeTypes.h"

/**
 * @brief Camera Shake Preset 관리 싱글톤
 *
 * 재사용 가능한 Camera Shake Preset을 관리합니다.
 * Preset은 JSON 파일로 저장/로드되며, FName으로 참조됩니다.
 *
 * 사용 예시:
 * ```cpp
 * UCameraShakePresetManager& Manager = UCameraShakePresetManager::GetInstance();
 *
 * // Preset 추가
 * FCameraShakePresetData ExplosionPreset;
 * ExplosionPreset.PresetName = FName("Explosion");
 * ExplosionPreset.Duration = 2.0f;
 * ExplosionPreset.LocationAmplitude = 30.0f;
 * Manager.AddPreset(ExplosionPreset);
 *
 * // Preset 찾기
 * FCameraShakePresetData* Preset = Manager.FindPreset(FName("Explosion"));
 * if (Preset)
 * {
 *     // Preset 사용
 * }
 *
 * // 저장/로드
 * Manager.SavePresetsToFile("Engine/Data/CameraShakePresets.json");
 * Manager.LoadPresetsFromFile("Engine/Data/CameraShakePresets.json");
 * ```
 */
UCLASS()
class UCameraShakePresetManager : public UObject
{
	GENERATED_BODY()
	DECLARE_SINGLETON_CLASS(UCameraShakePresetManager, UObject)

public:
	/**
	 * @brief 초기화 - 기본 Preset 추가
	 *
	 * 엔진 시작 시 호출되어 기본 Preset들을 생성합니다.
	 * (Explosion, Collision, Earthquake 등)
	 */
	void Initialize();

	// ===== Preset 파일 관리 =====

	/**
	 * @brief JSON 파일에서 Preset 로드
	 *
	 * 기존 Preset은 유지되며, 파일에서 로드된 Preset이 추가/업데이트됩니다.
	 *
	 * @param FilePath JSON 파일 경로 (예: "Engine/Data/CameraShakePresets.json")
	 * @return true면 성공, false면 실패
	 */
	bool LoadPresetsFromFile(const FString& FilePath);

	/**
	 * @brief 모든 Preset을 JSON 파일로 저장
	 *
	 * @param FilePath JSON 파일 경로 (예: "Engine/Data/CameraShakePresets.json")
	 * @return true면 성공, false면 실패
	 */
	bool SavePresetsToFile(const FString& FilePath);

	// ===== Preset 관리 =====

	/**
	 * @brief Preset 추가 또는 업데이트
	 *
	 * 같은 이름의 Preset이 있으면 덮어씁니다.
	 *
	 * @param Preset 추가할 Preset 데이터
	 */
	void AddPreset(const FCameraShakePresetData& Preset);

	/**
	 * @brief Preset 제거
	 *
	 * @param PresetName 제거할 Preset 이름
	 * @return true면 제거 성공, false면 존재하지 않음
	 */
	bool RemovePreset(FName PresetName);

	/**
	 * @brief Preset 찾기
	 *
	 * @param PresetName 찾을 Preset 이름
	 * @return Preset 포인터 (없으면 nullptr)
	 */
	FCameraShakePresetData* FindPreset(FName PresetName);

	/**
	 * @brief 모든 Preset 이름 가져오기
	 *
	 * Editor UI에서 Preset 목록을 표시할 때 사용합니다.
	 *
	 * @return Preset 이름 배열
	 */
	TArray<FName> GetAllPresetNames() const;

	/**
	 * @brief 모든 Preset 제거
	 *
	 * 주의: 저장되지 않은 데이터는 손실됩니다.
	 */
	void ClearAllPresets();

private:
	/**
	 * @brief Preset 저장소 (PresetName → Preset Data)
	 */
	TMap<FName, FCameraShakePresetData> Presets;

	/**
	 * @brief 기본 Preset 생성
	 *
	 * Initialize()에서 호출되어 Explosion, Collision, Earthquake 등을 생성합니다.
	 */
	void CreateDefaultPresets();
};
