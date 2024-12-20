// PerfectMirror.hlsl -- The goal here is to prove we can do some kind of environment mapping, even if it's not PBR-based.

cbuffer Constants : register(b0)
{
    float4x4 objToProj;
    float4x4 objToWorld;
    float3 worldViewPos;
};

TextureCube envTexture : register(t0);
SamplerState envSampler : register(s0);

struct VSInput
{
    float3 objPos : POSITION;
    float3 objNormal : NORMAL;
};

struct PSInput
{
    float4 projPos : SV_POSITION;
    float3 worldPos : POSITION;
    float3 worldNormal : NORMAL;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    
    output.projPos = mul(objToProj, float4(input.objPos, 1.0));
    output.worldPos = mul(objToWorld, float4(input.objPos, 1.0)).xyz;
    output.worldNormal = normalize(mul(objToWorld, float4(input.objNormal, 0.0)).xyz);
    
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float3 worldViewDir = worldViewPos - input.worldPos;
    float3 worldReflectionDir = 2.0 * dot(worldViewDir, input.worldNormal) * input.worldNormal - worldViewDir;
    float3 unitWorldReflectionDir = normalize(worldReflectionDir);
    float4 color = envTexture.Sample(envSampler, unitWorldReflectionDir);
    return color;
}