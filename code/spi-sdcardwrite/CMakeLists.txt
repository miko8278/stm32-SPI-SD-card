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
    -Wall -Wextra -Os -g3                     
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


function(add_stm32_executable TARGET)

    add_executable(${TARGET}
        ${ARGN}
        startup_stm32g431xx.s
        system_stm32g4xx.c
    )

    target_link_options(${TARGET} PRIVATE
        -T${CMAKE_SOURCE_DIR}/linker/STM32G431RBTX_FLASH.ld
    )

    target_include_directories(${TARGET} PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/littlefs
        ${CMAKE_SOURCE_DIR}/FatFs
    )

    add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMAND arm-none-eabi-objcopy
            -O ihex
            $<TARGET_FILE:${TARGET}>
            ${CMAKE_BINARY_DIR}/${TARGET}.hex

        COMMAND arm-none-eabi-size
            $<TARGET_FILE:${TARGET}>
    )

endfunction()

##Basic sd-card functionality test
add_stm32_executable(Test-SPI-SDCard
    test/sd_test.cpp
    FatFs/ff.c
    FatFs/ffunicode.c
    src/diskio.cpp
    src/fatfs_time.cpp
    src/sdcardlittlefs.cpp
    littlefs/lfs.c
    littlefs/lfs_util.c
)

##Tests for binary size
add_stm32_executable(Size-FatFs
    size/size_fatfs.cpp
    FatFs/ff.c
    FatFs/ffunicode.c
    src/diskio.cpp
    src/fatfs_time.cpp
)

add_stm32_executable(Size-littlefs
    size/size_littlefs.cpp
    src/sdcardlittlefs.cpp
    littlefs/lfs.c
    littlefs/lfs_util.c
)

add_stm32_executable(Size-nofs
    size/size_nofs.cpp
)


#Long time write tests
add_stm32_executable(fatfs-write-test
    test/fatfs_write_test.cpp
    FatFs/ff.c
    FatFs/ffunicode.c
    src/diskio.cpp
    src/fatfs_time.cpp
)


add_stm32_executable(littlefs-write-test
    test/littlefs_write_test.cpp
    src/sdcardlittlefs.cpp
    littlefs/lfs.c
    littlefs/lfs_util.c
)