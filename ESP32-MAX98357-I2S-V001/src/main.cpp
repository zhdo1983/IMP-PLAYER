//**********************************************************************************************************
// ESP32 网络收音机 + SD 卡播放器
// 重构版本：按功能拆分为多个头文件
//**********************************************************************************************************

#include "Arduino.h"  // Arduino核心库
#include "WiFiMulti.h"  // WiFi多连接库
#include "Audio.h"  // 音频播放库
#include "SPI.h"  // SPI通信库
#include "SD.h"  // SD卡库
#include "FS.h"  // 文件系统库
#include <U8g2lib.h>  // OLED显示库
#include <Wire.h>  // I2C通信库
#include <WebServer.h>  // Web服务器库
#include <DNSServer.h>  // DNS服务器库
#include <Preferences.h>  // 非易失性存储库
#include <time.h>  // 时间库
#include <HTTPClient.h>  // HTTP客户端库
#include <Update.h>  // OTA更新库
#include <freertos/task.h>  // FreeRTOS任务库
#include <freertos/semphr.h>  // FreeRTOS信号量库
#include <esp_task_wdt.h>  // 看门狗库
#include <NTPClient.h>  // NTP时间同步库

// 功能模块头文件
#include "config.h"  // 系统配置参数
#include "globals.h"  // 全局变量声明
#include "boot_animation.h"  // 开机动画
#include "icons.h"  // 图标定义
#include "battery.h"  // 电池检测
#include "ota.h"  // OTA升级功能
#include "stations.h"  // 电台管理
#include "audio_manager.h"  // 音频管理
#include "sd_player.h"  // SD卡播放器
#include "encoder.h"  // 编码器控制
#include "wifi_config.h"  // WiFi配置
#include "display.h"  // 显示相关
#include "menu.h"  // 菜单处理
#include "navidrome.h"  // Navidrome网络音乐播放器
#include "alarm_clock.h"  // 音乐闹钟

//===========================================================================
// 全局对象定义（globals.h 中声明为 extern）
//===========================================================================
Audio     audio;  // 音频对象，负责流媒体和MP3播放
WiFiMulti wifiMulti;  // WiFi多连接管理
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, SCL, SDA);  // OLED显示对象
WebServer   server(80);  // Web服务器，用于WiFi配网
DNSServer   dnsServer;  // DNS服务器，用于配网模式
Preferences preferences;  // 非易失性存储，保存WiFi配置等

// 显示字符串
char infoString[MAX_INFO_LENGTH]   = "Audio Info:";  // 音频信息显示字符串
char stationString[MAX_INFO_LENGTH] = "station :";  // 电台名称显示字符串
char lastHost[MAX_INFO_LENGTH]     = "";  // 上次播放的URL
char errorInfoString[MAX_INFO_LENGTH] = "";  // 错误信息显示字符串

// 菜单状态
MenuState menuState         = MENU_NONE;  // 当前菜单状态
MenuState previousMenuState = MENU_NONE;  // 上一个菜单状态
int       menuIndex         = 0;  // 菜单当前选项索引
unsigned long menuStartTime = 0;  // 菜单进入时间，用于超时检测

// WiFi 配置
bool   configMode      = false;  // 是否处于WiFi配网模式
unsigned long configStartTime = 0;  // 配网开始时间
String newSSID = "", newPass = "";  // 新WiFi的SSID和密码
bool   wifiConfigExiting  = false;  // WiFi配置是否正在退出
bool   configPortalExiting = false;  // 配网门户是否正在退出

// 电台
String stationNames[MAX_STATIONS];  // 电台名称数组
String stationUrls[MAX_STATIONS];  // 电台URL数组

// 编码器
volatile int   encoderPos     = DEFAULT_VOLUME;  // 编码器位置（音量值）
volatile bool  encoderChanged = false;  // 编码器值是否改变
unsigned long  lastEncoderTime = 0;  // 上次编码器操作时间
bool           showingVolume  = false;  // 是否正在显示音量
unsigned long  volumeDisplayTime = 0;  // 音量显示开始时间
int            lastVolume     = -1;  // 上次音量值
volatile int8_t encoderDelta  = 0;  // 编码器增量值
volatile bool  buttonPressed  = false;  // 编码器按钮是否按下
volatile unsigned long lastButtonPress = 0;  // 上次按钮按下时间

// 开机动画状态
bool   bootScreenActive  = true;  // 开机动画是否激活显示
unsigned long bootScreenStartTime = 0;  // 开机动画开始时间
bool   audioSystemReady  = false;  // 音频系统是否就绪
bool   skipBootAnimation = false;  // 是否跳过开机动画

// SD 卡播放状态
bool   sdPlaying = false, sdPaused = false;  // SD卡播放状态和暂停状态
int    currentTrackIndex = 0, totalTracks = 0;  // 当前曲目索引和总曲目数
String* trackList;  // 曲目列表（动态分配，进入SD播放器时创建，退出时释放）
String currentTrackName = "";  // 当前曲目名称
unsigned long trackStartTime = 0, trackDuration = 0;  // 曲目开始时间和总时长
int    sdPlayerAnimType = 0;  // 播放器动画类型

// SD 卡扫描状态
bool  sdScanInProgress = false, sdScanComplete = false;  // 扫描是否进行中和是否完成
bool  sdScanTaskCreated = false, sdScanTaskShouldStop = false;  // 扫描任务是否创建和是否应该停止
unsigned long sdScanStartTime = 0;  // 扫描开始时间
int   sdScanProgress = 0, sdCheckedFiles = 0;  // 扫描进度和已检查文件数
int   sdEstimatedTotalFiles = 500, sdTotalMP3Files = 0;  // 预估总文件和实际MP3文件数
String sdScanStatus = "Initializing...";  // 扫描状态显示字符串

// SD 卡退出状态
bool  sdExitingInProgress = false;  // SD卡退出是否正在进行
unsigned long sdExitStartTime = 0;  // 退出开始时间
int   sdExitAnimationAngle = 0, clockGridAnimationPos = 0;  // 退出动画角度和时钟网格动画位置

// 并行扫描任务句柄
TaskHandle_t scanTaskHandles[MAX_SCAN_TASKS] = {NULL};  // 扫描任务句柄数组
volatile int activeScanTasks = 0;  // 当前活跃的扫描任务数
SemaphoreHandle_t scanResultMutex = NULL;  // 扫描结果互斥锁
String* scanFoundPaths;  // 扫描路径缓冲（动态分配，与trackList同生命周期）
volatile int scanFoundCount = 0;  // 扫描发现的文件数量

// 播放时间信息
unsigned long currentPlayTime = 0, totalPlayTime = 0;  // 当前播放时间和总播放时间
unsigned long lastPlayTimeUpdate = 0;  // 上次播放时间更新时刻
bool timeInfoAvailable = false;  // 时间信息是否可用
PlayMode currentPlayMode = PLAY_MODE_RANDOM;  // 当前播放模式（随机播放）
bool autoPlayEnabled = true;  // 是否启用自动播放

