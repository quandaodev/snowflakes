#pragma once
#include "framework.h"
#include "wincodec.h"
#include "wincodecsdk.h"
#include "shlobj_core.h"

class DrawingManager
{
public:
	DrawingManager(HWND hWorkerWnd, UINT numSnowFlakes, UINT frameRate);
	~DrawingManager();

	void InitDrawing();
	HBITMAP CreateHBITMAP(IWICBitmapSource * ipBitmap);

	HBITMAP CreateBitmapMask(HBITMAP hbmColour, COLORREF crTransparent);
	void DrawDebugInfo(HDC hdc);
	void FillBackgroundColor(HDC hDC, COLORREF bkColor);
	void DrawBackgroundImage(HDC hDC);
	void DrawFlake(HDC hDC);
	bool DrawDesktop();

	HWND hWorkerWnd;
	UINT picWidth;
	UINT picHeight;
	UINT screenWidth;
	UINT screenHeight;
	UINT numSnowFlakes;
	UINT frameRate;
};

