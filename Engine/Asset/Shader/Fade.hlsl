#include "PostProcessing.hlsli"

cbuffer PostProcessParams : register(b0)
{
    float3 FadeColor;
    float FadeAlpha; // 0.0 - 씬 / 1.0 - 페이드 색상
}

float4 mainPS(PS_INPUT Input) : SV_Target
{
    float2 SceneTexUV = GetSceneColorUV(Input.Position.xy);

    float4 SceneColor = SceneTexture.Sample(SceneSampler, SceneTexUV);

    float3 FinalColor = lerp(SceneColor.rgb, FadeColor.rgb, FadeAlpha);

    return float4(FinalColor, SceneColor.a);
}