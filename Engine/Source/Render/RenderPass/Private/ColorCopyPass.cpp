#include "pch.h"
#include "Render/RenderPass/Public/ColorCopyPass.h"
#include "Render/Renderer/Public/RenderResourceFactory.h"

FColorCopyPass::FColorCopyPass(UPipeline* InPipeline, UDeviceResources* InDeviceResources)
    : FPostProcessPass(InPipeline, InDeviceResources)
{
    FRenderResourceFactory::CreateVertexShaderAndInputLayout(L"Asset/Shader/ColorCopy.hlsl", {}, VertexShader.ReleaseAndGetAddressOf(), nullptr);

    FRenderResourceFactory::CreatePixelShader(L"Asset/Shader/ColorCopy.hlsl", PixelShader.ReleaseAndGetAddressOf());

    SamplerState = FRenderResourceFactory::CreateSamplerState(D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP);
}

FColorCopyPass::~FColorCopyPass()
{

}
