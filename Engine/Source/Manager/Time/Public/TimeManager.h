#pragma once
#include "Core/Public/Object.h"

using std::chrono::high_resolution_clock;

UCLASS()
class UTimeManager :
	public UObject
{
	GENERATED_BODY()
	DECLARE_SINGLETON_CLASS(UTimeManager, UObject)

public:
	void Update();

	// Getter & Setter
	/** @brief Dilation이 적용되지않은 실제 FPS를 반환한다. */
	float GetFPS() const { return (UndilatedDeltaTime > 0.0f) ? (1.0f / UndilatedDeltaTime) : 0.0f; }

	/** @brief Dilation이 적용된 델타 타임을 반환한다. */
	float GetDeltaTime() const { return bIsPaused ? 0.0f : DeltaTime; }

	/** @brief Dilation이 적용되지않은 델타 타임을 반환한다. */
	float GetUndilatedDeltaTime() const { return UndilatedDeltaTime; }

	/** @brief Dilation이 적용된 총 게임 시간을 반환한다. */
	float GetGameTime() const { return GameTime; }
	
	bool IsPaused() const { return bIsPaused; }

	float GetTimeDilation() const { return TimeDilation; }
	
	void SetDeltaTime(float InDeltaTime)
	{
		UndilatedDeltaTime = InDeltaTime;
		
		DeltaTime = InDeltaTime * TimeDilation;
	}

	void SetTimeDilation(float InTimeDilation) { TimeDilation = (InTimeDilation < 0.0f) ? 0.0f : InTimeDilation; }

	void PauseGame() { bIsPaused = true; }
	
	void ResumeGame() { bIsPaused = false; }

private:
	/** Dilation이 적용된 총 게임 시간 */
	float GameTime;

	/** Dilation이 적용된 델타 타임 */
	float DeltaTime;

	/** Dilation이 적용되지않은 델타 타임 */
	float UndilatedDeltaTime;

	float TimeDilation;

	bool bIsPaused;

	void Initialize();
	
	void CalculateFPS();
};
