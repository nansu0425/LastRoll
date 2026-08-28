# 섀도우 매핑 — Directional / Spot / Point 3종 광원

> 엔진 개발 주차 WEEK08(2025-10-24 ~ 10-30) 작업. 본인 작업 범위는 **3종 광원의 shadow map 생성 경로**(light view-projection 계산과 depth 렌더링)와 shadow map 리소스 구성입니다. Shadow filtering(VSM 계열)과 CSM(Cascaded Shadow Maps)은 팀원 작업입니다. 파일별 지분은 [Contribution.md](Contribution.md) 참고.

## 개요

FutureEngine(DirectX 11)의 렌더 패스 중 `FShadowMapPass`가 매 프레임 메인 렌더링 전에 실행되어, 그림자를 켠 광원마다 depth map을 준비합니다. 광원 타입별로 투영 방식이 다릅니다.

| 광원 | 투영 | Shadow map |
|---|---|---|
| Directional | Orthographic (uniform) | 단일 |
| Spot | Perspective (cone frustum) | 단일 |
| Point | Perspective 90° × 6 | Cube (6면) |

메인 패스(`UberLit.hlsl`)는 픽셀을 광원의 light space로 변환해 shadow map의 깊이와 비교합니다.

관련 소스: [`ShadowMapPass.cpp`](../Engine/Source/Render/RenderPass/Private/ShadowMapPass.cpp), [`ShadowMapResources.h`](../Engine/Source/Texture/Public/ShadowMapResources.h)

## Directional — 씬 AABB 기반 uniform shadow map

Directional light는 위치가 없는 평행광이라 "어디서 찍을지"를 씬에서 역산해야 합니다.

1. 보이는 모든 static mesh의 world AABB를 합쳐 씬 전체 bounding box를 구함
2. 씬 중심에서 light 방향의 반대편으로 `sceneRadius + 여유` 만큼 물러난 지점에 가상 카메라를 두고 LookAt view 행렬 생성 — light가 수직에 가까울 때는 up 벡터를 바꿔 특이점 회피
3. AABB의 8개 코너를 light view space로 변환해 orthographic 투영의 left/right/top/bottom/near/far를 정확히 씌움. near/far에는 씬 깊이에 비례한 padding을 둬서, 시야 밖에 있지만 그림자를 드리우는 캐스터가 잘리지 않게 함

`FShadowMapPass::CalculateUniformShadowMapViewProj`의 핵심부입니다:

```cpp
// 2. Light direction 기준으로 view matrix 생성
FVector SceneCenter = (MinBounds + MaxBounds) * 0.5f;
float SceneRadius = (MaxBounds - MinBounds).Length() * 0.5f;

// Light position은 scene 중심에서 light direction 반대로 충분히 멀리
FVector LightPos = SceneCenter - LightDir * (SceneRadius + 50.0f);

// Up vector 계산 (Z-Up, X-Forward, Y-Right Left-Handed 좌표계)
FVector Up = FVector(0, 0, 1);  // Z-Up
if (std::abs(LightDir.Z) > 0.99f)  // Light가 거의 수직(Z축과 평행)이면
	Up = FVector(1, 0, 0);  // X-Forward를 fallback으로

OutView = FMatrix::CreateLookAtLH(LightPos, SceneCenter, Up);

// 3. AABB를 light view space로 변환하여 orthographic projection 범위 계산
// ... (AABB 8개 코너를 OutView.TransformPosition으로 변환해 LightSpaceMin/Max 계산)

// 4. Orthographic projection 생성
// Scene 크기에 비례한 padding 사용 (씬이 크면 padding도 크게)
float SceneSizeZ = LightSpaceMax.Z - LightSpaceMin.Z;
float Padding = std::max(SceneSizeZ * 0.5f, 50.0f);  // 최소 50, 씬 깊이의 50%

float Left = LightSpaceMin.X;
float Right = LightSpaceMax.X;
float Bottom = LightSpaceMin.Y;
float Top = LightSpaceMax.Y;
float Near = LightSpaceMin.Z - Padding;
float Far = LightSpaceMax.Z + Padding;

OutProj = FMatrix::CreateOrthoLH(Left, Right, Bottom, Top, Near, Far);
```

