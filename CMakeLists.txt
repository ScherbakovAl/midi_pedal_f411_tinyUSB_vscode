cmake_minimum_required(VERSION 3.28)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Include toolchain file for ARM
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/gcc-arm-none-eabi.cmake) #нужно для tinyUSB

# Include Rust configuration
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/rust.cmake)

# Setup compiler settings
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# =============================================================================
# Compiler Optimizations - Applied to ALL languages (C, C++, Rust)
# =============================================================================
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    # Maximum optimization for Release
    set(CMAKE_C_FLAGS_RELEASE "-Ofast -fno-stack-protector -ffunction-sections -fdata-sections")
    set(CMAKE_CXX_FLAGS_RELEASE "-Ofast -fno-stack-protector -ffunction-sections -fdata-sections")
    
    # Link-time optimization for whole program optimization
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -flto -Wl,--gc-sections")
    
elseif(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
    # Optimize for size
    set(CMAKE_C_FLAGS_MINSIZEREL "-Os -fno-stack-protector -ffunction-sections -fdata-sections")
    set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -fno-stack-protector -ffunction-sections -fdata-sections")
    set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL} -Wl,--gc-sections")
    
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    # Optimize but keep debug info
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g -fno-stack-protector -ffunction-sections -fdata-sections")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -fno-stack-protector -ffunction-sections -fdata-sections")
    set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} -flto -Wl,--gc-sections")
    
else()
    # Debug build - minimal optimizations for debugging
    set(CMAKE_C_FLAGS_DEBUG "-Og -g -fno-omit-frame-pointer")
    set(CMAKE_CXX_FLAGS_DEBUG "-Og -g -fno-omit-frame-pointer")
endif()

message(" ")
message("=== Compiler Optimization Settings ===")
message("Build type: ${CMAKE_BUILD_TYPE}")
message("C Flags: ${CMAKE_C_FLAGS_${CMAKE_BUILD_TYPE}}")
message("CXX Flags: ${CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}}")
message("Linker Flags: ${CMAKE_EXE_LINKER_FLAGS_${CMAKE_BUILD_TYPE}}")
message("=======================================")
message(" ")

# Set the project name
set(CMAKE_PROJECT_NAME midi_pedal_f411_tinyUSB_vscode)


# tinyUSB
include("tinyusb/src/CMakeLists.txt")


# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})

# Enable CMake support for ASM and C languages
enable_language(C CXX ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# =============================================================================
# Build Rust code (must be after add_executable)
# =============================================================================
if(RUST_AVAILABLE)
    message(STATUS "Building Rust code...")
    add_rust_file(${CMAKE_SOURCE_DIR}/Pedal_f411/logic_rust.rs)
else()
    message(WARNING "Rust not available - skipping Rust code compilation")
    message(WARNING "To enable Rust, install it from https://rustup.rs/")
endif()

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    stm32cubemx
    # Add user defined libraries
)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# tinyUSB
tinyusb_target_add(${CMAKE_PROJECT_NAME})

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    Pedal_f411/pedal.cpp
    Pedal_f411/power.cpp
    Pedal_f411/board_api.c
    # tinyUSB
    Pedal_f411/usb_descriptors.c
    tinyusb/src/tusb.c
    tinyusb/src/class/midi/midi_device.c
    tinyusb/src/portable/synopsys/dwc2/dcd_dwc2.c
    tinyusb/src/portable/synopsys/dwc2/dwc2_common.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    ${CMAKE_CURRENT_SOURCE_DIR}/Pedal_f411
    ${CMAKE_CURRENT_SOURCE_DIR}/tinyusb/src
    ${CMAKE_CURRENT_SOURCE_DIR}/tinyusb/hw/bsp
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
    CFG_TUSB_MCU=OPT_MCU_STM32F4
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

message(" ")
message("Build type: > > > >>>>>>>>>>>>> > > >  >  >   " ${CMAKE_BUILD_TYPE} "   <  <  < < < <<<<<<<<<<<<<<<<< < < < <")
message(" ")
