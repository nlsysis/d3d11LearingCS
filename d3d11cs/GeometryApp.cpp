#include "GeometryApp.h"
#include <d3dx10math.h>
#include "font/Font2D.h"

GeometryApp::GeometryApp(HINSTANCE hInstance)
	: D3DApp(hInstance), mVertexShader(0), mGeometryShader(0), mPixelShader(0), mInputLayout(0),
	mTheta(1.5f*MathHelper::Pi), mPhi(0.25f*MathHelper::Pi), mRadius(10.0f)
{
	mMainWndCaption = L"Geometry Demo";

	mLastMousePos.x = 0;
	mLastMousePos.y = 0;

	XMMATRIX I = XMMatrixIdentity();
	XMStoreFloat4x4(&mWorld, I);
	XMStoreFloat4x4(&mView, I);
	XMStoreFloat4x4(&mProj, I);
}

GeometryApp::~GeometryApp()
{
	for (int i = 0; i < 7; i++)
	{
		ReleaseCOM(mTriangleVB[i]);
	}
//	ReleaseCOM(mBoxIB);
	ReleaseCOM(mPixelShader);
	ReleaseCOM(mVertexShader);
	ReleaseCOM(mGeometryShader);
	ReleaseCOM(mInputLayout);
}

bool GeometryApp::Init()
{
	if (!D3DApp::Init())
		return false;

	mCurrIndex = 0;

	
	BuildFX();
	BuildConstantBuffer();
	BuildGeometryBuffers();

	Font2D::FontPrint::SetFont(md3dDeviceContext,"TestSS",100.0f,100.0f);
	return true;
}

void GeometryApp::OnResize()
{
	D3DApp::OnResize();

	// The window resized, so update the aspect ratio and recompute the projection matrix.
	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, GetAspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);
}

void GeometryApp::UpdateScene(float dt)
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
	UpdateInput(dt);
}


