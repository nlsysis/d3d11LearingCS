#pragma once
#include "d3dApp.h"
#include "MathHelper.h"
#include <D3Dcompiler.h>
#include "FluidMain.h"

#pragma comment(lib,"D3DCompiler.lib")
class Fluid3DApp : public D3DApp
{
private:
	struct Vertex
	{
		XMFLOAT3 Pos;
		XMFLOAT4 Color;
	};
	struct MatrixBufferType
	{
		XMMATRIX mvp;
	};

public:
	Fluid3DApp(HINSTANCE hInstance);
	~Fluid3DApp();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene();
	void DrawScene(float dt) {};

private:
	void BuildGeometryBuffers();
	void BuildFX();
	void BuildConstantBuffer();
	void SetShaderParameters();
	void UpdateInput(float dt);
private:
	ID3D11Buffer* mBoxVB;
	ID3D11Buffer* mBoxIB;
	ID3D11Buffer* mMatrixBuffer;

	ID3D11VertexShader* mVertexShader;
	ID3D11PixelShader* mPixelShader;

	ID3D11InputLayout* mInputLayout;

	XMFLOAT4X4 mWorld;
	XMFLOAT4X4 mView;
	XMFLOAT4X4 mProj;

	float mTheta;
	float mPhi;
	float mRadius;

	//in fluid
	XMFLOAT3 wall_min;
	XMFLOAT3 wall_max;
	XMFLOAT3 fluid_min;
	XMFLOAT3 fluid_max;
	XMFLOAT3 gravity;

	unsigned short particleNum_MAX;
	
	FluidMain* g_fludiMain;
};