#pragma once

#include "lfs.h"


enum class SdSlot : uint8_t
{
    SD1,
    SD2,
};

enum LFSINIT_RES : uint8_t
{
    LFSINIT_OK = 0x00,
    LFSINIT_SPIINIT_FAIL = 0x01,
    LFSINIT_GETCSD_FAIL = 0x02,
    LFSINIT_NOT_INIT = 0xFE,
};

uint8_t lfs_sdinit(SdSlot slot);

const lfs_config& lfs_sdconfig(SdSlot slot);
