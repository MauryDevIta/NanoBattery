#ifndef UPDATER_H
#define UPDATER_H

#include <windows.h>

void InitUpdaterAsync(void);
void CheckForUpdates(void);
void CheckForUpdatesManual(HWND hwnd);

#endif // UPDATER_H
