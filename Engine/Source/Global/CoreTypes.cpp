#include "pch.h"
#include "Global/CoreTypes.h"
#include <json.hpp>

using JSON = json::JSON;

void FPostProcessSettings::Serialize(bool bInIsLoading, JSON& InOutHandle)
{
	if (bInIsLoading)
	{
		// BlendWeight
		if (InOutHandle.hasKey("BlendWeight"))
			BlendWeight = static_cast<float>(InOutHandle["BlendWeight"].ToFloat());

		// VignetteIntensity
		if (InOutHandle.hasKey("VignetteIntensity"))
		{
			bOverride_VignetteIntensity = true;
			VignetteIntensity = static_cast<float>(InOutHandle["VignetteIntensity"].ToFloat());
		}

		// VignetteColor
		if (InOutHandle.hasKey("VignetteColor"))
		{
			bOverride_VignetteColor = true;
			JSON ColorJSON = InOutHandle["VignetteColor"];
			VignetteColor = FVector(
				static_cast<float>(ColorJSON["X"].ToFloat()),
				static_cast<float>(ColorJSON["Y"].ToFloat()),
				static_cast<float>(ColorJSON["Z"].ToFloat())
			);
		}
	}
	else
	{
		// BlendWeight (항상 저장)
		InOutHandle["BlendWeight"] = BlendWeight;

		// VignetteIntensity (Override된 경우만)
		if (bOverride_VignetteIntensity)
			InOutHandle["VignetteIntensity"] = VignetteIntensity;

		// VignetteColor (Override된 경우만)
		if (bOverride_VignetteColor)
		{
			JSON ColorJSON;
			ColorJSON["X"] = VignetteColor.X;
			ColorJSON["Y"] = VignetteColor.Y;
			ColorJSON["Z"] = VignetteColor.Z;
			InOutHandle["VignetteColor"] = ColorJSON;
		}
	}
}
