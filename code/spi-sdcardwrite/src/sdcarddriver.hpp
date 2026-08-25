/*
 * License: MIT
 *
 * Author: Michael Kolorz
 *
 * SPI-mode SD card driver for STM32G431.
 */

#pragma once
#include "stm32g431xx.h"
#include "spidriver.hpp"
#include <cstdint>


template<uintptr_t PortBase, uint32_t Pin>
struct ChipSelect {
private:
    static inline GPIO_TypeDef* const port = reinterpret_cast<GPIO_TypeDef*>(PortBase);
public:
    ChipSelect() {
        port->BSRR = (1U << (Pin + 16));
    }

    ~ChipSelect() {
        //port->BSRR = (1U << (Pin + 16));
        port->BSRR = (1U << Pin);
    }
};


template<typename Config>
uint8_t SD_SendCmd(uint8_t cmd, uint32_t arg, uint8_t crc, uint8_t *response_buf, uint8_t extra_bytes)
{


    uint8_t r1;
    uint16_t timeout = 0;

    // send cmd
    SpiDriver<Config::SpiBase>::Transfer(cmd | 0x40);
    
    // 4 byte argument
    SpiDriver<Config::SpiBase>::Transfer((uint8_t)(arg >> 24));
    SpiDriver<Config::SpiBase>::Transfer((uint8_t)(arg >> 16));
    SpiDriver<Config::SpiBase>::Transfer((uint8_t)(arg >> 8));
    SpiDriver<Config::SpiBase>::Transfer((uint8_t)arg);
    
    // 1 byte CRC (SPI-Mode ignores it, but still needed even then... Needed for CMD0 and CMD8 though!)
    SpiDriver<Config::SpiBase>::Transfer(crc);

    // wait for first answer byte
    do {
        r1 = SpiDriver<Config::SpiBase>::Transfer(0xFF);
        timeout++;
    } while ((r1 == 0xFF) && (timeout < 255));

    // if there is a buffer, we put the rest inside
    if (response_buf) {
        response_buf[0] = r1;
        
        // when r1 is valid, we put the rest inside the buffer
        if (r1 != 0xFF && extra_bytes > 0) {
            for (uint8_t i = 1; i <= extra_bytes; i++) {
                response_buf[i] = SpiDriver<Config::SpiBase>::Transfer(0xFF);
            }
        }
    }

    return r1; //always return r1, even when there is more
}

enum SD_READ_RESULT : uint8_t
{
    SD_READ_OK = 0x00,
    SD_READ_ERROR_CMD17_CMD18 = 0x01,
    SD_READ_ERROR_DATATRANS = 0x02,
    SD_READ_ERROR_STOPTRANS = 0x04,
};
//template<uintptr_t Base, uintptr_t PortBase, uint32_t Pin>
template<typename Config>
uint8_t SD_ReadBlock(uint32_t block_addr, uint8_t *buffer)
{
    //This gets automatically destructed when leaving the function at any point
    ChipSelect<Config::PortBase,Config::Pin> chipselect_tmp;

    uint8_t r1;
    uint32_t timeout = 0;
    constexpr uint32_t T_OUT1 = 1000;
    constexpr uint32_t T_OUT2 = 2000;
    //SD_CS_SELECT();

    // send CMD17
    // CRC not important anymore => 0xFF
    SpiDriver<Config::SpiBase>::Transfer(17 | 0x40);
    SpiDriver<Config::SpiBase>::Transfer((uint8_t)(block_addr >> 24));
    SpiDriver<Config::SpiBase>::Transfer((uint8_t)(block_addr >> 16));
    SpiDriver<Config::SpiBase>::Transfer((uint8_t)(block_addr >> 8));
    SpiDriver<Config::SpiBase>::Transfer((uint8_t)block_addr);
    SpiDriver<Config::SpiBase>::Transfer(0xFF); 

    // waiting for answer... it has to be 0x00
    timeout = 0;
    do {
        r1 = SpiDriver<Config::SpiBase>::Transfer(0xFF);
        timeout++;
    } while ((r1 == 0xFF) && (timeout < T_OUT1));

    if (r1 != 0x00) {
        //SD_CS_DESELECT();
        return 0x01; // did not accept command
    }

    // wait for start 0xFE
    timeout = 0;
    do {
        r1 = SpiDriver<Config::SpiBase>::Transfer(0xFF);
        timeout++;
    } while ((r1 != 0xFE) && (timeout < T_OUT2));

    if (r1 != 0xFE) {
        //SD_CS_DESELECT();
        return 0x02; // timeout-error waiting for 0xFE
    }

    // read 512 byte blockdata
    for (uint16_t i = 0; i < 512; i++) {
        buffer[i] = SpiDriver<Config::SpiBase>::Transfer(0xFF);
    }

    // read 2 byte CRC, do nothing with it
    SpiDriver<Config::SpiBase>::Transfer(0xFF);
    SpiDriver<Config::SpiBase>::Transfer(0xFF);


    //SD_CS_DESELECT();
    SpiDriver<Config::SpiBase>::Transfer(0xFF); // extra cycle

    return 0x00; // success
}


