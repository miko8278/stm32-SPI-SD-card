#include "init_conf.hpp"
#include <cstdio>
#include <cstring>
#include "spidriver.hpp"
#include "sdcarddriver.hpp"
#include "GPIO_HAL.hpp"

int write_errors = 0;
int write_number = 0;

int main()
{
    TIM2_Init();
    delay_ms<100>();
    GPIO_Init();
    using SPI_1 = SpiDriver<SD1_Config::SpiBase>;

    constexpr uint32_t T_OUT1_US = 2 * 1000 * 1000;
    uint32_t start_time = TIM2->CNT;

    //Configure the CS-Pins as Output
    GpioPin<SD1_Config::PortBase, SD1_Config::Pin>::OutputInit(Level::High);

    //Configure the CD-Pin as Input, we need Pullup
    GpioPin<SD1_Config::PortBase, SD1_Config::CD_Pin>::InputInit(Pull::Up);
    
    SPI_1::Init();
    delay_ms<1>();
    SD_InitSPI<SD1_Config>();

    char msg[32];
    constexpr int BUFSIZE = 1024;
    static uint8_t big_buf[BUFSIZE];
    static uint8_t read_buf[BUFSIZE];
    constexpr int LASTBLK = 3000;
    for(uint32_t blk = 0; blk < LASTBLK; blk=blk+2)
    {
        
        int tcnt_ms = TIM2->CNT/1000;
        int msg_len = std::snprintf(msg, sizeof(msg), "TIM2US:%08d\n", tcnt_ms);

        // Fill the complete 512/1024-byte buffer
        for (int j = 0; j < (BUFSIZE / msg_len); j++)
        {
            std::memcpy(big_buf + j * msg_len, msg, msg_len);
        }

        SD_WriteBlocks<SD1_Config>(blk, 2,big_buf);

        
    }
    SD_ReadBlock<SD1_Config>(0, read_buf);

    SD_ReadBlock<SD1_Config>(999, read_buf);
    return 0;
}