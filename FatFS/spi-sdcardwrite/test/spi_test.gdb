define spi_test
    printf "\n\n SD Card Test:\n\n"

    #SPI1
    if sdtest.cur_spi == 1
        printf "TESTING SPI1 PERIPHERAL \n"

        #CARD DETECT
        if sdtest.carddetect == 1
            printf "SPI1: CARD NOT DETECTED [FAILED]"
        end
        if sdtest.carddetect == 0
            printf "SPI1: CARD DETECTED [OK]"
        end

        #SD CARD INIT
        if sdtest.initsd == 0x00
            printf "SPI1: SD CARD INIT [OK]\n"
        end
        if sdtest.initsd == 0x02
            printf "SPI1: SD CARD INIT [FAILED: 0x02] => CMD0 FAILED\n"
        end
        if sdtest.initsd == 0x04
            printf "SPI1: SD CARD INIT [FAILED: 0x04] => CMD8 FAILED\n"
        end
        if sdtest.initsd == 0x08
            printf "SPI1: SD CARD INIT [FAILED: 0x08] => TIMEOUT: CMD55 + ACMD41 LOOP FAILED\n"
        end

        #SD CARD WRITE

        if sdtest.writeblock == 0x00
            printf "SPI1: SD CARD WRITE SINGLE BLOCK [OK]\n"
        end
        if sdtest.writeblock == 0x01
            printf "SPI1: SD CARD WRITE SINGLE BLOCK [FAILED: 0x01] => CMD24 FAILED \n"
        end
        if sdtest.writeblock == 0x02
            printf "SPI1: SD CARD WRITE SINGLE BLOCK [FAILED: 0x02] => DATA TRANSFER FAILED \n"
        end
        if sdtest.writeblock == 0x04
            printf "SPI1: SD CARD WRITE SINGLE BLOCK [FAILED: 0x04] => TIMEOUT WHILE WRITING \n"
        end
        
        #SD CARD READ

        if sdtest.readblock == 0x00
            printf "SPI1: SD CARD READ SINGLE BLOCK [OK]\n"
        end
        if sdtest.readblock == 0x01
            printf "SPI1: SD CARD READ SINGLE BLOCK [FAILED: 0x01] => CMD17 FAILED \n"
        end
        if sdtest.readblock == 0x02
            printf "SPI1: SD CARD WRITE SINGLE BLOCK [FAILED: 0x02] => DATA TRANSFER FAILED, NEVER GOT 0xFE \n"
        end


    end

end

break test_complete
commands
    spi_test
end
