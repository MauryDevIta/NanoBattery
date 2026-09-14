#include "system_utils.h"
#include "widget_globals.h"
#include <windows.h>
#include <shellapi.h>

void AddToStartup(void) {
    char path[MAX_PATH];
    GetModuleFileName(NULL, path, MAX_PATH);
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        RegSetValueEx(hKey, REGISTRY_APP_NAME, 0, REG_SZ, (const BYTE*)path, strlen(path) + 1);
        RegCloseKey(hKey);
    }
}

void RemoveFromStartup(void) {
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        RegDeleteValue(hKey, REGISTRY_APP_NAME);
        RegCloseKey(hKey);
    }
}

int IsInStartup(void) {
    HKEY hKey;
    int exists = 0;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS) {
        char buffer[MAX_PATH];
        DWORD bufferSize = sizeof(buffer);
        if (RegQueryValueEx(hKey, REGISTRY_APP_NAME, NULL, NULL, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS) {
            exists = 1;
        }
        RegCloseKey(hKey);
    }
    return exists;
}

void LoadSettings(void) {
    HKEY hKey;
    alertsEnabled = 1;
    if (RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\" REGISTRY_APP_NAME, 0, NULL, 0, KEY_READ | KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        DWORD val = 1;
        DWORD size = sizeof(val);
        if (RegQueryValueEx(hKey, "AlertsEnabled", NULL, NULL, (LPBYTE)&val, &size) == ERROR_SUCCESS) {
            alertsEnabled = val;
        } else {
            RegSetValueEx(hKey, "AlertsEnabled", 0, REG_DWORD, (const BYTE*)&val, sizeof(val));
        }
        RegCloseKey(hKey);
    }
}

void SaveSettings(void) {
    HKEY hKey;
    if (RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\" REGISTRY_APP_NAME, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        DWORD val = alertsEnabled;
        RegSetValueEx(hKey, "AlertsEnabled", 0, REG_DWORD, (const BYTE*)&val, sizeof(val));
        RegCloseKey(hKey);
    }
}

void ShowNotification(HWND hwnd, const char* title, const char* msg, int isWarning) {
    NOTIFYICONDATA nid = {0};
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = 1001;
    nid.uFlags = NIF_INFO | NIF_ICON | NIF_MESSAGE; 
    nid.uCallbackMessage = WM_APP + 5;
    nid.hIcon = LoadIcon(NULL, IDI_INFORMATION);
    
    strcpy(nid.szInfoTitle, title);
    strcpy(nid.szInfo, msg);
    nid.dwInfoFlags = isWarning ? NIIF_WARNING : NIIF_INFO;

    Shell_NotifyIcon(NIM_ADD, &nid);
    
    SetTimer(hwnd, ID_TIMER_TRAY, 8000, NULL);
}

void RemoveNotification(HWND hwnd) {
    NOTIFYICONDATA nid = {0};
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = 1001;
    Shell_NotifyIcon(NIM_DELETE, &nid);
}
