/*
 * License: MIT
 *
 * Author: Michael Kolorz
 *
 * File for measuring minimal RAM/Flash footprint using sdcarddriver only
 */
 
#include "init_conf.hpp"
#include "spidriver.hpp"
#include "sdcarddriver.hpp"
#include <stdint.h>

uint8_t sd_block_buffer[512]; 

int main()
 {
    TIM2_Init();
    delay_ms<500>();

    GPIO_Init();

    SD_InitSPI<SD1_Config>();
    using SPI_X = SpiDriver<SD1_Config::SpiBase>;

    //Configure the CS-Pins as Output
    GpioPin<SD1_Config::PortBase, SD1_Config::Pin>::OutputInit(Level::High);

    //Configure the CD-Pin as Input, we need Pullup
    GpioPin<SD1_Config::PortBase, SD1_Config::CD_Pin>::InputInit(Pull::Up);

    //Read the carddetect Pin
    uint8_t carddetect = static_cast<uint8_t>(GpioPin<SD1_Config::PortBase, SD1_Config::CD_Pin>::Read());

    //Configure the registers for SPI
    SpiDriver<SD1_Config::SpiBase>::Init();

    //Send some dummydata before starting
    for (uint8_t i = 0; i < 10; i++){
        SPI_X::Transfer(0xFF);
    }

    //Initialise SD-card into SPI-Mode
    SD_InitSPI<SD1_Config>();

    for (int i = 0; i < 10; i++)
    {
        SPI_X::Transfer(0xFF);
    }

    //Data for single block write
    for(uint16_t i = 0; i < 512; i++) 
    {
        sd_block_buffer[i] = i; //yea yea overflow, don't care
    }

    //Set to higher speed
    //SPI_1::SetHighSpeed(SPI_1::SpiDiv::Div16);
            //check state before write 
    // uint8_t status_before[2];
    // SD_SendCmd<SD1_Config>(13, 0, 0xFF, status_before, 1);

    SD_WriteBlock<SD1_Config>(200, sd_block_buffer);

    SD_ReadBlock<SD1_Config>(200, sd_block_buffer);


    return 0;
 }