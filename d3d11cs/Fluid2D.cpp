#include "Fluid2D.h"

//global variables

//Compute Shader Constants
//Grid cell key size for sorting.8-bit for x and y
const UINT NUM_GRID_INDICES = 65536;

//Numthreads size for the simulation
const UINT SIMULATION_BLOCK_SIZE = 256;

//Numthreads size for the sort
const UINT BITONIC_BLOCK_SIZE = 512;
const UINT TRANSPOSE_BLOCK_SIZE = 16;

const UINT NUM_PARTICLES_8K = 8 * 1024;
const UINT NUM_PARTICLES_16K =16 * 1024;
UINT g_iNumParticles = NUM_PARTICLES_16K;   //default use particle number

//Particle Properties
float g_fInitialParticleSpacing = 0.0045f;      //
float g_fSmoothlen = 0.012f;                   //光滑核函数作用范围长度
float g_fPressureStiffness = 200.0f;          //
float g_fRestDensity = 1000.0f;                //p0静态流体密度
float g_fParticleMass = 0.0002f;               //
float g_fViscosity = 0.1f;                     //
float g_fMaxAllowableTimeStep = 0.005f;        //
float g_fParticleRenderSize = 0.003f;          //

//Gravity Directions
const XMFLOAT2A GRAVITY_DOWN(0, -0.5f);
const XMFLOAT2A GRAVITY_UP(0, 0.5f);
const XMFLOAT2A GRAVITY_LEFT(-0.5f, 0);
const XMFLOAT2A GRAVITY_RIGHT(0.5f, 0);
XMFLOAT2A g_vGravity = GRAVITY_DOWN;  //default gracity direction

//Mao Size
//These values should not be larger than 256 * fSmothlen
//Since the map must be divided up into fSmoothlen sized grid cells
//And the grid cell is used as a 16-bit sort key,8-bits for x and y
float g_fMapHeight = 1.2f;
float g_fMapWidth = (4.0f / 3.0f) * g_fMapHeight;

//Map Wall Collision Planes
float g_fWallStiffness = 3000.0f;
XMFLOAT3A g_vPlanes[4] = {
	XMFLOAT3A(1,0,0),
	XMFLOAT3A(0,1,0),
	XMFLOAT3A(-1,0,g_fMapWidth),
	XMFLOAT3A(0,-1,g_fMapHeight)
};

//Simulation Algorithm
enum eSimulationMode
{
	SIM_MODE_SIMPLE,
	SIM_MODE_SHARED,
	SIM_MODE_GRID
};

eSimulationMode g_eSimMode = SIM_MODE_GRID;



//constant buffer layout
#pragma warning(push)
#pragma warning(disable:4324)   //structure was padded due to _declspec(align())
_DECLSPEC_ALIGN_16_ struct CBSimulationConstants
{
	UINT iNumParticles;
	float fTimeStep;
	float fSmoothlen;
	float fPressureStiffness;
	float fRestDensity;
	float fDensityCoef;
	float fGradPressureCoef;
	float fLapViscosityCoef;
	float fWallStiffness;

	XMFLOAT2A vGravity;
	XMFLOAT4 vGridDim;
	
	XMFLOAT3A vPlanes[4];
};

_DECLSPEC_ALIGN_16_ struct CBRenderConstants
{
	XMFLOAT4X4 mViewProjection;
	float fParticleSize;
};

_DECLSPEC_ALIGN_16_ struct SortCB
{
	UINT iLevel;
	UINT iLevelMask;
	UINT iWidth;
	UINT iHeight;
};

#pragma warning(pop)



// --------------------------------------------------------------------------------------
// Helper for creating constant buffers
//--------------------------------------------------------------------------------------
template <class T>
HRESULT CreateConstantBuffer(ID3D11Device* pd3dDevice, ID3D11Buffer** ppCB)
{
	HRESULT hr = S_OK;

	D3D11_BUFFER_DESC Desc;
	Desc.Usage = D3D11_USAGE_DEFAULT;
	Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	Desc.CPUAccessFlags = 0;
	Desc.MiscFlags = 0;
	Desc.ByteWidth = sizeof(T);
	HR(pd3dDevice->CreateBuffer(&Desc, NULL, ppCB));

	return hr;
}
template <class T>
HRESULT CreateConstantBuffer2(ID3D11Device* pd3dDevice, ID3D11Buffer** ppCB)
{
	HRESULT hr = S_OK;

	D3D11_BUFFER_DESC Desc;
	Desc.Usage = D3D11_USAGE_DYNAMIC;
	Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Desc.MiscFlags = 0;
	Desc.ByteWidth = sizeof(T);
	HR(pd3dDevice->CreateBuffer(&Desc, NULL, ppCB));

	return hr;
}

