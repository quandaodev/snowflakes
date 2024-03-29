#pragma once

#include "resource.h"

bool InitDesktopDrawing();

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL				IntializeShellIcon();
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	DialogProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

