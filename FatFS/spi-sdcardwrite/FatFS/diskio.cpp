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

#include "ff.h"			// Basic definitions of FatFs */
#include <cstdint>
#include "diskio.h"		// Declarations FatFs expects to be implemented


//Helper template
template<typename Config>
DSTATUS SD_disk_initialize()
{
    GpioPin<Config::PortBase, Config::Pin>::OutputHighInit();
    SpiDriver<Config::SpiBase>::Init();

    if (SD_InitSPI<Config>() != SD_INIT_OK)
        return STA_NOINIT;

    return RES_OK;
}

template<typename Config>
DRESULT SD_disk_ioctl(BYTE cmd, void* buff)
{
    constexpr int SD_SECTOR_SIZE = 512;
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
            uint8_t sd_csd_result = SD_GetCSD<Config>(buffer, 16);
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
            //in the end... It's just for mkfs in the end, not really important
            *(DWORD*)buff = 1;
            return RES_OK;
        break;
        //Note: FF_USE_TRIM must be 0, trim not implemented

        default: 
            return RES_PARERR;
        break;
    }
}

template<typename Config>
DRESULT SD_disk_read(BYTE* buff, LBA_t sector, UINT count)
{

    uint8_t result;

    if (count == 1)
    {
        result = SD_ReadBlock<Config>(sector, buff);
    }
    else
    {
        result = SD_ReadBlocks<Config>(sector, count, buff);
    }

    //The SD_INIT_OK is my enum... the other RES stuff is FatFS, just to be clear
    if (result != SD_READ_OK) return RES_ERROR;


    return RES_OK;
}

template<typename Config>
DRESULT SD_disk_write(const BYTE* buff, LBA_t sector, UINT count)
{

    uint8_t result;

    if (count == 1)
    {
        result = SD_WriteBlock<Config>(sector, buff);
    }
    else
    {
        result = SD_WriteBlocks<Config>(sector, count, buff);
    }

    if (result != SD_WRITE_OK) return RES_ERROR;

    return RES_OK;
}

extern "C" {

//Look in the correcsponding SD-Functions, this is just mapping
//the FatFS physical drives to my template functions
DSTATUS disk_initialize(BYTE pdrv)
{
    if (pdrv == 0){
        return SD_disk_initialize<SD1_Config>();
    }
    else if(pdrv == 1)
    {
        return SD_disk_initialize<SD3_Config>();
    }
    else
    {
        return STA_NOINIT;
    }
}

// :P we're always fine
DSTATUS disk_status(BYTE pdrv)
{
    return 0;
}

// Dummy for easy testing
// DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* buff)
// {
//     return RES_OK;
// }

//Look in the correcsponding SD-Functions, this is just mapping
//the FatFS physical drives to my template functions
DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* buff)
{
    if (pdrv == 0)
    {
        return SD_disk_ioctl<SD1_Config>(cmd, buff);
    }
    else if(pdrv == 1)
    {
        return SD_disk_ioctl<SD3_Config>(cmd, buff);
    }
    else
    {
        return RES_PARERR;
    }
}


//Look in the correcsponding SD-Functions, this is just mapping
//the FatFS physical drives to my template functions
DRESULT disk_read(BYTE pdrv, BYTE* buff, LBA_t sector, UINT count)
{
    if(pdrv == 0)
    {
        return SD_disk_read<SD1_Config>(buff, sector, count);
    }
    else if(pdrv == 1)
    {
        return SD_disk_read<SD3_Config>(buff, sector, count);
    }
    else
    {
        return RES_PARERR;
    }
}


DRESULT disk_write(BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count)
{
    if(pdrv == 0)
    {
        return SD_disk_write<SD1_Config>(buff, sector, count);
    }
    else if(pdrv == 1)
    {
        return SD_disk_write<SD3_Config>(buff, sector, count);
    }
    else
    {
        return RES_PARERR;
    }
}




}