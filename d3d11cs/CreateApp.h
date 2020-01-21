#pragma once
#include "d3dApp.h"
#include "d3dx11Effect.h"
#include "GeometryGenerator.h"
#include "MathHelper.h"
#include "LightHelper.h"

class CreateApp : public D3DApp
{
private:
	struct Vertex
	{
		XMFLOAT3 Pos;
		XMFLOAT3 Normal;
		XMFLOAT2 Tex;
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
	CreateApp(HINSTANCE hInstance);
	~CreateApp();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene();

	void OnMouseDown(WPARAM btnState, int x, int y);
	void OnMouseUp(WPARAM btnState, int x, int y);
	void OnMouseMove(WPARAM btnState, int x, int y);

private:
	void BuildGeometryBuffers();

	void SetShaderParameters(const XMFLOAT4X4 worldMatrix, const Material material);
	void BuildFX();
	void BuildConstantBuffer();
	void SetLightParameters();
	void BuildSampler();

private:
	ID3D11Buffer* mBoxVB;
	ID3D11Buffer* mBoxIB;

	ID3D11Buffer* mMatrixBuffer;
	ID3D11Buffer* mEyeMaterialBuffer;
	ID3D11SamplerState* mSampleState;
	ID3D11ShaderResourceView* mDiffuseMapSRV;

	DirectionalLight mDirLights[3];
	ID3D11Buffer* mDirLightBuffer;
	ID3D11ShaderResourceView* DirLightResourceView;
	Material mBoxMat;

	ID3D11VertexShader* mVertexShader;
	ID3D11PixelShader* mPixelShader;
	ID3D11InputLayout* mInputLayout;

	XMFLOAT4X4 mTexTransform;
	XMFLOAT4X4 mBoxWorld;

	XMFLOAT4X4 mView;
	XMFLOAT4X4 mProj;

	int mBoxVertexOffset;
	UINT mBoxIndexOffset;
	UINT mBoxIndexCount;

	XMFLOAT3 mEyePosW;

	float mTheta;
	float mPhi;
	float mRadius;

	POINT mLastMousePos;
};

