# 🔋 NanoBattery

![C](https://img.shields.io/badge/c-%2300599C.svg?style=for-the-badge&logo=c&logoColor=white)
![Windows 11](https://img.shields.io/badge/Windows%2011-%230078D6.svg?style=for-the-badge&logo=windows-11&logoColor=white)
![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg?style=for-the-badge)
![Size](https://img.shields.io/badge/Size-<100KB-brightgreen.svg?style=for-the-badge)

**NanoBattery** is an ultra-lightweight, zero-CPU battery widget for Windows 11, written entirely in pure C using the native Win32 API.

Designed to bypass the limitations of the modern Windows 11 XAML taskbar and the Desktop Window Manager (DWM), NanoBattery provides a clean, unobtrusive battery readout that stubbornly survives system gestures like the Task View swipe.

## ✨ Features

- ⚡ **Zero-CPU Footprint:** Operates on a highly optimized Win32 event loop. It sleeps completely when idle, updating only when necessary.
- 🛡️ **Task View Survivor:** Uses smart adaptive polling and hooks into `WM_WINDOWPOSCHANGING` to dynamically reinforce its `HWND_TOPMOST` priority during Windows 11 DWM animations and virtual desktop swipes.
- 🪶 **Bare Metal:** No Electron, no .NET runtime, no bloated dependencies. Just pure C code compiling to a tiny executable.
- 💻 **Laptop Optimized:** Ideal for portable workstations and dual-boot systems. Extensively tested to respect deep sleep states (ACPI) on hardware like Lenovo ThinkPads without keeping the CPU awake.
- 👻 **Ghost Mode:** Utilizes `WS_EX_NOACTIVATE` and layered transparency to blend seamlessly into the UI without stealing window focus.

## 🛠️ The Technical Problem (And How It's Solved)
Windows 11 aggressively hides standard floating windows during its "Task View" swipe animations. Traditional widgets either disappear entirely or require expensive, heavy workarounds.
NanoBattery solves this elegantly at the OS level: instead of running a high-frequency (and battery-draining) render loop, it listens for the exact millisecond the OS attempts to recalculate window positions, locking itself on screen only when actively challenged by the DWM.

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
If you prefer the command line, simply run:
```bash
gcc main.c -o battery_widget.exe -mwindows -lgdi32
```

## 🎯 Usage
Simply double-click `battery_widget.exe`. The widget will silently anchor itself to the bottom-left corner of your screen.
- **To close:** Right-click the widget and select "Chiudi Widget".

## 🤝 Contributing
Contributions, issues, and feature requests are welcome! Feel free to check the [issues page](https://github.com/MauryDevIta/NanoBattery/issues).

## 📜 License
This project is open-source and licensed under the [MIT License](LICENSE).