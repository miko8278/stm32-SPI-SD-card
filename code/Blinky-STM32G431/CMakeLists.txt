cmake_minimum_required(VERSION 3.16)

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

# Target Executable
add_executable(blinky
    main.c
    startup_stm32g431xx.s
    system_stm32g4xx.c
)

# Linker script
target_link_options(blinky PRIVATE
    -T${CMAKE_SOURCE_DIR}/linker/STM32G431RBTX_FLASH.ld
)

target_include_directories(blinky PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Post-build commands 
add_custom_command(TARGET blinky POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:blinky> ${CMAKE_BINARY_DIR}/blinky.elf
)

add_custom_command(TARGET blinky POST_BUILD
    COMMAND arm-none-eabi-objcopy -O ihex $<TARGET_FILE:blinky> ${CMAKE_BINARY_DIR}/blinky.hex
)

add_custom_command(TARGET blinky POST_BUILD
    COMMAND arm-none-eabi-size $<TARGET_FILE:blinky>
)