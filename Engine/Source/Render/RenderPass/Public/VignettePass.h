#pragma once

#include "PostProcessPass.h"

struct alignas(16) FVignetteConstants
{
    FVector VignetteColor;   // 비녜트 효과의 색상
    float VignetteIntensity; // 비녜트 효과의 강도
};

/**
 * @brief 비녜트(Vignette) 효과를 구현하는 포스트 프로세스 패스이다.
 */
class FVignettePass : public FPostProcessPass
{
public:
    FVignettePass(UPipeline* InPipeline, UDeviceResources* InDeviceResources);

    virtual ~FVignettePass();

protected:
    virtual void UpdateConstants() override;

private:
    Microsoft::WRL::ComPtr<ID3D11Buffer> VignetteConstantBuffer = nullptr;
};
