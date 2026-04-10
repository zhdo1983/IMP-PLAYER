#ifndef MENU_H // 防止头文件重复包含
#define MENU_H // 定义 MENU_H 宏

#include "globals.h" // 引入全局变量和常量头文件
#include "display.h" // 引入显示相关头文件
#include "stations.h" // 引入电台相关头文件
#include "audio_manager.h" // 引入音频管理头文件
#include "sd_player.h" // 引入 SD 卡播放器头文件
#include "ota.h" // 引入 OTA 升级头文件
#include "wifi_config.h" // 引入 WiFi 配置头文件
#include "navidrome.h" // 引入 Navidrome 播放器头文件
#include "alarm_clock.h" // 引入音乐闹钟头文件

// ---- 内存信息辅助 ----
struct MemoryInfo { uint32_t totalHeap, freeHeap, usedHeap, wifiMemory, sslMemory, audioMemory; }; // 定义内存信息结构体，包含各类型内存使用情况

MemoryInfo getMemoryInfo() { // 获取内存信息的函数
  MemoryInfo info; // 创建内存信息结构体变量
  info.totalHeap = ESP.getHeapSize(); // 获取总堆内存大小
  info.freeHeap  = ESP.getFreeHeap(); // 获取空闲堆内存大小
  info.usedHeap  = info.totalHeap - info.freeHeap; // 计算已使用堆内存大小
  info.wifiMemory  = (WiFi.status() == WL_CONNECTED) ? 40 : 0; // WiFi 已连接则分配 40KB 内存，否则为 0
  info.sslMemory   = (audio.inBufferFilled() > 0) ? 18 : 0; // 音频缓冲区有数据则分配 18KB 内存，否则为 0
  info.audioMemory = 32; // 音频系统固定使用 32KB 内存
  return info; // 返回内存信息结构体
}

// ---- 安全电台切换（混合方案：优先不重启） ----
bool safeStationSwitch(int stationIndex) { // 安全切换电台函数，优先不重启
  int realIdx = getValidStationIndex(stationIndex); // 获取有效的电台索引
  if (realIdx < 0) return false; // 如果索引无效则返回 false

  // 保存电台信息（重启时可能用到）
  preferences.putString("stName", stationNames[realIdx]); // 保存电台名称到偏好设置
  preferences.putString("stUrl",  stationUrls[realIdx]); // 保存电台 URL 到偏好设置

  connectingStationName = stationNames[realIdx]; // 保存连接的电台名称
  connectingStationUrl = stationUrls[realIdx]; // 保存连接的电台URL

  // 使用新的过渡状态，而不是重启模式
  menuState = MENU_STATION_SWITCHING; // 设置菜单状态为电台切换过渡
  menuStartTime = millis(); // 记录菜单开始时间

  // 从0开始，不是255（255表示重启模式）
  stationConnectRetryCount = 0; // 设置重试计数为0
  stationConnectAnimationAngle = random(0, 12) * 30; // 随机生成连接动画角度

  stationConnectingInProgress = true; // 标记电台连接进行中
  stationConnectStartTime = millis(); // 记录连接开始时间

  strcpy(infoString,    "准备切换..."); // 复制提示信息到字符串
  strcpy(stationString, stationNames[realIdx].c_str()); // 复制电台名称到字符串

  // 立即停止音频
  audio.stopSong(); // 停止当前播放
  delay(100); // 延迟100毫秒

  return true; // 返回成功
}

// ---- 电台切换处理（混合方案） ----
void processStationSwitching() { // 电台切换处理函数
  if (menuState != MENU_STATION_SWITCHING) return; // 如果不在切换状态则返回

  unsigned long elapsed = millis() - stationConnectStartTime; // 计算已过时间

  // 阶段1：检查内存（0-500ms）
  if (stationConnectRetryCount == 0 && elapsed < 500) {
    strcpy(infoString, "检查内存...");
    return; // 返回
  }

  // 阶段2：内存清理（500-1500ms）
  if (stationConnectRetryCount == 0 && elapsed >= 500 && elapsed < 1500) {
    if (elapsed < 600) { // 在600ms时执行内存清理
      enhancedMemoryCleanupForStationSwitch(); // 执行增强的内存清理
    }
    strcpy(infoString, "清理内存...");
    return; // 返回
  }

  // 阶段3：连接电台（1500ms后）
  if (stationConnectRetryCount == 0 && elapsed >= 1500) {
    // 检查内存是否足够
    uint32_t maxHeap = ESP.getMaxAllocHeap(); // 获取最大可用堆内存
    if (maxHeap < 20480) { // 如果内存不足20KB
      // 内存严重不足，需要重启
      preferences.putBool("rstSta", true); // 设置重启标志
      stationConnectRetryCount = 255; // 重启模式
      strcpy(infoString, "内存不足，准备重启..."); // 复制提示信息
      if (elapsed >= 2000) ESP.restart(); // 等待2秒后重启
      return; // 返回
    }

    // 内存足够，直接连接
    safeStopAudioSystem(); // 安全停止音频系统
    delay(100); // 延迟100毫秒
    safeRestartAudioSystem(); // 安全重启音频系统
    digitalWrite(I2S_SD_MODE, 1); // 设置I2S SD模式引脚为高电平

    audio.connecttohost(connectingStationUrl.c_str()); // 连接到电台URL
    stationConnectRetryCount = 1; // 下一次检查
    strcpy(infoString, "连接电台..."); // 复制连接提示
    return; // 返回
  }

  // 阶段4：等待连接结果（2000ms后）
  if (stationConnectRetryCount == 1 && elapsed >= 2000) {
    if (audioConnectionFailed) { // 如果连接失败
      // 连接失败，激进重试
      stationConnectRetryCount = 2; // 设置重试计数为2
      stationConnectStartTime = millis(); // 重置计时
      aggressiveMemoryCleanup(); // 执行激进的内存清理
      audio.connecttohost(connectingStationUrl.c_str()); // 重新连接
      strcpy(infoString, "重试连接..."); // 复制重试提示
      return; // 返回
    }

    // 连接成功
    stationConnectingInProgress = false; // 标记连接完成
    menuState = MENU_NONE; // 设置菜单状态为无
    return; // 返回
  }

  // 阶段5：重试后仍失败（8000ms后）
  if (stationConnectRetryCount == 2 && elapsed >= 8000) {
    if (audioConnectionFailed) { // 如果连接失败
      // 多次失败，准备重启
      preferences.putBool("rstSta", true); // 设置重启标志
      stationConnectRetryCount = 255; // 重启模式
      strcpy(infoString, "准备重启..."); // 复制重启提示
      if (elapsed >= 9000) ESP.restart(); // 等待9秒后重启
      return; // 返回
    }

    // 重试成功
    stationConnectingInProgress = false; // 标记连接完成
    menuState = MENU_NONE; // 设置菜单状态为无
    return; // 返回
  }

  // 超时（15秒）
  if (elapsed > 15000) { // 如果超过15秒
    stationConnectingInProgress = false; // 标记连接完成
    stationConnectFailed = true; // 标记连接失败
    menuState = MENU_NONE; // 设置菜单状态为无
    strcpy(infoString, "切换超时"); // 复制超时提示
    strcpy(stationString, "请重试"); // 复制重试提示
  }
}