void GeometryApp::DrawScene()
{

	md3dDeviceContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Colors::LightSteelBlue));
	md3dDeviceContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	SetShaderParameters();
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	//md3dDeviceContext->IASetVertexBuffers(0, 1, &mTriangleVB, &stride, &offset);
//	md3dDeviceContext->IASetIndexBuffer(mBoxIB, DXGI_FORMAT_R32_UINT, 0);
	ID3D11Buffer* nullBuffer = nullptr;
	md3dDeviceContext->SOSetTargets(1, &nullBuffer, &offset);

	md3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	md3dDeviceContext->IASetInputLayout(mInputLayout);
	md3dDeviceContext->VSSetShader(mVertexShader, nullptr, 0);

	md3dDeviceContext->GSSetShader(nullptr, nullptr, 0);

	md3dDeviceContext->RSSetState(nullptr);
	md3dDeviceContext->PSSetShader(mPixelShader, nullptr, 0);
	/*md3dDeviceContext->IASetInputLayout(mInputLayout);
	md3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	md3dDeviceContext->VSSetShader(mVertexShader, NULL, 0);
	md3dDeviceContext->PSSetShader(mPixelShader, NULL, 0);
	md3dDeviceContext->GSSetShader(mGeometryShader, NULL, 0);*/
	if (mCurrIndex == 0)
	{
		md3dDeviceContext->IASetVertexBuffers(0, 1, &mTriangleVB[0], &stride, &offset);
		md3dDeviceContext->Draw(3, 0);
	}
	else
	{
		md3dDeviceContext->IASetVertexBuffers(0, 1, &mTriangleVB[mCurrIndex], &stride, &offset);
		md3dDeviceContext->DrawAuto();
	}

	//draw Font
	md3dDeviceContext->GSSetShader(0, NULL, 0);   //reset the pipeline
	Font2D::FontPrint::DrawFont(md3dDeviceContext, m_orthoMatrix);
	HR(mSwapChain->Present(0, 0));
}
void GeometryApp::UpdateInput(float dt)
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
	// ******************
	// 切换阶数
	//
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	for (int i = 0; i < 7; ++i)
	{
		if (keyState.IsKeyDown((DirectX::Keyboard::Keys)((int)DirectX::Keyboard::D1 + i)))
		{
			mCurrIndex = i;
		}
	}

}
void GeometryApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void GeometryApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void GeometryApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = XMConvertToRadians(0.25f*static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f*static_cast<float>(y - mLastMousePos.y));

		// Update angles based on input to orbit camera around box.
		mTheta += dx;
		mPhi += dy;

		// Restrict the angle mPhi.
		mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
	}
	else if ((btnState & MK_RBUTTON) != 0)
	{
		// Make each pixel correspond to 0.005 unit in the scene.
		float dx = 0.005f*static_cast<float>(x - mLastMousePos.x);
		float dy = 0.005f*static_cast<float>(y - mLastMousePos.y);

		// Update the camera radius based on input.
		mRadius += dx - dy;

		// Restrict the radius.
		mRadius = MathHelper::Clamp(mRadius, 3.0f, 15.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void GeometryApp::BuildGeometryBuffers()
{
	// Create vertex buffer
	Vertex vertices[] =
	{
		{ XMFLOAT3(-1.0f * 3, -0.866f * 3, 0.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(0.0f * 3, 0.866f * 3, 0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(1.0f * 3, -0.866f * 3, 0.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) }
	};

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_DEFAULT;   //allow the stream write by GPU
	vbd.ByteWidth = sizeof(vertices);
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_STREAM_OUTPUT;  //add stream out lable
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;
	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = vertices;
	HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mTriangleVB[0]));
	//init all vertex buffers
	for (int i = 1; i < 7; ++i)
	{
		vbd.ByteWidth *= 3;
		HR(md3dDevice->CreateBuffer(&vbd, nullptr, &mTriangleVB[i]));
		SetStreamOutputSplitedTrianglet(mTriangleVB[i-1], mTriangleVB[i]);
		if (i == 1)
		{
			md3dDeviceContext->Draw(3, 0);
		}
		else
		{
			md3dDeviceContext->DrawAuto();
		}
	}
	
	
	//// Create the index buffer

	//UINT indices[] = {
	//	// front face
	//	0, 1, 2
	//};

	//D3D11_BUFFER_DESC ibd;
	//ibd.Usage = D3D11_USAGE_DEFAULT;
	//ibd.ByteWidth = sizeof(indices);
	//ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	//ibd.CPUAccessFlags = 0;
	//ibd.MiscFlags = 0;
	//ibd.StructureByteStride = 0;
	//D3D11_SUBRESOURCE_DATA iinitData;
	//iinitData.pSysMem = indices;
	//HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mBoxIB));
}

void GeometryApp::BuildFX()
{
	DWORD shaderFlags = 0;
#if defined( DEBUG ) || defined( _DEBUG )
	shaderFlags |= D3D10_SHADER_DEBUG;
	shaderFlags |= D3D10_SHADER_SKIP_OPTIMIZATION;
#endif

	ID3D10Blob* vertexShaderBuffer = 0;
	ID3D10Blob* pixelShaderBuffer = 0;
	ID3D10Blob* geometryShaderBuffer = 0;
	ID3D10Blob* geometryStreamVertexBuffer = 0;
	ID3D10Blob* compilationMsgs = 0;
	HRESULT hr = D3DX11CompileFromFile(L".\\shader\\Geometry.hlsl", 0, 0, "VS", "vs_5_0", D3D10_SHADER_ENABLE_STRICTNESS,
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
	hr = D3DX11CompileFromFile(L".\\shader\\Geometry.hlsl", 0, 0, "PS", "ps_5_0", D3D10_SHADER_ENABLE_STRICTNESS,
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
	D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	HR(md3dDevice->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(),
		0, &mVertexShader));
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

	const D3D11_SO_DECLARATION_ENTRY posColorLayout[] = {
		{ 0, "SV_POSITION", 0, 0, 3, 0 },
		{ 0, "COLOR", 0, 0, 4, 0 }
	};

	hr = D3DX11CompileFromFile(L".\\shader\\Geometry.hlsl", 0, 0, "SoVS", "vs_5_0", D3D10_SHADER_ENABLE_STRICTNESS,
		0, 0, &geometryStreamVertexBuffer, &compilationMsgs, 0);

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
	HR(md3dDevice->CreateVertexShader(geometryStreamVertexBuffer->GetBufferPointer(), geometryStreamVertexBuffer->GetBufferSize(),
		0, &mGeometryVertexShader));

	hr = D3DX11CompileFromFile(L".\\shader\\Geometry.hlsl", 0, 0, "GS", "gs_5_0", D3D10_SHADER_ENABLE_STRICTNESS,
		0, 0, &geometryShaderBuffer, &compilationMsgs, 0);
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
//	HR(md3dDevice->CreateGeometryShader(geometryShaderBuffer->GetBufferPointer(), geometryShaderBuffer->GetBufferSize(),
//		0,&mGeometryShader));
	UINT stridePosColor = sizeof(Vertex) ;
	HR(md3dDevice->CreateGeometryShaderWithStreamOutput(geometryShaderBuffer->GetBufferPointer(),
		geometryShaderBuffer->GetBufferSize(),
		posColorLayout,
		ARRAYSIZE(posColorLayout),
		&stridePosColor,
		1,
		D3D11_SO_NO_RASTERIZED_STREAM,
		nullptr,
		&mGeometryShader));
	
	ReleaseCOM(geometryShaderBuffer);
	
}

void GeometryApp::BuildConstantBuffer()
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

void GeometryApp::SetShaderParameters()
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

void GeometryApp::SetStreamOutputSplitedTrianglet(ID3D11Buffer * vertexBufferIn, ID3D11Buffer * vertexBufferOut)
{
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	ID3D11Buffer *nullBuffer = nullptr;

	//reset the outstream buffer
	md3dDeviceContext->SOSetTargets(1, &nullBuffer, &offset);

	md3dDeviceContext->IASetInputLayout(nullptr);
	md3dDeviceContext->SOSetTargets(0, nullptr, &offset);

	md3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	md3dDeviceContext->IASetInputLayout(mInputLayout);

	md3dDeviceContext->IASetVertexBuffers(0, 1, &vertexBufferIn, &stride, &offset);

	md3dDeviceContext->VSSetShader(mGeometryVertexShader, nullptr, 0);
	md3dDeviceContext->GSSetShader(mGeometryShader, nullptr, 0);

	md3dDeviceContext->SOSetTargets(1, &vertexBufferOut, &offset);

	md3dDeviceContext->RSSetState(nullptr);
	md3dDeviceContext->PSSetShader(nullptr, nullptr, 0);
}