// 流媒体连接状态
bool  stationConnectingInProgress = false;  // 电台连接是否正在进行
unsigned long stationConnectStartTime = 0;  // 连接开始时间
int   stationConnectAnimationAngle = 0, stationConnectRetryCount = 0;  // 连接动画角度和重试次数
String connectingStationName = "", connectingStationUrl = "";  // 正在连接的电台名称和URL
bool  stationConnectFailed = false, forceStationConnectRetry = false;  // 连接是否失败和是否强制重试
bool  stationSwitching = false;  // 电台切换是否在进行
unsigned long stationSwitchStartTime = 0;  // 电台切换开始时间
int   lastSelectedStationIndex = -1;  // 上次选择的电台索引

// 音频系统状态
bool  audioSystemEnabled = true, audioLoopPaused = false;  // 音频系统是否启用和循环是否暂停
bool  audioSystemStopping = false;  // 音频系统是否正在停止
unsigned long audioStopStartTime = 0;  // 音频停止开始时间
bool  audioConnectionFailed = false;  // 音频连接是否失败
unsigned long lastConnectionAttempt = 0;  // 上次连接尝试时间
int   connectionRetryCount = 0;  // 连接重试次数
bool  wifiWasConnected = false;  // WiFi之前是否已连接
String savedSSID = "", savedPassword = "";  // 保存的WiFi名称和密码

// 音频质量监控状态
unsigned long lastAudioCheck = 0;  // 上次音频检查时间
unsigned long audioBufferUnderruns = 0, audioBufferOverruns = 0;  // 音频缓冲区欠载和过载次数
bool   audioQualityWarning = false;  // 音频质量警告标志
String audioQualityStatus = "Normal";  // 音频质量状态字符串

// 电池电量状态
uint8_t batteryLevelPercent = 100;  // 电池电量百分比
unsigned long lastBatteryCheck = 0;  // 上次电池检查时间

// OTA 升级配置
// OTA 服务器 URL — 定义为 String，由 loadOtaUrls() 从 NVS 读取
// 默认值写在 config.h（OTA_DEFAULT_VER_URL / OTA_DEFAULT_FW_URL）
String OTA_VERSION_URL  = "";  // 版本检查URL（setup()中由loadOtaUrls()填充）
String OTA_FIRMWARE_URL = "";  // 固件下载URL（setup()中由loadOtaUrls()填充）
String currentVersion  = "";  // 当前固件版本
float  currentVersionNum = 1.1f, serverVersionNum = 0.0f;  // 当前版本号和服务器版本号（数值）
String serverVersionStr = "";  // 服务器版本字符串
bool   otaUpdateAvailable = false, otaCheckCompleted = false;  // 是否有可用更新和检查是否完成
unsigned long otaCheckStartTime = 0;  // OTA检查开始时间
int    otaProgress = 0;  // OTA升级进度
bool   otaWiFiConnect = false;    // OTA是否需要WiFi连接
bool   naviWiFiConnect = false;   // Navidrome是否需要WiFi连接
bool   alarmWiFiConnect = false;  // 音乐闹钟是否需要WiFi连接
int    otaCheckResult = 0, otaAnimAngle = 0;  // OTA检查结果和动画角度

// CPU 监控状态
volatile uint32_t idleCount0 = 0, idleCount1 = 0;  // CPU空闲计数（两个核心）
float cpuUsage = 0.0f;  // CPU使用率
const uint32_t tickHz = configTICK_RATE_HZ;  // 系统时钟频率

// SD 卡互斥锁
SemaphoreHandle_t sdMutex = NULL;  // SD卡访问互斥量

// PONG 游戏状态
int ballX = 64, ballY = 32, ballDX = 2, ballDY = 1;  // 球的坐标和速度
int leftPaddleY = 24, rightPaddleY = 24;  // 左挡板和右挡板Y坐标
int leftScore = 0, rightScore = 0;  // 左分数和右分数
bool playerMode = false;  // 是否为玩家模式
int  gameTick   = 0;  // 游戏帧计数器

// 滚动显示状态
bool scrollStateResetFlag = false;  // 滚动状态重置标志

// Navidrome 播放器状态
String      naviServer           = "";     // Navidrome服务器地址
String      naviUser             = "";     // 用户名
String      naviPass             = "";     // 密码
bool        naviConfigValid      = false;  // 配置是否有效
int         naviSongCount        = 0;      // 歌曲总数
int         naviCurrentIdx       = 0;      // 当前播放索引
String      naviStatusMsg        = "";     // 状态提示文字
unsigned long naviTrackStartTime = 0;      // 当前曲目开始时间
int         naviCurrentDuration  = 0;      // 当前曲目时长（秒）
bool        naviPlaying          = false;  // 是否正在播放
unsigned long naviCurrentPlaySec = 0;      // 已播放秒数
NaviState   naviState            = NAVI_IDLE;  // 播放状态
NaviSong    naviSongList[ND_MAX_SONGS];   // 歌曲列表数组
bool        naviNeedsConfig      = false;   // Navidrome需要配置WEB服务
bool        startInNaviMode     = false;   // 重启后进入Navidrome模式
volatile bool naviStartRequested = false;  // 请求在主任务中执行 naviStart()（OLED Task 栈太小无法做HTTPS）
volatile bool naviReleasingMemory = false; // Navidrome正在用假连接触发 buffers freed，屏蔽 aggressiveMemoryCleanup
volatile bool naviPlayRetryRequested = false; // Navidrome播放失败，请求重试

// 音乐闹钟状态
int   alarmHour           = 7;     // 闹钟小时
int   alarmMinute         = 0;     // 闹钟分钟
bool  alarmEnabled        = false; // 闹钟是否启用
int   alarmSource         = 0;     // 音乐来源（0=MP3 1=IMP 2=Navidrome）
bool  alarmRepeat         = false; // 每天重复
bool  alarmRepeatActive   = false; // 重复模式激活：正在播放，等待用户操作返回闹钟
bool  alarmTriggered      = false; // 触发标志
int   alarmLastTriggerMin = -1;    // 上次触发分钟（防重复）
int   alarmEditHour       = 7;     // 编辑中的小时
int   alarmEditMin        = 0;     // 编辑中的分钟
int   alarmEditField      = 0;     // 当前编辑字段（0=小时 1=分钟 2=确认）

//===========================================================================
// 系统任务定义
//===========================================================================

// 空闲监控任务0（用于CPU使用率计算）
void idleMonitorTask0(void* arg) { while(1) { idleCount0++; vTaskDelay(1); } }
// 空闲监控任务1（用于CPU使用率计算）
void idleMonitorTask1(void* arg) { while(1) { idleCount1++; vTaskDelay(1); } }

