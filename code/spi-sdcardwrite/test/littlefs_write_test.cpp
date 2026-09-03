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
    for(;;)
    {
        int tcnt_ms = TIM2->CNT/1000;
        std::snprintf(filename, sizeof(filename), "l_%06d_%06d_%08d.txt", file_number, cnt_number, tcnt_ms);

        char msg[32];
        int msg_len = std::snprintf(msg, sizeof(msg), "Write %08d!\n", file_number);
        for (int j = 0; j < 64; j++)
        {
            std::memcpy(lfswritebuf + j * msg_len, msg, msg_len);
        }

        int res_open = lfs_file_open(&lfs_inst, &file, filename, LFS_O_RDWR | LFS_O_CREAT);
        if(res_open == LFS_ERR_OK)
        {
            lfs_file_write(&lfs_inst, &file, &lfswritebuf, sizeof(lfswritebuf));
            // remember the storage is not updated until the file is closed successfully
            lfs_file_close(&lfs_inst, &file);
            file_number++;
        }
        cnt_number++;
    }
    // release any resources we were using
    lfs_unmount(&lfs_inst);

    return 0;
}