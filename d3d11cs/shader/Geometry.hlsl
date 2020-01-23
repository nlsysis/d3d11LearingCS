#include "LightHelper.hlsl"

cbuffer cbPerObject : register(b0)
{
    matrix gWorldViewProj;
    matrix gWorldInvTranspose;
};

struct VertexIn
{
    float3 PosL : POSITION;
    float4 Color : COLOR;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float4 Color : COLOR;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
	
	// Transform to homogeneous clip space.
    matrix mvp = transpose(gWorldViewProj);
    vout.PosH = mul(float4(vin.PosL, 1.0f), mvp);
	
	// Just pass vertex color into the pixel shader.
    vout.Color = vin.Color;
    
    return vout;
}


float4 PS(VertexOut pin) : SV_Target
{
    return pin.Color;
}


VertexOut SoVS(VertexOut vin)
{
    return vin;
}

[maxvertexcount(9)]
void GS(triangle VertexOut input[3], inout TriangleStream<VertexOut> output)
{
    //       v1
    //       /\
    //      /  \
    //   v3/____\v4
    //    /\xxxx/\
    //   /  \xx/  \
    //  /____\/____\
    // v0    v5    v2
    VertexOut vertices[6];
    int i;
    [unroll]
    for (i = 0; i < 3;++i)
    {
        vertices[i] = input[i];
        vertices[i + 3].Color = (input[i].Color + input[(i + 1) % 3].Color) / 2.0f;
        vertices[i + 3].PosH = (input[i].PosH + input[(i + 1) % 3].PosH) / 2.0f;
    }
    
    [unroll]
    for (i = 0; i < 3; ++i)
    {
        output.Append(vertices[i]);
        output.Append(vertices[3 + i]);
        output.Append(vertices[(i + 2) % 3 + 3]);
        output.RestartStrip();

    }
}