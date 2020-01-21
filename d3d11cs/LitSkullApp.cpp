#include "LitSkullApp.h"

LitSkullApp::LitSkullApp(HINSTANCE hInstance)
	: D3DApp(hInstance), mShapesVB(0), mShapesIB(0), mSkullVB(0), mSkullIB(0), mSkullIndexCount(0), mLightCount(1),
	mEyePosW(0.0f, 0.0f, 0.0f), mTheta(1.5f*MathHelper::Pi), mPhi(0.1f*MathHelper::Pi), mRadius(15.0f)
{
	mMainWndCaption = L"LitSkull Demo";

	mLastMousePos.x = 0;
	mLastMousePos.y = 0;

	XMMATRIX I = XMMatrixIdentity();
	XMStoreFloat4x4(&mGridWorld, I);
	XMStoreFloat4x4(&mView, I);
	XMStoreFloat4x4(&mProj, I);

	XMMATRIX boxScale = XMMatrixScaling(3.0f, 1.0f, 3.0f);
	XMMATRIX boxOffset = XMMatrixTranslation(0.0f, 0.5f, 0.0f);
	XMStoreFloat4x4(&mBoxWorld, XMMatrixMultiply(boxScale, boxOffset));

	XMMATRIX skullScale = XMMatrixScaling(0.5f, 0.5f, 0.5f);
	XMMATRIX skullOffset = XMMatrixTranslation(0.0f, 1.0f, 0.0f);
	XMStoreFloat4x4(&mSkullWorld, XMMatrixMultiply(skullScale, skullOffset));

	for (int i = 0; i < 5; ++i)
	{
		XMStoreFloat4x4(&mCylWorld[i * 2 + 0], XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i * 5.0f));
		XMStoreFloat4x4(&mCylWorld[i * 2 + 1], XMMatrixTranslation(+5.0f, 1.5f, -10.0f + i * 5.0f));

		XMStoreFloat4x4(&mSphereWorld[i * 2 + 0], XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i * 5.0f));
		XMStoreFloat4x4(&mSphereWorld[i * 2 + 1], XMMatrixTranslation(+5.0f, 3.5f, -10.0f + i * 5.0f));
	}

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

	mGridMat.Ambient = XMFLOAT4(0.48f, 0.77f, 0.46f, 1.0f);
	mGridMat.Diffuse = XMFLOAT4(0.48f, 0.77f, 0.46f, 1.0f);
	mGridMat.Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);

	mCylinderMat.Ambient = XMFLOAT4(0.7f, 0.85f, 0.7f, 1.0f);
	mCylinderMat.Diffuse = XMFLOAT4(0.7f, 0.85f, 0.7f, 1.0f);
	mCylinderMat.Specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);

	mSphereMat.Ambient = XMFLOAT4(0.1f, 0.2f, 0.3f, 1.0f);
	mSphereMat.Diffuse = XMFLOAT4(0.2f, 0.4f, 0.6f, 1.0f);
	mSphereMat.Specular = XMFLOAT4(0.9f, 0.9f, 0.9f, 16.0f);

	mBoxMat.Ambient = XMFLOAT4(0.651f, 0.5f, 0.392f, 1.0f);
	mBoxMat.Diffuse = XMFLOAT4(0.651f, 0.5f, 0.392f, 1.0f);
	mBoxMat.Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);

	mSkullMat.Ambient = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	mSkullMat.Diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	mSkullMat.Specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);
}

LitSkullApp::~LitSkullApp()
{
	ReleaseCOM(mShapesVB);
	ReleaseCOM(mShapesIB);
	ReleaseCOM(mSkullVB);
	ReleaseCOM(mSkullIB);

	ReleaseCOM(mPixelShader);
	ReleaseCOM(mVertexShader);
	ReleaseCOM(mInputLayout);
}

bool LitSkullApp::Init()
{
	if (!D3DApp::Init())
		return false;

	BuildFX();
	BuildConstantBuffer();
	BuildShapeGeometryBuffers();
	BuildSkullGeometryBuffers();

	return true;
}

void LitSkullApp::OnResize()
{
	D3DApp::OnResize();

	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, GetAspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);
}

