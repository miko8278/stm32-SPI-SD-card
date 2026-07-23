#include "stm32g431xx.h"
#include "spidriver.hpp"
#include "sdcarddriver.hpp"
#include "GPIO_HAL.hpp"



struct SD1_Config {
    static constexpr uintptr_t SpiBase  = SPI1_BASE;
    static constexpr uintptr_t PortBase = GPIOA_BASE;
    static constexpr uint32_t  Pin      = 8;
};

//Note, I messed up my schematic, so where SPI2 should be used
//SPI 3 is connected... use SD3_Config for the 2. slot
struct SD2_Config {
    static constexpr uintptr_t SpiBase  = SPI2_BASE;
    static constexpr uintptr_t PortBase = GPIOB_BASE;
    static constexpr uint32_t  Pin      = 10;
};

struct SD3_Config {
    static constexpr uintptr_t SpiBase  = SPI3_BASE;
    static constexpr uintptr_t PortBase = GPIOB_BASE;
    static constexpr uint32_t  Pin      = 11;
};


/**
 * @brief Initialises the needed GPIOs. It's here 
 * where the SPI Pins are being chosen.
 */
static void GPIO_Init()
{
    // Enable clocks
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;

    GpioPin<GPIOA_BASE, 5>::AF(5);
    GpioPin<GPIOA_BASE, 6>::AF(5);
    GpioPin<GPIOA_BASE, 7>::AF(5);


    //SPI3 chosen PINs
    // PB3 = SCK PB4 = MISO PB5 = MOSI -> Alternate Function mode
    GpioPin<GPIOB_BASE, 3>::AF(6);
    GpioPin<GPIOB_BASE, 4>::AF(6);
    GpioPin<GPIOB_BASE, 5>::AF(6);
    // GPIOB->MODER &= ~(GPIO_MODER_MODE4_Msk);
    // GPIOB->MODER |=  (0b10U << GPIO_MODER_MODE4_Pos);
    // GPIOB->OSPEEDR &= ~(3U << (4*2));
    // GPIOB->OSPEEDR |=  (3U << (4*2));

    // GPIOB->PUPDR &= ~(3U << (4*2));
    // GPIOB->PUPDR |=  (1U << (4*2)); // pull-up
    // GPIOB->MODER &= ~(3U << (4 * 2));
    // GPIOB->MODER |=  (2U << (4 * 2));

    // GPIOB->AFR[0] &= ~(0xFu << 16);
    // GPIOB->AFR[0] |=  (4U << 16);
    // // Select AF5 for PB4
    // GPIOB->AFR[0] &= ~(GPIO_AFRL_AFSEL4_Msk);
    // GPIOB->AFR[0] |=  (5U << GPIO_AFRL_AFSEL4_Pos);
    // PB3/PB4/PB5 -> Alternate Function mode
    // GPIOB->MODER &= ~(GPIO_MODER_MODE3_Msk |
    //                 GPIO_MODER_MODE4_Msk |
    //                 GPIO_MODER_MODE5_Msk);

    // GPIOB->MODER |=
    //     (2U << GPIO_MODER_MODE3_Pos) |
    //     (2U << GPIO_MODER_MODE4_Pos) |
    //     (2U << GPIO_MODER_MODE5_Pos);


    // Push-pull, this is standard actually, but somewhere I have to use open-drain I think
    // GPIOA->OTYPER &= ~((1U << 4) | (1U << 5) | (1U << 7) | (1U << 9));


    // // No pull-up/down
    // GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPD5_Msk | GPIO_PUPDR_PUPD7_Msk);

    // // Very high speed (no impact on freq, just driver strength)
    // GPIOA->OSPEEDR |=
    //     (3U << GPIO_OSPEEDR_OSPEED5_Pos) |
    //     (3U << GPIO_OSPEEDR_OSPEED7_Pos);

}



//Generate pseudorandom testdata
uint32_t xorshift32(uint32_t rnxg)
{
    static uint32_t rng = 0x12345678;   // seed
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    return rng;
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

        uint8_t write_result = SD_WriteBlock<SD1_Config>(11, sd_block_buffer);

        //check state after write 
        uint8_t status_after[2];
        SD_SendCmd<SD1_Config>(13, 0, 0xFF, status_after, 1);

        if (write_result == 0x00) {
            // read block again to verify
            uint8_t read_back_result = SD_ReadBlock<SD1_Config>(10, sd_block_buffer2);
            
            if (read_back_result == 0x00) {
                __BKPT(0); // debugger softbreakpoint
            }
        }

        uint8_t result_writeblocks = SD_WriteBlocks<SD1_Config>(10, 8, sd_block_buffer3);
        if (result_writeblocks == 0x00){
            uint8_t result_readblocks = SD_ReadBlocks<SD1_Config>(9,8, sd_block_buffer4);
        }
        for (volatile uint32_t i = 0; i < 10000; i++);
    }
}