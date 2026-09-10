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
#include "prng.hpp"

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
    int cnt = 0;
    std::snprintf(filename, sizeof(filename), "lfs_rand_test1.txt");

    uint32_t seednxt = 0;

    int res_open = lfs_file_open(&lfs_inst, &file, filename, LFS_O_RDWR | LFS_O_CREAT);
    if(res_open != LFS_ERR_OK) 
    {
        return 0;
    }
    lfs_file_seek(&lfs_inst, &file, 0, LFS_SEEK_END);
    for(;;)
    {

        constexpr int u32bitsize = 4; 
        // Generate 1024 bytes of pseudorandom data
        for (int i = 0; i < 1024; i += u32bitsize)
        {
            uint32_t random_value = xorrng32(0x01234567 + seednxt);
            std::memcpy(lfswritebuf + i, &random_value, u32bitsize);
            seednxt++;
        }


        lfs_file_write(&lfs_inst, &file, lfswritebuf, sizeof(lfswritebuf));

        //sync every 200. write for now
        if(cnt == 200)
        {
            lfs_file_sync(&lfs_inst, &file);
            cnt = 0;
        }
        cnt++;
    }
    // remember the storage is not updated until the file is closed successfully
    lfs_file_close(&lfs_inst, &file);
    // release any resources we were using
    lfs_unmount(&lfs_inst);

    return 0;
}