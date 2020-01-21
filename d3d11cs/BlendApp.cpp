#include "BlendApp.h"

BlendApp::BlendApp(HINSTANCE hInstance)
	: D3DApp(hInstance), mLandVB(0), mLandIB(0), mWavesVB(0), mWavesIB(0), mBoxVB(0), mBoxIB(0), mGrassMapSRV(0), mWavesMapSRV(0), mBoxMapSRV(0),
	mWaterTexOffset(0.0f, 0.0f), mEyePosW(0.0f, 0.0f, 0.0f), mLandIndexCount(0), mRenderOptions(RenderOptions::TexturesAndFog),
	mTheta(1.3f*MathHelper::Pi), mPhi(0.4f*MathHelper::Pi), mRadius(80.0f),mLightCount(0),mFogEnable(0)
{
	mMainWndCaption = L"Blend Demo";
	mEnable4xMsaa = false;

	mLastMousePos.x = 0;
	mLastMousePos.y = 0;

	XMMATRIX I = XMMatrixIdentity();
	XMStoreFloat4x4(&mLandWorld, I);
	XMStoreFloat4x4(&mWavesWorld, I);
	XMStoreFloat4x4(&mView, I);
	XMStoreFloat4x4(&mProj, I);

	XMMATRIX boxScale = XMMatrixScaling(15.0f, 15.0f, 15.0f);
	XMMATRIX boxOffset = XMMatrixTranslation(8.0f, 5.0f, -15.0f);
	XMStoreFloat4x4(&mBoxWorld, boxScale*boxOffset);
	XMStoreFloat4x4(&mBoxTexTransform, XMMatrixIdentity());

	XMMATRIX grassTexScale = XMMatrixScaling(5.0f, 5.0f, 0.0f);
	XMStoreFloat4x4(&mGrassTexTransform, grassTexScale);

	mDirLights[0].Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	mDirLights[0].Diffuse = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mDirLights[0].Specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mDirLights[0].Direction = XMFLOAT3(0.57735f, -0.57735f, 0.57735f);

	mDirLights[1].Ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	mDirLights[1].Diffuse = XMFLOAT4(0.20f, 0.20f, 0.20f, 1.0f);
	mDirLights[1].Specular = XMFLOAT4(0.25f, 0.25f, 0.25f, 1.0f);
	mDirLights[1].Direction = XMFLOAT3(-0.57735f, -0.57735f, 0.57735f);

	mDirLights[2].Ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	mDirLights[2].Diffuse = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	mDirLights[2].Specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	mDirLights[2].Direction = XMFLOAT3(0.0f, -0.707f, -0.707f);

	mLandMat.Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mLandMat.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	mLandMat.Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);

	mWavesMat.Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mWavesMat.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
	mWavesMat.Specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 32.0f);

	mBoxMat.Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mBoxMat.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	mBoxMat.Specular = XMFLOAT4(0.4f, 0.4f, 0.4f, 16.0f);
}

BlendApp::~BlendApp()
{
	ReleaseCOM(mLandVB);
	ReleaseCOM(mLandIB);
	ReleaseCOM(mWavesVB);
	ReleaseCOM(mWavesIB);
	ReleaseCOM(mBoxVB);
	ReleaseCOM(mBoxIB);
	ReleaseCOM(mGrassMapSRV);
	ReleaseCOM(mWavesMapSRV);
	ReleaseCOM(mBoxMapSRV);

	ReleaseCOM(mPixelShader);
	ReleaseCOM(mVertexShader);
	ReleaseCOM(mInputLayout);
	ReleaseCOM(mSampleState);
	RenderStates::DestroyAll();
}

bool BlendApp::Init()
{
	if (!D3DApp::Init())
		return false;

	mWaves.Init(160, 160, 1.0f, 0.03f, 5.0f, 0.3f);

	// Must init Effects first since InputLayouts depend on shader signatures.
	HR(D3DX11CreateShaderResourceViewFromFile(md3dDevice,
		L".\\Textures\\grass.dds", 0, 0, &mGrassMapSRV, 0));

	HR(D3DX11CreateShaderResourceViewFromFile(md3dDevice,
		L".\\Textures\\water2.dds", 0, 0, &mWavesMapSRV, 0));

	HR(D3DX11CreateShaderResourceViewFromFile(md3dDevice,
		L".\\Textures\\WireFence.dds", 0, 0, &mBoxMapSRV, 0));

	BuildLandGeometryBuffers();
	BuildWaveGeometryBuffers();
	BuildCrateGeometryBuffers();

	BuildFX();
	BuildConstantBuffer();
	BuildSampler();
	RenderStates::InitAll(md3dDevice);
	return true;
}

