#include "pch.h"

#include "Render/REnderPass/Public/PostProcessPass.h"

#include "Render/Renderer/Public/DeviceResources.h"
#include "Render/Renderer/Public/Pipeline.h"
#include "Render/Renderer/Public/Renderer.h"

FPostProcessPass::FPostProcessPass(UPipeline* InPipeline, UDeviceResources* InDeviceResources)
    : FRenderPass(InPipeline, nullptr, nullptr)
    , DeviceResources(InDeviceResources)
{
}

FPostProcessPass::~FPostProcessPass()
{
}

void FPostProcessPass::Execute(FRenderingContext& Context)
{
    SetRenderTargets();
    
    UpdateConstants();

    SetShaderResourcesViews();

    FPipelineInfo PipelineInfo = {};
    PipelineInfo.InputLayout = nullptr;
    PipelineInfo.VertexShader = VertexShader.Get();
    PipelineInfo.PixelShader = PixelShader.Get();
    PipelineInfo.DepthStencilState = nullptr;
    PipelineInfo.BlendState = nullptr;
    PipelineInfo.Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    Pipeline->UpdatePipeline(PipelineInfo);

    Pipeline->Draw(3, 0);

    ResetRenderTargets();
    
    ResetShaderResourcesViews();
}

void FPostProcessPass::Release()
{
    return;
}

void FPostProcessPass::UpdateConstants()
{
    return;
}

void FPostProcessPass::SetShaderResourcesViews()
{
    const auto& Renderer = URenderer::GetInstance();
    
    /**
     * @todo 핑퐁 렌더링 방식을 도입해야 한다. 지금 방식은 오류의 가능성이 매우 높고 확장성이 극도로 떨어진다.
     *       하지만, 나는 3인팀이고 현재 플러스 발제 중이어서 역량이 부족하여 개선이 불가능하다.
     */
    ID3D11ShaderResourceView* SRV = nullptr;
    if (Renderer.GetFXAA())
    {
        SRV = DeviceResources->GetSceneColorSRV();
    }
    else
    {
        SRV = DeviceResources->GetSceneColorShaderResourceView();
    }
    
    Pipeline->SetShaderResourceView(0, EShaderType::PS, SRV);

    Pipeline->SetSamplerState(0, EShaderType::PS, SamplerState.Get());
}

void FPostProcessPass::SetRenderTargets()
{
    const auto& Renderer = URenderer::GetInstance();

    /**
     * @todo 핑퐁 렌더링 방식을 도입해야 한다. 지금 방식은 오류의 가능성이 매우 높고 확장성이 극도로 떨어진다.
     *       하지만, 나는 3인팀이고 현재 플러스 발제 중이어서 역량이 부족하여 개선이 불가능하다.
     */
    ID3D11RenderTargetView* RTV = nullptr;
    if (Renderer.GetFXAA())
    {
        RTV = DeviceResources->GetSceneColorRenderTargetView();
    }
    else
    {
        RTV = DeviceResources->GetRenderTargetView();
    }
    
    DeviceResources->GetDeviceContext()->OMSetRenderTargets(1, &RTV, nullptr);
}

void FPostProcessPass::ResetShaderResourcesViews()
{
    Pipeline->SetShaderResourceView(0, EShaderType::PS, nullptr); 
}

void FPostProcessPass::ResetRenderTargets()
{
    /** @todo 기존 RTV 복원필요 여부 확인 */
}