// CPU监控任务（每秒计算一次CPU使用率）
void cpuMonitorTask(void* pvParameters) {
  uint32_t lastIdle0 = idleCount0, lastIdle1 = idleCount1;  // 上次的空闲计数
  while (1) {
    vTaskDelay(pdMS_TO_TICKS(1000));  // 每秒执行一次
    uint32_t nowIdle0 = idleCount0, nowIdle1 = idleCount1;  // 当前空闲计数
    uint32_t idleDiff  = (nowIdle0 - lastIdle0) + (nowIdle1 - lastIdle1);  // 计算空闲差值
    cpuUsage = constrain(100.0f * (1.0f - (float)idleDiff / (2 * tickHz)), 0.0f, 100.0f);  // 计算CPU使用率
    lastIdle0 = nowIdle0; lastIdle1 = nowIdle1;  // 更新上次计数
  }
}

// LED闪烁任务（蓝色LED，每秒切换一次状态）
void task1(void* pt) {
  pinMode(LED_BLUE, OUTPUT);  // 设置蓝色LED引脚为输出模式
  while (1) { digitalWrite(LED_BLUE, !digitalRead(LED_BLUE)); vTaskDelay(1000); }  // 每秒切换LED状态
}

//===========================================================================
// OLED 显示任务（菜单 + 动画 + SD 播放器界面）
// 注意：大量显示逻辑分布在此任务中，建议后续可拆到 oled_screens.h
//===========================================================================
void oledTask(void* pvParam);  // 声明，实现见下方

//===========================================================================
// FreeRTOS 栈溢出钩子 & 定期栈水位监控
// 需在 platformio.ini 中加入：
//   board_build.cmake_extra_args = -DCONFIG_FREERTOS_CHECK_STACKOVERFLOW=2
// 对应 idf.py menuconfig → FreeRTOS → Check for stack overflow → Method 2
//===========================================================================

// 栈溢出时由 FreeRTOS 内核调用（Method 2 可靠检测写越界）
extern "C" void vApplicationStackOverflowHook(TaskHandle_t xTask, char* pcTaskName) {
  Serial.print(F("[FATAL] Stack overflow in task: "));
  Serial.println(pcTaskName);
  Serial.printf("[FATAL] Remaining stack: %u words\n",
                (unsigned)uxTaskGetStackHighWaterMark(xTask));
  // 打印完后立即重启，避免系统进入未定义状态
  delay(200);
  ESP.restart();
}

// 定期打印所有任务的剩余栈水位（由 loop() 中的计时逻辑调用）
#if DEBUG_STACK_MONITOR
static void printTaskStackWatermarks() {
  static unsigned long _lastStackPrint = 0;
  if (millis() - _lastStackPrint < STACK_MONITOR_INTERVAL) return;
  _lastStackPrint = millis();

  Serial.println(F("=== Task Stack Watermarks (words) ==="));
  // 枚举所有任务（ESP-IDF 提供 uxTaskGetNumberOfTasks）
  UBaseType_t taskCount = uxTaskGetNumberOfTasks();
  TaskStatus_t* taskStatus =
      (TaskStatus_t*)pvPortMalloc(taskCount * sizeof(TaskStatus_t));
  if (taskStatus) {
    uint32_t totalRunTime = 0;
    UBaseType_t n = uxTaskGetSystemState(taskStatus, taskCount, &totalRunTime);
    for (UBaseType_t i = 0; i < n; i++) {
      Serial.printf("  %-20s  stack_min: %4u words  prio: %u\n",
                    taskStatus[i].pcTaskName,
                    (unsigned)taskStatus[i].usStackHighWaterMark,
                    (unsigned)taskStatus[i].uxCurrentPriority);
    }
    vPortFree(taskStatus);
  } else {
    Serial.println(F("  [WARN] pvPortMalloc failed for task list"));
  }
  Serial.printf("  Free heap: %u KB\n", ESP.getFreeHeap() / 1024);
  Serial.println(F("====================================="));
}
#endif  // DEBUG_STACK_MONITOR

