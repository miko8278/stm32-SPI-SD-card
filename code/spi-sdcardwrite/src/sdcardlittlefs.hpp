#pragma once

#include "lfs.h"




enum LFSINIT_RES : uint8_t
{
    LFSINIT_OK = 0x00,
    LFSINIT_SPIINIT_FAIL = 0x01,
    LFSINIT_GETCSD_FAIL = 0x02,
    LFSINIT_NOT_INIT = 0xFE,
};
uint8_t lfs_sdinit();

const lfs_config& lfs_sdconfig();
