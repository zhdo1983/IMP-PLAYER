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

Dynamic Memory Orchestration: Implements a "模式切换即清场" (Clear-on-Switch) strategy. Metadata and buffers are dynamically allocated and freed to ensure 40KB+ contiguous heap for HTTPS handshakes.

Anti-Fragmentation Hack: Uses a "Silent Handshake" technique (connecting to 127.0.0.1) to force the network stack to flush buffers and defragment the heap before critical operations.

Multitasking: Decoupled FreeRTOS tasks for OLED rendering, audio decoding, and system monitoring.
![mmexport1775838527708](https://github.com/user-attachments/assets/6b438693-66ef-4ff8-8dfa-9a03bb403db8)
2. Hardware
Component   Specification
MCU     	 ESP32-WROOM-32 (4MB Flash, No PSRAM)
Audio      DAC/Amp	MAX98357A I2S Class-D Amplifier
Display   	SSD1306 128x64 OLED (I2C)
Storage	   TF/MicroSD Card (SPI Mode)
Input	     EC11 Rotary Encoder

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

4. Operation Guide
Rotate Encoder: Adjust volume (Home screen) or navigate menus.

Single Click: Confirm selection or enter sub-menu.

Double Click: Quick switch between player modes.

Long Press: Access system settings or return to home.

5. Web Interface
The device hosts a local web server for advanced configuration:

Network: Scan and manage WiFi saved slots.

Navidrome: Configure server URL and User Token.

OTA: Upload new .bin firmware files wirelessly.
6. Disclaimer & Contribution
⚠️ Disclaimer
This project is an experimental implementation of high-concurrency tasks on low-resource hardware. While it includes several memory-safety hacks (like the 127.0.0.1 handshake), unexpected reboots may occur under extreme network jitter or with overly large SD card directories (recommended < 500 tracks).

🤝 Contributing
This project was developed with the assistance of AI to optimize low-level memory management. If you find bugs or have ideas for even tighter memory optimization, please feel free to open an Issue or submit a Pull Request.

7. License
This project is licensed under the MIT License.

