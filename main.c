#include <windows.h>
#include <stdio.h>

#define ID_TIMER_TOPMOST 1
#define ID_TIMER_BATTERY 2
#define WIDGET_WIDTH 250
#define WIDGET_HEIGHT 30

char batteryText[100] = "Caricamento...";

// Aggiunge l'eseguibile al registro di sistema
void AddToStartup() {
    char path[MAX_PATH];
    GetModuleFileName(NULL, path, MAX_PATH);
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        RegSetValueEx(hKey, "BatteryWidgetNative", 0, REG_SZ, (const BYTE*)path, strlen(path) + 1);
        RegCloseKey(hKey);
    }
}

// Legge i dati batteria nativamente da Windows e aggiorna il testo
void UpdateBatteryText(HWND hwnd) {
    SYSTEM_POWER_STATUS sps;
    if (GetSystemPowerStatus(&sps)) {
        if (sps.BatteryLifePercent == 255) {
            sprintf(batteryText, "Nessuna batteria rilevata");
        } else {
            int percent = sps.BatteryLifePercent;
            if (sps.ACLineStatus == 1) {
                sprintf(batteryText, "%d%% | In carica", percent);
            } else if (sps.BatteryLifeTime == (DWORD)-1) {
                sprintf(batteryText, "%d%% | Calcolo...", percent);
            } else {
                int hours = sps.BatteryLifeTime / 3600;
                int mins = (sps.BatteryLifeTime % 3600) / 60;
                sprintf(batteryText, "%d%% | %dh e %dm", percent, hours, mins);
            }
        }
        // Forza Windows a ridisegnare solo il testo aggiornato
        InvalidateRect(hwnd, NULL, TRUE);
    }
}

// Gestore Messaggi di Windows (Message Loop)
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HFONT hFont;

    switch (uMsg) {
        case WM_CREATE:
            // Crea il font (Segoe UI, grassetto, dimensione 16)
            hFont = CreateFont(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                               OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                               VARIABLE_PITCH, "Segoe UI");

            // Imposta i timer (senza usare thread esterni!)
            SetTimer(hwnd, ID_TIMER_TOPMOST, 500, NULL);   // Ciclo anti-swipe (ogni mezzo secondo)
            SetTimer(hwnd, ID_TIMER_BATTERY, 5000, NULL);  // Ciclo lettura batteria (ogni 5 secondi)
            UpdateBatteryText(hwnd);
            return 0;

        case WM_TIMER:
            if (wParam == ID_TIMER_TOPMOST) {
                // Forza Z-Order sopra a tutto
                SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
            } else if (wParam == ID_TIMER_BATTERY) {
                UpdateBatteryText(hwnd);
            }
            return 0;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            SelectObject(hdc, hFont);
            SetBkMode(hdc, TRANSPARENT); // Sfondo del testo trasparente
            SetTextColor(hdc, RGB(255, 255, 255)); // Colore testo Bianco

            RECT rect;
            GetClientRect(hwnd, &rect);
            DrawText(hdc, batteryText, -1, &rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_DESTROY:
            DeleteObject(hFont);
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// Entry point di Windows (sostituisce il main standard)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // 1. Controllo Istanza Singola
    HANDLE hMutex = CreateMutex(NULL, TRUE, "MinimalBatteryWidget_Native_Mutex_123");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        return 0; // Chiude silenziosamente se è già aperto
    }

    // 2. Avvio automatico
    AddToStartup();

    // 3. Registrazione Classe Finestra
    const char CLASS_NAME[] = "BatteryWidgetClass";
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = CreateSolidBrush(RGB(0, 0, 0)); // Sfondo nero (che diventerà invisibile)
    RegisterClass(&wc);

    // 4. Calcolo Posizione (sulla Taskbar)
    RECT workArea;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int taskbarH = screenH - workArea.bottom;

    int y = (taskbarH > 0) ? workArea.bottom + (taskbarH - WIDGET_HEIGHT) / 2 : screenH - WIDGET_HEIGHT - 10;
    int x = 20;

    // 5. Creazione Finestra Invisibile e Click-through
    HWND hwnd = CreateWindowEx(
        WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST,
        CLASS_NAME,
        "Battery Widget",
        WS_POPUP, // Finestra pura senza bordi o barra del titolo
        x, y, WIDGET_WIDTH, WIDGET_HEIGHT,
        NULL, NULL, hInstance, NULL
    );

    if (hwnd == NULL) return 0;

    // Rende il colore NERO (RGB 0,0,0) completamente trasparente
    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
    ShowWindow(hwnd, SW_SHOW);

    // 6. Ciclo dei Messaggi Nativo (non usa % CPU quando inattivo)
    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    ReleaseMutex(hMutex);
    CloseHandle(hMutex);
    return 0;
}