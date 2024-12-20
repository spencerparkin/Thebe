{
    "constants": {
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
        "worldViewPos": {
            "offset": 128,
            "size": 12,
            "type": "float3"
        }
    },
    "ps_entry_point": "PSMain",
    "ps_model": "ps_6_5",
    "ps_shader_object": "Shaders/PerfectMirrorPS.dxbc",
    "shader_code": "Shaders/PerfectMirror.hlsl",
    "texture_register_map": {
        "env_map": 0
    },
    "vs_entry_point": "VSMain",
    "vs_model": "vs_6_5",
    "vs_shader_object": "Shaders/PerfectMirror.dxbc"
}