{
    "constants": {
        "lightColor": {
            "offset": 144,
            "size": 12,
            "type": "float3"
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
        "unitWorldLightDir": {
            "offset": 128,
            "size": 12,
            "type": "float3"
        }
    },
    "ps_entry_point": "PSMain",
    "ps_model": "ps_6_5",
    "ps_shader_object": "Shaders/PBRShaderPS.dxbc",
    "shader_code": "Shaders/PBRShader.hlsl",
    "texture_register_map": {
        "albedo": 0,
        "metalic": 1,
        "normal": 3,
        "roughness": 2
    },
    "vs_entry_point": "VSMain",
    "vs_model": "vs_6_5",
    "vs_shader_object": "Shaders/PBRShaderVS.dxbc"
}