//--------------------------------------------------------------------------------------
// Helper for creating structured buffers with an SRV and UAV
//--------------------------------------------------------------------------------------
template <class T>
HRESULT CreateStructuredBuffer(ID3D11Device* pd3dDevice, UINT iNumElements, ID3D11Buffer** ppBuffer, ID3D11ShaderResourceView** ppSRV, ID3D11UnorderedAccessView** ppUAV, const T* pInitialData = NULL)
{
	HRESULT hr = S_OK;

	// Create SB
	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.ByteWidth = iNumElements * sizeof(T);
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	bufferDesc.StructureByteStride = sizeof(T);

	D3D11_SUBRESOURCE_DATA bufferInitData;
	ZeroMemory(&bufferInitData, sizeof(bufferInitData));
	bufferInitData.pSysMem = pInitialData;
	HR(pd3dDevice->CreateBuffer(&bufferDesc, (pInitialData) ? &bufferInitData : NULL, ppBuffer));

	// Create SRV
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.ElementWidth = iNumElements;
	HR(pd3dDevice->CreateShaderResourceView(*ppBuffer, &srvDesc, ppSRV));

	// Create UAV
	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	ZeroMemory(&uavDesc, sizeof(uavDesc));
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.NumElements = iNumElements;
	HR(pd3dDevice->CreateUnorderedAccessView(*ppBuffer, &uavDesc, ppUAV));

	return hr;
}

Fluid2D::Fluid2D(HINSTANCE hInstance)
	:D3DApp(hInstance)
{
	mMainWndCaption = L"Fluid Demo";
	mEnable4xMsaa = false;
	g_iNullUINT = 0;         //helper to clear buffers

	//Shaders
	g_pParticlesVS = NULL;
	g_pParticlesGS = NULL;
	g_pParticlesPS = NULL;

	g_pBuildGridCS = NULL;
	g_pClearGridIndicesCS = NULL;
	g_pBuildGridIndicesCS = NULL;
	g_pRearrangeParticlesCS = NULL;
	g_pDensity_SimpleCS = NULL;
	g_pForce_SimpleCS = NULL;
	g_pDensity_SharedCS = NULL;
	g_pForce_SharedCS = NULL;
	g_pDensity_GridCS = NULL;
	g_pForce_GridCS = NULL;
	g_pIntegrateCS = NULL;

	g_pSortBitonic = NULL;
	g_pSortTranspose = NULL;

	//structured buffers
	g_pParticles = NULL;
	g_pParticlesSRV = NULL;
	g_pParticlesUAV = NULL;

	g_pSortedParticles = NULL;
	g_pSortedParticlesSRV = NULL;
	g_pSortedParticlesUAV = NULL;

	g_pParticleDensity = NULL;
	g_pParticleDensitySRV = NULL;
	g_pParticleDensityUAV = NULL;

	g_pParticleForces = NULL;
	g_pParticleForcesSRV = NULL;
	g_pParticleForcesUAV = NULL;

	g_pGrid = NULL;
	g_pGridSRV = NULL;
	g_pGridUAV = NULL;

	g_pGridPingPong = NULL;
	g_pGridPingPongSRV = NULL;
	g_pGridPingPongUAV = NULL;

	g_pGridIndices = NULL;
	g_pGridIndicesSRV = NULL;
	g_pGridIndicesUAV = NULL;

	//Constant Buffers
	g_pcbSimulationConstants = NULL;
	g_pcbRenderConstants = NULL;
	g_pSortCB = NULL;
}

Fluid2D::~Fluid2D()
{
	//release constant shader
	SAFE_RELEASE(g_pcbSimulationConstants);
	SAFE_RELEASE(g_pcbRenderConstants);
	SAFE_RELEASE(g_pSortCB);

	SAFE_RELEASE(g_pParticlesVS);
	SAFE_RELEASE(g_pParticlesGS);
	SAFE_RELEASE(g_pParticlesPS);

	SAFE_RELEASE(g_pIntegrateCS);
	SAFE_RELEASE(g_pDensity_SimpleCS);
	SAFE_RELEASE(g_pForce_SimpleCS);
	SAFE_RELEASE(g_pDensity_SharedCS);
	SAFE_RELEASE(g_pForce_SharedCS);
	SAFE_RELEASE(g_pDensity_GridCS);
	SAFE_RELEASE(g_pForce_GridCS);
	SAFE_RELEASE(g_pBuildGridCS);
	SAFE_RELEASE(g_pClearGridIndicesCS);
	SAFE_RELEASE(g_pBuildGridIndicesCS);
	SAFE_RELEASE(g_pRearrangeParticlesCS);
	SAFE_RELEASE(g_pSortBitonic);
	SAFE_RELEASE(g_pSortTranspose);

	SAFE_RELEASE(g_pParticles);
	SAFE_RELEASE(g_pParticlesSRV);
	SAFE_RELEASE(g_pParticlesUAV);

	SAFE_RELEASE(g_pSortedParticles);
	SAFE_RELEASE(g_pSortedParticlesSRV);
	SAFE_RELEASE(g_pSortedParticlesUAV);

	SAFE_RELEASE(g_pParticleForces);
	SAFE_RELEASE(g_pParticleForcesSRV);
	SAFE_RELEASE(g_pParticleForcesUAV);

	SAFE_RELEASE(g_pParticleDensity);
	SAFE_RELEASE(g_pParticleDensitySRV);
	SAFE_RELEASE(g_pParticleDensityUAV);

	SAFE_RELEASE(g_pGridSRV);
	SAFE_RELEASE(g_pGridUAV);
	SAFE_RELEASE(g_pGrid);

	SAFE_RELEASE(g_pGridPingPongSRV);
	SAFE_RELEASE(g_pGridPingPongUAV);
	SAFE_RELEASE(g_pGridPingPong);

	SAFE_RELEASE(g_pGridIndicesSRV);
	SAFE_RELEASE(g_pGridIndicesUAV);
	SAFE_RELEASE(g_pGridIndices);
}

