#include "pch.h"

#include "Render/RenderPass/Public/GammaPass.h"

#include "Render/Renderer/Public/RenderResourceFactory.h"

FGammaPass::FGammaPass(UPipeline* InPipeline, UDeviceResources* InDeviceResources)
    : FPostProcessPass(InPipeline, InDeviceResources)
{
    FRenderResourceFactory::CreateVertexShaderAndInputLayout(L"Asset/Shader/Gamma.hlsl", {}, VertexShader.ReleaseAndGetAddressOf(), nullptr);

    FRenderResourceFactory::CreatePixelShader(L"Asset/Shader/Gamma.hlsl", PixelShader.ReleaseAndGetAddressOf());
    
    SamplerState = FRenderResourceFactory::CreateSamplerState(D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP);
}

bool FGammaPass::IsEnabled(FRenderingContext& Context) const
{
    return (Context.ShowFlags & EEngineShowFlags::SF_Gamma);
}
