#pragma once
#include "d3dApp.h"
#include "MathHelper.h"

class GeometryApp :public D3DApp
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
		XMMATRIX worldInvTranspose;
	};
public:
	GeometryApp(HINSTANCE hInstance);
	~GeometryApp();

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
	ID3D11Buffer* mTriangleVB;
	ID3D11Buffer* mTriangleIB;
	ID3D11Buffer* mMatrixBuffer;
	ID3D11VertexShader* mVertexShader;
	ID3D11PixelShader* mPixelShader;

	ID3D11GeometryShader* mGeometryShader;

	ID3D11InputLayout* mInputLayout;

	XMFLOAT4X4 mWorld;
	XMFLOAT4X4 mView;
	XMFLOAT4X4 mProj;

	float mTheta;
	float mPhi;
	float mRadius;

	POINT mLastMousePos;
};