/*
 * License: MIT
 *
 * Author: Michael Kolorz
 *
 */

#include "stm32g431xx.h"
#include "spidriver.hpp"
#include "sdcarddriver.hpp"
#include "GPIO_HAL.hpp"
#include "init_conf.hpp"

#include "ff.h"			/* Basic definitions of FatFs */
#include "diskio.h"		/* Declarations FatFs MAI */

extern "C" {


DSTATUS disk_initialize(BYTE pdrv)
{
    if (pdrv != 0)
        return STA_NOINIT;

    // CS pin as output, inactive state
    GpioPin<SD1_Config::PortBase, SD1_Config::Pin>::OutputHighInit();

    // Initialize SPI peripheral
    SpiDriver<SD1_Config::SpiBase>::Init();

    // Initialize SD card (CMD0, CMD8, ACMD41, CMD58...)
    if (SD_InitSPI<SD1_Config>() != 0)
    {
        return STA_NOINIT;
    }

    return 0;
}

DSTATUS disk_status(BYTE pdrv)
{
    return 0;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* buff)
{
    return RES_OK;
}


DRESULT disk_read(BYTE pdrv, BYTE* buff, LBA_t sector, UINT count)
{
    if (pdrv != 0) return RES_PARERR;

    uint8_t result;

    if (count == 1)
    {
        result = SD_ReadBlock<SD1_Config>(sector, buff);
    }
    else
    {
        result = SD_ReadBlocks<SD1_Config>(sector, count, buff);
    }

    //The SD_INIT_OK is my enum... the other RES stuff is FatFS, just to be clear
    if (result != SD_INIT_OK) return RES_ERROR;


    return RES_OK;
}




}