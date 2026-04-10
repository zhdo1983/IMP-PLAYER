#ifndef GLOBALS_H  // 防止头文件重复包含
#define GLOBALS_H  // 定义全局变量头文件宏

#include <Arduino.h>  // 包含Arduino核心库
#include <Audio.h>  // 包含音频库
#include <WiFiMulti.h>  // 包含WiFi多连接库
#include <U8g2lib.h>  // 包含U8G2 OLED库
#include <WebServer.h>  // 包含Web服务器库
#include <DNSServer.h>  // 包含DNS服务器库
#include <Preferences.h>  // 包含非易失性存储库
#include "config.h"  // 包含配置头文件

#define DEFAULT_VOLUME 16  // 默认音量值（0-21）

//========================== 菜单状态枚举 ==========================  // 菜单状态枚举定义区域
enum MenuState {  // 菜单状态枚举类型
  MENU_NONE,  // 无菜单状态
  MENU_PLAYER_SELECT,  // 播放器选择状态
  MENU_MAIN,  // 主菜单状态
  MENU_STATION_SELECT,  // 电台选择状态
  MENU_ANIMATION,  // 动画/游戏状态
  MENU_CLOCK,  // 时钟显示状态
  MENU_SD_PLAYER,  // SD卡播放器状态
  MENU_SD_CONTROL,  // SD卡控制菜单状态
  MENU_PLAY_MODE,  // 播放模式选择状态
  MENU_SD_LOADING,  // SD卡加载状态
  MENU_SD_EXITING,  // SD卡退出状态
  MENU_STATION_SWITCHING,  // 电台切换过渡状态
  MENU_STATION_CONNECTING,  // 电台连接中状态
  MENU_WIFI_CONNECTING,  // WiFi连接中状态
  MENU_FACTORY_RESET,  // 恢复出厂设置状态
  MENU_SD_ANIMATION_SELECT,  // SD动画选择状态
  MENU_MEMORY_INFO,  // 内存信息状态
  MENU_OTA_CHECK,  // OTA检查状态
  MENU_OTA_CONFIRM,  // OTA确认状态
  MENU_OTA_UPGRADING,  // OTA升级中状态
  MENU_NAVI_PLAYER,       // Navidrome播放器主界面状态
  MENU_NAVI_CONTROL,      // Navidrome控制菜单状态
  MENU_ALARM_CLOCK,       // 音乐闹钟时钟显示界面
  MENU_ALARM_SUBMENU,     // 闹钟子菜单（设定时间/音乐来源/返回）
  MENU_ALARM_SET_TIME,    // 闹钟时间设置界面
  MENU_ALARM_SET_SOURCE,  // 闹钟音乐来源选择界面
  MENU_NAVI_ANIMATION_SELECT  // Navidrome动画效果选择界面
};

//========================== 播放模式枚举 ==========================  // 播放模式枚举定义区域
enum PlayMode {  // 播放模式枚举类型
  PLAY_MODE_SINGLE,  // 单曲循环模式
  PLAY_MODE_SEQUENCE,  // 顺序播放模式
  PLAY_MODE_RANDOM  // 随机播放模式
};

//========================== 全局对象 ==========================  // 全局对象声明区域
extern Audio audio;  // 音频对象（外部引用）
extern WiFiMulti wifiMulti;  // WiFi多连接对象
extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;  // OLED显示对象
extern WebServer server;  // Web服务器对象
extern DNSServer dnsServer;  // DNS服务器对象
extern Preferences preferences;  // 非易失性存储对象

//========================== 显示字符串 ==========================  // 显示字符串声明区域
extern char infoString[MAX_INFO_LENGTH];  // 音频信息显示字符串
extern char stationString[MAX_INFO_LENGTH];  // 电台名称显示字符串
extern char lastHost[MAX_INFO_LENGTH];  // 上次播放的URL
extern char errorInfoString[MAX_INFO_LENGTH];  // 错误信息显示字符串

