// PBRShader.hlsl

cbuffer Constants : register(b0)
{
    float4x4 objToProj;
    float4x4 objToWorld;
    float3 unitWorldLightDir;
    float3 lightColor;
    float3 unitWorldCamDir;
};

static const float PI = 3.1415926536;
static const float reflectance = 0.1; // TODO: We could expose this as a material parameter.

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

float3 Mix(float3 vecA, float3 vecB, float alpha)
{
    return (1.0 - alpha) * vecA + alpha * vecB;
}

float3 FresnelReflectance_SchlickApproximation(float3 halfVector, float3 baseColor, float metalness)
{
    float component = 0.16 * reflectance * reflectance;
    float3 F0 = Mix(float3(component, component, component), baseColor, metalness);
    float dotProd = max(dot(halfVector, unitWorldCamDir), 0.0);
    return F0 + (1.0 - F0) * pow(clamp(1.0 - dotProd, 0.0, 1.0), 5.0);
}

float NormalDistribution_GGX(float3 unitWorldSurfaceNormal, float3 halfVector, float roughness)
{
    float alpha = roughness * roughness;
    float dotProd = max(dot(halfVector, unitWorldSurfaceNormal), 0.0);
    float term = dotProd * dotProd * (alpha * alpha - 1.0) + 1.0;
    return alpha * alpha / (PI * term * term);
}

float GeometryTerm_SchlickGGX(float3 unitWorldSurfaceNormal, float3 unitVector, float roughness)
{
    float dotProd = max(dot(unitWorldSurfaceNormal, unitVector), 0.0);
    float k = roughness / 2.0;
    return dotProd / (dotProd * (1.0 - k) + k);
}

float GeometryTerm_Smith(float3 unitWorldSurfaceNormal, float roughness)
{
    return GeometryTerm_SchlickGGX(unitWorldSurfaceNormal, unitWorldLightDir, roughness) *
            GeometryTerm_SchlickGGX(unitWorldSurfaceNormal, unitWorldCamDir, roughness);
}

float3 CookTorranceBRDF(PSInput input, float3 unitWorldSurfaceNormal)
{
    float roughness = roughnessTexture.Sample(generalSampler, input.texCoords).r;
    float3 baseColor = albedoTexture.Sample(generalSampler, input.texCoords).rgb;
    float metalness = metalicTexture.Sample(generalSampler, input.texCoords).r;
    float3 halfVector = normalize(unitWorldCamDir + unitWorldLightDir);

    float normalDotLight = max(dot(unitWorldSurfaceNormal, unitWorldLightDir), 0.0);
    float normalDotView = max(dot(unitWorldSurfaceNormal, unitWorldCamDir), 0.0);

    float3 F = FresnelReflectance_SchlickApproximation(halfVector, baseColor, metalness);
    float D = NormalDistribution_GGX(unitWorldSurfaceNormal, halfVector, roughness);
    float G = GeometryTerm_Smith(unitWorldSurfaceNormal, roughness);

    float3 diffusePart = (baseColor / PI) * clamp(1.0 - metalness, 0.0, 1.0) * clamp(1.0 - F, 0.0, 1.0);
    float3 specularPart = F * D * G / (4.0 * max(normalDotLight * normalDotView, 0.001));
    
    return diffusePart + specularPart;
}

// See: https://www.youtube.com/watch?v=gya7x9H3mV0
float4 PSMain(PSInput input) : SV_TARGET
{
    float3 worldBinormal = cross(input.worldNormal, input.worldTangent);
    float3x3 tanToWorld = float3x3(input.worldTangent, worldBinormal, input.worldNormal);
    float3 unitTanSurfaceNormal = normalize(normalTexture.Sample(generalSampler, input.texCoords).xyz);
    float3 unitWorldSurfaceNormal = mul(unitTanSurfaceNormal, tanToWorld);
    
    // TODO: For spot-lights, an inverse square law would need to be taken into account here.
    float3 lightIntensity = lightColor;
    
    float3 visibleColor = CookTorranceBRDF(input, unitWorldSurfaceNormal) * lightIntensity * max(dot(unitWorldSurfaceNormal, unitWorldLightDir), 0.0);
    
    return float4(visibleColor, 1.0);
}