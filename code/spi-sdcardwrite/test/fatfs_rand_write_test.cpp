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
#include "prng.hpp"

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
        constexpr char filename[] = "fat_wt3.txt";
        char big_buf[1024];

        FIL file;
        UINT written;
        uint32_t seednxt = 0;
        // Open existing file and position at the end.
        FRESULT result = f_open(&file,filename, FA_WRITE | FA_OPEN_APPEND);

        if (result == FR_OK)
        {   
            for(;;)
            {
                constexpr int u32bitsize = 4; 

                // Generate 1024 bytes of pseudorandom data
                for (int i = 0; i < 1024; i += u32bitsize)
                {
                    uint32_t random_value = xorrng32(0x01234567 + seednxt);
                    std::memcpy(big_buf + i, &random_value, u32bitsize);
                    seednxt++;
                }

                result = f_write(&file, big_buf, sizeof(big_buf), &written);

                if (result != FR_OK || written != sizeof(big_buf))
                {
                    write_errors++;
                }

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