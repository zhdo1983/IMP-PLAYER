#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include "globals.h"
#include <esp_task_wdt.h>

// wifi_config.h 中定义，此处前向声明以供内部调用
void connectToWiFi();

// ---- 滚动状态重置 ----
void resetScrollState() { scrollStateResetFlag = true; }

// ---- 内存碎片整理：通过分配/释放不同大小的块来合并堆空闲区间 ----
// 用于电台切换前，确保有足够的连续内存供SSL/MP3解码器使用
void enhancedMemoryCleanupForStationSwitch() {
  audio.stopSong();
  unsigned long waitStart = millis();
  while (audio.inBufferFilled() > 0 && (millis() - waitStart < 2000)) {
    delay(10); esp_task_wdt_reset();
  }

  void* testBlocks[15];
  int blockSizes[] = {512, 1024, 2048, 4096, 8192, 16384, 32768, 16384, 8192, 4096, 2048, 1024, 512, 256, 128};
  for (int i = 0; i < 15; i++) {
    testBlocks[i] = malloc(blockSizes[i]);
    if (testBlocks[i]) { memset(testBlocks[i], 0xAA, blockSizes[i]); free(testBlocks[i]); testBlocks[i] = NULL; }
    if (i % 5 == 0) { delay(5); esp_task_wdt_reset(); }
  }
}

// ---- 激进内存碎片整理（用于SSL内存分配失败、连接重置等场景）----
void aggressiveMemoryCleanup() {
  Serial.println("=== Aggressive Memory Cleanup ===");
  Serial.printf("Free heap before: %u KB\n", ESP.getFreeHeap() / 1024);

  void* testBlocks[20];
  int blockSizes[] = {128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536,
                      32768, 16384, 8192, 4096, 2048, 1024, 512, 256, 128, 64};
  for (int round = 0; round < 2; round++) {
    for (int i = 0; i < 20; i++) {
      testBlocks[i] = malloc(blockSizes[i]);
      if (testBlocks[i]) memset(testBlocks[i], 0xAA, blockSizes[i]);
    }
    delay(5);
    for (int i = 0; i < 20; i++) { if (testBlocks[i]) { free(testBlocks[i]); testBlocks[i] = NULL; } }
  }

  delay(10);
  Serial.printf("Free heap after:  %u KB\n", ESP.getFreeHeap() / 1024);
  Serial.println("=== Cleanup End ===");
}

// ---- 快速停止音频系统（非阻塞，异步等待缓冲区清空）----
void quickStopAudioSystem() {
  esp_task_wdt_reset();
  audio.stopSong();
  digitalWrite(I2S_SD_MODE, 0);
  audioLoopPaused = true;
  strcpy(stationString, "Audio System Stopped");
  strcpy(infoString, "Station Selection...");
  memset(lastHost, 0, sizeof(lastHost));
  audioSystemStopping = true;
  audioStopStartTime = millis();
}

// safeStopAudioSystem 与 quickStopAudioSystem 行为相同，保留别名以兼容调用方
void safeStopAudioSystem() { quickStopAudioSystem(); }

// ---- 在 loop() 中调用：等待音频缓冲区自然排空后完成停止 ----
void asyncAudioSystemCleanup() {
  if (!audioSystemStopping) return;
  if (millis() - audioStopStartTime > AUDIO_STOP_TIMEOUT) {
    audioSystemStopping = false; return;
  }
  if (audio.inBufferFilled() > 0) return;
  audioSystemStopping = false;
}

// ---- 重新使能音频系统 ----
void safeRestartAudioSystem() {
  esp_task_wdt_reset();
  audioLoopPaused = false;
  digitalWrite(I2S_SD_MODE, 1);
  strcpy(stationString, "Audio System Ready");
  strcpy(infoString, "Ready to play");
}

