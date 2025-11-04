#include "pch.h"

#include "Render/REnderPass/Public/PostProcessPass.h"

#include "Render/Renderer/Public/DeviceResources.h"
#include "Render/Renderer/Public/Pipeline.h"
#include "Render/Renderer/Public/Renderer.h"

FPostProcessPass::FPostProcessPass(UPipeline* InPipeline, UDeviceResources* InDeviceResources)
    : DeviceResources(InDeviceResources)
{
}

FPostProcessPass::~FPostProcessPass()
{
}

void FPostProcessPass::ExecutePP(FRenderingContext& Context, const uint32 PPIdx)
{
    SetRenderTargets(PPIdx);
    
    UpdateConstants();

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

void FPostProcessPass::SetShaderResourcesViews(const uint32 PPIdx)
{
    const auto& Renderer = URenderer::GetInstance();
    
    /**
     * @todo 핑퐁 렌더링 방식을 도입해야 한다. 지금 방식은 오류의 가능성이 매우 높고 확장성이 극도로 떨어진다.
     *       하지만, 나는 3인팀이고 현재 플러스 발제 중이어서 역량이 부족하여 개선이 불가능하다.
     */

    //0번째 FrameSRV 써야함
    ID3D11ShaderResourceView* SRV = DeviceResources->GetFrameShaderResourceView(PPIdx % 2 == 0);

    Pipeline->SetShaderResourceView(0, EShaderType::PS, SRV);

    Pipeline->SetSamplerState(0, EShaderType::PS, SamplerState.Get());
}

void FPostProcessPass::SetRenderTargets(const uint32 PPIdx)
{
    const auto& Renderer = URenderer::GetInstance();

    /**
     * @todo 핑퐁 렌더링 방식을 도입해야 한다. 지금 방식은 오류의 가능성이 매우 높고 확장성이 극도로 떨어진다.
     *       하지만, 나는 3인팀이고 현재 플러스 발제 중이어서 역량이 부족하여 개선이 불가능하다.
     */

    //0번째 PingPongRTV 써야함
    ID3D11RenderTargetView* RTV = DeviceResources->GetFrameRenderTargetView(PPIdx % 2 == 1);

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
