#include "pch.h"
#include "Render/RenderPass/Public/LightPass.h"
#include "Render/Renderer/Public/RenderResourceFactory.h"
#include "Component/Public/AmbientLightComponent.h"
#include "component/Public/DirectionalLightComponent.h"
#include "component/Public/PointLightComponent.h"
#include "Component/Public/SpotLightComponent.h"
#include "Render/RenderPass/Public/ShadowMapPass.h"
#include "Source/Editor/Public/Camera.h"
#include "Render/UI/Overlay/Public/StatOverlay.h"
#include "Actor/Public/PlayerCameraManager.h"
#include "Level/Public/World.h"

constexpr uint32 CSNumThread = 128;

FLightPass::FLightPass(UPipeline* InPipeline, ID3D11Buffer* InConstantBufferCamera) :
	FRenderPass(InPipeline, InConstantBufferCamera, nullptr)
{
	FRenderResourceFactory::CreateComputeShader(L"Asset/Shader/ViewClusterCS.hlsl", &ViewClusterCS);
	FRenderResourceFactory::CreateComputeShader(L"Asset/Shader/ClusteredLightCullingCS.hlsl", &ClusteredLightCullingCS);

	CreateOptionBuffers();
	CameraConstantBuffer = InConstantBufferCamera;
}
void FLightPass::CreateOptionBuffers()
{
	ViewClusterInfoConstantBuffer = FRenderResourceFactory::CreateConstantBuffer<FViewClusterInfo>();
	ClusterSliceInfoConstantBuffer = FRenderResourceFactory::CreateConstantBuffer<FClusterSliceInfo>();
	LightCountInfoConstantBuffer = FRenderResourceFactory::CreateConstantBuffer<FLightCountInfo>();
	GlobalLightConstantBuffer = FRenderResourceFactory::CreateConstantBuffer<FGlobalLightConstant>();
	PointLightStructuredBuffer = FRenderResourceFactory::CreateStructuredBuffer<FPointLightInfo>(PointLightBufferCount);
	SpotLightStructuredBuffer = FRenderResourceFactory::CreateStructuredBuffer<FSpotLightInfo>(SpotLightBufferCount);
	ClusterAABBRWStructuredBuffer = FRenderResourceFactory::CreateRWStructuredBuffer(24, GetClusterCount());
	PointLightIndicesRWStructuredBuffer = FRenderResourceFactory::CreateRWStructuredBuffer(4, GetClusterCount() * LightMaxCountPerCluster);
	SpotLightIndicesRWStructuredBuffer = FRenderResourceFactory::CreateRWStructuredBuffer(4, GetClusterCount() * LightMaxCountPerCluster);

	FRenderResourceFactory::CreateStructuredShaderResourceView(PointLightStructuredBuffer, &PointLightStructuredBufferSRV);
	FRenderResourceFactory::CreateStructuredShaderResourceView(SpotLightStructuredBuffer, &SpotLightStructuredBufferSRV);
	FRenderResourceFactory::CreateStructuredShaderResourceView(ClusterAABBRWStructuredBuffer, &ClusterAABBRWStructuredBufferSRV);
	FRenderResourceFactory::CreateUnorderedAccessView(ClusterAABBRWStructuredBuffer, &ClusterAABBRWStructuredBufferUAV);
	FRenderResourceFactory::CreateStructuredShaderResourceView(PointLightIndicesRWStructuredBuffer, &PointLightIndicesRWStructuredBufferSRV);
	FRenderResourceFactory::CreateStructuredShaderResourceView(SpotLightIndicesRWStructuredBuffer, &SpotLightIndicesRWStructuredBufferSRV);
	FRenderResourceFactory::CreateUnorderedAccessView(PointLightIndicesRWStructuredBuffer, &PointLightIndicesRWStructuredBufferUAV);
	FRenderResourceFactory::CreateUnorderedAccessView(SpotLightIndicesRWStructuredBuffer, &SpotLightIndicesRWStructuredBufferUAV);
}