씬 전체를 하나의 직교 투영으로 덮는 가장 단순하고 안정적인 방식입니다. 에디터에서는 이 uniform 방식과 팀원이 구현한 CSM 중 하나를 광원별로 선택할 수 있습니다 ([`DirectionalLightComponent.h`](../Engine/Source/Component/Public/DirectionalLightComponent.h)의 `EShadowProjectionMode`).

## Spot — cone frustum perspective

Spot light는 광원 자체가 시점이 됩니다. FOV를 `OuterConeAngle × 2`로, far plane을 `AttenuationRadius`로 잡은 perspective 투영이라 light frustum이 곧 조명 범위와 일치합니다. 퇴화 케이스(각도 0/180° 근처, far ≤ near)는 clamp로 방어합니다. `FShadowMapPass::CalculateSpotLightViewProj`:

```cpp
// 2. View Matrix 생성: Light 위치에서 Direction 방향으로
FVector Target = LightPos + LightDir;
// ... (up 벡터 특이점 회피는 directional과 동일)
OutView = ShadowMatrixHelper::CreateLookAtLH(LightPos, Target, Up);

// 3. Perspective Projection 생성: Cone 모양의 frustum
float FovY = Light->GetOuterConeAngle() * 2.0f;  // Full cone angle
float Aspect = 1.0f;  // Square shadow map
float Near = 1.0f;    // 너무 작으면 depth precision 문제
float Far = Light->GetAttenuationRadius();  // Light range

// FovY가 너무 작거나 크면 clamp (유효 범위: 0.1 ~ PI - 0.1)
FovY = std::clamp(FovY, 0.1f, PI - 0.1f);

// Far가 Near보다 작거나 같으면 기본값 사용
if (Far <= Near)
	Far = Near + 10.0f;

OutProj = ShadowMatrixHelper::CreatePerspectiveFovLH(FovY, Aspect, Near, Far);
```

## Point — cube shadow map + linear distance

Point light는 전방향이므로 +X/−X/+Y/−Y/+Z/−Z 6개 면을 90° FOV로 각각 렌더링합니다 (면별 up 벡터를 따로 둬서 gimbal 문제 회피). `FShadowMapPass::CalculatePointLightViewProj`:

```cpp
// 3. Perspective projection (90 degree FOV for cube faces)
FMatrix Proj = ShadowMatrixHelper::CreatePerspectiveFovLH(
	PI / 2.0f,  // 90 degrees FOV
	1.0f,       // Aspect ratio 1:1 (square)
	Near,
	Far
);

// ... (Directions[6]: +X/-X/+Y/-Y/+Z/-Z 단위 벡터)

// 5. Up vectors for each direction (avoid gimbal lock)
FVector Ups[6] = {
	FVector(0.0f, 1.0f, 0.0f),   // +X: Y-Up
	FVector(0.0f, 1.0f, 0.0f),   // -X: Y-Up
	FVector(0.0f, 0.0f, -1.0f),  // +Y: -Z-Up (looking up, so up is -Z)
	FVector(0.0f, 0.0f, 1.0f),   // -Y: +Z-Up (looking down, so up is +Z)
	FVector(0.0f, 1.0f, 0.0f),   // +Z: Y-Up
	FVector(0.0f, 1.0f, 0.0f)    // -Z: Y-Up
};

// 6. Calculate View-Projection for each face
for (int i = 0; i < 6; i++)
{
	FVector Target = LightPos + Directions[i];
	FMatrix View = ShadowMatrixHelper::CreateLookAtLH(LightPos, Target, Ups[i]);
	OutViewProj[i] = View * Proj;
}
```

