#include "CreateApp.h"

CreateApp::CreateApp(HINSTANCE hInstance)
	: D3DApp(hInstance), mBoxVB(0), mBoxIB(0), mDiffuseMapSRV(0), mEyePosW(0.0f, 0.0f, 0.0f),
	mTheta(1.3f*MathHelper::Pi), mPhi(0.4f*MathHelper::Pi), mRadius(2.5f)
{
	mMainWndCaption = L"Crate Demo";

	mLastMousePos.x = 0;
	mLastMousePos.y = 0;

	XMMATRIX I = XMMatrixIdentity();
	XMStoreFloat4x4(&mBoxWorld, I);
	XMStoreFloat4x4(&mTexTransform, I);
	XMStoreFloat4x4(&mView, I);
	XMStoreFloat4x4(&mProj, I);

	mDirLights[0].Ambient = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	mDirLights[0].Diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	mDirLights[0].Specular = XMFLOAT4(0.6f, 0.6f, 0.6f, 16.0f);
	mDirLights[0].Direction = XMFLOAT3(0.707f, -0.707f, 0.0f);

	mDirLights[1].Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	mDirLights[1].Diffuse = XMFLOAT4(1.4f, 1.4f, 1.4f, 1.0f);
	mDirLights[1].Specular = XMFLOAT4(0.3f, 0.3f, 0.3f, 16.0f);
	mDirLights[1].Direction = XMFLOAT3(-0.707f, 0.0f, 0.707f);

	mBoxMat.Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mBoxMat.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	mBoxMat.Specular = XMFLOAT4(0.6f, 0.6f, 0.6f, 16.0f);

	mEyePosW = XMFLOAT3(0.0f, 0.0f, -5.0f);
}

CreateApp::~CreateApp()
{
	ReleaseCOM(mBoxVB);
	ReleaseCOM(mBoxIB);
	ReleaseCOM(mDiffuseMapSRV);

	ReleaseCOM(mPixelShader);
	ReleaseCOM(mVertexShader);
	ReleaseCOM(mInputLayout);
	ReleaseCOM(mSampleState);
}

bool CreateApp::Init()
{
	if (!D3DApp::Init())
		return false;


	HR(D3DX11CreateShaderResourceViewFromFile(md3dDevice,
		L".\\Textures\\WoodCrate01.dds", 0, 0, &mDiffuseMapSRV, 0));

	BuildGeometryBuffers();
	BuildFX();
	BuildConstantBuffer();
	BuildSampler();

	return true;
}

void CreateApp::OnResize()
{
	D3DApp::OnResize();

	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, GetAspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);
}

void CreateApp::UpdateScene(float dt)
{
	// Convert Spherical to Cartesian coordinates.
	float x = mRadius * sinf(mPhi)*cosf(mTheta);
	float z = mRadius * sinf(mPhi)*sinf(mTheta);
	float y = mRadius * cosf(mPhi);

	mEyePosW = XMFLOAT3(x, y, z);

	// Build the view matrix.
	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX V = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, V);
}

void CreateApp::DrawScene()
{
	md3dDeviceContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Colors::LightSteelBlue));
	md3dDeviceContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	md3dDeviceContext->IASetInputLayout(mInputLayout);
	md3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	md3dDeviceContext->VSSetShader(mVertexShader, NULL, 0);
	md3dDeviceContext->PSSetShader(mPixelShader, NULL, 0);

	// Set per frame constants.
	SetLightParameters();

	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	md3dDeviceContext->IASetVertexBuffers(0, 1, &mBoxVB, &stride, &offset);
	md3dDeviceContext->IASetIndexBuffer(mBoxIB, DXGI_FORMAT_R32_UINT, 0);

	SetShaderParameters(mBoxWorld, mBoxMat);

	md3dDeviceContext->PSSetSamplers(0, 1, &mSampleState);
	md3dDeviceContext->PSSetShaderResources(1, 1, &mDiffuseMapSRV);
	

	md3dDeviceContext->DrawIndexed(mBoxIndexCount, mBoxIndexOffset, mBoxVertexOffset);

	HR(mSwapChain->Present(0, 0));
}

void CreateApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void CreateApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void CreateApp::OnMouseMove(WPARAM btnState, int x, int y)
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
		// Make each pixel correspond to 0.01 unit in the scene.
		float dx = 0.01f*static_cast<float>(x - mLastMousePos.x);
		float dy = 0.01f*static_cast<float>(y - mLastMousePos.y);

		// Update the camera radius based on input.
		mRadius += dx - dy;

		// Restrict the radius.
		mRadius = MathHelper::Clamp(mRadius, 1.0f, 15.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void CreateApp::BuildGeometryBuffers()
{
	GeometryGenerator::MeshData box;

	GeometryGenerator geoGen;
	geoGen.CreateBox(1.0f, 1.0f, 1.0f, box);

	// Cache the vertex offsets to each object in the concatenated vertex buffer.
	mBoxVertexOffset = 0;

	// Cache the index count of each object.
	mBoxIndexCount = box.Indices.size();

	// Cache the starting index for each object in the concatenated index buffer.
	mBoxIndexOffset = 0;

	UINT totalVertexCount = box.Vertices.size();

	UINT totalIndexCount = mBoxIndexCount;

	//
	// Extract the vertex elements we are interested in and pack the
	// vertices of all the meshes into one vertex buffer.
	//

	std::vector<Vertex> vertices(totalVertexCount);

	UINT k = 0;
	for (size_t i = 0; i < box.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = box.Vertices[i].Position;
		vertices[k].Normal = box.Vertices[i].Normal;
		vertices[k].Tex = box.Vertices[i].TexC;
	}

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex) * totalVertexCount;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = &vertices[0];
	HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mBoxVB));

	//
	// Pack the indices of all the meshes into one index buffer.
	//

	std::vector<UINT> indices;
	indices.insert(indices.end(), box.Indices.begin(), box.Indices.end());

	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * totalIndexCount;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = &indices[0];
	HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mBoxIB));
}
void CreateApp::SetShaderParameters(const XMFLOAT4X4 worldMatrix, const Material material)
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	MatrixBufferType* dataPtr;
	// Set constants
	XMMATRIX world = XMLoadFloat4x4(&worldMatrix);
	XMMATRIX view = XMLoadFloat4x4(&mView);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);
	XMMATRIX worldViewProj = world * view * proj;
	XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
	XMMATRIX texTransform = XMLoadFloat4x4(&mTexTransform);

	HR(md3dDeviceContext->Map(mMatrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));

	dataPtr = (MatrixBufferType*)mappedResource.pData;
	dataPtr->world = world;
	dataPtr->worldInvTranspose = worldInvTranspose;
	dataPtr->mvp = worldViewProj;
	dataPtr->texTransform = texTransform;
	md3dDeviceContext->Unmap(mMatrixBuffer, 0);

	md3dDeviceContext->VSSetConstantBuffers(0, 1, &mMatrixBuffer);

	//setup pixel constant buffer
	EyeMaterialType* dataPtr2;
	HR(md3dDeviceContext->Map(mEyeMaterialBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));
	dataPtr2 = (EyeMaterialType*)mappedResource.pData;
	dataPtr2->gMaterial = material;
	dataPtr2->gEyePosW = mEyePosW;
	md3dDeviceContext->PSSetConstantBuffers(1, 1, &mEyeMaterialBuffer);

}
void CreateApp::BuildFX()
{
	ID3D10Blob* vertexShaderBuffer = 0;
	ID3D10Blob* pixelShaderBuffer = 0;
	ID3D10Blob* compilationMsgs = 0;
	HRESULT hr = D3DX11CompileFromFile(L".\\shader\\LitSkull.hlsl", 0, 0, "VS", "vs_5_0", D3D10_SHADER_ENABLE_STRICTNESS,
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
	hr = D3DX11CompileFromFile(L".\\shader\\LitSkull.hlsl", 0, 0, "PS", "ps_5_0", D3D10_SHADER_ENABLE_STRICTNESS,
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
		{"NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0}
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

void CreateApp::BuildConstantBuffer()
{
	D3D11_BUFFER_DESC constantBufferDesc;
	ZeroMemory(&constantBufferDesc, sizeof(D3D11_BUFFER_DESC));
	constantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	constantBufferDesc.ByteWidth = sizeof(MatrixBufferType);
	constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	constantBufferDesc.MiscFlags = 0;
	constantBufferDesc.StructureByteStride = 0;
	HR(md3dDevice->CreateBuffer(&constantBufferDesc, NULL, &mMatrixBuffer));


	constantBufferDesc.ByteWidth = sizeof(EyeMaterialType);
	HR(md3dDevice->CreateBuffer(&constantBufferDesc, NULL, &mEyeMaterialBuffer));

	//create structured buffer SRV.
	{
		D3D11_BUFFER_DESC structuredBufferDesc;
		ZeroMemory(&structuredBufferDesc, sizeof(D3D11_BUFFER_DESC));
		structuredBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		structuredBufferDesc.ByteWidth = sizeof(DirectionalLight) * 2;
		structuredBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;                //structed buffer bind type
		structuredBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		structuredBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;     //enables a resource as a structured buffer
		structuredBufferDesc.StructureByteStride = sizeof(DirectionalLight);
		HR(md3dDevice->CreateBuffer(&structuredBufferDesc, NULL, &mDirLightBuffer));

		D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
		ZeroMemory(&shaderResourceViewDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
		shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;      //it is SRV for structured buffer
		shaderResourceViewDesc.Format = DXGI_FORMAT_UNKNOWN;      //it is SRV for structured buffer
		shaderResourceViewDesc.Buffer.FirstElement = 0;
		shaderResourceViewDesc.Buffer.NumElements = 2;           //the nums of buffer
		HR(md3dDevice->CreateShaderResourceView(mDirLightBuffer, &shaderResourceViewDesc, &DirLightResourceView));
	}
}

void CreateApp::SetLightParameters()
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	HR(md3dDeviceContext->Map(mDirLightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));
	memcpy(mappedResource.pData, &mDirLights, sizeof(DirectionalLight) * 2);
	md3dDeviceContext->Unmap(mDirLightBuffer, 0);

	md3dDeviceContext->PSSetShaderResources(0, 1, &DirLightResourceView);
}

void CreateApp::BuildSampler()
{
	D3D11_SAMPLER_DESC samplerDesc;
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	// Create the texture sampler state.
	HR(md3dDevice->CreateSamplerState(&samplerDesc, &mSampleState));
}