bool Fluid2D::Init()
{
	if (!D3DApp::Init())
		return false;
	CreateSimulationBuffers();
	BuildShader();
	return true;
}

void Fluid2D::OnResize()
{
	D3DApp::OnResize();

}

void Fluid2D::UpdateScene(float dt)
{
}

void Fluid2D::DrawScene()
{
}

void Fluid2D::DrawScene(float fElapsedTime)
{
	md3dDeviceContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Colors::Silver));
	md3dDeviceContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	
	SimulateFluid(md3dDeviceContext, fElapsedTime);

	RenderFluid(md3dDeviceContext, fElapsedTime);

	HR(mSwapChain->Present(0, 0));
}

HRESULT Fluid2D::CreateSimulationBuffers()
{
	HRESULT hr = S_OK;
	// Destroy the old buffers in case the number of particles has changed
	SAFE_RELEASE(g_pParticles);
	SAFE_RELEASE(g_pParticlesSRV);
	SAFE_RELEASE(g_pParticlesUAV);

	SAFE_RELEASE(g_pSortedParticles);
	SAFE_RELEASE(g_pSortedParticlesSRV);
	SAFE_RELEASE(g_pSortedParticlesUAV);

	SAFE_RELEASE(g_pParticleForces);
	SAFE_RELEASE(g_pParticleForcesSRV);
	SAFE_RELEASE(g_pParticleForcesUAV);

	SAFE_RELEASE(g_pParticleDensity);
	SAFE_RELEASE(g_pParticleDensitySRV);
	SAFE_RELEASE(g_pParticleDensityUAV);

	SAFE_RELEASE(g_pGridSRV);
	SAFE_RELEASE(g_pGridUAV);
	SAFE_RELEASE(g_pGrid);

	SAFE_RELEASE(g_pGridPingPongSRV);
	SAFE_RELEASE(g_pGridPingPongUAV);
	SAFE_RELEASE(g_pGridPingPong);

	SAFE_RELEASE(g_pGridIndicesSRV);
	SAFE_RELEASE(g_pGridIndicesUAV);
	SAFE_RELEASE(g_pGridIndices);

	// Create the initial particle positions
    // This is only used to populate the GPU buffers on creation
	const UINT iStartingWidth = (UINT)sqrt((FLOAT)g_iNumParticles);
	Particle* particles = new Particle[g_iNumParticles];
	ZeroMemory(particles, sizeof(Particle) * g_iNumParticles);
	for (UINT i = 0; i < g_iNumParticles; i++)
	{
		// Arrange the particles in a nice square
		UINT x = i % iStartingWidth;
		UINT y = i / iStartingWidth;
		particles[i].vPosition = XMFLOAT2(g_fInitialParticleSpacing * (FLOAT)x, g_fInitialParticleSpacing * (FLOAT)y);
		particles[i].vVelocity = XMFLOAT2(0.0f, 0.0f);
	}

	// Create Structured Buffers
	HR(CreateStructuredBuffer< Particle >(md3dDevice, g_iNumParticles, &g_pParticles, &g_pParticlesSRV, &g_pParticlesUAV, particles));
	D3D11SetDebugObjectName(g_pParticles, "Particles");
	D3D11SetDebugObjectName(g_pParticlesSRV, "Particles SRV");
	D3D11SetDebugObjectName(g_pParticlesUAV, "Particles UAV");
	HR(CreateStructuredBuffer< Particle >(md3dDevice, g_iNumParticles, &g_pSortedParticles, &g_pSortedParticlesSRV, &g_pSortedParticlesUAV, particles));
	D3D11SetDebugObjectName(g_pSortedParticles, "Sorted");
	D3D11SetDebugObjectName(g_pSortedParticlesSRV, "Sorted SRV");
	D3D11SetDebugObjectName(g_pSortedParticlesUAV, "Sorted UAV");
	HR(CreateStructuredBuffer< ParticleForces >(md3dDevice, g_iNumParticles, &g_pParticleForces, &g_pParticleForcesSRV, &g_pParticleForcesUAV));
	D3D11SetDebugObjectName(g_pParticleForces, "Forces");
	D3D11SetDebugObjectName(g_pParticleForcesSRV, "Forces SRV");
	D3D11SetDebugObjectName(g_pParticleForcesUAV, "Forces UAV");
	HR(CreateStructuredBuffer< ParticleDensity >(md3dDevice, g_iNumParticles, &g_pParticleDensity, &g_pParticleDensitySRV, &g_pParticleDensityUAV));
	D3D11SetDebugObjectName(g_pParticleDensity, "Density");
	D3D11SetDebugObjectName(g_pParticleDensitySRV, "Density SRV");
	D3D11SetDebugObjectName(g_pParticleDensityUAV, "Density UAV");
	HR(CreateStructuredBuffer< UINT >(md3dDevice, g_iNumParticles, &g_pGrid, &g_pGridSRV, &g_pGridUAV));
	D3D11SetDebugObjectName(g_pGrid, "Grid");
	D3D11SetDebugObjectName(g_pGridSRV, "Grid SRV");
	D3D11SetDebugObjectName(g_pGridUAV, "Grid UAV");
	HR(CreateStructuredBuffer< UINT >(md3dDevice, g_iNumParticles, &g_pGridPingPong, &g_pGridPingPongSRV, &g_pGridPingPongUAV));
	D3D11SetDebugObjectName(g_pGridPingPong, "PingPong");
	D3D11SetDebugObjectName(g_pGridPingPongSRV, "PingPong SRV");
	D3D11SetDebugObjectName(g_pGridPingPongUAV, "PingPong UAV");
	HR(CreateStructuredBuffer< UINT2 >(md3dDevice, NUM_GRID_INDICES, &g_pGridIndices, &g_pGridIndicesSRV, &g_pGridIndicesUAV));
	D3D11SetDebugObjectName(g_pGridIndices, "Indices");
	D3D11SetDebugObjectName(g_pGridIndicesSRV, "Indices SRV");
	D3D11SetDebugObjectName(g_pGridIndicesUAV, "Indices UAV");
	delete[] particles;

	// Create Constant Buffers
	HR(CreateConstantBuffer< CBSimulationConstants >(md3dDevice, &g_pcbSimulationConstants));
	HR(CreateConstantBuffer< CBRenderConstants >(md3dDevice, &g_pcbRenderConstants));
	HR(CreateConstantBuffer< SortCB >(md3dDevice, &g_pSortCB));

	D3D11SetDebugObjectName(g_pcbSimulationConstants, "Simluation");
	D3D11SetDebugObjectName(g_pcbRenderConstants, "Render");
	D3D11SetDebugObjectName(g_pSortCB, "Sort");
	return S_OK;
}

