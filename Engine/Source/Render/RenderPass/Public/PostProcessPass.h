#pragma once

#include "RenderPass.h"

class UDeviceResources;

/**
 * @brief 정점 3개(SV_VertexID)를 이용해 풀스크린 삼각형을 그리는 포스트 프로세스 패스의 기반 클래스이다.
 * @note 템플릿 메서드 패턴을 사용한다.
 * 파생 클래스는 UpdateConstants()와 SetRenderTargets()를 재정의하여 세부 동작을 구현해야 한다.
 * @note 쉐이더를 작성할 때 첫번째 슬롯에 씬 SRV를 바인딩해야한다.
 */
class FPostProcessPass
{
public:
    FPostProcessPass(UPipeline* InPipeline, UDeviceResources* InDeviceResources);

    virtual ~FPostProcessPass();

    /**
     * @brief 포스트 프로세스 패스의 고정된 실행 알고리즘을 정의한다.
     * @note final 함수이다. 파생 클래스는 이 함수 대신 UpdateConstants()와 SetRenderTargets()를 재정의해야 한다.
     */
    void ExecutePP(FRenderingContext& Context, const uint32 PPIdx);

    virtual void Release();

protected:
    /**
     * @brief 상수 버퍼를 업데이트하고 GPU에 바인딩한다.
     * @param Context 렌더링 컨텍스트 (카메라 정보, PostProcessSettings 등 포함)
     */
    virtual void UpdateConstants(const FRenderingContext& Context);

    virtual void SetShaderResourcesViews(const uint32 PPIdx);
    
    virtual void SetRenderTargets(const uint32 PPIdx);
    
    UPipeline* Pipeline;

    UDeviceResources* DeviceResources = nullptr;

    Microsoft::WRL::ComPtr<ID3D11VertexShader> VertexShader = nullptr;

    Microsoft::WRL::ComPtr<ID3D11PixelShader> PixelShader = nullptr;

    Microsoft::WRL::ComPtr<ID3D11SamplerState> SamplerState = nullptr;
};
