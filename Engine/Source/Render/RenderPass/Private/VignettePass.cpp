#include "pch.h"

#include "Render/RenderPass/Public/VignettePass.h"

#include "Render/Renderer/Public/RenderResourceFactory.h"

FVignettePass::FVignettePass(UPipeline* InPipeline, UDeviceResources* InDeviceResources)
    : FPostProcessPass(InPipeline, InDeviceResources)
{
    VignetteConstantBuffer = FRenderResourceFactory::CreateConstantBuffer<FVignetteConstants>();

    FRenderResourceFactory::CreateVertexShaderAndInputLayout(L"Asset/Shader/Vignette.hlsl", {}, VertexShader.ReleaseAndGetAddressOf(), nullptr);

    FRenderResourceFactory::CreatePixelShader(L"Asset/Shader/Vignette.hlsl", PixelShader.ReleaseAndGetAddressOf());
    
    SamplerState = FRenderResourceFactory::CreateSamplerState(D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP);
}

FVignettePass::~FVignettePass()
{
}

void FVignettePass::UpdateConstants()
{
    FVignetteConstants VignetteConstants = {};
    VignetteConstants.VignetteColor = FVector(0.0f, 1.0f, 1.0f);
    VignetteConstants.VignetteIntensity = 0.5f;

    FRenderResourceFactory::UpdateConstantBufferData(VignetteConstantBuffer.Get(), VignetteConstants);

    Pipeline->SetConstantBuffer(0, EShaderType::PS, VignetteConstantBuffer.Get());
}
