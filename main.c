/**
 * Windows Native Battery Widget with Remote Auto-Updater - VERSION 2.0.2
 */

#include <windows.h>
#include <stdio.h>
#include <shellapi.h>
#include <string.h>

#include "widget_globals.h"
#include "updater.h"
#include "battery.h"
#include "hooks.h"
#include "system_utils.h"

typedef BOOL(WINAPI *SetProcessDpiAwarenessContext_t)(HANDLE);
#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((HANDLE)-4)
#endif

// --- WINDOW PROCEDURE ---
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HFONT hFontMain;
    static HFONT hFontSmall;

    switch (uMsg) {
        case WM_CREATE:
            hFontMain = CreateFont(20, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                               OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, "Segoe UI Variable Display");
            hFontSmall = CreateFont(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                               OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, "Segoe UI");

            LoadSettings(); 

            SetTimer(hwnd, ID_TIMER_TOPMOST, 500, NULL);
            SetTimer(hwnd, ID_TIMER_BATTERY, 5000, NULL);
            UpdateBatteryText(hwnd);
            return 0;

        case WM_POWERBROADCAST:
            if (wParam == PBT_APMPOWERSTATUSCHANGE || wParam == PBT_APMRESUMEAUTOMATIC) {
                UpdateBatteryText(hwnd);
            }
            return TRUE;

        case WM_OPEN_SETTINGS:
            ShellExecute(NULL, "open", "ms-settings:powersleep", NULL, NULL, SW_SHOWNORMAL);
            return 0;

        case WM_USER_SHOW_MENU: {
            int x = (int)wParam;
            int y = (int)lParam;
            HMENU hMenu = CreatePopupMenu();
            
            UINT uStartupFlag = IsInStartup() ? MF_CHECKED : MF_UNCHECKED;
            UINT uAlertsFlag = alertsEnabled ? MF_CHECKED : MF_UNCHECKED;

            AppendMenu(hMenu, MF_STRING, 1, "Impostazioni Batteria");
            AppendMenu(hMenu, MF_STRING, 2, "Genera Battery Report (HTML)");
            AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
            AppendMenu(hMenu, MF_STRING | uStartupFlag, 4, "Avvio automatico con Windows");
            AppendMenu(hMenu, MF_STRING | uAlertsFlag, 5, "Avvisi Intelligenti (80% / 20%)");
            AppendMenu(hMenu, MF_STRING, 6, "Cerca aggiornamenti...");
            AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
            AppendMenu(hMenu, MF_STRING, 3, "Esci da NanoBattery");

            SetForegroundWindow(hwnd);
            
            isMenuOpen = 1; 
            int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, x, y, 0, hwnd, NULL);
            isMenuOpen = 0; 
            
            DestroyMenu(hMenu);

            if (cmd == 1) {
                ShellExecute(NULL, "open", "ms-settings:powersleep", NULL, NULL, SW_SHOWNORMAL);
            }
            if (cmd == 2) {
                ShellExecute(NULL, "open", "cmd.exe", "/c powercfg /batteryreport /output \"%TEMP%\\battery_report.html\" && start \"\" \"%TEMP%\\battery_report.html\"", NULL, SW_HIDE);
            }
            if (cmd == 4) {
                if (IsInStartup()) RemoveFromStartup(); else AddToStartup();
            }
            if (cmd == 5) {
                alertsEnabled = !alertsEnabled;
                SaveSettings();
            }
            if (cmd == 6) {
                CheckForUpdatesManual(hwnd);
            }
            if (cmd == 3) {
                PostQuitMessage(0);
            }
            return 0;
        }

        case WM_TIMER:
            if (wParam == ID_TIMER_TOPMOST) SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
            else if (wParam == ID_TIMER_BATTERY) UpdateBatteryText(hwnd);
            else if (wParam == ID_TIMER_TRAY) {
                RemoveNotification(hwnd);
                KillTimer(hwnd, ID_TIMER_TRAY);
            }
            return 0;

        case WM_USER_HOVER_ENTER:
        case WM_USER_HOVER_LEAVE:
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdcWindow = BeginPaint(hwnd, &ps);

            RECT rect;
            GetClientRect(hwnd, &rect);
            int width = rect.right - rect.left;
            int height = rect.bottom - rect.top;

            // ---- DOUBLE BUFFERING ----
            HDC hdc = CreateCompatibleDC(hdcWindow);
            HBITMAP hbm = CreateCompatibleBitmap(hdcWindow, width, height);
            HBITMAP hOldBm = (HBITMAP)SelectObject(hdc, hbm);

            // Sfondo Trasparente
            FillRect(hdc, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH)); 
            SetBkMode(hdc, TRANSPARENT);

            // 1. STATISTICHE AVANZATE IN HOVER
            if (isHovering && strlen(advancedBatteryText) > 0) {
                HBRUSH hBrushBg = CreateSolidBrush(RGB(24, 24, 24));
                RECT advRect = rect;
                advRect.bottom = DETAILS_HEIGHT;
                
                SelectObject(hdc, GetStockObject(NULL_PEN));
                SelectObject(hdc, hBrushBg);
                RoundRect(hdc, advRect.left, advRect.top, advRect.right, advRect.bottom, 15, 15);
                DeleteObject(hBrushBg);

                SelectObject(hdc, hFontSmall);
                SetTextColor(hdc, RGB(220, 220, 220)); 
                RECT textRect = advRect;
                textRect.left += 15;
                textRect.top += 12;
                DrawText(hdc, advancedBatteryText, -1, &textRect, DT_LEFT | DT_TOP);
            }

            // 2. TESTO PRINCIPALE ED ICONA
            RECT mainRect;
            mainRect.left = 0;
            mainRect.top = DETAILS_HEIGHT;
            mainRect.right = rect.right;
            mainRect.bottom = rect.bottom;

            int percent = 0;
            sscanf(batteryText, "%d%%", &percent);

            int iconX = 0;
            int iconY = DETAILS_HEIGHT + (MAIN_BAR_HEIGHT - 14) / 2;
            int iconW = 26;
            int iconH = 14;

            HPEN hPenIcon = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
            HPEN hOldPen = SelectObject(hdc, hPenIcon);
            HBRUSH hNullBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
            HBRUSH hOldBrush = SelectObject(hdc, hNullBrush);

            // Corpo
            RoundRect(hdc, iconX, iconY, iconX + iconW, iconY + iconH, 4, 4);
            // Polo
            Rectangle(hdc, iconX + iconW, iconY + 4, iconX + iconW + 3, iconY + 10);

            // Livello colore
            if (percent > 0 && percent <= 100) {
                HBRUSH hFillBrush;
                if (percent > 50) hFillBrush = CreateSolidBrush(RGB(50, 210, 50));
                else if (percent > 20) hFillBrush = CreateSolidBrush(RGB(230, 190, 40));
                else hFillBrush = CreateSolidBrush(RGB(230, 50, 50));
                
                int fillWidth = (iconW - 4) * percent / 100;
                if (fillWidth < 1) fillWidth = 1;
                
                RECT fillRect = {iconX + 2, iconY + 2, iconX + 2 + fillWidth, iconY + iconH - 2};
                FillRect(hdc, &fillRect, hFillBrush);
                DeleteObject(hFillBrush);
            }

            SelectObject(hdc, hOldPen);
            SelectObject(hdc, hOldBrush);
            DeleteObject(hPenIcon);

            // Testo
            mainRect.left = iconX + iconW + 12;
            SelectObject(hdc, hFontMain);
            SetTextColor(hdc, RGB(255, 255, 255));
            DrawText(hdc, batteryText, -1, &mainRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

            // Flush
            BitBlt(hdcWindow, 0, 0, width, height, hdc, 0, 0, SRCCOPY);

            SelectObject(hdc, hOldBm);
            DeleteObject(hbm);
            DeleteDC(hdc);

            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_DESTROY:
            DeleteObject(hFontMain);
            DeleteObject(hFontSmall);
            RemoveNotification(hwnd);
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// --- ENTRY POINT ---
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    HMODULE hUser32 = LoadLibrary("user32.dll");
    if (hUser32) {
        SetProcessDpiAwarenessContext_t setDpi = (SetProcessDpiAwarenessContext_t)GetProcAddress(hUser32, "SetProcessDpiAwarenessContext");
        if (setDpi) {
            setDpi(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        } else {
            SetProcessDPIAware();
        }
        FreeLibrary(hUser32);
    } else {
        SetProcessDPIAware();
    }

    HANDLE hMutex = CreateMutex(NULL, TRUE, MUTEX_NAME);
    if (GetLastError() == ERROR_ALREADY_EXISTS) return 0;

    if (strstr(lpCmdLine, "--updated") == NULL) {
        InitUpdaterAsync();
    }

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

    int taskbarY = (taskbarH > 0) ? workArea.bottom + (taskbarH - MAIN_BAR_HEIGHT) / 2 : screenH - MAIN_BAR_HEIGHT - 10;
    
    int y = taskbarY - DETAILS_HEIGHT;
    int x = WIDGET_OFFSET_X;

    hwndGlobal = CreateWindowEx(
        WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST,
        CLASS_NAME, "Battery Widget", WS_POPUP,
        x, y, WIDGET_WIDTH, WIDGET_HEIGHT, NULL, NULL, hInstance, NULL
    );

    if (hwndGlobal == NULL) return 0;

    SetLayeredWindowAttributes(hwndGlobal, RGB(0, 0, 0), 0, LWA_COLORKEY);
    ShowWindow(hwndGlobal, SW_SHOW);

    if (strstr(lpCmdLine, "--updated") != NULL) {
        ShowNotification(hwndGlobal, "NanoBattery 2.0.2", "Aggiornamento applicato con successo.\nGoditi la nuova grafica unita a prestazioni formidabili!", 0);
    }

    mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc, hInstance, 0);
    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(mouseHook);\n    ReleaseMutex(hMutex);\n    CloseHandle(hMutex);\n    return 0;\n}\n