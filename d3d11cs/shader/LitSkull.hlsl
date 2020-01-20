#include "LightHelper.hlsl"

cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
    float4x4 gWorldInvTranspose;
    float4x4 gWorldViewProj;
    float4x4 gTexTransform;
};

cbuffer cbPerFrame : register(b1)
{
    float3 gEyePosW;
    float gFogStart;
    float gFogRange;
    float3 padding;
    float4 gFogColor;
    Material gMaterial;
};

StructuredBuffer<DirectionalLight> gDirLights : register(t0);
//StructuredBuffer<Material> gMaterial : register(t1);

Texture2D gDiffuseMap;
SamplerState samAnisotropic;

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
	
	// Transform to world space space.
    vout.PosW = mul(float4(vin.PosL, 1.0f), gWorld).xyz;
    vout.NormalW = mul(vin.NormalL, (float3x3) gWorldInvTranspose);
		
	// Transform to homogeneous clip space.
    matrix mvp = transpose(gWorldViewProj);
    vout.PosH = mul(float4(vin.PosL, 1.0f), mvp);
	
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    int gLightCount = 1;
	// Interpolating normal can unnormalize it, so normalize it.
    pin.NormalW = normalize(pin.NormalW);

	// The toEye vector is used in lighting.
    float3 toEye = gEyePosW - pin.PosW;

	// Cache the distance to the eye from this surface point.
    float distToEye = length(toEye);

	// Normalize.
    toEye /= distToEye;
	
	//
	// Lighting.
	//

	// Start with a sum of zero. 
    float4 ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 spec = float4(0.0f, 0.0f, 0.0f, 0.0f);

	// Sum the light contribution from each light source.  
	[unroll]
    for (int i = 0; i < gLightCount; ++i)
    {
        float4 A, D, S;
        ComputeDirectionalLight(gMaterial, gDirLights[i], pin.NormalW, toEye,
			A, D, S);

        ambient += A;
        diffuse += D;
        spec += S;
    }
    float4 litColor = ambient + diffuse + spec;

	// Common to take alpha from diffuse material.
    litColor.a = gMaterial.Diffuse.a;

    return litColor;
}
