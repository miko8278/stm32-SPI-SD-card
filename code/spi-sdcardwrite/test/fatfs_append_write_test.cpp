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
int write_number = 0;

int main()
{
    TIM2_Init();
    delay_ms<500>();

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
    while (flg_mnt != FR_OK && ((TIM2->CNT - start_time) < T_OUT1_US));

    if (flg_mnt == FR_OK)
    {
        constexpr char filename[] = "fatfs_a2.txt";
        char big_buf[1024];


        // Create the 1024-byte message
        char msg[32];

        FIL file;
        UINT written;

        // Open existing file and position at the end.
        FRESULT result = f_open(&file,filename, FA_WRITE | FA_OPEN_APPEND);

        if (result == FR_OK)
        {   
            for(;;)
            {
                int tcnt_ms = TIM2->CNT/1000;
                int msg_len = std::snprintf(msg, sizeof(msg), "%07d\n", tcnt_ms);

                // Fill the complete 1024-byte buffer
                for (int j = 0; j < (1024 / msg_len); j++)
                {
                    std::memcpy(big_buf + j * msg_len, msg, msg_len);
                }
                result = f_write(&file,big_buf,sizeof(big_buf),&written);

                if (result != FR_OK || written != sizeof(big_buf))
                {
                    write_errors++;
                }

                //I'm not sure if sync is really needed, works without
                if ((write_number % 125) == 0)
                {
                    f_sync(&file);
                    write_number = 0;
                }
                write_number++;
            }
            f_close(&file);
        }
        else
        {
            write_errors++;
        }

        write_number++;
        
    }

    return 0;
}