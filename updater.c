#include "updater.h"
#include "widget_globals.h"
#include <windows.h>
#include <stdio.h>
#include <urlmon.h>
#include <shellapi.h>

void CheckForUpdates(void) {
    char currentExePath[MAX_PATH];
    char tempVersionPath[MAX_PATH], newExePath[MAX_PATH], batPath[MAX_PATH], dir[MAX_PATH];

    GetModuleFileName(NULL, currentExePath, MAX_PATH);
    strncpy(dir, currentExePath, MAX_PATH);
    char *lastSlash = strrchr(dir, '\\');
    if (lastSlash) *lastSlash = '\0';

    snprintf(tempVersionPath, sizeof(tempVersionPath), "%s\\version_temp.txt", dir);
    snprintf(newExePath, sizeof(newExePath), "%s\\BatteryWidget_new.exe", dir);
    snprintf(batPath, sizeof(batPath), "%s\\updater.bat", dir);

    if (URLDownloadToFile(NULL, GITHUB_VERSION_URL, tempVersionPath, 0, NULL) == S_OK) {
        FILE *f = fopen(tempVersionPath, "r");
        if (f) {
            int onlineVersion = 0;
            if (fscanf(f, "%d", &onlineVersion) == 1) {
                fclose(f);
                DeleteFile(tempVersionPath);

                if (onlineVersion > CURRENT_VERSION) {
                    if (URLDownloadToFile(NULL, GITHUB_EXE_URL, newExePath, 0, NULL) == S_OK) {
                        FILE *bat = fopen(batPath, "w");
                        if (bat) {
                            fprintf(bat, "@echo off\ntimeout /t 2 /nobreak > NUL\ndel \"%s\"\nmove \"%s\" \"%s\"\nstart \"\" \"%s\" --updated\ndel \"%%~f0\"\n", currentExePath, newExePath, currentExePath, currentExePath);
                            fclose(bat);
                            ShellExecute(NULL, "open", batPath, NULL, NULL, SW_HIDE);
                            ExitProcess(0);
                        }
                    }
                }
            } else {
                fclose(f);
                DeleteFile(tempVersionPath);
            }
        }
    }
}

DWORD WINAPI UpdateThreadProc(LPVOID lpParam) {
    CheckForUpdates();
    return 0;
}

void InitUpdaterAsync(void) {
    CreateThread(NULL, 0, UpdateThreadProc, NULL, 0, NULL);
}

void CheckForUpdatesManual(HWND hwnd) {
    char currentExePath[MAX_PATH];
    char tempVersionPath[MAX_PATH], newExePath[MAX_PATH], batPath[MAX_PATH], dir[MAX_PATH];

    GetModuleFileName(NULL, currentExePath, MAX_PATH);
    strncpy(dir, currentExePath, MAX_PATH);
    char *lastSlash = strrchr(dir, '\\');
    if (lastSlash) *lastSlash = '\0';

    snprintf(tempVersionPath, sizeof(tempVersionPath), "%s\\version_temp.txt", dir);
    snprintf(newExePath, sizeof(newExePath), "%s\\BatteryWidget_new.exe", dir);
    snprintf(batPath, sizeof(batPath), "%s\\updater.bat", dir);

    if (URLDownloadToFile(NULL, GITHUB_VERSION_URL, tempVersionPath, 0, NULL) == S_OK) {
        FILE *f = fopen(tempVersionPath, "r");
        if (f) {
            int onlineVersion = 0;
            if (fscanf(f, "%d", &onlineVersion) == 1) {
                fclose(f);
                DeleteFile(tempVersionPath);

                if (onlineVersion > CURRENT_VERSION) {
                    if (MessageBox(hwnd, "Nuova versione disponibile! Vuoi aggiornare ora?", "NanoBattery Update", MB_YESNO | MB_ICONINFORMATION | MB_TOPMOST) == IDYES) {
                        if (URLDownloadToFile(NULL, GITHUB_EXE_URL, newExePath, 0, NULL) == S_OK) {
                            FILE *bat = fopen(batPath, "w");
                            if (bat) {
                                fprintf(bat, "@echo off\ntimeout /t 2 /nobreak > NUL\ndel \"%s\"\nmove \"%s\" \"%s\"\nstart \"\" \"%s\" --updated\ndel \"%%~f0\"\n", currentExePath, newExePath, currentExePath, currentExePath);
                                fclose(bat);
                                ShellExecute(NULL, "open", batPath, NULL, NULL, SW_HIDE);
                                ExitProcess(0);
                            }
                        }
                    }
                } else {
                    MessageBox(hwnd, "L'applicazione è già aggiornata all'ultima versione.", "NanoBattery - Update Info", MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
                }
                return;
            } else {
                fclose(f);
            }
        }
        DeleteFile(tempVersionPath);
    }
    MessageBox(hwnd, "Impossibile controllare gli aggiornamenti. Verifica la connessione internet.", "NanoBattery - Errore", MB_OK | MB_ICONERROR | MB_TOPMOST);
}
