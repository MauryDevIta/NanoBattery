cmake_minimum_required(VERSION 3.20)
project(battery_widget C)

set(CMAKE_C_STANDARD 11)

# Il flag WIN32 equivale a -mwindows (evita l'apertura della console nera)
add_executable(battery_widget WIN32 main.c)

# Collega le librerie di sistema necessarie per ShellExecute e URLDownloadToFile
target_link_libraries(battery_widget PRIVATE
        urlmon
        shell32
)