void LitSkullApp::UpdateScene(float dt)
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
	// Switch the number of lights based on key presses.
	//
	if (GetAsyncKeyState('0') & 0x8000)
		mLightCount = 0;

	if (GetAsyncKeyState('1') & 0x8000)
		mLightCount = 1;

	if (GetAsyncKeyState('2') & 0x8000)
		mLightCount = 2;

	if (GetAsyncKeyState('3') & 0x8000)
		mLightCount = 3;
}

void LitSkullApp::DrawScene()
{
	md3dDeviceContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Colors::LightSteelBlue));
	md3dDeviceContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	md3dDeviceContext->IASetInputLayout(mInputLayout);
	md3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	md3dDeviceContext->VSSetShader(mVertexShader, NULL, 0);
	md3dDeviceContext->PSSetShader(mPixelShader, NULL, 0);

	SetLightParameters();

	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	
	md3dDeviceContext->IASetVertexBuffers(0, 1, &mShapesVB, &stride, &offset);
	md3dDeviceContext->IASetIndexBuffer(mShapesIB, DXGI_FORMAT_R32_UINT, 0);
	// Draw the grid.
	SetShaderParameters(mGridWorld, mGridMat);
	md3dDeviceContext->DrawIndexed(mGridIndexCount, mGridIndexOffset, mGridVertexOffset);

	// Draw the box.
	SetShaderParameters(mBoxWorld, mBoxMat);
	md3dDeviceContext->DrawIndexed(mBoxIndexCount, mBoxIndexOffset, mBoxVertexOffset);

	// Draw the cylinders.
	for (int i = 0; i < 10; ++i)
	{
		SetShaderParameters(mCylWorld[i], mCylinderMat);
		md3dDeviceContext->DrawIndexed(mCylinderIndexCount, mCylinderIndexOffset, mCylinderVertexOffset);
	}
		
	// Draw the spheres.
	for (int i = 0; i < 10; ++i)
	{
		SetShaderParameters(mSphereWorld[i], mSphereMat);
		md3dDeviceContext->DrawIndexed(mSphereIndexCount, mSphereIndexOffset, mSphereVertexOffset);
	}

	// Draw the skull.
	md3dDeviceContext->IASetVertexBuffers(0, 1, &mSkullVB, &stride, &offset);
	md3dDeviceContext->IASetIndexBuffer(mSkullIB, DXGI_FORMAT_R32_UINT, 0);
	SetShaderParameters(mSkullWorld, mSkullMat);
	md3dDeviceContext->DrawIndexed(mSkullIndexCount, 0, 0);

	HR(mSwapChain->Present(0, 0));
}

void LitSkullApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void LitSkullApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void LitSkullApp::OnMouseMove(WPARAM btnState, int x, int y)
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
		mRadius = MathHelper::Clamp(mRadius, 3.0f, 200.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void LitSkullApp::BuildShapeGeometryBuffers()
{
	GeometryGenerator::MeshData box;
	GeometryGenerator::MeshData grid;
	GeometryGenerator::MeshData sphere;
	GeometryGenerator::MeshData cylinder;

	GeometryGenerator geoGen;
	geoGen.CreateBox(1.0f, 1.0f, 1.0f, box);
	geoGen.CreateGrid(20.0f, 30.0f, 60, 40, grid);
	geoGen.CreateSphere(0.5f, 20, 20, sphere);
	geoGen.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20, cylinder);

	// Cache the vertex offsets to each object in the concatenated vertex buffer.
	mBoxVertexOffset = 0;
	mGridVertexOffset = box.Vertices.size();
	mSphereVertexOffset = mGridVertexOffset + grid.Vertices.size();
	mCylinderVertexOffset = mSphereVertexOffset + sphere.Vertices.size();

	// Cache the index count of each object.
	mBoxIndexCount = box.Indices.size();
	mGridIndexCount = grid.Indices.size();
	mSphereIndexCount = sphere.Indices.size();
	mCylinderIndexCount = cylinder.Indices.size();

	// Cache the starting index for each object in the concatenated index buffer.
	mBoxIndexOffset = 0;
	mGridIndexOffset = mBoxIndexCount;
	mSphereIndexOffset = mGridIndexOffset + mGridIndexCount;
	mCylinderIndexOffset = mSphereIndexOffset + mSphereIndexCount;

	UINT totalVertexCount =
		box.Vertices.size() +
		grid.Vertices.size() +
		sphere.Vertices.size() +
		cylinder.Vertices.size();

	UINT totalIndexCount =
		mBoxIndexCount +
		mGridIndexCount +
		mSphereIndexCount +
		mCylinderIndexCount;

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
	}

	for (size_t i = 0; i < grid.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = grid.Vertices[i].Position;
		vertices[k].Normal = grid.Vertices[i].Normal;
	}

	for (size_t i = 0; i < sphere.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = sphere.Vertices[i].Position;
		vertices[k].Normal = sphere.Vertices[i].Normal;
	}

	for (size_t i = 0; i < cylinder.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = cylinder.Vertices[i].Position;
		vertices[k].Normal = cylinder.Vertices[i].Normal;
	}

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex) * totalVertexCount;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = &vertices[0];
	HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mShapesVB));

	//
	// Pack the indices of all the meshes into one index buffer.
	//

	std::vector<UINT> indices;
	indices.insert(indices.end(), box.Indices.begin(), box.Indices.end());
	indices.insert(indices.end(), grid.Indices.begin(), grid.Indices.end());
	indices.insert(indices.end(), sphere.Indices.begin(), sphere.Indices.end());
	indices.insert(indices.end(), cylinder.Indices.begin(), cylinder.Indices.end());

	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * totalIndexCount;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = &indices[0];
	HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mShapesIB));
}

void LitSkullApp::BuildSkullGeometryBuffers()
{
	std::ifstream fin("Models/skull.txt");

	if (!fin)
	{
		MessageBox(0, L"Models/skull.txt not found.", 0, 0);
		return;
	}

	UINT vcount = 0;
	UINT tcount = 0;
	std::string ignore;

	fin >> ignore >> vcount;
	fin >> ignore >> tcount;
	fin >> ignore >> ignore >> ignore >> ignore;

	std::vector<Vertex> vertices(vcount);
	for (UINT i = 0; i < vcount; ++i)
	{
		fin >> vertices[i].Pos.x >> vertices[i].Pos.y >> vertices[i].Pos.z;
		fin >> vertices[i].Normal.x >> vertices[i].Normal.y >> vertices[i].Normal.z;
	}

	fin >> ignore;
	fin >> ignore;
	fin >> ignore;

	mSkullIndexCount = 3 * tcount;
	std::vector<UINT> indices(mSkullIndexCount);
	for (UINT i = 0; i < tcount; ++i)
	{
		fin >> indices[i * 3 + 0] >> indices[i * 3 + 1] >> indices[i * 3 + 2];
	}

	fin.close();

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex) * vcount;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = &vertices[0];
	HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mSkullVB));

	//
	// Pack the indices of all the meshes into one index buffer.
	//

	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * mSkullIndexCount;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = &indices[0];
	HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mSkullIB));
}

void LitSkullApp::SetShaderParameters(const XMFLOAT4X4 worldMatrix, const Material material)
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

	//setup pixel constant buffer
	EyeMaterialType* dataPtr2;
	HR(md3dDeviceContext->Map(mEyeMaterialBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));
	dataPtr2 = (EyeMaterialType*)mappedResource.pData;
	dataPtr2->gMaterial = material;
	dataPtr2->gEyePosW = mEyePosW;
	md3dDeviceContext->PSSetConstantBuffers(1, 1, &mEyeMaterialBuffer);

}
void LitSkullApp::BuildFX()
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

void LitSkullApp::BuildConstantBuffer()
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

void LitSkullApp::SetLightParameters()
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	HR(md3dDeviceContext->Map(mDirLightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));
	memcpy(mappedResource.pData, &mDirLights, sizeof(DirectionalLight) * 3);
	md3dDeviceContext->Unmap(mDirLightBuffer, 0);

	md3dDeviceContext->PSSetShaderResources(0, 1, &DirLightResourceView);
}