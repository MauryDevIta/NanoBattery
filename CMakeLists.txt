cmake_minimum_required(VERSION 3.20)
project(battery_widget C)

set(CMAKE_C_STANDARD 99)

add_executable(battery_widget WIN32
        main.c
        globals.c
        updater.c
        battery.c
        hooks.c
        system_utils.c
)

target_link_libraries(battery_widget urlmon shell32 setupapi user32 gdi32 advapi32)
