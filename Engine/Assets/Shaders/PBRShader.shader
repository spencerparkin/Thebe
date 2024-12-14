{
    "constants": {
        "lightColor": {
            "offset": 224,
            "size": 12,
            "type": "float3"
        },
        "lightDistanceInfinite": {
            "offset": 212,
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
        "shadowMatrix": {
            "offset": 172,
            "size": 36,
            "type": "float3x3"
        },
        "shadowVolumeExtent": {
            "offset": 208,
            "size": 4,
            "type": "float"
        },
        "worldLightDir": {
            "offset": 160,
            "size": 12,
            "type": "float3"
        },
        "worldLightPos": {
            "offset": 144,
            "size": 12,
            "type": "float3"
        },
        "worldViewPos": {
            "offset": 128,
            "size": 12,
            "type": "float3"
        }
    },
    "ps_entry_point": "PSMain",
    "ps_model": "ps_6_5",
    "ps_shader_object": "Shaders/PBRShaderPS.dxbc",
    "shader_code": "Shaders/PBRShader.hlsl",
    "shdaow_map_register": 4,
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