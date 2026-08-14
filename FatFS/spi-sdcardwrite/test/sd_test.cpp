/*
 * License: MIT
 *
 * Author: Michael Kolorz
 *
 * SPI-mode SD card driver tests for STM32G431.
 */


#include "stm32g431xx.h"
#include "spidriver.hpp"
#include "sdcarddriver.hpp"
#include "GPIO_HAL.hpp"
#include "init_conf.hpp"
#include "ff.h"

//Generate pseudorandom testdata
uint32_t xorshift32(uint32_t rng)
{
    //static uint32_t rng = 0x12345678;   // seed
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    return rng;
}

//This is just for gdb to have a 
//defined breakpoint
void test_complete()
{
    // nothing
}

void tests_done()
{
    // nothing
}

void debug_gdb_print(const char* str)
{
    // breakpoint target only
    //
}

static struct{
    FRESULT mount = FR_NOT_READY;
    FRESULT open = FR_NOT_READY;
    FRESULT write = FR_NOT_READY;
    FRESULT close = FR_NOT_READY;
}fat_test;
FATFS fs;
template<typename Config>
void FatFS_Test()
{
    
    fat_test.mount = f_mount(&fs, "", 1);

    if(fat_test.mount == FR_OK){
        FIL file;
        UINT written;   

        fat_test.open = f_open(&file, "Bello.txt", FA_WRITE | FA_CREATE_ALWAYS);

        if (fat_test.open == FR_OK)
        {
            const char msg[] = "Hello SD!\n";

            fat_test.write = f_write(&file, msg, sizeof(msg) - 1, &written);
            fat_test.close = f_close(&file);
        }
    }
}


//Singleblock buffers
uint8_t sd_block_buffer[512]; 
uint8_t sd_block_buffer2[512];

//Multiblock buffers
constexpr int BUF3_SIZE = 4096;
constexpr int BUF4_SIZE = BUF3_SIZE;
uint8_t sd_block_buffer3[BUF3_SIZE];
uint8_t sd_block_buffer4[BUF4_SIZE];

//CSD-register buffer for getting information like the card-size with CMD9
constexpr int CSD_REG_SIZE = 16;
uint8_t csd_reg[CSD_REG_SIZE];

//This struct is basically there so gdb can read those values back in a automated script
static struct{
    uint8_t cur_spi = 0;
    uint8_t carddetect;
    uint8_t initsd;
    uint8_t writeblock;
    uint8_t readblock;
    uint16_t sameblock;
    uint8_t writeblockmulti;
    uint8_t readblockmulti;
    uint16_t sameblockmulti;
    uint8_t csd;
    uint32_t csizeMB;
    int timeout = 0;
} sdtest;

//Wasting some ram here, but this is nothing
//compared to those 4kb buffers for the read write multi.
//Basically, it's either v1 or v2 depending on what the card says.
//CSD_V1 csd_v1;
//CSD_V2 csd_v2;
Csd_Common csd;
template<typename Config>
void test_sd(){
    sdtest.cur_spi = Config::cur_spi;
    using SPI_X = SpiDriver<Config::SpiBase>;

    //Configure the CS-Pins as Output
    GpioPin<Config::PortBase, Config::Pin>::OutputInit(Level::High);

    //Configure the CD-Pin as Input, we need Pullup
    GpioPin<Config::PortBase, Config::CD_Pin>::InputInit(Pull::Up);

    //Read the carddetect Pin
    sdtest.carddetect = static_cast<uint8_t>(GpioPin<Config::PortBase, Config::CD_Pin>::Read());

    //Configure the registers for SPI
    SpiDriver<Config::SpiBase>::Init();

    //Send some dummydata before starting
    for (uint8_t i = 0; i < 10; i++){
        SPI_X::Transfer(0xFF);
    }

    //Initialise SD-card into SPI-Mode
    sdtest.initsd = SD_InitSPI<Config>();

    for (int i = 0; i < 10; i++)
    {
        SPI_X::Transfer(0xFF);
    }

    //Data for single block write
    for(uint16_t i = 0; i < 512; i++) 
    {
        sd_block_buffer[i] = (uint8_t)xorshift32(0x12345678); 
    }

    //Data for multi block write
    for(uint16_t i = 0; i < BUF3_SIZE; i++) 
    {
        sd_block_buffer3[i] = (uint8_t)xorshift32(0x12345670); 
    }

    //Set to higher speed
    //SPI_1::SetHighSpeed(SPI_1::SpiDiv::Div16);
            //check state before write 
    // uint8_t status_before[2];
    // SD_SendCmd<SD1_Config>(13, 0, 0xFF, status_before, 1);

    sdtest.writeblock = SD_WriteBlock<Config>(200, sd_block_buffer);

    sdtest.readblock = SD_ReadBlock<Config>(200, sd_block_buffer2);

    sdtest.writeblockmulti = SD_WriteBlocks<Config>(200, 8, sd_block_buffer3);
        
    sdtest.readblockmulti = SD_ReadBlocks<Config>(200, 8, sd_block_buffer4);

    //Check if written data = read data

    //Single Block
    sdtest.sameblock = 0;
    for(int i = 0; i < 512; ++i)
    {
        if(sd_block_buffer[i] != sd_block_buffer2[i]) sdtest.sameblock++;
    }


    //Multi Block
    sdtest.sameblockmulti = 0;
    for(int i = 0; i < BUF3_SIZE; ++i)
    {
        if(sd_block_buffer3[i] != sd_block_buffer4[i]) sdtest.sameblockmulti++;
    }


    //Get Sector Count test
    sdtest.csd = SD_GetCSD<Config>(csd_reg, CSD_REG_SIZE);
    parse_csd(csd_reg, &csd);
    sdtest.csizeMB = csd.capacityMB;


    //If sdcard-initialisation worked, then test FatFS
    if(sdtest.initsd)
    {
        FatFS_Test<Config>();
    }

    test_complete();
    //check state after write 
    // uint8_t status_after[2];
    // SD_SendCmd<SD1_Config>(13, 0, 0xFF, status_after, 1);
}