void BlendApp::OnResize()
{
	D3DApp::OnResize();

	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, GetAspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);
}

void BlendApp::UpdateScene(float dt)
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

	//
	// Every quarter second, generate a random wave.
	//
	static float t_base = 0.0f;
	if ((mTimer.TotalTime() - t_base) >= 0.1f)
	{
		t_base += 0.1f;

		DWORD i = 5 + rand() % (mWaves.RowCount() - 10);
		DWORD j = 5 + rand() % (mWaves.ColumnCount() - 10);

		float r = MathHelper::RandF(0.5f, 1.0f);

		mWaves.Disturb(i, j, r);
	}

	mWaves.Update(dt);

	//
	// Update the wave vertex buffer with the new solution.
	//

	D3D11_MAPPED_SUBRESOURCE mappedData;
	HR(md3dDeviceContext->Map(mWavesVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));

	Vertex* v = reinterpret_cast<Vertex*>(mappedData.pData);
	for (UINT i = 0; i < mWaves.VertexCount(); ++i)
	{
		v[i].Pos = mWaves[i];
		v[i].Normal = mWaves.Normal(i);

		// Derive tex-coords in [0,1] from position.
		v[i].Tex.x = 0.5f + mWaves[i].x / mWaves.Width();
		v[i].Tex.y = 0.5f - mWaves[i].z / mWaves.Depth();
	}

	md3dDeviceContext->Unmap(mWavesVB, 0);

	//
	// Animate water texture coordinates.
	//

	// Tile water texture.
	XMMATRIX wavesScale = XMMatrixScaling(5.0f, 5.0f, 0.0f);

	// Translate texture over time.
	mWaterTexOffset.y += 0.05f*dt;
	mWaterTexOffset.x += 0.1f*dt;
	XMMATRIX wavesOffset = XMMatrixTranslation(mWaterTexOffset.x, mWaterTexOffset.y, 0.0f);

	// Combine scale and translation.
	XMStoreFloat4x4(&mWaterTexTransform, wavesScale*wavesOffset);

	//
	// Switch the render mode based in key input.
	//
	if (GetAsyncKeyState('1') & 0x8000)
		mRenderOptions = RenderOptions::Lighting;

	if (GetAsyncKeyState('2') & 0x8000)
		mRenderOptions = RenderOptions::Textures;

	if (GetAsyncKeyState('3') & 0x8000)
		mRenderOptions = RenderOptions::TexturesAndFog;
}

void BlendApp::DrawScene()
{
	md3dDeviceContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Colors::Silver));
	md3dDeviceContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	md3dDeviceContext->IASetInputLayout(mInputLayout);
	md3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	md3dDeviceContext->PSSetSamplers(0, 1, &mSampleState);
	md3dDeviceContext->VSSetShader(mVertexShader, NULL, 0);
	md3dDeviceContext->PSSetShader(mPixelShader, NULL, 0);

	float blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };

	
	// Set per frame constants.
	SetLightParameters();
	switch (mRenderOptions)
	{
	case RenderOptions::Lighting:
		mLightCount = 3;
		mFogEnable = 0;
		break;
	case RenderOptions::Textures:
		mFogEnable = 0;
		break;
	case RenderOptions::TexturesAndFog:
		mFogEnable = 1;
		mLightCount = 3;
		break;
	}


	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	//
	// Draw the box with alpha clipping.
	// 
	{
		md3dDeviceContext->IASetVertexBuffers(0, 1, &mBoxVB, &stride, &offset);
		md3dDeviceContext->IASetIndexBuffer(mBoxIB, DXGI_FORMAT_R32_UINT, 0);

		SetShaderParameters(mBoxWorld, mBoxMat, mBoxTexTransform);
		md3dDeviceContext->PSSetShaderResources(1, 1, &mBoxMapSRV);
		md3dDeviceContext->RSSetState(RenderStates::NoCullRS);
		md3dDeviceContext->DrawIndexed(36, 0, 0);
		// Restore default render state.
		md3dDeviceContext->RSSetState(0);
	}
	

	//
	// Draw the hills and water with texture and fog (no alpha clipping needed).
	//
	{
		//
		// Draw the hills.
		//
		md3dDeviceContext->IASetVertexBuffers(0, 1, &mLandVB, &stride, &offset);
		md3dDeviceContext->IASetIndexBuffer(mLandIB, DXGI_FORMAT_R32_UINT, 0);

		SetShaderParameters(mLandWorld, mLandMat, mGrassTexTransform);
		md3dDeviceContext->PSSetShaderResources(1, 1, &mGrassMapSRV);
		md3dDeviceContext->DrawIndexed(mLandIndexCount, 0, 0);
	}
	{
		//
		// Draw the waves.
		//
		md3dDeviceContext->IASetVertexBuffers(0, 1, &mWavesVB, &stride, &offset);
		md3dDeviceContext->IASetIndexBuffer(mWavesIB, DXGI_FORMAT_R32_UINT, 0);
		SetShaderParameters(mWavesWorld, mWavesMat, mWaterTexTransform);
		md3dDeviceContext->PSSetShaderResources(1, 1, &mWavesMapSRV);
		md3dDeviceContext->OMSetBlendState(RenderStates::TransparentBS, blendFactor, 0xffffffff);
		md3dDeviceContext->DrawIndexed(3 * mWaves.TriangleCount(), 0, 0);
		// Restore default blend state
		md3dDeviceContext->OMSetBlendState(0, blendFactor, 0xffffffff);
	}
		
	HR(mSwapChain->Present(0, 0));
}

void BlendApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void BlendApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void BlendApp::OnMouseMove(WPARAM btnState, int x, int y)
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
		float dx = 0.1f*static_cast<float>(x - mLastMousePos.x);
		float dy = 0.1f*static_cast<float>(y - mLastMousePos.y);

		// Update the camera radius based on input.
		mRadius += dx - dy;

		// Restrict the radius.
		mRadius = MathHelper::Clamp(mRadius, 20.0f, 500.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

float BlendApp::GetHillHeight(float x, float z)const
{
	return 0.3f*(z*sinf(0.1f*x) + x * cosf(0.1f*z));
}

XMFLOAT3 BlendApp::GetHillNormal(float x, float z)const
{
	// n = (-df/dx, 1, -df/dz)
	XMFLOAT3 n(
		-0.03f*z*cosf(0.1f*x) - 0.3f*cosf(0.1f*z),
		1.0f,
		-0.3f*sinf(0.1f*x) + 0.03f*x*sinf(0.1f*z));

	XMVECTOR unitNormal = XMVector3Normalize(XMLoadFloat3(&n));
	XMStoreFloat3(&n, unitNormal);

	return n;
}

void BlendApp::BuildLandGeometryBuffers()
{
	GeometryGenerator::MeshData grid;

	GeometryGenerator geoGen;

	geoGen.CreateGrid(160.0f, 160.0f, 50, 50, grid);

	mLandIndexCount = grid.Indices.size();

	//
	// Extract the vertex elements we are interested and apply the height function to
	// each vertex.  
	//

	std::vector<Vertex> vertices(grid.Vertices.size());
	for (UINT i = 0; i < grid.Vertices.size(); ++i)
	{
		XMFLOAT3 p = grid.Vertices[i].Position;

		p.y = GetHillHeight(p.x, p.z);

		vertices[i].Pos = p;
		vertices[i].Normal = GetHillNormal(p.x, p.z);
		vertices[i].Tex = grid.Vertices[i].TexC;
	}

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex) * grid.Vertices.size();
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = &vertices[0];
	HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mLandVB));

	//
	// Pack the indices of all the meshes into one index buffer.
	//

	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * mLandIndexCount;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = &grid.Indices[0];
	HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mLandIB));
}

void BlendApp::BuildWaveGeometryBuffers()
{
	// Create the vertex buffer.  Note that we allocate space only, as
	// we will be updating the data every time step of the simulation.

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_DYNAMIC;
	vbd.ByteWidth = sizeof(Vertex) * mWaves.VertexCount();
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	vbd.MiscFlags = 0;
	HR(md3dDevice->CreateBuffer(&vbd, 0, &mWavesVB));


	// Create the index buffer.  The index buffer is fixed, so we only 
	// need to create and set once.

	std::vector<UINT> indices(3 * mWaves.TriangleCount()); // 3 indices per face

	// Iterate over each quad.
	UINT m = mWaves.RowCount();
	UINT n = mWaves.ColumnCount();
	int k = 0;
	for (UINT i = 0; i < m - 1; ++i)
	{
		for (DWORD j = 0; j < n - 1; ++j)
		{
			indices[k] = i * n + j;
			indices[k + 1] = i * n + j + 1;
			indices[k + 2] = (i + 1)*n + j;

			indices[k + 3] = (i + 1)*n + j;
			indices[k + 4] = i * n + j + 1;
			indices[k + 5] = (i + 1)*n + j + 1;

			k += 6; // next quad
		}
	}

	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * indices.size();
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = &indices[0];
	HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mWavesIB));
}

