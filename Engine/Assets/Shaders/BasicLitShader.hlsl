// BasicLitShader.hlsl

cbuffer Constants : register(b0)
{
    float4x4 objToProj;
    float4x4 objToWorld;
    float4 color;
    float3 worldLightDir;
    float3 lightColor;
    float3 ambientLightColor;
    float lightIntensity;
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
    output.normal = normalize(mul(objToWorld, float4(input.normal, 0.0)).xyz);
    
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float cosAngle = max(dot(input.normal, worldLightDir), 0.0);
    float4 lightContribution = float4((ambientLightColor + lightColor * cosAngle) * lightIntensity, 1.0);

    return color * lightContribution;
}