template<typename Config>
uint8_t SD_ReadBlocks(uint32_t block_addr, uint32_t block_count, uint8_t *buffer)
{
    ChipSelect<Config::PortBase, Config::Pin> chipselect_tmp;

    uint8_t r1;
    uint32_t timeout;
    constexpr uint32_t T_OUT1 = 1000;
    constexpr uint32_t T_OUT2 = 2000;
    // CMD18
    SpiDriver<Config::SpiBase>::Transfer(18 | 0x40);
    SpiDriver<Config::SpiBase>::Transfer((uint8_t)(block_addr >> 24));
    SpiDriver<Config::SpiBase>::Transfer((uint8_t)(block_addr >> 16));
    SpiDriver<Config::SpiBase>::Transfer((uint8_t)(block_addr >> 8));
    SpiDriver<Config::SpiBase>::Transfer((uint8_t)block_addr);
    SpiDriver<Config::SpiBase>::Transfer(0xFF);

    // Wait for R1
    timeout = 0;
    do {
        r1 = SpiDriver<Config::SpiBase>::Transfer(0xFF);
        timeout++;
    } while ((r1 == 0xFF) && timeout < T_OUT1);

    if (r1 != 0x00)
        return 0x01;

    uint8_t *ptr = buffer;

    for (uint32_t block = 0; block < block_count; block++)
    {
        // Wait for data token
        timeout = 0;
        do {
            r1 = SpiDriver<Config::SpiBase>::Transfer(0xFF);
            timeout++;
        } while ((r1 != 0xFE) && timeout < T_OUT2);

        if (r1 != 0xFE)
            return 0x02;

        // Read 512 bytes
        for (uint16_t i = 0; i < 512; i++)
        {
            ptr[i] = SpiDriver<Config::SpiBase>::Transfer(0xFF);
        }

        ptr += 512;

        // Ignore CRC
        SpiDriver<Config::SpiBase>::Transfer(0xFF);
        SpiDriver<Config::SpiBase>::Transfer(0xFF);
    }

    // CMD12 STOP_TRANSMISSION
    SpiDriver<Config::SpiBase>::Transfer(12 | 0x40);
    SpiDriver<Config::SpiBase>::Transfer(0x00);
    SpiDriver<Config::SpiBase>::Transfer(0x00);
    SpiDriver<Config::SpiBase>::Transfer(0x00);
    SpiDriver<Config::SpiBase>::Transfer(0x00);
    SpiDriver<Config::SpiBase>::Transfer(0xFF);

    // CMD12 has a stuff byte before R1, bizarre, but does not work without it properly
    SpiDriver<Config::SpiBase>::Transfer(0xFF);
    do {
        r1 = SpiDriver<Config::SpiBase>::Transfer(0xFF);
    } while (r1 == 0xFF);

    if (r1 != 0x00){
        return 0x04;
    }

    return 0x00;
}


