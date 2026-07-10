# openvela Support for the r528s3-dshanpi Development Board

\[ English | [简体中文](README_zh-cn.md) \]

## Overview

This directory provides the openvela board support package (BSP) for the **100ask r528s3-dshanpi** development board based on the Allwinner R528 platform. It is one of the recommended development boards for the openvela AI Hardware Competition. The r528s3-dshanpi ships with openvela preinstalled and targets IoT developers, makers, and educational scenarios.

### Main Hardware Specifications

- **SoC**: Allwinner R528, dual-core Arm Cortex-A7
- **Display**: On-board 3.5-inch SPI display with capacitive touch
- **Audio**: On-board microphone with voice interaction support
- **Wireless**: Dual-mode Wi-Fi + Bluetooth
- **Interfaces**: GPIO, I2C, SPI, UART, RS485*2, CAN*2, and more

> For full hardware documentation, pin definitions, peripheral lists, and driver development manuals, see the official 100ask documentation: [100ask r528s3-dshanpi](https://download.100ask.net/project/item7/index.html).

> Competition participants should develop based on the `dev-ai-contest-2026` branch.

### Tutorials and Videos

* `openvela快速入门与工程实践_v1.2.pdf`, available in the cloud drive linked from [100ask r528s3-dshanpi](https://download.100ask.net/project/item7/index.html)
* Bilibili video: https://www.bilibili.com/video/BV19PsAzCEuy

## Directory Structure

```text
r528s3-dshanpi/
├── Kconfig            # Board-level Kconfig options
├── include/           # Board headers (board.h, memory map, etc.)
├── src/               # Board bring-up sources (boot, initialization, LEDs, etc.)
├── scripts/           # Linker scripts and build rules (Make.defs, sdram.ld.S)
├── build/             # Firmware packaging and OTA-related scripts
└── configs/           # Build configurations
    └── nsh            # Basic NSH command-line configuration
```

## Setting Up the Development Environment

On Ubuntu 20.04 or later, run the following commands:

```shell
sudo apt install \
bison flex gettext texinfo libncurses5-dev libncursesw5-dev xxd \
git gperf automake libtool build-essential gperf genromfs \
libgmp-dev libmpc-dev libmpfr-dev libisl-dev binutils-dev libelf-dev \
libexpat1-dev gcc-multilib g++-multilib picocom u-boot-tools util-linux \
dfu-util libx11-dev libxext-dev net-tools pkgconf unionfs-fuse zlib1g-dev \
libusb-1.0-0-dev libv4l-dev libuv1-dev npm nodejs nasm yasm libdivsufsort-dev \
libc++-dev libc++abi-dev libprotobuf-dev protobuf-compiler protobuf-c-compiler mtools

sudo apt-get install -y dfu-util genromfs gettext gperf mtools nasm net-tools nodejs npm pkgconf protobuf-c-compiler protobuf-compiler yasm

curl https://storage.googleapis.com/git-repo-downloads/repo > repo
chmod +x repo
sudo mv repo /usr/local/bin/

sudo apt install kconfig-frontends

sudo apt install python3 python3-pip python-is-python3

sudo pip3 install kconfiglib pyelftools cxxfilt
```

## Downloading the Source Code

Run the following commands to download the `trunk5.5` source tree:

```shell
mkdir ~/openvela_trunk5.5
cd ~/openvela_trunk5.5/

repo init -u https://gitee.com/open-vela/manifests.git -b trunk -m tags/trunk-5.5.xml --repo-url=https://mirrors.tuna.tsinghua.edu.cn/git/git-repo/ --git-lfs

repo sync -j 8
```

Run the following commands to download the board support package:

```shell
cd ~
git clone https://gitee.com/open-vela/vendor_allwinnertech.git
cd vendor_allwinnertech
git checkout dev-ai-contest-2026
```

Copy the board package into the `openvela-trunk5.5` directory:

```shell
rm -rf ~/openvela_trunk5.5/vendor/allwinnertech/*

cp  -rf ~/vendor_allwinnertech/* ~/openvela_trunk5.5/vendor/allwinnertech/
```

## Applying the Patch

Run the following commands to apply the patch:

```shell
cd ~/openvela_trunk5.5

patch -p1 < ~/vendor_allwinnertech/boards/r528/r528s3-dshanpi/dshanpi_for_trunk5.5.patch
```

## Building

`build.sh` in the root directory of the openvela project is the unified build entry point. The following example uses the basic `nsh` configuration:

```bash
# distclean: only required when switching configurations or after modifying menuconfig
./build.sh vendor/allwinnertech/boards/r528/r528s3-dshanpi/configs/nsh/ -j8 distclean

# clean: If the build is interrupted, we recommend cleaning the build artifacts before rebuilding
./build.sh vendor/allwinnertech/boards/r528/r528s3-dshanpi/configs/nsh/ -j8 clean

# build
./build.sh vendor/allwinnertech/boards/r528/r528s3-dshanpi/configs/nsh/ -j8
```

## Firmware Packaging

Run the following commands to package the firmware:

```shell
cd  ~/openvela_trunk5.5/vendor/allwinnertech/lichee/
source  envsetup.sh
lunch_nuttx 

You're building on Linux

Lunch menu... pick a combo:
     1. r528s3-dshanpi
     2. r528s3-evb4
     3. r528s3-gemini-s1
     4. r528s3-velaevb1

Which would you like?: 1  # select 1, it is for r528s3-dshanpi


pack  

# you can get this file:
# /home/ubuntu/openvela_trunk5.5/vendor/allwinnertech/lichee/out/r528s3/dshanpi_nand/rtos_nuttx_r528s3-dshanpi_uart0_256Mnand.img

```

## Flashing and Testing

Refer to `openvela快速入门与工程实践_v1.2.pdf`, which is available in the cloud drive linked from [100ask r528s3-dshanpi](https://download.100ask.net/project/item7/index.html).



## License

Files in this directory follow the license declared in each file's header.