여기서 depth를 일반 shadow map처럼 투영 깊이로 저장하면, 샘플링할 때 "이 픽셀이 어느 면의 어떤 투영을 거쳤는지"를 알아야 비교할 수 있습니다. 대신 **광원으로부터의 linear distance를 `AttenuationRadius`로 정규화해 저장**하는 전용 pixel shader를 씁니다. 샘플링 쪽은 면 판별 없이 `distance(pixel, light) / range`와 저장값을 직접 비교하면 됩니다.

[`LinearDepthOnly.hlsl`](../Engine/Asset/Shader/LinearDepthOnly.hlsl)의 pixel shader입니다. 현재 파일에는 이후 팀원이 얹은 VSM 확장(moment 계산·출력)이 섞여 있어 그 부분은 생략했습니다:

```hlsl
cbuffer PointLightShadowParams : register(b2)
{
    float3 LightPosition;
    float LightRange;
};

PS_OUTPUT mainPS(PS_INPUT Input)
{
    PS_OUTPUT Output;

    // 1. 선형 거리 계산 (Light -> Pixel)
    float Distance = length(Input.WorldPosition - LightPosition);

    // 2. [0, 1] 범위로 정규화
    float Depth = saturate(Distance / LightRange);

    // ... (VSM moment 계산·출력 — 팀원 작업)

    Output.Depth = Depth;

    return Output;
}
```

## Shadow acne와 rasterizer state 캐싱

Shadow acne(자기 그림자 아티팩트)는 rasterizer의 `DepthBias` + `SlopeScaledDepthBias`로 처리하는데, 이 값들은 rasterizer state 생성 시점에 고정되고 광원마다 튜닝 값이 다릅니다. 매 프레임 state를 만들고 버리지 않도록, 광원별로 한 번 생성한 state를 캐싱해 재사용하는 구조를 도입했습니다.

`FShadowMapPass::GetOrCreateRasterizerState`의 캐싱 경로입니다. bias 값을 양자화해 캐시 키를 안정화하는 부분은 이후 팀원 수정(`92d0f73`)이 반영된 현재 형태입니다:

```cpp
FLOAT QuantizedShadowBias = Quantize(InShadowBias, 0.0001f);     // 0.0001 단위
FLOAT QuantizedSlopeBias  = Quantize(InShadowSlopBias, 0.01f);   // 0.01 단위
INT DepthBias = static_cast<INT>(QuantizedShadowBias * 100000.0f);
FLOAT SlopeScaledDepthBias = QuantizedSlopeBias;

FString RasterizeMapKey = to_string(QuantizedShadowBias) + to_string(QuantizedSlopeBias);

// 이미 생성된 state가 있으면 재사용
auto It = LightRasterizerStates.find(RasterizeMapKey);
if (It != LightRasterizerStates.end())
	return It->second;

// 새로 생성
const auto& Renderer = URenderer::GetInstance();
D3D11_RASTERIZER_DESC RastDesc = {};
ShadowRasterizerState->GetDesc(&RastDesc);

RastDesc.DepthBias = DepthBias;
RastDesc.SlopeScaledDepthBias = SlopeScaledDepthBias;

ID3D11RasterizerState* NewState = nullptr;
Renderer.GetDevice()->CreateRasterizerState(&RastDesc, &NewState);

// 캐시에 저장
LightRasterizerStates[RasterizeMapKey] = NewState;

return NewState;
```

## 한계

- Uniform directional shadow map은 씬 전체 AABB를 한 장에 담으므로, 씬이 커질수록 텍셀 밀도가 떨어져 그림자 가장자리가 계단화됩니다. 거리별로 해상도를 배분하는 CSM(팀원 작업)이 이 문제의 해법이고, 에디터에서 모드를 전환해 비교할 수 있습니다.
- Point light shadow의 near plane이 1.0 고정이라, 광원에 극단적으로 가까운 캐스터는 잘릴 수 있습니다.

## 당시 작업 문서

구현하면서 쓴 광원별 상세 문서입니다.

- [DirectionalLight_ShadowMap.md](DirectionalLight_ShadowMap.md)
- [SpotLight_ShadowMap.md](SpotLight_ShadowMap.md)
- [PointLight_ShadowMap.md](PointLight_ShadowMap.md)