// ---- 电台切换超时检查 ----
void checkStationSwitchTimeout() { // 检查电台切换是否超时的函数
  if (stationSwitching && (millis() - stationSwitchStartTime > STATION_SWITCH_TIMEOUT)) { // 如果正在切换电台且超时
    stationSwitching = false; // 标记切换结束
    safeRestartAudioSystem(); // 安全重启音频系统
    digitalWrite(I2S_SD_MODE, 1); // 设置 I2S SD 模式引脚为高电平
  }
}

// ---- 流媒体源连接处理（在 loop() 中调用） ----
void processStationConnection() { // 处理电台连接的函数，在主循环中调用
  if (!stationConnectingInProgress) return; // 如果没有正在进行的连接则直接返回

  // 重启模式：等 2 秒后重启
  if (stationConnectRetryCount == 255) { // 如果是重启模式（特殊值 255）
    if (millis() - stationConnectStartTime >= 2000) ESP.restart(); // 等待 2 秒后重启 ESP
    return; // 返回
  }

  // 强制重试
  if (forceStationConnectRetry && stationConnectRetryCount <= 2) { // 如果强制重试且重试次数不超过 2 次
    forceStationConnectRetry = false; // 清除强制重试标志
    safeStopAudioSystem(); // 安全停止音频系统
    unsigned long cs = millis(); // 记录当前时间
    while (audioSystemStopping && millis() - cs < 3000) { delay(10); esp_task_wdt_reset(); } // 等待音频系统停止，最多 3 秒
    audioSystemStopping = false; // 标记音频系统停止完成
    aggressiveMemoryCleanup(); // 执行激进的内存清理
    safeRestartAudioSystem(); // 安全重启音频系统
    audio.connecttohost(connectingStationUrl.c_str()); // 连接到电台 URL
    stationConnectStartTime = millis(); // 重置连接开始时间
    stationConnectAnimationAngle = random(0, 12) * 30; // 随机生成连接动画角度
    strcpy(infoString, "重试连接..."); // 复制重试提示信息
    return; // 返回
  }

  if (stationConnectRetryCount >= 3) { // 如果重试次数达到 3 次
    stationConnectFailed = true; // 标记连接失败
    stationConnectingInProgress = false; // 标记连接结束
    menuState = MENU_NONE; // 设置菜单状态为无
    strcpy(infoString, "连接失败，请更换源"); // 复制失败提示信息
    return; // 返回
  }

  unsigned long elapsed = millis() - stationConnectStartTime; // 计算已过时间

  if (elapsed > STATION_CONNECT_TIMEOUT) { // 如果已超过连接超时时间
    stationConnectFailed = true; // 标记连接失败
    stationConnectingInProgress = false; // 标记连接结束
    menuState = MENU_NONE; // 设置菜单状态为无
    return; // 返回
  }

  if (stationConnectRetryCount == 0 && elapsed < 2000) return; // 如果首次尝试且时间不足 2 秒则返回

  if (stationConnectRetryCount == 0 && elapsed >= 2000) { // 如果首次尝试且时间达到 2 秒
    if (ESP.getMaxAllocHeap() < 25600) { // 如果最大可用堆内存小于 25600 字节
      enhancedMemoryCleanupForStationSwitch(); // 执行增强的内存清理
      if (ESP.getMaxAllocHeap() < 20480) { // 如果清理后最大可用堆内存仍小于 20480 字节
        stationConnectFailed = true; // 标记连接失败
        stationConnectingInProgress = false; // 标记连接结束
        menuState = MENU_NONE; // 设置菜单状态为无
        return; // 返回
      }
    }
    safeStopAudioSystem(); // 安全停止音频系统
    unsigned long cs = millis(); // 记录当前时间
    while (audioSystemStopping && millis() - cs < 3000) { delay(10); esp_task_wdt_reset(); } // 等待音频系统停止，最多 3 秒
    if (audioSystemStopping) audioSystemStopping = false; // 如果仍在停止则强制标记为停止
    safeRestartAudioSystem(); // 安全重启音频系统
    digitalWrite(I2S_SD_MODE, 1); // 设置 I2S SD 模式引脚为高电平
    audio.connecttohost(connectingStationUrl.c_str()); // 连接到电台 URL
    stationConnectRetryCount = 1; // 设置重试次数为 1
  } else if (stationConnectRetryCount == 1 && elapsed >= 8000 && audioConnectionFailed) { // 如果第 1 次重试且时间达到 8 秒且连接失败
    safeStopAudioSystem(); // 安全停止音频系统
    unsigned long cs = millis(); // 记录当前时间
    while (audioSystemStopping && millis() - cs < 3000) { delay(10); esp_task_wdt_reset(); } // 等待音频系统停止，最多 3 秒
    if (audioSystemStopping) audioSystemStopping = false; // 如果仍在停止则强制标记为停止
    aggressiveMemoryCleanup(); // 执行激进的内存清理
    safeRestartAudioSystem(); // 安全重启音频系统
    audio.connecttohost(connectingStationUrl.c_str()); // 连接到电台 URL
    stationConnectRetryCount = 2; // 设置重试次数为 2
    audioConnectionFailed = false; // 清除连接失败标志
  } else if (stationConnectRetryCount == 2 && elapsed >= 12000) { // 如果第 2 次重试且时间达到 12 秒
    stationConnectFailed = true; // 标记连接失败
    stationConnectingInProgress = false; // 标记连接结束
    menuState = MENU_NONE; // 设置菜单状态为无
    digitalWrite(I2S_SD_MODE, 1); // 设置 I2S SD 模式引脚为高电平
    strcpy(infoString,    "Connection failed"); // 复制连接失败提示信息
    strcpy(stationString, "Connection failed"); // 复制连接失败提示信息
  }
}