//Compile the Shaders
void Fluid2D::BuildShader()
{
	ID3DBlob* pBlob = NULL;
	//rendering shaders
	SafeCompileShaderFromFile(L".\\shader\\FluidRender.hlsl", "ParticleVS", "vs_5_0", &pBlob);
	HR(md3dDevice->CreateVertexShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pParticlesVS));
	D3D11SetDebugObjectName(g_pParticlesVS, "ParticlesVS");
	SAFE_RELEASE(pBlob);

	SafeCompileShaderFromFile(L".\\shader\\FluidRender.hlsl", "ParticleGS", "gs_5_0", &pBlob);
	HR(md3dDevice->CreateGeometryShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pParticlesGS));
	D3D11SetDebugObjectName(g_pParticlesGS, "ParticlesGS");
	SAFE_RELEASE(pBlob);

	SafeCompileShaderFromFile(L".\\shader\\FluidRender.hlsl", "ParticlePS", "ps_5_0", &pBlob);
	HR(md3dDevice->CreatePixelShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pParticlesPS));
	D3D11SetDebugObjectName(g_pParticlesPS, "ParticlesPS");
	SAFE_RELEASE(pBlob);

	// Compute Shaders
	const char* CSTarget = (md3dDevice->GetFeatureLevel() >= D3D_FEATURE_LEVEL_11_0) ? "cs_5_0" : "cs_4_0";
	SafeCompileShaderFromFile(L".\\shader\\FluidCS11.hlsl", "IntegrateCS", CSTarget, &pBlob);
	HR(md3dDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pIntegrateCS));
	D3D11SetDebugObjectName(g_pIntegrateCS, "IntegrateCS");
	SAFE_RELEASE(pBlob);

	SafeCompileShaderFromFile(L".\\shader\\FluidCS11.hlsl", "DensityCS_Simple", CSTarget, &pBlob);
	HR(md3dDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pDensity_SimpleCS));
	D3D11SetDebugObjectName(g_pDensity_SimpleCS, "DensityCS_Simple");
	SAFE_RELEASE(pBlob);

	SafeCompileShaderFromFile(L".\\shader\\FluidCS11.hlsl", "ForceCS_Simple", CSTarget, &pBlob);
	HR(md3dDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pForce_SimpleCS));
	D3D11SetDebugObjectName(g_pForce_SimpleCS, "ForceCS_Simple");
	SAFE_RELEASE(pBlob);

	SafeCompileShaderFromFile(L".\\shader\\FluidCS11.hlsl", "DensityCS_Shared", CSTarget, &pBlob);
	HR(md3dDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pDensity_SharedCS));
	D3D11SetDebugObjectName(g_pDensity_SharedCS, "DensityCS_Shared");
	SAFE_RELEASE(pBlob);

	SafeCompileShaderFromFile(L".\\shader\\FluidCS11.hlsl", "ForceCS_Shared", CSTarget, &pBlob);
	HR(md3dDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pForce_SharedCS));
	D3D11SetDebugObjectName(g_pForce_SharedCS, "ForceCS_Shared");
	SAFE_RELEASE(pBlob);

	SafeCompileShaderFromFile(L".\\shader\\FluidCS11.hlsl", "DensityCS_Grid", CSTarget, &pBlob);
	HR(md3dDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pDensity_GridCS));
	D3D11SetDebugObjectName(g_pDensity_GridCS, "DensityCS_Grid");
	SAFE_RELEASE(pBlob);

	SafeCompileShaderFromFile(L".\\shader\\FluidCS11.hlsl", "ForceCS_Grid", CSTarget, &pBlob);
	HR(md3dDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pForce_GridCS));
	D3D11SetDebugObjectName(g_pForce_GridCS, "ForceCS_Grid");
	SAFE_RELEASE(pBlob);

	SafeCompileShaderFromFile(L".\\shader\\FluidCS11.hlsl", "BuildGridCS", CSTarget, &pBlob);
	HR(md3dDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pBuildGridCS));
	D3D11SetDebugObjectName(g_pBuildGridCS, "BuildGridCS");
	SAFE_RELEASE(pBlob);

	SafeCompileShaderFromFile(L".\\shader\\FluidCS11.hlsl", "ClearGridIndicesCS", CSTarget, &pBlob);
	HR(md3dDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pClearGridIndicesCS));
	D3D11SetDebugObjectName(g_pClearGridIndicesCS, "ClearGridIndicesCS");
	SAFE_RELEASE(pBlob);

	SafeCompileShaderFromFile(L".\\shader\\FluidCS11.hlsl", "BuildGridIndicesCS", CSTarget, &pBlob);
	HR(md3dDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pBuildGridIndicesCS));
	D3D11SetDebugObjectName(g_pBuildGridIndicesCS, "BuildGridIndicesCS");
	SAFE_RELEASE(pBlob);

	SafeCompileShaderFromFile(L".\\shader\\FluidCS11.hlsl", "RearrangeParticlesCS", CSTarget, &pBlob);
	HR(md3dDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pRearrangeParticlesCS));
	D3D11SetDebugObjectName(g_pRearrangeParticlesCS, "RearrangeParticlesCS");
	SAFE_RELEASE(pBlob);

	// Sort Shaders
	SafeCompileShaderFromFile(L".\\shader\\ComputeShaderSort11.hlsl", "BitonicSort", CSTarget, &pBlob);
	HR(md3dDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pSortBitonic));
	D3D11SetDebugObjectName(g_pSortBitonic, "BitonicSort");
	SAFE_RELEASE(pBlob);

	SafeCompileShaderFromFile(L".\\shader\\ComputeShaderSort11.hlsl", "MatrixTranspose", CSTarget, &pBlob);
	HR(md3dDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pSortTranspose));
	D3D11SetDebugObjectName(g_pSortTranspose, "MatrixTranspose");
	SAFE_RELEASE(pBlob);
}


