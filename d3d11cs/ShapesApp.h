#pragma once
#include "d3dApp.h"
#include "d3dx11Effect.h"
#include "GeometryGenerator.h"
#include "MathHelper.h"

class ShapesApp : public D3DApp
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
	ShapesApp(HINSTANCE hInstance);
	~ShapesApp();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene();

	void OnMouseDown(WPARAM btnState, int x, int y);
	void OnMouseUp(WPARAM btnState, int x, int y);
	void OnMouseMove(WPARAM btnState, int x, int y);
private:
	float GetHeight(float x, float z)const;
	void BuildGeometryBuffers();
	void BuildFX();
	void BuildConstantBuffer();
	void SetShaderParameters(const XMFLOAT4X4 worldMatrix);

private:
	ID3D11Buffer* mVB;
	ID3D11Buffer* mIB;
	ID3D11Buffer* mMatrixBuffer;

	ID3D11VertexShader* mVertexShader;
	ID3D11PixelShader* mPixelShader;

	ID3D11InputLayout* mInputLayout;

	ID3D11RasterizerState* mWireframeRS;
	// Define transformations from local spaces to world space.
	XMFLOAT4X4 mSphereWorld[10];
	XMFLOAT4X4 mCylWorld[10];
	XMFLOAT4X4 mBoxWorld;
	XMFLOAT4X4 mGridWorld;
	XMFLOAT4X4 mCenterSphere;

	XMFLOAT4X4 mView;
	XMFLOAT4X4 mProj;

	int mBoxVertexOffset;
	int mGridVertexOffset;
	int mSphereVertexOffset;
	int mCylinderVertexOffset;

	UINT mBoxIndexOffset;
	UINT mGridIndexOffset;
	UINT mSphereIndexOffset;
	UINT mCylinderIndexOffset;

	UINT mBoxIndexCount;
	UINT mGridIndexCount;
	UINT mSphereIndexCount;
	UINT mCylinderIndexCount;

	float mTheta;
	float mPhi;
	float mRadius;

	POINT mLastMousePos;   //long(x,y)

};