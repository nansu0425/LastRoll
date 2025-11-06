#pragma once

#include "PostProcessPass.h"
#include "Global/Vector.h"

struct alignas(16) FPPTextureConstant
{
    FVector4 Color;
};

/**
 * @brief 비녜트(Vignette) 효과를 구현하는 포스트 프로세스 패스이다.
 */
class FPPTexturePass : public FPostProcessPass
{
public:
    FPPTexturePass(UPipeline* InPipeline, UDeviceResources* InDeviceResources);

    float CurAlpha = 0;
    virtual ~FPPTexturePass();
    static FVector4 FadeAlpha;
    static void SetFadeAlpha(float alpha);
protected:
    virtual void UpdateConstants(const FRenderingContext& Context) override;

private:
    Microsoft::WRL::ComPtr<ID3D11Buffer> PPTextureConstantBuffer = nullptr;
};
