cbuffer PostProcessParams : register(b0)
{
    float3 VignetteColor;    // 비녜트 색상
    float VignetteIntensity; // 비녜트 강도 (0.0 - 1.0)
}

Texture2D SceneTexture : register(t0); // 씬 컬러

SamplerState SceneSampler : register(s0);

struct PS_INPUT
{
    float4 Position : SV_Position;
    float2 TexCoord : TEXCOORD0;
};

PS_INPUT mainVS(uint VertexID : SV_VertexID)
{
    PS_INPUT Output;

    // 0 -> (0, 0)
    // 1 -> (2, 0)
    // 2 -> (0, 2)
    Output.TexCoord = float2((VertexID << 1) & 2, (VertexID & 2));

    // (0, 0) -> (-1, 1)
    // (2, 0) -> ( 3, 1)
    // (0, 2) -> (-1,-3)
    Output.Position = float4(Output.TexCoord * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);

    return Output;
}

float4 mainPS(PS_INPUT Input) : SV_Target
{
    float4 SceneColor = SceneTexture.Sample(SceneSampler, Input.TexCoord);

    float2 TexCoord = Input.TexCoord - 0.5f;

    float VignetteMask = dot(TexCoord, TexCoord) * 2.0f;

    float VignetteFactor = saturate(VignetteMask * VignetteIntensity);
    
    float3 FinalColor = lerp(SceneColor.rgb, VignetteColor.rgb, VignetteFactor);
    
    return float4(FinalColor, SceneColor.a);
}