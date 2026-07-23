#pragma once
#include "stm32g431xx.h"
#include "spidriver.hpp"


template<uintptr_t PortBase, uint32_t Pin>
struct ChipSelect {
private:
    static inline GPIO_TypeDef* const port = reinterpret_cast<GPIO_TypeDef*>(PortBase);
public:
    ChipSelect() {
        port->BSRR = (1U << (Pin + 16));
    }

    ~ChipSelect() {
        port->BSRR = (1U << Pin);
    }
};


template<typename Config>
uint8_t SD_SendCmd(uint8_t cmd, uint32_t arg, uint8_t crc, uint8_t *response_buf, uint8_t extra_bytes)
{

    //This gets automatically destructed when leaving the function at any point
    ChipSelect<Config::PortBase, Config::Pin> chipselect_tmp;


    uint8_t r1;
    uint16_t timeout = 0;

    //chip select
    //SD_CS_SELECT();

    // send cmd
    SpiDriver<Config::SpiBase>::Transfer(cmd | 0x40);
    
    // 4 byte argument
    SpiDriver<Config::SpiBase>::Transfer((uint8_t)(arg >> 24));
    SpiDriver<Config::SpiBase>::Transfer((uint8_t)(arg >> 16));
    SpiDriver<Config::SpiBase>::Transfer((uint8_t)(arg >> 8));
    SpiDriver<Config::SpiBase>::Transfer((uint8_t)arg);
    
    // 1 byte CRC (SPI-Mode ignores it, but still needed even then)
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

    //SD_CS_DESELECT();

    //Maybe a filler Byte is needed later
    //SpiDriver<Base>::Transfer(0xFF);

    return r1; //always return r1, even when there is more
}


//template<uintptr_t Base, uintptr_t PortBase, uint32_t Pin>
template<typename Config>
uint8_t SD_ReadBlock(uint32_t block_addr, uint8_t *buffer)
{
    //This gets automatically destructed when leaving the function at any point
    ChipSelect<Config::PortBase,Config::Pin> chipselect_tmp;

    uint8_t r1;
    uint16_t timeout = 0;

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
    } while ((r1 == 0xFF) && (timeout < 1000));

    if (r1 != 0x00) {
        //SD_CS_DESELECT();
        return 0x01; // did not accept command
    }

    // wait for start 0xFE
    timeout = 0;
    do {
        r1 = SpiDriver<Config::SpiBase>::Transfer(0xFF);
        timeout++;
    } while ((r1 != 0xFE) && (timeout < 0xFFFF));

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
    uint16_t timeout;

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
    } while ((r1 == 0xFF) && timeout < 1000);

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
        } while ((r1 != 0xFE) && timeout < 0xFFFF);

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

    // CMD12 has a stuff byte before R1
    do {
        r1 = SpiDriver<Config::SpiBase>::Transfer(0xFF);
    } while (r1 == 0xFF);

    if (r1 != 0x00)
        return 0x03;

    return 0x00;
}


//template<uintptr_t Base, uintptr_t PortBase, uint32_t Pin>
template<typename Config>
uint8_t SD_WriteBlock(uint32_t block_addr, const uint8_t *buffer)
{
    //This gets automatically destructed when leaving the function at any point
    ChipSelect<Config::PortBase, Config::Pin> chipselect_tmp;
    uint8_t r1;
    uint16_t timeout = 0;

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
    } while ((r1 == 0xFF) && (timeout < 1000));

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
    } while ((r1 == 0x00) && (timeout < 0xFFFF));

    //SD_CS_DESELECT();
    SpiDriver<Config::SpiBase>::Transfer(0xFF); // // extra cycle

    if (r1 == 0x00) {
        return 0x03; // timeout while writing
    }

    return 0x00; // success
}




template<typename Config>
uint8_t SD_WriteBlocks(uint32_t block_addr, uint32_t block_count, const uint8_t *buffer)
{
    ChipSelect<Config::PortBase, Config::Pin> chipselect_tmp;

    uint8_t r1;
    uint16_t timeout;

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
    } while ((r1 == 0xFF) && (timeout < 1000));

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


        // wait while programming
        timeout = 0;
        do {
            r1 = SpiDriver<Config::SpiBase>::Transfer(0xFF);
            timeout++;
        } while ((r1 == 0x00) && (timeout < 0xFFFF));


        if (timeout >= 0xFFFF)
            return 0x03;
    }


    // stop transmission token
    SpiDriver<Config::SpiBase>::Transfer(0xFD);


    // wait until card is no longer busy
    timeout = 0;
    do {
        r1 = SpiDriver<Config::SpiBase>::Transfer(0xFF);
        timeout++;
    } while ((r1 == 0x00) && (timeout < 0xFFFF));


    if (timeout >= 0xFFFF)
        return 0x04;


    // extra clocks
    SpiDriver<Config::SpiBase>::Transfer(0xFF);

    return 0x00;
}

template<typename Config>
bool SD_InitSPI()
{
    uint8_t cmd8_resp[5];
    uint8_t cmd0_resp = 0xFF;
    uint8_t cmd55_resp = 0xFF;
    uint8_t acmd41_resp = 0xFF;

    // CMD0
    cmd0_resp = SD_SendCmd<Config>(0, 0, 0x95, nullptr, 0);
    if (cmd0_resp != 0x01)
        return false;

    // CMD8
    SD_SendCmd<Config>(8, 0x000001AA, 0x87, cmd8_resp, 4);

    if (cmd8_resp[0] != 0x01 || cmd8_resp[4] != 0xAA)
        return false;

    // ACMD41
    uint32_t timeout = 1000;

    while (timeout--)
    {
        cmd55_resp = SD_SendCmd<Config>(55, 0, 0xFF, nullptr, 0);

        if (cmd55_resp <= 0x01)
        {
            acmd41_resp = SD_SendCmd<Config>(41, 0x40000000, 0xFF, nullptr, 0);

            if (acmd41_resp == 0x00)
                return true;
        }

        //pause, probably fine without
        //for (volatile uint32_t i = 0; i < 1000; i++);
        //SD_DelayMs(1);
    }

    return false;
}