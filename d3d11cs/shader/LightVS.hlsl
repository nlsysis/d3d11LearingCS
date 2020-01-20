#include "LightHelper.hlsl"

cbuffer cbPerObject : register(b0)
{
 //   matrix gWorld;
 //   matrix gWorldInvTranspose;
    matrix gWorldViewProj;
};


struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float4 Color : COLOR;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;    //position in the world
    float3 NormalW : NORMAL;
    float4 Color : COLOR;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
	
	// Transform to world space space.
  //  vout.PosW = mul(float4(vin.PosL, 1.0f), gWorld).xyz;
  //  vout.NormalW = mul(vin.NormalL, (float3x3) gWorldInvTranspose); //the space change of normal is not like vertex
		
	// Transform to homogeneous clip space.
    matrix mvp = transpose(gWorldViewProj);
    vout.PosH = mul(float4(vin.PosL, 1.0f), mvp);
    vout.Color = vin.Color;
    return vout;
}

