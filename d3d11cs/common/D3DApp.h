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
	HINSTANCE mhAppInst;     // ?pö?áå¿
	HWND      mhMainWnd;     // åâxûå¿
	bool      mAppPaused;    // ö¥Û?Ý?âó?
	bool      mMinimized;    // ö¥ÛÅ¬»
	bool      mMaximized;    // ö¥ÛÅå»
	bool      mResizing;     // ö¥Û?Ýü?å¬Ió?
	UINT      m4xMsaaQuality;// 4X MSAA?Ê?

	// p°??"delta-time"aà???(4.3)
	GameTimer mTimer;

	//  D3D11??(4.2.1)Cð??(4.2.4)Cp°[x/ÍÂ?¶I2D?(4.2.6)C
	//  ?õÚ?(4.2.5)a[x/ÍÂ??(4.2.6)Ca?û(4.2.8)B
	ID3D11Device* md3dDevice;
	ID3D11DeviceContext* md3dDeviceContext;
	IDXGISwapChain* mSwapChain;
	ID3D11Texture2D* mDepthStencilBuffer;
	ID3D11RenderTargetView* mRenderTargetView;
	ID3D11DepthStencilView* mDepthStencilView;
	D3D11_VIEWPORT mScreenViewport;

	//  ºÊI?Ê¥ÝD3DApp?¢?uIBA¥C?ÂÈÝh¶?dÊ?±?B

	//  âxû??BD3DAppIàÒ???¥"D3D11 Application"B
	std::wstring mMainWndCaption;

	//  Hardware device?¥reference deviceHD3DAppàÒ?gpD3D_DRIVER_TYPE_HARDWAREB
	D3D_DRIVER_TYPE md3dDriverType;
	// âxûInå¬BD3DAppàÒ??800x600BÓCâxûå¬Ý?s?iü??C?±?çïVü?B
	int mClientWidth;
	int mClientHeight;
	//  ?u?true?gp4XMSAA(4.1.8)CàÒ??falseB
	bool mEnable4xMsaa;

};