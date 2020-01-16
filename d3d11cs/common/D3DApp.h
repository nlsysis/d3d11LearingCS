#pragma once

#include "d3dUtil.h"
#include "GameTimer.h"
#include <string>

#pragma comment(lib,"d3d11.lib")
#pragma comment(lib,"d3dx11d.lib")
//#pragma comment(lib,"D3DCompiler.lib")
class D3DApp
{
public:
	D3DApp(HINSTANCE hInstance);
	virtual ~D3DApp();

	HINSTANCE GetAppInst()const;
	HWND      GetMainWnd()const;
	float     GetAspectRatio()const;   //the ratio of back buffer

	int Run();

	virtual bool Init();
	virtual void OnResize();
	virtual void UpdateScene(float dt) = 0;
	virtual void DrawScene() = 0;
	virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	//deal with mouse input
	virtual void OnMouseDown(WPARAM btnState, int x, int y) { }
	virtual void OnMouseUp(WPARAM btnState, int x, int y) { }
	virtual void OnMouseMove(WPARAM btnState, int x, int y) { }

protected:
	bool InitMainWindow();
	bool InitDirect3D();

	void CalculateFrameStats();

protected:
	HINSTANCE mhAppInst;     // ?�p����?��啿
	HWND      mhMainWnd;     // ���x���啿
	bool      mAppPaused;    // ��������?��?���?
	bool      mMinimized;    // �������ۍŏ���
	bool      mMaximized;    // �������ۍő剻
	bool      mResizing;     // ��������?�݉�?�召�I��?
	UINT      m4xMsaaQuality;// 4X MSAA?�ʓ�?

	// �p��??"delta-time"�a��???(��4.3)
	GameTimer mTimer;

	//  D3D11??(��4.2.1)�C��??(��4.2.4)�C�p���[�x/�͔�?���I2D?��(��4.2.6)�C
	//  ?����?(��4.2.5)�a�[�x/�͔�??(��4.2.6)�C�a?��(��4.2.8)�B
	ID3D11Device* md3dDevice;
	ID3D11DeviceContext* md3dDeviceContext;
	IDXGISwapChain* mSwapChain;
	ID3D11Texture2D* mDepthStencilBuffer;
	ID3D11RenderTargetView* mRenderTargetView;
	ID3D11DepthStencilView* mDepthStencilView;
	D3D11_VIEWPORT mScreenViewport;

	//  ���ʓI?�ʐ���D3DApp?��������?�u�I�B�A���C?�ȍݔh��?���d��?��?�B

	//  �x��??�BD3DApp�I��???��"D3D11 Application"�B
	std::wstring mMainWndCaption;

	//  Hardware device?��reference device�HD3DApp��?�g�pD3D_DRIVER_TYPE_HARDWARE�B
	D3D_DRIVER_TYPE md3dDriverType;
	// �x���I���n�召�BD3DApp��??800x600�B���ӁC���x���召��?�s?�i��??�C?��?���V��?�B
	int mClientWidth;
	int mClientHeight;
	//  ?�u?true?�g�p4XMSAA(��4.1.8)�C��??false�B
	bool mEnable4xMsaa;

};