#pragma once
#include "d3dApp.h"
#include "d3dx11Effect.h"
#include "GeometryGenerator.h"
#include "MathHelper.h"

class Fluid2D : public D3DApp
{
private:
	struct Particle
	{
		XMFLOAT2 vPosition;
		XMFLOAT2 vVelocity;
	};

	struct ParticleDensity
	{
		float fDensity;
	};

	struct ParticleForces
	{
		XMFLOAT2 vAcceleration;
	};

	struct UINT2
	{
		UINT x;
		UINT y;
	};

public:
	Fluid2D(HINSTANCE hInstance);
	~Fluid2D();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene();
	void DrawScene(float dt);
private:
	HRESULT CreateSimulationBuffers();
	void BuildShader();
	void SafeCompileShaderFromFile(const WCHAR* fileName,LPCSTR enterPoint,LPCSTR shaderModel,ID3DBlob **ppBlob);
	void GPUSort(ID3D11DeviceContext* pd3dImmediateContext,
		ID3D11UnorderedAccessView* inUAV, ID3D11ShaderResourceView* inSRV,
		ID3D11UnorderedAccessView* tempUAV, ID3D11ShaderResourceView* tempSRV);
	void SimulateFluid_Simple(ID3D11DeviceContext* pd3dImmediateContext);
	void SimulateFluid_Shared(ID3D11DeviceContext* pd3dImmediateContext);
	void SimulateFluid_Grid(ID3D11DeviceContext* pd3dImmediateContext);
	void SimulateFluid(ID3D11DeviceContext* pd3dImmediateContext, float fElapsedTime);
	void RenderFluid(ID3D11DeviceContext* pd3dImmediateContext, float fElapsedTime);
//public:

};