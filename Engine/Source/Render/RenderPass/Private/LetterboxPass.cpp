#include "pch.h"

#include "Render/RenderPass/Public/LetterboxPass.h"

#include "Render/Renderer/Public/RenderResourceFactory.h"

FLetterboxPass::FLetterboxPass(UPipeline* InPipeline, UDeviceResources* InDeviceResources)
    : FPostProcessPass(InPipeline, InDeviceResources)
{
    LetterboxConstantBuffer = FRenderResourceFactory::CreateConstantBuffer<FLetterboxConstants>();

    FRenderResourceFactory::CreateVertexShaderAndInputLayout(L"Asset/Shader/Letterbox.hlsl", {}, VertexShader.ReleaseAndGetAddressOf(), nullptr);

    FRenderResourceFactory::CreatePixelShader(L"Asset/Shader/Letterbox.hlsl", PixelShader.ReleaseAndGetAddressOf());

    SamplerState = FRenderResourceFactory::CreateSamplerState(D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP); 
}

FLetterboxPass::~FLetterboxPass()
{
}

void FLetterboxPass::UpdateConstants()
{
    FLetterboxConstants LetterboxConstants = {};
    LetterboxConstants.LetterboxColor = FVector(0.0f, 0.0f, 0.0f);
    /**
     * @todo 현재는 하드코딩된 값을 사용하지만, 컨텍스트에서 가져오도록 수정해야함
     */
    float TargetAspectRatio = 2.0f;
    if (TargetAspectRatio <= 0.0f)
    {
        // 유효하지 않은 종횡비가 전달될 경우 무시함
        LetterboxConstants.LetterboxRect = FVector4(0.0f, 0.0f, 1.0f, 1.0f); 
    }
    else
    {
        ID3D11DeviceContext* DeviceContext = DeviceResources->GetDeviceContext();
        
        D3D11_VIEWPORT Viewport;
        UINT NumViewport = 1;
        DeviceContext->RSGetViewports(&NumViewport, &Viewport);

        float Width = Viewport.Width;
        float Height = Viewport.Height;
        float ViewportAspectRatio = Width / Height;

        float MinX = 0.0f;
        float MinY = 0.0f;
        float MaxX = 1.0f;
        float MaxY = 1.0f;

        // --- 화면 좌우에 레터박스 생성 ---
        if (ViewportAspectRatio > TargetAspectRatio)
        {
            float TargetWidth = Height * TargetAspectRatio;
            float BarWidth = (Width - TargetWidth) / 2.0f;
            float BarRatio = BarWidth / Width;

            MinX = BarRatio;
            MaxX = 1.0f - BarRatio;
        }
        // --- 화면 상하에 레터박스 생성 ---
        else if (ViewportAspectRatio < TargetAspectRatio)
        {
            float TargetHeight = Width / TargetAspectRatio;
            float BarHeight = Height - TargetHeight;
            float BarRatio = BarHeight / Height;

            MinY = BarRatio;
            MaxY = 1.0f - BarRatio;
        }

        LetterboxConstants.LetterboxRect = FVector4(MinX, MinY, MaxX, MaxY);
    }

    FRenderResourceFactory::UpdateConstantBufferData(LetterboxConstantBuffer.Get(), LetterboxConstants);

    Pipeline->SetConstantBuffer(0, EShaderType::PS, LetterboxConstantBuffer.Get());
}
