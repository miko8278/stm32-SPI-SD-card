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

void test_complete()
{
    // nothing
}

uint8_t sd_block_buffer[512]; 
uint8_t sd_block_buffer2[512];
uint8_t sd_block_buffer3[4096];
uint8_t sd_block_buffer4[4096];

int main()
{
    uint8_t cmd0_resp = 0xFF;
    uint8_t cmd8_resp[5];
    uint8_t cmd55_resp = 0xFF;
    uint8_t acmd41_resp = 0xFF;
    uint16_t timeout = 0;
    using SPI_1 = SpiDriver<SPI1_BASE>;
    //using SPI_2 = SpiDriver<SPI2_BASE>;

    //Configure the alternate functions
    GPIO_Init();

    //Configure the CS-Pins as Output
    GpioPin<GPIOA_BASE, SD1_Config::Pin>::OutputHighInit();

    //Configure as indicatorpin
    GpioPin<GPIOA_BASE, 4>::OutputLowInit();

    //Configure the registers for SPI
    SpiDriver<SPI1_BASE>::Init();

    //Send some dummydata before starting
    for (uint8_t i = 0; i < 10; i++){
        SPI_1::Transfer(0xFF);
    }

    while (1)
    {
        
        //Initialise SD-card into SPI-Mode
        SD_InitSPI<SD1_Config>();
    
        for (int i = 0; i < 10; i++)
        {
            SPI_1::Transfer(0xFF);
        }
        //Set to higher speed
        //SPI_1::SetHighSpeed(SPI_1::SpiDiv::Div16);

        for(uint16_t i = 0; i < 512; i++) {
            sd_block_buffer[i] = (uint8_t)xorshift32(0x12345678); 
        }

        //check state before write 
        uint8_t status_before[2];
        SD_SendCmd<SD1_Config>(13, 0, 0xFF, status_before, 1);

        uint8_t write_result = SD_WriteBlock<SD1_Config>(20, sd_block_buffer);

        //check state after write 
        uint8_t status_after[2];
        SD_SendCmd<SD1_Config>(13, 0, 0xFF, status_after, 1);

        if (write_result == 0x00) {
            // read block again to verify
            uint8_t read_back_result = SD_ReadBlock<SD1_Config>(21, sd_block_buffer2);
            
            if (read_back_result == 0x00) {
                test_complete();
                volatile uint8_t testtest = 0;
                //__BKPT(0); // debugger softbreakpoint
            }
        }

        uint8_t result_writeblocks = SD_WriteBlocks<SD1_Config>(10, 8, sd_block_buffer3);
        if (result_writeblocks == 0x00){
            uint8_t result_readblocks = SD_ReadBlocks<SD1_Config>(9,8, sd_block_buffer4);
        }
        for (volatile uint32_t i = 0; i < 10000; i++);
    }
}
