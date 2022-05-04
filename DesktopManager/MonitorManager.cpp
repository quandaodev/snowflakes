#include "MonitorManager.h"

MonitorManager* MonitorManager::Instance {nullptr};

MonitorManager* MonitorManager::GetInstance()
{
    if (Instance == nullptr)
        Instance = new MonitorManager();

    return Instance;
}

void MonitorManager::Destroy()
{
    if (Instance)
    {
        delete Instance;
        Instance = nullptr;
    }
}

BOOL CALLBACK MonitorManager::OnMonitorEnumCallBack(HMONITOR hMon, HDC hdc, LPRECT lprcMonitor, LPARAM pData)
{
    MonitorManager* pThis = reinterpret_cast<MonitorManager*>(pData);

    MonitorInfo monitor {0};
    monitor.Handle = hMon;
    monitor.Hdc = hdc;

    monitor.Rect.left = lprcMonitor->left;
    monitor.Rect.top = lprcMonitor->top;
    monitor.Rect.right = lprcMonitor->right;
    monitor.Rect.bottom = lprcMonitor->bottom;

    pThis->Monitors.push_back(monitor);

    return TRUE;
}

MonitorManager::MonitorManager()
{
    EnumDisplayMonitors(0, 0, OnMonitorEnumCallBack, (LPARAM)this);
}