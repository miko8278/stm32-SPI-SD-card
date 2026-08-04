#include "stm32g431xx.h"
#include "spidriver.hpp"
#include "sdcarddriver.hpp"
#include "GPIO_HAL.hpp"
#include "init_conf.hpp"

//Generate pseudorandom testdata
uint32_t xorshift32(uint32_t rnxg)
{
    static uint32_t rng = 0x12345678;   // seed
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

uint8_t sd_block_buffer[512]; 
uint8_t sd_block_buffer2[512];
//volatile uint8_t sd_block_buffer3[4096];
//volatile uint8_t sd_block_buffer4[4096];

//This struct is basically there so gdb can read those values back in a automated script
static struct{
    uint8_t cur_spi = 0;
    uint8_t carddetect;
    uint8_t initsd;
    uint8_t writeblock;
    uint8_t readblock;
    uint8_t sameblock;
    uint8_t writeblockmulti;
    uint8_t readblockmulti;
    uint8_t sameblockmulti;
} sdtest;


template<typename Config>
void test_sd(){
    sdtest.cur_spi = 1;
    using SPI_X = SpiDriver<Config::SpiBase>;

    //Configure the CS-Pins as Output
    GpioPin<Config::PortBase, Config::Pin>::OutputHighInit();

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

    for(uint16_t i = 0; i < 512; i++) {
        sd_block_buffer[i] = (uint8_t)xorshift32(0x12345678); 
    }
    //Set to higher speed
    //SPI_1::SetHighSpeed(SPI_1::SpiDiv::Div16);
            //check state before write 
    // uint8_t status_before[2];
    // SD_SendCmd<SD1_Config>(13, 0, 0xFF, status_before, 1);

    sdtest.writeblock = SD_WriteBlock<Config>(20, sd_block_buffer);

    sdtest.readblock = SD_ReadBlock<Config>(20, sd_block_buffer2);
        


    //check state after write 
    // uint8_t status_after[2];
    // SD_SendCmd<SD1_Config>(13, 0, 0xFF, status_after, 1);
}

int main()
{

    while (1)
    {
        GPIO_Init();
        test_sd<SD1_Config>();
        test_complete();
        for (volatile uint32_t i = 0; i < 10000; i++);
    }
}