//===========================================================================
// setup() - 系统初始化函数
//===========================================================================
void setup() {
  trackList      = NULL;  // 动态分配，进入SD播放器时由allocSDBuffers()创建
  scanFoundPaths = NULL;  // 动态分配，与trackList同生命周期

  Serial.begin(115200);  // 初始化串口通信，波特率115200
  delay(100);  // 等待串口稳定

  Serial.println("");  // 输出空行
  Serial.println("========== ESP32 Network Radio ==========");  // 打印标题
  Serial.print("Chip: "); Serial.println(ESP.getChipModel());  // 打印芯片型号
  Serial.print("Free RAM: "); Serial.print(ESP.getFreeHeap() / 1024.0, 2); Serial.println(" KB");  // 打印空闲内存
  Serial.println("=========================================");  // 打印分隔线

  // 电池 ADC 配置
  pinMode(BAT_DAC, INPUT);  // 设置电池ADC引脚为输入模式
  analogReadResolution(12);  // 设置ADC分辨率为12位
  analogSetAttenuation(ADC_11db);  // 设置ADC衰减为11dB

  // 看门狗定时器配置
  esp_task_wdt_init(60, true);  // 初始化看门狗，超时时间60秒
  esp_task_wdt_add(NULL);  // 将当前任务添加到看门狗监控

  // SPI 和 SD 卡初始化
  pinMode(SD_CS, OUTPUT);  // 设置SD卡片选引脚为输出
  digitalWrite(SD_CS, HIGH);  // 初始为高电平（未选中）
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);  // 初始化SPI总线
  SPI.setFrequency(4000000);  // 设置SPI频率为4MHz
  delay(100);  // 等待SPI稳定

  // 检测SD卡是否正常
  if (!SD.begin(SD_CS)) {
    Serial.println("WARNING: SD card initialization failed!");  // SD卡初始化失败
  } else {
    Serial.println("SD card initialized successfully.");  // SD卡初始化成功
    File testFile = SD.open("/test.txt", FILE_WRITE);  // 尝试创建测试文件
    if (testFile) { testFile.println("test"); testFile.close(); SD.remove("/test.txt"); }  // 写入测试后删除
  }

  // I2S 功放使能
  pinMode(I2S_SD_MODE, OUTPUT);  // 设置I2S功放使能引脚为输出
  digitalWrite(I2S_SD_MODE, 1);  // 使能I2S功放

  strcpy(infoString, "Initializing...");  // 初始化显示信息字符串

  // 非易失性存储（NVS）初始化
  preferences.begin("wifi-config", false);  // 打开名为wifi-config的NVS命名空间
  loadOtaUrls();  // 从NVS加载OTA服务器URL（首次运行时写入默认值）
  loadAlarmConfig();  // 从NVS加载闹钟配置
  currentVersion    = getFirmwareVersion();  // 获取当前固件版本
  currentVersionNum = parseVersionNumber(currentVersion);  // 解析版本号为数值
  float defaultVersionNum = parseVersionNumber(DEFAULT_VERSION);  // 解析默认版本号
  if (currentVersionNum < defaultVersionNum) {  // 如果当前版本低于默认版本
    preferences.putString(OTA_VERSION_KEY, DEFAULT_VERSION);  // 保存默认版本号
    currentVersion    = DEFAULT_VERSION;  // 更新当前版本
    currentVersionNum = defaultVersionNum;  // 更新版本数值
  }

  // SD 卡互斥锁创建
  sdMutex = xSemaphoreCreateMutex();  // 创建SD卡访问互斥量
  if (!sdMutex) {
    Serial.println("[FATAL] Failed to create sdMutex!");
  }

  // 扫描结果互斥锁创建
  scanResultMutex = xSemaphoreCreateMutex();  // 创建扫描结果互斥锁
  if (!scanResultMutex) {
    Serial.println("[FATAL] Failed to create scanResultMutex!");
  }

  // 随机数种子初始化
  randomSeed(esp_random() ^ (millis() << 12) ^ micros() ^ analogRead(0));  // 混合多个随机源生成种子

  // 旋转编码器初始化
  setupEncoder();  // 配置编码器引脚和中断
  lastVolume = audio.getVolume();  // 获取当前音量
  encoderPos = lastVolume;  // 同步编码器位置
  encoderDelta = 0;  // 重置编码器增量

  // 创建系统任务
  xTaskCreatePinnedToCore(cpuMonitorTask, "CPU Monitor", 2048, NULL, 1, NULL, 1);  // 创建CPU监控任务在核心1
  xTaskCreatePinnedToCore(idleMonitorTask1, "Idle1", 1024, NULL, 0, NULL, 1);  // 创建空闲监控任务1在核心1
  xTaskCreatePinnedToCore(idleMonitorTask0, "Idle0", 1024, NULL, 0, NULL, 0);  // 创建空闲监控任务0在核心0

  // ---- 检测从电台切换重启 ----
  bool restartFromStation = preferences.getBool("rstSta", false);  // 读取是否从电台切换重启标志
  if (restartFromStation) {
    String lastStationName = preferences.getString("stName", "");  // 读取上次电台名称
    String lastStationUrl  = preferences.getString("stUrl",  "");  // 读取上次电台URL

    // 立即创建OLED任务，显示过渡动画（新增）
    xTaskCreatePinnedToCore(oledTask, "OLED Task", 1024*6, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(task1, "Blink blue led", 1024, NULL, 0, NULL, 1);

    skipBootAnimation = true; bootScreenActive = false;  // 跳过开机动画
    audioSystemReady  = true; menuState = MENU_NONE;  // 音频系统就绪
    preferences.putBool("rstSta", false);  // 清除重启标志
    encoderPos = DEFAULT_VOLUME;  // 重置音量

    // 立即设置过渡状态（新增）
    menuState = MENU_STATION_SWITCHING;  // 设置菜单状态为电台切换过渡
    menuStartTime = millis();  // 记录菜单开始时间
    stationConnectingInProgress = true;  // 标记电台连接进行中
    stationConnectStartTime = millis();  // 记录连接开始时间
    stationConnectRetryCount = 0;  // 从0开始，不是255
    stationConnectAnimationAngle = random(0, 12) * 30;  // 随机生成连接动画角度

    connectingStationName = lastStationName;  // 设置连接的电台名称
    connectingStationUrl = lastStationUrl;  // 设置连接的电台URL

    strcpy(stationString, lastStationName.c_str());  // 更新电台显示
    strcpy(infoString, "重启完成，连接WiFi...");  // 更新状态信息

    // 连接WiFi（不要阻塞）
    WiFi.mode(WIFI_STA);  // 设置WiFi为Station模式
    String ssids[MAX_WIFI_COUNT], passes[MAX_WIFI_COUNT];  // 创建临时数组
    int cnt = loadSavedWiFiList(ssids, passes);  // 加载已保存的WiFi列表
    for (int i = 0; i < cnt; i++) {  // 遍历WiFi列表
      wifiMulti.addAP(ssids[i].c_str(), passes[i].c_str());  // 添加AP
    }

    // 让 processStationSwitching() 来处理连接过程
    // 不要在这里阻塞等待WiFi连接
    return;  // 返回，让OLED任务接管显示
  }

  configPortalExiting = false;  // 重置配置门户退出标志

  // ---- 检测 WiFi 配置模式重启 ----
  bool wifiConfigMode = preferences.getBool("wifiConfigMode", false);  // 读取WiFi配置模式标志
  if (wifiConfigMode) {
    preferences.putBool("wifiConfigMode", false);  // 清除配置模式标志
    skipBootAnimation = true; bootScreenActive = false;  // 跳过开机动画
    audioSystemReady  = true; menuState = MENU_NONE;  // 音频系统就绪
    wifiConfigExiting = false;  // 重置WiFi配置退出标志
    startConfigPortal();  // 启动配置门户
    xTaskCreatePinnedToCore(oledTask, "OLED Task", 1024*6, NULL, 1, NULL, 1);  // 创建OLED任务
    xTaskCreatePinnedToCore(task1, "Blink blue led", 1024, NULL, 0, NULL, 1);  // 创建LED任务
    esp_task_wdt_init(60, true);  // 重新初始化看门狗
    esp_task_wdt_add(NULL);  // 添加当前任务到看门狗
    return;  // 返回，不再执行后续初始化
  }

  // ---- 检测从 SD 卡退出后重启 ----
  bool restartFromSD = preferences.getBool("restartFromSD", false);
  if (restartFromSD) {
    preferences.putBool("restartFromSD", false);
    skipBootAnimation = true;
    bootScreenActive  = false;
    audioSystemReady  = true;
    encoderPos        = 12;
    loadStations();

    // 先完成 WiFi 连接和音频流启动，再创建 OLED 任务。
    // 这样 OLED 任务启动时连接已建立，用户看到的第一帧就是正在播放的状态，
    // 不会出现网络播放器界面卡顿等待的情况。
    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio.setVolume(encoderPos);
    connectToWiFi();
    WiFi.scanDelete();

    if (WiFi.status() == WL_CONNECTED) {
      stationConnectingInProgress  = true;
      stationConnectStartTime      = millis();
      stationConnectRetryCount     = 0;
      connectingStationName        = stationNames[7];
      connectingStationUrl         = stationUrls[7];
      stationConnectFailed         = false;
      menuState = MENU_STATION_CONNECTING;
      menuIndex = 0;
      menuStartTime = millis();
      stationConnectAnimationAngle = random(0, 12) * 30;
      digitalWrite(I2S_SD_MODE, 1);
      audio.connecttohost(stationUrls[7].c_str());
      strcpy(stationString, stationNames[7].c_str());
      strcpy(infoString, "Connecting...");
    } else {
      menuState = MENU_WIFI_CONNECTING;
      menuIndex = 0;
      menuStartTime = millis();
      strcpy(stationString, "正在连接WiFi...");
      strcpy(infoString, "请稍候");
    }

    // 连接/启动完成后再创建 OLED 任务，第一帧即显示正确状态
    xTaskCreatePinnedToCore(oledTask, "OLED Task", 1024*6, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(task1,    "Blink blue led", 1024, NULL, 0, NULL, 1);
    return;
  }

  // 创建 OLED 任务和 LED 任务
  if (xTaskCreatePinnedToCore(oledTask, "OLED Task", 1024*6, NULL, 1, NULL, 1) == pdPASS)  // 创建OLED显示任务
    Serial.println("OLED task created.");  // 打印创建成功信息

  if (xTaskCreatePinnedToCore(task1, "Blink blue led", 1024, NULL, 0, NULL, 1) == pdPASS)  // 创建LED闪烁任务
    Serial.println("LED task created.");  // 打印创建成功信息

  // 配置I2S引脚和默认音量（在连接WiFi之前完成，避免连接失败后音频未初始化）
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);  // 配置I2S引脚
  audio.setVolume(12);  // 设置默认音量12

  loadStations();  // 加载电台列表

  // ---- 预初始化 WiFi STA 模式 ----
  // LWIP 栈需要在启动网络连接前先初始化，这里先设置 STA 模式
  // 实际的 WiFi 连接将在选择模式超时后启动
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);  // 确保初始状态干净

  bool sdScanDone = preferences.getBool("sdScanDone", false);  // 读取SD扫描完成标志
  if (sdScanDone) sdScanComplete = true;  // 如果扫描过则标记完成

  // 注册Web服务器路由
  server.on("/",              HTTP_GET,  handleRoot);  // 根路径
  server.on("/config",         HTTP_POST, handleConfig);  // WiFi配置
  server.on("/saveRestart",    HTTP_POST, handleSaveRestart);  // 保存并重启
  server.on("/stations",       HTTP_GET,  handleStationsList);  // 获取电台列表
  server.on("/station/update", HTTP_POST, handleStationUpdate);  // 更新电台
  server.on("/station/delete", HTTP_POST, handleStationDelete);  // 删除电台
  server.on("/station/add",    HTTP_POST, handleStationAdd);  // 添加电台
  server.on("/wifi/list",     HTTP_GET,  handleWifiList);  // WiFi列表
  server.on("/wifi/add",      HTTP_POST, handleWifiAdd);  // 添加WiFi
  server.on("/wifi/delete",   HTTP_POST, handleWifiDelete);  // 删除WiFi
  server.on("/wifi/scan",     HTTP_GET,  handleWifiScan);  // 扫描周边WiFi
  // ESP32 WebServer不支持同路径注册两个方法(第二个覆盖第一个)，改用HTTP_ANY合并
  server.on("/navi/config",    HTTP_ANY,  handleNaviConfig);    // Navidrome配置(GET查/POST存)
  server.on("/navi/ping",      HTTP_GET,  handleNaviPing);  // 测试Navidrome连接
  server.on("/ota/config",     HTTP_ANY,  handleOtaConfig);  // OTA服务器URL配置(GET查/POST存)
  server.begin();  // 启动WEB服务器

  // 检查是否需要进入Navidrome模式
  startInNaviMode = preferences.getBool("startInNaviMode", false);
  if (startInNaviMode) {
    preferences.putBool("startInNaviMode", false);  // 清除标志
    // naviStart()不在setup()中调用，避免WiFi未稳定时就尝试连接
    // Navidrome播放由用户在菜单中选择触发
  }
}

