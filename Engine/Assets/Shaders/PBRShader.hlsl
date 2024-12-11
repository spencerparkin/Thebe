// PBRShader.hlsl

// Helpful resource: https://www.youtube.com/watch?v=XK_p2MxGBQs

cbuffer Constants : register(b0)
{
    float4x4 objToProj;
    float4x4 objToCam;
    float3 unitLightDir;
};

SamplerState generalSampler : register(s0);

Texture2D albedoTexture : register(t0);
Texture2D metalicTexture : register(t1);
Texture2D roughnessTexture : register(t2);
Texture2D normalTexture : register(t3);

struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texCoords : TEXCOORD;
    float3 tangent : TEXCOORD1;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 texCoords : TEXCOORD;
    float3 tangent : TEXCOORD1;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    
    output.position = mul(objToProj, float4(input.position, 1.0));
    output.normal = normalize(mul(objToCam, float4(input.normal, 0.0)).xyz);
    output.tangent = normalize(mul(objToCam, float4(input.tangent, 0.0)).xyz);
    output.texCoords = input.texCoords;
    
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return albedoTexture.Sample(generalSampler, input.texCoords);
}