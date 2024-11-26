// BasicShader.hlsl

cbuffer Constants : register(b0)
{
    float4x4 objToProj;
    float pad[192];
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
};

PSInput VSMain(float4 position : POSITION, float3 normal : NORMAL)
{
    PSInput output;
    
    output.position = mul(objToProj, position);
    output.normal = mul(objToProj, float4(normal.xyz, 0.0)).xyz;
    
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return float4(input.normal.x, input.normal.y, input.normal.z, 1.0);
}