void Fluid2D::SafeCompileShaderFromFile(const WCHAR * fileName, LPCSTR enterPoint, LPCSTR shaderModel, ID3DBlob **ppBlob)
{
	ID3D10Blob* compilationMsgs = NULL;
	DWORD dwShaderFlags = D3D10_SHADER_ENABLE_STRICTNESS;
#ifdef _DEBUG
	dwShaderFlags |= D3D10_SHADER_DEBUG;
	dwShaderFlags |= D3D10_SHADER_SKIP_OPTIMIZATION;
#endif
	HRESULT hr = D3DX11CompileFromFile(fileName, 0, 0, enterPoint, shaderModel, D3D10_SHADER_ENABLE_STRICTNESS,
		0, 0, ppBlob, &compilationMsgs, 0);
	// compilationMsgs can store errors or warnings.
	if (compilationMsgs != 0)
	{
		MessageBoxA(0, (char*)compilationMsgs->GetBufferPointer(), 0, 0);
		ReleaseCOM(compilationMsgs);
	}

	// Even if there are no compilationMsgs, check to make sure there were no other errors.
	if (FAILED(hr))
	{
		DXTrace(__FILE__, (DWORD)__LINE__, hr, L"D3DX11CompileFromFile", true);
	}
}

//--------------------------------------------------------------------------------------
// GPU Bitonic Sort
// For more information, please see the ComputeShaderSort11 sample
//--------------------------------------------------------------------------------------
void Fluid2D::GPUSort(ID3D11DeviceContext * pd3dImmediateContext, 
	ID3D11UnorderedAccessView * inUAV, 
	ID3D11ShaderResourceView * inSRV,
	ID3D11UnorderedAccessView * tempUAV, 
	ID3D11ShaderResourceView * tempSRV)
{
	pd3dImmediateContext->CSSetConstantBuffers(0, 1, &g_pSortCB);

	const UINT NUM_ELEMENTS = g_iNumParticles;
	const UINT MATRIX_WIDTH = BITONIC_BLOCK_SIZE;
	const UINT MATRIX_HEIGHT = NUM_ELEMENTS / BITONIC_BLOCK_SIZE;

	// Sort the data
   // First sort the rows for the levels <= to the block size
	for (UINT level = 2; level <= BITONIC_BLOCK_SIZE; level <<= 1)
	{
		SortCB constants = { level, level, MATRIX_HEIGHT, MATRIX_WIDTH };
		pd3dImmediateContext->UpdateSubresource(g_pSortCB, 0, NULL, &constants, 0, 0);

		// Sort the row data
		UINT UAVInitialCounts = 0;
		pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, &inUAV, &UAVInitialCounts);
		pd3dImmediateContext->CSSetShader(g_pSortBitonic, NULL, 0);
		pd3dImmediateContext->Dispatch(NUM_ELEMENTS / BITONIC_BLOCK_SIZE, 1, 1);
	}
	// Then sort the rows and columns for the levels > than the block size
  // Transpose. Sort the Columns. Transpose. Sort the Rows.
	for (UINT level = (BITONIC_BLOCK_SIZE << 1); level <= NUM_ELEMENTS; level <<= 1)
	{
		SortCB constants1 = { (level / BITONIC_BLOCK_SIZE), (level & ~NUM_ELEMENTS) / BITONIC_BLOCK_SIZE, MATRIX_WIDTH, MATRIX_HEIGHT };
		pd3dImmediateContext->UpdateSubresource(g_pSortCB, 0, NULL, &constants1, 0, 0);

		// Transpose the data from buffer 1 into buffer 2
		ID3D11ShaderResourceView* pViewNULL = NULL;
		UINT UAVInitialCounts = 0;
		pd3dImmediateContext->CSSetShaderResources(0, 1, &pViewNULL);
		pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, &tempUAV, &UAVInitialCounts);
		pd3dImmediateContext->CSSetShaderResources(0, 1, &inSRV);
		pd3dImmediateContext->CSSetShader(g_pSortTranspose, NULL, 0);
		pd3dImmediateContext->Dispatch(MATRIX_WIDTH / TRANSPOSE_BLOCK_SIZE, MATRIX_HEIGHT / TRANSPOSE_BLOCK_SIZE, 1);

		// Sort the transposed column data
		pd3dImmediateContext->CSSetShader(g_pSortBitonic, NULL, 0);
		pd3dImmediateContext->Dispatch(NUM_ELEMENTS / BITONIC_BLOCK_SIZE, 1, 1);

		SortCB constants2 = { BITONIC_BLOCK_SIZE, level, MATRIX_HEIGHT, MATRIX_WIDTH };
		pd3dImmediateContext->UpdateSubresource(g_pSortCB, 0, NULL, &constants2, 0, 0);

		// Transpose the data from buffer 2 back into buffer 1
		pd3dImmediateContext->CSSetShaderResources(0, 1, &pViewNULL);
		pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, &inUAV, &UAVInitialCounts);
		pd3dImmediateContext->CSSetShaderResources(0, 1, &tempSRV);
		pd3dImmediateContext->CSSetShader(g_pSortTranspose, NULL, 0);
		pd3dImmediateContext->Dispatch(MATRIX_HEIGHT / TRANSPOSE_BLOCK_SIZE, MATRIX_WIDTH / TRANSPOSE_BLOCK_SIZE, 1);

		// Sort the row data
		pd3dImmediateContext->CSSetShader(g_pSortBitonic, NULL, 0);
		pd3dImmediateContext->Dispatch(NUM_ELEMENTS / BITONIC_BLOCK_SIZE, 1, 1);
	}
}

