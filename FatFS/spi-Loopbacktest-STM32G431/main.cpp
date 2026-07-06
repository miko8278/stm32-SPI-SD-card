#include "stm32g431xx.h"

static void SPI1_GPIO_Init(void)
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

    // Push-pull
    GPIOA->OTYPER &= ~((1U << 5) | (1U << 7));

    // No pull-up/down
    GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPD5_Msk | GPIO_PUPDR_PUPD7_Msk);

    // Very high speed (no impact on freq, just driver strength)
    GPIOA->OSPEEDR |=
        (3U << GPIO_OSPEEDR_OSPEED5_Pos) |
        (3U << GPIO_OSPEEDR_OSPEED7_Pos);
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

static void SPI1_SendByte(uint8_t data)
{
    while (!(SPI1->SR & SPI_SR_TXE));


    //DR is a 16 bit register, to send 8bit one possible
    //way is to use this super weird cast
    *(__IO uint8_t *)&SPI1->DR = data;

    while (SPI1->SR & SPI_SR_BSY);
}

int main(void)
{
    SPI1_GPIO_Init();
    SPI1_Init();

    while (1)
    {
        SPI1_SendByte(0x55);

        for (volatile uint32_t i = 0; i < 100000; i++);
    }
}