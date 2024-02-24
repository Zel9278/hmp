cmake_minimum_required(VERSION 3.1)

project(hachimitsu-midi-player C)

include_directories(includes)

add_executable(hmp
    src/main.c
    src/midiplayer.c
)
target_link_libraries(hmp m)

