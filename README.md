



https://github.com/user-attachments/assets/7eb9eef1-41f4-46a2-858e-66c2568c89ac


![mmexport1775849875180](https://github.com/user-attachments/assets/efdf3724-83d1-4d39-af24-0dfab7c0ee93)

IMP-PLAYER: The ESP32-WROOM Network Audio Terminal
1. Project Overview
IMP-PLAYER is a high-performance audio terminal based on the ESP32-WROOM-32 (No-PSRAM version). It is designed to push the hardware's physical limits, integrating network streaming, local playback, and complex UI interactions within a highly constrained memory environment.
![mmexport1775849868553](https://github.com/user-attachments/assets/61f3db9f-55e8-4881-be24-4f9b69547e80)

🚀 Core Features
Network Streaming: High-stability streaming (MP3/AAC) with HTTPS support.

Navidrome Integration: Full Subsonic/Navidrome API support for personal cloud music.

SD Card Player: Local MP3 playback with dynamic folder scanning.

Advanced UI/UX: 128x64 OLED display with real-time spectrum analysis and smooth animations.

System Utilities: NTP network clock, battery monitoring, and a built-in PONG game.

Maintenance: Web-based WiFi configuration portal and OTA (Over-the-Air) firmware updates.


![mmexport1775849851039](https://github.com/user-attachments/assets/f884a9c2-d020-415a-a0fb-d436b60d9ebf)

This project is specifically optimized for the Non-PSRAM ESP32-WROOM chip:

Dynamic Memory Orchestration: Implements a "模式切换即清场" (Clear-on-Switch) strategy. Metadata and buffers are dynamically allocated and freed to ensure 80KB+ contiguous heap for HTTPS handshakes.

Anti-Fragmentation Hack: Uses a "Silent Handshake" technique (connecting to 127.0.0.1) to force the network stack to flush buffers and defragment the heap before critical operations.

Multitasking: Decoupled FreeRTOS tasks for OLED rendering, audio decoding, and system monitoring.
2. Hardware
Component   Specification
MCU     	 ESP32-WROOM-32 (4MB Flash, No PSRAM)
Audio      DAC/Amp	MAX98357A I2S Class-D Amplifier
Display   	SSD1306 128x64 OLED (I2C)
Storage	   TF/MicroSD Card (SPI Mode)
Input	     EC11 Rotary Encoder
Lithium battery (3.7V) 602535P 600mAh x1
2.1 Pin Mapping
ESP32     Pin	     Component	Description
GPIO5	    SD_CS	SD    Card Chip Select
GPIO18   	SPI_SCK	    SD Card Clock
GPIO19	  SPI_MISO  	SD Card MISO
GPIO23	  SPI_MOSI	  SD Card MOSI
GPIO4   	OLED_SDA	  OLED I2C Data
GPIO14	  OLED_SCL	  OLED I2C Clock
GPIO25	  I2S_LRCK	  I2S Left/Right Clock
GPIO26	  I2S_BCLK	  I2S Bit Clock
GPIO22	  I2S_DOUT	  I2S Data Out
GPIO32	  ENC_A	      Encoder Phase A
GPIO33	  ENC_B	      Encoder Phase B
GPIO35	  ENC_SW	    Encoder Button
GPIO34	  BAT_ADC	    Battery Voltage Detection

3. Installation & Compilation
Prerequisites
Framework: Arduino IDE 2.x or PlatformIO.

Core: ESP32 Arduino Core (v2.0.x recommended).

Required Libraries
Please install the following libraries via the Library Manager:

ESP32-audioI2S (by schreibfaul1)

U8g2 (by olikraus)

ArduinoJson

NTPClient

Configuration
Open config.h.

(Optional) Set your default WiFi credentials or use the Web Portal later.

Configure your Navidrome server address and API credentials.

### 🔨 Building with PlatformIO
This project uses **PlatformIO** for advanced memory and library management.

1. Clone the repository.
2. Open the folder in VS Code with the PlatformIO extension installed.
3. The `platformio.ini` will automatically handle dependencies (Audio, U8g2, ArduinoJson, etc.).
4. Click **Upload** to flash the ESP32-WROOM.


## 🔌 Hardware & PCB Design

The project includes complete hardware design files, allowing you to build your own IMP-PLAYER.

### Files Location:
### Files Location:
- **Schematic:** [/Hardware/PCB_PCB1_2026-04-11.pdf](./Hardware/PCB_PCB1_2026-04-11.pdf)  - Check here for pin definitions and circuit logic.
- **EasyEDA Project:** [/Hardware/ProDoc_PCB1_2026-04-11.EPRO2](./Hardware/ProDoc_PCB1_2026-04-11.EPRO2)
- You can import this directly into [LCEDA (EasyEDA)](https://pro.lceda.cn/) for modification.

### Key Hardware Features:
- **ESP32-WROOM-32** Core (Fully utilizing the internal SRAM).
- **MAX98357A** I2S DAC for high-quality audio output.
- **Battery Management:** Integrated charging circuit with ADC voltage monitoring.
- **Form Factor:** Optimized to fit the custom 3D-printed case.

### Manufacturing:
You can export the **Gerber files** from the `.EPRO2` file using EasyEDA and send them to manufacturers like JLCPCB for production.


3.1 Initial Configuration (Code Level)
Before flashing, you must set your default server endpoints in the source code:
----------------------------------------------------------------------------------------------------------
OTA Configuration: Locate lines 96-97 in config.h:
  #define OTA_DEFAULT_VER_URL  "http://your-server/version.txt"
  #define OTA_DEFAULT_FW_URL   "http://your-server/firmware.bin"

Navidrome Default: Locate line 41 in navidrome.h:  
   #define ND_DEFAULT_SERVER "https://your-navidrome-url"
----------------------------------------------------------------------------------------------------------



5. Web Interface
The device hosts a local web server for advanced configuration:

Network: Scan and manage WiFi saved slots.

Navidrome: Configure server URL and User Token.

OTA: Upload new .bin firmware files wirelessly.

5.1 Backend Server Requirements
Navidrome Setup
Create a standard user account in your Navidrome instance.

You do not need to hardcode the password. Open the filebrowser Web Portal (after connecting to the device's WiFi) to enter the Username and Password. The device will automatically generate the MD5 Token for secure API communication.

OTA File Server
To use the OTA update feature, you need a simple HTTP file server (like Nginx, Apache, or a simple Python file server) hosting two files:

firmware.bin: The compiled binary of the project.

version.txt: A plain text file containing only the version number (e.g., 1.0.2).
The device compares the string in version.txt with its internal version to trigger an update.


6. Disclaimer & Contribution
⚠️ Disclaimer
This project is an experimental implementation of high-concurrency tasks on low-resource hardware. While it includes several memory-safety hacks (like the 127.0.0.1 handshake), unexpected reboots may occur under extreme network jitter or with overly large SD card directories (recommended < 500 tracks).

🤝 Contributing
This project was developed with the assistance of AI to optimize low-level memory management. If you find bugs or have ideas for even tighter memory optimization, please feel free to open an Issue or submit a Pull Request.

7. License
This project is licensed under the MIT License.

