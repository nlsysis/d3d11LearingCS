#pragma once
#include "d3dApp.h"
#include "d3dx11Effect.h"
#include "GeometryGenerator.h"
#include "MathHelper.h"
#include "Waves.h"

class WavesApp :public D3DApp
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
	WavesApp(HINSTANCE hInstance);
	~WavesApp();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene();

	void OnMouseDown(WPARAM btnState, int x, int y);
	void OnMouseUp(WPARAM btnState, int x, int y);
	void OnMouseMove(WPARAM btnState, int x, int y);
private:
	float GetHeight(float x, float z)const;
	void BuildLandGeometryBuffers();
	void BuildWavesGeometryBuffers();
	void BuildFX();
	void BuildConstantBuffer();
	void SetShaderParameters(const XMFLOAT4X4 worldMatrix);
	void BuildVertexLayout();
private:
	ID3D11Buffer* mLandVB;
	ID3D11Buffer* mLandIB;
	ID3D11Buffer* mWavesVB;
	ID3D11Buffer* mWavesIB; 
	ID3D11Buffer* mMatrixBuffer;

	ID3D11VertexShader* mVertexShader;
	ID3D11PixelShader* mPixelShader;
	ID3D11InputLayout* mInputLayout;

	ID3D11RasterizerState* mWireframeRS;
	// Define transformations from local spaces to world space.
	XMFLOAT4X4 mGridWorld;
	XMFLOAT4X4 mWavesWorld;
	UINT mGridIndexCount;

	Waves mWaves;

	XMFLOAT4X4 mView;
	XMFLOAT4X4 mProj;

	float mTheta;
	float mPhi;
	float mRadius;

	POINT mLastMousePos;
};