//--------------------------------------------------------------------------------------
// GPU Fluid Simulation - Simple N^2 Algorithm
//--------------------------------------------------------------------------------------
void Fluid2D::SimulateFluid_Simple(ID3D11DeviceContext* pd3dImmediateContext)
{
	UINT UAVInitialCounts = 0;

	// Setup
	pd3dImmediateContext->CSSetConstantBuffers(0, 1, &g_pcbSimulationConstants);
	pd3dImmediateContext->CSSetShaderResources(0, 1, &g_pParticlesSRV);

	// Density
	pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, &g_pParticleDensityUAV, &UAVInitialCounts);
	pd3dImmediateContext->CSSetShader(g_pDensity_SimpleCS, NULL, 0);
	pd3dImmediateContext->Dispatch(g_iNumParticles / SIMULATION_BLOCK_SIZE, 1, 1);

	// Force
	pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, &g_pParticleForcesUAV, &UAVInitialCounts);
	pd3dImmediateContext->CSSetShaderResources(1, 1, &g_pParticleDensitySRV);
	pd3dImmediateContext->CSSetShader(g_pForce_SimpleCS, NULL, 0);
	pd3dImmediateContext->Dispatch(g_iNumParticles / SIMULATION_BLOCK_SIZE, 1, 1);

	// Integrate
	pd3dImmediateContext->CopyResource(g_pSortedParticles, g_pParticles);
	pd3dImmediateContext->CSSetShaderResources(0, 1, &g_pSortedParticlesSRV);
	pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, &g_pParticlesUAV, &UAVInitialCounts);
	pd3dImmediateContext->CSSetShaderResources(2, 1, &g_pParticleForcesSRV);
	pd3dImmediateContext->CSSetShader(g_pIntegrateCS, NULL, 0);
	pd3dImmediateContext->Dispatch(g_iNumParticles / SIMULATION_BLOCK_SIZE, 1, 1);

}

