// ShadowShader.hlsl

cbuffer Constants : register(b0)
{
    float4x4 objToProj;
};

struct VSInput
{
    float3 objPosition : POSITION;
};

struct VSOutput
{
    float4 projPosition : SV_POSITION;
};

VSOutput VSMain(VSInput input)
{
    VSOutput output;
    output.projPosition = mul(objToProj, float4(input.objPosition, 1.0f));
    return output;
}

float PSMain(VSOutput input) : SV_DEPTH
{
    return input.projPosition.z;
}