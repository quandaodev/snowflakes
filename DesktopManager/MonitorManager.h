#pragma once
#include "framework.h"

struct MonitorInfo
{
    HMONITOR Handle;
    HDC Hdc;
    RECT Rect;

    int Width() { return Rect.right - Rect.left;}
    int Height() { return Rect.bottom - Rect.top;}
    int Left() { return Rect.left;}
    int Top() { return Rect.top;}
};

class MonitorManager
{
public:
    std::vector<MonitorInfo> Monitors;

    static MonitorManager* GetInstance();
    static void Destroy();

    static BOOL CALLBACK OnMonitorEnumCallBack(HMONITOR hMon, HDC hdc, LPRECT lprcMonitor, LPARAM pData);

private:
    static MonitorManager* Instance;

    MonitorManager();
};