//--------------------------------------------------------------------------------------
// GPU Fluid Simulation - Optimized N^2 Algorithm using Shared Memory
//--------------------------------------------------------------------------------------
void Fluid2D::SimulateFluid_Shared(ID3D11DeviceContext* pd3dImmediateContext)
{
	UINT UAVInitialCounts = 0;

	// Setup
	pd3dImmediateContext->CSSetConstantBuffers(0, 1, &g_pcbSimulationConstants);
	pd3dImmediateContext->CSSetShaderResources(0, 1, &g_pParticlesSRV);

	// Density
	pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, &g_pParticleDensityUAV, &UAVInitialCounts);
	pd3dImmediateContext->CSSetShader(g_pDensity_SharedCS, NULL, 0);
	pd3dImmediateContext->Dispatch(g_iNumParticles / SIMULATION_BLOCK_SIZE, 1, 1);

	// Force
	pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, &g_pParticleForcesUAV, &UAVInitialCounts);
	pd3dImmediateContext->CSSetShaderResources(1, 1, &g_pParticleDensitySRV);
	pd3dImmediateContext->CSSetShader(g_pForce_SharedCS, NULL, 0);
	pd3dImmediateContext->Dispatch(g_iNumParticles / SIMULATION_BLOCK_SIZE, 1, 1);

	// Integrate
	pd3dImmediateContext->CopyResource(g_pSortedParticles, g_pParticles);
	pd3dImmediateContext->CSSetShaderResources(0, 1, &g_pSortedParticlesSRV);
	pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, &g_pParticlesUAV, &UAVInitialCounts);
	pd3dImmediateContext->CSSetShaderResources(2, 1, &g_pParticleForcesSRV);
	pd3dImmediateContext->CSSetShader(g_pIntegrateCS, NULL, 0);
	pd3dImmediateContext->Dispatch(g_iNumParticles / SIMULATION_BLOCK_SIZE, 1, 1);
}
//--------------------------------------------------------------------------------------
// GPU Fluid Simulation - Optimized Algorithm using a Grid + Sort
// Algorithm Overview:
//    Build Grid: For every particle, calculate a hash based on the grid cell it is in
//    Sort Grid: Sort all of the particles based on the grid ID hash
//        Particles in the same cell will now be adjacent in memory
//    Build Grid Indices: Located the start and end offsets for each cell
//    Rearrange: Rearrange the particles into the same order as the grid for easy lookup
//    Density, Force, Integrate: Perform the normal fluid simulation algorithm
//        Except now, only calculate particles from the 8 adjacent cells + current cell
//--------------------------------------------------------------------------------------
void Fluid2D::SimulateFluid_Grid(ID3D11DeviceContext* pd3dImmediateContext)
{
	UINT UAVInitialCounts = 0;

	// Setup
	pd3dImmediateContext->CSSetConstantBuffers(0, 1, &g_pcbSimulationConstants);
	pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, &g_pGridUAV, &UAVInitialCounts);
	pd3dImmediateContext->CSSetShaderResources(0, 1, &g_pParticlesSRV);

	// Build Grid
	pd3dImmediateContext->CSSetShader(g_pBuildGridCS, NULL, 0);
	pd3dImmediateContext->Dispatch(g_iNumParticles / SIMULATION_BLOCK_SIZE, 1, 1);

	// Sort Grid
	GPUSort(pd3dImmediateContext, g_pGridUAV, g_pGridSRV, g_pGridPingPongUAV, g_pGridPingPongSRV);

	// Setup
	pd3dImmediateContext->CSSetConstantBuffers(0, 1, &g_pcbSimulationConstants);
	pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, &g_pGridIndicesUAV, &UAVInitialCounts);
	pd3dImmediateContext->CSSetShaderResources(3, 1, &g_pGridSRV);

	// Build Grid Indices
	pd3dImmediateContext->CSSetShader(g_pClearGridIndicesCS, NULL, 0);
	pd3dImmediateContext->Dispatch(NUM_GRID_INDICES / SIMULATION_BLOCK_SIZE, 1, 1);
	pd3dImmediateContext->CSSetShader(g_pBuildGridIndicesCS, NULL, 0);
	pd3dImmediateContext->Dispatch(g_iNumParticles / SIMULATION_BLOCK_SIZE, 1, 1);

	// Setup
	pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, &g_pSortedParticlesUAV, &UAVInitialCounts);
	pd3dImmediateContext->CSSetShaderResources(0, 1, &g_pParticlesSRV);
	pd3dImmediateContext->CSSetShaderResources(3, 1, &g_pGridSRV);

	// Rearrange
	pd3dImmediateContext->CSSetShader(g_pRearrangeParticlesCS, NULL, 0);
	pd3dImmediateContext->Dispatch(g_iNumParticles / SIMULATION_BLOCK_SIZE, 1, 1);

	// Setup
	pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, &g_pNullUAV, &UAVInitialCounts);
	pd3dImmediateContext->CSSetShaderResources(0, 1, &g_pNullSRV);
	pd3dImmediateContext->CSSetShaderResources(0, 1, &g_pSortedParticlesSRV);
	pd3dImmediateContext->CSSetShaderResources(3, 1, &g_pGridSRV);
	pd3dImmediateContext->CSSetShaderResources(4, 1, &g_pGridIndicesSRV);

	// Density
	pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, &g_pParticleDensityUAV, &UAVInitialCounts);
	pd3dImmediateContext->CSSetShader(g_pDensity_GridCS, NULL, 0);
	pd3dImmediateContext->Dispatch(g_iNumParticles / SIMULATION_BLOCK_SIZE, 1, 1);

	// Force
	pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, &g_pParticleForcesUAV, &UAVInitialCounts);
	pd3dImmediateContext->CSSetShaderResources(1, 1, &g_pParticleDensitySRV);
	pd3dImmediateContext->CSSetShader(g_pForce_GridCS, NULL, 0);
	pd3dImmediateContext->Dispatch(g_iNumParticles / SIMULATION_BLOCK_SIZE, 1, 1);

	// Integrate
	pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, &g_pParticlesUAV, &UAVInitialCounts);
	pd3dImmediateContext->CSSetShaderResources(2, 1, &g_pParticleForcesSRV);
	pd3dImmediateContext->CSSetShader(g_pIntegrateCS, NULL, 0);
	pd3dImmediateContext->Dispatch(g_iNumParticles / SIMULATION_BLOCK_SIZE, 1, 1);

}

