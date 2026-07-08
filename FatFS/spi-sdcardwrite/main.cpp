#include "stm32g431xx.h"


#define SD_CS_SELECT()     (GPIOA->BSRR = (1U << (9 + 16))) // PA9 LOW
#define SD_CS_DESELECT()   (GPIOA->BSRR = (1U << 9))        // PA9 HIGH


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
    GPIOA->OTYPER &= ~((1U << 4) | (1U << 5) | (1U << 7) | (1U << 9));


    // No pull-up/down
    GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPD5_Msk | GPIO_PUPDR_PUPD7_Msk);

    // Very high speed (no impact on freq, just driver strength)
    GPIOA->OSPEEDR |=
        (3U << GPIO_OSPEEDR_OSPEED5_Pos) |
        (3U << GPIO_OSPEEDR_OSPEED7_Pos);

    //PA9 as CS-Pin, it's common just to use any PIN for this
    GPIOA->MODER &= ~GPIO_MODER_MODE9_Msk;
    GPIOA->MODER |= (1U << GPIO_MODER_MODE9_Pos);
    GPIOA->OSPEEDR &= ~GPIO_OSPEEDR_OSPEED9_Msk;
    GPIOA->OSPEEDR |= (2U << GPIO_OSPEEDR_OSPEED9_Pos);

    //Note, CS is lowactive, so the DESELECT puts PA9 in HIGH-state
    SD_CS_DESELECT();

    //PA4 as indicatorpin
    GPIOA->MODER &= ~GPIO_MODER_MODE4_Msk;
    GPIOA->MODER |= (1U << GPIO_MODER_MODE4_Pos);
    GPIOA->OSPEEDR &= ~GPIO_OSPEEDR_OSPEED4_Msk;
    GPIOA->OSPEEDR |= (2U << GPIO_OSPEEDR_OSPEED4_Pos);
}

static void SPI1_Init(void)
{
    // Disable SPI while configuring
    SPI1->CR1 = 0;
    SPI1->CR2 = 0;

    // Master mode
    // Software NSS
    // Baud = APB2CLK / 128 => 125KHz
    //We need to send at least 74 cycles of slow data
    //for the sd-card init => 10 8bit messages
    SPI1->CR1 =
          SPI_CR1_MSTR
        | SPI_CR1_SSM
        | SPI_CR1_SSI
        | (6U << SPI_CR1_BR_Pos);

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



uint8_t SD_SendCmd(uint8_t cmd, uint32_t arg, uint8_t crc, uint8_t *response_buf, uint8_t extra_bytes)
{
    uint8_t r1;
    uint16_t timeout = 0;

    //chip select
    SD_CS_SELECT();

    // send cmd
    SPI1_Transfer(cmd | 0x40);
    
    // 4 byte argument
    SPI1_Transfer((uint8_t)(arg >> 24));
    SPI1_Transfer((uint8_t)(arg >> 16));
    SPI1_Transfer((uint8_t)(arg >> 8));
    SPI1_Transfer((uint8_t)arg);
    
    // 1 byte CRC (SPI-Mode ignores it, but still needed even then)
    SPI1_Transfer(crc);

    // wait for first answer byte
    do {
        r1 = SPI1_Transfer(0xFF);
        timeout++;
    } while ((r1 == 0xFF) && (timeout < 255));

    // if there is a buffer, we put the rest inside
    if (response_buf)
    {
        response_buf[0] = r1;
        
        // when r1 is valid, we put the rest inside the buffer
        if (r1 != 0xFF && extra_bytes > 0)
        {
            for (uint8_t i = 1; i <= extra_bytes; i++)
            {
                response_buf[i] = SPI1_Transfer(0xFF);
            }
        }
    }

    SD_CS_DESELECT();

    //Maybe a filler Byte is needed later
    //SPI1_Transfer(0xFF);

    return r1; //always return r1, even when there is more
}


int main(void)
{
    uint8_t cmd0_resp = 0xFF;
    uint8_t cmd8_resp[5];
    uint8_t cmd55_resp = 0xFF;
    uint8_t acmd41_resp = 0xFF;
    uint16_t timeout = 0;
    GPIO_Init();
    SPI1_Init();

    SD_CS_DESELECT();
    for (uint8_t i = 0; i < 10; i++){
        SPI1_Transfer(0xFF);
    }

    while (1)
    {
        timeout = 0;
        //CMD0
        cmd0_resp = SD_SendCmd(0, 0, 0x95, nullptr, 0);
        if(cmd0_resp != 0x01){
            GPIOA->ODR |= (1U << 4); 
            continue;
        }
        //CMD8
        SD_SendCmd(8, 0x000001AA, 0x87, cmd8_resp, 4);
        if (cmd8_resp[0] != 0x01 && cmd8_resp[4] != 0xAA){
            GPIOA->ODR |= (1U << 4); 
            continue;
        }

        //CMD55 + ACMD41
        do {
            // send CMD55, always needed before sending an application command
            cmd55_resp = SD_SendCmd(55, 0, 0xFF, nullptr, 0);

            if (cmd55_resp <= 0x01) 
            {
                // argument: 0x40000000 (HCS-Bits set -> support for SDHC/SDXC)
                acmd41_resp = SD_SendCmd(41, 0x40000000, 0xFF, nullptr, 0);
            }

            timeout++;
            
            //pause
            for (volatile uint32_t i = 0; i < 1000; i++);

        } while ((acmd41_resp != 0x00) && (timeout < 1000)); // either it works or timeout

    
        if (acmd41_resp == 0x00)
        {
            //ACMD41 was successful! SD-Card is ready now!
            GPIOA->ODR ^= (1U << 4); 
        }

        //SD-Card initialised in SPI-Mode


        for (volatile uint32_t i = 0; i < 10000; i++);
    }
}