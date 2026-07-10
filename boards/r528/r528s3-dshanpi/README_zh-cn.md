# r528s3-dshanpi开发板对 openvela 的支持

\[ [English](README.md) | 简体中文 \]

## 简介

本目录为 **百问网r528s3-dshanpi**（全志 R528 平台）开发板提供 openvela 板级支持（BSP），是 openvela AI 硬件大赛推荐的开发板之一。r528s3-dshanpi 原生搭载 openvela 系统，面向物联网开发者、创客与教育场景。

### 主要硬件规格

- **主控**：全志 R528，双核 Arm Cortex-A7
- **显示**：板载 3.5 寸 SPI 屏+电容触摸
- **音频**：板载麦克风，支持语音交互
- **无线**：Wi-Fi + 蓝牙双模
- **接口**：GPIO、I2C、SPI、UART、RS485\*2、CAN\*2 等

> 完整硬件说明、引脚定义、外设列表与驱动开发手册，请参考百问网官方文档：[百问网r528s3-dshanpi](https://download.100ask.net/project/item7/index.html)。

> 大赛参赛者请基于大赛分支 `dev-ai-contest-2026` 进行开发。

### 配套教程与视频

* openvela快速入门与工程实践_v1.2.pdf，在[百问网r528s3-dshanpi](https://download.100ask.net/project/item7/index.html)的网盘中
* B站视频：https://www.bilibili.com/video/BV19PsAzCEuy

## 目录结构

```
r528s3-dshanpi/
├── Kconfig            # 板级 Kconfig 选项
├── include/           # 板级头文件（board.h、内存映射等）
├── src/               # 板级 bring-up 源码（启动、初始化、LED 等）
├── scripts/           # 链接脚本与构建规则（Make.defs、sdram.ld.S）
├── build/             # 固件打包与 OTA 相关脚本
└── configs/           # 编译配置
    └── nsh            # 基础 NSH 命令行配置
```



## 搭建开发环境

在Ubuntu 20.04或更高版本上，执行如下命令：

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



## 下载源码

执行如下命令下载trunk5.5源码：

```shell
mkdir ~/openvela_trunk5.5
cd ~/openvela_trunk5.5/

repo init -u https://gitee.com/open-vela/manifests.git -b trunk -m tags/trunk-5.5.xml --repo-url=https://mirrors.tuna.tsinghua.edu.cn/git/git-repo/ --git-lfs

repo sync -j 8
```



执行如下命令下载板级开发包：

```shell
cd ~
git clone https://gitee.com/open-vela/vendor_allwinnertech.git
cd vendor_allwinnertech
git checkout dev-ai-contest-2026
```

把板级开发板放入openvela-trunk5.5目录：

```shell
rm -rf ~/openvela_trunk5.5/vendor/allwinnertech/*

cp  -rf ~/vendor_allwinnertech/* ~/openvela_trunk5.5/vendor/allwinnertech/
```



## 打补丁

执行如下命令打补丁：

```shell
cd ~/openvela_trunk5.5

patch -p1 < ~/vendor_allwinnertech/boards/r528/r528s3-dshanpi/dshanpi_for_trunk5.5.patch
```



## 编译

openvela 工程根目录的 `build.sh` 是统一编译入口。以基础 `nsh` 配置为例：

```bash
# distclean：仅在切换配置或修改 menuconfig 后需要清理
./build.sh vendor/allwinnertech/boards/r528/r528s3-dshanpi/configs/nsh/ -j8 distclean

# clean，如果中途中止编译的话，如果要再次编译，建议先clean
./build.sh vendor/allwinnertech/boards/r528/r528s3-dshanpi/configs/nsh/ -j8 clean

# 编译
./build.sh vendor/allwinnertech/boards/r528/r528s3-dshanpi/configs/nsh/ -j 32
```



## 固件打包

执行如下命令打包：

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

Which would you like?: 1  # 选择1，对应r528s3-dshanpi


pack  # 打包命令

# 生成如下文件
# /home/ubuntu/openvela_trunk5.5/vendor/allwinnertech/lichee/out/r528s3/dshanpi_nand/rtos_nuttx_r528s3-dshanpi_uart0_256Mnand.img

```



## 烧录与测试

参考openvela快速入门与工程实践_v1.2.pdf，在[百问网r528s3-dshanpi](https://download.100ask.net/project/item7/index.html)的网盘中。



## 许可协议

本目录下文件遵循各自文件头部声明的许可协议。