//========================== 菜单状态变量 ==========================  // 菜单状态变量声明区域
extern MenuState menuState;  // 当前菜单状态
extern MenuState previousMenuState;  // 上一个菜单状态
extern int menuIndex;  // 菜单当前选项索引
extern unsigned long menuStartTime;  // 菜单进入时间

//========================== WiFi配置状态 ==========================  // WiFi配置状态变量区域
extern bool configMode;  // 是否处于WiFi配网模式
extern unsigned long configStartTime;  // 配网开始时间
extern String newSSID;  // 新WiFi的SSID
extern String newPass;  // 新WiFi的密码
extern bool wifiConfigExiting;  // WiFi配置是否正在退出
extern bool configPortalExiting;  // 配网门户是否正在退出

//========================== 电台管理 ==========================  // 电台管理变量区域
extern String stationNames[MAX_STATIONS];  // 电台名称数组
extern String stationUrls[MAX_STATIONS];  // 电台URL数组

//========================== 编码器状态 ==========================  // 编码器状态变量区域
extern volatile int encoderPos;  // 编码器位置（音量值）
extern volatile bool encoderChanged;  // 编码器值是否改变
extern unsigned long lastEncoderTime;  // 上次编码器操作时间
extern bool showingVolume;  // 是否正在显示音量
extern unsigned long volumeDisplayTime;  // 音量显示开始时间
extern int lastVolume;  // 上次音量值
extern volatile int8_t encoderDelta;  // 编码器增量值
extern volatile bool buttonPressed;  // 编码器按钮是否按下
extern volatile unsigned long lastButtonPress;  // 上次按钮按下时间

//========================== 开机动画状态 ==========================  // 开机动画状态变量区域
extern bool bootScreenActive;  // 开机动画是否激活显示
extern unsigned long bootScreenStartTime;  // 开机动画开始时间
extern bool audioSystemReady;  // 音频系统是否就绪
extern bool skipBootAnimation;  // 是否跳过开机动画

//========================== SD卡播放 ==========================  // SD卡播放状态变量区域
extern bool sdPlaying;  // SD卡是否正在播放
extern bool sdPaused;  // SD卡是否暂停
extern int currentTrackIndex;  // 当前曲目索引
extern int totalTracks;  // 总曲目数
extern String* trackList;       // 曲目列表（动态分配，进入SD播放器时创建，退出时释放）
extern String currentTrackName;  // 当前曲目名称
extern unsigned long trackStartTime;  // 曲目开始播放时间
extern unsigned long trackDuration;  // 曲目总时长
extern int sdPlayerAnimType;  // 播放器动画类型

// SD扫描相关  // SD卡扫描相关变量区域
extern bool sdScanInProgress;  // 扫描是否进行中
extern bool sdScanComplete;  // 扫描是否完成
extern bool sdScanTaskCreated;  // 扫描任务是否已创建
extern bool sdScanTaskShouldStop;  // 扫描任务是否应该停止
extern unsigned long sdScanStartTime;  // 扫描开始时间
extern int sdScanProgress;  // 扫描进度百分比
extern int sdCheckedFiles;  // 已检查文件数
extern int sdEstimatedTotalFiles;  // 预估总文件数
extern int sdTotalMP3Files;  // 实际MP3文件数
extern String sdScanStatus;  // 扫描状态显示字符串

// SD退出相关  // SD卡退出相关变量区域
extern bool sdExitingInProgress;  // SD卡退出是否正在进行
extern unsigned long sdExitStartTime;  // 退出开始时间
extern int sdExitAnimationAngle;  // 退出动画角度
extern int clockGridAnimationPos;  // 时钟网格动画位置

// 并行扫描  // 并行扫描任务变量区域
extern TaskHandle_t scanTaskHandles[MAX_SCAN_TASKS];  // 扫描任务句柄数组
extern volatile int activeScanTasks;  // 当前活跃扫描任务数
extern SemaphoreHandle_t scanResultMutex;  // 扫描结果互斥锁
extern String* scanFoundPaths;  // 扫描发现的曲目路径（动态分配，与trackList同生命周期）
extern volatile int scanFoundCount;  // 扫描发现的文件数量

