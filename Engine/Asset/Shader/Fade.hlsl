cbuffer PostProcessParams : register(b0)
{
    float3 FadeColor;
    float FadeAmount; // 0.0 - 씬 / 1.0 - 페이드 색상
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

    float3 FinalColor = lerp(SceneColor.rgb, FadeColor.rgb, FadeAmount);

    return float4(FinalColor, SceneColor.a);
}