/*
 * License: MIT
 *
 * Author: Michael Kolorz
 *
 * Writetest for FatFs using a known and humanreadable byte-sequence
 * Each file has a size of 1 kB
 */

#include "init_conf.hpp"
#include "ff.h"
#include <cstdio>
#include <cstring>

int write_errors = 0;

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

    //Ok system mounted
    if(flg_mnt == FR_OK){
        FIL file;
        UINT written;
        char filename[32];
        char big_buf[1024];
        int file_number = 0;
        int cnt_number = 0;
        for(;;)
        {
            //Filename was changed in later versions to include timestamps just like the littlefs version 
            int tcnt_ms = TIM2->CNT/1000;
            std::snprintf(filename, sizeof(filename), "l_%06d_%06d_%08d.txt", file_number, cnt_number, tcnt_ms);
            FRESULT flg_open = f_open(&file, filename, FA_WRITE | FA_CREATE_ALWAYS);
            if (flg_open == FR_OK)
            {
                //const char msg[] = "Writetest new!\n";
                //The following creates the 1024 Byte message we write
                char msg[32];
                int msg_len = std::snprintf(msg, sizeof(msg), "Write %08d!\n", file_number);
                for (int j = 0; j < 64; j++)
                {
                    std::memcpy(big_buf + j * msg_len, msg, msg_len);
                }


                FRESULT result = f_write(&file, big_buf, sizeof(big_buf), &written);
                if (result != FR_OK || written != sizeof(big_buf))
                {
                    write_errors++;                   
                }
                f_close(&file);
                file_number++;
            }
            cnt_number++;
        }
    }

    return 0;
}