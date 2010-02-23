cbuffer Global
{
    matrix      view;
    matrix      projection;
    float3      eye_pos;
};

cbuffer Material
{
    float4    ambient_color;
    float4    diffuse_color;
    float4    emissive_color;
    float4    specular_color;
    float material_power = 10;
};

cbuffer Object
{
    matrix    world;
};

Texture2D diffuse_texture;

float far_plane = 100;

SamplerState samLinear
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
};

struct VS_INPUT
{
    float4 Pos : POSITION;
    float3 Normal : NORMAL;
    float2 Tex : TEXCOORD;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD0;
    float3 Normal : TEXTURE1;
    float3 View : TEXTURE2;
    float3 Light : TEXTURE3;
};

PS_INPUT VS(VS_INPUT input, uniform bool use_texture)
{
    PS_INPUT output = (PS_INPUT)0;
    float4 world_pos = mul(input.Pos, world);
    float4 view_space = mul(world_pos, view);
    float4 clip_space = mul(view_space, projection);
    output.Pos = clip_space;
    output.Normal = mul(input.Normal, world);
    output.View = eye_pos - world_pos.xyz;
    output.Light = eye_pos - world_pos.xyz;

    if (use_texture) {
        output.Tex = input.Tex;
    } else {
        output.Tex = 0;
    }
    return output;
}

float4 PS(PS_INPUT input, uniform bool use_texture) : SV_Target
{
	return float4(1,1,1,1);
    float3 light = normalize(input.Light);
    float3 view = normalize(input.View);
    float3 normal = normalize(input.Normal);
    float3 halfway = normalize(light + view);
    float3 diffuse_value = saturate(dot(normal, light));
    float3 specular = pow(saturate(dot(normal, halfway)), material_power);
    
    float4 diffuse = use_texture ? diffuse_texture.Sample(samLinear, input.Tex) : diffuse_color;
    
    return float4(specular + diffuse_value * diffuse, diffuse.a);

}

technique10 render
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, VS(true) ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, PS(true) ) );
    }
}

technique10 render_no_texture
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, VS(false) ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, PS(false) ) );
    }
}
