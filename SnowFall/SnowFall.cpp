// SnowFall.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "SnowFall.h"
#include "Flake.h"
#include "DrawingManager.h"
#include "shellapi.h"
#include "commdlg.h"
#include "commctrl.h"
#include "wincodec.h"
#include "wincodecsdk.h"
#include "shlobj_core.h"

#define MAX_LOADSTRING 100
#define APPWM_ICONNOTIFY (WM_APP + 1)
class __declspec(uuid("{283c9cbf-3d93-423c-8c01-a1bfaf75815a}")) SnowFallUuid;

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

HWND hWorkerWnd{ 0 };

wchar_t SnowAppToolTip[] = L"Snow fall application";
HWND hSnowFallWnd{ 0 };
bool mainWndMinized{ false };
NOTIFYICONDATA nid = {};

int g_nFrameRate = 60;
int g_nNumSnowFlakes = 30;

DrawingManager* g_pDrawMan = nullptr; 

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

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex{};

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

BOOL IntializeShellIcon()
{
	nid.cbSize = sizeof(nid);
	nid.hWnd = hSnowFallWnd;
	nid.uFlags = NIF_ICON | NIF_TIP | NIF_GUID | NIF_MESSAGE;
	nid.uCallbackMessage = APPWM_ICONNOTIFY;
	nid.guidItem = __uuidof(SnowFallUuid);
	lstrcpyW(nid.szTip, SnowAppToolTip);
	LoadIconMetric(hInst, MAKEINTRESOURCE(IDI_SNOWMAN), LIM_SMALL, &(nid.hIcon));

	return TRUE;
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	auto res = CoInitialize(NULL);

	hInst = hInstance; // Store instance handle in our global variable

	hSnowFallWnd = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_MAIN), NULL, (DLGPROC)DialogProc);

	if (!ShowWindow(hSnowFallWnd, SW_SHOW))
	{
		OutputDebugString(L"ShowWindow failed");
	}

    InitDesktopDrawing();

	g_pDrawMan = new DrawingManager(hWorkerWnd, g_nNumSnowFlakes, g_nFrameRate);
	g_pDrawMan->InitDrawing();

	if (!IntializeShellIcon())
	{
		OutputDebugString(L"InitializeShellIcon failed");
	}

	if (hSnowFallWnd)
	{
		SetDlgItemText(hSnowFallWnd, IDC_EDIT_WPP_LOCATION, L"Current Wallpaper");
		SetDlgItemText(hSnowFallWnd, IDC_EDIT_FRAME_RATE, std::to_wstring(g_nFrameRate).c_str());
		SetDlgItemText(hSnowFallWnd, IDC_EDIT_NUM_SNOWFLAKES, std::to_wstring(g_nNumSnowFlakes).c_str());
	}

    return TRUE;
}

INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case APPWM_ICONNOTIFY:
		{
			switch (lParam)
			{
			case WM_LBUTTONUP:
				ShowWindow(hSnowFallWnd, SW_SHOW);
				if (mainWndMinized)
					ShowWindow(hSnowFallWnd, SW_RESTORE);
				break;
			case WM_RBUTTONUP:
				HMENU popMenu = CreatePopupMenu();
				AppendMenu(popMenu, MF_STRING, IDM_SHOW, L"Show");
				AppendMenu(popMenu, MF_STRING, IDM_EXIT, L"Exit");

				POINT pCursor;
				GetCursorPos(&pCursor);
				TrackPopupMenu(popMenu, TPM_LEFTBUTTON | TPM_RIGHTALIGN, pCursor.x, pCursor.y, 0, hSnowFallWnd, NULL);
				break;
			}
			return TRUE;
		}
		break;
		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDM_ABOUT:
				{
					DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hSnowFallWnd, About);
					return TRUE;
				}
				break;
				case IDM_EXIT:
				{
					DestroyWindow(hSnowFallWnd);
					return TRUE;
				}
				break;
				case IDCANCEL:
				{
					SendMessage(hDlg, WM_CLOSE, 0, 0);
					return TRUE;
				}
				case IDOK:
				{
					wchar_t buffer[256] = {0};

					auto pDrawingMan = DrawingManager::GetInstance();

					GetDlgItemText(hSnowFallWnd, IDC_EDIT_WPP_LOCATION, buffer, _countof(buffer));
					pDrawingMan->path = std::wstring(buffer);

					GetDlgItemText(hSnowFallWnd, IDC_EDIT_FRAME_RATE, buffer, _countof(buffer));
					pDrawingMan->frameRate = _wtoi(buffer);

					GetDlgItemText(hSnowFallWnd, IDC_EDIT_NUM_SNOWFLAKES, buffer, _countof(buffer));
					pDrawingMan->numSnowFlakes = _wtoi(buffer);

					// IDC_EDIT_FALLING_SPEED

					DrawingManager::GetInstance()->InitDrawing();

					return TRUE;
				}
				break;
				case IDC_WPP_BROWSE:
				{
					OPENFILENAME ofn;
					wchar_t szFileName[MAX_PATH] = L"";

					ZeroMemory(&ofn, sizeof(ofn));

					ofn.lStructSize = sizeof(ofn);
					ofn.hwndOwner = NULL;
					ofn.lpstrFilter = L"All Files (*.*)\0*.*\0";
					ofn.lpstrFile = szFileName;
					ofn.nMaxFile = MAX_PATH;
					ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
					ofn.lpstrDefExt = L"jpg";

					GetOpenFileName(&ofn);
					SetDlgItemText(hSnowFallWnd, IDC_EDIT_WPP_LOCATION, szFileName);
					return TRUE;
				}
				break;
			}
		}
		break;
		case WM_CLOSE:
			if (MessageBox(hDlg, TEXT("Close the program?"), TEXT("Close"),
				MB_ICONQUESTION | MB_YESNO) == IDYES)
			{
				DestroyWindow(hDlg);
			}
			return FALSE;
		case WM_SIZE:
		{
			if (wParam == SIZE_MINIMIZED) {
				mainWndMinized = true;
				ShowWindow(hSnowFallWnd, SW_HIDE);

				Shell_NotifyIcon(NIM_DELETE, &nid); // delete before adding for cases not deleted last time
				Shell_NotifyIcon(NIM_ADD, &nid);

				return TRUE;
			}
			else if (wParam == SIZE_RESTORED) {
				
				return TRUE;
			}
			return FALSE;
		}
		break;
		case WM_DESTROY:
		{
			delete g_pDrawMan;
			g_pDrawMan = nullptr;

			Shell_NotifyIcon(NIM_DELETE, &nid);
			PostQuitMessage(0);
			return TRUE;
		}
		break;
	}
	return FALSE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProc(hWnd, message, wParam, lParam);
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