uint8_t initres1;
uint8_t initres2;
uint8_t r1;
int main()
{
    debug_gdb_print("Hello Debug\n");
    GPIO_Init();
    while (1)
    {
        
        //Test for Solderbridges/not connected Pins
        //RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
        // GpioPin<GPIOB_BASE, 3>::OutputHighInit();
        // GpioPin<GPIOB_BASE, 4>::OutputHighInit();
        //GpioPin<GPIOB_BASE, 5>::OutputHighInit();
        // GpioPin<GPIOB_BASE, 11>::OutputHighInit();

        GpioPin<GPIOC_BASE, 13>::InputInit(Pull::Up);
        GpioPin<GPIOC_BASE, 14>::InputInit(Pull::Up);

        GpioPin<GPIOB_BASE, 4>::InputInit(Pull::Up);
        
        test_sd<SD1_Config>();
        test_sd<SD3_Config>();

        //manual spi3 test...
        // GpioPin<SD1_Config::PortBase, SD1_Config::Pin>::OutputInit(Level::High);
        // SpiDriver<SPI1_BASE>::Init();
        // SpiDriver<SPI1_BASE>::Transfer(0xAA);
        // SpiDriver<SPI1_BASE>::Transfer(0x55);
        // SpiDriver<SPI1_BASE>::Transfer(0xAA);
        // SpiDriver<SPI1_BASE>::Transfer(0x55);
        // SpiDriver<SPI1_BASE>::Transfer(0xAA);
        // SpiDriver<SPI1_BASE>::Transfer(0x55);
        //initres1 = SD_InitSPI<SD1_Config>();

        // GpioPin<SD3_Config::PortBase, SD3_Config::Pin>::OutputInit(Level::High);
        // SpiDriver<SPI3_BASE>::Init();
        // SpiDriver<SPI3_BASE>::Transfer(0xAA);
        // SpiDriver<SPI3_BASE>::Transfer(0x55);
        // SpiDriver<SPI3_BASE>::Transfer(0xAA);
        // SpiDriver<SPI3_BASE>::Transfer(0x55);
        // SpiDriver<SPI3_BASE>::Transfer(0xAA);
        // SpiDriver<SPI3_BASE>::Transfer(0x55);
        //initres2 = SD_InitSPI<SD3_Config>();
        // {
        //     int timeout = 0;
        //     GpioPin<SD3_Config::PortBase, SD3_Config::Pin>::OutputInit(Level::High);
        //     SpiDriver<SPI3_BASE>::Init();
        //     SpiDriver<SPI3_BASE>::Transfer(0xFF);
        //     SpiDriver<SPI3_BASE>::Transfer(0xFF);
        //     SpiDriver<SPI3_BASE>::Transfer(0xFF);
        //     SpiDriver<SPI3_BASE>::Transfer(0xFF);
        //     SpiDriver<SPI3_BASE>::Transfer(0xFF);
        //     SpiDriver<SPI3_BASE>::Transfer(0xFF);
        //     SpiDriver<SPI3_BASE>::Transfer(0xFF);
        //     SpiDriver<SPI3_BASE>::Transfer(0xFF);
        //     SpiDriver<SPI3_BASE>::Transfer(0xFF);
        //     SpiDriver<SPI3_BASE>::Transfer(0xFF);
        //     ChipSelect<GPIOB_BASE, 11> chipselect_tmp;
        //     //manual cmd0-test SPI3, why does this s**t not work
        //     SpiDriver<SPI3_BASE>::Transfer(0x40);
        //     SpiDriver<SPI3_BASE>::Transfer(0x00);
        //     SpiDriver<SPI3_BASE>::Transfer(0x00);
        //     SpiDriver<SPI3_BASE>::Transfer(0x00);
        //     SpiDriver<SPI3_BASE>::Transfer(0x00);
        //     SpiDriver<SPI3_BASE>::Transfer(0x95);
        //     do {
        //         r1 = SpiDriver<SPI3_BASE>::Transfer(0xFF);
        //         sdtest.timeout++;
        //     } while (((r1 == 0xFF)) && (sdtest.timeout < 255));
        // }
        tests_done();

        for (volatile uint32_t i = 0; i < 10000; i++);

    }
}