void FLightPass::ReleaseOptionBuffers()
{
	SafeRelease(PointLightStructuredBufferSRV);
	SafeRelease(SpotLightStructuredBufferSRV);
	SafeRelease(ClusterAABBRWStructuredBufferSRV);
	SafeRelease(ClusterAABBRWStructuredBufferUAV);
	SafeRelease(PointLightIndicesRWStructuredBufferSRV);
	SafeRelease(PointLightIndicesRWStructuredBufferUAV);
	SafeRelease(SpotLightIndicesRWStructuredBufferSRV);
	SafeRelease(SpotLightIndicesRWStructuredBufferUAV);

	SafeRelease(ViewClusterInfoConstantBuffer);
	SafeRelease(ClusterSliceInfoConstantBuffer);
	SafeRelease(LightCountInfoConstantBuffer);
	SafeRelease(GlobalLightConstantBuffer);
	SafeRelease(PointLightStructuredBuffer);
	SafeRelease(SpotLightStructuredBuffer);
	SafeRelease(ClusterAABBRWStructuredBuffer);
	SafeRelease(PointLightIndicesRWStructuredBuffer);
	SafeRelease(SpotLightIndicesRWStructuredBuffer);
}
bool FLightPass::IsChangeOption()
{
	static uint32 StaticClusterSliceNumX = ClusterSliceNumX;
	static uint32 StaticClusterSliceNumY = ClusterSliceNumY;
	static uint32 StaticClusterSliceNumZ = ClusterSliceNumZ;
	static uint32 StaticLightMaxCountPerCluster = LightMaxCountPerCluster;

	if ((StaticClusterSliceNumX == ClusterSliceNumX && StaticClusterSliceNumY == ClusterSliceNumY &&
		StaticClusterSliceNumZ == ClusterSliceNumZ && StaticLightMaxCountPerCluster == LightMaxCountPerCluster) == false)
	{
		StaticClusterSliceNumX = ClusterSliceNumX;
		StaticClusterSliceNumY = ClusterSliceNumY;
		StaticClusterSliceNumZ = ClusterSliceNumZ;
		StaticLightMaxCountPerCluster = LightMaxCountPerCluster;
		return true;
	}
	return false;
}
void FLightPass::Execute(FRenderingContext& Context)
{
	if (IsChangeOption())
	{
		ReleaseOptionBuffers();
		CreateOptionBuffers();
	}
	Pipeline->SetConstantBuffer(3, EShaderType::VS | EShaderType::PS, nullptr);
	Pipeline->SetConstantBuffer(4, EShaderType::VS | EShaderType::PS, nullptr);
	Pipeline->SetConstantBuffer(5, EShaderType::VS | EShaderType::PS, nullptr);

	Pipeline->SetShaderResourceView(6, EShaderType::VS | EShaderType::PS, nullptr);
	Pipeline->SetShaderResourceView(7, EShaderType::VS | EShaderType::PS, nullptr);
	Pipeline->SetShaderResourceView(8, EShaderType::VS | EShaderType::PS, nullptr);
	Pipeline->SetShaderResourceView(9, EShaderType::VS | EShaderType::PS, nullptr);


	// Setup lighting constant buffer from scene lights
	FGlobalLightConstant GlobalLightData = {};
	TArray<FPointLightInfo> PointLightDatas;
	TArray<FSpotLightInfo> SpotLightDatas;
	// 수정 필요: Context에서 가져오기
	if (!Context.AmbientLights.empty())
	{
		UAmbientLightComponent* VisibleAmbient = nullptr;
		for (UAmbientLightComponent* Ambient : Context.AmbientLights)
		{
			if (Ambient != nullptr && Ambient->GetVisible() && Ambient->GetLightEnabled())
			{
				VisibleAmbient = Ambient;
				break;
			}
		}
		if (VisibleAmbient != nullptr)
		{
			GlobalLightData.Ambient = VisibleAmbient->GetAmbientLightInfo();
		}
	}

	if (!Context.DirectionalLights.empty())
	{
		UDirectionalLightComponent* VisibleDirectional = nullptr;
		for (UDirectionalLightComponent* Directional : Context.DirectionalLights)
		{
			if (Directional != nullptr && Directional->GetVisible() && Directional->GetLightEnabled())
			{
				VisibleDirectional = Directional;
				break;
			}
		}
		if (VisibleDirectional != nullptr)
		{
			GlobalLightData.Directional = VisibleDirectional->GetDirectionalLightInfo();
		}
	}

	// Fill point lights from scene
	int PointLightComponentCount = static_cast<int>(Context.PointLights.size());
	PointLightDatas.reserve(PointLightComponentCount);
	for (int32 i = 0; i < PointLightComponentCount; ++i)
	{
		UPointLightComponent* Light = Context.PointLights[i];
		if (!Light || !Light->GetVisible() || !Light->GetLightEnabled()) continue;
		PointLightDatas.push_back(Light->GetPointlightInfo());
	}
	// 5. Spot Lights 배열 채우기 (최대 NUM_SPOT_LIGHT개)
	int SpotLightComponentCount = static_cast<int>(Context.SpotLights.size());
	SpotLightDatas.reserve(SpotLightComponentCount);
	int CurSpotLightIdx = 0;
	for (int32 i = 0; i < SpotLightComponentCount; ++i)
	{
		USpotLightComponent* Light = Context.SpotLights[i];
		if (!Light || !Light->GetVisible() || !Light->GetLightEnabled()) continue;
		SpotLightDatas.push_back(Light->GetSpotLightInfo());
	}

	uint32 PointLightCount = static_cast<uint32>(PointLightDatas.size());
	uint32 SpotLightCount = static_cast<uint32>(SpotLightDatas.size());
	// 최대갯수 재할당
	if (PointLightBufferCount < PointLightCount)
	{
		while (PointLightBufferCount < PointLightCount)
		{
			PointLightBufferCount = PointLightBufferCount << 1;
		}
		SafeRelease(PointLightStructuredBuffer);
		PointLightStructuredBuffer = FRenderResourceFactory::CreateStructuredBuffer<FPointLightInfo>(PointLightBufferCount);
		SafeRelease(PointLightStructuredBufferSRV);
		FRenderResourceFactory::CreateStructuredShaderResourceView(PointLightStructuredBuffer, &PointLightStructuredBufferSRV);
	}
	if (SpotLightBufferCount < SpotLightCount)
	{
		while (SpotLightBufferCount < SpotLightCount)
		{
			SpotLightBufferCount = SpotLightBufferCount << 1;
		}
		SafeRelease(SpotLightStructuredBuffer);
		SpotLightStructuredBuffer = FRenderResourceFactory::CreateStructuredBuffer<FSpotLightInfo>(SpotLightBufferCount);
		SafeRelease(SpotLightStructuredBufferSRV);
		FRenderResourceFactory::CreateStructuredShaderResourceView(SpotLightStructuredBuffer, &SpotLightStructuredBufferSRV);
	}

	int ThreadGroupCount = (ClusterSliceNumX * ClusterSliceNumY * ClusterSliceNumZ + CSNumThread - 1) / CSNumThread;


	FRenderResourceFactory::UpdateConstantBufferData(GlobalLightConstantBuffer, GlobalLightData);
	FRenderResourceFactory::UpdateStructuredBuffer(PointLightStructuredBuffer, PointLightDatas);
	FRenderResourceFactory::UpdateStructuredBuffer(SpotLightStructuredBuffer, SpotLightDatas);

	// Cluster AABB Set
	// PIE/Game 모드인 경우 PlayerCameraManager의 카메라 사용, Editor 모드인 경우 EditorCamera 사용
	FMatrix ProjectionInv;
	FMatrix ViewInv;
	FMatrix ViewMatrix;
	float CamNear;
	float CamFar;
	float Aspect;
	float fov;
	uint32 Orthographic;

	if (GWorld && (GWorld->GetWorldType() == EWorldType::PIE || GWorld->GetWorldType() == EWorldType::Game))
	{
		APlayerCameraManager* CameraManager = GWorld->GetCameraManager();
		if (CameraManager)
		{
			// PlayerCameraManager에서 카메라 정보 가져오기
			const FCameraConstants& CamConstant = CameraManager->GetCameraConstants();
			const FMinimalViewInfo& POV = CameraManager->GetCameraCachePOV();

			ViewMatrix = CamConstant.View;
			ProjectionInv = CamConstant.Projection.InverseGeneral();
			ViewInv = CamConstant.View.InverseGeneral();
			CamNear = CamConstant.NearClip;
			CamFar = CamConstant.FarClip;
			Aspect = POV.AspectRatio;
			fov = POV.FOV;
			Orthographic = POV.bUsePerspectiveProjection ? 0 : 1;
		}
		else
		{
			// Fallback to EditorCamera if PlayerCameraManager doesn't exist
			FCameraConstants CamConstant = Context.CurrentCamera->GetFViewProjConstants();
			FCameraConstants CamConstantInv = Context.CurrentCamera->GetFViewProjConstantsInverse();

			ViewMatrix = CamConstant.View;
			ProjectionInv = CamConstantInv.Projection;
			ViewInv = CamConstantInv.View;
			CamNear = Context.CurrentCamera->GetNearZ();
			CamFar = Context.CurrentCamera->GetFarZ();
			Aspect = Context.CurrentCamera->GetAspect();
			fov = Context.CurrentCamera->GetFovY();
			Orthographic = ECameraType::ECT_Orthographic == Context.CurrentCamera->GetCameraType() ? 1 : 0;
		}
	}
	else
	{
		// Editor 모드인 경우 EditorCamera 사용
		FCameraConstants CamConstant = Context.CurrentCamera->GetFViewProjConstants();
		FCameraConstants CamConstantInv = Context.CurrentCamera->GetFViewProjConstantsInverse();

		ViewMatrix = CamConstant.View;
		ProjectionInv = CamConstantInv.Projection;
		ViewInv = CamConstantInv.View;
		CamNear = Context.CurrentCamera->GetNearZ();
		CamFar = Context.CurrentCamera->GetFarZ();
		Aspect = Context.CurrentCamera->GetAspect();
		fov = Context.CurrentCamera->GetFovY();
		Orthographic = ECameraType::ECT_Orthographic == Context.CurrentCamera->GetCameraType() ? 1 : 0;
	}

	FRenderResourceFactory::UpdateConstantBufferData(ViewClusterInfoConstantBuffer,
		FViewClusterInfo{ ProjectionInv, ViewInv, ViewMatrix, CamNear,CamFar,Aspect,fov});
	FRenderResourceFactory::UpdateConstantBufferData(ClusterSliceInfoConstantBuffer,
		FClusterSliceInfo{ ClusterSliceNumX, ClusterSliceNumY,ClusterSliceNumZ,LightMaxCountPerCluster,bSpotIntersectOpti, Orthographic });
	FRenderResourceFactory::UpdateConstantBufferData(LightCountInfoConstantBuffer,
		FLightCountInfo{ PointLightCount, SpotLightCount });
	Pipeline->SetConstantBuffer(0, EShaderType::CS, ViewClusterInfoConstantBuffer);
	Pipeline->SetConstantBuffer(1, EShaderType::CS, ClusterSliceInfoConstantBuffer);
	Pipeline->SetConstantBuffer(2, EShaderType::CS, LightCountInfoConstantBuffer);

	Pipeline->SetUnorderedAccessView(0, ClusterAABBRWStructuredBufferUAV);
	Pipeline->DispatchCS(ViewClusterCS, ThreadGroupCount, 1, 1);


	// Light 분류
	Pipeline->SetUnorderedAccessView(0, PointLightIndicesRWStructuredBufferUAV);
	Pipeline->SetUnorderedAccessView(1, SpotLightIndicesRWStructuredBufferUAV);
	Pipeline->SetShaderResourceView(0, EShaderType::CS, ClusterAABBRWStructuredBufferSRV);
	Pipeline->SetShaderResourceView(1, EShaderType::CS, PointLightStructuredBufferSRV);
	Pipeline->SetShaderResourceView(2, EShaderType::CS, SpotLightStructuredBufferSRV);
	Pipeline->DispatchCS(ClusteredLightCullingCS, ThreadGroupCount, 1, 1);
	Pipeline->SetUnorderedAccessView(0, nullptr);
	Pipeline->SetUnorderedAccessView(1, nullptr);
	Pipeline->SetShaderResourceView(0, EShaderType::CS, nullptr);
	Pipeline->SetShaderResourceView(1, EShaderType::CS, nullptr);
	Pipeline->SetShaderResourceView(2, EShaderType::CS, nullptr);

	// 다음 메쉬 드로우를 위한 빛 관련 정보 업로드
	Pipeline->SetConstantBuffer(3, EShaderType::VS | EShaderType::PS, GlobalLightConstantBuffer);
	Pipeline->SetConstantBuffer(4, EShaderType::VS | EShaderType::PS, ClusterSliceInfoConstantBuffer);
	Pipeline->SetConstantBuffer(5, EShaderType::VS | EShaderType::PS, LightCountInfoConstantBuffer);

	Pipeline->SetShaderResourceView(6, EShaderType::VS | EShaderType::PS, PointLightIndicesRWStructuredBufferSRV);
	Pipeline->SetShaderResourceView(7, EShaderType::VS | EShaderType::PS, SpotLightIndicesRWStructuredBufferSRV);
	Pipeline->SetShaderResourceView(8, EShaderType::VS | EShaderType::PS, PointLightStructuredBufferSRV);
	Pipeline->SetShaderResourceView(9, EShaderType::VS | EShaderType::PS, SpotLightStructuredBufferSRV);

	// 라이트 통계 기록
	uint32 DirectionalCount = 0;
	uint32 AmbientCount = 0;
	if (!Context.DirectionalLights.empty())
	{
		for (UDirectionalLightComponent* Light : Context.DirectionalLights)
		{
			if (Light && Light->GetVisible() && Light->GetLightEnabled())
			{
				++DirectionalCount;
			}
		}
	}
	if (!Context.AmbientLights.empty())
	{
		for (UAmbientLightComponent* Light : Context.AmbientLights)
		{
			if (Light && Light->GetVisible() && Light->GetLightEnabled())
			{
				++AmbientCount;
			}
		}
	}

	// 섀도우맵 메모리 및 아틀라스 정보 계산
	uint64 ShadowMapMemory = 0;
	uint64 RenderTargetMemory = 0;
	uint32 UsedAtlasTiles = 0;
	uint32 MaxAtlasTiles = 0;

	URenderer& Renderer = URenderer::GetInstance();
	FShadowMapPass* ShadowMapPass = Renderer.GetShadowMapPass();
	if (ShadowMapPass)
	{
		ShadowMapMemory = ShadowMapPass->GetTotalShadowMapMemory();
		UsedAtlasTiles = ShadowMapPass->GetUsedAtlasTileCount();
		MaxAtlasTiles = ShadowMapPass->GetMaxAtlasTileCount();
	}

	// 렌더 타겟 메모리 계산
	UDeviceResources* DeviceResources = Renderer.GetDeviceResources();
	if (DeviceResources)
	{
		RenderTargetMemory = DeviceResources->GetTotalRenderTargetMemory();
	}

	UStatOverlay::GetInstance().RecordShadowStats(DirectionalCount, PointLightCount, SpotLightCount, AmbientCount, ShadowMapMemory, RenderTargetMemory, UsedAtlasTiles, MaxAtlasTiles);
}


void FLightPass::Release()
{
	SafeRelease(ViewClusterCS);
	SafeRelease(ClusteredLightCullingCS);

	ReleaseOptionBuffers();
}