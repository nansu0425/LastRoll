#include "pch.h"

#include "Render/RenderPass/Public/FXAAPass.h"

#include "Render/Renderer/Public/RenderResourceFactory.h"

FFXAAPass::FFXAAPass(UPipeline* InPipeline, UDeviceResources* InDeviceResources)
    : FPostProcessPass(InPipeline, InDeviceResources)
{
    FXAAConstantBuffer = FRenderResourceFactory::CreateConstantBuffer<FFXAAConstants>();

    FRenderResourceFactory::CreateVertexShaderAndInputLayout(L"Asset/Shader/FXAAShader.hlsl", {}, VertexShader.ReleaseAndGetAddressOf(), nullptr);

    FRenderResourceFactory::CreatePixelShader(L"Asset/Shader/FXAAShader.hlsl", PixelShader.ReleaseAndGetAddressOf());

    SamplerState = FRenderResourceFactory::CreateSamplerState(D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP);
}

FFXAAPass::~FFXAAPass()
{

}

void FFXAAPass::UpdateConstants(const FRenderingContext& Context)
{
    const D3D11_VIEWPORT& VP = DeviceResources->GetViewportInfo();
    FXAAParams.InvResolution = FVector2(1.0f / VP.Width, 1.0f / VP.Height);

    // FXAA 품질 설정값을 명시적으로 업데이트                                           
    FXAAParams.FXAASpanMax = 8.0f;
    FXAAParams.FXAAReduceMul = 1.0f / 8.0f;
    FXAAParams.FXAAReduceMin = 1.0f / 128.0f;

    FRenderResourceFactory::UpdateConstantBufferData(FXAAConstantBuffer.Get(), FXAAParams);

    Pipeline->SetConstantBuffer(0, EShaderType::PS, FXAAConstantBuffer.Get());
}