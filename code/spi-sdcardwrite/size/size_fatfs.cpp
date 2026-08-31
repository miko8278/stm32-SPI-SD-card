/*
 * License: MIT
 *
 * Author: Michael Kolorz
 *
 * File for measuring minimal RAM/Flash footprint for FatFs
 */

#include "init_conf.hpp"
// #include "sdcarddriver.hpp"
#include "ff.h"



int main()
{
    TIM2_Init();
    delay_ms<500>();
    // //debug_gdb_print("Hello Debug\n");
    GPIO_Init();

    FATFS fs;
    FRESULT flg_mnt;
    constexpr uint32_t T_OUT1_US = 2 * 1000 * 1000; 
    uint32_t start_time = TIM2->CNT;
    do 
    {
        flg_mnt = f_mount(&fs, "", 1);
        delay_ms<500>();
    }    
    while(flg_mnt != FR_OK && ((TIM2->CNT - start_time) < T_OUT1_US));
    if(flg_mnt == FR_OK){
        FIL file;
        UINT written;   

        FRESULT flg_open = f_open(&file, "sizetest.txt", FA_WRITE | FA_CREATE_ALWAYS);

        if (flg_open == FR_OK)
        {
            const char msg[] = "Hello SD!\n";

            f_write(&file, msg, sizeof(msg) - 1, &written);
            f_close(&file);
        }
    }

    return 0;
}