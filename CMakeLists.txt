cmake_minimum_required(VERSION 3.13)

set(PICO_SDK_PATH $ENV{HOME}/work/pico-sdk)
include(pico_sdk_import.cmake)

project(weact_epd C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_library(weact_epd STATIC
    src/epd.c
    src/gfx.c
    src/font8x16.c
)

target_include_directories(weact_epd PUBLIC src)

target_link_libraries(weact_epd
    pico_stdlib
    hardware_spi
    hardware_dma
    hardware_gpio
)

add_subdirectory(examples/demo)
add_subdirectory(examples/tetris)
add_subdirectory(examples/rot)
