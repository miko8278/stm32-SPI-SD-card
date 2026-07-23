#pragma once
#include "stm32g431xx.h"

//static class
template<uintptr_t Base>
struct SpiDriver {

    SpiDriver() = delete;
    private:
    static inline SPI_TypeDef* const SPIx = reinterpret_cast<SPI_TypeDef*>(Base);

    //Each SPI-Device needs clock enabled seperately
    static void EnableClock() {
        if constexpr (Base == SPI1_BASE) {
            RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
        }
        else if constexpr (Base == SPI2_BASE) {
            RCC->APB1ENR1 |= RCC_APB1ENR1_SPI2EN;
        }
        else if constexpr (Base == SPI3_BASE) {
            RCC->APB1ENR1 |= RCC_APB1ENR1_SPI3EN;
        }
    }

    public:
    static void Init() {
        EnableClock();
        SPIx->CR1 = 0;
        SPIx->CR2 = 0;

        // Master, Software NSS, Baud = APBCLK / 128
        SPIx->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | (6U << SPI_CR1_BR_Pos);
        
        // 8-Bit
        SPIx->CR2 = (7U << SPI_CR2_DS_Pos) | SPI_CR2_FRXTH;
        
        SPIx->CR1 |= SPI_CR1_SPE;
    }

    //Div just for SetHighSpeed
    enum class SpiDiv : uint32_t {
        Div2   = 0,
        Div4   = 1,
        Div8   = 2,
        Div16  = 3,
        Div32  = 4,
        Div64  = 5,
        Div128 = 6,
        Div256 = 7,
    };

    static void SetHighSpeed(SpiDiv div) {
        SPIx->CR1 &= ~SPI_CR1_SPE;
        SPIx->CR1 &= ~SPI_CR1_BR_Msk;
        SPIx->CR1 |= (static_cast<uint32_t>(div) << SPI_CR1_BR_Pos);
        SPIx->CR1 |= SPI_CR1_SPE;
    }

    static uint8_t Transfer(uint8_t data) {
        // wait till tx empty
        while (!(SPIx->SR & SPI_SR_TXE));

        // force an 8-bit write to the data register.
        *(__IO uint8_t *)&SPIx->DR = data;

        // wait until a received byte is available.
        while (!(SPIx->SR & SPI_SR_RXNE));

        return *(__IO uint8_t *)&SPIx->DR;
    }
};