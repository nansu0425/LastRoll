#include "pch.h"
#include "Global/Public/CameraShakeTypes.h"
#include "Component/Camera/Public/CameraModifier_CameraShake.h"
#include <json.hpp>

FCameraShakePresetData::FCameraShakePresetData()
{
	// 기본값 설정
	PresetName = FName("Default");
	Duration = 1.0f;
	LocationAmplitude = 5.0f;
	RotationAmplitude = 2.0f;
	Pattern = ECameraShakePattern::Perlin;
	Frequency = 10.0f;
	bUseDecayCurve = false;
	DecayCurve = FCubicBezierCurve(); // 기본 선형 곡선
}

void FCameraShakePresetData::Serialize(bool bIsLoading, JSON& InOutHandle)
{
	if (bIsLoading)
	{
		// JSON에서 로드
		if (InOutHandle.hasKey("PresetName"))
		{
			PresetName = FName(InOutHandle["PresetName"].ToString());
		}

		if (InOutHandle.hasKey("Duration"))
		{
			Duration = static_cast<float>(InOutHandle["Duration"].ToFloat());
		}

		if (InOutHandle.hasKey("LocationAmplitude"))
		{
			LocationAmplitude = static_cast<float>(InOutHandle["LocationAmplitude"].ToFloat());
		}

		if (InOutHandle.hasKey("RotationAmplitude"))
		{
			RotationAmplitude = static_cast<float>(InOutHandle["RotationAmplitude"].ToFloat());
		}

		if (InOutHandle.hasKey("Pattern"))
		{
			FString PatternStr = InOutHandle["Pattern"].ToString();
			TOptional<ECameraShakePattern> PatternOpt = StringToEnum<ECameraShakePattern>(PatternStr.c_str());
			if (PatternOpt.has_value())
			{
				Pattern = PatternOpt.value();
			}
		}

		if (InOutHandle.hasKey("Frequency"))
		{
			Frequency = static_cast<float>(InOutHandle["Frequency"].ToFloat());
		}

		if (InOutHandle.hasKey("UseDecayCurve"))
		{
			bUseDecayCurve = InOutHandle["UseDecayCurve"].ToBool();
		}

		if (InOutHandle.hasKey("DecayCurve"))
		{
			DecayCurve.Serialize(true, InOutHandle["DecayCurve"]);
		}
	}
	else
	{
		// JSON으로 저장
		InOutHandle["PresetName"] = PresetName.ToString();
		InOutHandle["Duration"] = Duration;
		InOutHandle["LocationAmplitude"] = LocationAmplitude;
		InOutHandle["RotationAmplitude"] = RotationAmplitude;
		InOutHandle["Pattern"] = EnumToString(Pattern);
		InOutHandle["Frequency"] = Frequency;
		InOutHandle["UseDecayCurve"] = bUseDecayCurve;

		// DecayCurve 직렬화
		JSON CurveJSON;
		DecayCurve.Serialize(false, CurveJSON);
		InOutHandle["DecayCurve"] = CurveJSON;
	}
}
