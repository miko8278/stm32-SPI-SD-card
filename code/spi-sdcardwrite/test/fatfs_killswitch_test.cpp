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
        uint8_t write_number = 0;
        // Find first filename that does not exist
        for (;;)
        {
            std::snprintf(filename, sizeof(filename), "test%04d.txt", file_number);

            FILINFO fno;
            FRESULT result = f_stat(filename, &fno);

            if (result == FR_NO_FILE)
            {
                // This is the first filename that doesnt exist
                break;
            }

            if (result != FR_OK)
            {
                // Some other filesystem error, doesn't matter I guess
                file_number++;
                continue;
            }

            file_number++;
        }

        for(;;)
        {
            //Filename was changed in later versions to include timestamps just like the littlefs version 
            FRESULT flg_open = f_open(&file, filename, FA_WRITE | FA_CREATE_ALWAYS);
            if (flg_open == FR_OK)
            {   
                for(;;)
                {
                    char msg[32];
                    int msg_len = std::snprintf(msg, sizeof(msg), "Write %08d!\n", file_number);
                    // Fill the complete 1024-byte buffer
                    for (int j = 0; j < (1024 / msg_len); j++)
                    {
                        std::memcpy(big_buf + j * msg_len, msg, msg_len);
                    }
                    flg_open = f_write(&file,big_buf,sizeof(big_buf),&written);

                    if (flg_open != FR_OK || written != sizeof(big_buf))
                    {
                        write_errors++;
                    }

                    //I'm not sure if sync is really needed, works without, 
                    //but lets do it for some extra safety here
                    if ((write_number % 125) == 0)
                    {
                        f_sync(&file);
                        write_number = 0;
                    }
                    write_number++;
                }
                f_close(&file);
            }
        }
    }

    return 0;
}