//===========================================================================
// loop() - 主循环函数
//===========================================================================
void loop() {

  static unsigned long _loopCount = 0;
static unsigned long _lastLoopLog = 0;
if (millis() - _lastLoopLog > 30000) {
    _lastLoopLog = millis();
    Serial.printf("[LOOP] alive cnt=%lu heap=%u menuState=%d naviState=%d\n",
                  _loopCount, ESP.getFreeHeap(), menuState, naviState);
}
_loopCount++;

  // 串口命令处理
  if (Serial.available()) {  // 检查串口是否有数据
    String cmd = Serial.readStringUntil('\n');  // 读取一行命令
    cmd.trim();  // 去除首尾空格
    if (cmd == "refresh" || cmd == "refreshsd") {  // 刷新SD卡数据库命令
      Serial.println("Refreshing SD card database...");  // 打印提示信息
      deleteTrackDatabase();  // 删除曲目数据库
      totalTracks = 0; sdScanComplete = false; sdScanInProgress = true;  // 重置扫描状态
      sdScanStatus = "Refreshing...";  // 更新扫描状态
    } else if (cmd == "dbinfo") {  // 数据库信息命令
      Serial.print("Total tracks: ");   Serial.println(totalTracks);  // 打印总曲目数
      Serial.print("DB file exists: "); Serial.println(SD.exists(DB_FILE) ? "Yes" : "No");  // 打印数据库文件是否存在
      Serial.print("Free heap: ");      Serial.println(ESP.getFreeHeap());  // 打印空闲内存
    } else if (cmd == "help") {  // 帮助命令
      Serial.println("refresh / dbinfo / help");  // 打印可用命令
    }
  }

  // 音频主循环（处理音频解码和播放）
  if (!audioLoopPaused) audio.loop();  // 如果未暂停则执行音频循环

  unsigned long currentTime = millis();  // 获取当前运行时间
  esp_task_wdt_reset();  // 重置看门狗定时器

  asyncAudioSystemCleanup();  // 异步清理音频系统
  processStationConnection();  // 处理电台连接
  processStationSwitching();  // 处理电台切换（新增）
  pollWiFiConnection();  // 轮询WiFi连接状态（非阻塞启动后在此检测结果）

  // 电池电量检测（按固定间隔检测）
  if (currentTime - lastBatteryCheck >= BATTERY_CHECK_INTERVAL) {  // 达到检测间隔
    checkBatteryLevel(); lastBatteryCheck = currentTime;  // 检测电池电量并更新时间
  }

  // 非音频操作节流（50ms）
  static unsigned long lastNonAudioTime = 0;  // 上次非音频操作时间
  if (currentTime - lastNonAudioTime < 50) return;  // 如果小于50ms则返回
  lastNonAudioTime = currentTime;  // 更新上次时间

  // 串口 URL 输入（直接播放网络流）
  if (Serial.available()) {  // 检查串口是否有数据
    audio.stopSong();  // 停止当前播放
    if (ESP.getMaxAllocHeap() < 16384) aggressiveMemoryCleanup();  // 内存不足时清理
    digitalWrite(I2S_SD_MODE, 1);  // 使能I2S功放
    esp_task_wdt_reset();  // 重置看门狗
    String r = Serial.readString(); r.trim();  // 读取URL字符串
    if (r.length() > 5) { audio.connecttohost(r.c_str()); resetConfigMode(); }  // 连接播放网络流
  }

  // ── 每天重复模式拦截 ──────────────────────────────────────────────────
  // 闹钟触发后处于重复模式时，任何按钮按下或旋转编码器操作都会：
  //   1. 停止当前播放
  //   2. 回到音乐闹钟界面等待下次触发
  // 用户如需切换播放器，需从闹钟子菜单选择"返回选择界面"
  if (alarmRepeatActive && (buttonPressed || encoderChanged)) {
    Serial.println("[Alarm] Repeat mode: user input detected, returning to alarm clock");
    buttonPressed  = false;
    encoderChanged = false;
    encoderDelta   = 0;

    // 停止当前播放（根据当前状态做对应清理）
    if (sdPlaying || menuState == MENU_SD_PLAYER || menuState == MENU_SD_LOADING) {
      // MP3播放器：停止SD播放，释放内存，重连WiFi
      audio.stopSong(); sdPlaying = false; sdPaused = false;
      sdScanTaskShouldStop = true; sdScanInProgress = false;
      for (int i = 0; i < MAX_TRACKS; i++) { if (trackList)      { trackList[i]      = ""; trackList[i].reserve(0); } }
      for (int i = 0; i < MAX_TRACKS; i++) { if (scanFoundPaths) { scanFoundPaths[i]  = ""; scanFoundPaths[i].reserve(0); } }
      freeSDBuffers();
      totalTracks = 0; sdTotalMP3Files = 0; scanFoundCount = 0;
      safeStopAudioSystem(); delay(300);
      aggressiveMemoryCleanup();
      // 重连WiFi（闹钟时钟界面需要WiFi保持NTP同步）
      if (WiFi.status() != WL_CONNECTED) {
        alarmWiFiConnect = true;
        menuState  = MENU_WIFI_CONNECTING;
        menuIndex  = 0;
        menuStartTime = millis();
        stationConnectAnimationAngle = random(0, 12) * 30;
        strcpy(stationString, "正在连接WiFi...");
        strcpy(infoString, "请稍候");
      } else {
        menuState = MENU_ALARM_CLOCK;
        menuIndex = 0;
        menuStartTime = millis();
        digitalWrite(I2S_SD_MODE, 0);  // 关闭功放省电
      }
    } else if (naviPlaying || menuState == MENU_NAVI_PLAYER) {
      // Navidrome：停止播放
      audio.stopSong();
      naviPlaying = false;
      naviState   = NAVI_IDLE;
      menuState = MENU_ALARM_CLOCK;
      menuIndex = 0;
      menuStartTime = millis();
      digitalWrite(I2S_SD_MODE, 0);  // 关闭功放省电
    } else {
      // IMP网络播放器或其他状态
      audio.stopSong();
      menuState = MENU_ALARM_CLOCK;
      menuIndex = 0;
      menuStartTime = millis();
      digitalWrite(I2S_SD_MODE, 0);  // 关闭功放省电
    }

    alarmRepeatActive = false;  // 清除拦截标志，等待下次触发重新设置
  }

  // 编码器旋转处理
  if (encoderChanged) {  // 检测到编码器变化
    encoderChanged = false;  // 清除变化标志
    if (menuState != MENU_NONE) {  // 如果在菜单中
      handleMenuRotation();  // 处理菜单旋转
    } else {  // 如果不在菜单中
      encoderPos = constrain(encoderPos, 0, 21);  // 限制音量范围
      audio.setVolume(encoderPos);  // 设置音量
      showingVolume = true; volumeDisplayTime = millis();  // 显示音量
    }
  }

  // 音量显示超时处理
  if (showingVolume && (millis() - volumeDisplayTime > 2000)) showingVolume = false;  // 2秒后隐藏音量显示
  if (menuState == MENU_ANIMATION && showingVolume) showingVolume = false;  // 动画模式下隐藏音量

  // 菜单超时处理
  if ((menuState == MENU_MAIN || menuState == MENU_STATION_SELECT) &&  // 主菜单或电台选择菜单
      (millis() - menuStartTime > MENU_TIMEOUT)) {  // 超过菜单超时时间
    if (menuState == MENU_STATION_SELECT) safeRestartAudioSystem();  // 电台选择菜单需要重启音频系统
    resetScrollState();
    // 如果Navidrome处于活动状态，超时后返回Navidrome界面而非主屏幕
    if (naviState == NAVI_PLAYING || naviState == NAVI_PAUSED ||
        naviState == NAVI_CONNECTING || naviState == NAVI_LOADING ||
        naviState == NAVI_ERROR) {
      menuState = MENU_NAVI_PLAYER;  // 返回Navidrome播放器界面
      menuStartTime = millis();
    } else {
      menuState = MENU_NONE;  // 退出菜单
    }
  }

  // Navidrome控制菜单超时处理（15秒返回播放器界面，菜单项增加到8项适当延长）
  if (menuState == MENU_NAVI_CONTROL && millis() - menuStartTime > 15000) {
    encoderPos = constrain(encoderPos, 0, 21);
    menuState = MENU_NAVI_PLAYER;
    menuStartTime = millis();
  }

  // 闹钟子菜单/时间设置/来源设置超时（30秒无操作返回时钟界面）
  if ((menuState == MENU_ALARM_SUBMENU ||
       menuState == MENU_ALARM_SET_TIME ||
       menuState == MENU_ALARM_SET_SOURCE) &&
      millis() - menuStartTime > 30000) {
    menuState = MENU_ALARM_CLOCK;
    menuIndex = 0;
    menuStartTime = millis();
  }

  // SD控制菜单超时处理
  if (menuState == MENU_SD_CONTROL && millis() - menuStartTime > 10000) {  // SD控制菜单超时10秒
    encoderPos = constrain(encoderPos, 0, 21); menuState = MENU_SD_PLAYER;  // 返回播放器界面
  }

  // SD动画选择菜单超时处理
  if (menuState == MENU_SD_ANIMATION_SELECT && millis() - menuStartTime > 10000) {  // 动画选择超时10秒
    encoderPos = constrain(encoderPos, 0, 21); menuState = MENU_SD_PLAYER;  // 返回播放器界面
  }

  // 播放器选择菜单超时处理：启动WiFi连接，连接成功后进入IMP播放器
  if (menuState == MENU_PLAYER_SELECT && millis() - menuStartTime > 30000) {  // 选择器超时30秒
    encoderPos = DEFAULT_VOLUME; audio.setVolume(encoderPos); digitalWrite(I2S_SD_MODE, 1);  // 恢复默认音量
    bool wifiStarted = startConnectToWiFi();  // 启动WiFi连接（非阻塞）
    if (wifiStarted) {
      menuState     = MENU_WIFI_CONNECTING;  // 进入WiFi连接等待界面
      menuStartTime = millis();               // 记录开始时间
      strcpy(stationString, "正在连接WiFi");
      strcpy(infoString,    "请稍候...");
    }
  }

  // SD 卡退出状态机
  // 重启是清除 SD 播放造成的堆碎片的唯一可靠方法。
  // 最小化重启前的操作，让 OLED 的滚动方块动画播放一到两帧后立即重启，
  // 避免用户看到空白或卡顿的网络播放器界面。
  // 重启后 setup() 检测到 restartFromSD=true，在干净的堆上连接 WiFi，
  // 进入网络播放器时声音几乎立即出现。
  if (menuState == MENU_SD_EXITING && sdExitingInProgress) {
    if (wifiConfigExiting) {
      // WiFi 配置退出：不重启，直接切换到配置门户
      audio.stopSong(); audioLoopPaused = true; delay(100);
      WiFi.disconnect(true); WiFi.mode(WIFI_OFF); delay(100);
      aggressiveMemoryCleanup();
      wifiConfigExiting    = false;
      sdExitingInProgress  = false;
      startConfigPortal();
      menuState = MENU_NONE;
      return;
    } else if (configPortalExiting) {
      // 配置门户退出：不重启，直接切换
      WiFi.softAPdisconnect(true); WiFi.mode(WIFI_OFF); delay(100);
      aggressiveMemoryCleanup();
      sdExitingInProgress  = false;
      configPortalExiting  = false;
      menuState = MENU_WIFI_CONNECTING;
      menuIndex = 0;
      menuStartTime = millis();
      stationConnectAnimationAngle = random(0, 12) * 30;
      strcpy(stationString, "正在扫描WiFi...");
      strcpy(infoString,    "请稍候");
    } else {
      // 正常 SD 退出：停止音频，立即重启
      // OLED 任务此时正在显示滚动方块动画，重启在动画播放约1-2帧后触发
      audio.stopSong();
      sdScanTaskShouldStop = true;
      sdPlaying = false; sdPaused = false;
      delay(300);  // 让 OLED 至少渲染一帧动画，用户能看到过渡效果
      Serial.println("[SD Exit] Restarting for clean heap...");
      preferences.putBool("restartFromSD", true);
      ESP.restart();
    }
  }

  // 编码器按钮处理
  if (buttonPressed) {  // 检测到按钮按下
    buttonPressed = false;  // 清除按钮按下标志
    if (configMode) {  // 在配置模式中
      preferences.putBool("restartFromSD", true);  // 保存重启标志
      configPortalExiting = true;  // 标记配置门户退出
      menuState = MENU_SD_EXITING; sdExitingInProgress = true; sdExitStartTime = millis();  // 进入退出状态
      configMode = false; WiFi.softAPdisconnect(true); dnsServer.stop();  // 停止配置模式
    } else if (menuState != MENU_NONE) {  // 在菜单中
      handleMenuButton();  // 处理菜单按钮
    } else {  // 不在菜单中
      // 如果Navidrome处于活动状态，按钮进入Navidrome播放器界面而非主菜单
      if (naviState == NAVI_PLAYING || naviState == NAVI_PAUSED ||
          naviState == NAVI_CONNECTING || naviState == NAVI_LOADING ||
          naviState == NAVI_ERROR) {
        menuState = MENU_NAVI_PLAYER; menuIndex = 0; menuStartTime = millis();  // 进入Navidrome界面
      } else {
        menuState = MENU_MAIN; menuIndex = 0; menuStartTime = millis();  // 进入主菜单
      }
    }
  }

  // Web服务器请求处理
  // configMode(AP配网)时需要DNS重定向；正常联网时只需处理Web请求
  // naviNeedsConfig已废弃——Navidrome未配置时保持WiFi连接，用局域网IP访问即可
  if (configMode) {
    dnsServer.processNextRequest();  // AP模式：DNS重定向到配网页
    server.handleClient();
  } else if (WiFi.status() == WL_CONNECTED) {
    server.handleClient();           // 正常联网：响应所有Web API请求（含Navidrome配置）
  }

  // SD 卡播放时间更新
  if (sdPlaying && trackStartTime > 0 && millis() - lastPlayTimeUpdate > 5000) {  // 播放中且5秒未更新
    if (!sdPaused) currentPlayTime = (millis() - trackStartTime) / 1000;  // 更新当前播放时间
    if (totalPlayTime == 0) totalPlayTime = 300;  // 默认时长5分钟
  }

  checkStationSwitchTimeout();  // 检查电台切换超时
  handleAudioConnectionFailure();  // 处理音频连接失败
  checkAudioQuality();  // 检查音频质量
  naviUpdate();  // 更新Navidrome播放状态（时间推进、自动下一首）

  // 闹钟触发检测（仅在 MENU_ALARM_CLOCK 状态下生效）
  checkAlarmTrigger();
  if (alarmTriggered) triggerAlarm();

#if DEBUG_STACK_MONITOR
  printTaskStackWatermarks();  // 定期打印各任务栈水位（仅调试版本）
#endif

  // 在主任务（loop）中执行 naviStart()
  // OLED Task 栈仅 6KB，TLS 握手需要 >20KB，必须在此执行
  // 添加节流：重试间隔至少 2 秒，避免频繁重试
  static unsigned long lastNaviRetryTime = 0;
  if (naviStartRequested && (millis() - lastNaviRetryTime > 2000)) {
    naviStartRequested = false;
    lastNaviRetryTime = millis();
    naviStart();
  }
}

