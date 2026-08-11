/*
 * License: MIT
 *
 * Author: Michael Kolorz
 *
 * GPIO-abstractions for STM32G431.
 */


#pragma once
#include "stm32g431xx.h"

//old def, not used right now
#define SD_CS_SELECT()     (GPIOA->BSRR = (1U << (8 + 16))) // PA9 LOW
#define SD_CS_DESELECT()   (GPIOA->BSRR = (1U << 8))        // PA9 HIGH

//For pullup/down
//I'd be better if I capsuled it inside the class, but I did that
//somewhere else and cpp makes the variable completely unreadable with having
//to call the class first
enum class Pull
{
    None,
    Up,
    Down
};

template<uintptr_t PortBase, uint32_t Pin>
struct GpioPin
{
private:
    static inline GPIO_TypeDef* const GPIOx =
        reinterpret_cast<GPIO_TypeDef*>(PortBase);

public:
    //Configure Pin in Outputmode and set it LOW by default
    static void OutputLowInit()
    {
        // Output mode
        GPIOx->MODER &= ~(0b11u << (Pin * 2));
        GPIOx->MODER |=  (0b01u << (Pin * 2));

        // High speed
        GPIOx->OSPEEDR &= ~(0b11u << (Pin * 2));
        GPIOx->OSPEEDR |=  (0b10u << (Pin * 2));

        // Push-pull
        GPIOx->OTYPER &= ~(1u << Pin);

        // No pull-up/down
        GPIOx->PUPDR &= ~(0b11u << (Pin * 2));
    }

    //Configure Pin in Outputmode and set it HIGH by default
    static void OutputHighInit()
    {
        OutputLowInit();
        // Default HIGH (CS inactive in SDcard context)
        GPIOx->BSRR = (1u << Pin);
    }

    static void setHigh()
    {
        GPIOx->BSRR = (1u << Pin);
    }
    static void setLow()
    {
        GPIOx->BSRR = (1u << (Pin + 16));
    }

    static void PullUpDown(Pull pull = Pull::None)
    {
        GPIOx->PUPDR &= ~(0b11u << (Pin * 2));
        switch (pull)
        {
            case Pull::Up:
                GPIOx->PUPDR |= (0b01u << (Pin * 2));
                break;

            case Pull::Down:
                GPIOx->PUPDR |= (0b10u << (Pin * 2));
                break;
        }
    }

    static void InputInit(Pull pull = Pull::None)
    {
        // Input mode (00)
        GPIOx->MODER &= ~(0b11u << (Pin * 2));


        PullUpDown(pull);
        // switch (pull)
        // {
        // case Pull::Up:
        //     GPIOx->PUPDR |= (0b01u << (Pin * 2));
        //     break;

        // case Pull::Down:
        //     GPIOx->PUPDR |= (0b10u << (Pin * 2));
        //     break;
        // }
    }

    static uint32_t Read()
    {
        return (GPIOx->IDR >> Pin) & 1u;
    }


    static void AF(uint8_t af)
    {
        // Alternate function mode
        GPIOx->MODER &= ~(0b11 << (Pin * 2));
        GPIOx->MODER |=  (0b10 << (Pin * 2));

        // AFRL or AFRH
        if constexpr (Pin < 8)
        {   
            
            GPIOx->AFR[0] &= ~(0xF << (Pin * 4));
            GPIOx->AFR[0] |=  (static_cast<uint32_t>(af) << (Pin * 4));
        }
        else
        {
            constexpr uint32_t shift = (Pin - 8) * 4;
            GPIOx->AFR[1] &= ~(0xF << shift);
            GPIOx->AFR[1] |=  (static_cast<uint32_t>(af) << shift);
        }
    }

};