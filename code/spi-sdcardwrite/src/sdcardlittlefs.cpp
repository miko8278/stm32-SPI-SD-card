#include "sdcardlittlefs.hpp"

#include "stm32g431xx.h"
#include "spidriver.hpp"
#include "sdcarddriver.hpp"
#include "GPIO_HAL.hpp"
#include "init_conf.hpp"

#include <cstdint>
#include <cstring>

namespace {

constexpr uint32_t SD_SECTOR_SIZE = 512;

// LittleFS block-device callbacks
int lfs_read(const struct lfs_config*, lfs_block_t block, lfs_off_t off, void* buffer, lfs_size_t size)
{
    if (off + size > SD_SECTOR_SIZE)
        return LFS_ERR_INVAL;

    uint8_t sector[SD_SECTOR_SIZE];

    if (SD_ReadBlock<SD1_Config>(block, sector) != SD_READ_OK)
        return LFS_ERR_IO;

    uint8_t* dst = static_cast<uint8_t*>(buffer);

    for (lfs_size_t i = 0; i < size; ++i)
    {
        dst[i] = sector[off + i];
    }

    return 0;
}

int lfs_prog(const struct lfs_config*, lfs_block_t block, lfs_off_t off, const void* buffer, lfs_size_t size)
{
    if (off + size > SD_SECTOR_SIZE)
        return LFS_ERR_INVAL;

    uint8_t sector[SD_SECTOR_SIZE];
    const uint8_t* src = static_cast<const uint8_t*>(buffer);
    if(off != 0 || size != SD_SECTOR_SIZE){
        if (SD_ReadBlock<SD1_Config>(block, sector) != SD_READ_OK)
            return LFS_ERR_IO;
    }
    for (lfs_size_t i = 0; i < size; ++i)
    {
        sector[off + i] = src[i];
    }
    if (SD_WriteBlock<SD1_Config>(block, sector) != SD_WRITE_OK)
        return LFS_ERR_IO;

    return 0;
}

int lfs_erase(const struct lfs_config*, lfs_block_t)
{
    // SD cards handle flash erasure internally.
    return 0;
}

int lfs_sync(const struct lfs_config*)
{
    //write also syncs...
    return 0;
}


}

lfs_config lfsconfig{
    .read = lfs_read,
    .prog = lfs_prog,
    .erase = lfs_erase,
    .sync = lfs_sync,

    .read_size = SD_SECTOR_SIZE,
    .prog_size = SD_SECTOR_SIZE,

    .block_size = SD_SECTOR_SIZE,
    .block_count = 0,
    .block_cycles = 500,
    
    .cache_size = SD_SECTOR_SIZE,
    .lookahead_size = 16,
};

uint8_t lfs_sdinit()
{
    GpioPin<SD1_Config::PortBase, SD1_Config::Pin>::OutputInit(Level::High);
    SpiDriver<SD1_Config::SpiBase>::Init();

    if (SD_InitSPI<SD1_Config>() != SD_INIT_OK)
        return LFSINIT_SPIINIT_FAIL;

    uint8_t buffer[16];
    Csd_Common csd;

    if (SD_GetCSD<SD1_Config>(buffer, 16) != SD_CSD_OK)
        return LFSINIT_GETCSD_FAIL;

    parse_csd(buffer, &csd);

    lfsconfig.block_count = csd.capacity / SD_SECTOR_SIZE;

    return LFSINIT_OK;
}


const lfs_config& lfs_sdconfig()
{
    return lfsconfig;
}