//===========================================================================
// 音频事件回调函数
//===========================================================================

// 音频信息回调（处理各种音频系统信息）
void audio_info(const char* info) {
  #if DEBUG_STREAM_INFO  // 调试模式下打印信息
  Serial.print("info        "); Serial.println(info);
  #endif

  // 从 BitRate 计算MP3时长
  if (sdPlaying && totalPlayTime == 0) {  // SD卡播放中且未获取时长
    static unsigned long audioLength = 0, bitRate = 0;  // 静态变量保持数值
    if (strstr(info, "Audio-Length:"))  // 检查音频长度信息
      audioLength = atoi(strstr(info, "Audio-Length:") + strlen("Audio-Length:"));  // 解析音频长度
    if (strstr(info, "BitRate:"))  // 检查比特率信息
      bitRate = atoi(strstr(info, "BitRate:") + strlen("BitRate:"));  // 解析比特率
    if (audioLength > 0 && bitRate > 0) {  // 两个值都有效
      unsigned long duration = audioLength / (bitRate / 8);  // 计算时长（秒）
      if (duration > 10 && duration < 3600) { totalPlayTime = duration; timeInfoAvailable = true; }  // 合理范围内则保存
    }
  }

  // 新主机/重定向时执行内存碎片整理
  // 跳过条件：
  //   1. naviPlaying=true        — Navidrome 流媒体，stopSong() 会误杀正在建立的连接
  //   2. naviReleasingMemory=true — Navidrome 故意发出的假连接，不应触发清理
  //   3. stationConnectingInProgress=true — 刚通过 connecttohost() 发起连接，
  //      音频库立即回调 "Connect to new host"，此时 stopSong() 会中断尚未建立的连接，
  //      导致界面永远停留在 Connecting... buf:0%
  if ((strstr(info, "Connect to new host") || strstr(info, "redirect to new host"))
      && !naviPlaying && !naviReleasingMemory && !stationConnectingInProgress) {
    Serial.println("[MEM] New host - performing cleanup...");
    audio.stopSong(); delay(100);
    int w = 0;
    while (WiFi.status() != WL_CONNECTED && w++ < 20) delay(100);
    aggressiveMemoryCleanup();
    strcpy(infoString, "Memory cleanup...");
  }

  // 检测连接失败
  if (strstr(info, "not enough memory") || strstr(info, "connection failed") ||  // 内存不足
      strstr(info, "timeout") || strstr(info, "SSL - Memory allocation failed") ||  // 超时/SSL错误
      strstr(info, "MP3Decoder_AllocateBuffers")) {  // MP3解码器分配失败
    audioConnectionFailed = true;  // 标记连接失败
    if (stationConnectingInProgress && strstr(info, "MP3Decoder_AllocateBuffers")) {  // 正在连接且解码器失败
      forceStationConnectRetry = true;  // 强制重试
      stationConnectRetryCount = min(stationConnectRetryCount + 1, 3);  // 增加重试计数
      stationConnectStartTime = millis();  // 重置开始时间
      stationConnectAnimationAngle = random(0, 12) * 30;  // 随机动画角度
      strcpy(infoString, "内存整理/重试...");  // 更新显示
    }
  }

  // Navidrome 连接被重置检测
  if (naviPlaying && strstr(info, "Connection reset")) {
    #if DEBUG_NAVI_VERBOSE
    Serial.println("[Navi] Connection reset detected, requesting retry...");
    #endif
    naviPlayRetryRequested = true;
  }

  // 检测连接成功
  if (strstr(info, "stream ready") || strstr(info, "syncword found")) {  // 流就绪或同步字找到
    audioConnectionFailed = false;  // 清除失败标志
    connectionRetryCount  = 0;  // 重置重试计数
    sdScanInProgress = false;  // 停止扫描
    // Navidrome：流就绪后才开始计时，避免缓冲等待时间被误计入播放时长
    if (naviPlaying && naviTrackStartTime == 0) {
      naviTrackStartTime = millis();
      #if DEBUG_NAVI_VERBOSE
      Serial.println("[Navi] Stream ready, playback timer started.");
      #endif
    }
    if (stationConnectingInProgress) {  // 如果正在连接电台
      stationConnectingInProgress = false;  // 清除连接中标志
      stationConnectFailed = false;  // 清除失败标志
      menuState = MENU_NONE;  // 退出菜单状态
      digitalWrite(I2S_SD_MODE, 1);  // 使能功放
      strncpy(stationString, connectingStationName.c_str(), MAX_INFO_LENGTH - 1);  // 复制电台名称
      stationString[MAX_INFO_LENGTH - 1] = '\0';  // 字符串结束符
      strcpy(infoString, "Connected successfully");  // 更新状态信息
      resetScrollState();  // 重置滚动状态
    }
  }

  // 错误信息显示
  if (strstr(info, "slow stream") || strstr(info, "dropouts")) {  // 数据流缓慢或断续
    strncpy(errorInfoString, "数据流缓慢,可能断连", MAX_INFO_LENGTH - 1);  // 复制错误信息
    strncpy(infoString, info, MAX_INFO_LENGTH - 1);  // 复制原始信息
  } else {
    strncpy(infoString, info, MAX_INFO_LENGTH - 1);  // 直接复制信息
  }
  infoString[MAX_INFO_LENGTH - 1] = '\0';  // 字符串结束符
  errorInfoString[MAX_INFO_LENGTH - 1] = '\0';  // 错误字符串结束符
}

