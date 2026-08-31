/*
 * License: MIT
 *
 * Author: Michael Kolorz
 *
 * File for measuring minimal RAM/Flash footprint for littlefs
 */

#include "init_conf.hpp"
#include "sdcardlittlefs.hpp"



int main()
{
    TIM2_Init();
    delay_ms<500>();
    // //debug_gdb_print("Hello Debug\n");
    GPIO_Init();

    constexpr int BUFSIZE = 32;
    lfs_sdinit(SdSlot::SD1);
    char lfswritebuf[BUFSIZE] = "Hello littlefs";
    char lfsreadbuf[BUFSIZE];

    lfs_t lfs_inst;
    lfs_file_t file;
    const lfs_config& cfg = lfs_sdconfig(SdSlot::SD1);


    int mount_res = lfs_mount(&lfs_inst, &cfg);
    if (mount_res != 0)
    {
        return 1; //mount failed
    }
    lfs_file_open(&lfs_inst, &file, "hellolittlefs.txt", LFS_O_RDWR | LFS_O_CREAT);

    lfs_file_write(&lfs_inst, &file, &lfswritebuf, sizeof(lfswritebuf));
    lfs_file_rewind(&lfs_inst, &file);
    lfs_file_read(&lfs_inst, &file, &lfsreadbuf, sizeof(lfsreadbuf));

    
    // remember the storage is not updated until the file is closed successfully
    lfs_file_close(&lfs_inst, &file);

    // release any resources we were using
    lfs_unmount(&lfs_inst);

    return 0;
}