# MIBOX-X1 (R528) 开发板对 openvela 的支持

\[ [English](README.md) | 简体中文 \]

## 简介

本目录为 **MIBOX-X1**（全志 R528 平台）开发板提供 openvela 板级支持（BSP），面向物联网开发者、创客与教育场景。

主要硬件规格：

- **主控**：全志 R528，双核 Arm Cortex-A7
- **内存**：128MB DDR3
- **存储**：256MB SPI NAND（W25N02KVZEIR）
- **显示**：480x480 MIPI DSI 屏（ST7701S，HD40007C30）
- **触摸**：GT911 电容触摸控制器（I2C）
- **音频**：NS4150B 功放 + 喇叭，MIC1/MIC2 输入，耳机输出
- **无线**：RTL8723DS —— WiFi 走 SDIO，蓝牙走 UART
- **传感器**：ICM20948 九轴 IMU（SPI）
- **接口**：UART、I2C、SPI、PWM、ADC、GPIO、红外收发
- **RTC**：32.768kHz 晶振 + 纽扣电池备份
- **SD 卡**：与 UART0 调试口共用 PF2/PF4，由板载拨码开关选择，因此 SD 卡与 UART0 调试不能同时使用


## 目录结构

```
r528s3-mibox-x1/
├── Kconfig            # 板级 Kconfig 选项
├── include/           # 板级头文件（board.h、内存映射等）
├── src/               # 板级初始化代码（boot、init、LED 等）
├── scripts/           # 链接脚本与构建规则（Make.defs、sdram.ld.S）
├── build/             # 构建辅助脚本（原厂模板）
└── configs/           # 构建配置
    ├── nsh            # 默认配置：UART0 控制台，无 SD 卡
    ├── nsh_sd         # SD 变体：UART2 控制台 + TF 卡
    └── bootloader     # Bootloader 配置（本板未使用）
```

## 编译

openvela 项目根目录下的 `build.sh` 是统一的编译入口。以默认 `nsh` 配置为例：

```bash
# 可选：切换配置或修改过 menuconfig 后才需要执行
./build.sh vendor/allwinnertech/boards/r528/r528s3-mibox-x1/configs/nsh -j8 distclean

# 编译
./build.sh vendor/allwinnertech/boards/r528/r528s3-mibox-x1/configs/nsh -j8
```

`nsh` 是默认配置（UART0 控制台，无 SD 卡）。
需要 SD 卡功能时，使用 `nsh_sd`（UART2 控制台 + TF 卡）。

## 固件打包与烧录

完整的固件打包（`pack`）、字体配置、应用部署与烧录流程，请参考开发板文档：

- [开发板适配案例](https://github.com/open-vela/docs/blob/dev-ai-contest-2026/zh-cn/dev_board/Development_Board.md)
- [芯片移植指南](https://github.com/open-vela/docs/blob/dev-ai-contest-2026/zh-cn/chip_porting/porting_guide.md)

## 许可证

本目录下的文件遵循各文件头部声明的许可证。
