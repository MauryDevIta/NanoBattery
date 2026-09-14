#include "widget_globals.h"

char batteryText[128] = "Loading...";
char advancedBatteryText[256] = "";
HWND hwndGlobal = NULL;
HHOOK mouseHook = NULL;
int isHovering = 0;
int isMenuOpen = 0;
int alertsEnabled = 1;