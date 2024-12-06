// BasicShader.hlsl

cbuffer Constants : register(b0)
{
    float4x4 objToProj;
    float4x4 objToCam;
};

Texture2D diffuseTexture : register(t0);
SamplerState generalSampler : register(s0);

struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texCoords : TEXCOORD;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 texCoords : TEXCOORD;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    
    output.position = mul(objToProj, float4(input.position, 1.0));
    output.normal = normalize(mul(objToCam, float4(input.normal, 0.0)).xyz);
    output.texCoords = input.texCoords;
    
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return diffuseTexture.Sample(generalSampler, input.texCoords);
}