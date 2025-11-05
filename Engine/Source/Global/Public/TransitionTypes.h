#pragma once

#include "Global/Types.h"
#include "Global/Public/BezierCurve.h"

namespace json { class JSON; }
using JSON = json::JSON;

/**
 * @brief Camera Transition Preset Data
 *
 * Stores reusable transition configurations.
 * Presets are saved/loaded from JSON files and managed by UTransitionPresetManager.
 *
 * Example Presets:
 * - "QuickCut": Duration=0.1s, Linear
 * - "SlowZoom": Duration=2.0s, EaseInOut
 * - "SmoothPan": Duration=1.5s, EaseOut
 * - "Cinematic": Duration=3.0s, Custom Bezier
 */
struct FTransitionPresetData
{
	/**
	 * @brief Preset name (unique identifier)
	 *
	 * Examples: "QuickCut", "SlowZoom", "SmoothPan"
	 */
	FName PresetName;

	/**
	 * @brief Transition duration (seconds)
	 *
	 * Range: 0.1 ~ 10.0
	 * Default: 1.0
	 */
	float Duration = 1.0f;

	/**
	 * @brief Use Bezier curve timing
	 *
	 * true: Use TimingCurve
	 * false: Linear interpolation
	 */
	bool bUseTimingCurve = true;

	/**
	 * @brief Bezier curve for timing control
	 *
	 * Only used if bUseTimingCurve == true.
	 * X = Normalized time [0,1]
	 * Y = Blend alpha [0,1]
	 */
	FCubicBezierCurve TimingCurve;

	/**
	 * @brief Default constructor
	 */
	FTransitionPresetData();

	/**
	 * @brief JSON serialization
	 *
	 * @param bIsLoading true = load from JSON, false = save to JSON
	 * @param InOutHandle JSON handle
	 */
	void Serialize(bool bIsLoading, JSON& InOutHandle);
};
