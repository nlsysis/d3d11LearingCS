#include "Fluid3DApp.h"
#include <d3dx10math.h>


Fluid3DApp::Fluid3DApp(HINSTANCE hInstance)
	: D3DApp(hInstance), mBoxVB(0), mBoxIB(0), mVertexShader(0), mPixelShader(0), mInputLayout(0),
	mTheta(1.5f*MathHelper::Pi), mPhi(0.25f*MathHelper::Pi), mRadius(200.0f)
{
	mMainWndCaption = L"Box Demo";

	XMMATRIX I = XMMatrixIdentity();
	XMStoreFloat4x4(&mWorld, I);
	XMStoreFloat4x4(&mView, I);
	XMStoreFloat4x4(&mProj, I);

	particleNum_MAX = 4096 * 2;
	wall_min = { -25, 00, -25 };
	wall_max = { 25, 50, 25 };
	fluid_min = { -15, 5, -15 };
	fluid_max = { 15, 35, 15 };
	gravity = { 0.0, -9.8f, 0 };
	
}

Fluid3DApp::~Fluid3DApp()
{
	ReleaseCOM(mBoxVB);
	ReleaseCOM(mBoxIB);
	ReleaseCOM(mPixelShader);
	ReleaseCOM(mVertexShader);
	ReleaseCOM(mInputLayout);
}

bool Fluid3DApp::Init()
{
	if (!D3DApp::Init())
		return false;

	g_fludiMain = new FluidMain();
	g_fludiMain->Init(particleNum_MAX, wall_min, wall_max, fluid_min, fluid_max, gravity);

	BuildGeometryBuffers();
	BuildFX();
	BuildConstantBuffer();
	
	return true;
}

void Fluid3DApp::OnResize()
{
	D3DApp::OnResize();

	// The window resized, so update the aspect ratio and recompute the projection matrix.
	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, GetAspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);
}

void Fluid3DApp::UpdateScene(float dt)
{
	// Convert Spherical to Cartesian coordinates.
	float x = mRadius * sinf(mPhi)*cosf(mTheta);
	float z = mRadius * sinf(mPhi)*sinf(mTheta);
	float y = mRadius * cosf(mPhi);

	// Build the view matrix.
	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX V = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, V);

	
	//update the particle per frame
	g_fludiMain->tick();
	Vertex* pVertices = new Vertex[g_fludiMain->getPointCounts()];

	XMFLOAT3 *p = g_fludiMain->getPointBuf();
	UINT stride = g_fludiMain->getPointStride();

	for (unsigned int n = 0; n < g_fludiMain->getPointCounts(); n++)
	{
		pVertices[n].Pos.x = p->x;
		pVertices[n].Pos.y = p->y;
		pVertices[n].Pos.z = p->z;

		pVertices[n].Color = XMFLOAT4(255, 255, 255,255);

		p = (XMFLOAT3*)(((char*)p) + stride);
	}
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	Vertex*  data;
	HR(md3dDeviceContext->Map(mBoxVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));
	memcpy((Vertex*)mappedResource.pData, pVertices, sizeof(Vertex) * g_fludiMain->getPointCounts());
	/*data = (Vertex*)mappedResource.pData;
	for (unsigned int n = 0; n < g_fludiMain->getPointCounts(); n++)
	{
		data[n] = pVertices[n];
	}*/
	md3dDeviceContext->Unmap(mBoxVB,0);

	UpdateInput(dt);
}


void Fluid3DApp::DrawScene()
{

	md3dDeviceContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Colors::Black));
	md3dDeviceContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	SetShaderParameters();
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	md3dDeviceContext->IASetVertexBuffers(0, 1, &mBoxVB, &stride, &offset);

	md3dDeviceContext->IASetInputLayout(mInputLayout);
	md3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	md3dDeviceContext->VSSetShader(mVertexShader, NULL, 0);
	md3dDeviceContext->PSSetShader(mPixelShader, NULL, 0);
	md3dDeviceContext->Draw(g_fludiMain->getPointCounts(),0);

	HR(mSwapChain->Present(0, 0));
}


void Fluid3DApp::BuildGeometryBuffers()
{
	
	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_DYNAMIC;
	vbd.ByteWidth = sizeof(Vertex)* g_fludiMain->getPointCounts();
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	vbd.MiscFlags = 0;
	HR(md3dDevice->CreateBuffer(&vbd, NULL, &mBoxVB));
}

