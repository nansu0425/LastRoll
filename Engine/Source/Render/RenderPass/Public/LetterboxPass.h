#pragma once

#include "PostProcessPass.h"

struct alignas(16) FLetterboxConstants
{
    FVector LetterboxColor;
    float Pad; 
    FVector4 LetterboxRect; // (MinX, MinY, MaxX, MaxY)
};

class FLetterboxPass : public FPostProcessPass
{
public:
    FLetterboxPass(UPipeline* InPipeline, UDeviceResources* InDeviceResources);

    virtual ~FLetterboxPass();

protected:
    virtual void UpdateConstants() override;

private:
    Microsoft::WRL::ComPtr<ID3D11Buffer> LetterboxConstantBuffer = nullptr;
};
