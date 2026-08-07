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
#include <cstdint>
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

// Dummy for easy testing
// DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* buff)
// {
//     return RES_OK;
// }

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* buff)
{
    constexpr int SD_SECTOR_SIZE = 512;
    if (pdrv != 0) return RES_PARERR;

    
    switch (cmd)
    {
        //This is important for caching. My driver just waits until everything is written
        case CTRL_SYNC: return RES_OK;

        // Check out https://elm-chan.org/fsw/ff/doc/dioctl.html for the strange return types FatFS is
        // expecting. Those are just some int casts in the end, kind of overly complicated, but optimised.
        case GET_SECTOR_SIZE:
            *(WORD*)buff = SD_SECTOR_SIZE;
            return RES_OK;
        break;

        case GET_SECTOR_COUNT:
        {
            uint8_t buffer[16];
            Csd_Common csd;
            uint8_t sd_csd_result = SD_GetCSD<SD1_Config>(buffer, 16);
            if(sd_csd_result == SD_CSD_OK){
                parse_csd(buffer, &csd);
                *(LBA_t*)buff = csd.capacity / SD_SECTOR_SIZE; //
                return RES_OK;
            }
            return RES_ERROR;
        break;
        }
        
        case GET_BLOCK_SIZE:
            //*(DWORD*)buff = SD_GetEraseBlockSize();
            //The damn blocksize is written in CMD55+ACMD13, I'll leave this for when I have time 
            //in the end...
            *(DWORD*)buff = 1;
            return RES_OK;
        break;
        //Note: FF_USE_TRIM must be 0, trim not implemented

        default: 
            return RES_PARERR;
        break;
    }
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
    if (result != SD_READ_OK) return RES_ERROR;


    return RES_OK;
}


DRESULT disk_write(BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count)
{
    if (pdrv != 0) return RES_PARERR;

    uint8_t result;

    if (count == 1)
    {
        result = SD_WriteBlock<SD1_Config>(sector, buff);
    }
    else
    {
        result = SD_WriteBlocks<SD1_Config>(sector, count, buff);
    }

    if (result != SD_WRITE_OK) return RES_ERROR;

    return RES_OK;
}




}