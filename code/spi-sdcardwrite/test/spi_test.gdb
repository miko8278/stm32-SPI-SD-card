define spi_test
    printf "\n\n SD Card Test:\n\n"

    ###SPI1###
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
        if sdtest.initsd == SD_INIT_OK
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


        #SD CARD READ MULTI

        if sdtest.readblockmulti == 0x00
            printf "SPI1: SD CARD READ MULTIPLE BLOCK [OK]\n"
        end
        if sdtest.readblockmulti == 0x01
            printf "SPI1: SD CARD READ MULTIPLE BLOCK [FAILED: 0x01] => CMD18 FAILED \n"
        end
        if sdtest.readblockmulti == 0x02
            printf "SPI1: SD CARD READ MULTIPLE BLOCK [FAILED: 0x02] => DATA TRANSFER FAILED, NEVER GOT 0xFE \n"
        end
        if sdtest.readblockmulti == 0x04
            printf "SPI1: SD CARD READ MULTIPLE BLOCK [FAILED: 0x04] => CMD12 STOP TRANSMISSION FAILED \n"
        end

        #SD CARD WRITE MULTI

        if sdtest.writeblockmulti == 0x00
            printf "SPI1: SD CARD WRITE MULTIPLE BLOCK [OK]\n"
        end
        if sdtest.writeblockmulti == 0x01
            printf "SPI1: SD CARD WRITE MULTIPLE BLOCK [FAILED: 0x01] => CMD25 FAILED \n"
        end
        if sdtest.writeblockmulti == 0x02
            printf "SPI1: SD CARD WRITE MULTIPLE BLOCK [FAILED: 0x02] => AT LEAST ONE BLOCK OF DATA TRANSFER FAILED \n"
        end
        if sdtest.writeblockmulti == 0x04
            printf "SPI1: SD CARD WRITE MULTIPLE BLOCK [FAILED: 0x04] => TIMEOUT AFTER ACCEPTING A BLOCK \n"
        end
        if sdtest.writeblockmulti == 0x08
            printf "SPI1: SD CARD WRITE MULTIPLE BLOCK [FAILED: 0x08] => TIMEOUT AFTER CMD25 TERMINATION \n"
        end

        #READBACK: CHECK IF SOMETHING REALLY WAS WRITTEN
        #SINGLE
        if sdtest.sameblock == 0
            printf "SPI1: READ SINGLE BLOCK SAME AS WRITTEN [OK]"
        else 
            printf "SPI1: READ SINGLE BLOCK NOT SAME AS WRITTEN [FAILED] => %d FAILED BYTES", sdtest.sameblock
        end

        #MULTI
        if sdtest.sameblockmulti == 0
            printf "SPI1: READ MULTIPLE BLOCK SAME AS WRITTEN [OK]"
        else 
            printf "SPI1: READ MULTIPLE BLOCK NOT SAME AS WRITTEN [FAILED] => %d FAILED BYTES", sdtest.sameblockmulti
        end

        #And here I noticed that gdb indeed can use the defined enums... maybe even enum classes?
        #GET SECTOR COUNT: CMD9
        if sdtest.csd == SD_CSD_OK
            printf "SPI1: GOT CSD [OK]"
            printf "SPI1: CARD SIZE IN MB: %d", sdtest.csizeMB
        end
        if sdtest.csd == SD_CSD_ERROR_CMD9
            printf "SPI1: GET CSD [FAILED] => CMD9 FAILED"
        end
        if sdtest.csd == SD_CSD_ERROR_DATATRANS
            printf "SPI1: GET CSD [FAILED] => ERROR DURING DATA TRANSMISSION"
        end
        if sdtest.csd == SD_CSD_ERROR_WRONGBUFSIZE
            printf "SPI1: GET CSD [FAILED] => BUFFER SHOULD BE (AT LEAST) 16 BYTES"
        end


        #FATFS + LITTLEFS TEST
        if sdtest.initsd == SD_INIT_OK
            #####FATFS####
            if fat_test.mount == FR_OK
                printf "SPI1, FatFS: MOUNT [OK]"
            else
                printf "SPI1, FatFS: MOUNT [FAILED]"
            end
            if fat_test.open == FR_OK
                printf "SPI1, FatFS: FOPEN [OK]"
            else
                printf "SPI1, FatFS: FOPEN [FAILED]"
            end
            if fat_test.write == FR_OK
                printf "SPI1, FatFS: FWRITE [OK]"
            else
                printf "SPI1, FatFS: FWRITE [FAILED]"
            end
            if fat_test.close == FR_OK
                printf "SPI1, FatFS: CLOSE [OK]"
            else
                printf "SPI1, FatFS: CLOSE [FAILED]"
            end


            #####LITTLEFS####
            if lfs_test.init_res == LFSINIT_OK
                printf "SPI1, littlefs: INIT [OK]"
            end
            if lfs_test.init_res == LFSINIT_SPIINIT_FAIL
                printf "SPI1, littlefs: SPI INIT [FAILED]"
            end
            if lfs_test.init_res == LFSINIT_GETCSD_FAIL
                printf "SPI1, littlefs: SD GETCSD [FAILED]"
            end

            if lfs_test.format_res == 0
                printf "SPI1, littlefs: FORMAT [OK]"
            else
                if lfs_test.format_res == 0xFE
                    printf "SPI1, littlefs: FORMAT [NONE]"
                else
                    printf "SPI1, littlefs: FORMAT [FAILED]"
                end
            end

            if lfs_test.mount_res == 0
                printf "SPI1, littlefs: MOUNT [OK]"
            else
                printf "SPI1, littlefs: MOUNT [FAILED]"
            end

            if lfs_test.open_res == 0
                printf "SPI1, littlefs: OPEN [OK]"
            else
                printf "SPI1, littlefs: OPEN [FAILED]"
            end

            if lfs_test.write_res == -1
                printf "SPI1, littlefs: WRITE [FAILED]"
            else
                printf "SPI1, littlefs: WROTE %d BYTES", lfs_test.write_res
            end

            if lfs_test.rewind_res == 0
                printf "SPI1, littlefs: REWIND [OK]"
            else
                printf "SPI1, littlefs: REWIND [FAILED]"
            end

            if lfs_test.read_res == -1
                printf "SPI1, littlefs: READ [FAILED]"
            else
                printf "SPI1, littlefs: READ %d BYTES", lfs_test.write_res
            end

            if lfs_test.readwrite_res == -1
                printf "SPI1, littlefs: READBUFFER IS SAME AS WRITEBUFFER [OK]"
            else
                printf "SPI1, littlefs: READBUFFER NOT SAME AS WRITEBUFFER, FAILED INDEX: %d [FAILED] ", lfs_test.readwrite_res
            end

            if lfs_test.close_res == 0
                printf "SPI1, littlefs: CLOSE [OK]"
            else
                printf "SPI1, littlefs: CLOSE [FAILED]"
            end

            if lfs_test.unmount_res == 0
                printf "SPI1, littlefs: UNMOUNT [OK]"
            else
                printf "SPI1, littlefs: UNMOUNT [FAILED]"
            end

        end

    end



    ###SPI3###
    if sdtest.cur_spi == 3
        printf "TESTING SPI3 PERIPHERAL \n"

        #CARD DETECT
        if sdtest.carddetect == 1
            printf "SPI3: CARD NOT DETECTED [FAILED]"
        end
        if sdtest.carddetect == 0
            printf "SPI3: CARD DETECTED [OK]"
        end

        #SD CARD INIT
        if sdtest.initsd == 0x00
            printf "SPI3: SD CARD INIT [OK]\n"
        end
        if sdtest.initsd == 0x02
            printf "SPI3: SD CARD INIT [FAILED: 0x02] => CMD0 FAILED\n"
        end
        if sdtest.initsd == 0x04
            printf "SPI3: SD CARD INIT [FAILED: 0x04] => CMD8 FAILED\n"
        end
        if sdtest.initsd == 0x08
            printf "SPI3: SD CARD INIT [FAILED: 0x08] => TIMEOUT: CMD55 + ACMD41 LOOP FAILED\n"
        end

        #SD CARD WRITE

        if sdtest.writeblock == 0x00
            printf "SPI3: SD CARD WRITE SINGLE BLOCK [OK]\n"
        end
        if sdtest.writeblock == 0x01
            printf "SPI3: SD CARD WRITE SINGLE BLOCK [FAILED: 0x01] => CMD24 FAILED \n"
        end
        if sdtest.writeblock == 0x02
            printf "SPI3: SD CARD WRITE SINGLE BLOCK [FAILED: 0x02] => DATA TRANSFER FAILED \n"
        end
        if sdtest.writeblock == 0x04
            printf "SPI3: SD CARD WRITE SINGLE BLOCK [FAILED: 0x04] => TIMEOUT WHILE WRITING \n"
        end
        
        #SD CARD READ

        if sdtest.readblock == 0x00
            printf "SPI3: SD CARD READ SINGLE BLOCK [OK]\n"
        end
        if sdtest.readblock == 0x01
            printf "SPI3: SD CARD READ SINGLE BLOCK [FAILED: 0x01] => CMD17 FAILED \n"
        end
        if sdtest.readblock == 0x02
            printf "SPI3: SD CARD WRITE SINGLE BLOCK [FAILED: 0x02] => DATA TRANSFER FAILED, NEVER GOT 0xFE \n"
        end


        #SD CARD READ MULTI

        if sdtest.readblockmulti == 0x00
            printf "SPI3: SD CARD READ MULTIPLE BLOCK [OK]\n"
        end
        if sdtest.readblockmulti == 0x01
            printf "SPI3: SD CARD READ MULTIPLE BLOCK [FAILED: 0x01] => CMD18 FAILED \n"
        end
        if sdtest.readblockmulti == 0x02
            printf "SPI3: SD CARD READ MULTIPLE BLOCK [FAILED: 0x02] => DATA TRANSFER FAILED, NEVER GOT 0xFE \n"
        end
        if sdtest.readblockmulti == 0x04
            printf "SPI3: SD CARD READ MULTIPLE BLOCK [FAILED: 0x04] => CMD12 STOP TRANSMISSION FAILED \n"
        end

        #SD CARD WRITE MULTI

        if sdtest.writeblockmulti == 0x00
            printf "SPI3: SD CARD WRITE MULTIPLE BLOCK [OK]\n"
        end
        if sdtest.writeblockmulti == 0x01
            printf "SPI3: SD CARD WRITE MULTIPLE BLOCK [FAILED: 0x01] => CMD25 FAILED \n"
        end
        if sdtest.writeblockmulti == 0x02
            printf "SPI3: SD CARD WRITE MULTIPLE BLOCK [FAILED: 0x02] => AT LEAST ONE BLOCK OF DATA TRANSFER FAILED \n"
        end
        if sdtest.writeblockmulti == 0x04
            printf "SPI3: SD CARD WRITE MULTIPLE BLOCK [FAILED: 0x04] => TIMEOUT AFTER ACCEPTING A BLOCK \n"
        end
        if sdtest.writeblockmulti == 0x08
            printf "SPI3: SD CARD WRITE MULTIPLE BLOCK [FAILED: 0x08] => TIMEOUT AFTER CMD25 TERMINATION \n"
        end

        #READBACK: CHECK IF SOMETHING REALLY WAS WRITTEN
        #SINGLE
        if sdtest.sameblock == 0
            printf "SPI3: READ SINGLE BLOCK SAME AS WRITTEN [OK]"
        else 
            printf "SPI3: READ SINGLE BLOCK NOT SAME AS WRITTEN [FAILED] => %d FAILED BYTES", sdtest.sameblock
        end

        #MULTI
        if sdtest.sameblockmulti == 0
            printf "SPI3: READ MULTIPLE BLOCK SAME AS WRITTEN [OK]"
        else 
            printf "SPI3: READ MULTIPLE BLOCK NOT SAME AS WRITTEN [FAILED] => %d FAILED BYTES", sdtest.sameblockmulti
        end


        #GET SECTOR COUNT: CMD9
        if sdtest.csd == SD_CSD_OK
            printf "SPI3: GOT CSD [OK]"
            printf "SPI3: CARD SIZE IN MB: %d", sdtest.csizeMB
        end
        if sdtest.csd == SD_CSD_ERROR_CMD9
            printf "SPI3: GET CSD [FAILED] => CMD9 FAILED"
        end
        if sdtest.csd == SD_CSD_ERROR_DATATRANS
            printf "SPI3: GET CSD [FAILED] => ERROR DURING DATA TRANSMISSION"
        end
        if sdtest.csd == SD_CSD_ERROR_WRONGBUFSIZE
            printf "SPI3: GET CSD [FAILED] => BUFFER SHOULD BE (AT LEAST) 16 BYTES"
        end
    end



end

#
#break debug_gdb_print
#commands
#    printf "%s", str
#    continue
#end

break test_complete
commands
    spi_test
    continue
end

break tests_done