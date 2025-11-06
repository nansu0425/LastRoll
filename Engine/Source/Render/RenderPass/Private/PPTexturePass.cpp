#include "pch.h"

#include "Render/RenderPass/Public/PPTexturePass.h"

#include "Render/Renderer/Public/RenderResourceFactory.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Source/Texture/Public/Texture.h"
FVector4 FPPTexturePass::FadeAlpha = FVector4(1, 1, 1, 0);
void FPPTexturePass::SetFadeAlpha(float alpha)
{
    FadeAlpha = FVector4(1, 1, 1, alpha);
}
FPPTexturePass::FPPTexturePass(UPipeline* InPipeline, UDeviceResources* InDeviceResources)
    : FPostProcessPass(InPipeline, InDeviceResources)
{
    PPTextureConstantBuffer = FRenderResourceFactory::CreateConstantBuffer<FPPTextureConstant>();

    FRenderResourceFactory::CreateVertexShaderAndInputLayout(L"Asset/Shader/PPTexture.hlsl", {}, VertexShader.ReleaseAndGetAddressOf(), nullptr);

    FRenderResourceFactory::CreatePixelShader(L"Asset/Shader/PPTexture.hlsl", PixelShader.ReleaseAndGetAddressOf());

    SamplerState = FRenderResourceFactory::CreateSamplerState(D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP);
}

FPPTexturePass::~FPPTexturePass()
{

}

void FPPTexturePass::UpdateConstants(const FRenderingContext& Context)
{
    if (FadeAlpha.W == 1)
    {
        CurAlpha += 0.01f;
        if (CurAlpha > 1)
        {
            CurAlpha = 1;
        }
    }
    if (FadeAlpha.W == 0)
    {
        CurAlpha -= 0.01f;
        if (CurAlpha < 0)
        {
            CurAlpha = 0;
        }
    }
    // ===== PostProcessSettings에서 Vignette 설정 읽기 =====
    UTexture* Tex = UAssetManager::GetInstance().LoadTexture("Data/Icons/Team7NameCard.png");
    Pipeline->SetShaderResourceView(1, EShaderType::PS, Tex->GetTextureSRV());

    const FPostProcessSettings& PPSettings = Context.ViewInfo.PostProcessSettings;

    FPPTextureConstant VignetteConstants = {FVector4(1,1,1,CurAlpha)};

    FRenderResourceFactory::UpdateConstantBufferData(PPTextureConstantBuffer.Get(), VignetteConstants);

    Pipeline->SetConstantBuffer(0, EShaderType::PS, PPTextureConstantBuffer.Get());
}
