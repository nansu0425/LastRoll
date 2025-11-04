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

float2 GetSceneColorUV(float2 PosNDC)
{
    uint TexWidth, TexHeight = 0;
    SceneTexture.GetDimensions(TexWidth, TexHeight);
    float2 TexSizeRCP = float2(1 / (float) TexWidth, 1 / (float) TexHeight);
    return float2(PosNDC.x / TexWidth, PosNDC.y / TexHeight);
}