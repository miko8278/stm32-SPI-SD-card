#include "stm32g431xx.h"

static void GPIO_Init(void)
{
    // Enable clocks
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

    // PA5 = SCK, PA6 = MISO ,PA7 = MOSI (Alternate Function)
    GPIOA->MODER &= ~(GPIO_MODER_MODE5_Msk | GPIO_MODER_MODE6_Msk | GPIO_MODER_MODE7_Msk);
    GPIOA->MODER |=
        (2U << GPIO_MODER_MODE5_Pos) |
        (2U << GPIO_MODER_MODE6_Pos) |
        (2U << GPIO_MODER_MODE7_Pos);

    // AF5 = SPI1
    GPIOA->AFR[0] &= ~(GPIO_AFRL_AFSEL5_Msk | GPIO_AFRL_AFSEL6_Msk | GPIO_AFRL_AFSEL7_Msk);
    GPIOA->AFR[0] |=
        (5U << GPIO_AFRL_AFSEL5_Pos) |
        (5U << GPIO_AFRL_AFSEL6_Pos) |
        (5U << GPIO_AFRL_AFSEL7_Pos);

    // Push-pull, this is standard actually, but somewhere I have to use open-drain I think
    GPIOA->OTYPER &= ~((1U << 5) | (1U << 7) | (1U << 9));


    // No pull-up/down
    GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPD5_Msk | GPIO_PUPDR_PUPD7_Msk);

    // Very high speed (no impact on freq, just driver strength)
    GPIOA->OSPEEDR |=
        (3U << GPIO_OSPEEDR_OSPEED5_Pos) |
        (3U << GPIO_OSPEEDR_OSPEED7_Pos);

    //PA9 as indicatorpin weather it works (Note: PA5 LED on nucleo is used for SCK)
    GPIOA->MODER &= ~GPIO_MODER_MODE9_Msk;
    GPIOA->MODER |= (1U << GPIO_MODER_MODE9_Pos);
}

static void SPI1_Init(void)
{
    // Disable SPI while configuring
    SPI1->CR1 = 0;
    SPI1->CR2 = 0;

    // Master mode
    // Software NSS
    // Baud = APB2CLK / 16 => 1 MHz
    SPI1->CR1 =
          SPI_CR1_MSTR
        | SPI_CR1_SSM
        | SPI_CR1_SSI
        | (3U << SPI_CR1_BR_Pos);

    // 8-bit transfers, mystical FRXTH
    SPI1->CR2 =
          (7U << SPI_CR2_DS_Pos)
        | SPI_CR2_FRXTH;

    // Enable SPI
    SPI1->CR1 |= SPI_CR1_SPE;
}

// static void SPI1_SendByte(uint8_t data)
// {
//     while (!(SPI1->SR & SPI_SR_TXE));


//     //DR is a 16 bit register, to send 8bit one possible
//     //way is to use this super weird cast
//     *(__IO uint8_t *)&SPI1->DR = data;

//     while (SPI1->SR & SPI_SR_BSY);
// }

static uint8_t SPI1_Transfer(uint8_t data)
{
    // w8 till txe empty
    while (!(SPI1->SR & SPI_SR_TXE));

    // write data
    *(__IO uint8_t *)&SPI1->DR = data;

    // wait till rxe empty
    while (!(SPI1->SR & SPI_SR_RXNE));

    // return the received byte
    return *(__IO uint8_t *)&SPI1->DR;
}

int main(void)
{
    GPIO_Init();
    SPI1_Init();

    volatile uint8_t received = 0;

    while (1)
    {
        //SPI1_SendByte(0x55);
        //Connect PA6 and PA7, 
        //if received correctly you get a squarewave signal
        received = SPI1_Transfer(0x55);
        if(received == 0x55){
            GPIOA->ODR ^= (1U << 9);
        }
        else {
            GPIOA->ODR |= (1U << 9);
        }

        for (volatile uint32_t i = 0; i < 10000; i++);
    }
}