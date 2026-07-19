#include "stm32g431xx.h"

//old def
#define SD_CS_SELECT()     (GPIOA->BSRR = (1U << (9 + 16))) // PA9 LOW
#define SD_CS_DESELECT()   (GPIOA->BSRR = (1U << 9))        // PA9 HIGH


template<uintptr_t PortBase, uint32_t Pin>
struct ChipSelect {
private:
    static inline GPIO_TypeDef* const port = reinterpret_cast<GPIO_TypeDef*>(PortBase);
public:
    ChipSelect() {
        port->BSRR = (1U << (Pin + 16));
    }

    ~ChipSelect() {
        port->BSRR = (1U << Pin);
    }
};


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
    //RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

    //SPI1 chosen PINs
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

    //SPI2 chosen PINs
    // PB3 = SCK PB4 = MISO PB5 = MOSI -> Alternate Function mode
    GPIOB->MODER &= ~(GPIO_MODER_MODE3_Msk | GPIO_MODER_MODE4_Msk | GPIO_MODER_MODE5_Msk);
    GPIOB->MODER |=
        (2U << GPIO_MODER_MODE3_Pos) |
        (2U << GPIO_MODER_MODE4_Pos) |
        (2U << GPIO_MODER_MODE5_Pos);

    // PB3/PB4/PB5 -> Alternate Function mode
    GPIOB->MODER &= ~(GPIO_MODER_MODE3_Msk |
                    GPIO_MODER_MODE4_Msk |
                    GPIO_MODER_MODE5_Msk);

    GPIOB->MODER |=
        (2U << GPIO_MODER_MODE3_Pos) |
        (2U << GPIO_MODER_MODE4_Pos) |
        (2U << GPIO_MODER_MODE5_Pos);


    // Push-pull, this is standard actually, but somewhere I have to use open-drain I think
    GPIOA->OTYPER &= ~((1U << 4) | (1U << 5) | (1U << 7) | (1U << 9));


    // No pull-up/down
    GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPD5_Msk | GPIO_PUPDR_PUPD7_Msk);

    // Very high speed (no impact on freq, just driver strength)
    GPIOA->OSPEEDR |=
        (3U << GPIO_OSPEEDR_OSPEED5_Pos) |
        (3U << GPIO_OSPEEDR_OSPEED7_Pos);

}

struct SD1_Config {
    static constexpr uintptr_t SpiBase  = SPI1_BASE;
    static constexpr uintptr_t PortBase = GPIOA_BASE;
    static constexpr uint32_t  Pin      = 8;
};

struct SD2_Config {
    static constexpr uintptr_t SpiBase  = SPI2_BASE;
    static constexpr uintptr_t PortBase = GPIOB_BASE;
    static constexpr uint32_t  Pin      = 11;
};

struct SD3_Config {
    static constexpr uintptr_t SpiBase  = SPI3_BASE;
    static constexpr uintptr_t PortBase = GPIOB_BASE;
    static constexpr uint32_t  Pin      = 10;
};

//template<uintptr_t Base, uintptr_t PortBase, uint32_t Pin>
template<typename Config>
uint8_t SD_SendCmd(uint8_t cmd, uint32_t arg, uint8_t crc, uint8_t *response_buf, uint8_t extra_bytes)
{

    //This gets automatically destructed when leaving the function at any point
    ChipSelect<Config::PortBase, Config::Pin> chipselect_tmp;


    uint8_t r1;
    uint16_t timeout = 0;

    //chip select
    //SD_CS_SELECT();

    // send cmd
    SpiDriver<Config::SpiBase>::Transfer(cmd | 0x40);
    
    // 4 byte argument
    SpiDriver<Config::SpiBase>::Transfer((uint8_t)(arg >> 24));
    SpiDriver<Config::SpiBase>::Transfer((uint8_t)(arg >> 16));
    SpiDriver<Config::SpiBase>::Transfer((uint8_t)(arg >> 8));
    SpiDriver<Config::SpiBase>::Transfer((uint8_t)arg);
    
    // 1 byte CRC (SPI-Mode ignores it, but still needed even then)
    SpiDriver<Config::SpiBase>::Transfer(crc);

    // wait for first answer byte
    do {
        r1 = SpiDriver<Config::SpiBase>::Transfer(0xFF);
        timeout++;
    } while ((r1 == 0xFF) && (timeout < 255));

    // if there is a buffer, we put the rest inside
    if (response_buf) {
        response_buf[0] = r1;
        
        // when r1 is valid, we put the rest inside the buffer
        if (r1 != 0xFF && extra_bytes > 0) {
            for (uint8_t i = 1; i <= extra_bytes; i++) {
                response_buf[i] = SpiDriver<Config::SpiBase>::Transfer(0xFF);
            }
        }
    }

    //SD_CS_DESELECT();

    //Maybe a filler Byte is needed later
    //SpiDriver<Base>::Transfer(0xFF);

    return r1; //always return r1, even when there is more
}