enum SD_WRITE_RESULT : uint8_t
{
    SD_WRITE_OK = 0x00,
    SD_WRITE_ERROR_CMD24_CMD25 = 0x01,
    SD_WRITE_ERROR_DATATRANS = 0x02,
    SD_WRITE_ERROR_TIMEOUTBLOCK = 0x04,
    SD_WRITE_ERROR_TIMEOUTTERMINATION = 0x08,
};
//template<uintptr_t Base, uintptr_t PortBase, uint32_t Pin>
template<typename Config>
uint8_t SD_WriteBlock(uint32_t block_addr, const uint8_t *buffer)
{
    //This gets automatically destructed when leaving the function at any point
    ChipSelect<Config::PortBase, Config::Pin> chipselect_tmp;
    uint8_t r1;
    uint32_t timeout = 0;
    constexpr uint32_t T_OUT1 = 1000;
    constexpr uint32_t T_OUT2 = 2000;
    //SD_CS_SELECT();

    // send CMD24
    SpiDriver<Config::SpiBase>::Transfer(24 | 0x40);
    SpiDriver<Config::SpiBase>::Transfer((uint8_t)(block_addr >> 24));
    SpiDriver<Config::SpiBase>::Transfer((uint8_t)(block_addr >> 16));
    SpiDriver<Config::SpiBase>::Transfer((uint8_t)(block_addr >> 8));
    SpiDriver<Config::SpiBase>::Transfer((uint8_t)block_addr);
    SpiDriver<Config::SpiBase>::Transfer(0xFF); // CRC doesn't matter


    // wait for answer 0x00
    timeout = 0;
    do {
        r1 = SpiDriver<Config::SpiBase>::Transfer(0xFF);
        timeout++;
    } while ((r1 == 0xFF) && (timeout < T_OUT1));

    if (r1 != 0x00) {
        //SD_CS_DESELECT();
        return 0x01; // Error: cmd not accepted
    }

    // one cycle for sync
    SpiDriver<Config::SpiBase>::Transfer(0xFF);

    // send start sign for single-block-write
    SpiDriver<Config::SpiBase>::Transfer(0xFE);

    // send 512 byte blockdata
    for (uint16_t i = 0; i < 512; i++) {
        SpiDriver<Config::SpiBase>::Transfer(buffer[i]);
    }

    // 2 byte dummy crc
    SpiDriver<Config::SpiBase>::Transfer(0xFF);
    SpiDriver<Config::SpiBase>::Transfer(0xFF);

    // data response from card
    r1 = SpiDriver<Config::SpiBase>::Transfer(0xFF);
    if ((r1 & 0x1F) != 0x05) {
        // data not accepted
        //SD_CS_DESELECT();
        return 0x02; 
    }

    // wait as long as SD-card is writing
    timeout = 0;
    do {
        r1 = SpiDriver<Config::SpiBase>::Transfer(0xFF);
        timeout++;
    } while ((r1 == 0x00) && (timeout < T_OUT2));

    //SD_CS_DESELECT();
    SpiDriver<Config::SpiBase>::Transfer(0xFF); // // extra cycle

    if (r1 == 0x00) {
        return 0x04; // timeout while writing
    }

    return 0x00; // success
}




