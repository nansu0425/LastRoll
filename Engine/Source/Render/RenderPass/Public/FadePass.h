#pragma once

#include "PostProcessPass.h"

struct alignas(16) FFadeConstants
{
    FVector FadeColor;
    float FadeAmount;
};

class FFadePass : public FPostProcessPass
{
public:
    FFadePass(UPipeline* InPipeline, UDeviceResources* InDeviceResources);

    virtual ~FFadePass();

protected:
    virtual void UpdateConstants() override;

private:
    Microsoft::WRL::ComPtr<ID3D11Buffer> FadeConstantBuffer;
};
