#pragma once
#include "d3dApp.h"
#include "d3dx11Effect.h"
#include "GeometryGenerator.h"
#include "MathHelper.h"
#include "LightHelper.h"

class LitSkullApp : public D3DApp
{
private:
	struct Vertex
	{
		XMFLOAT3 Pos;
		XMFLOAT3 Normal;
	};
	struct MatrixBufferType
	{
		XMMATRIX world;
		XMMATRIX worldInvTranspose;
		XMMATRIX mvp;
		XMMATRIX texTransform;
	};
	struct EyeMaterialType
	{
		XMFLOAT3 gEyePosW;
		float gFogStart;
		float gFogRange;
		XMFLOAT3 padding;
		XMFLOAT4 gFogColor;
		Material gMaterial;
		
	};
public:
	LitSkullApp(HINSTANCE hInstance);
	~LitSkullApp();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene();

	void OnMouseDown(WPARAM btnState, int x, int y);
	void OnMouseUp(WPARAM btnState, int x, int y);
	void OnMouseMove(WPARAM btnState, int x, int y);

private:
	void BuildShapeGeometryBuffers();
	void BuildSkullGeometryBuffers();
	void SetShaderParameters(const XMFLOAT4X4 worldMatrix, const Material material);
	void BuildFX();
	void BuildConstantBuffer();
	void SetLightParameters();
private:
	ID3D11Buffer* mShapesVB;
	ID3D11Buffer* mShapesIB;

	ID3D11Buffer* mSkullVB;
	ID3D11Buffer* mSkullIB;
	ID3D11Buffer* mMatrixBuffer;
	ID3D11Buffer* mEyeMaterialBuffer;

	DirectionalLight mDirLights[3];
	ID3D11Buffer* mDirLightBuffer;
	ID3D11ShaderResourceView* DirLightResourceView;

	Material mGridMat;
	Material mBoxMat;
	Material mCylinderMat;
	Material mSphereMat;
	Material mSkullMat;

	ID3D11VertexShader* mVertexShader;
	ID3D11PixelShader* mPixelShader;
	ID3D11InputLayout* mInputLayout;

	// Define transformations from local spaces to world space.
	XMFLOAT4X4 mSphereWorld[10];
	XMFLOAT4X4 mCylWorld[10];
	XMFLOAT4X4 mBoxWorld;
	XMFLOAT4X4 mGridWorld;
	XMFLOAT4X4 mSkullWorld;

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

	UINT mSkullIndexCount;

	UINT mLightCount;

	XMFLOAT3 mEyePosW;
	float mTheta;
	float mPhi;
	float mRadius;

	POINT mLastMousePos;
};