//template<uintptr_t Base, uintptr_t PortBase, uint32_t Pin>
template<typename Config>
uint8_t SD_ReadBlock(uint32_t block_addr, uint8_t *buffer)
{
    //This gets automatically destructed when leaving the function at any point
    ChipSelect<Config::PortBase,Config::Pin> chipselect_tmp;

    uint8_t r1;
    uint16_t timeout = 0;

    //SD_CS_SELECT();

    // send CMD17
    // CRC not important anymore => 0xFF
    SpiDriver<Config::SpiBase>::Transfer(17 | 0x40);
    SpiDriver<Config::SpiBase>::Transfer((uint8_t)(block_addr >> 24));
    SpiDriver<Config::SpiBase>::Transfer((uint8_t)(block_addr >> 16));
    SpiDriver<Config::SpiBase>::Transfer((uint8_t)(block_addr >> 8));
    SpiDriver<Config::SpiBase>::Transfer((uint8_t)block_addr);
    SpiDriver<Config::SpiBase>::Transfer(0xFF); 

    // waiting for answer... it has to be 0x00
    timeout = 0;
    do {
        r1 = SpiDriver<Config::SpiBase>::Transfer(0xFF);
        timeout++;
    } while ((r1 == 0xFF) && (timeout < 1000));

    if (r1 != 0x00) {
        //SD_CS_DESELECT();
        return 0x01; // did not accept command
    }

    // wait for start 0xFE
    timeout = 0;
    do {
        r1 = SpiDriver<Config::SpiBase>::Transfer(0xFF);
        timeout++;
    } while ((r1 != 0xFE) && (timeout < 0xFFFF));

    if (r1 != 0xFE) {
        //SD_CS_DESELECT();
        return 0x02; // timeout-error waiting for 0xFE
    }

    // read 512 byte blockdata
    for (uint16_t i = 0; i < 512; i++) {
        buffer[i] = SpiDriver<Config::SpiBase>::Transfer(0xFF);
    }

    // read 2 byte CRC, do nothing with it
    SpiDriver<Config::SpiBase>::Transfer(0xFF);
    SpiDriver<Config::SpiBase>::Transfer(0xFF);


    //SD_CS_DESELECT();
    SpiDriver<Config::SpiBase>::Transfer(0xFF); // extra cycle

    return 0x00; // success
}

//template<uintptr_t Base, uintptr_t PortBase, uint32_t Pin>
template<typename Config>
uint8_t SD_WriteBlock(uint32_t block_addr, const uint8_t *buffer)
{
    //This gets automatically destructed when leaving the function at any point
    ChipSelect<Config::PortBase, Config::Pin> chipselect_tmp;
    uint8_t r1;
    uint16_t timeout = 0;

    //SD_CS_SELECT();

    // send CMD24
    SpiDriver<Config::SpiBase>::Transfer(24 | 0x40);
    SpiDriver<Config::SpiBase>::Transfer((uint8_t)(block_addr >> 24));
    SpiDriver<Config::SpiBase>::Transfer((uint8_t)(block_addr >> 16));
    SpiDriver<Config::SpiBase>::Transfer((uint8_t)(block_addr >> 8));
    SpiDriver<Config::SpiBase>::Transfer((uint8_t)block_addr);
    SpiDriver<Config::SpiBase>::Transfer(0xFF); // CRC doesn't matter


    // wait for answer 0x00
    timeout = 0;
    do {
        r1 = SpiDriver<Config::SpiBase>::Transfer(0xFF);
        timeout++;
    } while ((r1 == 0xFF) && (timeout < 1000));

    if (r1 != 0x00) {
        //SD_CS_DESELECT();
        return 0x01; // Error: cmd not accepted
    }

    // one cycle for sync
    SpiDriver<Config::SpiBase>::Transfer(0xFF);

    // send start sign for single-block-write
    SpiDriver<Config::SpiBase>::Transfer(0xFE);

    // send 512 byte blockdata
    for (uint16_t i = 0; i < 512; i++) {
        SpiDriver<Config::SpiBase>::Transfer(buffer[i]);
    }

    // 2 byte dummy crc
    SpiDriver<Config::SpiBase>::Transfer(0xFF);
    SpiDriver<Config::SpiBase>::Transfer(0xFF);

    // data response from card
    r1 = SpiDriver<Config::SpiBase>::Transfer(0xFF);
    if ((r1 & 0x1F) != 0x05) {
        // data not accepted
        //SD_CS_DESELECT();
        return 0x02; 
    }

    // wait as long as SD-card is writing
    timeout = 0;
    do {
        r1 = SpiDriver<Config::SpiBase>::Transfer(0xFF);
        timeout++;
    } while ((r1 == 0x00) && (timeout < 0xFFFF));

    //SD_CS_DESELECT();
    SpiDriver<Config::SpiBase>::Transfer(0xFF); // // extra cycle

    if (r1 == 0x00) {
        return 0x03; // timeout while writing
    }

    return 0x00; // success
}

