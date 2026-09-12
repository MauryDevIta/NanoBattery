/**
 * Windows Native Battery Widget with Remote Auto-Updater
 * A lightweight, zero-dependency, DPI-aware battery monitor for the Windows Taskbar.
 * Implements a global mouse hook for transparent UI interaction and silent self-updating.
 */

#include <windows.h>
#include <stdio.h>
#include <shellapi.h>
#include <string.h>
#include <urlmon.h>

// --- CONFIGURATION & CONSTANTS ---
#define CURRENT_VERSION 5
#define GITHUB_VERSION_URL "https://raw.githubusercontent.com/MauryDevIta/NanoBattery/refs/heads/main/version.txt"
#define GITHUB_EXE_URL     "https://raw.githubusercontent.com/MauryDevIta/NanoBattery/refs/heads/main/battery_widget.exe"

#define ID_TIMER_TOPMOST 1
#define ID_TIMER_BATTERY 2
#define WM_OPEN_SETTINGS (WM_APP + 1)

#define WIDGET_WIDTH 300
#define WIDGET_HEIGHT 40
#define WIDGET_OFFSET_X 20
#define MUTEX_NAME "Enterprise_BatteryWidget_Mutex"
#define REGISTRY_APP_NAME "BatteryWidgetNative"

// --- GLOBAL VARIABLES ---
char batteryText[128] = "Loading...";
HWND hwndGlobal = NULL;
HHOOK mouseHook = NULL;

// --- AUTO-UPDATE ENGINE ---
void CheckForUpdates(void) {
    char currentExePath[MAX_PATH];
    char tempVersionPath[MAX_PATH];
    char newExePath[MAX_PATH];
    char batPath[MAX_PATH];
    char dir[MAX_PATH];

    GetModuleFileName(NULL, currentExePath, MAX_PATH);
    strncpy(dir, currentExePath, MAX_PATH);
    char *lastSlash = strrchr(dir, '\\');
    if (lastSlash) *lastSlash = '\0';

    snprintf(tempVersionPath, sizeof(tempVersionPath), "%s\\version_temp.txt", dir);
    snprintf(newExePath, sizeof(newExePath), "%s\\BatteryWidget_new.exe", dir);
    snprintf(batPath, sizeof(batPath), "%s\\updater.bat", dir);

    // Silently query GitHub for the latest version tag
    if (URLDownloadToFile(NULL, GITHUB_VERSION_URL, tempVersionPath, 0, NULL) == S_OK) {
        FILE *f = fopen(tempVersionPath, "r");
        if (f) {
            int onlineVersion = 0;
            if (fscanf(f, "%d", &onlineVersion) == 1) {
                fclose(f);
                DeleteFile(tempVersionPath);

                if (onlineVersion > CURRENT_VERSION) {
                    // Download replacement binary
                    if (URLDownloadToFile(NULL, GITHUB_EXE_URL, newExePath, 0, NULL) == S_OK) {
                        FILE *bat = fopen(batPath, "w");
                        if (bat) {
                            // Relay script: waits for current process termination, swaps binaries, restarts, self-destructs
                            fprintf(bat, "@echo off\n");
                            fprintf(bat, "timeout /t 2 /nobreak > NUL\n");
                            fprintf(bat, "del \"%s\"\n", currentExePath);
                            fprintf(bat, "move \"%s\" \"%s\"\n", newExePath, currentExePath);

                            // Aggiungiamo il flag --updated per scatenare il popup al prossimo avvio
                            fprintf(bat, "start \"\" \"%s\" --updated\n", currentExePath);

                            fprintf(bat, "del \"%%~f0\"\n");
                            fclose(bat);

                            ShellExecute(NULL, "open", batPath, NULL, NULL, SW_HIDE);
                            ExitProcess(0);
                        }
                    }
                }
            } else {
                fclose(f);
                DeleteFile(tempVersionPath);
            }
        }
    }
}

// --- MOUSE HOOK: Handles clicks on the transparent window ---
LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        if (wParam == WM_LBUTTONUP || wParam == WM_RBUTTONUP) {
            MSLLHOOKSTRUCT *mhs = (MSLLHOOKSTRUCT*)lParam;
            RECT rect;

            if (hwndGlobal && GetWindowRect(hwndGlobal, &rect)) {
                if (PtInRect(&rect, mhs->pt)) {
                    if (wParam == WM_LBUTTONUP) {
                        PostMessage(hwndGlobal, WM_OPEN_SETTINGS, 0, 0);
                    }
                    else if (wParam == WM_RBUTTONUP) {
                        PostMessage(hwndGlobal, WM_CLOSE, 0, 0);
                    }
                    return 1;
                }
            }
        }
    }
    return CallNextHookEx(mouseHook, nCode, wParam, lParam);
}

