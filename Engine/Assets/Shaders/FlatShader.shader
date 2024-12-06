{
    "constants": {
        "color": {
            "offset": 128,
            "size": 16,
            "type": "float4"
        },
        "objToCam": {
            "offset": 64,
            "size": 64,
            "type": "float4x4"
        },
        "objToProj": {
            "offset": 0,
            "size": 64,
            "type": "float4x4"
        }
    },
    "ps_entry_point": "PSMain",
    "ps_model": "ps_6_5",
    "ps_shader_object": "Shaders/FlatShaderPS.dxbc",
    "shader_code": "Shaders/FlatShader.hlsl",
    "vs_entry_point": "VSMain",
    "vs_model": "vs_6_5",
    "vs_shader_object": "Shaders/FlatShaderPS.dxbc"
}