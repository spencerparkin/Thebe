// BasicShader.hlsl

cbuffer Constants : register(b0)
{
    float4x4 objToProj;
    float pad[192];
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
    output.normal = float3(0.0, 0.0, 0.0);
    
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return float4(1.0, 1.0, 1.0, 1.0);
}