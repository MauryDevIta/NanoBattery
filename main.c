#include <windows.h>
#include <stdio.h>

#define ID_TIMER_TOPMOST 1
#define ID_TIMER_BATTERY 2
#define WIDGET_WIDTH 250
#define WIDGET_HEIGHT 30

char batteryText[100] = "Loading...";

// Adds the executable to the system registry for auto-start
void AddToStartup() {
    char path[MAX_PATH];
    GetModuleFileName(NULL, path, MAX_PATH);
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        RegSetValueEx(hKey, "BatteryWidgetNative", 0, REG_SZ, (const BYTE*)path, strlen(path) + 1);
        RegCloseKey(hKey);
    }
}

// Reads battery data natively from Windows and updates the text
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
        // Forces Windows to redraw only the updated text
        InvalidateRect(hwnd, NULL, TRUE);
    }
}

// Windows Message Handler (Message Loop)
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HFONT hFont;

    switch (uMsg) {
        case WM_CREATE:
            // Creates the font (Segoe UI, bold, size 16)
            hFont = CreateFont(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                               OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                               VARIABLE_PITCH, "Segoe UI");

            // Sets up the timers (without using external threads!)
            SetTimer(hwnd, ID_TIMER_TOPMOST, 500, NULL);   // Anti-swipe cycle (every half second)
            SetTimer(hwnd, ID_TIMER_BATTERY, 5000, NULL);  // Battery reading cycle (every 5 seconds)
            UpdateBatteryText(hwnd);
            return 0;

        case WM_TIMER:
            if (wParam == ID_TIMER_TOPMOST) {
                // Forces Z-Order on top of everything
                SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
            } else if (wParam == ID_TIMER_BATTERY) {
                UpdateBatteryText(hwnd);
            }
            return 0;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            SelectObject(hdc, hFont);
            SetBkMode(hdc, TRANSPARENT); // Transparent text background
            SetTextColor(hdc, RGB(255, 255, 255)); // White text color

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

// Windows entry point (replaces standard main)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // 1. Single Instance Check
    HANDLE hMutex = CreateMutex(NULL, TRUE, "MinimalBatteryWidget_Native_Mutex_123");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        return 0; // Closes silently if already running
    }

    // 2. Auto-start on boot
    AddToStartup();

    // 3. Window Class Registration
    const char CLASS_NAME[] = "BatteryWidgetClass";
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = CreateSolidBrush(RGB(0, 0, 0)); // Black background (which will become invisible)
    RegisterClass(&wc);

    // 4. Position Calculation (on the Taskbar)
    RECT workArea;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int taskbarH = screenH - workArea.bottom;

    int y = (taskbarH > 0) ? workArea.bottom + (taskbarH - WIDGET_HEIGHT) / 2 : screenH - WIDGET_HEIGHT - 10;
    int x = 20;

    // 5. Invisible and Click-through Window Creation
    HWND hwnd = CreateWindowEx(
        WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST,
        CLASS_NAME,
        "Battery Widget",
        WS_POPUP, // Pure window without borders or title bar
        x, y, WIDGET_WIDTH, WIDGET_HEIGHT,
        NULL, NULL, hInstance, NULL
    );

    if (hwnd == NULL) return 0;

    // Makes the BLACK color (RGB 0,0,0) completely transparent
    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
    ShowWindow(hwnd, SW_SHOW);

    // 6. Native Message Loop (uses 0% CPU when idle)
    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    ReleaseMutex(hMutex);
    CloseHandle(hMutex);
    return 0;
}