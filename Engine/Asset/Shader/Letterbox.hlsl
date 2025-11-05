#include "PostProcessing.hlsli"

cbuffer PostProcessParams : register(b0)
{
    float3 LetterboxColor;
    float Pad;
    float4 LetterboxRect; // (MinX, MinY, MaxX, MaxY): [0.0, 1.0]
}

float4 mainPS(PS_INPUT Input) : SV_Target
{
    float2 SceneTexUV = GetSceneColorUV(Input.Position.xy);

    if (Input.TexCoord.x < LetterboxRect.x ||
        Input.TexCoord.x > LetterboxRect.z ||
        Input.TexCoord.y < LetterboxRect.y ||
        Input.TexCoord.y > LetterboxRect.w)
    {
        return float4(LetterboxColor, 1.0f);
    }
    else
    {
        return SceneTexture.Sample(SceneSampler, SceneTexUV);
    }
}