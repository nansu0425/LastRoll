#include "PostProcessing.hlsli"

float4 mainPS(PS_INPUT Input) : SV_Target
{
    float2 uv = GetSceneColorUV(Input.Position.xy);
    float4 SceneColor = SceneTexture.Sample(SceneSampler, uv);
    
    return float4(SceneColor.rgb, 1);
    //return float4(SceneColor.rgb, 1);
}