// --- REGISTRY: Registers the application for auto-start on boot ---
void AddToStartup(void) {
    char path[MAX_PATH];
    GetModuleFileName(NULL, path, MAX_PATH);
    HKEY hKey;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        RegSetValueEx(hKey, REGISTRY_APP_NAME, 0, REG_SZ, (const BYTE*)path, strlen(path) + 1);
        RegCloseKey(hKey);
    }
}

// --- HARDWARE I/O: Fetches battery status via Windows API ---
void UpdateBatteryText(HWND hwnd) {
    SYSTEM_POWER_STATUS sps;
    if (GetSystemPowerStatus(&sps)) {
        if (sps.BatteryLifePercent == 255) {
            sprintf(batteryText, "No battery detected");
        } else {
            int percent = sps.BatteryLifePercent;
            if (sps.ACLineStatus == 1) {
                sprintf(batteryText, "%d%% | Charging", percent);
            } else if (sps.BatteryLifeTime == (DWORD)-1) {
                sprintf(batteryText, "%d%% | Calculating...", percent);
            } else {
                int hours = sps.BatteryLifeTime / 3600;
                int mins = (sps.BatteryLifeTime % 3600) / 60;
                sprintf(batteryText, "%d%% | %dh %dm", percent, hours, mins);
            }
        }
        InvalidateRect(hwnd, NULL, TRUE);
    }
}

// --- WINDOW PROCEDURE ---
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HFONT hFont;

    switch (uMsg) {
        case WM_CREATE:
            hFont = CreateFont(22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                               OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                               VARIABLE_PITCH, "Segoe UI");

            SetTimer(hwnd, ID_TIMER_TOPMOST, 500, NULL);
            SetTimer(hwnd, ID_TIMER_BATTERY, 5000, NULL);
            UpdateBatteryText(hwnd);
            return 0;

        case WM_TIMER:
            if (wParam == ID_TIMER_TOPMOST) {
                SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
            } else if (wParam == ID_TIMER_BATTERY) {
                UpdateBatteryText(hwnd);
            }
            return 0;

        case WM_OPEN_SETTINGS:
            ShellExecute(NULL, "open", "ms-settings:powersleep", NULL, NULL, SW_SHOWNORMAL);
            return 0;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            SelectObject(hdc, hFont);
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(255, 255, 255));

            RECT rect;
            GetClientRect(hwnd, &rect);
            DrawText(hdc, batteryText, -1, &rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_DESTROY:
            DeleteObject(hFont);
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// --- ENTRY POINT ---
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    SetProcessDPIAware();

    HANDLE hMutex = CreateMutex(NULL, TRUE, MUTEX_NAME);
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        return 0;
    }

    AddToStartup();

    // Perform pre-flight silent update check before rendering UI
    CheckForUpdates();

    const char CLASS_NAME[] = "BatteryWidgetClass";
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    RegisterClass(&wc);

    RECT workArea;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int taskbarH = screenH - workArea.bottom;

    int y = (taskbarH > 0) ? workArea.bottom + (taskbarH - WIDGET_HEIGHT) / 2 : screenH - WIDGET_HEIGHT - 10;
    int x = WIDGET_OFFSET_X;

    hwndGlobal = CreateWindowEx(
        WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST,
        CLASS_NAME, "Battery Widget", WS_POPUP,
        x, y, WIDGET_WIDTH, WIDGET_HEIGHT, NULL, NULL, hInstance, NULL
    );

    if (hwndGlobal == NULL) return 0;

    SetLayeredWindowAttributes(hwndGlobal, RGB(0, 0, 0), 0, LWA_COLORKEY);
    ShowWindow(hwndGlobal, SW_SHOW);

    // --- CONTROLLO MESSAGGIO DI AGGIORNAMENTO ---
    // Se il programma è stato avviato dallo script .bat con il flag --updated, mostra il popup
    if (strstr(lpCmdLine, "--updated") != NULL) {
        MessageBox(NULL,
            "NanoBattery has been successfully updated to the latest version!\n\nEnjoy the new features.",
            "✨ Update Successful",
            MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
    }
    // --------------------------------------------

    mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc, hInstance, 0);

    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(mouseHook);
    ReleaseMutex(hMutex);
    CloseHandle(hMutex);
    return 0;
}