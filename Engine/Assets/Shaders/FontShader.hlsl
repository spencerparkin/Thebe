// FontShader.hlsl

Texture2D atlasTexture : register(t0);
SamplerState atlasSampler : register(s0);

cbuffer Constants : register(b0)
{
    float4x4 objToProj;
    float3 textColor;
};

struct VS_Input
{
    float3 objPos : POSITION;
    float2 texCoord : TEXCOORD;
};

struct VS_Output
{
    float4 projPos : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

//----------------------------- VS_Main -----------------------------

VS_Output VS_Main(VS_Input input)
{
    VS_Output output;
    output.projPos = mul(objToProj, float4(input.objPos, 1.0f));
    output.texCoord = input.texCoord;
    return output;
}

//----------------------------- PS_Main -----------------------------

float4 PS_Main(VS_Output input) : SV_TARGET
{
    float alpha = atlasTexture.Sample(atlasSampler, input.texCoord).a;
    alpha = clamp(alpha, 0.0f, 1.0f);
    float4 color = float4(textColor, alpha);
    return color;
}