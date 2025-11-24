cmake_minimum_required(VERSION 3.16)

# -------------------------------
# Toolchain
# -------------------------------
set(ZOS_TOOLCHAIN sdcc)
include($ENV{ZOS_PATH}/cmake/zos_init.cmake)

# -------------------------------
# Project
# -------------------------------
project(closure C ASM)

# -------------------------------
# Executable
# -------------------------------
add_executable(closure
    src/closure.c
)

# # -------------------------------
# # Include and link SDKs
# # -------------------------------
zos_use(zgdk)
zos_use(zvb)

find_package(ZVB_SDK REQUIRED)
target_link_libraries(closure PUBLIC zvb_gfx zvb_sound)

find_package(ZGDK REQUIRED)
target_link_libraries(closure PUBLIC zgdk)

# -------------------------------
# Convert to binary
# -------------------------------
zos_add_outputs(closure)
