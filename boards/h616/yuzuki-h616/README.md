# Yuzuki Chameleon (H616) Board Support for openvela

\[ English | [简体中文](README_zh-cn.md) \]

## Overview

This directory provides the openvela board support package (BSP) for the
**Yuzuki Chameleon** development board (Allwinner H616 platform).

Key hardware specifications:

- **SoC**: Allwinner H616, quad-core Arm Cortex-A53 (SMP, 4 cores online)
- **Memory**: 1 GB DDR3 (2 GB/4 GB variants exist)
- **Storage**: on-board eMMC (29 GB on the tested board), 4-bit bus with
  interrupt-driven CIU + IDMAC transfers
- **Console**: 16550 UART0, 115200 8N1, with optional DMA on Tx/Rx
- **Interfaces**: 4x 16550 UART, USB, HDMI, GPIO

The port boots NuttX from DRAM: the board's boot0/U-Boot (or the FEL
boot chain) loads `nuttx.bin` into DRAM and jumps to it, so the port does
not depend on any on-board bootloader bits.

> **Note**: this board depends on the Allwinner H616 chip support
> (`arch/arm64/src/h616`) contributed to the openvela nuttx repository.
> Please make sure your nuttx source tree contains the H616 chip port
> before building this board.

## Directory Structure

```
yuzuki-h616/
├── Kconfig            # Board-level Kconfig options
├── include/           # Board headers (board.h)
├── src/               # Board bring-up sources (init, reset, eMMC slot, etc)
├── scripts/           # Linker script and build rules (Make.defs, dramboot.ld)
└── configs/           # Build configurations
    └── nsh            # NSH shell configuration (SMP 4 cores, eMMC at /mnt)
```

## Build

The `build.sh` script at the openvela project root is the unified build
entry point:

```bash
# Optional: only needed when switching configs or after menuconfig changes
./build.sh vendor/allwinnertech/boards/h616/yuzuki-h616/configs/nsh -j8 distclean

# Build
./build.sh vendor/allwinnertech/boards/h616/yuzuki-h616/configs/nsh -j8
```

The resulting image is `nuttx/nuttx.bin`.

## Firmware Packaging and Deployment

The board boots `nuttx.bin` directly from DRAM. Wrap it with the
Allwinner boot chain (boot0/U-Boot) into a `boot.img` and flash it over
FEL, or load it over the FEL/UART download path. For the general
porting and flashing flow, refer to:

- [Development Board adaptation cases](https://github.com/open-vela/docs/blob/dev-ai-contest-2026/en/dev_board/Development_Board.md)
- [Chip Porting guide](https://github.com/open-vela/docs/blob/dev-ai-contest-2026/en/chip_porting/porting_guide.md)

## Verified behaviour

- SMP: `ps` shows CPU0..CPU3 idle threads, affinity mask 0x0f.
- eMMC: FAT filesystem on `/dev/mmcsd0p1` mounted at `/mnt`; files
  persist across reboots; sequential write ~15 MB/s, read ~75 MB/s.
- Console: UART0, 115200 8N1; boot log also available via `dmesg`.

## License

Files in this directory follow the license declared in each file's header.
