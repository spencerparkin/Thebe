{
    "constants": {
        "ambientLightColor": {
            "offset": 176,
            "size": 12,
            "type": "float3"
        },
        "color": {
            "offset": 128,
            "size": 16,
            "type": "float4"
        },
        "lightColor": {
            "offset": 160,
            "size": 12,
            "type": "float3"
        },
        "lightIntensity": {
            "offset": 188,
            "size": 4,
            "type": "float"
        },
        "objToProj": {
            "offset": 0,
            "size": 64,
            "type": "float4x4"
        },
        "objToWorld": {
            "offset": 64,
            "size": 64,
            "type": "float4x4"
        },
        "worldLightDir": {
            "offset": 144,
            "size": 12,
            "type": "float3"
        }
    },
    "ps_entry_point": "PSMain",
    "ps_model": "ps_6_5",
    "ps_shader_object": "Shaders/BasicLitShaderPS.dxbc",
    "shader_code": "Shaders/BasicLitShader.hlsl",
    "vs_entry_point": "VSMain",
    "vs_model": "vs_6_5",
    "vs_shader_object": "Shaders/BasicLitShaderVS.dxbc"
}