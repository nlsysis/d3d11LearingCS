#pragma once
#include "d3dApp.h"
#include "d3dx11Effect.h"
#include "GeometryGenerator.h"
#include "MathHelper.h"
#include "LightHelper.h"
#include "Waves.h"
#include "RenderStates.h"
#include "BlurFilter.h"

enum RenderOptions
{
	Lighting = 0,
	Textures = 1,
	TexturesAndFog = 2
};

class BlendApp : public D3DApp
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
		//XMFLOAT3 padding;
		_int32 gLightCount;
		_int32 gFogEnabled;
		float padding;
		XMFLOAT4 gFogColor;
		Material gMaterial;
	};
public:
	BlendApp(HINSTANCE hInstance);
	~BlendApp();

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
	void BuildCrateGeometryBuffers();
	void BuildScreenQuadGeometryBuffers();
	void BuildOffscreenViews();
	void DrawScreenQuad();
	void DrawWrapper();

	void SetShaderParameters(const XMFLOAT4X4 worldMatrix, const Material* material, const XMFLOAT4X4 texTransform);
	void SetShaderParameters();
	void BuildFX();
	void BuildConstantBuffer();
	void SetLightParameters();
	void BuildSampler();
	void UpdateInput(float dt);
private:
	ID3D11Buffer* mLandVB;
	ID3D11Buffer* mLandIB;

	ID3D11Buffer* mWavesVB;
	ID3D11Buffer* mWavesIB;

	ID3D11Buffer* mBoxVB;
	ID3D11Buffer* mBoxIB;

	ID3D11ShaderResourceView* mGrassMapSRV;
	ID3D11ShaderResourceView* mWavesMapSRV;
	ID3D11ShaderResourceView* mBoxMapSRV;

	ID3D11Buffer* mScreenQuadVB;
	ID3D11Buffer* mScreenQuadIB;

	ID3D11ShaderResourceView* mOffscreenSRV;
	ID3D11UnorderedAccessView* mOffscreenUAV;
	ID3D11RenderTargetView* mOffscreenRTV;

	BlurFilter mBlur;

	ID3D11Buffer* mMatrixBuffer;
	ID3D11Buffer* mEyeMaterialBuffer;
	ID3D11SamplerState* mSampleState;

	ID3D11VertexShader* mVertexShader;
	ID3D11PixelShader* mPixelShader;
	ID3D11InputLayout* mInputLayout;

	Waves mWaves;

	DirectionalLight mDirLights[3];
	ID3D11Buffer* mDirLightBuffer;
	ID3D11ShaderResourceView* DirLightResourceView;
	Material mLandMat;
	Material mWavesMat;
	Material mBoxMat;

	XMFLOAT4X4 mBoxTexTransform;
	XMFLOAT4X4 mGrassTexTransform;
	XMFLOAT4X4 mWaterTexTransform;
	XMFLOAT4X4 mLandWorld;
	XMFLOAT4X4 mWavesWorld;
	XMFLOAT4X4 mBoxWorld;

	XMFLOAT4X4 mView;
	XMFLOAT4X4 mProj;

	UINT mLandIndexCount;

	XMFLOAT2 mWaterTexOffset;

	RenderOptions mRenderOptions;

	XMFLOAT3 mEyePosW;

	float mTheta;
	float mPhi;
	float mRadius;
	
	int mLightCount;
	int mFogEnable;   //0-disable 1-enable
	POINT mLastMousePos;
};