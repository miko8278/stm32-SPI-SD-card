#include "init_conf.hpp"
#include "stm32g431xx.h"
#include "GPIO_HAL.hpp"

int main()
{
    GPIO_Init();
    TIM2_Init();

    GpioPin<GPIOA_BASE,10>::OutputInit(Level::High);
    for(;;)
    {
        //ON for 10 secs
        GpioPin<GPIOA_BASE,10>::setHigh();
        delay_s<10>();
        //OFF for 3 secs
        GpioPin<GPIOA_BASE,10>::setLow();
        delay_s<3>();
    }

    return 0;
}