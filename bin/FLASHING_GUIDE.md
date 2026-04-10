\# IMP-PLAYER 烧录指南 | Flashing Guide



\## 中文说明



\### 一、所需工具与文件

1\. 烧录工具：flash\_download\_tool（乐鑫官方）

2\. 3 个固件文件

&#x20;  - bootloader.bin

&#x20;  - partitions.bin

&#x20;  - firmware.bin

3\. 硬件：ESP32-WROOM-32（4MB Flash，无PSRAM）、USB 数据线

4\. 电脑系统：Windows



\---



\### 二、工具下载（官方安全）

https://www.espressif.com/zh-CN/support/download/other-tools



下载后解压到任意文件夹，直接运行 `flash\_download\_tool.exe`，无需安装。



\---



\### 三、烧录步骤（零基础可操作）

1\. 打开工具，芯片选择 \*\*ESP32\*\*，点击 OK

2\. 按以下配置添加文件并勾选：

&#x20;  - bootloader.bin    地址：0x1000

&#x20;  - partitions.bin    地址：0x8000

&#x20;  - firmware.bin      地址：0x10000

3\. 底部参数设置

&#x20;  - SPI Speed：40MHz

&#x20;  - SPI Mode：DIO

&#x20;  - Flash Size：4MB

4\. 选择正确的 COM 端口

5\. 点击 \*\*START\*\* 开始烧录

6\. 显示 \*\*FINISH\*\* 即为烧录成功



\---



\### 四、常见问题

1\. 烧录失败：按住开发板 \*\*BOOT\*\* 键再点击 START，进度条走动后松开

2\. 找不到 COM 口：安装 CH340/CP2102 驱动，更换 USB 线

3\. 烧录后不运行：检查地址是否正确、Flash Size 是否设为 4MB



\---



\---



\## English Instruction



\### 1. Required Tools \& Files

1\. Flashing Tool: flash\_download\_tool (Espressif Official)

2\. 3 Firmware Files

&#x20;  - bootloader.bin

&#x20;  - partitions.bin

&#x20;  - firmware.bin

3\. Hardware: ESP32-WROOM-32 (4MB Flash, No PSRAM), USB Cable

4\. OS: Windows



\---



\### 2. Tool Download (Official \& Safe)

https://www.espressif.com/en/support/download/other-tools



Extract the ZIP package, run `flash\_download\_tool.exe` directly. No installation required.



\---



\### 3. Flashing Steps (For Beginners)

1\. Open the tool, select \*\*ESP32\*\*, click OK

2\. Add and check files as below:

&#x20;  - bootloader.bin    Address: 0x1000

&#x20;  - partitions.bin    Address: 0x8000

&#x20;  - firmware.bin      Address: 0x10000

3\. Bottom Settings

&#x20;  - SPI Speed: 40MHz

&#x20;  - SPI Mode: DIO

&#x20;  - Flash Size: 4MB

4\. Select the correct COM port

5\. Click \*\*START\*\* to flash

6\. \*\*FINISH\*\* means flashing completed



\---



\### 4. Troubleshooting

1\. Flash failed: Hold the \*\*BOOT\*\* button, click START, release after progress starts

2\. No COM port: Install CH340/CP2102 driver, change USB cable

3\. Not running after flash: Check addresses and set Flash Size to 4MB



\---



\### 固定烧录参数 | Fixed Configuration

\- bootloader.bin  : 0x1000

\- partitions.bin  : 0x8000

\- firmware.bin    : 0x10000

\- SPI Mode        : DIO

\- Flash Size      : 4MB

