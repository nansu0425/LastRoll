#include "PostProcessing.hlsli"
Texture2D Texture : register(t1);
cbuffer PostProcessParams : register(b0)
{
    float4 Color;
}
float4 mainPS(PS_INPUT Input) : SV_Target
{
    float2 SceneTexUV = GetSceneColorUV(Input.Position.xy);

    float4 SceneColor = SceneTexture.Sample(SceneSampler, SceneTexUV);
    if(Input.TexCoord.x > 0.4f && Input.TexCoord.x < 0.6f && Input.TexCoord.y > 0.1f && Input.TexCoord.y < 0.5f)
    {
        //0.4~0.6 ->0~1
        float2 uv = float2((Input.TexCoord.x - 0.4) * 5, (Input.TexCoord.y - 0.1f) * 2.5f);
        float4 TexColor = Texture.Sample(SceneSampler, uv);
        return TexColor * Color.a + SceneColor * (1 - Color.a);
    }
    else
    {
        return SceneColor;
    }
    
}