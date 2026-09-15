#ifndef BATTERY_H
#define BATTERY_H

#pragma once
#include <windows.h>

void FetchAdvancedBatteryStats(void);
void UpdateBatteryText(HWND hwnd);

#endif // BATTERY_H
