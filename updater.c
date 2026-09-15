#include "updater.h"
#include "widget_globals.h"
#include <windows.h>
#include <stdio.h>
#include <urlmon.h>
#include <shellapi.h>

static void BuildUpdaterScript(const char *batPath, const char *currentExe, const char *newExe) {
    FILE *bat = fopen(batPath, "w");
    if (bat) {
        fprintf(bat,
                "@echo off\n"
                ":retry\n"
                "timeout /t 1 /nobreak > NUL\n"
                "del \"%s\" > NUL 2>&1\n"
                "if exist \"%s\" goto retry\n"
                "move \"%s\" \"%s\" > NUL\n"
                "start \"\" \"%s\" --updated\n"
                "del \"%%~f0\"\n",
                currentExe, currentExe, newExe, currentExe, currentExe);
        fclose(bat);
    }
}

void CheckForUpdates(void) {
    char currentExePath[MAX_PATH];
    char tempVersionPath[MAX_PATH], newExePath[MAX_PATH], batPath[MAX_PATH], dir[MAX_PATH];

    if (GetModuleFileName(NULL, currentExePath, MAX_PATH) == 0) return;
    snprintf(dir, sizeof(dir), "%s", currentExePath);
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
                        BuildUpdaterScript(batPath, currentExePath, newExePath);
                        ShellExecute(NULL, "open", batPath, NULL, NULL, SW_HIDE);
                        ExitProcess(0);
                    }
                }
                return;
            }
            fclose(f);
        }
        DeleteFile(tempVersionPath);
    }
}

static DWORD WINAPI UpdateThreadProc(LPVOID lpParam) {
    (void)lpParam;
    CheckForUpdates();
    return 0;
}

void InitUpdaterAsync(void) {
    HANDLE hThread = CreateThread(NULL, 0, UpdateThreadProc, NULL, 0, NULL);
    if (hThread) {
        CloseHandle(hThread);
    }
}

void CheckForUpdatesManual(HWND hwnd) {
    char currentExePath[MAX_PATH];
    char tempVersionPath[MAX_PATH], newExePath[MAX_PATH], batPath[MAX_PATH], dir[MAX_PATH];

    if (GetModuleFileName(NULL, currentExePath, MAX_PATH) == 0) return;
    snprintf(dir, sizeof(dir), "%s", currentExePath);
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
                    char msgPrompt[256];
                    snprintf(msgPrompt, sizeof(msgPrompt),
                             "Nuova versione disponibile (build %d)!\nVersione attuale: build %d (v%s).\n\nVuoi aggiornare ora?",
                             onlineVersion, CURRENT_VERSION, APP_VERSION_STR);

                    if (MessageBox(hwnd, msgPrompt, "NanoBattery Update", MB_YESNO | MB_ICONINFORMATION | MB_TOPMOST) == IDYES) {
                        if (URLDownloadToFile(NULL, GITHUB_EXE_URL, newExePath, 0, NULL) == S_OK) {
                            BuildUpdaterScript(batPath, currentExePath, newExePath);
                            ShellExecute(NULL, "open", batPath, NULL, NULL, SW_HIDE);
                            ExitProcess(0);
                        } else {
                            MessageBox(hwnd, "Impossibile scaricare il file di aggiornamento.", "NanoBattery - Errore", MB_OK | MB_ICONERROR | MB_TOPMOST);
                        }
                    }
                } else {
                    char msgCurrent[256];
                    snprintf(msgCurrent, sizeof(msgCurrent),
                             "L'applicazione \xC3\xA8 gi\xC3\xA0 aggiornata all'ultima versione.\nVersione: %s (build %d)",
                             APP_VERSION_STR, CURRENT_VERSION);
                    MessageBox(hwnd, msgCurrent, "NanoBattery - Update Info", MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
                }
                return;
            }
            fclose(f);
        }
        DeleteFile(tempVersionPath);
    }
    MessageBox(hwnd, "Impossibile verificare gli aggiornamenti.\nControlla la connessione internet.", "NanoBattery - Errore", MB_OK | MB_ICONERROR | MB_TOPMOST);
}