template<uintptr_t PortBase, uint32_t Pin>
struct GpioPin
{
private:
    static inline GPIO_TypeDef* const GPIOx =
        reinterpret_cast<GPIO_TypeDef*>(PortBase);

public:
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

    static void OutputHighInit()
    {
        OutputLowInit();
        // Default HIGH (CS inactive)
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

};


uint8_t sd_block_buffer[512]; 
uint8_t sd_block_buffer2[512];

int main()
{
    uint8_t cmd0_resp = 0xFF;
    uint8_t cmd8_resp[5];
    uint8_t cmd55_resp = 0xFF;
    uint8_t acmd41_resp = 0xFF;
    uint16_t timeout = 0;
    using SPI_1 = SpiDriver<SPI1_BASE>;
    using SPI_2 = SpiDriver<SPI2_BASE>;

    //Configure the alternate functions
    GPIO_Init();

    //Configure the CS-Pins as Output
    GpioPin<GPIOA_BASE, SD1_Config::Pin>::OutputHighInit();

    //Configure as indicatorpin
    GpioPin<GPIOA_BASE, 4>::OutputLowInit();

    //Configure the registers for SPI
    SpiDriver<SPI1_BASE>::Init();

    //Ssend some dummydata before starting
    for (uint8_t i = 0; i < 10; i++){
        SPI_1::Transfer(0xFF);
    }

    while (1)
    {
        timeout = 0;
        //CMD0
        cmd0_resp = SD_SendCmd<SD1_Config>(0, 0, 0x95, nullptr, 0);
        if(cmd0_resp != 0x01){
            GPIOA->ODR |= (1U << 4); 
            continue;
        }
        //CMD8
        SD_SendCmd<SD1_Config>(8, 0x000001AA, 0x87, cmd8_resp, 4);
        if (cmd8_resp[0] != 0x01 && cmd8_resp[4] != 0xAA){
            GPIOA->ODR |= (1U << 4); 
            continue;
        }

        //CMD55 + ACMD41
        do {
            // send CMD55, always needed before sending an application command
            cmd55_resp = SD_SendCmd<SD1_Config>(55, 0, 0xFF, nullptr, 0);

            if (cmd55_resp <= 0x01) 
            {
                // argument: 0x40000000 (HCS-Bits set -> support for SDHC/SDXC, Blockadressing instead of Byteadressing)
                acmd41_resp = SD_SendCmd<SD1_Config>(41, 0x40000000, 0xFF, nullptr, 0);
            }

            timeout++;
            
            //pause
            for (volatile uint32_t i = 0; i < 1000; i++);

        } while ((acmd41_resp != 0x00) && (timeout < 1000)); // either it works or timeout

    

        //SD-Card initialised in SPI-Mode

        //Set to higher speed
        SPI_1::SetHighSpeed(SPI_1::SpiDiv::Div16);

        for(uint16_t i = 0; i < 512; i++) {
            sd_block_buffer[i] = (uint8_t)i; 
        }

        uint8_t write_result = SD_WriteBlock<SD1_Config>(10, sd_block_buffer);

        if (write_result == 0x00) {
            // read block again to verify
            uint8_t read_back_result = SD_ReadBlock<SD1_Config>(10, sd_block_buffer2);
            
            if (read_back_result == 0x00) {
                __BKPT(0); // debugger softbreakpoint
            }
        }


        for (volatile uint32_t i = 0; i < 10000; i++);
    }
}