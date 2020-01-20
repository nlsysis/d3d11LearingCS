#pragma once
#include "d3dApp.h"
#include "GeometryGenerator.h"
#include "MathHelper.h"
#include "LightHelper.h"
#include "Waves.h"

class LightingApp : public D3DApp
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
	};
	struct LightType
	{
		//DirectionalLight dirLight;
		//PointLight pointLight;
		//SpotLight spotLight;
		//Material material;
		XMFLOAT3 eyePos;
		float padding;
	};
public:
	LightingApp(HINSTANCE hInstance);
	~LightingApp();

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
	void BuildFX();
	void BuildConstantBuffer();
	void SetShaderParameters(const XMFLOAT4X4 worldMatrix, const Material material);
	void SetLightParameters();
private:
	ID3D11Buffer* mLandVB;
	ID3D11Buffer* mLandIB;
	ID3D11Buffer* mWavesVB;
	ID3D11Buffer* mWavesIB;

	ID3D11Buffer* mMatrixBuffer;
	ID3D11Buffer* mLightBuffer;
	ID3D11Buffer* mMaterialBuffer;
	ID3D11ShaderResourceView* MaterialResourceView;
	ID3D11Buffer* mDirLightBuffer;
	ID3D11ShaderResourceView* DirLightResourceView;
	ID3D11Buffer* mPointLightBuffer;
	ID3D11ShaderResourceView* PointLightResourceView;
	ID3D11Buffer* mSpotLightBuffer;
	ID3D11ShaderResourceView* SpotLightBufferResourceView;


	ID3D11VertexShader* mVertexShader;
	ID3D11PixelShader* mPixelShader;
	ID3D11InputLayout* mInputLayout;

	ID3D11RasterizerState* mWireframeRS;
	// Define transformations from local spaces to world space.
	XMFLOAT4X4 mLandWorld;
	XMFLOAT4X4 mWavesWorld;

	UINT mLandIndexCount;

	Waves mWaves;

	LightType mLight;

	DirectionalLight mDirLight;
	PointLight mPointLight;
	SpotLight mSpotLight;
	Material mLandMat;
	Material mWavesMat;

	XMFLOAT4X4 mView;
	XMFLOAT4X4 mProj;

	float mTheta;
	float mPhi;
	float mRadius;

	POINT mLastMousePos;

};