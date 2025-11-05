#include "PostProcessing.hlsli"

cbuffer PostProcessParams : register(b0)
{
    float3 VignetteColor;    // 비녜트 색상
    float VignetteIntensity; // 비녜트 강도 (0.0 - 1.0)
}

float4 mainPS(PS_INPUT Input) : SV_Target
{
    float2 SceneTexUV = GetSceneColorUV(Input.Position.xy);

    float4 SceneColor = SceneTexture.Sample(SceneSampler, SceneTexUV);

    float2 TexCoord = Input.TexCoord - 0.5f;

    float VignetteMask = dot(TexCoord, TexCoord) * 2.0f;

    float VignetteFactor = saturate(VignetteMask * VignetteIntensity);
    
    float3 FinalColor = lerp(SceneColor.rgb, VignetteColor.rgb, VignetteFactor);
    
    return float4(FinalColor, SceneColor.a);
}