/*
 * License: MIT
 *
 * Author: Michael Kolorz
 *
 */
 
#pragma once
#include "stm32g431xx.h"
#include "GPIO_HAL.hpp"
#include "spidriver.hpp"
#include "sdcarddriver.hpp"
#include <cstdint>

struct SD1_Config {
    static constexpr uint8_t cur_spi = 1;
    static constexpr uintptr_t SpiBase  = SPI1_BASE;
    static constexpr uintptr_t PortBase = GPIOA_BASE;
    static constexpr uint32_t  Pin      = 8; //Pin for Chip Select
    static constexpr uint32_t CD_Pin = 3; // Pin for Card Detect, a feature of sdcard-slot
    // Note, we are always having the detect and select on the same
    // GPIO as used in PortBase in this configuration, although it would be possible to design it freely 
};

//Note, I messed up my schematic, so where SPI2 should be used
//SPI 3 is connected... use SD3_Config for the 2. slot
struct SD2_Config {
    static constexpr uint8_t cur_spi = 2;
    static constexpr uintptr_t SpiBase  = SPI2_BASE;
    static constexpr uintptr_t PortBase = GPIOB_BASE;
    static constexpr uint32_t  Pin      = 10;
};

struct SD3_Config {
    static constexpr uint8_t cur_spi = 3;
    static constexpr uintptr_t SpiBase  = SPI3_BASE;
    static constexpr uintptr_t PortBase = GPIOB_BASE;
    static constexpr uint32_t  Pin      = 11;
    static constexpr uint32_t CD_Pin = 12;
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

    //SPI1
    GpioPin<GPIOA_BASE, 5>::AF(5);
    GpioPin<GPIOA_BASE, 6>::AF(5);
    GpioPin<GPIOA_BASE, 7>::AF(5);

    //SPI2 not needed right now

    //SPI3 chosen PINs
    // PB3 = SCK PB4 = MISO PB5 = MOSI -> Alternate Function mode
    GpioPin<GPIOB_BASE, 3>::AF(6);
    GpioPin<GPIOB_BASE, 4>::AF(6);
    GpioPin<GPIOB_BASE, 5>::AF(6);
    // GPIOB->MODER &= ~(GPIO_MODER_MODE4_Msk);
    // GPIOB->MODER |=  (0b10U << GPIO_MODER_MODE4_Pos);
    // GPIOB->OSPEEDR &= ~(3U << (4*2));
    // GPIOB->OSPEEDR |=  (3U << (4*2));

    //Set the CS-Pins to Low
    GpioPin<SD1_Config::PortBase, SD1_Config::Pin>::OutputInit(Level::High);
    GpioPin<SD3_Config::PortBase, SD3_Config::Pin>::OutputInit(Level::High);
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


//Free running 32-bit Timer... 
//Timer_HAL.hpp is a project for another time
static void TIM2_Init()
{
    // Enable clock TIM2
    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;

    // 16 MHz / (15 + 1) = 1 MHz
    TIM2->PSC = 15;

    //update prescaler, this seems a thing with the new stms...
    TIM2->EGR = TIM_EGR_UG;

    // Maximum period
    TIM2->ARR = 0xFFFFFFFF;

    TIM2->CNT = 0;
    // Start timer
    TIM2->CR1 |= TIM_CR1_CEN;
}




constexpr uint32_t TIM2_MAX_DELAY_US = 0xFFFFFFFFUL;
//This depends on TIM2 going in microsecond steps
template<uint32_t us>
static void delay_us()
{
    //if someone specifies more than 71 min
    static_assert(us <= 0xFFFFFFFFUL, "delay_us(): duration exceeds TIM2 range");

    const uint32_t start = TIM2->CNT;

    while (static_cast<uint32_t>(TIM2->CNT - start) < us)
    {
    }
}

template<uint32_t ms>
static void delay_ms()
{
    static_assert(ms <= TIM2_MAX_DELAY_US / 1000ULL, "Delay_ms(): duration exceeds TIM2 range");

    delay_us<static_cast<uint64_t>(ms) * 1000ULL>();
}

template<uint32_t s>
static void delay_s()
{
    static_assert(s <= TIM2_MAX_DELAY_US / 1000000ULL, "Delay_s(): duration exceeds TIM2 range");

    delay_us<static_cast<uint64_t>(s) * 1000000ULL>();
}