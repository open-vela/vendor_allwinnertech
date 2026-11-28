# 百变龙 Yuzuki Chameleon（H616）开发板对 openvela 的支持

\[ [English](README.md) | 简体中文 \]

## 简介

本目录为 **Yuzuki Chameleon**（全志 H616 平台）开发板提供 openvela 板级支持（BSP）。

主要硬件规格：

- **主控**：全志 H616，四核 Arm Cortex-A53（SMP，4 核在线）
- **内存**：1 GB DDR3（另有 2 GB/4 GB 版本）
- **存储**：板载 eMMC（实测板为 29 GB），4-bit 总线，中断驱动 CIU + IDMAC 传输
- **控制台**：16550 UART0，115200 8N1，Tx/Rx 可选 DMA
- **接口**：4 路 16550 UART、USB、HDMI、GPIO

本移植从 DRAM 启动 NuttX：板载 boot0/U-Boot（或 FEL 下载链）将
`nuttx.bin` 载入 DRAM 后跳转执行，不依赖任何板载 bootloader 专用位。

> **注意**：本板依赖提交到 openvela nuttx 仓库的全志 H616 芯片层支持
> （`arch/arm64/src/h616`）。编译本板前，请确认 nuttx 源码树中已包含
> H616 芯片移植。

## 目录结构

```
yuzuki-h616/
├── Kconfig            # 板级 Kconfig 选项
├── include/           # 板级头文件（board.h）
├── src/               # 板级 bring-up 源码（初始化、复位、eMMC 插槽等）
├── scripts/           # 链接脚本与构建规则（Make.defs、dramboot.ld）
└── configs/           # 编译配置
    └── nsh            # NSH 命令行配置（SMP 4 核，eMMC 挂载于 /mnt）
```

## 编译

openvela 工程根目录的 `build.sh` 是统一编译入口：

```bash
# 可选：仅在切换配置或修改 menuconfig 后需要清理
./build.sh vendor/allwinnertech/boards/h616/yuzuki-h616/configs/nsh -j8 distclean

# 编译
./build.sh vendor/allwinnertech/boards/h616/yuzuki-h616/configs/nsh -j8
```

编译产物为 `nuttx/nuttx.bin`。

## 固件打包与部署

本板直接从 DRAM 启动 `nuttx.bin`：可将其与全志启动链
（boot0/U-Boot）封装为 `boot.img` 后经 FEL 烧录，或经 FEL/UART
下载路径载入。通用移植与烧录流程请参考：

- [开发板适配案例](https://github.com/open-vela/docs/blob/dev-ai-contest-2026/zh-cn/dev_board/Development_Board.md)
- [芯片移植（Chip Porting）章节](https://github.com/open-vela/docs/blob/dev-ai-contest-2026/zh-cn/chip_porting/porting_guide.md)

## 已验证行为

- SMP：`ps` 显示 CPU0..CPU3 idle 线程，affinity 掩码 0x0f。
- eMMC：`/dev/mmcsd0p1` 上的 FAT 文件系统挂载于 `/mnt`；文件重启后
  保留；顺序写约 15 MB/s，读约 75 MB/s。
- 控制台：UART0，115200 8N1；启动日志可通过 `dmesg` 回看。

## 许可协议

本目录下文件遵循各自文件头部声明的许可协议。
