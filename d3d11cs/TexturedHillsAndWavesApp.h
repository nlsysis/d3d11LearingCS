#pragma once
#include "d3dApp.h"
#include "d3dx11Effect.h"
#include "GeometryGenerator.h"
#include "MathHelper.h"
#include "LightHelper.h"
#include "Waves.h"

class TexturedHillsAndWavesApp : public D3DApp
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
	TexturedHillsAndWavesApp(HINSTANCE hInstance);
	~TexturedHillsAndWavesApp();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene();

	void OnMouseDown(WPARAM btnState, int x, int y);
	void OnMouseUp(WPARAM btnState, int x, int y);
	void OnMouseMove(WPARAM btnState, int x, int y);

private:
	float GetHillHeight(float x, float z)const;
	XMFLOAT3 GetHillNormal(float x, float z)const;
	void BuildLandGeometryBuffers();
	void BuildWaveGeometryBuffers();

	void SetShaderParameters(const XMFLOAT4X4 worldMatrix, const Material material, const XMFLOAT4X4 texTransform);
	void BuildFX();
	void BuildConstantBuffer();
	void SetLightParameters();
	void BuildSampler();
private:
	ID3D11Buffer* mLandVB;
	ID3D11Buffer* mLandIB;

	ID3D11Buffer* mWavesVB;
	ID3D11Buffer* mWavesIB;

	ID3D11ShaderResourceView* mGrassMapSRV;
	ID3D11ShaderResourceView* mWavesMapSRV;

	ID3D11Buffer* mMatrixBuffer;
	ID3D11Buffer* mEyeMaterialBuffer;
	ID3D11SamplerState* mSampleState;

	ID3D11VertexShader* mVertexShader;
	ID3D11PixelShader* mPixelShader;
	ID3D11InputLayout* mInputLayout;

	Waves mWaves;

	DirectionalLight mDirLights[3];    //set the direction light as SRV
	ID3D11Buffer* mDirLightBuffer;
	ID3D11ShaderResourceView* DirLightResourceView;

	Material mLandMat;
	Material mWavesMat;

	XMFLOAT4X4 mGrassTexTransform;
	XMFLOAT4X4 mWaterTexTransform;
	XMFLOAT4X4 mLandWorld;
	XMFLOAT4X4 mWavesWorld;

	XMFLOAT4X4 mView;
	XMFLOAT4X4 mProj;

	UINT mLandIndexCount;
	XMFLOAT2 mWaterTexOffset;

	XMFLOAT3 mEyePosW;

	float mTheta;
	float mPhi;
	float mRadius;

	POINT mLastMousePos;
};