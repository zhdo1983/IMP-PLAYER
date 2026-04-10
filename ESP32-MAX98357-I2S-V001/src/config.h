#ifndef CONFIG_H  // 防止头文件重复包含
#define CONFIG_H  // 定义配置头文件宏

//========================== 调试开关 ==========================  // 调试功能配置区域
#define DEBUG_MODE false  // 通用调试模式开关（true开启，false关闭）
#define DEBUG_STREAM_INFO false  // 流媒体信息调试开关
#define DEBUG_NAVI_VERBOSE false  // Navidrome详细日志开关
#define DEBUG_MP3_INFO true  // MP3播放信息调试开关

//========================== FreeRTOS 栈溢出检测 ==========================
// 说明：需在 platformio.ini（或 sdkconfig）中同时启用：
//   board_build.cmake_extra_args =
//     -DCONFIG_FREERTOS_CHECK_STACKOVERFLOW=2
// 或通过 idf.py menuconfig → FreeRTOS → Check for stack overflow → Method 2
//
// 启用后，当任何任务栈溢出时 vApplicationStackOverflowHook() 被调用，
// 串口打印任务名 + 剩余栈深度，随后触发 ESP.restart() 保护系统。
//
// DEBUG_STACK_MONITOR true  → 每隔 STACK_MONITOR_INTERVAL ms 打印各任务剩余栈水位
// DEBUG_STACK_MONITOR false → 仅在溢出时打印（生产环境推荐值）
#define DEBUG_STACK_MONITOR     false        // 定期打印任务栈水位（开发阶段设为true）
#define STACK_MONITOR_INTERVAL  30000UL      // 栈水位打印间隔（毫秒）
#define OLED_TASK_STACK_SIZE    (1024 * 6)   // OLED Task栈大小（字节），与xTaskCreate保持一致

//========================== 引脚定义 ==========================  // 硬件引脚配置区域

// SD卡硬件引脚定义  // SD卡SPI接口引脚配置
#define SD_CS          5  // SD卡片选引脚（CS）
#define SPI_MOSI      23  // SPI主机输出从机输入引脚
#define SPI_MISO      19  // SPI主机输入从机输出引脚
#define SPI_SCK       18  // SPI时钟引脚

// OLED显示屏引脚  // OLED显示模块I2C接口引脚
#define SCL 14  // I2C时钟引脚
#define SDA 4  // I2C数据引脚

// I2S音频引脚  // I2S音频接口配置
#define I2S_DOUT      25  // I2S数据输出引脚
#define I2S_BCLK      27  // I2S位时钟引脚
#define I2S_LRC       26  // I2S左右声道时钟引脚
#define I2S_SD_MODE   21  // I2S功放使能引脚
#define LED_BLUE      33  // 蓝色LED指示灯引脚

// 旋转编码器引脚  // 旋转编码器接口配置
#define ENCODER_CLK    32  // 编码器时钟引脚（CLK）
#define ENCODER_DT     16  // 编码器数据引脚（DT）
#define ENCODER_SW     17  // 编码器按钮引脚（SW）

// 液晶背光引脚  // LCD背光控制引脚（预留）
#define LED_LAN_1     14  // 背光控制引脚1
#define LED_LAN_2     28  // 背光控制引脚2
#define LED_LAN_3     44  // 背光控制引脚3

// 电池电量检测引脚  // 电池ADC检测引脚
#define BAT_DAC      36  // 电池电量ADC输入引脚

//========================== WiFi配置 ==========================  // WiFi网络配置区域
#define AP_SSID "ESP32_Radio"  // 设备作为AP热点时的SSID名称
#define AP_PASS ""  // AP热点密码（空字符串表示无密码）
#define CONFIG_PORTAL_TIMEOUT 180  // 配置门户超时时间（秒）

//========================== 电台配置 ==========================  // 网络电台配置区域
#define MAX_STATIONS 20  // 最大保存电台数量

//========================== 菜单配置 ==========================  // 菜单系统配置区域
#define MENU_TIMEOUT 10000  // 菜单自动退出超时时间（毫秒）
#define MAIN_MENU_COUNT 8  // 主菜单选项数量（含Navidrome）

//========================== SD扫描配置 ==========================  // SD卡扫描配置区域
#define MAX_SCAN_TASKS 2  // 最大并行扫描任务数（减少SD卡竞争）
#define MAX_TRACKS 500  // 最大曲目数量
#define SD_SCAN_TIMEOUT_ENABLED false  // SD扫描超时检测是否启用
#define SCAN_TIMEOUT 300000  // SD扫描超时时间（毫秒）

//========================== 时间配置 ==========================  // 超时时间配置区域
#define STATION_SWITCH_TIMEOUT 10000  // 电台切换超时时间（毫秒）
#define CONNECTION_TIMEOUT 15000  // 连接超时时间（毫秒）
#define STATION_CONNECT_TIMEOUT 20000  // 电台连接超时时间（毫秒）
#define SD_EXIT_TIMEOUT 8000  // SD卡模式退出超时时间（毫秒）
#define BATTERY_CHECK_INTERVAL 10000  // 电池电量检测间隔（毫秒）
#define AUDIO_STOP_TIMEOUT 3000  // 音频停止超时时间（毫秒）

//========================== 显示配置 ==========================  // 显示相关配置区域
#define MAX_INFO_LENGTH 128  // 信息字符串最大长度

//========================== OTA配置 ==========================  // OTA升级配置区域
#define OTA_VERSION_KEY      "fw_ver"                                              // NVS中存储固件版本的键名
#define DEFAULT_VERSION      "1.1.05"                                              // 默认固件版本号
#define DB_FILE              "/tracks.db"                                          // SD卡曲目数据库文件名

//========================== OTA URL 配置（NVS 可覆盖） ==========================
// 两个 URL 优先从 NVS 读取（键名如下），若 NVS 中为空则回落到以下编译期默认值。
// 用户可通过 Web 界面 /ota/config 修改并保存到 NVS，无需重新编译固件。
#define OTA_NVS_KEY_VER_URL  "ota_ver_url"                                        // NVS键：版本检查URL
#define OTA_NVS_KEY_FW_URL   "ota_fw_url"                                         // NVS键：固件下载URL
#define OTA_DEFAULT_VER_URL  "https://"  // 默认版本检查URL
#define OTA_DEFAULT_FW_URL   "https://"  // 默认固件下载URL

//========================== 全局变量声明 ==========================  // 全局变量声明区域

#endif // CONFIG_H  // 结束头文件保护