// ID3数据回调（用于调试）
void audio_id3data(const char* info) {
  #if DEBUG_MODE  // 调试模式下打印
  Serial.print("id3data     "); Serial.println(info);
  #endif
}

// 音频时长回调
void audio_duration(const char* info) {
  #if DEBUG_MP3_INFO  // 调试模式下打印
  Serial.print("duration    "); Serial.println(info);
  #endif
  if (sdPlaying) {  // SD卡播放中
    unsigned long d = atoi(info);  // 解析时长
    if (d > 0) { totalPlayTime = d; timeInfoAvailable = true; }  // 保存时长
  }
}

// 播放位置回调
void audio_position(const char* info) {
  if (sdPlaying) { currentPlayTime = atoi(info); lastPlayTimeUpdate = millis(); }  // 更新播放位置
}

// MP3播放结束回调
void audio_eof_mp3(const char* info) {
  #if DEBUG_MP3_INFO  // 调试模式下打印
  Serial.print("eof_mp3     "); Serial.println(info);
  #endif
  if (sdPlaying && !sdPaused) autoPlayNext();  // 非暂停状态则自动播放下一首（SD模式）
  if (naviPlaying) naviOnTrackEnd();  // Navidrome模式自动下一首
}

// 电台名称显示回调
void audio_showstation(const char* info) {
  #if DEBUG_STREAM_INFO  // 调试模式下打印
  Serial.print("station     "); Serial.println(info);
  #endif
  strncpy(stationString, info, MAX_INFO_LENGTH - 1);  // 复制电台名称
  stationString[MAX_INFO_LENGTH - 1] = '\0';  // 字符串结束符
}

