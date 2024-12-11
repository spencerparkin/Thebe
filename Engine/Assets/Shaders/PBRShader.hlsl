// PBRShader.hlsl

// Helpful resource: https://www.youtube.com/watch?v=XK_p2MxGBQs

cbuffer Constants : register(b0)
{
    float4x4 objToProj;
    float4x4 objToWorld;
    float3 unitWorldLightDir;
    float3 lightColor;
};

SamplerState generalSampler : register(s0);

Texture2D albedoTexture : register(t0);
Texture2D metalicTexture : register(t1);
Texture2D roughnessTexture : register(t2);
Texture2D normalTexture : register(t3);

struct VSInput
{
    float3 objPosition : POSITION;
    float3 objNormal : NORMAL;
    float3 objTangent : TEXCOORD1;
    float2 texCoords : TEXCOORD;
};

struct PSInput
{
    float4 projPosition : SV_POSITION;
    float3 worldPosition : POSITION;
    float3 worldNormal : NORMAL;
    float3 worldTangent : TEXCOORD1;
    float2 texCoords : TEXCOORD;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    
    output.projPosition = mul(objToProj, float4(input.objPosition, 1.0));
    output.worldPosition = mul(objToWorld, float4(input.objPosition, 1.0)).xyz;
    output.worldNormal = normalize(mul(objToWorld, float4(input.objNormal, 0.0)).xyz);
    output.worldTangent = normalize(mul(objToWorld, float4(input.objTangent, 0.0)).xyz);
    output.texCoords = input.texCoords;
    
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    //...

    //return albedoTexture.Sample(generalSampler, input.texCoords);
    
    return float4(1.0, 0.0, 0.0, 1.0);
}