#include "LightingApp.h"

LightingApp::LightingApp(HINSTANCE hInstance)
	: D3DApp(hInstance), mLandVB(0), mLandIB(0), mWavesVB(0), mWavesIB(0),mMatrixBuffer(0),mLightBuffer(0),mMaterialBuffer(0),
	mInputLayout(0), mTheta(1.5f*MathHelper::Pi), mPhi(0.1f*MathHelper::Pi), mRadius(80.0f)
{
	mMainWndCaption = L"Lighting Demo";

	mLastMousePos.x = 0;
	mLastMousePos.y = 0;

	XMMATRIX I = XMMatrixIdentity();
	XMStoreFloat4x4(&mLandWorld, I);
	XMStoreFloat4x4(&mWavesWorld, I);
	XMStoreFloat4x4(&mView, I);
	XMStoreFloat4x4(&mProj, I);

	XMMATRIX wavesOffset = XMMatrixTranslation(0.0f, -3.0f, 0.0f);
	XMStoreFloat4x4(&mWavesWorld, wavesOffset);

	// Directional light.
	mDirLight.Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	mDirLight.Diffuse = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mDirLight.Specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mDirLight.Direction = XMFLOAT3(0.57735f, -0.57735f, 0.57735f);

	// Point light--position is changed every frame to animate in UpdateScene function.
	mPointLight.Ambient = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	mPointLight.Diffuse = XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f);
	mPointLight.Specular = XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f);
	mPointLight.Att = XMFLOAT3(0.0f, 0.1f, 0.0f);
	mPointLight.Range = 25.0f;

	// Spot light--position and direction changed every frame to animate in UpdateScene function.
	mSpotLight.Ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	mSpotLight.Diffuse = XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f);
	mSpotLight.Specular = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	mSpotLight.Att = XMFLOAT3(1.0f, 0.0f, 0.0f);
	mSpotLight.Spot = 96.0f;
	mSpotLight.Range = 10000.0f;

	mLandMat.Ambient = XMFLOAT4(0.48f, 0.77f, 0.46f, 1.0f);
	mLandMat.Diffuse = XMFLOAT4(0.48f, 0.77f, 0.46f, 1.0f);
	mLandMat.Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);

	mWavesMat.Ambient = XMFLOAT4(0.137f, 0.42f, 0.556f, 1.0f);
	mWavesMat.Diffuse = XMFLOAT4(0.137f, 0.42f, 0.556f, 1.0f);
	mWavesMat.Specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 96.0f);

	mLight.dirLight = mDirLight;
	mLight.pointLight = mPointLight;
	mLight.spotLight = mSpotLight;
	mLight.eyePos = XMFLOAT3(0.0f, 0.0f, 0.0f);
}

LightingApp::~LightingApp()
{
	ReleaseCOM(mLandVB);
	ReleaseCOM(mLandIB);
	ReleaseCOM(mWavesVB);
	ReleaseCOM(mWavesIB);
	ReleaseCOM(mPixelShader);
	ReleaseCOM(mVertexShader);
	ReleaseCOM(mInputLayout);
}

bool LightingApp::Init()
{
	if (!D3DApp::Init())
		return false;

	mWaves.Init(160, 160, 1.0f, 0.03f, 3.25f, 0.4f);

	BuildLandGeometryBuffers();
	BuildWaveGeometryBuffers();
	BuildFX();
	BuildConstantBuffer();

	
	
	return true;
}

void LightingApp::OnResize()
{
	D3DApp::OnResize();

	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, GetAspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);
}

void LightingApp::UpdateScene(float dt)
{
	// Convert Spherical to Cartesian coordinates.
	float x = mRadius * sinf(mPhi)*cosf(mTheta);
	float z = mRadius * sinf(mPhi)*sinf(mTheta);
	float y = mRadius * cosf(mPhi);

	mLight.eyePos = XMFLOAT3(x, y, z);

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
	if ((mTimer.TotalTime() - t_base) >= 0.25f)
	{
		t_base += 0.25f;

		DWORD i = 5 + rand() % (mWaves.RowCount() - 10);
		DWORD j = 5 + rand() % (mWaves.ColumnCount() - 10);

		float r = MathHelper::RandF(1.0f, 2.0f);

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
	}

	md3dDeviceContext->Unmap(mWavesVB, 0);

	//
	// Animate the lights.
	//

	// Circle light over the land surface.
	mPointLight.Position.x = 70.0f*cosf(0.2f*mTimer.TotalTime());
	mPointLight.Position.z = 70.0f*sinf(0.2f*mTimer.TotalTime());
	mPointLight.Position.y = MathHelper::Max(GetHillHeight(mPointLight.Position.x,
		mPointLight.Position.z), -3.0f) + 10.0f;


	// The spotlight takes on the camera position and is aimed in the
	// same direction the camera is looking.  In this way, it looks
	// like we are holding a flashlight.
	mSpotLight.Position = mLight.eyePos;
	XMStoreFloat3(&mSpotLight.Direction, XMVector3Normalize(target - pos));
}