//========================== 播放时间 ==========================  // 播放时间变量区域
extern unsigned long currentPlayTime;  // 当前播放时间（秒）
extern unsigned long totalPlayTime;  // 总播放时间（秒）
extern unsigned long lastPlayTimeUpdate;  // 上次播放时间更新时间
extern bool timeInfoAvailable;  // 时间信息是否可用
extern PlayMode currentPlayMode;  // 当前播放模式
extern bool autoPlayEnabled;  // 是否启用自动播放

//========================== 流媒体连接 ==========================  // 流媒体连接状态变量区域
extern bool stationConnectingInProgress;  // 电台连接是否正在进行
extern unsigned long stationConnectStartTime;  // 连接开始时间
extern int stationConnectAnimationAngle;  // 连接动画角度
extern int stationConnectRetryCount;  // 连接重试次数
extern String connectingStationName;  // 正在连接的电台名称
extern String connectingStationUrl;  // 正在连接的电台URL
extern bool stationConnectFailed;  // 连接是否失败
extern bool forceStationConnectRetry;  // 是否强制重试连接
extern bool stationSwitching;  // 电台是否正在切换
extern unsigned long stationSwitchStartTime;  // 电台切换开始时间
extern int lastSelectedStationIndex;  // 上次选择的电台索引

//========================== 音频系统状态 ==========================  // 音频系统状态变量区域
extern bool audioSystemEnabled;  // 音频系统是否启用
extern bool audioLoopPaused;  // 音频循环是否暂停
extern bool audioSystemStopping;  // 音频系统是否正在停止
extern unsigned long audioStopStartTime;  // 音频停止开始时间
extern bool audioConnectionFailed;  // 音频连接是否失败
extern unsigned long lastConnectionAttempt;  // 上次连接尝试时间
extern int connectionRetryCount;  // 连接重试次数
extern bool wifiWasConnected;  // WiFi之前是否已连接
extern String savedSSID;  // 保存的WiFi名称
extern String savedPassword;  // 保存的WiFi密码

//========================== 音频质量监控 ==========================  // 音频质量监控变量区域
extern unsigned long lastAudioCheck;  // 上次音频检查时间
extern unsigned long audioBufferUnderruns;  // 音频缓冲区欠载次数
extern unsigned long audioBufferOverruns;  // 音频缓冲区过载次数
extern bool audioQualityWarning;  // 音频质量警告标志
extern String audioQualityStatus;  // 音频质量状态字符串

//========================== 电池检测 ==========================  // 电池检测变量区域
extern uint8_t batteryLevelPercent;  // 电池电量百分比
extern unsigned long lastBatteryCheck;  // 上次电池检查时间

//========================== OTA ==========================  // OTA升级变量区域
extern String currentVersion;  // 当前固件版本字符串
extern float currentVersionNum;  // 当前固件版本号（数值）
extern float serverVersionNum;  // 服务器固件版本号（数值）
extern String serverVersionStr;  // 服务器版本字符串
extern bool otaUpdateAvailable;  // 是否有可用更新
extern bool otaCheckCompleted;  // OTA检查是否完成
extern unsigned long otaCheckStartTime;  // OTA检查开始时间
extern int otaProgress;  // OTA升级进度百分比
extern bool otaWiFiConnect;      // OTA是否需要WiFi连接
extern bool naviWiFiConnect;     // Navidrome是否需要WiFi连接
extern bool alarmWiFiConnect;    // 音乐闹钟需要WiFi连接（连接后进入MENU_ALARM_CLOCK）
extern int otaCheckResult;  // OTA检查结果
extern int otaAnimAngle;  // OTA动画角度

//========================== CPU监控 ==========================  // CPU监控变量区域
extern volatile uint32_t idleCount0;  // CPU核心0空闲计数
extern volatile uint32_t idleCount1;  // CPU核心1空闲计数
extern float cpuUsage;  // CPU使用率

//========================== SD互斥锁 ==========================  // SD卡互斥锁区域
extern SemaphoreHandle_t sdMutex;  // SD卡访问互斥量