template<typename Config>
uint8_t SD_WriteBlocks(uint32_t block_addr, uint32_t block_count, const uint8_t *buffer)
{
    ChipSelect<Config::PortBase, Config::Pin> chipselect_tmp;

    uint8_t r1;
    uint32_t timeout;
    constexpr uint32_t T_OUT1 = 1000;
    constexpr uint32_t T_OUT2 = 2000;
    constexpr uint32_t T_OUT3 = 2000;
    // CMD25 WRITE_MULTIPLE_BLOCK
    SpiDriver<Config::SpiBase>::Transfer(25 | 0x40);
    SpiDriver<Config::SpiBase>::Transfer((uint8_t)(block_addr >> 24));
    SpiDriver<Config::SpiBase>::Transfer((uint8_t)(block_addr >> 16));
    SpiDriver<Config::SpiBase>::Transfer((uint8_t)(block_addr >> 8));
    SpiDriver<Config::SpiBase>::Transfer((uint8_t)block_addr);
    SpiDriver<Config::SpiBase>::Transfer(0xFF);

    // wait for R1
    timeout = 0;
    do {
        r1 = SpiDriver<Config::SpiBase>::Transfer(0xFF);
        timeout++;
    } while ((r1 == 0xFF) && (timeout < T_OUT1));

    if (r1 != 0x00)
        return 0x01;


    const uint8_t *ptr = buffer;

    for (uint32_t block = 0; block < block_count; block++)
    {
        // multiple block write token
        SpiDriver<Config::SpiBase>::Transfer(0xFC);

        // write 512 bytes
        for (uint16_t i = 0; i < 512; i++)
        {
            SpiDriver<Config::SpiBase>::Transfer(ptr[i]);
        }

        ptr += 512;

        // dummy CRC
        SpiDriver<Config::SpiBase>::Transfer(0xFF);
        SpiDriver<Config::SpiBase>::Transfer(0xFF);


        // data response
        r1 = SpiDriver<Config::SpiBase>::Transfer(0xFF);

        if ((r1 & 0x1F) != 0x05)
            return 0x02;


        // wait while writing
        timeout = 0;
        do
        {
            r1 = SpiDriver<Config::SpiBase>::Transfer(0xFF);
            timeout++;
        }
        while (r1 != 0xFF && timeout < T_OUT2);

        if (timeout >= T_OUT2)
            return 0x04;
    }


    // stop transmission token
    SpiDriver<Config::SpiBase>::Transfer(0xFD);


    // wait until card is no longer busy
    timeout = 0;
    do {
        r1 = SpiDriver<Config::SpiBase>::Transfer(0xFF);
        timeout++;
    } while ((r1 != 0xFF) && (timeout < T_OUT3));

    if (timeout >= T_OUT3)
        return 0x08;

    // extra clock
    SpiDriver<Config::SpiBase>::Transfer(0xFF);

    return 0x00;
}


//Kind of ended up just using those in diskio.cpp
enum SD_INIT_RESULT : uint8_t
{
    SD_INIT_OK = 0x00,
    SD_INIT_ERROR_CMD0 = 0x02,
    SD_INIT_ERROR_CMD8 = 0x04,
    SD_INIT_ERROR_CMD55ACMD41 = 0x08
};
template<typename Config>
uint8_t SD_InitSPI()
{
    
    //For safety: When initialising more than once put
    //the CS-Pins into low state...
     

    uint8_t cmd8_resp[5];
    uint8_t cmd0_resp = 0xFF;
    uint8_t cmd55_resp = 0xFF;
    uint8_t acmd41_resp = 0xFF;

    //Without sending some of those it does not properly initialise
    for(int i = 0; i < 10; ++i)
    {
        SpiDriver<Config::SpiBase>::Transfer(0xFF);
    }

    ChipSelect<Config::PortBase, Config::Pin> chipselect_tmp;
    // CMD0
    //cmd0_resp = SD_SendCmd<Config>(0, 0, 0x95, nullptr, 0);
    //Give it some tries, might not work the first time, because card is busy...
    for (int i = 0; i < 20; ++i)
    {
        cmd0_resp = SD_SendCmd<Config>(0, 0, 0x95, nullptr, 0);

        if (cmd0_resp == 0x01)
            break;
    }

    if (cmd0_resp != 0x01)
        return SD_INIT_ERROR_CMD0;

    // CMD8
    SD_SendCmd<Config>(8, 0x000001AA, 0x87, cmd8_resp, 4);

    if (cmd8_resp[0] != 0x01 || cmd8_resp[4] != 0xAA)
        return SD_INIT_ERROR_CMD8;

    // ACMD41
    uint32_t timeout = 1000;

    while (timeout--)
    {
        cmd55_resp = SD_SendCmd<Config>(55, 0, 0xFF, nullptr, 0);

        if (cmd55_resp <= 0x01)
        {
            acmd41_resp = SD_SendCmd<Config>(41, 0x40000000, 0xFF, nullptr, 0);

            if (acmd41_resp == 0x00)
                return SD_INIT_OK;
        }

        //pause, probably fine without
        for (uint32_t i = 0; i < 1000; i++) 
        { 
            asm volatile("");
        }
        //SD_DelayMs(1);
    }

    //Timeout: loop of CMD55 and ACMD41 failed
    return SD_INIT_ERROR_CMD55ACMD41;
}

