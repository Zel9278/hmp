cmake_minimum_required(VERSION 3.1)
project(hachimitsu-midi-player C)
add_executable(hmp main.c midiplayer.c)
target_link_libraries(hmp m)
# to vscode debugger
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_C_FLAGS_DEBUG "-g -O0")
