#include "resource.h"
#include "DrawingManager.h"
#include "Flake.h"
#include "MonitorManager.h"
#include <thread>

using namespace std;

IWICImagingFactory *g_pFactory = nullptr;

bool endTimer(false);

std::thread drawingThread;
int screenWidth{ 0 }, screenHeight{ 0 }, screenOffsetX{ 0 }, screenOffsetY{ 0 };

HBITMAP g_hFlake{ nullptr }, g_hMaskFlake{nullptr}, g_hImage{ nullptr };
std::vector<std::unique_ptr<Flake>> g_vtFlakes;

void OnDrawing();
DrawingManager* g_drawingManager{ nullptr }; // TODO: refactor singleton

DrawingManager* DrawingManager::GetInstance()
{
	return g_drawingManager;
}

DrawingManager::DrawingManager(HWND hWorkerWnd, UINT numSnowFlakes, UINT frameRate)
	:hWorkerWnd{ hWorkerWnd }, picWidth{ 0 }, picHeight{ 0 }, numSnowFlakes{ numSnowFlakes }, frameRate{frameRate}
{
    //auto wholeScreenWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    //auto wholeScreenHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);

	// Get the primary monitor coordinates
	auto monitorMan = MonitorManager::GetInstance();
	screenWidth = monitorMan->Monitors[0].Width();
    screenHeight = monitorMan->Monitors[0].Height();
    
	for (int i = 1; i < monitorMan->Monitors.size(); ++i) {
		screenOffsetX += monitorMan->Monitors[i].Left() < 0 ? std::abs(monitorMan->Monitors[i].Left()) : 0;
		screenOffsetY += monitorMan->Monitors[i].Top() < 0 ? std::abs(monitorMan->Monitors[i].Top()) : 0;
	}
	screenOffsetX += monitorMan->Monitors[0].Left();
    screenOffsetY += monitorMan->Monitors[0].Top();


	if (!g_pFactory) {
		// Create the COM imaging factory
		HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&g_pFactory));
	}
	g_drawingManager = this;
}

DrawingManager::~DrawingManager()
{
	if (drawingThread.get_id() != std::thread::id())
	{
		endTimer = true;
		drawingThread.join();
	}

	g_vtFlakes.clear();

	if (g_hImage) {
		DeleteObject(g_hImage);
	}
	if (g_hMaskFlake) {
		DeleteObject(g_hMaskFlake);
	}
	if (g_pFactory) {
		g_pFactory->Release();
	}

}

void DrawingManager::InitBackgroundImage(wstring path)
{
	std::wostringstream w;

	if (path.empty() || path == L"Current Wallpaper")
	{
		wchar_t szAppPath[MAX_PATH]{0};
		if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, szAppPath))) {
			w << szAppPath << L"\\Microsoft\\Windows\\Themes\\TranscodedWallpaper";
		}
	}
	else
	{
		w << path;
	}
	
	// Create a decoder
	IWICBitmapDecoder* pDecoder{ nullptr };
	HRESULT hr = g_pFactory->CreateDecoderFromFilename(
		w.str().c_str(),                      // Image to be decoded
		NULL,                            // Do not prefer a particular vendor
		GENERIC_READ,                    // Desired read access to the file
		WICDecodeMetadataCacheOnDemand,  // Cache metadata when needed
		&pDecoder                        // Pointer to the decoder
	);
	// Retrieve the first frame of the image from the decoder
	IWICBitmapFrameDecode* pFrame{ nullptr };
	if (SUCCEEDED(hr)) {
		hr = pDecoder->GetFrame(0, &pFrame);
	}

	IWICBitmapSource* ipBitmap{ nullptr };
	if (pFrame){
		WICConvertBitmapSource(GUID_WICPixelFormat32bppPBGRA, pFrame, &ipBitmap);
		pFrame->Release();
		pFrame = nullptr;
	}
	
	g_hImage = CreateHBITMAP(ipBitmap);

	pDecoder->Release();
}

void DrawingManager::InitDrawing()
{
	if (g_hFlake == nullptr && g_hMaskFlake == nullptr)
	{
		HINSTANCE hInstance = GetModuleHandle(NULL);
		g_hFlake = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_FLAKE));
		g_hMaskFlake = CreateBitmapMask(g_hFlake, RGB(255, 255, 255));
	}

	if (drawingThread.get_id() != std::thread::id())
	{
		endTimer = true;
		drawingThread.join();
	}
	
	g_vtFlakes.clear();
	for (UINT i = 0; i < numSnowFlakes; ++i) {
		g_vtFlakes.emplace_back(std::make_unique<Flake>(screenWidth, screenHeight));
	}
	endTimer = false;
	drawingThread = std::thread(OnDrawing);

	InitBackgroundImage(path);
}

