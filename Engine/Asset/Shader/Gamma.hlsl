#include "PostProcessing.hlsli"

float4 mainPS(PS_INPUT Input) : SV_Target
{
    float2 SceneTexUV = GetSceneColorUV(Input.Position.xy);

    float4 SceneColor = SceneTexture.Sample(SceneSampler, SceneTexUV);

    float3 GammaCorrectedColor = pow(SceneColor.rgb, 1.0f / 2.2f);
    
    return float4(GammaCorrectedColor, SceneColor.a);
}