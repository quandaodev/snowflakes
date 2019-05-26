// SnowFall.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "SnowFall.h"
#include "Flake.h"
#include "shellapi.h"
#include "commctrl.h"
#include <chrono>
#include <thread>
#include <iostream>
#include <sstream>
#include <vector>
#include <memory>

#define MAX_LOADSTRING 100
#define APPWM_ICONNOTIFY (WM_APP + 1)
class __declspec(uuid("{283c9cbf-3d93-423c-8c00-a1a6af75815d}")) SnowFallUuid;

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
HWND hWorkerWnd{ 0 };
std::vector<std::unique_ptr<Flake>> g_vtFlakes;
bool endTimer(false);
std::thread drawingThread;
int screenWidth{ 0 }, screenHeight{ 0 };
HBITMAP g_hFlake, g_hMaskFlake;
HBITMAP g_hImage = (HBITMAP)::LoadImage(NULL, L"C:\\Wallpaper.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION | LR_DEFAULTCOLOR);
wchar_t SnowAppToolTip[] = L"Snow fall application";
HWND hSnowFallWnd{ 0 };
bool mainWndMinized{ false };
NOTIFYICONDATA nid = {};

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_SNOWFALL, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SNOWFALL));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SNOWMAN));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_SNOWFALL);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SNOWMAN));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   hSnowFallWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hSnowFallWnd)
   {
      return FALSE;
   }

   //ShowWindow(hWnd, nCmdShow);
   //UpdateWindow(hWnd);

   InitDesktopDrawing();

   screenWidth = 1680; //1920;//GetSystemMetrics(SM_CXVIRTUALSCREEN) - GetSystemMetrics(SM_XVIRTUALSCREEN);
   screenHeight = 1050; //1080;//GetSystemMetrics(SM_CYVIRTUALSCREEN) - GetSystemMetrics(SM_YVIRTUALSCREEN);

   g_hFlake = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_FLAKE));
   g_hMaskFlake = CreateBitmapMask(g_hFlake, RGB(255, 255, 255));

   for (int i = 0; i < 15; ++i) {
	   g_vtFlakes.emplace_back(std::make_unique<Flake>(screenWidth, screenHeight));
   }

   drawingThread = std::thread(timer);

   // Shell Notify Icon
   nid.cbSize = sizeof(nid);
   nid.hWnd = hSnowFallWnd;
   nid.uFlags = NIF_ICON | NIF_TIP | NIF_GUID | NIF_MESSAGE;
   nid.uCallbackMessage = APPWM_ICONNOTIFY;
   nid.guidItem = __uuidof(SnowFallUuid);
   lstrcpyW(nid.szTip, SnowAppToolTip);
   LoadIconMetric(hInst, MAKEINTRESOURCE(IDI_SNOWMAN), LIM_SMALL, &(nid.hIcon));

   Shell_NotifyIcon(NIM_ADD, &nid);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
	case APPWM_ICONNOTIFY:
		{
			switch (lParam)
			{
			case WM_LBUTTONUP:
				ShowWindow(hSnowFallWnd, SW_SHOW);
				if (mainWndMinized) 
					ShowWindow(hSnowFallWnd, SW_RESTORE);
				Shell_NotifyIcon(NIM_DELETE, &nid);
				break;
			case WM_RBUTTONUP:
				HMENU popMenu = CreatePopupMenu();
				AppendMenu(popMenu, MF_STRING, IDM_SHOW, L"Show");
				AppendMenu(popMenu, MF_STRING, IDM_EXIT, L"Exit");

				POINT pCursor;
				GetCursorPos(&pCursor);
				TrackPopupMenu(popMenu, TPM_LEFTBUTTON|TPM_RIGHTALIGN, pCursor.x, pCursor.y, 0, hWnd, NULL);
				break;
			}

			return 0;
		}
		break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
			case IDM_SHOW:
				break;
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
	case WM_SIZE:
	{
		if (wParam == SIZE_MINIMIZED) {
			mainWndMinized = true;
			ShowWindow(hSnowFallWnd, SW_HIDE);
			Shell_NotifyIcon(NIM_ADD, &nid);
		}
	}
		break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
		endTimer = true;
		drawingThread.join();

		if (g_hImage) {
			DeleteObject(g_hImage);
		}
		if (g_hMaskFlake){
			DeleteObject(g_hMaskFlake);
		}
		Shell_NotifyIcon(NIM_DELETE, &nid);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}


BOOL CALLBACK MyWndEnum(HWND hWnd, LPARAM lParam)
{
	HWND h = FindWindowEx(hWnd, 0, L"SHELLDLL_DefView", 0);
	if (h) {
		hWorkerWnd = FindWindowEx(0, hWnd, L"WorkerW", 0);
	}
	return TRUE;
}

bool InitDesktopDrawing()
{
	HWND hWndProgman = FindWindow(L"Progman", nullptr);
	DWORD_PTR result{ 0 };
	SendMessageTimeout(hWndProgman, 0x052C, 0, 0, SMTO_NORMAL, 1000, &result);

	EnumWindows(MyWndEnum, 0);
	return true;
}

HBITMAP CreateBitmapMask(HBITMAP hbmColour, COLORREF crTransparent)
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

void DrawDebugInfo(HDC hdc)
{
	std::wostringstream strStream;
	strStream << "Width:" << screenWidth << " Height:" << screenHeight;

	RECT rtText;
	rtText.left = 500;
	rtText.right = rtText.left + 1000;
	rtText.top = 800;
	rtText.bottom = rtText.top + 100;

	::DrawText(hdc, strStream.str().c_str(), (int)strStream.str().length(), &rtText, 0);
}

void FillBackgroundColor(HDC hDC, COLORREF bkColor)
{
	HBRUSH brushBk = ::CreateSolidBrush(bkColor);
	HGDIOBJ hBrushBack = ::SelectObject(hDC, brushBk);
	::Rectangle(hDC, 0, 0, screenWidth, screenHeight);
	::SelectObject(hDC, hBrushBack);
	::DeleteObject(brushBk);
}

void DrawBackgroundImage(HDC hDC)
{
	HDC hMemDC = ::CreateCompatibleDC(hDC);
	HGDIOBJ backObject = ::SelectObject(hMemDC, g_hImage);
	BitBlt(hDC, 0, 0, screenWidth, screenHeight, hMemDC, 0, 0, SRCCOPY);
	::SelectObject(hMemDC, backObject);
	::DeleteObject(hMemDC);
}

void DrawFlake(HDC hDC)
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

bool DrawDesktop()
{
	HDC hdc = ::GetDCEx(hWorkerWnd, 0, 0x403);
	HDC hMemDC = ::CreateCompatibleDC(NULL);

	// Create a buffer for memDC
	HBITMAP hBckBmp = ::CreateCompatibleBitmap(hdc, screenWidth, screenHeight);
	HGDIOBJ hOldBmp = ::SelectObject(hMemDC, hBckBmp);

	DrawBackgroundImage(hMemDC);
	DrawFlake(hMemDC);
	// DrawDebugInfo();

	::BitBlt(hdc, 0, 0, screenWidth, screenHeight, hMemDC, 0, 0, SRCCOPY);

	::SelectObject(hMemDC, hOldBmp);

	::DeleteObject(hBckBmp);
	::DeleteDC(hMemDC);
	::ReleaseDC(hWorkerWnd, hdc);

	return true;
}

void timer()
{
	while (!endTimer) {
		for (auto &f : g_vtFlakes) {
			f->MoveNext();
		}

		DrawDesktop();

		std::this_thread::sleep_for(std::chrono::milliseconds(25));
	}
}
