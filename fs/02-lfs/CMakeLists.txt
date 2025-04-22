# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD 16bit)
set(PICO_BOARD_HEADER_DIRS ${CMAKE_CURRENT_LIST_DIR}/boards)

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(02-lfs C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(02-lfs main.c)

pico_set_program_name(02-lfs "02-lfs")
pico_set_program_version(02-lfs "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(02-lfs 1)
pico_enable_stdio_usb(02-lfs 0)

# Add LittleFS library
add_library(littlefs STATIC
    ${CMAKE_CURRENT_LIST_DIR}/lib/littlefs/lfs.c
    ${CMAKE_CURRENT_LIST_DIR}/lib/littlefs/lfs_util.c
)

# Add the standard library to the build
target_link_libraries(02-lfs
        pico_stdlib
        hardware_flash
        littlefs
        )

# Add the standard include files to the build
target_include_directories(02-lfs PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/lib/littlefs
)

pico_add_extra_outputs(02-lfs)

