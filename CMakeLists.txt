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
set(CMAKE_PROJECT_NAME STM32F407)


# Include toolchain file
include("cmake/gcc-arm-none-eabi.cmake")

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME}
        Bsp/KEY/bsp_Key.h)

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)
            
# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
        Bsp/UART/bsp_UART.c
        Bsp/OLED_0.96_4P/bsp_OLED.c
        Bsp/DHT11/dht11.c
        Bsp/KEY/bsp_Key.c
        Bsp/W25Q128/bsp_W25Q128.c
        Core/Src/app_led.c
        Core/Src/app_pir.c
        Core/Src/app_relay.c
        Core/Src/app_servo.c
        Core/Src/app_esp8266.c
        



)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
        Bsp/UART
        Bsp/DHT11
        Bsp/KEY
        Bsp/LED
        Bsp/OLED_0.96_4P
        Bsp/W25Q128

)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)

target_link_libraries(${CMAKE_PROJECT_NAME}
        stm32cubemx
        -lc  # C标准库（包含printf实现）
        -lm  # 数学库（可选，若用到浮点打印）
        -lnosys  # 避免嵌入式环境下的系统调用依赖
)
