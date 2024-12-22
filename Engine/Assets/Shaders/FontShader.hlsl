// FontShader.hlsl

Texture2D atlasTexture : register(t0);
SamplerState atlasSampler : register(s0);

cbuffer Constants : register(b0)
{
    float4x4 objToProj;
    float3 textColor;
};

struct CharInfo
{
    float2 minUV;
    float2 maxUV;
    float2 scale;
    float2 delta;
};

StructuredBuffer<CharInfo> charInfoArray : register(t1);

struct VS_Input
{
    float2 objPos : POSITION;
};

struct VS_Output
{
    float4 projPos : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

//----------------------------- VS_Main -----------------------------

VS_Output VS_Main(VS_Input input, uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    VS_Output output;
    
    CharInfo charInfo = charInfoArray[instanceID];
    float2 objVertex = input.objPos * charInfo.scale + charInfo.delta;
    output.projPos = mul(objToProj, float4(objVertex, 0.0f, 1.0f));
    
    if(vertexID == 0 || vertexID == 3)
        output.texCoord = float2(charInfo.minUV.x, charInfo.maxUV.y);
    else if(vertexID == 1)
        output.texCoord = charInfo.maxUV;
    else if(vertexID == 2 || vertexID == 4)
        output.texCoord = float2(charInfo.maxUV.x, charInfo.minUV.y);
    else
        output.texCoord = charInfo.minUV;
    
    return output;
}

//----------------------------- PS_Main -----------------------------

float4 PS_Main(VS_Output input) : SV_TARGET
{
    float alpha = atlasTexture.Sample(atlasSampler, input.texCoord).a;
    alpha = clamp(alpha, 0.0f, 1.0f);
    float4 color = float4(textColor, alpha);
    return color;
}