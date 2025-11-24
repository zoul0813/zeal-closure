cmake_minimum_required(VERSION 3.16)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(ZOS_TOOLCHAIN sdcc)
include($ENV{ZOS_PATH}/cmake/zos_init.cmake)

zos_use(zvb)
find_package(ZVB_SDK REQUIRED)
zos_use(zgdk)
find_package(ZGDK REQUIRED)


project(closure C ASM)

add_executable(closure
    src/closure.c
)

# target_link_libraries(closure PUBLIC zvb_gfx)
target_include_directories(closure PRIVATE $ENV{ZVB_SDK_PATH}/include)
target_link_libraries(closure PUBLIC zgdk)

target_compile_options(closure PRIVATE
    $<$<COMPILE_LANGUAGE:C>:--nostdlib>
)

zos_add_outputs(closure)