//This is a little terrible, there are 2 versions of this struct depending on 
//what is written in the first 2 bits...
struct CSD_V1
{
    uint8_t CSD_STRUCTURE : 2;       // [127:126]
    //uint8_t RESERVED_125_120 : 6;    // [125:120]
    uint8_t TAAC : 8;                    // [119:112]
    uint8_t NSAC : 8 ;                    // [111:104]
    uint8_t TRAN_SPEED: 8;              // [103:96]
    uint16_t CCC : 12;               // [95:84]
    uint16_t READ_BL_LEN : 4;        // [83:80]
    uint8_t READ_BL_PARTIAL : 1;     // [79]
    uint8_t WRITE_BLK_MISALIGN : 1;  // [78]
    uint8_t READ_BLK_MISALIGN : 1;   // [77]
    uint8_t DSR_IMP : 1;             // [76]
    //uint8_t RESERVED_75_74 : 2;      // [75:74]
    uint16_t C_SIZE : 12;        // [73:62]
    uint8_t VDD_R_CURR_MIN : 3;      // [61:59]
    uint8_t VDD_R_CURR_MAX : 3;      // [58:56]
    uint8_t VDD_W_CURR_MIN : 3;      // [55:53]
    uint8_t VDD_W_CURR_MAX : 3;      // [52:50]
    uint8_t C_SIZE_MULT : 3;     // [49:47]
    uint8_t ERASE_BLK_EN : 1;        // [46]
    uint8_t SECTOR_SIZE : 7;     // [45:39]
    uint8_t WP_GRP_SIZE : 7;         // [38:32]
    uint8_t WP_GRP_ENABLE : 1;       // [31]
    //uint8_t RESERVED_30_29 :2;         // [30:29]
    uint8_t R2W_FACTOR : 3;          // [28:26]
    uint8_t WRITE_BL_LEN : 4;        // [25:22]
    uint8_t WRITE_BL_PARTIAL : 1;    // [21]
    //uint8_t RESERVED_20_16 : 5     // [20:16]
    uint8_t FILE_FORMAT_GRP : 1;     // [15]
    uint8_t COPY : 1;                // [14]
    uint8_t PERM_WRITE_PROTECT : 1;  // [13]
    uint8_t TMP_WRITE_PROTECT : 1;   // [12]
    uint8_t FILE_FORMAT : 2;         // [11:10]
    uint8_t _CRC : 7;                 // [7:1]
    //uint8_t NOT_USED_ALWAYS_ONE : 1 // [0]
};

struct CSD_V2
{
    //[0]
    uint8_t CSD_STRUCTURE : 2;       // [127:126]
    //uint8_t RESERVED_125_120 : 6;    // [125:120]
    //[1]
    uint8_t TAAC : 8;                // [119:112]
    //[2]
    uint8_t NSAC : 8;                // [111:104]
    //[3]
    uint8_t TRAN_SPEED : 8;          // [103:96]
    //[4..5]
    uint16_t CCC : 12;               // [95:84]
    uint16_t READ_BL_LEN : 4;        // [83:80]
    //[6..7..8..9]
    uint8_t READ_BL_PARTIAL : 1;     // [79]
    uint8_t WRITE_BLK_MISALIGN : 1;  // [78]
    uint8_t READ_BLK_MISALIGN : 1;   // [77]
    uint8_t DSR_IMP : 1;             // [76]
    //uint8_t RESERVED_75_70 : 6;    // [75:70]
    uint32_t C_SIZE : 22;            // [69:48]

