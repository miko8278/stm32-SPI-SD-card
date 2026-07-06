#include "stm32g431xx.h"

void delay(volatile uint32_t count) {
    while (count--) {
        __NOP(); 
    }
}

int main(void) {
    // Enable the clock for GPIO Port A
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;

    // Configure PA5 as a General Purpose Output
    GPIOA->MODER &= ~(GPIO_MODER_MODE5);
    GPIOA->MODER |= (1 << GPIO_MODER_MODE5_Pos);

    while (1) {

        GPIOA->ODR ^= GPIO_ODR_OD5;

        delay(1000000);
    }
}