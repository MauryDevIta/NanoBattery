/**
 * Windows Native Battery Widget
 * A lightweight, zero-dependency, DPI-aware battery monitor for the Windows Taskbar.
 * Implements a global mouse hook for transparent UI interaction and Z-order persistence.
 */

#include <windows.h>
#include <stdio.h>
#include <shellapi.h>

// --- CONFIGURATION & CONSTANTS ---
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

// --- MOUSE HOOK: Handles clicks on the transparent window ---
LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        if (wParam == WM_LBUTTONUP || wParam == WM_RBUTTONUP) {
            MSLLHOOKSTRUCT *mhs = (MSLLHOOKSTRUCT*)lParam;
            RECT rect;

            // Verify if the click is within the widget's boundaries
            if (hwndGlobal && GetWindowRect(hwndGlobal, &rect)) {
                if (PtInRect(&rect, mhs->pt)) {
                    if (wParam == WM_LBUTTONUP) {
                        // Left Click: Asynchronously open Windows Power Settings
                        PostMessage(hwndGlobal, WM_OPEN_SETTINGS, 0, 0);
                    }
                    else if (wParam == WM_RBUTTONUP) {
                        // Right Click: Gracefully terminate the application
                        PostMessage(hwndGlobal, WM_CLOSE, 0, 0);
                    }
                    return 1; // Consume the event (prevents clicking underlying windows)
                }
            }
        }
    }
    return CallNextHookEx(mouseHook, nCode, wParam, lParam);
}

// --- REGISTRY: Registers the application for auto-start on boot ---
void AddToStartup() {
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
        // Trigger a UI repaint
        InvalidateRect(hwnd, NULL, TRUE);
    }
}

// --- WINDOW PROCEDURE: Handles Windows messages ---
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HFONT hFont;

    switch (uMsg) {
        case WM_CREATE:
            hFont = CreateFont(22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                               OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                               VARIABLE_PITCH, "Segoe UI");

            // Initialize system timers
            SetTimer(hwnd, ID_TIMER_TOPMOST, 500, NULL);   // Z-Order enforcement
            SetTimer(hwnd, ID_TIMER_BATTERY, 5000, NULL);  // Battery polling
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
            SetTextColor(hdc, RGB(255, 255, 255)); // White text

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
    // 1. Enable DPI Awareness for high-resolution displays
    SetProcessDPIAware();

    // 2. Prevent multiple instances (Race condition prevention)
    HANDLE hMutex = CreateMutex(NULL, TRUE, MUTEX_NAME);
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        return 0; // Exit silently
    }

    // 3. Register for Auto-Startup
    AddToStartup();

    // 4. Register Window Class
    const char CLASS_NAME[] = "BatteryWidgetClass";
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH); // Optimized resource allocation
    RegisterClass(&wc);

    // 5. Calculate Taskbar Coordinates
    RECT workArea;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int taskbarH = screenH - workArea.bottom;

    int y = (taskbarH > 0) ? workArea.bottom + (taskbarH - WIDGET_HEIGHT) / 2 : screenH - WIDGET_HEIGHT - 10;
    int x = WIDGET_OFFSET_X;

    // 6. Create the Overlay Window
    hwndGlobal = CreateWindowEx(
        WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST,
        CLASS_NAME, "Battery Widget", WS_POPUP,
        x, y, WIDGET_WIDTH, WIDGET_HEIGHT, NULL, NULL, hInstance, NULL
    );

    if (hwndGlobal == NULL) return 0;

    // Set Chroma Key Transparency (Black becomes invisible)
    SetLayeredWindowAttributes(hwndGlobal, RGB(0, 0, 0), 0, LWA_COLORKEY);
    ShowWindow(hwndGlobal, SW_SHOW);

    // 7. Install Global Mouse Hook
    mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc, hInstance, 0);

    // 8. Main Message Loop
    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // 9. Clean Exit
    UnhookWindowsHookEx(mouseHook);
    ReleaseMutex(hMutex);
    CloseHandle(hMutex);
    return 0;
}