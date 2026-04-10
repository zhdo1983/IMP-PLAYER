#ifndef ALARM_CLOCK_H
#define ALARM_CLOCK_H

// ============================================================
// 音乐闹钟模块（含诊断日志版）
// ============================================================

#include "globals.h"
#include "config.h"

// ---- NVS 键名 ----
#define ALARM_NVS_HOUR    "alm_hour"
#define ALARM_NVS_MIN     "alm_min"
#define ALARM_NVS_ENABLED "alm_en"
#define ALARM_NVS_SOURCE  "alm_src"
#define ALARM_NVS_REPEAT  "alm_rep"

// ============================================================
// ★ 诊断开关 — 上线调试期间设为 true，确认稳定后改回 false
// ============================================================
#define ALARM_DEBUG true

// 诊断日志宏（只在 ALARM_DEBUG=true 时编译）
#if ALARM_DEBUG
  #define ALARM_LOG(fmt, ...)  Serial.printf("[ALARM-DBG] " fmt "\n", ##__VA_ARGS__)
#else
  #define ALARM_LOG(fmt, ...)
#endif

// 定期内存清理间隔（3分钟）
#define ALARM_MEMORY_CLEANUP_INTERVAL_MS 180000UL  // 3 * 60 * 1000
static unsigned long lastAlarmMemoryCleanup = 0;

// ============================================================
// 诊断：定期心跳（每 60 秒打印一次系统健康信息）
// 在 loop() 内调用 alarmHeartbeat()，或由 checkAlarmTrigger() 内嵌调用
// ============================================================
void alarmHeartbeat() {
#if ALARM_DEBUG
  static unsigned long _lastHeartbeat = 0;
  unsigned long now = millis();
  if (now - _lastHeartbeat < 60000UL) return;
  _lastHeartbeat = now;

  // ---- 基础内存 ----
  uint32_t freeHeap    = ESP.getFreeHeap();
  uint32_t maxAlloc    = ESP.getMaxAllocHeap();
  uint32_t minFreeHeap = ESP.getMinFreeHeap();

  // ---- WiFi 状态 ----
  bool wifiConn  = (WiFi.status() == WL_CONNECTED);
  int  rssi      = wifiConn ? WiFi.RSSI() : 0;

  // ---- NTP 时间 ----
  struct tm ti;
  bool gotTime = getLocalTime(&ti, 0);

  // ---- 音频系统 ----
  int  bufFilled = audio.inBufferFilled();
  int  bufFree   = audio.inBufferFree();

  Serial.println(F("====== [ALARM HEARTBEAT] ======"));
  Serial.printf("  Uptime     : %lu s\n",       now / 1000);
  Serial.printf("  Free heap  : %u KB\n",       freeHeap / 1024);
  Serial.printf("  MinFree    : %u KB\n",       minFreeHeap / 1024);
  Serial.printf("  MaxAlloc   : %u KB\n",       maxAlloc / 1024);
  Serial.printf("  WiFi       : %s  RSSI=%d dBm\n", wifiConn ? "OK" : "DOWN", rssi);
  Serial.printf("  NTP time   : %s  %s\n",
                gotTime ? "OK" : "FAIL",
                gotTime ? asctime(&ti) : "---");
  Serial.printf("  audioLoop  : paused=%d  bufFilled=%d  bufFree=%d\n",
                (int)audioLoopPaused, bufFilled, bufFree);
  Serial.printf("  menuState  : %d\n", (int)menuState);
  Serial.printf("  alarmEn    : %d  %02d:%02d  src=%d  repeat=%d\n",
                (int)alarmEnabled, alarmHour, alarmMinute,
                alarmSource, (int)alarmRepeat);
  Serial.printf("  lastTrigMin: %d\n", alarmLastTriggerMin);

  // ---- FreeRTOS 任务栈水位（诊断关键：判断栈是否即将溢出）----
  // uxTaskGetStackHighWaterMark(NULL) = 当前任务（loop/main task）
  Serial.printf("  Stack(loop): %u words remaining\n",
                (unsigned)uxTaskGetStackHighWaterMark(NULL));

  Serial.println(F("============================================================"));
#endif
}

