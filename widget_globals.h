#ifndef WIDGET_GLOBALS_H
#define WIDGET_GLOBALS_H

#pragma once

#include <windows.h>

// --- VERSION CONFIGURATION ---
// CURRENT_VERSION numeric build ID (overridden by CMakeLists.txt via version.txt)
#ifndef CURRENT_VERSION
#define CURRENT_VERSION 11
#endif
#define APP_VERSION_STR "2.0.2"

// --- REMOTE REPOSITORY CONFIGURATION ---
#define GITHUB_VERSION_URL "https://raw.githubusercontent.com/MauryDevIta/NanoBattery/refs/heads/main/version.txt"
#define GITHUB_EXE_URL     "https://raw.githubusercontent.com/MauryDevIta/NanoBattery/refs/heads/main/battery_widget.exe"

// --- TIMER IDENTIFIERS ---
#define ID_TIMER_TOPMOST 1
#define ID_TIMER_BATTERY 2
#define ID_TIMER_TRAY    3

// --- CUSTOM WINDOW MESSAGES ---
#define WM_OPEN_SETTINGS       (WM_APP + 1)
#define WM_USER_HOVER_ENTER    (WM_APP + 2)
#define WM_USER_HOVER_LEAVE    (WM_APP + 3)
#define WM_USER_SHOW_MENU      (WM_APP + 4)
#define WM_USER_TRAY_CALLBACK  (WM_APP + 5)

// --- TRAY ICON IDENTIFIER ---
#define ID_TRAY_ICON 1001

// --- CONTEXT MENU IDENTIFIERS ---
#define ID_MENU_SETTINGS 1
#define ID_MENU_REPORT   2
#define ID_MENU_EXIT     3
#define ID_MENU_STARTUP  4
#define ID_MENU_ALERTS   5
#define ID_MENU_UPDATE   6

// --- BATTERY ALERT THRESHOLDS (%) ---
#define BATTERY_ALERT_HIGH 80
#define BATTERY_ALERT_LOW  20

// --- UI LAYOUT CONSTANTS ---
#define WIDGET_WIDTH 340
#define MAIN_BAR_HEIGHT 40
#define DETAILS_HEIGHT 85
#define WIDGET_HEIGHT (MAIN_BAR_HEIGHT + DETAILS_HEIGHT) // 125 px
#define WIDGET_OFFSET_X 20

// --- SYSTEM CONSTANTS ---
#define MUTEX_NAME "Enterprise_BatteryWidget_Mutex"
#define REGISTRY_APP_NAME "BatteryWidgetNative"

// --- GLOBAL VARIABLES ---
extern char batteryText[128];
extern char advancedBatteryText[256];
extern int batteryPercent;
extern int isCharging;
extern int isBatteryPresent;

extern HWND hwndGlobal;
extern HHOOK mouseHook;
extern int isHovering;
extern int isMenuOpen;
extern int alertsEnabled;

#endif // WIDGET_GLOBALS_H
