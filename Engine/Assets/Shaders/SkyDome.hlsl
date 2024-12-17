// SkyDome.hlsl

TextureCube cubeTexture : register(t0);
SamplerState cubeSampler : register(s0);

cbuffer Constants : register(b0)
{
    float4x4 objToProj;
}

struct VSInput
{
    float3 objPos : POSITION;
};

struct VSOutput
{
    float3 objPos : POSITION;
    float4 projPos : SV_POSITION;
};

VSOutput VSMain(VSInput input)
{
    VSOutput output;
    output.objPos = input.objPos;
    output.projPos = mul(objToProj, float4(input.objPos, 1.0f));
    //output.projPos.z = 0.0;
    return output;
}

float4 PSMain(VSOutput input) : SV_TARGET
{
    float3 texCoordVec = normalize(input.objPos);
    float4 color = cubeTexture.Sample(cubeSampler, texCoordVec);
    color.a = 1.0f;
    return color;
}