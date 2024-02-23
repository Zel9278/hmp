cmake_minimum_required(VERSION 3.1)
project(hachimitsu-midi-player C)
add_executable(hmp main.c)
target_link_libraries(hmp m)