// ---- 从 NVS 加载闹钟配置 ----
void loadAlarmConfig() {
    alarmHour    = preferences.getInt(ALARM_NVS_HOUR,    7);
    alarmMinute  = preferences.getInt(ALARM_NVS_MIN,     0);
    alarmEnabled = preferences.getBool(ALARM_NVS_ENABLED, false);
    alarmSource  = preferences.getInt(ALARM_NVS_SOURCE,  0);
    alarmRepeat  = preferences.getBool(ALARM_NVS_REPEAT, false);
    Serial.printf("[Alarm] Loaded: %02d:%02d enabled=%d src=%d repeat=%d\n",
                  alarmHour, alarmMinute, alarmEnabled, alarmSource, alarmRepeat);
}

// ---- 将闹钟配置保存到 NVS ----
void saveAlarmConfig() {
    preferences.putInt(ALARM_NVS_HOUR,    alarmHour);
    preferences.putInt(ALARM_NVS_MIN,     alarmMinute);
    preferences.putBool(ALARM_NVS_ENABLED, alarmEnabled);
    preferences.putInt(ALARM_NVS_SOURCE,  alarmSource);
    preferences.putBool(ALARM_NVS_REPEAT, alarmRepeat);
    Serial.printf("[Alarm] Saved: %02d:%02d enabled=%d src=%d repeat=%d\n",
                  alarmHour, alarmMinute, alarmEnabled, alarmSource, alarmRepeat);
}

// ---- 音乐来源名称（用于显示） ----
const char* alarmSourceName(int src) {
    switch (src) {
        case 0: return "MP3播放器";
        case 1: return "IMP播放器";
        case 2: return "Navidrome";
        default: return "MP3播放器";
    }
}

// ---- 触发闹钟：根据 alarmSource 启动对应播放器 ----
void triggerAlarm() {
    Serial.printf("[Alarm] TRIGGERED! source=%d repeat=%d\n", alarmSource, alarmRepeat);
    ALARM_LOG("triggerAlarm(): freeHeap=%u KB  maxAlloc=%u KB  WiFi=%s",
              ESP.getFreeHeap() / 1024, ESP.getMaxAllocHeap() / 1024,
              (WiFi.status() == WL_CONNECTED) ? "OK" : "DOWN");

    // 触发前统一清理内存
    Serial.println("[Alarm] Pre-alarm memory cleanup...");
    aggressiveMemoryCleanup();

    alarmTriggered = false;

    if (alarmRepeat) {
        alarmRepeatActive = true;
        alarmLastTriggerMin = alarmMinute;
        Serial.println("[Alarm] Repeat mode: will return to alarm clock on user input");
    } else {
        alarmEnabled = false;
        alarmRepeatActive = false;
        saveAlarmConfig();
    }

    if (alarmSource == 0) {
        // ── MP3 播放器 ──────────────────────────────────────────
        audio.stopSong();
        audioLoopPaused = true;
        delay(200);
        disconnectWiFiForSDPlayback();
        delay(200);
        encoderPos = DEFAULT_VOLUME;
        audio.setVolume(encoderPos);

        if (!sdScanTaskCreated) {
            if (xTaskCreatePinnedToCore(sdScanTask, "SD Scan Task", 8192, NULL, 1, NULL, 1) == pdPASS)
                sdScanTaskCreated = true;
        }
        if ((!sdScanComplete && !sdScanInProgress) || (sdScanComplete && totalTracks == 0)) {
            sdScanInProgress = true;
            sdScanComplete   = false;
            sdScanStartTime  = millis();
            sdScanProgress   = 0;
            sdScanStatus     = "Starting scan...";
            menuState        = MENU_SD_LOADING;
        } else if (sdScanComplete && totalTracks > 0) {
            safeRestartAudioSystem();
            delay(100);
            int idx = (millis() % totalTracks);
            if (idx == 0) idx = 1;
            playTrack(idx);
            digitalWrite(I2S_SD_MODE, 1);
            menuState  = MENU_SD_PLAYER;
            loadSDAnimationType();
            sdPlaying  = true;
            sdPaused   = false;
        } else {
            menuState = MENU_SD_LOADING;
        }
        menuIndex     = 0;
        menuStartTime = millis();

    } else if (alarmSource == 1) {
        // ── IMP（网络）播放器 ────────────────────────────────────
        sdScanInProgress = false;
        encoderPos = DEFAULT_VOLUME;
        audio.setVolume(encoderPos);
        digitalWrite(I2S_SD_MODE, 1);
        menuState  = MENU_WIFI_CONNECTING;
        menuIndex  = 0;
        menuStartTime = millis();
        stationConnectAnimationAngle = random(0, 12) * 30;

    } else {
        // ── Navidrome ────────────────────────────────────────────
        safeStopAudioSystem();
        delay(100);
        safeRestartAudioSystem();
        encoderPos = DEFAULT_VOLUME;
        audio.setVolume(encoderPos);
        digitalWrite(I2S_SD_MODE, 1);
        naviPlaying   = false;
        naviSongCount = 0;
        loadSDAnimationType();
        menuState   = MENU_NAVI_PLAYER;
        menuIndex   = 0;
        menuStartTime = millis();
        naviState   = NAVI_CONNECTING;
        strncpy(stationString, "Navidrome", MAX_INFO_LENGTH - 1);
        strncpy(infoString,    "连接中...", MAX_INFO_LENGTH - 1);
    }
}