    uint8_t RESERVED_47 : 1;         // [47]
    uint8_t ERASE_BLK_EN : 1;        // [46]
    uint8_t SECTOR_SIZE : 7;         // [45:39]
    uint8_t WP_GRP_SIZE : 7;         // [38:32]
    uint8_t WP_GRP_ENABLE : 1;       // [31]
    //uint8_t RESERVED_30_29 : 2;    // [30:29]
    uint8_t R2W_FACTOR : 3;           // [28:26]
    uint8_t WRITE_BL_LEN : 4;         // [25:22]
    uint8_t WRITE_BL_PARTIAL : 1;     // [21]
    //uint8_t RESERVED_20_16 : 5;    // [20:16]
    uint8_t FILE_FORMAT_GRP : 1;      // [15]
    uint8_t COPY : 1;                 // [14]
    uint8_t PERM_WRITE_PROTECT : 1;   // [13]
    uint8_t TMP_WRITE_PROTECT : 1;    // [12]
    uint8_t FILE_FORMAT : 2;          // [11:10]
    //uint8_t RESERVED_9_8 : 2;       // [9:8]
    uint8_t _CRC : 7;                  // [7:1]
    //uint8_t NOT_USED_ALWAYS_ONE : 1; // [0]
};

//Ok, let's keep it low, here just goes in what 
//I need right now and can parse easily
//Keeping the V1 and V2 structs for documentation though
struct Csd_Common{
    uint8_t CSD_STRUCTURE : 2; 
    uint32_t CSIZE;
    uint8_t READ_BL_LEN : 4;
    uint8_t C_SIZE_MULT : 3;
    uint8_t ERASE_BLK_EN : 1; 
    uint8_t SECTOR_SIZE : 7;
    uint64_t capacity; // this is calculated using CSIZE
    uint32_t capacityMB;
};


enum SD_CSD_RESULT : uint8_t
{
    SD_CSD_OK = 0x00,
    SD_CSD_ERROR_CMD9 = 0x01,
    SD_CSD_ERROR_DATATRANS = 0x02,
    SD_CSD_ERROR_WRONGBUFSIZE = 0x04,
};
template<typename Config>
uint8_t SD_GetCSD(uint8_t *buffer, const uint8_t BUF_SIZE)
{
    //So the dumb thing is that my SendCmd does not work, because we have to wait for some 0xFE, just
    //like in the read. This function looks very much like the read function -.-
    //uint8_t cmd9_resp = SD_SendCmd<Config>(9, 0, 0xFF, nullptr, 0);
    //This gets automatically destructed when leaving the function at any point


    if(BUF_SIZE < 16) return SD_CSD_ERROR_WRONGBUFSIZE;

    ChipSelect<Config::PortBase,Config::Pin> chipselect_tmp;

    uint8_t r1 = 0xFF;
    uint32_t timeout = 0;
    constexpr uint32_t T_OUT1 = 1000;
    constexpr uint32_t T_OUT2 = 2000;
    //SD_CS_SELECT();

    // send CMD9
    // No Data
    // CRC not important anymore => 0xFF
    SpiDriver<Config::SpiBase>::Transfer(9 | 0x40);
    SpiDriver<Config::SpiBase>::Transfer(0x00);
    SpiDriver<Config::SpiBase>::Transfer(0x00);
    SpiDriver<Config::SpiBase>::Transfer(0x00);
    SpiDriver<Config::SpiBase>::Transfer(0x00);
    SpiDriver<Config::SpiBase>::Transfer(0xFF); 

    // waiting for answer... it has to be 0x00
    timeout = 0;
    do {
        r1 = SpiDriver<Config::SpiBase>::Transfer(0xFF);
        timeout++;
    } while ((r1 == 0xFF) && (timeout < T_OUT1));

    if (r1 != 0x00 || timeout >= T_OUT1) {
        //SD_CS_DESELECT();
        //This does not trigger when sd-card slot is empty... why?
        //Proably miso-line low => gets a 0 when transfered
        return SD_CSD_ERROR_CMD9; // did not accept command
    }

    // wait for start 0xFE
    timeout = 0;
    do {
        r1 = SpiDriver<Config::SpiBase>::Transfer(0xFF);
        timeout++;
    } while ((r1 != 0xFE) && (timeout < T_OUT2));

    if (r1 != 0xFE) {
        //SD_CS_DESELECT();
        return SD_CSD_ERROR_DATATRANS; // timeout-error waiting for 0xFE
    }

    // read 16 byte CSD register
    constexpr uint8_t CSD_SIZE = 16;
    for (uint8_t i = 0; i < CSD_SIZE; i++) {
        buffer[i] = SpiDriver<Config::SpiBase>::Transfer(0xFF);
    }

    // read 2 byte CRC, do nothing with it
    SpiDriver<Config::SpiBase>::Transfer(0xFF);
    SpiDriver<Config::SpiBase>::Transfer(0xFF);


    //SD_CS_DESELECT();
    SpiDriver<Config::SpiBase>::Transfer(0xFF); // extra cycle

    return SD_CSD_OK; // success


}

