cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME dj)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME}
)

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    Core/Src/mux.c
    Core/Src/debug_uart.c
    Core/Src/app_tasks.c
    Core/Src/analog.c
    Core/Src/analog_filter.c
    Core/Src/debounce.c
    Core/Src/buttons.c
    Core/Src/usb_midi.c
    Core/Src/usb_descriptors.c
)

# TinyUSB (git submodule lib/tinyusb): device core, MIDI class, STM32 FS driver.
# Configured by Core/Inc/tusb_config.h.
set(TINYUSB_DIR ${CMAKE_SOURCE_DIR}/lib/tinyusb/src)
set(TINYUSB_SOURCES
    ${TINYUSB_DIR}/tusb.c
    ${TINYUSB_DIR}/common/tusb_fifo.c
    ${TINYUSB_DIR}/device/usbd.c
    ${TINYUSB_DIR}/class/midi/midi_device.c
    ${TINYUSB_DIR}/portable/st/stm32_fsdev/dcd_stm32_fsdev.c
    ${TINYUSB_DIR}/portable/st/stm32_fsdev/fsdev_common.c
)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${TINYUSB_SOURCES})

# Always optimise the library for size, also in Debug builds: at -O0 it alone
# takes ~32 KB of the 64 KB flash. Our own code keeps the build type's flags,
# so it stays easy to step through in the debugger.
set_source_files_properties(${TINYUSB_SOURCES} PROPERTIES COMPILE_OPTIONS "-Os")

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    ${TINYUSB_DIR}
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)