// ---- 在 loop() 中每帧调用，检查是否到达闹钟时间 ----
void checkAlarmTrigger() {
    // 心跳诊断 + 定期内存清理（每 60 秒，不受 alarmEnabled 限制，在闹钟界面始终运行）
    if (menuState == MENU_ALARM_CLOCK) {
        alarmHeartbeat();

        // 定期内存清理（每3分钟）
        unsigned long now = millis();
        if (now - lastAlarmMemoryCleanup > ALARM_MEMORY_CLEANUP_INTERVAL_MS) {
            lastAlarmMemoryCleanup = now;
            Serial.println("[Alarm] Periodic memory cleanup (3min)...");
            aggressiveMemoryCleanup();
            Serial.printf("[Alarm] After cleanup: freeHeap=%u KB\n", ESP.getFreeHeap() / 1024);
        }
    }

    if (!alarmEnabled) return;
    if (menuState != MENU_ALARM_CLOCK) return;

    struct tm timeinfo;
    // ★ 修复：使用 timeout=0 非阻塞调用，与 oled_task 保持一致
    //   原始代码使用默认 timeout（可能阻塞 ~5s），在 loop() 中每帧调用会
    //   造成 loop() 周期被拉长，WDT 超时风险升高，同时 audio.loop() 无法
    //   得到调度，长期运行后导致死机
    if (!getLocalTime(&timeinfo, 0)) {
        ALARM_LOG("getLocalTime() failed (NTP not synced yet)");
        return;
    }

    // 每天 00:00 重置上次触发记录，确保重复模式次日可再次触发
    if (timeinfo.tm_hour == 0 && timeinfo.tm_min == 0 && alarmLastTriggerMin != -2) {
        if (alarmLastTriggerMin != -1) {
            ALARM_LOG("Midnight reset of alarmLastTriggerMin");
        }
        alarmLastTriggerMin = -1;
    }

    if (timeinfo.tm_hour == alarmHour &&
        timeinfo.tm_min  == alarmMinute &&
        timeinfo.tm_min  != alarmLastTriggerMin) {
        alarmLastTriggerMin = timeinfo.tm_min;
        alarmTriggered      = true;
        ALARM_LOG("Alarm matched! Time=%02d:%02d  Triggering...",
                  timeinfo.tm_hour, timeinfo.tm_min);
    }
}

#endif // ALARM_CLOCK_H