//Don't forget to inline inside a .hpp when using non-template functions!
inline void parse_csd_v1(const uint8_t* rawcsd, Csd_Common* csd){
    //V1 is terrible, i don't know if this is correct, I've no csd_v1 card here...
    //Don't know if they are even sold right now
    csd->CSIZE = ((rawcsd[7] & 0x03) << 10) | (rawcsd[8] << 2) | ((rawcsd[9] & 0xC0) >> 6);

    csd->READ_BL_LEN = rawcsd[5] & 0x0F;

    csd->C_SIZE_MULT =((rawcsd[10] & 0x03) << 1) | ((rawcsd[11] & 0x80) >> 7);

    uint32_t block_len = 1 << csd->READ_BL_LEN;
    uint32_t mult = 1 << (csd->C_SIZE_MULT + 2);

    csd->capacity =(uint64_t)(csd->CSIZE + 1) * mult * block_len;

    //It'll still work, this is mainly for letting mkfs make smart decisions
    csd->SECTOR_SIZE = 1;
}

inline void parse_csd_v2(const uint8_t* rawcsd, Csd_Common* csd){
    //V2 is much easier
    //csize for card size calculation (they call it capacity though)
    csd->CSIZE = ((rawcsd[7] & 0x3F) << 16) | (rawcsd[8] << 8) | rawcsd[9];

    //This is how big the erase blocks are... NOT... this damn block is just 
    //for V1 and not used in V2... what a SCAM, I'd need CMD55 + ACMD13
    //The naming is not my fault, it's from the SD protocol
    //csd->ERASE_BLK_EN  = (csd[10] >> 6) & 0x01;
    //csd->SECTOR_SIZE = ((csd[10] & 0x3F) << 1) | ((csd[11] >> 7) & 0x01);
    //this is directly from the protocol
    csd->capacityMB = (uint64_t)(csd->CSIZE + 1) * 512 * 1024 / 1000000; //Marketing: This is really how it's calculated, a lie kind of :D
    csd->capacity = (uint64_t)(csd->CSIZE + 1) * 512 * 1024;
}

enum PARSE_CSD_RESULT : uint8_t
{
    PARSE_CSD_V1 = 0x00,
    PARSE_CSD_V2 = 0x01,
    PARSE_CSD_RESERVED1 = 0x02,
    PARSE_CSD_RESERVED2 = 0x03,
};
inline uint8_t parse_csd(const uint8_t* rawcsd, Csd_Common* csd)
{
    //Find out V1 or V2
    csd->CSD_STRUCTURE = (rawcsd[0] >> 6) & 0x03;

    switch(csd->CSD_STRUCTURE)
    {
        case PARSE_CSD_V1:
            parse_csd_v1(rawcsd, csd);
            return PARSE_CSD_V1;
        break;

        case PARSE_CSD_V2:
            parse_csd_v2(rawcsd, csd);
            return PARSE_CSD_V2;
        break;

        case PARSE_CSD_RESERVED1:
        case PARSE_CSD_RESERVED2:
            return PARSE_CSD_RESERVED1;
        break;
    }
}

