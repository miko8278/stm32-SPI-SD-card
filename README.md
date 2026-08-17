# stm32-SPI-SD-card

This project was developed as part of a Bachelor's thesis for the [BHT Berlin](https://www.bht-berlin.de/) and makes use of the following open-source filesystem implementations:

## FatFs

This project uses **FatFs** by ChaN.

- Project: [FatFs – Official Website](https://elm-chan.org/fsw/ff/00index_e.html)
- The LittleFS source code used by this project is included in this repository under `code/spi-sdcardwrite/FatFs`.

## LittleFS

This project also uses **LittleFS** by the littlefs-project.

- Project: [LittleFS – GitHub Repository](https://github.com/littlefs-project/littlefs)
- The LittleFS source code used by this project is included in this repository under `code/spi-sdcardwrite/littlefs`.


## Project

The purpose of this project is to investigate and compare filesystem implementations for SD cards operated in SPI mode, using an **STM32G431** microcontroller.

The project is written in **C++20** and developed using **VS Codium**, **Cortex-Debug**, and the **GNU Arm Embedded Toolchain**.

The filesystem implementations are third-party software and are subject to their respective licenses. Please refer to the corresponding license files for the complete license terms.