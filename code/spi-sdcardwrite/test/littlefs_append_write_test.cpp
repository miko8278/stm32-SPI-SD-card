 /*
 * License: MIT
 *
 * Author: Michael Kolorz
 *
 * Writetest for littlefs using a known and humanreadable byte-sequence
 * Each file has a size of 1 kB
 */

#include "init_conf.hpp"
#include "sdcardlittlefs.hpp"
#include <cstdio>
#include <cstring>


int main()
{
    TIM2_Init();
    delay_ms<500>();
    // //debug_gdb_print("Hello Debug\n");
    GPIO_Init();

    constexpr int BUFSIZE = 1024;
    lfs_sdinit(SdSlot::SD1);
    char lfswritebuf[BUFSIZE];

    lfs_t lfs_inst;
    lfs_file_t file;
    const lfs_config& cfg = lfs_sdconfig(SdSlot::SD1);


    int mount_res = lfs_mount(&lfs_inst, &cfg);
    if (mount_res != 0)
    {
        return 1; //mount failed
    }

    char filename[32];
    int file_number = 0;
    int cnt_number = 0;

    std::snprintf(filename, sizeof(filename), "lfs_append_test1.txt");

    char msg[32];


    int res_open = lfs_file_open(&lfs_inst, &file, filename, LFS_O_RDWR | LFS_O_CREAT);
    if(res_open != LFS_ERR_OK) 
    {
        return 0;
    }
    lfs_file_seek(&lfs_inst, &file, 0, LFS_SEEK_END);
    for(;;)
    {
        int tcnt_ms = TIM2->CNT/1000;
        int msg_len = std::snprintf(msg, sizeof(msg), "%07d\n", tcnt_ms);

        // Fill the complete 1024-byte buffer
        for (int j = 0; j < (1024 / msg_len); j++)
        {
            std::memcpy(lfswritebuf + j * msg_len, msg, msg_len);
        }
        
        lfs_file_write(&lfs_inst, &file, lfswritebuf, sizeof(lfswritebuf));

        //sync for now
        lfs_file_sync(&lfs_inst, &file);

    }
    // remember the storage is not updated until the file is closed successfully
    lfs_file_close(&lfs_inst, &file);
    // release any resources we were using
    lfs_unmount(&lfs_inst);

    return 0;
}