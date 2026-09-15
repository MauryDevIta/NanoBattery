#include "widget_globals.h"

char batteryText[128] = "Loading...";
char advancedBatteryText[256] = "";
int batteryPercent = 0;
int isCharging = 0;
int isBatteryPresent = 1;

HWND hwndGlobal = NULL;
HHOOK mouseHook = NULL;
int isHovering = 0;
int isMenuOpen = 0;
int alertsEnabled = 1;
