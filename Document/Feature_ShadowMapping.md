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

씬 전체를 하나의 직교 투영으로 덮는 가장 단순하고 안정적인 방식입니다. 에디터에서는 이 uniform 방식과 팀원이 구현한 CSM 중 하나를 광원별로 선택할 수 있습니다 ([`DirectionalLightComponent.h`](../Engine/Source/Component/Public/DirectionalLightComponent.h)의 `EShadowProjectionMode`).

## Spot — cone frustum perspective

Spot light는 광원 자체가 시점이 됩니다. FOV를 `OuterConeAngle × 2`로, far plane을 `AttenuationRadius`로 잡은 perspective 투영이라 light frustum이 곧 조명 범위와 일치합니다. 퇴화 케이스(각도 0/180° 근처, far ≤ near)는 clamp로 방어합니다.

## Point — cube shadow map + linear distance

Point light는 전방향이므로 +X/−X/+Y/−Y/+Z/−Z 6개 면을 90° FOV로 각각 렌더링합니다 (면별 up 벡터를 따로 둬서 gimbal 문제 회피).

여기서 depth를 일반 shadow map처럼 투영 깊이로 저장하면, 샘플링할 때 "이 픽셀이 어느 면의 어떤 투영을 거쳤는지"를 알아야 비교할 수 있습니다. 대신 **광원으로부터의 linear distance를 `AttenuationRadius`로 정규화해 저장**하는 전용 pixel shader를 씁니다. 샘플링 쪽은 면 판별 없이 `distance(pixel, light) / range`와 저장값을 직접 비교하면 됩니다.

## Shadow acne와 rasterizer state 캐싱

Shadow acne(자기 그림자 아티팩트)는 rasterizer의 `DepthBias` + `SlopeScaledDepthBias`로 처리하는데, 이 값들은 rasterizer state 생성 시점에 고정되고 광원마다 튜닝 값이 다릅니다. 매 프레임 state를 만들고 버리지 않도록, 광원별로 한 번 생성한 state를 캐싱해 재사용하는 구조를 도입했습니다.

## 한계

- Uniform directional shadow map은 씬 전체 AABB를 한 장에 담으므로, 씬이 커질수록 텍셀 밀도가 떨어져 그림자 가장자리가 계단화됩니다. 거리별로 해상도를 배분하는 CSM(팀원 작업)이 이 문제의 해법이고, 에디터에서 모드를 전환해 비교할 수 있습니다.
- Point light shadow의 near plane이 1.0 고정이라, 광원에 극단적으로 가까운 캐스터는 잘릴 수 있습니다.

## 당시 작업 문서

구현하면서 쓴 광원별 상세 문서입니다.

- [DirectionalLight_ShadowMap.md](DirectionalLight_ShadowMap.md)
- [SpotLight_ShadowMap.md](SpotLight_ShadowMap.md)
- [PointLight_ShadowMap.md](PointLight_ShadowMap.md)
