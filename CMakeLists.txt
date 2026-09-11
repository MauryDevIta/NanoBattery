cmake_minimum_required(VERSION 3.20)
project(BatteryWidget C)

set(CMAKE_C_STANDARD 11)

# Il flag WIN32 nasconde la console, target_link_libraries aggiunge gdi32
add_executable(battery_widget WIN32 main.c)
target_link_libraries(battery_widget gdi32)