HBITMAP DrawingManager::CreateHBITMAP(IWICBitmapSource * ipBitmap)
{
	HBITMAP hbmp = NULL;

	// get image attributes and check for valid image
	if (FAILED(ipBitmap->GetSize(&picWidth, &picHeight)) || picWidth == 0 || picHeight == 0)
		return hbmp;

	// prepare structure giving bitmap information (negative height indicates a top-down DIB)
	BITMAPINFO bminfo;
	ZeroMemory(&bminfo, sizeof(bminfo));
	bminfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bminfo.bmiHeader.biWidth = picWidth;
	bminfo.bmiHeader.biHeight = -((LONG)picHeight);
	bminfo.bmiHeader.biPlanes = 1;
	bminfo.bmiHeader.biBitCount = 32;
	bminfo.bmiHeader.biCompression = BI_RGB;

	// create a DIB section that can hold the image
	void * pvImageBits = NULL;
	HDC hdcScreen = GetDC(NULL);
	hbmp = CreateDIBSection(hdcScreen, &bminfo, DIB_RGB_COLORS, &pvImageBits, NULL, 0);
	ReleaseDC(NULL, hdcScreen);
	if (hbmp == NULL)
		return hbmp;

	// extract the image into the HBITMAP
	const UINT cbStride = picWidth * 4;
	const UINT cbImage = cbStride * picHeight;
	if (FAILED(ipBitmap->CopyPixels(NULL, cbStride, cbImage, static_cast<BYTE *>(pvImageBits))))
	{
		// couldn't extract image; delete HBITMAP
		DeleteObject(hbmp);
		hbmp = NULL;
	}

	return hbmp;
}

HBITMAP DrawingManager::CreateBitmapMask(HBITMAP hbmColour, COLORREF crTransparent)
{
	HDC hdcMem, hdcMem2;
	HBITMAP hbmMask;
	BITMAP bm;

	// Create monochrome (1 bit) mask bitmap.  

	GetObject(hbmColour, sizeof(BITMAP), &bm);
	hbmMask = CreateBitmap(bm.bmWidth, bm.bmHeight, 1, 1, NULL);

	// Get some HDCs that are compatible with the display driver

	hdcMem = CreateCompatibleDC(0);
	hdcMem2 = CreateCompatibleDC(0);

	HBITMAP hOldBmp1 = (HBITMAP)SelectObject(hdcMem, hbmColour);
	HBITMAP hOldBmp2 = (HBITMAP)SelectObject(hdcMem2, hbmMask);

	// Set the background colour of the colour image to the colour
	// you want to be transparent.
	SetBkColor(hdcMem, crTransparent);

	// Copy the bits from the colour image to the B+W mask... everything
	// with the background colour ends up white while everythig else ends up
	// black...Just what we wanted.

	BitBlt(hdcMem2, 0, 0, bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, SRCCOPY);

	// Take our new mask and use it to turn the transparent colour in our
	// original colour image to black so the transparency effect will
	// work right.
	BitBlt(hdcMem, 0, 0, bm.bmWidth, bm.bmHeight, hdcMem2, 0, 0, SRCINVERT);

	// Clean up.

	::SelectObject(hdcMem, hOldBmp1);
	::SelectObject(hdcMem2, hOldBmp2);

	::DeleteDC(hdcMem);
	::DeleteDC(hdcMem2);

	return hbmMask;
}

void DrawingManager::DrawDebugInfo(HDC hdc)
{
	std::wostringstream strStream;
	strStream << "Width:" << screenWidth << " Height:" << screenHeight << " X:" << screenOffsetX << " Y:" << screenOffsetY;

	RECT rtText;
	rtText.left = 500;
	rtText.right = rtText.left + 1000;
	rtText.top = 800;
	rtText.bottom = rtText.top + 100;

	::DrawText(hdc, strStream.str().c_str(), (int)strStream.str().length(), &rtText, 0);
}

void DrawingManager::FillBackgroundColor(HDC hDC, COLORREF bkColor)
{
	HBRUSH brushBk = ::CreateSolidBrush(bkColor);
	HGDIOBJ hBrushBack = ::SelectObject(hDC, brushBk);
	::Rectangle(hDC, 0, 0, screenWidth, screenHeight);
	::SelectObject(hDC, hBrushBack);
	::DeleteObject(brushBk);
}

