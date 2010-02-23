cbuffer Global
{
    matrix      view;
    matrix      projection;
    float f1;
    float2 f2;
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


//---------------------------------------------------------------------------------------
// Fullscreen shader

struct VsColorInput
{
    float4 Pos : POSITION;
    float4 Color : COLOR;
};

struct PsColorInput
{
    float4 Pos : SV_POSITION;
    float4 Color : TEXTURE0;
};

PsColorInput VsColor(VsColorInput input)
{
    PsColorInput output = (PsColorInput)0;
    output.Pos = input.Pos;
    output.Color = input.Color;
    return output;
}

float4 PsColor(PsColorInput input) : SV_Target
{
    return input.Color;
}

technique10 fullscreen
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, VsColor() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, PsColor() ) );
    }
}

//---------------------------------------------------------------------------------------
struct VS_INPUT
{
    float4 Pos : POSITION;
    float3 Normal : NORMAL;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float3 Normal : TEXTURE0;
    float3 View : TEXTURE1;
    float3 Light : TEXTURE2;
};

PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT)0;
    float4 world_pos = mul(input.Pos, world);
    float4 view_space = mul(world_pos, view);
    float4 clip_space = mul(view_space, projection);
    output.Pos = clip_space;
    output.Normal = mul(input.Normal, world);
    output.View = eye_pos - world_pos.xyz;
    output.Light = eye_pos - world_pos.xyz;
    return output;
}

float4 PS(PS_INPUT input) : SV_Target
{
    float3 light = normalize(input.Light);
    float3 view = normalize(input.View);
    float3 normal = normalize(input.Normal);
    float3 halfway = normalize(light + view);
    float3 diffuse_value = saturate(dot(normal, light));
    float3 specular = pow(saturate(dot(normal, halfway)), material_power);
    
    float4 diffuse = diffuse_color;
    
    return float4(specular + diffuse_value * diffuse, diffuse.a);
}

BlendState opaque_blend_state 
{
    BlendEnable[0] = FALSE;
};

DepthStencilState DepthEnable
{
    DepthEnable = TRUE;
};

technique10 render
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, VS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, PS() ) );
        SetBlendState( opaque_blend_state, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
        SetDepthStencilState(DepthEnable, 0);
    }
}

