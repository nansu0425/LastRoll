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

void FVignettePass::UpdateConstants(const FRenderingContext& Context)
{
    // ===== PostProcessSettings에서 Vignette 설정 읽기 =====
    const FPostProcessSettings& PPSettings = Context.PostProcessSettings;

    FVignetteConstants VignetteConstants = {};

    // Override가 활성화된 경우만 설정 적용, 아니면 0(효과 없음)
    if (PPSettings.bOverride_VignetteColor)
    {
        VignetteConstants.VignetteColor = PPSettings.VignetteColor;
    }
    else
    {
        VignetteConstants.VignetteColor = FVector(0.0f, 0.0f, 0.0f);  // 기본값: 검은색
    }

    if (PPSettings.bOverride_VignetteIntensity)
    {
        VignetteConstants.VignetteIntensity = PPSettings.VignetteIntensity;
    }
    else
    {
        VignetteConstants.VignetteIntensity = 0.0f;  // 기본값: 효과 없음
    }

    FRenderResourceFactory::UpdateConstantBufferData(VignetteConstantBuffer.Get(), VignetteConstants);

    Pipeline->SetConstantBuffer(0, EShaderType::PS, VignetteConstantBuffer.Get());
}
