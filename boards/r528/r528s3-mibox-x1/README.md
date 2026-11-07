# MIBOX-X1 (R528) Board Support for openvela

\[ English | [简体中文](README_zh-cn.md) \]

## Overview

This directory provides the openvela board support package (BSP) for the **MIBOX-X1** development board (Allwinner R528 platform). It is intended for IoT developers, makers and education scenarios.

Key hardware specifications:

- **SoC**: Allwinner R528, dual-core Arm Cortex-A7
- **Memory**: 128MB DDR3
- **Storage**: 256MB SPI NAND (W25N02KVZEIR)
- **Display**: 480x480 MIPI DSI panel (ST7701S, HD40007C30)
- **Touch**: GT911 capacitive touch controller (I2C)
- **Audio**: NS4150B power amplifier with speaker, MIC1/MIC2 inputs, headphone output
- **Wireless**: RTL8723DS - Wi-Fi over SDIO, Bluetooth over UART
- **Sensors**: ICM20948 9-axis IMU (SPI)
- **Interfaces**: UART, I2C, SPI, PWM, ADC, GPIO, IR TX/RX
- **RTC**: 32.768kHz crystal with coin-cell backup
- **SD card**: shares PF2/PF4 with the UART0 debug port, selected by an on-board DIP switch, so SD and UART0 debug cannot be used at the same time


## Directory Structure

```
r528s3-mibox-x1/
├── Kconfig            # Board-level Kconfig options
├── include/           # Board headers (board.h, memory map, etc.)
├── src/               # Board bring-up sources (boot, init, LEDs, etc.)
├── scripts/           # Linker script and build rules (Make.defs, sdram.ld.S)
├── build/             # Build helper scripts (inherited templates)
└── configs/           # Build configurations
    ├── nsh            # Default: NSH over UART0, no SD card
    ├── nsh_sd         # SD variant: NSH over UART2 + TF card
    └── bootloader     # Bootloader configuration (not used)
```

## Build

The `build.sh` script at the openvela project root is the unified build entry point. Using the default `nsh` configuration as an example:

```bash
# Optional: only needed when switching configs or after menuconfig changes
./build.sh vendor/allwinnertech/boards/r528/r528s3-mibox-x1/configs/nsh -j8 distclean

# Build
./build.sh vendor/allwinnertech/boards/r528/r528s3-mibox-x1/configs/nsh -j8
```

`nsh` is the default configuration (UART0 console, no SD card).
Use `nsh_sd` for the SD variant (UART2 console + TF card).

## Firmware Packaging and Deployment

For the full firmware packaging (`pack`), font setup, application deployment, and flashing flow, refer to the development board documentation:

- [Development Board adaptation cases](https://github.com/open-vela/docs/blob/dev-ai-contest-2026/en/dev_board/Development_Board.md)
- [Chip Porting guide](https://github.com/open-vela/docs/blob/dev-ai-contest-2026/en/chip_porting/porting_guide.md)

## License

Files in this directory follow the license declared in each file's header.
