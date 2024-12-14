// PBRShader.hlsl

cbuffer Constants : register(b0)
{
    float4x4 objToProj;
    float4x4 objToWorld;
    float3 worldViewPos;
    float3 worldLightPos;
    float3 worldLightDir;
    float3x3 shadowMatrix;
    float shadowVolumeExtent;
    float lightDistanceInfinite;        // This is a binary (0 or 1) value.
    float3 lightColor;
};

static const float PI = 3.1415926536;

SamplerState generalSampler : register(s0);

Texture2D albedoTexture : register(t0);
Texture2D metalicTexture : register(t1);
Texture2D roughnessTexture : register(t2);
Texture2D normalTexture : register(t3);
Texture2D shadowTexture : register(t4);

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

// This is the Trowbridge-Reitz GGX normal distribution function.
float NormalDistribution(float halfwayVectorDotSurfaceNormal, float roughness)
{
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float denomFactor = halfwayVectorDotSurfaceNormal * halfwayVectorDotSurfaceNormal * (alpha2 - 1.0) + 1.0;
    return alpha2 / (PI * denomFactor * denomFactor);
}

// This is Smith's helper method for getting the Schlick GGX approximation.
float GeometryHelper(float dotProduct, float roughness)
{
    float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    return dotProduct / (dotProduct * (1.0 - k) + k);
}

// This is the Schlick GGX approximation.
float Geometry(float viewDirDotSurfaceNormal, float lightDirDotSurfaceNormal, float roughness)
{
    return GeometryHelper(viewDirDotSurfaceNormal, roughness) * GeometryHelper(lightDirDotSurfaceNormal, roughness);
}

// This is the Schlick approximation to the Frenel function.  Note that metalness is typically 0.0 or 1.0, but nothing inbetween.
float3 Fernel(float halfwayVectorDotViewDir, float3 baseColor, float metalness)
{
    // We use 0.04 as an average base reflectivity for all dielectrics (non-metals.)
    float3 F0 = float3(0.04, 0.04, 0.04);
    
    // Calculate the base reflectivity of the surface.  For metals, this is just the base color.
    F0 = lerp(F0, baseColor, metalness);        // TODO: Should we scale the base color by an ambient light color here or an environment map color here instead?
    
    // Approximate the Fernel effect.  Things viewed from lower angles of incidence have higher reflectivity, I think.
    return F0 + (1.0 - F0) * pow(clamp(1.0 - halfwayVectorDotViewDir, 0.0, 1.0), 5.0);
}

// See: https://learnopengl.com/PBR/Theory
float4 PSMain(PSInput input) : SV_TARGET
{
    // Calculate some stuff we'll need later.
    float3 unitWorldLightDir = normalize(lerp(worldLightPos - input.worldPosition, worldLightDir, lightDistanceInfinite));
    float3 unitWorldViewDir = normalize(worldViewPos - input.worldPosition);
    float roughness = roughnessTexture.Sample(generalSampler, input.texCoords).r;
    float metalness = metalicTexture.Sample(generalSampler, input.texCoords).r;
    float3 baseColor = albedoTexture.Sample(generalSampler, input.texCoords).rgb;
    
    // Calculate our surface normal using the normal map.
#if true
    float3 worldBinormal = cross(normalize(input.worldNormal), normalize(input.worldTangent));
    float3x3 tanToWorld = float3x3(input.worldTangent, worldBinormal, input.worldNormal);
    float3 unitTanSurfaceNormal = normalize(normalTexture.Sample(generalSampler, input.texCoords).xyz);
    float3 unitWorldSurfaceNormal = mul(unitTanSurfaceNormal, tanToWorld);
#else
    float3 unitWorldSurfaceNormal = normalize(input.worldNormal);
#endif
    
    // Pre-compute some dot-products that we'll use later.
    float3 halfwayVector = normalize(unitWorldLightDir + unitWorldViewDir);
    float halfwayVectorDotSurfaceNormal = max(dot(halfwayVector, unitWorldSurfaceNormal), 0.0);
    float halfwayVectorDotViewDir = max(dot(halfwayVector, unitWorldViewDir), 0.0);
    float viewDirDotSurfaceNormal = max(dot(unitWorldViewDir, unitWorldSurfaceNormal), 0.0);
    float lightDirDotSurfaceNormal = max(dot(unitWorldLightDir, unitWorldSurfaceNormal), 0.0);
    
    // Calculate our light intensity.
    // TODO: For spot-lights, an inverse square law would need to be taken into account here.
    float3 lightIntensity = 5.0 * lightColor;      // TODO: Not sure what the scale of the light intensity is.  What is it?
    
    // Calculate statistical percentage of surface area in the fragment containing microfacets that alignw ith the half-way vector.
    // These facets are going to contribute most to the specular highlight on the surface.
    float D = NormalDistribution(halfwayVectorDotSurfaceNormal, roughness);
    
    // Calculate statistical percentage of light rays occluded by microfacets, or microfacets shadowed by other microfacets.
    float G = Geometry(viewDirDotSurfaceNormal, lightDirDotSurfaceNormal, roughness);
    
    // Calculate the percentage of the light that is reflected (as apposed to refracted).
    float3 F = Fernel(halfwayVectorDotViewDir, baseColor, metalness);
    
    // Calculate the Cook-Torrance BRDF.  Note that the F and 1-F terms give us the conservation of energy property.
    float3 diffusePart = (baseColor / PI) * (float3(1.0, 1.0, 1.0) - F) * (1.0 - metalness); // Metals are just reflective.
    float3 specularPart = D * F * G / (4.0 * max(viewDirDotSurfaceNormal * lightDirDotSurfaceNormal, 0.001));
    float3 visibleColor = (diffusePart + specularPart) * lightIntensity * lightDirDotSurfaceNormal;
    
 #if false
    // Gamma correction?
    float gamma = 1.0 / 2.2;
    visibleColor.r = clamp(pow(visibleColor.r, gamma), 0.0, 1.0);
    visibleColor.g = clamp(pow(visibleColor.g, gamma), 0.0, 1.0);
    visibleColor.b = clamp(pow(visibleColor.b, gamma), 0.0, 1.0);
#endif
    
    // TODO: I think the metalic parts will look wrong until there's either some ambient
    //       light added to the scene or I'm doing some sort of environment mapping.
    
#if false
    float shadowFactor = 1.0;
    if (lightDistanceInfinite == 1.0)
    {
        float distanceToLightPlane = dot(worldLightPos - input.worldPosition, unitWorldLightDir);
        float3 projectedPoint = input.worldPosition + distanceToLightPlane * unitWorldLightDir;
        float3 projectedVector = projectedPoint - worldLightPos;
        float2 shadowUV = mul(shadowMatrix, projectedVector).xy;
        if(0.0 <= shadowUV.x && shadowUV.x <= 1.0 && 0.0 <= shadowUV.y && shadowUV.y <= 1.0)
        {
            // TODO: If we multi-sample here, maybe we could get softwer shadows with a shadow factor in [0,1].
            float depth = shadowTexture.Sample(generalSampler, shadowUV).r;
            float nearestDistance = depth * shadowVolumeExtent;
            float eps = 1e-2;
            if(distanceToLightPlane < nearestDistance - eps)
            {
                shadowFactor = 0.0;
            }
        }
    }
    
    visibleColor *= shadowFactor;
#endif
    
    return float4(visibleColor, 1.0);
}