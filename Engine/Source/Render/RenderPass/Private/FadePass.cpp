#include "pch.h"

#include "Render/RenderPass/Public/FadePass.h"

#include "Render/Renderer/Public/RenderResourceFactory.h"

FFadePass::FFadePass(UPipeline* InPipeline, UDeviceResources* InDeviceResources)
    : FPostProcessPass(InPipeline, InDeviceResources)
{
    FadeConstantBuffer = FRenderResourceFactory::CreateConstantBuffer<FFadeConstants>();
    
    FRenderResourceFactory::CreateVertexShaderAndInputLayout(L"Asset/Shader/Fade.hlsl", {}, VertexShader.ReleaseAndGetAddressOf(), nullptr);

    FRenderResourceFactory::CreatePixelShader(L"Asset/Shader/Fade.hlsl", PixelShader.ReleaseAndGetAddressOf());

    SamplerState = FRenderResourceFactory::CreateSamplerState(D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP);
}

FFadePass::~FFadePass()
{
}

void FFadePass::UpdateConstants(const FRenderingContext& Context)
{
    const FMinimalViewInfo& ViewInfo = Context.ViewInfo;
    
    FFadeConstants FadeConstants = {};
    FadeConstants.FadeColor = ViewInfo.OverlayColor;
    FadeConstants.FadeAlpha = ViewInfo.OverlayColor.W;

    FRenderResourceFactory::UpdateConstantBufferData(FadeConstantBuffer.Get(), FadeConstants);

    Pipeline->SetConstantBuffer(0, EShaderType::PS, FadeConstantBuffer.Get());
}
