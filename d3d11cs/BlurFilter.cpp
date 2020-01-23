#include "BlurFilter.h"
#define M_PI       3.14159265358979323846

BlurFilter::BlurFilter()
	: mBlurredOutputTexSRV(0), mBlurredOutputTexUAV(0), mWeithtsBuffer(0)
{
	for (int i = 0; i < 11; i++)
	{
		mWeights[i] = 0.0f;
	}
}
BlurFilter::~BlurFilter()
{
	ReleaseCOM(mBlurredOutputTexSRV);
	ReleaseCOM(mBlurredOutputTexUAV);
	ReleaseCOM(mWeithtsBuffer);
}
ID3D11ShaderResourceView* BlurFilter::GetBlurredOutput()
{
	return mBlurredOutputTexSRV;
}
void BlurFilter::SetGaussianWeights(float sigma)
{
	float d = 2.0f*sigma*sigma;

	float weights[11];
	float sum = 0.0f;
	for (int i = 0; i < 11; ++i)
	{
		float x = (float)i;
		weights[i] = 1.0 / (sqrt(2 * M_PI)*sigma)*expf(-pow((x - 11 / 2), 2) / d);

		sum += weights[i];
	}

	// Divide by the sum so all the weights add up to 1.0.
	for (int i = 0; i < 11; ++i)
	{
		weights[i] /= sum;
	}

	memcpy(mWeights, weights, sizeof(weights));
}

void BlurFilter::SetWeights(const float weights[9])
{
	memcpy(mWeights, weights, sizeof(weights));
	mWeights[9] = 0;
	mWeights[10] = 11;
}

void BlurFilter::Init(ID3D11Device* device, UINT width, UINT height, DXGI_FORMAT format)
{
	// Start fresh.
	ReleaseCOM(mBlurredOutputTexSRV);
	ReleaseCOM(mBlurredOutputTexUAV);

	mWidth = width;
	mHeight = height;
	mFormat = format;

	// Note, compressed formats cannot be used for UAV.  We get error like:
	// ERROR: ID3D11Device::CreateTexture2D: The format (0x4d, BC3_UNORM) 
	// cannot be bound as an UnorderedAccessView, or cast to a format that
	// could be bound as an UnorderedAccessView.  Therefore this format 
	// does not support D3D11_BIND_UNORDERED_ACCESS.
	D3D11_TEXTURE2D_DESC blurredTexDesc;
	blurredTexDesc.Width = width;
	blurredTexDesc.Height = height;
	blurredTexDesc.MipLevels = 1;
	blurredTexDesc.ArraySize = 1;
	blurredTexDesc.Format = format;
	blurredTexDesc.SampleDesc.Count = 1;
	blurredTexDesc.SampleDesc.Quality = 0;
	blurredTexDesc.Usage = D3D11_USAGE_DEFAULT;
	blurredTexDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	blurredTexDesc.CPUAccessFlags = 0;
	blurredTexDesc.MiscFlags = 0;

	ID3D11Texture2D* blurredTex = 0;
	HR(device->CreateTexture2D(&blurredTexDesc, 0, &blurredTex));

	//set texture to SRV resources.
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	HR(device->CreateShaderResourceView(blurredTex, &srvDesc, &mBlurredOutputTexSRV));
	//set texture to UAV resources.
	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	uavDesc.Format = format;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;
	HR(device->CreateUnorderedAccessView(blurredTex, &uavDesc, &mBlurredOutputTexUAV));

	// Views save a reference to the texture so we can release our reference.
	ReleaseCOM(blurredTex);
	BuiltFx(device);
	BuildConstantBuffer(device);
}