void BlendApp::BuildCrateGeometryBuffers()
{
	GeometryGenerator::MeshData box;

	GeometryGenerator geoGen;
	geoGen.CreateBox(1.0f, 1.0f, 1.0f, box);

	//
	// Extract the vertex elements we are interested in and pack the
	// vertices of all the meshes into one vertex buffer.
	//

	std::vector<Vertex> vertices(box.Vertices.size());

	for (UINT i = 0; i < box.Vertices.size(); ++i)
	{
		vertices[i].Pos = box.Vertices[i].Position;
		vertices[i].Normal = box.Vertices[i].Normal;
		vertices[i].Tex = box.Vertices[i].TexC;
	}

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex) * box.Vertices.size();
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = &vertices[0];
	HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mBoxVB));

	//
	// Pack the indices of all the meshes into one index buffer.
	//

	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * box.Indices.size();
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = &box.Indices[0];
	HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mBoxIB));
}
void BlendApp::SetShaderParameters(const XMFLOAT4X4 worldMatrix, const Material material, const XMFLOAT4X4 texTransform)
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	MatrixBufferType* dataPtr;
	// Set constants
	XMMATRIX world = XMLoadFloat4x4(&worldMatrix);
	XMMATRIX view = XMLoadFloat4x4(&mView);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);
	XMMATRIX worldViewProj = world * view * proj;
	XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
	XMMATRIX mtexTransform = XMLoadFloat4x4(&texTransform);

	HR(md3dDeviceContext->Map(mMatrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));

	dataPtr = (MatrixBufferType*)mappedResource.pData;
	dataPtr->world = world;
	dataPtr->worldInvTranspose = worldInvTranspose;
	dataPtr->mvp = worldViewProj;
	dataPtr->texTransform = mtexTransform;
	md3dDeviceContext->Unmap(mMatrixBuffer, 0);

	md3dDeviceContext->VSSetConstantBuffers(0, 1, &mMatrixBuffer);

	//setup pixel constant buffer
	EyeMaterialType* dataPtr2;
	HR(md3dDeviceContext->Map(mEyeMaterialBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));
	dataPtr2 = (EyeMaterialType*)mappedResource.pData;
	dataPtr2->gMaterial = material;
	dataPtr2->gEyePosW = mEyePosW;
	dataPtr2->gFogColor = (const float*)&Colors::Silver;
	dataPtr2->gFogStart = 15.0f;
	dataPtr2->gFogRange = 175.0f;
//	dataPtr2->padding = XMFLOAT3(0.0f,0.0f,0.0f);
	dataPtr2->gLightCount = mLightCount;
	dataPtr2->gFogEnabled = mFogEnable;
	md3dDeviceContext->PSSetConstantBuffers(1, 1, &mEyeMaterialBuffer);

}
void BlendApp::BuildFX()
{
	ID3D10Blob* vertexShaderBuffer = 0;
	ID3D10Blob* pixelShaderBuffer = 0;
	ID3D10Blob* compilationMsgs = 0;
	HRESULT hr = D3DX11CompileFromFile(L".\\shader\\Blending.hlsl", 0, 0, "VS", "vs_5_0", D3D10_SHADER_ENABLE_STRICTNESS,
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
	hr = D3DX11CompileFromFile(L".\\shader\\Blending.hlsl", 0, 0, "PS", "ps_5_0", D3D10_SHADER_ENABLE_STRICTNESS,
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

void BlendApp::BuildConstantBuffer()
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
		structuredBufferDesc.ByteWidth = sizeof(DirectionalLight) * 3;
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
		shaderResourceViewDesc.Buffer.NumElements = 3;           //the nums of buffer
		HR(md3dDevice->CreateShaderResourceView(mDirLightBuffer, &shaderResourceViewDesc, &DirLightResourceView));
	}
}

void BlendApp::SetLightParameters()
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	HR(md3dDeviceContext->Map(mDirLightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));
	memcpy(mappedResource.pData, &mDirLights, sizeof(DirectionalLight) * 3);
	md3dDeviceContext->Unmap(mDirLightBuffer, 0);

	md3dDeviceContext->PSSetShaderResources(0, 1, &DirLightResourceView);
}

void BlendApp::BuildSampler()
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