// ---- 按钮处理 ----
void handleMenuButton() { // 处理菜单按钮的函数
  switch (menuState) { // 根据菜单状态进行分支处理

    case MENU_NONE: // 无菜单状态
      menuState = MENU_MAIN; // 切换到主菜单
      menuIndex = 0; // 重置菜单索引
      menuStartTime = millis(); // 记录菜单开始时间
      break; // 跳出 switch

    case MENU_PLAYER_SELECT: // 播放器选择菜单
      if (menuIndex == 0) { // 如果选择的是第一个选项
        // MP3 播放器
        audio.stopSong(); audioLoopPaused = true; delay(200); // 停止当前歌曲，暂停循环，设置延迟
        disconnectWiFiForSDPlayback(); delay(200); // 断开 WiFi 以便 SD 卡播放，设置延迟
        encoderPos = DEFAULT_VOLUME; audio.setVolume(encoderPos);

        if (!sdScanTaskCreated) { // 如果 SD 卡扫描任务未创建
          if (xTaskCreatePinnedToCore(sdScanTask,"SD Scan Task",8192,NULL,1,NULL,1) == pdPASS) // 创建 SD 卡扫描任务
            sdScanTaskCreated = true; // 标记 SD 卡扫描任务已创建
        }

        if ((!sdScanComplete && !sdScanInProgress) || (sdScanComplete && totalTracks == 0)) { // 如果未扫描或扫描完成但无曲目
          sdScanInProgress = true; sdScanComplete = false; // 标记扫描进行中，未完成
          sdScanStartTime = millis(); sdScanProgress = 0; // 记录扫描开始时间，重置扫描进度
          sdScanStatus = "Starting scan..."; // 设置扫描状态文本
          menuState = MENU_SD_LOADING; // 切换到 SD 卡加载菜单
        } else if (sdScanComplete && totalTracks > 0) { // 如果扫描完成且有曲目
          safeRestartAudioSystem(); delay(100); // 安全重启音频系统，设置延迟
          int idx = (millis() % totalTracks); if (idx == 0) idx = 1; // 随机选择一首曲目
          playTrack(idx); // 播放选中的曲目
          digitalWrite(I2S_SD_MODE, 1); // 设置 I2S SD 模式引脚为高电平
          menuState = MENU_SD_PLAYER; // 切换到 SD 卡播放器菜单
          loadSDAnimationType(); // 加载 SD 卡播放器动画类型
          sdPlaying = true; sdPaused = false; // 标记正在播放，未暂停
        } else if (sdScanInProgress) { // 如果扫描正在进行中
          menuState = MENU_SD_LOADING; // 切换到 SD 卡加载菜单
        }

      } else if (menuIndex == 1) { // 如果选择的是第二个选项
        // 网络播放器
        sdScanInProgress = false; // 标记 SD 卡扫描未进行
        encoderPos = DEFAULT_VOLUME; audio.setVolume(encoderPos);
        digitalWrite(I2S_SD_MODE, 1); // 设置 I2S SD 模式引脚为高电平
        menuState = MENU_WIFI_CONNECTING; // 先切换状态，让OLED显示扫描信息
        menuStartTime = millis(); // 记录开始时间
        stationConnectAnimationAngle = random(0, 12) * 30; // 随机生成连接动画角度
        strcpy(stationString, "正在扫描WiFi...");
        strcpy(infoString, "请稍候");
        // WiFi初始化在loop()的pollWiFiConnection()中执行

      } else if (menuIndex == 2) { // 如果选择的是第三个选项（OTA）
        // OTA 升级
        currentVersion = getFirmwareVersion(); // 获取当前固件版本
        currentVersionNum = parseVersionNumber(currentVersion); // 解析版本号
        otaCheckCompleted = false; otaUpdateAvailable = false; // 重置 OTA 检查状态
        serverVersionNum = 0.0; serverVersionStr = ""; // 重置服务器版本信息
        otaCheckResult = 0; // 重置 OTA 检查结果

        if (WiFi.status() != WL_CONNECTED) { // 如果 WiFi 未连接
          otaWiFiConnect = true; // 标记需要连接 WiFi
          menuState = MENU_WIFI_CONNECTING; // 切换到 WiFi 连接菜单
          stationConnectAnimationAngle = random(0, 12) * 30; // 随机生成连接动画角度
        } else { // 如果 WiFi 已连接
          menuState = MENU_OTA_CHECK; // 切换到 OTA 检查菜单
          menuIndex = 0; // 重置菜单索引
          otaCheckStartTime = millis(); // 记录 OTA 检查开始时间
        }

      } else if (menuIndex == 3) { // 如果选择的是第四个选项（Navidrome）
        // Navidrome 网络音乐播放器
        safeStopAudioSystem(); // 停止音频系统
        delay(100); // 延迟
        safeRestartAudioSystem(); // 重启音频系统
        encoderPos = DEFAULT_VOLUME; audio.setVolume(encoderPos);
        digitalWrite(I2S_SD_MODE, 1); // 使能功放
        naviPlaying = false; // 重置播放状态
        naviSongCount = 0;   // 重置歌单
        loadSDAnimationType(); // 加载动画类型

        // 如果WiFi未连接，先启动WiFi连接
        if (WiFi.status() != WL_CONNECTED) {
          naviWiFiConnect = true;  // 标记WiFi连接后需要进入Navidrome
          menuState = MENU_WIFI_CONNECTING; // 切换到WiFi连接状态
          menuStartTime = millis();
          stationConnectAnimationAngle = random(0, 12) * 30;
          strcpy(stationString, "正在连接WiFi...");
          strcpy(infoString, "请稍候");
        } else {
          menuState = MENU_NAVI_PLAYER; // 进入Navidrome播放器界面
          menuStartTime = millis();
          naviState = NAVI_CONNECTING; // 标记正在连接，oled_task会调用naviStart()
          strncpy(stationString, "Navidrome", MAX_INFO_LENGTH - 1);
          strncpy(infoString, "连接中...", MAX_INFO_LENGTH - 1);
        }

      } else if (menuIndex == 4) { // 如果选择的是第五个选项（音乐闹钟）
        // 音乐闹钟 — 需要WiFi+NTP才能显示时间
        if (WiFi.status() != WL_CONNECTED) {
          // WiFi未连接，先走连接流程，连上后再进入闹钟界面
          alarmWiFiConnect = true;
          menuState = MENU_WIFI_CONNECTING;
          menuStartTime = millis();
          stationConnectAnimationAngle = random(0, 12) * 30;
          strcpy(stationString, "正在连接WiFi...");
          strcpy(infoString, "请稍候");
        } else {
          // WiFi已连接，直接进入并同步NTP
          setupNTP();
          menuState = MENU_ALARM_CLOCK;
          digitalWrite(I2S_SD_MODE, 0);  // 关闭功放省电
        }
      }
      menuIndex = 0; // 重置菜单索引
      menuStartTime = millis(); // 记录菜单开始时间
      break; // 跳出 switch

    case MENU_MAIN: // 主菜单
      if (menuIndex == 0) { // 如果选择的是第一个选项（电台选择）
        safeStopAudioSystem(); // 安全停止音频系统
        menuState = MENU_STATION_SELECT; // 切换到电台选择菜单
        menuIndex = 0; menuStartTime = millis(); // 重置菜单索引和开始时间

      } else if (menuIndex == 1) { // 如果选择的是第二个选项（SD 卡播放器）
        audio.stopSong(); audioLoopPaused = true; delay(200); // 停止当前歌曲，暂停循环，设置延迟

        if ((!sdScanComplete && !sdScanInProgress) || (sdScanComplete && totalTracks == 0)) { // 如果未扫描或扫描完成但无曲目
          safeStopAudioSystem(); delay(200); // 安全停止音频系统，设置延迟
          disconnectWiFiForSDPlayback(); // 断开 WiFi 以便 SD 卡播放
          if (!sdScanTaskCreated) { // 如果 SD 卡扫描任务未创建
            if (xTaskCreatePinnedToCore(sdScanTask,"SD Scan Task",8192,NULL,1,NULL,1) == pdPASS) // 创建 SD 卡扫描任务
              sdScanTaskCreated = true; // 标记 SD 卡扫描任务已创建
          }
          sdScanInProgress = true; sdScanComplete = false; // 标记扫描进行中，未完成
          sdScanStartTime = millis(); sdScanProgress = 0; // 记录扫描开始时间，重置扫描进度
          sdScanStatus = "Starting scan..."; // 设置扫描状态文本
          menuState = MENU_SD_LOADING; // 切换到 SD 卡加载菜单

        } else if (sdScanComplete) { // 如果扫描完成
          if (totalTracks > 0) { // 如果有曲目
            safeRestartAudioSystem(); delay(100); // 安全重启音频系统，设置延迟
            int idx = (millis() % totalTracks); if (idx == 0) idx = 1; // 随机选择一首曲目
            playTrack(idx); // 播放选中的曲目
            digitalWrite(I2S_SD_MODE, 1); // 设置 I2S SD 模式引脚为高电平
            menuState = MENU_SD_PLAYER; // 切换到 SD 卡播放器菜单
            loadSDAnimationType(); // 加载 SD 卡播放器动画类型
            encoderPos = constrain(encoderPos, 0, 21); // 限制编码器位置在 0-21 范围内
            sdPlaying = true; sdPaused = false; // 标记正在播放，未暂停
          } else { // 如果没有曲目
            menuState = MENU_SD_PLAYER; // 切换到 SD 卡播放器菜单
            loadSDAnimationType(); // 加载 SD 卡播放器动画类型
            sdPlaying = false; sdPaused = false; // 标记未播放，未暂停
          }
        } else if (sdScanInProgress) { // 如果扫描正在进行中
          if (!audioLoopPaused) safeStopAudioSystem(); // 如果音频循环未暂停则安全停止音频系统
          menuState = MENU_SD_LOADING; // 切换到 SD 卡加载菜单
        }
        menuStartTime = millis(); // 记录菜单开始时间
        break; // 跳出 switch

      } else if (menuIndex == 2) { // 如果选择的是第三个选项（Navidrome）
        safeStopAudioSystem(); // 停止音频系统
        delay(100); // 延迟
        safeRestartAudioSystem(); // 重启音频系统
        encoderPos = DEFAULT_VOLUME; audio.setVolume(encoderPos);
        digitalWrite(I2S_SD_MODE, 1); // 使能功放
        naviPlaying = false; // 重置播放状态
        naviSongCount = 0;   // 重置歌单
        loadSDAnimationType(); // 加载动画类型
        menuState = MENU_NAVI_PLAYER; // 进入Navidrome播放器界面
        menuIndex = 0;
        menuStartTime = millis();
        naviState = NAVI_CONNECTING; // 标记正在连接，oled_task会调用naviStart()
        strncpy(stationString, "Navidrome", MAX_INFO_LENGTH - 1);
        strncpy(infoString, "连接中...", MAX_INFO_LENGTH - 1);
        break;

      } else if (menuIndex == 3) { // 如果选择的是第四个选项（音乐闹钟）
        // 停止当前网络音频播放
        audio.stopSong();
        audioLoopPaused = true;
        strcpy(stationString, "音乐闹钟");
        strcpy(infoString, "");
        if (WiFi.status() != WL_CONNECTED) {
          alarmWiFiConnect = true;
          menuState = MENU_WIFI_CONNECTING;
          menuStartTime = millis();
          stationConnectAnimationAngle = random(0, 12) * 30;
          strcpy(stationString, "正在连接WiFi...");
          strcpy(infoString, "请稍候");
        } else {
          setupNTP();
          menuState = MENU_ALARM_CLOCK;
          menuIndex = 0; menuStartTime = millis();
          digitalWrite(I2S_SD_MODE, 0);  // 关闭功放省电
        }

      } else if (menuIndex == 4) { // 如果选择的是第五个选项（动画）
        previousMenuState = MENU_MAIN; // 保存上一个菜单状态
        menuState = MENU_ANIMATION; // 切换到动画菜单
        menuIndex = 0; menuStartTime = millis(); // 重置菜单索引和开始时间
        showingVolume = false; // 关闭音量显示
        ballX = 64; ballY = random(16,48); // 初始化球的初始位置
        ballDX = random(0,2)*2-1; ballDY = random(-3,4); // 初始化球的运动方向
        if (ballDY == 0) ballDY = random(0,2)*2-1; // 确保球在 Y 轴有运动
        leftPaddleY = 24; rightPaddleY = 24; // 初始化挡板位置
        leftScore = 0; rightScore = 0; playerMode = false; gameTick = 0; // 初始化游戏状态

      } else if (menuIndex == 5) { // 如果选择的是第六个选项（WiFi 配置）
        menuState = MENU_SD_EXITING; // 切换到 SD 卡退出菜单
        sdExitingInProgress = true; // 标记 SD 卡退出进行中
        sdExitStartTime = millis(); // 记录退出开始时间
        wifiConfigExiting = true; // 标记 WiFi 配置退出进行中
        preferences.putBool("wifiConfigMode", true); // 保存 WiFi 配置模式标志
        resetScrollState(); // 重置滚动状态

      } else if (menuIndex == 6) { // 如果选择的是第七个选项（恢复出厂设置）
        menuState = MENU_FACTORY_RESET; // 切换到恢复出厂设置菜单
        menuStartTime = millis(); // 记录菜单开始时间

      } else if (menuIndex == 7) { // 如果选择的是第八个选项（内存信息）
        previousMenuState = MENU_MAIN; // 保存上一个菜单状态
        menuState = MENU_MEMORY_INFO; // 切换到内存信息菜单
        menuIndex = 0; menuStartTime = millis(); // 重置菜单索引和开始时间
      }
      break; // 跳出 switch

    case MENU_SD_PLAYER: // SD 卡播放器菜单
      if (totalTracks > 0) { // 如果有曲目
        menuState = MENU_SD_CONTROL; menuIndex = 0; menuStartTime = millis(); // 切换到 SD 卡控制菜单
      } else { // 如果没有曲目
        sdScanInProgress = false; sdScanComplete = false; // 重置扫描状态
        sdScanProgress = 0; sdScanStatus = "Restarting scan..."; // 重置扫描进度和状态
        sdScanInProgress = true; // 标记扫描进行中
        menuState = MENU_SD_LOADING; menuStartTime = millis(); // 切换到 SD 卡加载菜单
      }
      break; // 跳出 switch

    case MENU_SD_CONTROL: // SD 卡控制菜单
      if      (menuIndex == 0) { nextTrack();     encoderPos = constrain(encoderPos,0,21); menuState = MENU_SD_PLAYER; } // 下一曲
      else if (menuIndex == 1) { previousTrack(); encoderPos = constrain(encoderPos,0,21); menuState = MENU_SD_PLAYER; } // 上一曲
      else if (menuIndex == 2) { // 暂停/继续
        audio.pauseResume(); sdPaused = !sdPaused; // 暂停或继续播放，切换暂停状态
        encoderPos = constrain(encoderPos,0,21); menuState = MENU_SD_PLAYER; // 限制编码器位置，切换到 SD 卡播放器菜单
      } else if (menuIndex == 3) { // 播放模式
        menuState = MENU_PLAY_MODE; menuIndex = currentPlayMode; menuStartTime = millis(); // 切换到播放模式菜单
      } else if (menuIndex == 4) { // 动画
        previousMenuState = MENU_SD_CONTROL; // 保存上一个菜单状态
        menuState = MENU_ANIMATION; menuIndex = 0; menuStartTime = millis(); // 切换到动画菜单
        showingVolume = false; // 关闭音量显示
        ballX = 64; ballY = random(16,48); // 初始化球的初始位置
        ballDX = random(0,2)*2-1; ballDY = random(-3,4); // 初始化球的运动方向
        if (ballDY == 0) ballDY = random(0,2)*2-1; // 确保球在 Y 轴有运动
        leftPaddleY = 24; rightPaddleY = 24; // 初始化挡板位置
        leftScore = 0; rightScore = 0; playerMode = false; gameTick = 0; // 初始化游戏状态
      } else if (menuIndex == 5) { // 动画选择
        menuState = MENU_SD_ANIMATION_SELECT; menuIndex = sdPlayerAnimType; menuStartTime = millis(); // 切换到 SD 卡动画选择菜单
      } else if (menuIndex == 6) { // 刷新曲库：删除数据库文件，重新扫描所有MP3
        audio.stopSong(); sdPlaying = false; sdPaused = false; // 停止当前播放
        refreshTrackDatabase(); // 删除旧DB并触发重扫，自动跳转 MENU_SD_LOADING
      } else if (menuIndex == 7) { // 退出 SD 卡播放
        sdScanTaskShouldStop = true; sdScanInProgress = false; // 标记停止 SD 卡扫描任务
        audio.stopSong(); sdPlaying = false; sdPaused = false; // 停止播放，标记未播放和未暂停
        preferences.putBool("sdScanDone",   true); // 保存 SD 卡扫描完成标志
        preferences.putBool("restartFromSD", true); // 保存从 SD 卡重启标志
        menuState = MENU_SD_EXITING; menuStartTime = millis(); // 切换到 SD 卡退出菜单
        sdExitingInProgress = true; sdExitStartTime = millis(); // 标记退出进行中，记录开始时间
      }
      break; // 跳出 switch

    case MENU_PLAY_MODE: // 播放模式菜单
      currentPlayMode = (PlayMode)menuIndex; // 设置当前播放模式
      encoderPos = constrain(encoderPos, 0, 21); // 限制编码器位置在 0-21 范围内
      menuState = MENU_SD_PLAYER; // 切换到 SD 卡播放器菜单
      break; // 跳出 switch

    case MENU_SD_ANIMATION_SELECT: // SD 卡动画选择菜单
      sdPlayerAnimType = menuIndex; // 设置 SD 卡播放器动画类型
      saveSDAnimationType(); // 保存 SD 卡播放器动画类型
      encoderPos = constrain(encoderPos, 0, 21); // 限制编码器位置在 0-21 范围内
      menuState = MENU_SD_PLAYER; // 切换到 SD 卡播放器菜单
      break; // 跳出 switch

    case MENU_NAVI_ANIMATION_SELECT: // Navidrome动画选择菜单
      sdPlayerAnimType = menuIndex; // 设置动画类型（与SD播放器共用同一变量）
      saveSDAnimationType(); // 保存动画类型
      encoderPos = constrain(encoderPos, 0, 21); // 限制编码器位置
      menuState = MENU_NAVI_PLAYER; // 确认后返回Navidrome播放器
      menuIndex = 0; menuStartTime = millis();
      break; // 跳出 switch

    case MENU_STATION_SELECT: { // 电台选择菜单
      int validCount = getValidStationCount(); // 获取有效电台数量
      if (validCount == 0) break; // 如果没有有效电台则跳出
      if (!safeStationSwitch(menuIndex)) strcpy(infoString, "Switch failed - try again"); // 尝试安全切换电台，失败则显示错误信息
      break; // 跳出 switch
    }

    case MENU_ANIMATION: // 动画菜单
      menuState = (previousMenuState != MENU_NONE) ? previousMenuState : MENU_MAIN; // 返回上一个菜单状态或主菜单
      previousMenuState = MENU_NONE; // 重置上一个菜单状态
      menuIndex = 0; menuStartTime = millis(); // 重置菜单索引和开始时间
      ballX = 64; ballY = random(16,48); // 初始化球的初始位置
      ballDX = random(0,2)*2-1; ballDY = random(-3,4); // 初始化球的运动方向
      if (ballDY == 0) ballDY = random(0,2)*2-1; // 确保球在 Y 轴有运动
      leftPaddleY = 24; rightPaddleY = 24; // 初始化挡板位置
      leftScore = 0; rightScore = 0; playerMode = false; gameTick = 0; // 初始化游戏状态
      break; // 跳出 switch

    case MENU_CLOCK: // 时钟菜单
      menuState = (previousMenuState != MENU_NONE) ? previousMenuState : MENU_MAIN; // 返回上一个菜单状态或主菜单
      previousMenuState = MENU_NONE; // 重置上一个菜单状态
      menuIndex = 0; menuStartTime = millis(); // 重置菜单索引和开始时间
      break; // 跳出 switch

    case MENU_MEMORY_INFO: // 内存信息菜单
      menuState = (previousMenuState != MENU_NONE) ? previousMenuState : MENU_MAIN;
      previousMenuState = MENU_NONE;
      menuIndex = 0; menuStartTime = millis();
      break; // 跳出 switch

    case MENU_NAVI_PLAYER: // Navidrome播放器主界面
      if (naviState == NAVI_ERROR && naviConfigValid) {
        // 已配置但发生错误：根据选择执行操作
        if (menuIndex == 0) {
          // 重试：重新加载歌单
          audio.stopSong();
          naviPlaying = false;
          naviSongCount = 0;
          delay(100);
          yield();
          menuState = MENU_NAVI_PLAYER;
          menuIndex = 0;
          menuStartTime = millis();
          naviRefreshPlaylist();
        } else if (menuIndex == 1) {
          // 配置WiFi：直接启动AP配网模式
          naviPlaying = false;
          audio.stopSong();
          naviState = NAVI_IDLE;
          naviSongCount = 0;
          configMode = true;
          configStartTime = millis();
          WiFi.disconnect(true);
          delay(100);
          WiFi.softAP(AP_SSID, AP_PASS);
          delay(500);
          IPAddress apIP(192, 168, 3, 3);
          WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
          dnsServer.start(53, "*", apIP);
          strcpy(stationString, "Config Mode:");
          strcpy(infoString, "Connect to: " AP_SSID);
          Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());
          menuState = MENU_NONE;
          menuIndex = 0;
          resetScrollState();
        } else {
          // 返回：退出Navidrome，返回网络电台
          naviPlaying = false;
          naviNeedsConfig = false;
          audio.stopSong();
          naviState = NAVI_IDLE;
          naviSongCount = 0;
          safeRestartAudioSystem();
          audio.connecttohost(stationUrls[7].c_str());
          strncpy(stationString, stationNames[7].c_str(), MAX_INFO_LENGTH - 1);
          strncpy(infoString, "Reconnecting...", MAX_INFO_LENGTH - 1);
          menuState = MENU_NONE;
          menuIndex = 0;
        }
      } else if (naviSongCount > 0) {
        // 有歌单则进入控制菜单
        menuState = MENU_NAVI_CONTROL;
        menuIndex = 0;
        menuStartTime = millis();
      }
      break;

    case MENU_NAVI_CONTROL: // Navidrome控制菜单
      if (menuIndex == 0) { // 下一首
        naviNextSong();
        encoderPos = constrain(encoderPos, 0, 21);
        menuState = MENU_NAVI_PLAYER;
      } else if (menuIndex == 1) { // 上一首
        naviPrevSong();
        encoderPos = constrain(encoderPos, 0, 21);
        menuState = MENU_NAVI_PLAYER;
      } else if (menuIndex == 2) { // 暂停/继续
        naviTogglePause();
        encoderPos = constrain(encoderPos, 0, 21);
        menuState = MENU_NAVI_PLAYER;
      } else if (menuIndex == 3) { // 刷新歌单（重新随机获取）
        menuState = MENU_NAVI_PLAYER;
        menuIndex = 0;
        menuStartTime = millis();
        naviRefreshPlaylist();
      } else if (menuIndex == 4) { // 音乐闹钟
        // 停止 Navidrome 播放并重置状态
        audio.stopSong();
        naviPlaying = false;
        naviState   = NAVI_IDLE;
        audioLoopPaused = true;
        strcpy(stationString, "音乐闹钟");
        strcpy(infoString, "");
        if (WiFi.status() != WL_CONNECTED) {
          alarmWiFiConnect = true;
          menuState = MENU_WIFI_CONNECTING;
          menuStartTime = millis();
          stationConnectAnimationAngle = random(0, 12) * 30;
          strcpy(stationString, "正在连接WiFi...");
          strcpy(infoString, "请稍候");
        } else {
          setupNTP();
          menuState = MENU_ALARM_CLOCK;
          menuIndex = 0; menuStartTime = millis();
          digitalWrite(I2S_SD_MODE, 0);  // 关闭功放省电
        }
      } else if (menuIndex == 5) { // PONG游戏
        previousMenuState = MENU_NAVI_PLAYER; // 返回时回到Navidrome播放器
        menuState = MENU_ANIMATION;
        menuIndex = 0; menuStartTime = millis();
        showingVolume = false;
        ballX = 64; ballY = random(16, 48);
        ballDX = random(0,2)*2-1; ballDY = random(-3, 4);
        if (ballDY == 0) ballDY = random(0,2)*2-1;
        leftPaddleY = 24; rightPaddleY = 24;
        leftScore = 0; rightScore = 0; playerMode = false; gameTick = 0;
      } else if (menuIndex == 6) { // 内存分配
        previousMenuState = MENU_NAVI_CONTROL; // 返回时回到Navidrome控制菜单
        menuState = MENU_MEMORY_INFO;
        menuIndex = 0; menuStartTime = millis();
      } else if (menuIndex == 7) { // 切换动画效果 → 进入动画选择子菜单
        menuState = MENU_NAVI_ANIMATION_SELECT; menuIndex = sdPlayerAnimType; menuStartTime = millis();
      } else if (menuIndex == 8) { // 退出Navidrome，返回网络电台
        naviPlaying = false;
        naviNeedsConfig = false;  // 清除WEB服务标志
        audio.stopSong();
        naviState = NAVI_IDLE;
        naviSongCount = 0;
        safeRestartAudioSystem();
        audio.connecttohost(stationUrls[7].c_str());
        strncpy(stationString, stationNames[7].c_str(), MAX_INFO_LENGTH - 1);
        strncpy(infoString, "Reconnecting...", MAX_INFO_LENGTH - 1);
        menuState = MENU_NONE;
        menuIndex = 0;
      }
      break;

    case MENU_SD_LOADING: // SD 卡加载菜单
      sdScanInProgress = false; sdScanComplete = false; // 重置扫描状态
      safeRestartAudioSystem(); // 安全重启音频系统
      reconnectWiFiAfterSDPlayback(); // 重新连接 WiFi
      menuState = MENU_MAIN; menuIndex = 0; // 切换到主菜单
      break; // 跳出 switch

    case MENU_OTA_CHECK: // OTA 检查菜单
      if (otaCheckCompleted) { // 如果 OTA 检查完成
        menuState = MENU_PLAYER_SELECT; menuIndex = 0; menuStartTime = millis(); // 切换到播放器选择菜单
      }
      break; // 跳出 switch

    case MENU_OTA_CONFIRM: // OTA 确认菜单
      if (menuIndex == 0) { // 如果选择确认升级
        menuState = MENU_OTA_UPGRADING; menuIndex = 0; menuStartTime = millis(); // 切换到 OTA 升级菜单
        performOTAUpdate(); // 执行 OTA 升级
      } else { // 如果选择取消
        ESP.restart(); // 重启 ESP
      }
      break; // 跳出 switch

    // ── 音乐闹钟界面：按下进入子菜单 ──────────────────────────────
    case MENU_ALARM_CLOCK:
      menuState = MENU_ALARM_SUBMENU;
      menuIndex = 0;
      menuStartTime = millis();
      break;

    // ── 闹钟子菜单（0=设定时间 1=音乐来源 2=返回）─────────────────
    case MENU_ALARM_SUBMENU:
      if (menuIndex == 0) { // 设定闹钟时间
        alarmEditHour  = alarmHour;
        alarmEditMin   = alarmMinute;
        alarmEditField = 0;
        menuState = MENU_ALARM_SET_TIME;
        menuIndex = 0;
        menuStartTime = millis();
      } else if (menuIndex == 1) { // 选择音乐来源
        menuState = MENU_ALARM_SET_SOURCE;
        menuIndex = alarmSource;
        menuStartTime = millis();
      } else if (menuIndex == 2) { // 每天重复：切换开关
        alarmRepeat = !alarmRepeat;
        saveAlarmConfig();
        menuStartTime = millis(); // 刷新超时，留在子菜单
      } else if (menuIndex == 3) { // 返回闹钟界面
        menuState = MENU_ALARM_CLOCK;
        menuIndex = 0;
        menuStartTime = millis();
        digitalWrite(I2S_SD_MODE, 0);  // 关闭功放省电
      } else { // 返回播放器选择
        menuState = MENU_PLAYER_SELECT;
        menuIndex = 4;
        menuStartTime = millis();
      }
      break;

    // ── 闹钟时间设置：逐字段确认 ──────────────────────────────────
    case MENU_ALARM_SET_TIME:
      if (alarmEditField == 0) { // 确认小时，跳到分钟
        alarmEditField = 1;
      } else if (alarmEditField == 1) { // 确认分钟，保存并返回
        alarmHour   = alarmEditHour;
        alarmMinute = alarmEditMin;
        alarmEnabled = true; // 设定时间即视为启用
        alarmLastTriggerMin = -1; // 重置防重复触发标志
        saveAlarmConfig();
        menuState = MENU_ALARM_SUBMENU;
        menuIndex = 0;
        menuStartTime = millis();
      }
      break;

    // ── 闹钟音乐来源选择 ────────────────────────────────────────
    case MENU_ALARM_SET_SOURCE:
      alarmSource = menuIndex; // 0=MP3 1=IMP 2=Navidrome
      saveAlarmConfig();
      menuState = MENU_ALARM_SUBMENU;
      menuIndex = 1;
      menuStartTime = millis();
      break;

    default: break; // 默认情况，跳出 switch
  }
}

