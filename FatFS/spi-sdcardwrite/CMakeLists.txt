cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Tell CMake not to run the OS-dependent executable test
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")

project(STM32-SDcardFatFS LANGUAGES C CXX ASM)


set(MCU_FLAGS 
    -mcpu=cortex-m4 
    -mthumb 
    -mfpu=fpv4-sp-d16 
    -mfloat-abi=hard
)

add_compile_options(
    ${MCU_FLAGS}
    -Wall -Wextra -O0 -g3                     
    -ffunction-sections -fdata-sections       
)

add_link_options(
    ${MCU_FLAGS}
    -specs=nano.specs                         # Uses lightweight C library
    -specs=nosys.specs                        # Provides dummy stubs for _exit, _sbrk, etc.
    -Wl,--gc-sections                         
)

# Set source code definitions and file properties
add_compile_definitions(STM32G431xx)
set_source_files_properties(startup_stm32g431xx.s PROPERTIES LANGUAGE ASM) 

set(BINARY_NAME SPI-SDCard)

# Target Executable
add_executable(${BINARY_NAME}
    main.cpp
    startup_stm32g431xx.s
    system_stm32g4xx.c
)

# Linker script
target_link_options(${BINARY_NAME} PRIVATE
    -T${CMAKE_SOURCE_DIR}/linker/STM32G431RBTX_FLASH.ld
)

target_include_directories(${BINARY_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Post-build commands 
add_custom_command(TARGET ${BINARY_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${BINARY_NAME}> ${CMAKE_BINARY_DIR}/${BINARY_NAME}.elf
)

add_custom_command(TARGET ${BINARY_NAME} POST_BUILD
    COMMAND arm-none-eabi-objcopy -O ihex $<TARGET_FILE:${BINARY_NAME}> ${CMAKE_BINARY_DIR}/${BINARY_NAME}.hex
)

add_custom_command(TARGET ${BINARY_NAME} POST_BUILD
    COMMAND arm-none-eabi-size $<TARGET_FILE:${BINARY_NAME}>
)