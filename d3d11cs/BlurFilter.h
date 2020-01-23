#pragma once
#include <Windows.h>
#include <xnamath.h>
#include "d3dUtil.h"

class BlurFilter
{
private:
	struct Settings
	{
		float gWeights[11];
		float padding;
	};
public:
	BlurFilter();
	~BlurFilter();


	ID3D11ShaderResourceView* GetBlurredOutput();

	// Generate Gaussian blur weights.
	void SetGaussianWeights(float sigma);

	// Manually specify blur weights.
	void SetWeights(const float weights[9]);

	/// The width and height should match the dimensions of the input texture to blur.
	/// It is OK to call Init() again to reinitialize the blur filter with a different 
	/// dimension or format.
	void Init(ID3D11Device* device, UINT width, UINT height, DXGI_FORMAT format);

	/// Blurs the input texture blurCount times.  Note that this modifies the input texture, not a copy of it.
	void BlurInPlace(ID3D11DeviceContext* dc, ID3D11ShaderResourceView* inputSRV, ID3D11UnorderedAccessView* inputUAV, int blurCount);

private:
	void SetWeightscBuffer(ID3D11DeviceContext* dc);
	void BuiltFx(ID3D11Device* device);
	void BuildConstantBuffer(ID3D11Device* device);
private:

	UINT mWidth;
	UINT mHeight;
	DXGI_FORMAT mFormat;
	ID3D11Buffer* mWeithtsBuffer;
	ID3D11ComputeShader* mHorBlurShader;
	ID3D11ComputeShader* mVerBlurShader;

	ID3D11ShaderResourceView* mBlurredOutputTexSRV;
	ID3D11UnorderedAccessView* mBlurredOutputTexUAV;

	float mWeights[11];
};