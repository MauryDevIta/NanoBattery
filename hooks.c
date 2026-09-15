#include "hooks.h"
#include "widget_globals.h"

LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        MSLLHOOKSTRUCT *mhs = (MSLLHOOKSTRUCT*)lParam;
        POINT pt = mhs->pt;
        RECT rect;

        if (hwndGlobal && GetWindowRect(hwndGlobal, &rect)) {
            // La barra principale della batteria occupa i 40 pixel in basso (sulla taskbar)
            RECT mainBarRect = rect;
            mainBarRect.top = rect.bottom - MAIN_BAR_HEIGHT;

            if (wParam == WM_MOUSEMOVE) {
                if (!isHovering) {
                    if (PtInRect(&mainBarRect, pt)) {
                        isHovering = 1;
                        PostMessage(hwndGlobal, WM_USER_HOVER_ENTER, 0, 0);
                    }
                } else {
                    if (!PtInRect(&rect, pt)) {
                        isHovering = 0;
                        PostMessage(hwndGlobal, WM_USER_HOVER_LEAVE, 0, 0);
                    }
                }
            } else {
                RECT activeRect = isHovering ? rect : mainBarRect;
                if ((wParam == WM_LBUTTONDOWN || wParam == WM_LBUTTONUP) && PtInRect(&activeRect, pt)) {
                    if (!isMenuOpen) {
                        if (wParam == WM_LBUTTONUP) {
                            PostMessage(hwndGlobal, WM_OPEN_SETTINGS, 0, 0);
                        }
                        return 1; // Intercetta il clic (SIA PRESSIONE CHE RILASCIO)
                    }
                } else if ((wParam == WM_RBUTTONDOWN || wParam == WM_RBUTTONUP) && PtInRect(&activeRect, pt)) {
                    if (!isMenuOpen) {
                        if (wParam == WM_RBUTTONUP) {
                            PostMessage(hwndGlobal, WM_USER_SHOW_MENU, (WPARAM)pt.x, (LPARAM)pt.y);
                        }
                        return 1; // Intercetta il clic (SIA PRESSIONE CHE RILASCIO)
                    }
                }
            }
        }
    }
    return CallNextHookEx(mouseHook, nCode, wParam, lParam);
}
