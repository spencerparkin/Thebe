// LineShader.hlsl

cbuffer Constants : register(b0)
{
    float4x4 worldToProj;
};

struct VSInput
{
    float3 worldPos : POSITION;
    float3 color : COLOR;
};

struct PSInput
{
    float4 projPos : SV_POSITION;
    float3 color : COLOR;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    
    output.projPos = mul(worldToProj, float4(input.worldPos, 1.0f));
    output.color = input.color;
    
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return float4(input.color, 1.0f);
}