// ---- SD卡播放前断开WiFi以释放约40KB内存 ----
void disconnectWiFiForSDPlayback() {
  if (WiFi.status() == WL_CONNECTED) {
    wifiWasConnected = true;
    savedSSID     = WiFi.SSID();
    savedPassword = WiFi.psk();
  } else {
    wifiWasConnected = false;
  }
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(100);
}

// ---- SD卡播放结束后恢复WiFi连接 ----
void reconnectWiFiAfterSDPlayback() {
  if (!wifiWasConnected || savedSSID.length() == 0) return;
  WiFi.mode(WIFI_STA);
  WiFi.begin(savedSSID.c_str(), savedPassword.c_str());
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500); attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    strcpy(stationString, "WiFi Reconnected");
    strcpy(infoString, savedSSID.c_str());
  } else {
    strcpy(stationString, "WiFi Reconnect Failed");
    strcpy(infoString, "Check credentials");
  }
}

// ---- SD -> 网络流切换专项清理：等待SD缓冲区排空后整理堆内存 ----
void cleanupFromSDToNetworkStream() {
  audio.stopSong();
  unsigned long waitStart = millis();
  while (audio.inBufferFilled() > 0 && (millis() - waitStart < 3000)) {
    delay(50); esp_task_wdt_reset();
  }

  void* largeBlocks[5];
  int blockSizes[] = {32768, 16384, 8192, 4096, 2048};
  for (int i = 0; i < 5; i++) {
    largeBlocks[i] = malloc(blockSizes[i]);
    if (largeBlocks[i]) { memset(largeBlocks[i], 0xBB, blockSizes[i]); free(largeBlocks[i]); largeBlocks[i] = NULL; }
    delay(10); esp_task_wdt_reset();
  }
}

// ---- 每5秒检查一次音频缓冲区，更新 audioQualityStatus ----
void checkAudioQuality() {
  unsigned long currentTime = millis();
  if (currentTime - lastAudioCheck <= 5000) return;

  int filled = audio.inBufferFilled();
  int total  = filled + audio.inBufferFree();

  if (total > 0) {
    if (filled < total * 0.1) {
      audioBufferUnderruns++;
      audioQualityWarning = true;
      audioQualityStatus = "Buffer Low";
    } else if (filled > total * 0.9) {
      audioBufferOverruns++;
      audioQualityWarning = true;
      audioQualityStatus = "Buffer Full";
    } else {
      if (audioQualityWarning) {
        audioQualityWarning = false;
        audioQualityStatus = "Normal";
        strcpy(errorInfoString, "");
      }
    }
  }
  lastAudioCheck = currentTime;
}

// ---- 连接失败后最多重试3次，每次间隔 CONNECTION_TIMEOUT ms ----
void handleAudioConnectionFailure() {
  if (!audioConnectionFailed) return;
  unsigned long currentTime = millis();
  if (currentTime - lastConnectionAttempt > CONNECTION_TIMEOUT) {
    if (connectionRetryCount < 3) {
      enhancedMemoryCleanupForStationSwitch();
      if (lastSelectedStationIndex >= 0) {
        int realIdx = getValidStationIndex(lastSelectedStationIndex);
        if (realIdx >= 0) {
          audio.connecttohost(stationUrls[realIdx].c_str());
          strcpy(infoString, "Retrying connection...");
          lastConnectionAttempt = currentTime;
          connectionRetryCount++;
        }
      }
    } else {
      strcpy(infoString, "Connection failed");
      audioConnectionFailed = false;
      connectionRetryCount = 0;
    }
  }
}

// ---- 重新连接到默认电台（index 7）----
void reinitializeNetworkStream() {
  audio.stopSong();
  delay(100);
  digitalWrite(I2S_SD_MODE, 1);
  delay(10);
  audio.connecttohost(stationUrls[7].c_str());
  strcpy(stationString, stationNames[7].c_str());
  strcpy(infoString, "Reconnecting...");
}

#endif // AUDIO_MANAGER_H