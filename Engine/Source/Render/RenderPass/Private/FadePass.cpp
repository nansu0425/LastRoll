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

void FFadePass::UpdateConstants()
{
    /**
     * @todo 하드코딩된 값, 추후 컨텍스트에서 값을 가져오는 것으로 변경할 예정
     */
    FFadeConstants FadeConstants = {};
    FadeConstants.FadeColor = FVector(0.0f, 0.0f, 1.0f);
    FadeConstants.FadeAmount = 0.5f;

    FRenderResourceFactory::UpdateConstantBufferData(FadeConstantBuffer.Get(), FadeConstants);

    Pipeline->SetConstantBuffer(0, EShaderType::PS, FadeConstantBuffer.Get());
}
