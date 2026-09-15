#include "battery.h"
#include "widget_globals.h"
#include "system_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <initguid.h>
#include <setupapi.h>
#include <devguid.h>
#include <winioctl.h>

#define IOCTL_BATTERY_QUERY_TAG CTL_CODE(FILE_DEVICE_BATTERY, 0x10, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_BATTERY_QUERY_INFORMATION CTL_CODE(FILE_DEVICE_BATTERY, 0x11, METHOD_BUFFERED, FILE_READ_ACCESS)

typedef enum { BatteryInformation } BATTERY_QUERY_INFORMATION_LEVEL;

typedef struct _BATTERY_QUERY_INFORMATION {
    ULONG BatteryTag;
    BATTERY_QUERY_INFORMATION_LEVEL InformationLevel;
    ULONG AtRate;
} BATTERY_QUERY_INFORMATION;

typedef struct _BATTERY_INFORMATION {
    ULONG Capabilities;
    UCHAR Technology;
    UCHAR Reserved[3];
    UCHAR Chemistry[4];
    ULONG DesignedCapacity;
    ULONG FullChargedCapacity;
    ULONG DefaultAlert1;
    ULONG DefaultAlert2;
    ULONG CriticalBias;
    ULONG CycleCount;
} BATTERY_INFORMATION;

void FetchAdvancedBatteryStats(void) {
    HDEVINFO hdev = SetupDiGetClassDevs(&GUID_DEVCLASS_BATTERY, 0, 0, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (hdev == INVALID_HANDLE_VALUE) {
        snprintf(advancedBatteryText, sizeof(advancedBatteryText), "Dati batteria non disponibili");
        return;
    }

    SP_DEVICE_INTERFACE_DATA did = {0};
    did.cbSize = sizeof(did);

    int success = 0;
    if (SetupDiEnumDeviceInterfaces(hdev, 0, &GUID_DEVCLASS_BATTERY, 0, &did)) {
        DWORD cbRequired = 0;
        SetupDiGetDeviceInterfaceDetail(hdev, &did, 0, 0, &cbRequired, 0);
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER && cbRequired > 0) {
            PSP_DEVICE_INTERFACE_DETAIL_DATA pdidd = (PSP_DEVICE_INTERFACE_DETAIL_DATA)LocalAlloc(LPTR, cbRequired);
            if (pdidd) {
                pdidd->cbSize = sizeof(*pdidd);
                if (SetupDiGetDeviceInterfaceDetail(hdev, &did, pdidd, cbRequired, &cbRequired, 0)) {
                    HANDLE hBattery = CreateFile(pdidd->DevicePath, GENERIC_READ | GENERIC_WRITE,
                                                 FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                                 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                    if (hBattery != INVALID_HANDLE_VALUE) {
                        ULONG dwWait = 0;
                        ULONG batteryTag = 0;
                        ULONG dwOut = 0;
                        if (DeviceIoControl(hBattery, IOCTL_BATTERY_QUERY_TAG, &dwWait, sizeof(dwWait),
                                            &batteryTag, sizeof(batteryTag), &dwOut, NULL) && batteryTag) {
                            BATTERY_QUERY_INFORMATION bqi = {0};
                            bqi.BatteryTag = batteryTag;
                            bqi.InformationLevel = BatteryInformation;
                            BATTERY_INFORMATION bi = {0};
                            if (DeviceIoControl(hBattery, IOCTL_BATTERY_QUERY_INFORMATION, &bqi, sizeof(bqi),
                                                &bi, sizeof(bi), &dwOut, NULL)) {
                                int wear = 0;
                                if (bi.DesignedCapacity > 0) {
                                    wear = 100 - (int)((bi.FullChargedCapacity * 100ULL) / bi.DesignedCapacity);
                                    if (wear < 0) wear = 0;
                                    if (wear > 100) wear = 100;
                                }
                                int health = 100 - wear;

                                char cycleBuf[32];
                                if (bi.CycleCount == 0 || bi.CycleCount == (ULONG)-1) {
                                    snprintf(cycleBuf, sizeof(cycleBuf), "N/D");
                                } else {
                                    snprintf(cycleBuf, sizeof(cycleBuf), "%lu", bi.CycleCount);
                                }

                                snprintf(advancedBatteryText, sizeof(advancedBatteryText),
                                         "Salute: %d%% | Usura: %d%%\nCicli: %s\nCapacit\xC3\xA0: %lu / %lu mWh",
                                         health, wear, cycleBuf, bi.FullChargedCapacity, bi.DesignedCapacity);
                                success = 1;
                            }
                        }
                        CloseHandle(hBattery);
                    }
                }
                LocalFree(pdidd);
            }
        }
    }
    SetupDiDestroyDeviceInfoList(hdev);

    if (!success && advancedBatteryText[0] == '\0') {
        snprintf(advancedBatteryText, sizeof(advancedBatteryText), "Dati avanzati non disponibili");
    }
}

void UpdateBatteryText(HWND hwnd) {
    SYSTEM_POWER_STATUS sps;
    static int smoothedLifeTime = -1;
    static int lastPercent = -1;

    if (GetSystemPowerStatus(&sps)) {
        if (sps.BatteryLifePercent == 255) {
            isBatteryPresent = 0;
            batteryPercent = 0;
            isCharging = (sps.ACLineStatus == 1);
            snprintf(batteryText, sizeof(batteryText), "Nessuna batteria");
            smoothedLifeTime = -1;
        } else {
            isBatteryPresent = 1;
            int percent = (int)sps.BatteryLifePercent;
            if (percent > 100) percent = 100;
            batteryPercent = percent;
            isCharging = (sps.ACLineStatus == 1);

            if (lastPercent != -1 && alertsEnabled) {
                if (lastPercent < BATTERY_ALERT_HIGH && percent >= BATTERY_ALERT_HIGH && isCharging) {
                    ShowNotification(hwnd, "NanoBattery - Smart Alert",
                                     "La batteria ha raggiunto l'80%.\nPuoi scollegare l'alimentatore per preservarne la salute e ridurre l'usura.", 0);
                }
                if (lastPercent > BATTERY_ALERT_LOW && percent <= BATTERY_ALERT_LOW && !isCharging) {
                    ShowNotification(hwnd, "NanoBattery - Smart Alert",
                                     "La batteria \xC3\xA8 scesa al 20%.\n\xC3\x88 consigliabile collegare l'alimentatore.", 1);
                }
            }
            lastPercent = percent;

            if (isCharging) {
                snprintf(batteryText, sizeof(batteryText), "%d%% | In carica", percent);
                smoothedLifeTime = -1;
            } else if (sps.BatteryLifeTime == (DWORD)-1) {
                snprintf(batteryText, sizeof(batteryText), "%d%% | Calcolando...", percent);
            } else {
                int currentOsTime = (int)sps.BatteryLifeTime;
                if (smoothedLifeTime == -1 || abs(currentOsTime - smoothedLifeTime) > 1800) {
                    smoothedLifeTime = currentOsTime;
                } else {
                    smoothedLifeTime = (int)((currentOsTime * 0.1) + (smoothedLifeTime * 0.9));
                }
                int hours = smoothedLifeTime / 3600;
                int mins = (smoothedLifeTime % 3600) / 60;
                snprintf(batteryText, sizeof(batteryText), "%d%% | %dh %dm", percent, hours, mins);
            }
        }

        // Se non abbiamo ancora le statistiche avanzate o siamo in hover, le aggiorniamo
        if (isHovering || advancedBatteryText[0] == '\0') {
            FetchAdvancedBatteryStats();
        }

        InvalidateRect(hwnd, NULL, FALSE);
    }
}
