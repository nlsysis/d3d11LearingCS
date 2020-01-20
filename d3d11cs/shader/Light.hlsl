//#include "LightHelper.hlsl"

cbuffer cbPerObject
{
 //   matrix gWorld;
 //   matrix gWorldInvTranspose;
    matrix gWorldViewProj;
};

struct VertexIn
{
    float3 PosL : POSITION;
    float3 Normal : NORMAL;
    float4 Color : COLOR;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
  //  float3 PosW : POSITION; //position in the world
  //  float3 NormalW : NORMAL;
    float4 Color : COLOR;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
	
	// Transform to homogeneous clip space.
    matrix mvp = transpose(gWorldViewProj);
    vout.PosH = mul(float4(vin.PosL, 1.0f), mvp);
  //  vout.PosW = float3(0.0f, 0.0f, 0.0f);
  //  vout.NormalW = float3(0.0f, 0.0f, -1.0f);
	// Just pass vertex color into the pixel shader.
    vout.Color = vin.Color;
    
    return vout;
}


float4 PS(VertexOut pin) : SV_Target
{
    return pin.Color;
}