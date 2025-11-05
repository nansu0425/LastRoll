#include "pch.h"
#include "Global/Public/TransitionTypes.h"
#include <json.hpp>

FTransitionPresetData::FTransitionPresetData()
	: Duration(1.0f)
	, bUseTimingCurve(true)
	, TimingCurve(FCubicBezierCurve::CreateLinear())
{
}

void FTransitionPresetData::Serialize(bool bIsLoading, JSON& InOutHandle)
{
	if (bIsLoading)
	{
		// Load from JSON
		if (InOutHandle.hasKey("PresetName"))
		{
			PresetName = FName(InOutHandle["PresetName"].ToString());
		}

		if (InOutHandle.hasKey("Duration"))
		{
			Duration = InOutHandle["Duration"].ToFloat();
		}

		if (InOutHandle.hasKey("UseTimingCurve"))
		{
			bUseTimingCurve = InOutHandle["UseTimingCurve"].ToBool();
		}

		if (InOutHandle.hasKey("TimingCurve"))
		{
			TimingCurve.Serialize(true, InOutHandle["TimingCurve"]);
		}
	}
	else
	{
		// Save to JSON
		InOutHandle["PresetName"] = PresetName.ToString();
		InOutHandle["Duration"] = Duration;
		InOutHandle["UseTimingCurve"] = bUseTimingCurve;

		JSON CurveHandle;
		TimingCurve.Serialize(false, CurveHandle);
		InOutHandle["TimingCurve"] = CurveHandle;
	}
}
