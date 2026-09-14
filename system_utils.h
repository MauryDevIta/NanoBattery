#ifndef SYSTEM_UTILS_H
#define SYSTEM_UTILS_H

#include <windows.h>

void AddToStartup(void);
void RemoveFromStartup(void);
int IsInStartup(void);

void LoadSettings(void);
void SaveSettings(void);

void ShowNotification(HWND hwnd, const char* title, const char* msg, int isWarning);
void RemoveNotification(HWND hwnd);

#endif // SYSTEM_UTILS_H
