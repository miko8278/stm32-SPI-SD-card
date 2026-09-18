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
    delay_ms<500>();
    GPIO_Init();
    using SPI_1 = SpiDriver<SD1_Config::SpiBase>;

    constexpr uint32_t T_OUT1_US = 2 * 1000 * 1000;
    uint32_t start_time = TIM2->CNT;

    //Configure the CS-Pins as Output
    GpioPin<SD1_Config::PortBase, SD1_Config::Pin>::OutputInit(Level::High);

    //Configure the CD-Pin as Input, we need Pullup
    GpioPin<SD1_Config::PortBase, SD1_Config::CD_Pin>::InputInit(Pull::Up);
    
    SPI_1::Init();

    for(;;)
    {
        int tcnt_ms = TIM2->CNT/1000;
        int msg_len = std::snprintf(msg, sizeof(msg), "%07d\n", tcnt_ms);

        // Fill the complete 1024-byte buffer
        for (int j = 0; j < (1024 / msg_len); j++)
        {
            std::memcpy(big_buf + j * msg_len, msg, msg_len);
        }
        result = f_write(&file,big_buf,sizeof(big_buf),&written);

        if (result != FR_OK || written != sizeof(big_buf))
        {
            write_errors++;
        }

        //I'm not sure if sync is really needed, works without
        if ((write_number % 125) == 0)
        {
            f_sync(&file);
            write_number = 0;
        }
        write_number++;
    }
    f_close(&file);
}
        else
        {
            write_errors++;
        }

        write_number++;
        
    

    return 0;
}