void DrawingManager::DrawBackgroundImage(HDC hDC)
{
	HDC hMemDC = ::CreateCompatibleDC(hDC);
	HGDIOBJ backObject = ::SelectObject(hMemDC, g_hImage);
	SetStretchBltMode(hDC, COLORONCOLOR);
	StretchBlt(hDC, 0, 0, screenWidth, screenHeight, hMemDC, 0, 0, picWidth, picHeight, SRCCOPY);

	//BitBlt(hDC, 0, 0, screenWidth, screenHeight, hMemDC, 0, 0, SRCCOPY);
	::SelectObject(hMemDC, backObject);
	::DeleteObject(hMemDC);
}

void DrawingManager::DrawFlake(HDC hDC)
{
	// Get bitmap size
	BITMAP bm;
	GetObject(g_hFlake, sizeof(bm), &bm);

	HDC hFlakeDC = CreateCompatibleDC(hDC);
	HDC hMaskFlakeDC = CreateCompatibleDC(hDC);
	if (hFlakeDC && hMaskFlakeDC)
	{
		HBITMAP hOldBmpFlake = (HBITMAP)::SelectObject(hFlakeDC, g_hFlake);
		HBITMAP hOldBmpMaskFlake = (HBITMAP)::SelectObject(hMaskFlakeDC, g_hMaskFlake);

		for (auto& f : g_vtFlakes) {
			BitBlt(hDC, f->GetX(), f->GetY(), bm.bmWidth, bm.bmHeight, hMaskFlakeDC, 0, 0, SRCAND);
			BitBlt(hDC, f->GetX(), f->GetY(), bm.bmWidth, bm.bmHeight, hFlakeDC, 0, 0, SRCPAINT);
		}

		::SelectObject(hFlakeDC, hOldBmpFlake);
		::SelectObject(hMaskFlakeDC, hOldBmpMaskFlake);

		::DeleteDC(hFlakeDC);
		::DeleteDC(hMaskFlakeDC);
	}
}

bool DrawingManager::DrawDesktop()
{
	HDC hdc = ::GetDCEx(hWorkerWnd, 0, 0x403);
	HDC hMemDC = ::CreateCompatibleDC(NULL);

	// Create a buffer for memDC
	HBITMAP hBckBmp = ::CreateCompatibleBitmap(hdc, screenWidth, screenHeight);
	HGDIOBJ hOldBmp = ::SelectObject(hMemDC, hBckBmp);

	DrawBackgroundImage(hMemDC);
	DrawFlake(hMemDC);
	DrawDebugInfo(hMemDC);

	::BitBlt(hdc, screenOffsetX, screenOffsetY, screenWidth, screenHeight, hMemDC, 0, 0, SRCCOPY);

	::SelectObject(hMemDC, hOldBmp);

	::DeleteObject(hBckBmp);
	::DeleteDC(hMemDC);
	::ReleaseDC(hWorkerWnd, hdc);

	return true;
}

static void OnDrawing()
{
	auto lastFrame = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> lastDrawing{ 0 };
	auto pDrawingMan = DrawingManager::GetInstance();

	_ASSERT(pDrawingMan);

	while (!endTimer) {
		//std::wostringstream w;

		auto start = std::chrono::high_resolution_clock::now();
		
		auto delayBetweenFrame = 1000 / pDrawingMan->frameRate - lastDrawing.count();
		//w << "Delay between frame:" << delayBetweenFrame << " milSecs" << endl;

		std::chrono::duration<double, std::milli> elapsedSinceLastFrame = start - lastFrame;
        if (elapsedSinceLastFrame.count() < delayBetweenFrame)
        {
			if (delayBetweenFrame > lastDrawing.count())
			{
				//w << "Skip frame" << endl;
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}

            continue;
        }

		for (auto &f : g_vtFlakes) {
			f->MoveNext();
		}

		g_drawingManager->DrawDesktop();
		auto end = std::chrono::high_resolution_clock::now();

		std::chrono::duration<double, std::milli> timeSinceLastFrame = end - lastFrame;
		lastFrame = end;

		lastDrawing = end - start;
		//w << "Time for drawing:" << lastDrawing.count() << " milSecs" << endl;
		//w << "Time since last frame:" << timeSinceLastFrame.count() << " milSecs" << endl;

		//OutputDebugString(w.str().c_str());
	}
}

