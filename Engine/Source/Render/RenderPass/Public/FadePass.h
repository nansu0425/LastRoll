#pragma once

#include "PostProcessPass.h"

struct alignas(16) FFadeConstants
{
    FVector FadeColor; // 카메라 페이드 색상
    float FadeAlpha;  // 카메라 페이드 진행도 (0.0 - 씬 컬러, 1.0 - 페이드 컬러)
};

/**
 * @brief 카메라 페이드 효과를 구현하는 포스트 프로세스 패스이다.
 */
class FFadePass : public FPostProcessPass
{
public:
    FFadePass(UPipeline* InPipeline, UDeviceResources* InDeviceResources);

    virtual ~FFadePass();

protected:
    virtual void UpdateConstants(const FRenderingContext& Context) override;

private:
    Microsoft::WRL::ComPtr<ID3D11Buffer> FadeConstantBuffer;
};
