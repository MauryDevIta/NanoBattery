#ifndef WIDGET_GLOBALS_H
#define WIDGET_GLOBALS_H

#include <windows.h>

// --- CONFIGURATION & CONSTANTS ---
// Version is now defined in CMakeLists.txt via version.txt
#define GITHUB_VERSION_URL "https://raw.githubusercontent.com/MauryDevIta/NanoBattery/refs/heads/main/version.txt"
#define GITHUB_EXE_URL     "https://raw.githubusercontent.com/MauryDevIta/NanoBattery/refs/heads/main/battery_widget.exe"

#define ID_TIMER_TOPMOST 1
#define ID_TIMER_BATTERY 2
#define ID_TIMER_TRAY    3

#define WM_OPEN_SETTINGS (WM_APP + 1)
#define WM_USER_HOVER_ENTER (WM_APP + 2)
#define WM_USER_HOVER_LEAVE (WM_APP + 3)
#define WM_USER_SHOW_MENU   (WM_APP + 4)

#define WIDGET_WIDTH 340
#define MAIN_BAR_HEIGHT 40
#define DETAILS_HEIGHT 85
#define WIDGET_HEIGHT (MAIN_BAR_HEIGHT + DETAILS_HEIGHT) // 125 px
#define WIDGET_OFFSET_X 20

#define MUTEX_NAME "Enterprise_BatteryWidget_Mutex"
#define REGISTRY_APP_NAME "BatteryWidgetNative"

// --- GLOBAL VARIABLES ---
extern char batteryText[128];
extern char advancedBatteryText[256];
extern HWND hwndGlobal;
extern HHOOK mouseHook;
extern int isHovering;
extern int isMenuOpen;
extern int alertsEnabled;

#endif // WIDGET_GLOBALS_H