//--------------------------------------------------------------------------------------
// GPU Fluid Simulation
//--------------------------------------------------------------------------------------
void Fluid2D::SimulateFluid(ID3D11DeviceContext* pd3dImmediateContext, float fElapsedTime)
{
	UINT UAVInitialCounts = 0;

	// Update per-frame variables
	CBSimulationConstants pData = {};

	// Simulation Constants
	pData.iNumParticles = g_iNumParticles;
	// Clamp the time step when the simulation runs slowly to prevent numerical explosion
	pData.fTimeStep = min(g_fMaxAllowableTimeStep, fElapsedTime);
	pData.fSmoothlen = g_fSmoothlen;
	pData.fPressureStiffness = g_fPressureStiffness;
	pData.fRestDensity = g_fRestDensity;
	pData.fDensityCoef = g_fParticleMass * 315.0f / (64.0f * XM_PI * pow(g_fSmoothlen, 9));
	pData.fGradPressureCoef = g_fParticleMass * -45.0f / (XM_PI * pow(g_fSmoothlen, 6));
	pData.fLapViscosityCoef = g_fParticleMass * g_fViscosity * 45.0f / (XM_PI * pow(g_fSmoothlen, 6));

	pData.vGravity = g_vGravity;

	// Cells are spaced the size of the smoothing length search radius
	// That way we only need to search the 8 adjacent cells + current cell
	//-------------------
	//|     |     |     |    
	//|  c   |     |     |       
	//-------------------
	//|     |     |     | 
	//|     |     |     |       
	//-------------------
	//|     |     |     | 
	//|     |     |     |       
	//-------------------
	pData.vGridDim.x = 1.0f / g_fSmoothlen;
	pData.vGridDim.y = 1.0f / g_fSmoothlen;
	pData.vGridDim.z = 0.0f;
	pData.vGridDim.w = 0.0f;

	// Collision information for the map
	pData.fWallStiffness = g_fWallStiffness;
	pData.vPlanes[0] = g_vPlanes[0];
	pData.vPlanes[1] = g_vPlanes[1];
	pData.vPlanes[2] = g_vPlanes[2];
	pData.vPlanes[3] = g_vPlanes[3];

	pd3dImmediateContext->UpdateSubresource(g_pcbSimulationConstants, 0, NULL, &pData, 0, 0);

	switch (g_eSimMode) {
		// Simple N^2 Algorithm
	case SIM_MODE_SIMPLE:
		SimulateFluid_Simple(pd3dImmediateContext);
		break;

		// Optimized N^2 Algorithm using Shared Memory
	case SIM_MODE_SHARED:
		SimulateFluid_Shared(pd3dImmediateContext);
		break;

		// Optimized Grid + Sort Algorithm
	case SIM_MODE_GRID:
		SimulateFluid_Grid(pd3dImmediateContext);
		break;
	}

	// Unset
	pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, &g_pNullUAV, &UAVInitialCounts);
	pd3dImmediateContext->CSSetShaderResources(0, 1, &g_pNullSRV);
	pd3dImmediateContext->CSSetShaderResources(1, 1, &g_pNullSRV);
	pd3dImmediateContext->CSSetShaderResources(2, 1, &g_pNullSRV);
	pd3dImmediateContext->CSSetShaderResources(3, 1, &g_pNullSRV);
	pd3dImmediateContext->CSSetShaderResources(4, 1, &g_pNullSRV);


}

//--------------------------------------------------------------------------------------
// GPU Fluid Rendering
//--------------------------------------------------------------------------------------
void  Fluid2D::RenderFluid(ID3D11DeviceContext* pd3dImmediateContext, float fElapsedTime)
{
	// Simple orthographic projection to display the entire map
	XMMATRIX mView = XMMatrixTranslation(-g_fMapWidth / 2.0f, -g_fMapHeight / 2.0f, 0);
	XMMATRIX mProjection = XMMatrixOrthographicLH(g_fMapWidth, g_fMapHeight, 0, 1);
	XMMATRIX mViewProjection = mView * mProjection;

	// Update Constants
	CBRenderConstants pData = {};

	XMStoreFloat4x4(&pData.mViewProjection, XMMatrixTranspose(mViewProjection));
	pData.fParticleSize = g_fParticleRenderSize;

	pd3dImmediateContext->UpdateSubresource(g_pcbRenderConstants, 0, NULL, &pData, 0, 0);

	// Set the shaders
	pd3dImmediateContext->VSSetShader(g_pParticlesVS, NULL, 0);
	pd3dImmediateContext->GSSetShader(g_pParticlesGS, NULL, 0);
	pd3dImmediateContext->PSSetShader(g_pParticlesPS, NULL, 0);

	// Set the constant buffers
	pd3dImmediateContext->GSSetConstantBuffers(0, 1, &g_pcbRenderConstants);

	// Setup the particles buffer and IA
	pd3dImmediateContext->VSSetShaderResources(0, 1, &g_pParticlesSRV);
	pd3dImmediateContext->VSSetShaderResources(1, 1, &g_pParticleDensitySRV);
	pd3dImmediateContext->IASetVertexBuffers(0, 1, &g_pNullBuffer, &g_iNullUINT, &g_iNullUINT);
	pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	// Draw the mesh
	pd3dImmediateContext->Draw(g_iNumParticles, 0);

	// Unset the particles buffer
	pd3dImmediateContext->VSSetShaderResources(0, 1, &g_pNullSRV);
	pd3dImmediateContext->VSSetShaderResources(1, 1, &g_pNullSRV);
}