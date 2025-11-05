#include "pch.h"

#include "Render/REnderPass/Public/PostProcessPass.h"

#include "Render/Renderer/Public/DeviceResources.h"
#include "Render/Renderer/Public/Pipeline.h"
#include "Render/Renderer/Public/Renderer.h"

FPostProcessPass::FPostProcessPass(UPipeline* InPipeline, UDeviceResources* InDeviceResources)
    : Pipeline(InPipeline), DeviceResources(InDeviceResources)
{
}

FPostProcessPass::~FPostProcessPass()
{
}

void FPostProcessPass::ExecutePP(FRenderingContext& Context, const uint32 PPIdx)
{
    SetRenderTargets(PPIdx);

    UpdateConstants(Context);

    SetShaderResourcesViews(PPIdx);

    FPipelineInfo PipelineInfo = {};
    PipelineInfo.InputLayout = nullptr;
    PipelineInfo.VertexShader = VertexShader.Get();
    PipelineInfo.PixelShader = PixelShader.Get();
    PipelineInfo.DepthStencilState = nullptr;
    PipelineInfo.BlendState = nullptr;
    PipelineInfo.Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    Pipeline->UpdatePipeline(PipelineInfo);

    Pipeline->Draw(3, 0);

    ResetShaderResourceViews();
}

void FPostProcessPass::Release()
{
    return;
}

void FPostProcessPass::UpdateConstants(const FRenderingContext& Context)
{
    return;
}

void FPostProcessPass::SetShaderResourcesViews(const uint32 PPIdx)
{
    const auto& Renderer = URenderer::GetInstance();
    
    //0번째 FrameSRV 써야함
    ID3D11ShaderResourceView* SRV = DeviceResources->GetFrameShaderResourceView(PPIdx % 2 == 0);

    Pipeline->SetShaderResourceView(0, EShaderType::PS, SRV);

    Pipeline->SetSamplerState(0, EShaderType::PS, SamplerState.Get());
}

void FPostProcessPass::SetRenderTargets(const uint32 PPIdx)
{
    const auto& Renderer = URenderer::GetInstance();

    //0번째 PingPongRTV 써야함
    ID3D11RenderTargetView* RTV = DeviceResources->GetFrameRenderTargetView(PPIdx % 2 == 1);

    DeviceResources->GetDeviceContext()->OMSetRenderTargets(1, &RTV, nullptr);
}

void FPostProcessPass::ResetShaderResourceViews()
{
    Pipeline->SetShaderResourceView(0, EShaderType::PS, nullptr);
}
