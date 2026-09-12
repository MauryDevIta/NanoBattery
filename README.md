# 🔋 NanoBattery

![C](https://img.shields.io/badge/c-%2300599C.svg?style=for-the-badge&logo=c&logoColor=white)
![Windows 11](https://img.shields.io/badge/Windows%2011-%230078D6.svg?style=for-the-badge&logo=windows-11&logoColor=white)
![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg?style=for-the-badge)
![Size](https://img.shields.io/badge/Size-<100KB-brightgreen.svg?style=for-the-badge)

**NanoBattery** is an ultra-lightweight, zero-CPU battery widget for Windows 11, written entirely in pure C using the native Win32 API.

Designed to bypass the limitations of the modern Windows 11 XAML taskbar and the Desktop Window Manager (DWM), NanoBattery provides a clean, unobtrusive battery readout that stubbornly survives system gestures like the Task View swipe.

## ✨ Features

- ⚡ **Zero-CPU Footprint:** Operates on a highly optimized Win32 event loop. It sleeps completely when idle, updating only when necessary.
- 🛡️ **Task View Survivor:** Dynamically reinforces its `HWND_TOPMOST` priority to survive Windows 11 DWM animations and virtual desktop swipes.
- 🖱️ **Interactive Hitbox:** Uses a global low-level mouse hook (`WH_MOUSE_LL`) to bypass Chroma-Key transparency limitations. **Left-click** opens Windows Power Settings, **Right-click** gracefully kills the widget.
- 🔄 **Silent Auto-Updater:** Checks GitHub for updates on boot and updates itself seamlessly in the background using a temporary relay script.
- 🚀 **Auto-Start:** Automatically registers itself in the Windows Registry (`HKCU\...\Run`) for a seamless, zero-touch boot experience.
- 🪶 **Bare Metal:** No Electron, no .NET runtime, no bloated dependencies. Just pure C code compiling to a tiny executable.
- 👻 **Ghost Mode:** Utilizes layered transparency to blend seamlessly into the UI without stealing window focus.

## ⚠️ Antivirus False Positives (Important!)
Because NanoBattery is a highly optimized, unsigned executable that uses low-level system techniques to function seamlessly, proactive antiviruses (like **Kaspersky**) might flag it as a generic threat (e.g., `PDM:Trojan.Win32.Generic`).

**Why does this happen?**
The antivirus heuristic engine gets suspicious because the widget:
1. Runs silently in the background without a standard window/console.
2. Intercepts mouse clicks system-wide using `WH_MOUSE_LL` (to make the transparent background clickable).
3. Connects to the internet to check for updates (`urlmon.h`).
4. Modifies the Windows Registry to enable auto-start on boot.

**Solution:** This is a **False Positive**. The code is 100% open-source and safe. Please add the `BatteryWidget.exe` file (or its installation folder) to your **antivirus exclusions/whitelist**. You are strongly encouraged to read the source code and compile it yourself!

## 🛠️ The Technical Problem (And How It's Solved)
Windows 11 aggressively hides standard floating windows during its "Task View" swipe animations, and making a window 100% transparent (`RGB(0,0,0)` Chroma Key) usually makes it unclickable.
NanoBattery solves this elegantly at the OS level: it hooks into system timers to lock itself on screen, and uses a mathematical Global Mouse Hook to create an invisible, pixel-perfect "hitbox" around the text, ensuring you can always click it without hitting the taskbar underneath.

## 🚀 Building from Source

You can build NanoBattery using either CMake or a raw GCC command.

### Option A: CMake (Recommended)
1. Clone the repository:
   ```bash
   git clone https://github.com/MauryDevIta/NanoBattery.git
   cd NanoBattery
   ```
2. Generate build files and compile:
   ```bash
   mkdir build && cd build
   cmake ..
   cmake --build .
   ```

### Option B: Raw GCC (MinGW)
If you prefer the command line, ensure you link the required system and networking libraries:
```bash
gcc main.c -o BatteryWidget.exe -mwindows -lshell32 -lurlmon
```

## 🎯 Usage
Simply double-click `BatteryWidget.exe`. The widget will silently anchor itself to the bottom-left corner of your screen (automatically calculating your taskbar height) and will start with Windows automatically.
- **Left-Click the text:** Opens Windows 11 Power & Battery Settings.
- **Right-Click the text:** Safely terminates the widget and clears it from memory.

## 🤝 Contributing
Contributions, issues, and feature requests are welcome! Feel free to check the [issues page](https://github.com/MauryDevIta/NanoBattery/issues).

## 📜 License
This project is open-source and licensed under the [MIT License](LICENSE).