void Fluid3DApp::BuildFX()
{
	DWORD shaderFlags = 0;
#if defined( DEBUG ) || defined( _DEBUG )
	shaderFlags |= D3D10_SHADER_DEBUG;
	shaderFlags |= D3D10_SHADER_SKIP_OPTIMIZATION;
#endif

	ID3D10Blob* vertexShaderBuffer = 0;
	ID3D10Blob* pixelShaderBuffer = 0;
	ID3D10Blob* compilationMsgs = 0;
	HRESULT hr = D3DX11CompileFromFile(L".\\shader\\color.hlsl", 0, 0, "VS", "vs_5_0", D3D10_SHADER_ENABLE_STRICTNESS,
		0, 0, &vertexShaderBuffer, &compilationMsgs, 0);

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
	compilationMsgs = 0;
	hr = D3DX11CompileFromFile(L".\\shader\\color.hlsl", 0, 0, "PS", "ps_5_0", D3D10_SHADER_ENABLE_STRICTNESS,
		0, 0, &pixelShaderBuffer, &compilationMsgs, 0);

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

	HR(md3dDevice->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(),
		0, &mVertexShader));

	// Create the vertex input layout.
	const D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	// Create the input layout
	unsigned int numElements = sizeof(vertexDesc) / sizeof(vertexDesc[0]);
	HR(md3dDevice->CreateInputLayout(vertexDesc, numElements, vertexShaderBuffer->GetBufferPointer(),
		vertexShaderBuffer->GetBufferSize(), &mInputLayout));

	// Done with compiled shader.
	ReleaseCOM(vertexShaderBuffer);

	HR(md3dDevice->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(),
		0, &mPixelShader));

	// Done with compiled shader.
	ReleaseCOM(pixelShaderBuffer);
}

void Fluid3DApp::BuildConstantBuffer()
{
	D3D11_BUFFER_DESC matrixBufferDesc;

	matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	matrixBufferDesc.ByteWidth = sizeof(MatrixBufferType);
	matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	matrixBufferDesc.MiscFlags = 0;
	matrixBufferDesc.StructureByteStride = 0;

	HR(md3dDevice->CreateBuffer(&matrixBufferDesc, NULL, &mMatrixBuffer));
}

void Fluid3DApp::SetShaderParameters()
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	MatrixBufferType* dataPtr;
	// Set constants
	XMMATRIX world = XMLoadFloat4x4(&mWorld);
	XMMATRIX view = XMLoadFloat4x4(&mView);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);
	XMMATRIX worldViewProj = world * view * proj;

	HR(md3dDeviceContext->Map(mMatrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));

	dataPtr = (MatrixBufferType*)mappedResource.pData;
	dataPtr->mvp = worldViewProj;

	md3dDeviceContext->Unmap(mMatrixBuffer, 0);

	md3dDeviceContext->VSSetConstantBuffers(0, 1, &mMatrixBuffer);

}

void Fluid3DApp::UpdateInput(float dt)
{
	//get mouse state
	DirectX::Mouse::State mouseState = m_pMouse->GetState();
	DirectX::Mouse::State lastMouseState = m_MouseTracker.GetLastState();
	//get keyboard state
	DirectX::Keyboard::State keyState = m_pKeyboard->GetState();
	DirectX::Keyboard::State lastKeyState = m_KeyboardTracker.GetLastState();

	//update mousestate
	m_MouseTracker.Update(mouseState);
	m_KeyboardTracker.Update(keyState);
	m_KeyboardTracker.Update(keyState);
	if (mouseState.leftButton == true && m_MouseTracker.leftButton == m_MouseTracker.HELD)
	{
		mTheta -= (mouseState.x - lastMouseState.x) * 0.01f;
		mPhi -= (mouseState.y - lastMouseState.y) * 0.01f;
	}
	if (keyState.IsKeyDown(DirectX::Keyboard::W))
		mPhi += dt * 2;
	if (keyState.IsKeyDown(DirectX::Keyboard::S))
		mPhi -= dt * 2;
	if (keyState.IsKeyDown(DirectX::Keyboard::A))
		mTheta += dt * 2;
	if (keyState.IsKeyDown(DirectX::Keyboard::D))
		mTheta -= dt * 2;

}