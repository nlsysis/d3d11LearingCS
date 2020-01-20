#include "LightHelper.hlsl"

//cbuffer cbPerFrame : register(b0)
//{
//    float3 gEyePosW;
//    float padding;
//};
cbuffer PSConstantBuffer : register(b1)
{
    DirectionalLight gDirLight;
    PointLight gPointLight;
    SpotLight gSpotLight;
    Material gMaterial;
    float3 gEyePosW;
    float gPad;
}


//StructuredBuffer<DirectionalLight> gDirLight : register(t0);
//StructuredBuffer<PointLight> gPointLight : register(t1);
//StructuredBuffer<SpotLight> gSpotLight : register(t2);
//StructuredBuffer<Material> gMaterial : register(t3);

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION; //position in the world
    float3 NormalW : NORMAL;
};


float4 PS(VertexOut pin) : SV_Target
{
 
	// Interpolating normal can unnormalize it, so normalize it.
    pin.NormalW = normalize(pin.NormalW);

    float3 toEyeW = normalize(gEyePosW - pin.PosW);

	// Start with a sum of zero. 
    float4 ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 spec = float4(0.0f, 0.0f, 0.0f, 0.0f);

	// Sum the light contribution from each light source.
    float4 A, D, S;

    ComputeDirectionalLight(gMaterial, gDirLight, pin.NormalW, toEyeW, A, D, S);
    ambient += A;
    diffuse += D;
    spec += S;

    ComputePointLight(gMaterial, gPointLight, pin.PosW, pin.NormalW, toEyeW, A, D, S);
    ambient += A;
    diffuse += D;
    spec += S;

    ComputeSpotLight(gMaterial, gSpotLight, pin.PosW, pin.NormalW, toEyeW, A, D, S);
    ambient += A;
    diffuse += D;
    spec += S;
	   
    float4 litColor = ambient + diffuse + spec;

	// Common to take alpha from diffuse material.
    litColor.a = gMaterial.Diffuse.a;

    return litColor;
}