void BlurFilter::BlurInPlace(ID3D11DeviceContext* dc,
	ID3D11ShaderResourceView* inputSRV,
	ID3D11UnorderedAccessView* inputUAV,
	int blurCount)
{

	SetWeightscBuffer(dc);
	//
	// Run the compute shader to blur the offscreen texture.
	// 
	for (int i = 0; i < blurCount; ++i)
	{
		dc->CSSetShaderResources(0, 1, &inputSRV);
		dc->CSSetUnorderedAccessViews(0, 1, &mBlurredOutputTexUAV, nullptr);

		// How many groups do we need to dispatch to cover a row of pixels, where each
		// group covers 256 pixels (the 256 is defined in the ComputeShader).
		UINT numGroupsX = (UINT)ceilf(mWidth / 256.0f);
		dc->CSSetShader(mHorBlurShader, nullptr, 0);
		dc->Dispatch(numGroupsX, mHeight, 1);

		// Unbind the input texture from the CS for good housekeeping.
		ID3D11ShaderResourceView* nullSRV[1] = { 0 };
		dc->CSSetShaderResources(0, 1, nullSRV);

		// Unbind output from compute shader(we are going to use this output as an input in the next pass,
		// and a resource cannot be both an output and input at the same time.
		ID3D11UnorderedAccessView* nullUAV[1] = { 0 };
		dc->CSSetUnorderedAccessViews(0, 1, nullUAV, 0);

		// VERTICAL blur pass.
		dc->CSSetShaderResources(0, 1, &mBlurredOutputTexSRV);
		dc->CSSetUnorderedAccessViews(0, 1, &inputUAV, nullptr);
		UINT numGroupsY = (UINT)ceilf(mHeight / 256.0f);
		dc->CSSetShader(mVerBlurShader, nullptr, 0);
		dc->Dispatch(mWidth, numGroupsY, 1);

		dc->CSSetShaderResources(0, 1, nullSRV);
		dc->CSSetUnorderedAccessViews(0, 1, nullUAV, 0);
	}
	// Disable compute shader.
	dc->CSSetShader(0, 0, 0);
}

void BlurFilter::SetWeightscBuffer(ID3D11DeviceContext * md3dDeviceContext)
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;

	HR(md3dDeviceContext->Map(mWeithtsBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));

	memcpy((Settings*)mappedResource.pData, mWeights, sizeof(mWeights));

	md3dDeviceContext->Unmap(mWeithtsBuffer, 0);

	md3dDeviceContext->CSSetConstantBuffers(0, 1, &mWeithtsBuffer);
}

void BlurFilter::BuiltFx(ID3D11Device* device)
{
	ID3D10Blob* csBuffer = 0;
	ID3D10Blob* compilationMsgs = 0;
	HRESULT hr = D3DX11CompileFromFile(L".\\shader\\Blur.hlsl", 0, 0, "HorzBlurCS", "cs_5_0", D3D10_SHADER_ENABLE_STRICTNESS,
		0, 0, &csBuffer, &compilationMsgs, 0);

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
	HR(device->CreateComputeShader(csBuffer->GetBufferPointer(), csBuffer->GetBufferSize(),
		0, &mHorBlurShader));
	compilationMsgs = 0;
	hr = D3DX11CompileFromFile(L".\\shader\\Blur.hlsl", 0, 0, "VertBlurCS", "cs_5_0", D3D10_SHADER_ENABLE_STRICTNESS,
		0, 0, &csBuffer, &compilationMsgs, 0);

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
	HR(device->CreateComputeShader(csBuffer->GetBufferPointer(), csBuffer->GetBufferSize(),
		0, &mVerBlurShader));
}

void BlurFilter::BuildConstantBuffer(ID3D11Device * device)
{
	D3D11_BUFFER_DESC weightsBufferDesc;

	weightsBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	weightsBufferDesc.ByteWidth = sizeof(Settings);
	weightsBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	weightsBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	weightsBufferDesc.MiscFlags = 0;
	weightsBufferDesc.StructureByteStride = 0;

	HR(device->CreateBuffer(&weightsBufferDesc, NULL, &mWeithtsBuffer));
}
