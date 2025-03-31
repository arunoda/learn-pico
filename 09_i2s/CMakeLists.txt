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
set(PICO_BOARD pico2 CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(09_i2s C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(09_i2s main.c )

pico_set_program_name(09_i2s "09_i2s")
pico_set_program_version(09_i2s "0.1")

# Generate PIO header
pico_generate_pio_header(09_i2s ${CMAKE_CURRENT_LIST_DIR}/blink.pio)
pico_generate_pio_header(09_i2s ${CMAKE_CURRENT_LIST_DIR}/i2s.pio)

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(09_i2s 0)
pico_enable_stdio_usb(09_i2s 1)

# Add the standard library to the build
target_link_libraries(09_i2s
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(09_i2s PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

# Add any user requested libraries
target_link_libraries(09_i2s 
        hardware_pio
        hardware_dma
        hardware_irq
        pico_stdlib
        pico_malloc
        m
        )

pico_add_extra_outputs(09_i2s)