// streamtitle显示回调（歌曲标题）
void audio_showstreamtitle(const char* info) {
  #if DEBUG_STREAM_INFO  // 调试模式下打印
  Serial.print("streamtitle "); Serial.println(info);
  #endif
  const char* prefix = "StreamTitle=";  // 流标题前缀
  const char* ptr    = strstr(info, prefix);  // 查找前缀位置
  if (ptr) {  // 找到前缀
    ptr += strlen(prefix);  // 跳过前缀
    int i = 0;
    while (i < MAX_INFO_LENGTH - 1 && ptr[i] != '\0' && ptr[i] != ';')  // 复制到分号或结束
      infoString[i] = ptr[i++];
    infoString[i] = '\0';  // 字符串结束符
  } else {  // 未找到前缀
    strncpy(infoString, info, MAX_INFO_LENGTH - 1);  // 直接复制
    infoString[MAX_INFO_LENGTH - 1] = '\0';  // 字符串结束符
  }
}

// 以下为空回调（保留接口但不做处理）
void audio_bitrate(const char* info)   { }  // 比特率信息回调
void audio_commercial(const char* info){ }  // 商业信息回调
void audio_icyurl(const char* info)    { }  // ICY URL回调

// 上次主机回调
void audio_lasthost(const char* info) {
  strncpy(lastHost, info, MAX_INFO_LENGTH - 1);  // 复制上次主机URL
  lastHost[MAX_INFO_LENGTH - 1] = '\0';  // 字符串结束符
}

//===========================================================================
// OLED 显示任务实现（包含所有界面绘制逻辑）
//===========================================================================
// 注意：该任务体积较大，建议进一步拆分为 oled_screens.h
// 此处保留完整实现以保证编译通过
#include "oled_task.h"