//========================== PONG游戏 ==========================  // PONG游戏变量区域
extern int ballX, ballY;  // 球的X,Y坐标
extern int ballDX, ballDY;  // 球的X,Y速度
extern int leftPaddleY;  // 左挡板Y坐标
extern int rightPaddleY;  // 右挡板Y坐标
extern int leftScore, rightScore;  // 左,右玩家分数
extern bool playerMode;  // 是否为玩家模式
extern int gameTick;  // 游戏帧计数器

//========================== 滚动状态 ==========================  // 滚动显示状态区域
extern bool scrollStateResetFlag;  // 滚动状态重置标志

//========================== OTA URL ==========================  // OTA升级URL区域
extern String OTA_VERSION_URL;  // OTA版本检查URL（从NVS加载，可通过Web配置）
extern String OTA_FIRMWARE_URL;  // OTA固件下载URL（从NVS加载，可通过Web配置）

//========================== Navidrome播放器 ==========================  // Navidrome相关变量区域
enum NaviState {  // Navidrome播放状态枚举
  NAVI_IDLE,           // 空闲，未播放
  NAVI_CONNECTING,     // 正在连接服务器
  NAVI_PLAYING,        // 正在播放
  NAVI_PAUSED,         // 已暂停
  NAVI_ERROR,          // 发生错误
  NAVI_LOADING         // 正在加载歌单
};

struct NaviSong {  // 歌曲信息结构体
  String id;        // 歌曲ID
  String title;     // 歌曲标题
  String artist;    // 歌手名称
  String album;     // 专辑名称
  int    duration;  // 时长（秒）
};

#define ND_MAX_SONGS 10  // 随机歌曲列表最大条数

extern String      naviServer;            // Navidrome服务器地址（host:port）
extern String      naviUser;              // Navidrome用户名
extern String      naviPass;              // Navidrome密码
extern bool        naviConfigValid;       // Navidrome配置是否完整有效
extern NaviState   naviState;             // Navidrome播放状态
extern NaviSong    naviSongList[ND_MAX_SONGS];  // 随机歌曲列表
extern int         naviSongCount;         // 已加载的歌曲总数
extern int         naviCurrentIdx;        // 当前播放歌曲的索引
extern String      naviStatusMsg;         // 显示在OLED的状态提示文字
extern unsigned long naviTrackStartTime;  // 当前曲目开始播放的时间戳（ms）
extern int         naviCurrentDuration;   // 当前曲目总时长（秒）
extern bool        naviPlaying;           // Navidrome是否处于播放状态
extern unsigned long naviCurrentPlaySec;  // 已播放秒数（实时计算）
extern bool        naviNeedsConfig;      // Navidrome需要配置WEB服务
extern bool        startInNaviMode;     // 重启后进入Navidrome模式
extern volatile bool naviStartRequested; // 请求在主任务中执行 naviStart()（避免在OLED Task栈中调用HTTPS）
extern volatile bool naviPlayRetryRequested; // Navidrome播放失败，请求重试
extern volatile bool naviReleasingMemory;    // 正在用假连接触发buffers freed，屏蔽aggressiveMemoryCleanup

//========================== 音乐闹钟 ==========================
extern int   alarmHour;            // 闹钟小时（0-23）
extern int   alarmMinute;          // 闹钟分钟（0-59）
extern bool  alarmEnabled;         // 闹钟是否启用
extern int   alarmSource;          // 音乐来源（0=MP3 1=IMP 2=Navidrome）
extern bool  alarmRepeat;          // 每天重复（true=每天触发，false=单次触发）
extern bool  alarmRepeatActive;    // 重复模式激活中：正在播放，等待用户操作返回闹钟
extern bool  alarmTriggered;       // 闹钟触发标志
extern int   alarmLastTriggerMin;  // 上次触发的分钟值（防重复触发）
// 时间设置临时变量（进入设置界面时使用）
extern int   alarmEditHour;        // 正在编辑的小时
extern int   alarmEditMin;         // 正在编辑的分钟
extern int   alarmEditField;       // 当前编辑字段（0=小时 1=分钟 2=确认）

#endif // GLOBALS_H  // 结束头文件保护