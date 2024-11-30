// BasicShader.hlsl

cbuffer Constants : register(b0)
{
    float4x4 objToProj;
    float4x4 objToCam;
};

struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    
    output.position = mul(objToProj, float4(input.position, 1.0));
    output.normal = normalize(mul(objToCam, float4(input.normal, 0.0)).xyz);
    
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return float4(1.0, 1.0, 1.0, 1.0);
}