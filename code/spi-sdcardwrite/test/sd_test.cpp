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
#include "sdcardlittlefs.hpp"



//Generate pseudorandom testdata
uint32_t xorshift32(uint32_t rng)
{
    //static uint32_t rng = 0x12345678;   // seed
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    return rng;
}

//Compare two buffers
//return the index of mismatch,
//if successful return -1
int bufcmp(const char* a, const char* b, int size)
{
    for (int i = 0; i < size; ++i)
    {
        if (a[i] != b[i])
            return i;
    }
    return -1;
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


static struct{
    uint8_t init_res;
    int format_res;
    int mount_res;
    int open_res;
    lfs_ssize_t read_res;
    lfs_ssize_t write_res;
    int rewind_res;
    int close_res;
    int unmount_res;
    int readwrite_res;
}lfs_test;

void format_littlefs(){
    lfs_test.init_res = lfs_sdinit();
    lfs_t lfs_inst;
    const lfs_config& cfg = lfs_sdconfig();
    lfs_test.format_res = lfs_format(&lfs_inst, &cfg);

    if (lfs_test.format_res != 0)
    {
        return; // format failed
    }
}


//Fusewrapper for testing/mounting on PC
//https://github.com/littlefs-project/littlefs-fuse
void littlefs_Test()
{
    constexpr int BUFSIZE = 32;
    lfs_test.init_res = lfs_sdinit();
    char lfswritebuf[BUFSIZE] = "Hello littlefs";
    char lfsreadbuf[BUFSIZE];

    lfs_t lfs_inst;
    lfs_file_t file;
    const lfs_config& cfg = lfs_sdconfig();

    // Format obviously just needed once...
    // lfs_test.format_res = lfs_format(&lfs_inst, &cfg);
    // if (lfs_test.format_res != 0)
    // {
    //     return; // format failed
    // }

    lfs_test.mount_res = lfs_mount(&lfs_inst, &cfg);
    if (lfs_test.mount_res != 0)
    {
        return; //mount failed
    }
    lfs_test.open_res = lfs_file_open(&lfs_inst, &file, "hellolittlefs.txt", LFS_O_RDWR | LFS_O_CREAT);

    lfs_test.write_res = lfs_file_write(&lfs_inst, &file, &lfswritebuf, sizeof(lfswritebuf));
    lfs_test.rewind_res = lfs_file_rewind(&lfs_inst, &file);
    lfs_test.read_res = lfs_file_read(&lfs_inst, &file, &lfsreadbuf, sizeof(lfsreadbuf));

    lfs_test.readwrite_res = bufcmp(lfswritebuf, lfsreadbuf, sizeof(lfsreadbuf));
    
    // remember the storage is not updated until the file is closed successfully
    lfs_test.close_res = lfs_file_close(&lfs_inst, &file);

    // release any resources we were using
    lfs_test.unmount_res = lfs_unmount(&lfs_inst);

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
enum class Testfilesystem
{
    None,
    FatFs,
    littlefs,
};

Csd_Common csd;
template<typename Config>
void test_sd(Testfilesystem testfilesystem = Testfilesystem::None){
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

    //Doing this so the debugger knows the symbols
    //even if not testing for FatFs... is this a little annoying
    fat_test.mount = FR_NOT_READY;
    fat_test.open = FR_NOT_READY;
    fat_test.write = FR_NOT_READY;
    fat_test.close = FR_NOT_READY;

    //If sdcard-initialisation worked, then test FatFS
    if(sdtest.initsd == SD_INIT_OK && testfilesystem == Testfilesystem::FatFs)
    {
        FatFS_Test<Config>();
    }


    //Doing this so the debugger knows the symbols
    //even if not testing for littlefs...
    lfs_test.init_res = LFSINIT_NOT_INIT;
    lfs_test.format_res = 0xFE;
    lfs_test.mount_res = 0xFE;
    lfs_test.open_res = 0xFE;
    lfs_test.read_res = -1;
    lfs_test.write_res = -1;
    lfs_test.rewind_res = 0xFE;
    lfs_test.close_res = 0xFE;
    lfs_test.unmount_res = 0xFE;
    lfs_test.readwrite_res = 0xFE;
    //If sdcard-initialisation worked, then test littlefs
    if(sdtest.initsd == SD_INIT_OK && testfilesystem == Testfilesystem::littlefs)
    {
        littlefs_Test();
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

        // GpioPin<GPIOC_BASE, 13>::InputInit(Pull::Up);
        // GpioPin<GPIOC_BASE, 14>::InputInit(Pull::Up);
        // GpioPin<GPIOB_BASE, 4>::InputInit(Pull::Up);
        
        test_sd<SD1_Config>(Testfilesystem::littlefs);
        //test_sd<SD3_Config>();

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
