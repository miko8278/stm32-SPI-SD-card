#pragma once
#include "stm32g431xx.h"
#include "GPIO_HAL.hpp"
#include "spidriver.hpp"
#include "sdcarddriver.hpp"

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