// ---- 编码器旋转处理 ----
void handleMenuRotation() { // 处理编码器旋转的函数
  int8_t delta     = encoderDelta; encoderDelta = 0; // 获取编码器增量并重置
  int8_t direction = (delta > 0) ? 1 : -1; // 根据增量确定旋转方向

  switch (menuState) { // 根据菜单状态进行分支处理
    case MENU_PLAYER_SELECT: // 播放器选择菜单
      menuIndex = (menuIndex + direction + 5) % 5; menuStartTime = millis(); break; // 现在有5个选项（含闹钟）
    case MENU_MAIN: // 主菜单
      menuIndex = (menuIndex + direction + MAIN_MENU_COUNT) % MAIN_MENU_COUNT; menuStartTime = millis(); break; // 循环切换选项
    case MENU_SD_CONTROL: // SD 卡控制菜单
      menuIndex = (menuIndex + direction + 8) % 8; menuStartTime = millis(); break; // 循环切换选项（8项含刷新曲库）
    case MENU_PLAY_MODE: // 播放模式菜单
      menuIndex = (menuIndex + direction + 3) % 3; menuStartTime = millis(); break; // 循环切换选项
    case MENU_STATION_SELECT: { // 电台选择菜单
      int validCount = getValidStationCount(); // 获取有效电台数量
      if (validCount == 0) break; // 如果没有有效电台则跳出
      menuIndex = (menuIndex + direction + validCount) % validCount; // 循环切换选项
      menuStartTime = millis(); break; // 记录菜单开始时间
    }
    case MENU_SD_PLAYER: // SD 卡播放器菜单
      encoderPos = constrain(encoderPos, 0, 21); // 限制编码器位置在 0-21 范围内
      audio.setVolume(encoderPos); // 设置音量
      showingVolume = true; volumeDisplayTime = millis(); break; // 显示音量
    case MENU_ANIMATION: // 动画菜单
      break; // 编码器控制挡板，不调音量
    case MENU_SD_ANIMATION_SELECT: // SD 卡动画选择菜单
      menuIndex = (menuIndex + direction + 12) % 12; menuStartTime = millis(); break; // 循环切换选项
    case MENU_NAVI_ANIMATION_SELECT: // Navidrome动画选择菜单
      menuIndex = (menuIndex + direction + 12) % 12; menuStartTime = millis(); break; // 循环切换选项
    case MENU_MEMORY_INFO: // 内存信息菜单
      menuState = MENU_MAIN; menuIndex = 0; menuStartTime = millis(); break; // 返回主菜单
    case MENU_NAVI_PLAYER: // Navidrome播放器主界面
      if (naviState == NAVI_ERROR && naviConfigValid) {
        menuIndex = (menuIndex + direction + 3) % 3;
        menuStartTime = millis();
      } else {
        encoderPos = constrain(encoderPos, 0, 21);
        audio.setVolume(encoderPos);
        showingVolume = true; volumeDisplayTime = millis();
      }
      break;
    case MENU_NAVI_CONTROL: // Navidrome控制菜单
      menuIndex = (menuIndex + direction + 9) % 9; menuStartTime = millis(); break; // 9个控制选项
    case MENU_OTA_CHECK: // OTA 检查菜单
      if (otaCheckCompleted) menuIndex = 0; menuStartTime = millis(); break; // 如果检查完成则重置
    case MENU_OTA_CONFIRM: // OTA 确认菜单
      menuIndex = (menuIndex + direction + 2) % 2; menuStartTime = millis(); break; // 循环切换选项
    // ── 音乐闹钟旋转处理 ─────────────────────────────────────────
    case MENU_ALARM_CLOCK: // 时钟界面：旋转无操作（按钮进子菜单）
      break;
    case MENU_ALARM_SUBMENU: // 子菜单5项
      menuIndex = (menuIndex + direction + 5) % 5; menuStartTime = millis(); break;
    case MENU_ALARM_SET_TIME: // 时间设置：根据当前字段调整小时或分钟
      if (alarmEditField == 0) {
        alarmEditHour = (alarmEditHour + direction + 24) % 24;
      } else {
        alarmEditMin = (alarmEditMin + direction + 60) % 60;
      }
      menuStartTime = millis(); break;
    case MENU_ALARM_SET_SOURCE: // 来源选择3项
      menuIndex = (menuIndex + direction + 3) % 3; menuStartTime = millis(); break;
    default: // 默认情况
      encoderPos = constrain(encoderPos, 0, 21); // 限制编码器位置在 0-21 范围内
      audio.setVolume(encoderPos); // 设置音量
      showingVolume = true; volumeDisplayTime = millis(); break; // 显示音量
  }
}

#endif // MENU_H // 结束头文件保护