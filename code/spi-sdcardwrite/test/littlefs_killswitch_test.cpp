/*
 * License: MIT
 *
 * Author: Michael Kolorz
 *
 * Writetest for littlefs using a known and humanreadable byte-sequence
 * It is externally being shut off from power periodically
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

    //find the next available filename
    for (;;)
    {
        std::snprintf(filename, sizeof(filename),
                    "test%04d.txt", file_number);

        struct lfs_info info;
        int result = lfs_stat(&lfs_inst, filename, &info);

        if (result == LFS_ERR_NOENT)
        {
            // This filename does not exist
            break;
        }

        if (result < 0)
        {
            // Some other filesystem error
            file_number++;
            continue;
        }

        // File/directory exists
        file_number++;
    }

    //create the 1024byte buffer
    char msg[32];
    int msg_len = std::snprintf(msg, sizeof(msg), "Write %08d!\n", file_number);
    for (int j = 0; j < 64; j++)
    {
        std::memcpy(lfswritebuf + j * msg_len, msg, msg_len);
    }

    int res_open = lfs_file_open(&lfs_inst, &file, filename, LFS_O_RDWR | LFS_O_CREAT);
    if(res_open != LFS_ERR_OK) 
    {
        return 0;
    }
    lfs_file_seek(&lfs_inst, &file, 0, LFS_SEEK_END);
    
    //write in a loop till killswitch
    for(;;)
    {
        
        lfs_file_write(&lfs_inst, &file, lfswritebuf, sizeof(lfswritebuf));
        //sync for now
        lfs_file_sync(&lfs_inst, &file);
    }
    
    // shouldn't reach
    lfs_unmount(&lfs_inst);

    return 0;
}