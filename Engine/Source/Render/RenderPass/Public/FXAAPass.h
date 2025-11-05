#pragma once
#include "PostProcessPass.h"

struct alignas(16) FFXAAConstants
{
    FVector2 InvResolution = FVector2();
    float FXAASpanMax = 16.0f;
    float FXAAReduceMul = 1.0f / 16.0f;
    float FXAAReduceMin = 1.0f / 256.0f;
    float Padding = 0.0f;
};

class FFXAAPass : public FPostProcessPass
{
public:
	FFXAAPass(UPipeline* InPipeline, UDeviceResources* InDeviceResources);

	/**
		* @brief FXAAPass 클래스의 소멸자입니다.
		*/
	virtual ~FFXAAPass();

protected:
	virtual void UpdateConstants() override;
private:
	Microsoft::WRL::ComPtr<ID3D11Buffer> FXAAConstantBuffer = nullptr;
	FFXAAConstants FXAAParams{};
};