void LightingApp::DrawScene()
{
	md3dDeviceContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Colors::LightSteelBlue));
	md3dDeviceContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	md3dDeviceContext->IASetInputLayout(mInputLayout);
	md3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	md3dDeviceContext->VSSetShader(mVertexShader, NULL, 0);
	md3dDeviceContext->PSSetShader(mPixelShader, NULL, 0);


	// Set per frame constants.
	//SetLightParameters();

	//
	// Draw the hills.
	//
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	SetShaderParameters(mLandWorld, mLandMat);
	md3dDeviceContext->IASetVertexBuffers(0, 1, &mLandVB, &stride, &offset);
	md3dDeviceContext->IASetIndexBuffer(mLandIB, DXGI_FORMAT_R32_UINT, 0);

	md3dDeviceContext->DrawIndexed(mLandIndexCount, 0, 0);

	//
	// Draw the waves.
	//
	SetShaderParameters(mWavesWorld, mWavesMat);
	md3dDeviceContext->IASetVertexBuffers(0, 1, &mWavesVB, &stride, &offset);
	md3dDeviceContext->IASetIndexBuffer(mWavesIB, DXGI_FORMAT_R32_UINT, 0);
	md3dDeviceContext->DrawIndexed(3 * mWaves.TriangleCount(), 0, 0);

	HR(mSwapChain->Present(0, 0));
}

void LightingApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void LightingApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void LightingApp::OnMouseMove(WPARAM btnState, int x, int y)
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
		// Make each pixel correspond to 0.2 unit in the scene.
		float dx = 0.2f*static_cast<float>(x - mLastMousePos.x);
		float dy = 0.2f*static_cast<float>(y - mLastMousePos.y);

		// Update the camera radius based on input.
		mRadius += dx - dy;

		// Restrict the radius.
		mRadius = MathHelper::Clamp(mRadius, 50.0f, 500.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

float LightingApp::GetHillHeight(float x, float z)const
{
	return 0.3f*(z*sinf(0.1f*x) + x * cosf(0.1f*z));
}

XMFLOAT3 LightingApp::GetHillNormal(float x, float z)const
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

void LightingApp::BuildLandGeometryBuffers()
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
	for (size_t i = 0; i < grid.Vertices.size(); ++i)
	{
		XMFLOAT3 p = grid.Vertices[i].Position;

		p.y = GetHillHeight(p.x, p.z);

		vertices[i].Pos = p;
		vertices[i].Normal = GetHillNormal(p.x, p.z);
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

void LightingApp::BuildWaveGeometryBuffers()
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

void LightingApp::BuildFX()
{
	ID3D10Blob* vertexShaderBuffer = 0;
	ID3D10Blob* pixelShaderBuffer = 0;
	ID3D10Blob* compilationMsgs = 0;
	HRESULT hr = D3DX11CompileFromFile(L".\\shader\\Light.hlsl", 0, 0, "VS", "vs_5_0", D3D10_SHADER_ENABLE_STRICTNESS,
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
	hr = D3DX11CompileFromFile(L".\\shader\\LightPS.hlsl", 0, 0, "PS", "ps_5_0", D3D10_SHADER_ENABLE_STRICTNESS,
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
		{"NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
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

void LightingApp::BuildConstantBuffer()
{
	D3D11_BUFFER_DESC matrixBufferDesc,lightBufferDesc;
	ZeroMemory(&matrixBufferDesc, sizeof(D3D11_BUFFER_DESC));
	matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	matrixBufferDesc.ByteWidth = sizeof(MatrixBufferType);
	matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	matrixBufferDesc.MiscFlags = 0;
	matrixBufferDesc.StructureByteStride = 0;
	HR(md3dDevice->CreateBuffer(&matrixBufferDesc, NULL, &mMatrixBuffer));

	ZeroMemory(&lightBufferDesc, sizeof(D3D11_BUFFER_DESC));
	lightBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	lightBufferDesc.ByteWidth = sizeof(LightType);
	lightBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	lightBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	lightBufferDesc.MiscFlags = 0;
	lightBufferDesc.StructureByteStride = 0;
	HR(md3dDevice->CreateBuffer(&lightBufferDesc, NULL, &mLightBuffer));

	//{
	//	D3D11_BUFFER_DESC materialBufferDesc;
	//	materialBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	//	materialBufferDesc.ByteWidth = sizeof(Material) * 1;
	//	materialBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;                //structed buffer bind type
	//	materialBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	//	materialBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;     //enables a resource as a structured buffer it must be setted
	//	materialBufferDesc.StructureByteStride = sizeof(Material);               // must be setted
	//	HR(md3dDevice->CreateBuffer(&materialBufferDesc, NULL, &mMaterialBuffer));

	//}
	////bufferrs for light
	//{
	//	D3D11_BUFFER_DESC dirLightBufferDesc, pointLightBufferDesc, spotLightBufferDesc;

	//	dirLightBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	//	dirLightBufferDesc.ByteWidth = sizeof(DirectionalLight) * 1;
	//	dirLightBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;                //structed buffer bind type
	//	dirLightBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	//	dirLightBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;     //enables a resource as a structured buffer
	//	dirLightBufferDesc.StructureByteStride = sizeof(DirectionalLight);
	//	HR(md3dDevice->CreateBuffer(&dirLightBufferDesc, NULL, &mDirLightBuffer));
	//	

	//	pointLightBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	//	pointLightBufferDesc.ByteWidth = sizeof(PointLight) * 1;
	//	pointLightBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	//	pointLightBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	//	pointLightBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	//	pointLightBufferDesc.StructureByteStride = sizeof(PointLight);
	//	HR(md3dDevice->CreateBuffer(&pointLightBufferDesc, NULL, &mPointLightBuffer));

	//	spotLightBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	//	spotLightBufferDesc.ByteWidth = sizeof(SpotLight) * 1;
	//	spotLightBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	//	spotLightBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	//	spotLightBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	//	spotLightBufferDesc.StructureByteStride = sizeof(SpotLight);
	//	HR(md3dDevice->CreateBuffer(&spotLightBufferDesc, NULL, &mSpotLightBuffer));

	//	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	//	ZeroMemory(&shaderResourceViewDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
	//	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;      //it is SRV for structured buffer
	//	shaderResourceViewDesc.Format = DXGI_FORMAT_UNKNOWN;      //it is SRV for structured buffer
	//	shaderResourceViewDesc.Buffer.FirstElement = 0;
	//	shaderResourceViewDesc.Buffer.NumElements = 1;           //the nums of buffer
	//	HR(md3dDevice->CreateShaderResourceView(mDirLightBuffer, &shaderResourceViewDesc, &DirLightResourceView));
	//	HR(md3dDevice->CreateShaderResourceView(mPointLightBuffer, &shaderResourceViewDesc, &PointLightResourceView));
	//	HR(md3dDevice->CreateShaderResourceView(mSpotLightBuffer, &shaderResourceViewDesc, &SpotLightBufferResourceView));
	//	HR(md3dDevice->CreateShaderResourceView(mMaterialBuffer, &shaderResourceViewDesc, &MaterialResourceView));
	//}
}

void LightingApp::SetShaderParameters(const XMFLOAT4X4 worldMatrix,const Material material)
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	MatrixBufferType* dataPtr;
	// Set constants
	XMMATRIX world = XMLoadFloat4x4(&worldMatrix);
	XMMATRIX view = XMLoadFloat4x4(&mView);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);
	XMMATRIX worldViewProj = world * view * proj;
	XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);

	
	HR(md3dDeviceContext->Map(mMatrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));

	dataPtr = (MatrixBufferType*)mappedResource.pData;
	dataPtr->world = world;
	dataPtr->worldInvTranspose = worldInvTranspose;
	dataPtr->mvp = worldViewProj;
	md3dDeviceContext->Unmap(mMatrixBuffer, 0);

	md3dDeviceContext->VSSetConstantBuffers(0, 1, &mMatrixBuffer);
	
	mLight.material = material;
	HR(md3dDeviceContext->Map(mLightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));
	memcpy(mappedResource.pData, &mLight, sizeof(LightType));
	md3dDeviceContext->Unmap(mLightBuffer, 0);
	md3dDeviceContext->PSSetConstantBuffers(1, 1, &mLightBuffer);
	//HR(md3dDeviceContext->Map(mMaterialBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));
	//memcpy(mappedResource.pData, &material, sizeof(material));
	//md3dDeviceContext->Unmap(mMaterialBuffer, 0);
	//md3dDeviceContext->PSSetShaderResources(3,1,&MaterialResourceView);
}

void LightingApp::SetLightParameters()
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	LightType* dataPtr;
	HR(md3dDeviceContext->Map(mLightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));

//	dataPtr = (LightType*)mappedResource.pData;

//	dataPtr->eyePos = mLight.eyePos;
//	dataPtr->padding = 0.0f;
	md3dDeviceContext->Unmap(mLightBuffer, 0);

	md3dDeviceContext->PSSetConstantBuffers(0, 1, &mLightBuffer);

	//HR(md3dDeviceContext->Map(mDirLightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));
	//memcpy(mappedResource.pData, &mDirLight, sizeof(mDirLight));
	//md3dDeviceContext->Unmap(mDirLightBuffer, 0);

	//HR(md3dDeviceContext->Map(mPointLightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));
	//memcpy(mappedResource.pData, &mPointLight, sizeof(mPointLight));
	//md3dDeviceContext->Unmap(mPointLightBuffer, 0);

	//HR(md3dDeviceContext->Map(mSpotLightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));
	//memcpy(mappedResource.pData, &mSpotLight, sizeof(mSpotLight));
	//md3dDeviceContext->Unmap(mSpotLightBuffer, 0);

	//md3dDeviceContext->PSSetShaderResources(0, 1,&DirLightResourceView);
	//md3dDeviceContext->PSSetShaderResources(1, 1, &PointLightResourceView);
	//md3dDeviceContext->PSSetShaderResources(2, 1, &SpotLightBufferResourceView);

}