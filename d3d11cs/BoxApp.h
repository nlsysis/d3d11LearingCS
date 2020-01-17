#pragma once
#include "d3dApp.h"
#include "MathHelper.h"
#include <D3Dcompiler.h>

#pragma comment(lib,"D3DCompiler.lib")
class BoxApp : public D3DApp
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
	BoxApp(HINSTANCE hInstance);
	~BoxApp();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene();

	void OnMouseDown(WPARAM btnState, int x, int y);
	void OnMouseUp(WPARAM btnState, int x, int y);
	void OnMouseMove(WPARAM btnState, int x, int y);

private:
	void BuildGeometryBuffers();
	void BuildFX();
	void BuildConstantBuffer();
	void SetShaderParameters();

private:
	ID3D11Buffer* mBoxVB;
	ID3D11Buffer* mBoxIB;
	ID3D11Buffer* mMatrixBuffer;
//	ID3DX11Effect* mFX;
//	ID3DX11EffectTechnique* mTech;
//	ID3DX11EffectMatrixVariable* mfxWorldViewProj;

	ID3D11VertexShader* mVertexShader;
	ID3D11PixelShader* mPixelShader;

	ID3D11InputLayout* mInputLayout;

	XMFLOAT4X4 mWorld;
	XMFLOAT4X4 mView;
	XMFLOAT4X4 mProj;

	float mTheta;
	float mPhi;
	float mRadius;

	POINT mLastMousePos;
};