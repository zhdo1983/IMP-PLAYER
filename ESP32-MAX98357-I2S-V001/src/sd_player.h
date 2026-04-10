#ifndef SD_PLAYER_H        // 防止头文件重复包含
#define SD_PLAYER_H        // 定义SD_PLAYER_H宏

#include "globals.h"       // 引入全局变量和函数声明
#include "audio_manager.h" // 引入音频管理器
#include <SD.h>            // 引入SD卡库
#include <esp_task_wdt.h> // 引入任务看门狗库

// ---- 工具函数 ---- // Utility functions

// 格式化秒数为 MM:SS 字符串 // Format seconds to MM:SS string
String formatTime(unsigned long seconds) {     // 格式化时间函数
  unsigned long minutes = seconds / 60;      // 计算分钟数
  unsigned long secs    = seconds % 60;       // 计算秒数
  char timeStr[10];                           // 时间字符串缓冲区
  snprintf(timeStr, sizeof(timeStr), "%02lu:%02lu", minutes, secs);  // 格式化输出
  return String(timeStr);                     // 返回格式化后的字符串
}

// 获取用于显示的曲名（去除路径和扩展名）// Get track name for display (remove path and extension)
String getDisplayTrackName(const String& fullPath) {  // 获取显示曲名函数
  int start = fullPath.lastIndexOf('/') + 1;   // 查找最后一个斜杠位置加1
  int end   = fullPath.length();                // 获取字符串长度
  if (fullPath.endsWith(".mp3") || fullPath.endsWith(".MP3")) end -= 4;  // 去掉.mp3扩展名
  return fullPath.substring(start, end);        // 返回处理后的曲名
}

// 保存 / 加载 SD 动画类型 // Save/load SD animation type
void saveSDAnimationType() { preferences.putInt("sdAnimType", sdPlayerAnimType); }  // 保存动画类型到NVS
void loadSDAnimationType() { sdPlayerAnimType = preferences.getInt("sdAnimType", 0); }  // 从NVS加载动画类型

// ---- MP3 文件名判断 ---- // MP3 filename check

// 判断文件是否为MP3文件 // Check if file is MP3
inline bool isMP3File(const char* filename, size_t len) {  // 判断是否为MP3文件函数
  if (len < 5) return false;                  // 文件名太短则不是
  const char* ext = filename + len - 4;       // 指向扩展名位置
  return (ext[0] == '.' &&                     // 检查扩展名
          ((ext[1] == 'm' || ext[1] == 'M') &&  // m或M
           (ext[2] == 'p' || ext[2] == 'P') &&  // p或P
           (ext[3] == '3')));                  // 3
}

// ---- SD 缓冲区动态分配 / 释放 ---- // Dynamic allocation of SD track buffers
//
// 设计原则：trackList 和 scanFoundPaths 平时为 NULL，不占堆内存。
// 进入 SD 播放器（sdScanTask 启动前）调用 allocSDBuffers() 一次性分配。
// 退出 SD 播放器（executeSDPlayerExitCleanup 末尾）调用 freeSDBuffers() 归还内存。
// 500 条空 String 对象 × 2 个数组 ≈ 12KB 对象头 + 路径内容动态增长（最多约 30KB）。
// 不用时完全不占用，HTTPS / Navidrome 等功能可以安全使用这块内存。

bool allocSDBuffers() {  // 分配SD缓冲区，返回true表示成功
  if (trackList != NULL) return true;  // 已分配则直接返回

  trackList = new (std::nothrow) String[MAX_TRACKS];  // 分配曲目列表
  if (!trackList) {
    Serial.println("[SD] allocSDBuffers: trackList alloc FAILED (out of memory)");
    return false;
  }

  scanFoundPaths = new (std::nothrow) String[MAX_TRACKS];  // 分配扫描路径缓冲
  if (!scanFoundPaths) {
    Serial.println("[SD] allocSDBuffers: scanFoundPaths alloc FAILED, freeing trackList");
    delete[] trackList;
    trackList = NULL;
    return false;
  }

  Serial.printf("[SD] allocSDBuffers: OK, allocated %u bytes\n",
                (unsigned)(sizeof(String) * MAX_TRACKS * 2));
  return true;
}

void freeSDBuffers() {  // 释放SD缓冲区，归还全部内存
  if (trackList) {
    delete[] trackList;
    trackList = NULL;
  }
  if (scanFoundPaths) {
    delete[] scanFoundPaths;
    scanFoundPaths = NULL;
  }
  totalTracks    = 0;
  scanFoundCount = 0;
  Serial.println("[SD] freeSDBuffers: buffers released");
}

// ---- 数据库操作 ---- // Database operations

// 保存曲目数据库到SD卡 // Save track database to SD card
bool saveTrackDatabase() {                     // 保存曲目数据库函数
  if (!trackList) return false;               // 缓冲区未分配则跳过
  File dbFile = SD.open(DB_FILE, FILE_WRITE); // 打开数据库文件（写入模式）
  if (!dbFile) return false;                   // 打开失败则返回false
  dbFile.println(totalTracks);                // 写入总曲目数
  for (int i = 0; i < totalTracks; i++) dbFile.println(trackList[i]);  // 写入每个曲目路径
  dbFile.close();                             // 关闭文件
  return true;                                // 返回成功
}

// 从SD卡加载曲目数据库 // Load track database from SD card
bool loadTrackDatabase() {                     // 加载曲目数据库函数
  if (!trackList) { Serial.println("[SD] loadTrackDatabase: buffer not allocated"); return false; }
  if (!SD.exists(DB_FILE)) return false;      // 文件不存在则返回false
  File dbFile = SD.open(DB_FILE, FILE_READ);  // 打开数据库文件（读取模式）
  if (!dbFile) return false;                  // 打开失败则返回false

  char line[256];                             // 行缓冲区
  int lineIdx = 0;                            // 行索引
  bool firstLine = true;                      // 首个行标志
  int savedCount = 0;                         // 保存的曲目数
  totalTracks = 0;                            // 初始化总曲目数为0

  while (totalTracks < MAX_TRACKS && dbFile.available()) {  // 遍历文件内容
    char c = dbFile.read();                   // 读取一个字符
    if (c == '\n' || c == '\r' || lineIdx >= 255) {  // 遇到换行或缓冲区满
      if (lineIdx > 0) {                      // 有有效内容
        line[lineIdx] = '\0';                 // 字符串结束符
        if (firstLine) { savedCount = atoi(line); firstLine = false; }  // 首行为总数
        else           { trackList[totalTracks++] = String(line); }  // 其他行为曲目
        lineIdx = 0;                          // 重置索引
      }
    } else if (c != '\r') {                  // 忽略回车符
      line[lineIdx++] = c;                   // 存入缓冲区
    }
    // 每读取200条喂一次看门狗，防止大曲库加载时WDT超时  // Feed watchdog during large DB load
    if (totalTracks % 200 == 0 && totalTracks > 0) esp_task_wdt_reset();
  }
  dbFile.close();                             // 关闭文件
  sdTotalMP3Files = totalTracks;             // 设置MP3文件总数
  return (totalTracks == savedCount && totalTracks > 0);  // 验证并返回结果
}

// 删除曲目数据库文件 // Delete track database file
void deleteTrackDatabase() { if (SD.exists(DB_FILE)) SD.remove(DB_FILE); }  // 删除数据库文件

// ---- 快速验证数据库（仅读首行，不遍历文件系统）---- // Fast DB validation: read first line only
// 返回数据库记录的曲目数；文件不存在或为空返回 -1
// 【修改说明】替换原来的 quickCountMP3Files()，避免每次启动都遍历SD卡所有文件
int quickValidateDatabase() {                 // 快速验证数据库函数
  if (!SD.exists(DB_FILE)) return -1;        // 文件不存在返回-1
  File dbFile = SD.open(DB_FILE, FILE_READ); // 打开数据库文件
  if (!dbFile) return -1;                    // 打开失败返回-1
  String firstLine = dbFile.readStringUntil('\n');  // 只读第一行（曲目总数）
  dbFile.close();                            // 立即关闭，不遍历任何文件
  int count = firstLine.toInt();             // 转换为整数
  Serial.printf("[SD] quickValidateDatabase: savedCount=%d\n", count);
  return (count > 0) ? count : -1;           // 返回曲目数或-1
}

// ---- 刷新曲库（用户手动触发，删除DB后重扫）---- // Manual refresh track database
// 在 SD 控制菜单中调用此函数即可强制重扫，解决增删歌曲后曲库不更新的问题
void refreshTrackDatabase() {               // 手动刷新曲库函数
  deleteTrackDatabase();                    // 删除旧数据库文件
  freeSDBuffers();                          // 释放内存缓冲区
  totalTracks      = 0;                    // 重置曲目数
  sdScanComplete   = false;               // 重置扫描完成标志
  sdScanInProgress = true;                // 触发重新扫描
  sdScanProgress   = 0;                   // 重置进度
  sdScanStatus     = "重新扫描...";       // 更新状态
  menuState        = MENU_SD_LOADING;     // 切换到加载界面
  Serial.println("[SD] refreshTrackDatabase: DB deleted, rescan triggered");
}

// ---- 快速统计 MP3 文件数量（保留备用，全量扫描时使用）---- // Quick count MP3 files (used only during full scan)
// 【修改说明】此函数仅在 scanAndBuildDatabase() 内部调用，不再在每次启动时调用
int quickCountMP3Files() {                    // 快速统计MP3文件数量函数
  int count = 0;                             // 初始化计数器
  sdCheckedFiles = 0;                        // 已检查文件数置零
  sdEstimatedTotalFiles = 1000;               // 估计总文件数
  File root = SD.open("/");                   // 打开根目录
  if (!root) return 0;                       // 打开失败返回0

  while (true) {                             // 循环遍历文件
    File file = root.openNextFile();          // 获取下一个文件
    if (!file) break;                         // 没有更多文件则退出
    sdCheckedFiles++;                         // 已检查文件数加1
    if (!file.isDirectory()) {               // 如果不是目录
      const char* fname = file.name();        // 获取文件名
      size_t len = strlen(fname);             // 获取文件名长度
      if (len > 4 && isMP3File(fname, len)) count++;  // 是MP3文件则计数加1
    }
    file.close();                            // 关闭文件
    if (sdCheckedFiles % 10 == 0) {          // 【修改】每10个文件喂一次看门狗（原50）
      sdScanProgress = (sdTotalMP3Files > 0)  // 计算扫描进度
          ? constrain((sdCheckedFiles * 70) / sdTotalMP3Files, 0, 70)
          : constrain(map(sdCheckedFiles, 0, 2000, 0, 70), 0, 70);
      esp_task_wdt_reset();                   // 重置看门狗
    }
  }
  root.close();                              // 关闭根目录
  sdScanProgress = 70;                       // 设置扫描进度为70%
  return count;                              // 返回MP3文件数量
}

// ---- 并行子目录扫描任务 ---- // Parallel subdirectory scan task
void scanDirectoryTask(void* pvParameters) {   // 扫描目录任务函数
  const char* dirPath = (const char*)pvParameters;  // 获取目录路径参数
  File dir = SD.open(dirPath);               // 打开目录
  if (!dir || !dir.isDirectory()) {         // 打开失败或不是目录
    dir.close();                             // 关闭目录
    if (scanResultMutex) xSemaphoreTake(scanResultMutex, portMAX_DELAY);  // 获取互斥锁
    activeScanTasks--;                       // 活跃任务数减1
    if (scanResultMutex) xSemaphoreGive(scanResultMutex);  // 释放互斥锁
    vTaskDelete(NULL); return;                // 删除任务
  }

  while (true) {                             // 循环遍历目录
    if (sdScanTaskShouldStop) break;         // 如果应停止则退出
    File file = dir.openNextFile();          // 获取下一个文件
    if (!file) break;                         // 没有更多文件则退出
    if (scanResultMutex) xSemaphoreTake(scanResultMutex, portMAX_DELAY);  // 获取互斥锁
    sdCheckedFiles++;                         // 已检查文件数加1
    if (scanResultMutex) xSemaphoreGive(scanResultMutex);  // 释放互斥锁

    if (!file.isDirectory()) {               // 如果不是目录
      const char* fname = file.name();        // 获取文件名
      size_t len = strlen(fname);             // 获取文件名长度
      if (len > 4 && isMP3File(fname, len)) {  // 是MP3文件
        if (scanResultMutex) xSemaphoreTake(scanResultMutex, portMAX_DELAY);  // 获取互斥锁
        if (scanFoundPaths && scanFoundCount < MAX_TRACKS) {   // 缓冲区有效且未达到最大曲目数
          scanFoundPaths[scanFoundCount] = "/";  // 设置路径前缀
          scanFoundPaths[scanFoundCount].concat(fname);  // 拼接文件名
          scanFoundCount++;                   // 找到的曲目数加1
        }
        if (scanResultMutex) xSemaphoreGive(scanResultMutex);  // 释放互斥锁
      }
    }
    file.close();                            // 关闭文件
    if (sdCheckedFiles % 20 == 0) esp_task_wdt_reset();  // 【修改】每20个文件喂狗（原100）
  }
  dir.close();                               // 关闭目录
  if (scanResultMutex) xSemaphoreTake(scanResultMutex, portMAX_DELAY);  // 获取互斥锁
  activeScanTasks--;                         // 活跃任务数减1
  if (scanResultMutex) xSemaphoreGive(scanResultMutex);  // 释放互斥锁
  vTaskDelete(NULL);                          // 删除任务
}

// ---- 构建曲库 ---- // Build track library
void scanAndBuildDatabase() {                 // 扫描并构建数据库函数
  if (!allocSDBuffers()) {                   // 确保缓冲区已分配
    sdScanStatus = "内存不足，无法扫描!";
    sdScanInProgress = false;
    return;
  }
  // 【修改】全量扫描时才调用 quickCountMP3Files()，用于进度条估算
  // 原逻辑在每次启动时都调用，是慢的根源；现在只有真正需要重扫时才执行
  if (sdTotalMP3Files == 0) sdTotalMP3Files = quickCountMP3Files();  // 估算总数（用于进度条）
  if (!SD.begin(SD_CS)) { sdScanStatus = "SD card init failed!"; return; }  // SD卡初始化失败
  if (ESP.getFreeHeap() < 15000) { sdScanStatus = "Insufficient memory!"; return; }  // 内存不足

  if (sdMutex && xSemaphoreTake(sdMutex, pdMS_TO_TICKS(1000)) != pdTRUE) {  // 获取SD互斥锁
    sdScanStatus = "SD busy!"; return;  // SD忙则返回
  }

  sdScanInProgress = true;                 // 扫描进行中
  sdScanComplete   = false;               // 扫描未完成
  sdScanStartTime  = millis();           // 记录扫描开始时间

  // 收集子目录 // Collect subdirectories
  String subDirs[20]; int subDirCount = 0;  // 子目录数组和计数器
  File root = SD.open("/");               // 打开根目录
  if (root) {                             // 成功打开
    while (subDirCount < 20) {            // 最多20个子目录
      File f = root.openNextFile();       // 获取下一个文件
      if (!f) break;                      // 没有更多文件则退出
      if (f.isDirectory()) subDirs[subDirCount++] = String("/") + String(f.name());  // 是目录则添加
      f.close();                          // 关闭文件
      esp_task_wdt_reset();               // 【修改】收集子目录时也喂狗
    }
    root.close();                         // 关闭根目录
  }

  totalTracks = 0; sdCheckedFiles = 0; scanFoundCount = 0; activeScanTasks = 0;  // 初始化变量
  const unsigned long TIMEOUT = SCAN_TIMEOUT;  // 扫描超时时间
  unsigned long scanStart = millis();   // 记录扫描开始时间

  // 扫描根目录 // Scan root directory
  root = SD.open("/");                  // 打开根目录
  if (root) {                           // 成功打开
    while (totalTracks < MAX_TRACKS) { // 未达到最大曲目数
      File f = root.openNextFile();    // 获取下一个文件
      if (!f) break;                   // 没有更多文件则退出
      sdCheckedFiles++;                 // 已检查文件数加1
      if (millis() - scanStart > TIMEOUT || ESP.getFreeHeap() < 5000) break;  // 超时或内存不足
      if (!f.isDirectory()) {          // 不是目录
        const char* fname = f.name();  // 获取文件名
        size_t len = strlen(fname);    // 获取文件名长度
        if (len > 4 && isMP3File(fname, len)) {  // 是MP3文件
          trackList[totalTracks] = "/";  // 设置路径前缀
          trackList[totalTracks].concat(fname);  // 拼接文件名
          totalTracks++;                 // 曲目数加1
        }
      }
      f.close();                       // 关闭文件
      if (sdCheckedFiles % 5 == 0) {  // 每5个文件更新进度
        sdScanProgress = (sdTotalMP3Files > 0 && totalTracks > 0)  // 计算进度
            ? constrain(70 + (totalTracks * 15) / sdTotalMP3Files, 70, 85)
            : constrain(map(sdCheckedFiles, 0, sdEstimatedTotalFiles, 70, 85), 70, 85);
        esp_task_wdt_reset();           // 重置看门狗
      }
      yield();                         // 让出CPU，防止阻塞
    }
    root.close();                      // 关闭根目录
  }
  sdScanProgress = 85;                 // 进度设为85%

  // 并行扫描子目录 // Parallel scan subdirectories
  if (subDirCount > 0 && totalTracks < MAX_TRACKS) {  // 有子目录且未达到最大曲目数
    for (int i = 0; i < min(subDirCount, MAX_SCAN_TASKS); i++) {  // 创建扫描任务
      if (totalTracks >= MAX_TRACKS) break;  // 达到最大曲目数则退出
      char taskName[16];              // 任务名称
      snprintf(taskName, sizeof(taskName), "ScanDir%d", i);  // 格式化任务名
      xTaskCreatePinnedToCore(scanDirectoryTask, taskName, 4096, (void*)subDirs[i].c_str(), 1, &scanTaskHandles[i], 0);  // 创建任务
      activeScanTasks++;              // 活跃任务数加1
    }
    unsigned long lastProgress = 0;   // 上次进度更新时间
    while (activeScanTasks > 0 && totalTracks < MAX_TRACKS) {  // 等待任务完成
      if (millis() - scanStart > TIMEOUT) { sdScanTaskShouldStop = true; break; }  // 超时则停止
      if (millis() - lastProgress > 1000) {  // 每秒更新进度
        if (scanResultMutex) xSemaphoreTake(scanResultMutex, portMAX_DELAY);  // 获取互斥锁
        sdScanProgress = (sdTotalMP3Files > 0 && totalTracks > 0)  // 计算进度
            ? constrain(85 + (totalTracks * 15) / sdTotalMP3Files, 85, 100)
            : constrain(map(sdCheckedFiles, 0, sdEstimatedTotalFiles, 85, 100), 85, 100);
        if (scanResultMutex) xSemaphoreGive(scanResultMutex);  // 释放互斥锁
        lastProgress = millis();      // 更新上次时间
      }
      if (scanResultMutex) xSemaphoreTake(scanResultMutex, portMAX_DELAY);  // 获取互斥锁
      while (scanFoundCount > 0 && totalTracks < MAX_TRACKS) {  // 处理找到的曲目
        scanFoundCount--;              // 找到的曲目数减1
        trackList[totalTracks] = scanFoundPaths[scanFoundCount];  // 复制曲目路径
        totalTracks++;                 // 曲目数加1
      }
      if (scanResultMutex) xSemaphoreGive(scanResultMutex);  // 释放互斥锁
      vTaskDelay(pdMS_TO_TICKS(20));  // 【修改】延时改为20ms（原50ms），让喂狗更及时
      esp_task_wdt_reset();           // 【修改】在等待循环里主动喂狗
    }
    sdScanTaskShouldStop = true;      // 停止扫描任务
    vTaskDelay(pdMS_TO_TICKS(200));  // 等待任务停止
    if (scanResultMutex) xSemaphoreTake(scanResultMutex, portMAX_DELAY);  // 获取互斥锁
    while (scanFoundCount > 0 && totalTracks < MAX_TRACKS) {  // 处理剩余曲目
      scanFoundCount--;               // 找到的曲目数减1
      trackList[totalTracks] = scanFoundPaths[scanFoundCount];  // 复制曲目路径
      totalTracks++;                  // 曲目数加1
    }
    if (scanResultMutex) xSemaphoreGive(scanResultMutex);  // 释放互斥锁
  }

  saveTrackDatabase();                // 保存曲目数据库
  sdTotalMP3Files = totalTracks;     // 设置MP3文件总数
  sdScanComplete  = true;            // 扫描完成
  sdScanInProgress = false;          // 扫描不再进行
  char statusStr[64];                // 状态字符串
  snprintf(statusStr, sizeof(statusStr), "已加载 %d 首", totalTracks);  // 格式化状态
  sdScanStatus = statusStr;          // 设置状态字符串
  if (sdMutex) xSemaphoreGive(sdMutex);  // 释放SD互斥锁
  Serial.printf("[SD SCAN] 曲库建立完成! 共 %d 首, 耗时 %lu ms\n", totalTracks, millis() - scanStart);  // 打印完成信息
}

// ---- 异步 SD 扫描主任务（修改版）---- // Async SD scan main task
// 【核心改动】原逻辑：DB存在时先 quickCountMP3Files() 遍历所有文件核对数量，几乎每次不匹配
//             → 触发全量 scanAndBuildDatabase()，启动耗时数十秒，WDT容易重启。
// 【新逻辑】：DB存在时直接调用 quickValidateDatabase() 只读首行，1ms内完成；
//             直接加载数据库，启动时间 < 1秒。
//             只有数据库不存在（首次使用）或损坏时才全量扫描。
//             用户增删歌曲后可通过菜单"刷新曲库"手动触发重扫。
void sdScanTask(void* pvParameters) {           // SD扫描任务函数
  Serial.println("[SD SCAN] 任务已启动");      // 打印任务启动信息
  while (1) {                                 // 无限循环
    if (sdScanTaskShouldStop) {              // 如果应停止
      sdScanTaskShouldStop = false;          // 重置停止标志
      sdScanTaskCreated = false;             // 任务未创建
      vTaskDelete(NULL);                      // 删除任务
    }
    esp_task_wdt_reset();                     // 重置看门狗

    if (sdScanInProgress && !sdScanComplete) {  // 扫描进行中且未完成
      try {                                   // 尝试执行

        // ---- Step 1: 初始化SD卡 ----
        if (!SD.begin(SD_CS)) {              // SD卡初始化失败
          sdScanStatus = "SD init failed!";  // 设置状态
          sdScanInProgress = false;          // 扫描不再进行
          vTaskDelay(pdMS_TO_TICKS(1000));  // 延时1秒
          continue;                           // 继续
        }
        esp_task_wdt_reset();               // SD初始化后喂狗

        // ---- Step 2: 数据库不存在 → 首次全量扫描 ----
        if (!SD.exists(DB_FILE)) {
          sdScanStatus = "首次扫描SD卡...";  // 设置状态（首次使用提示）
          sdScanProgress = 0;
          esp_task_wdt_reset();
          scanAndBuildDatabase();            // 全量扫描并建库
          continue;                           // 继续
        }

        // ---- Step 3: 数据库存在 → 快速验证（只读首行，不遍历文件）----
        // 【关键修改】原来在这里调用 quickCountMP3Files() 遍历全部文件，
        // 耗时长且与savedCount（含子目录）天然不匹配，几乎每次都触发重扫。
        // 现在改为只读DB首行，< 1ms 完成。
        int savedCount = quickValidateDatabase();  // 快速读取DB中记录的曲目数

        if (savedCount > 0) {
          // ---- Step 4a: DB有效 → 直接加载，跳过所有文件系统遍历 ----
          sdScanStatus = "正在加载曲库...";  // 设置状态
          sdScanProgress = 30;             // 设置初始进度
          esp_task_wdt_reset();

          if (!allocSDBuffers()) {          // 确保缓冲区已分配
            sdScanStatus = "内存不足!";
            sdScanInProgress = false;
            continue;
          }
          esp_task_wdt_reset();

          sdScanProgress = 50;
          if (loadTrackDatabase()) {        // 加载数据库成功
            sdScanProgress = 100;           // 设置进度100%
            sdScanComplete = true;          // 扫描完成
            sdScanInProgress = false;       // 扫描不再进行
            sdTotalMP3Files = totalTracks;  // 同步总数
            char statusStr[64];            // 状态字符串
            snprintf(statusStr, sizeof(statusStr), "已加载 %d 首", totalTracks);  // 格式化
            sdScanStatus = statusStr;        // 设置状态
            Serial.printf("[SD SCAN] 快速加载完成! 共 %d 首\n", totalTracks);
          } else {
            // 数据库文件存在但读取失败（损坏）→ 删除并重扫
            Serial.println("[SD SCAN] 数据库损坏，删除后重新扫描...");
            deleteTrackDatabase();          // 删除损坏的数据库
            sdScanStatus = "曲库损坏，重扫...";
            sdScanProgress = 0;
            esp_task_wdt_reset();
            scanAndBuildDatabase();        // 全量重扫
          }
        } else {
          // ---- Step 4b: DB首行无效（savedCount <= 0）→ 全量扫描 ----
          Serial.println("[SD SCAN] DB验证失败，重新扫描...");
          deleteTrackDatabase();            // 删除无效的数据库
          sdScanStatus = "正在扫描SD卡...";
          sdScanProgress = 0;
          esp_task_wdt_reset();
          scanAndBuildDatabase();           // 全量扫描
        }

      } catch (...) {                      // 捕获异常
        Serial.println("[SD SCAN] 扫描出现异常!");  // 打印异常信息
        sdScanInProgress = false;          // 扫描不再进行
        sdScanStatus = "扫描出错!";       // 设置状态
      }
    } else {                               // 扫描未进行或已完成
      if (sdScanComplete && totalTracks > 0) {  // 扫描完成且有曲目
        vTaskDelay(pdMS_TO_TICKS(5000));  // 延时5秒
        continue;                           // 继续
      }
    }
    vTaskDelay(pdMS_TO_TICKS(200));      // 延时200ms
    esp_task_wdt_reset();                // 【修改】空闲循环也喂狗
    yield();                               // 让出CPU
  }
}

// ---- 播放指定索引曲目 ---- // Play track by index
void playTrack(int index) {                   // 播放指定索引曲目函数
  if (!trackList) return;                    // 缓冲区未分配则返回
  if (index < 0 || index >= totalTracks) return;  // 索引无效则返回

  if (ESP.getMaxAllocHeap() < 16384) aggressiveMemoryCleanup();  // 内存不足则清理

  audio.stopSong();                          // 停止当前播放
  for (int i = 0; i < 5; i++) { delay(10); esp_task_wdt_reset(); }  // 延时等待

  strcpy(stationString, "SD Card Player");  // 设置电台字符串
  strcpy(infoString, "Playing local MP3");   // 设置信息字符串

  currentTrackIndex = index;               // 设置当前曲目索引
  currentTrackName  = trackList[index];     // 设置当前曲目名
  trackStartTime    = millis();             // 记录开始时间
  currentPlayTime   = 0;                    // 当前播放时间置零
  totalPlayTime     = 0;                    // 总播放时间置零
  timeInfoAvailable = false;               // 时间信息不可用
  lastPlayTimeUpdate = millis();            // 上次更新时间

  String filePath = currentTrackName;      // 构建文件路径（已有/前缀）
  File audioFile = SD.open(filePath);        // 打开音频文件
  if (audioFile) {                           // 打开成功
    unsigned long fileSize = audioFile.size();  // 获取文件大小
    audioFile.close();                       // 关闭文件
    unsigned long estimatedDuration = fileSize / 40000;  // 估算时长
    if (estimatedDuration > 10 && estimatedDuration < 3600) {  // 时长合理
      totalPlayTime = estimatedDuration;      // 设置总播放时间
      timeInfoAvailable = true;              // 时间信息可用
    }
  }

  audio.connecttoFS(SD, filePath.c_str());  // 连接到SD卡播放

  #if DEBUG_MP3_INFO                         // 调试信息
  Serial.print("Playing MP3: "); Serial.println(currentTrackName);  // 打印曲目名
  Serial.print("Track: "); Serial.print(currentTrackIndex + 1);  // 打印曲目序号
  Serial.print("/"); Serial.println(totalTracks);  // 打印总数
  #endif
}

// ---- 下一首 / 上一首 ---- // Next/Previous track
void nextTrack() {                           // 下一首函数
  if (totalTracks == 0) return;            // 没有曲目则返回
  int nextIndex = 0;                        // 初始化下一首索引
  if (currentPlayMode == PLAY_MODE_RANDOM) {  // 随机播放模式
    if (totalTracks > 1) {                  // 多于一首
      randomSeed(esp_random() ^ millis() ^ micros());  // 随机数种子
      nextIndex = random(0, totalTracks);   // 随机选择
      int retries = 0;                     // 重试计数器
      while (nextIndex == currentTrackIndex && retries++ < 5)  // 避免重复
        nextIndex = random(0, totalTracks);  // 重新随机选择
    }
  } else {                                 // 其他播放模式
    nextIndex = (currentTrackIndex + 1) % totalTracks;  // 顺序下一首
  }
  scrollStateResetFlag = true;              // 重置滚动状态
  playTrack(nextIndex);                    // 播放下一首
}

void previousTrack() {                      // 上一首函数
  if (totalTracks == 0) return;            // 没有曲目则返回
  int prevIndex = 0;                       // 初始化上一首索引
  if (currentPlayMode == PLAY_MODE_RANDOM) {  // 随机播放模式
    if (totalTracks > 1) {                  // 多于一首
      randomSeed(esp_random() ^ millis() ^ micros());  // 随机数种子
      prevIndex = random(0, totalTracks);  // 随机选择
      int retries = 0;                     // 重试计数器
      while (prevIndex == currentTrackIndex && retries++ < 5)  // 避免重复
        prevIndex = random(0, totalTracks);  // 重新随机选择
    }
  } else {                                 // 其他播放模式
    prevIndex = (currentTrackIndex - 1 + totalTracks) % totalTracks;  // 顺序上一首
  }
  scrollStateResetFlag = true;              // 重置滚动状态
  playTrack(prevIndex);                     // 播放上一首
}

// ---- 自动播放下一首（MP3 结束时调用）---- // Auto play next (called when MP3 ends)
void autoPlayNext() {                       // 自动播放下一首函数
  if (!autoPlayEnabled || totalTracks == 0) return;  // 自动播放未启用或无曲目则返回

  switch (currentPlayMode) {               // 根据播放模式处理
    case PLAY_MODE_SINGLE:                  // 单曲循环模式
      playTrack(currentTrackIndex);        // 播放当前曲目
      break;
    case PLAY_MODE_SEQUENCE:                // 顺序播放模式
      if (currentTrackIndex < totalTracks - 1) {  // 不是最后一首
        playTrack(currentTrackIndex + 1);  // 播放下一首
      } else {                             // 最后一首
        audio.stopSong();                  // 停止播放
        sdPlaying = false;                 // 设置为未播放
        digitalWrite(I2S_SD_MODE, 0);      // 关闭I2S
      }
      break;
    case PLAY_MODE_RANDOM: {               // 随机播放模式
      if (totalTracks > 1) {              // 多于一首
        unsigned long seed = millis() + currentTrackIndex + esp_random() + micros() + analogRead(0);  // 随机种子
        randomSeed(seed);                 // 设置随机种子
        int nextIndex = random(0, totalTracks);  // 随机选择
        int retries = 0;                  // 重试计数器
        while (nextIndex == currentTrackIndex && retries++ < 5)  // 避免重复
          nextIndex = random(0, totalTracks);  // 重新随机选择
        playTrack(nextIndex);             // 播放随机曲目
      } else {                           // 只有一首
        playTrack(0);                    // 播放第一首
      }
      break;
    }
  }
}

// ---- 执行完整的 SD 退出清理 ---- // Execute complete SD exit cleanup
void executeSDPlayerExitCleanup() {          // SD播放器退出清理函数
  // 重置 SD 相关状态 // Reset SD related states
  currentTrackIndex = 0; currentTrackName = "";  // 重置曲目索引和名称
  trackStartTime = 0; trackDuration = 0;  // 重置播放时间
  currentPlayTime = 0; totalPlayTime = 0;  // 重置当前和总播放时间
  timeInfoAvailable = false; lastPlayTimeUpdate = 0;  // 重置时间信息

  for (int i = 0; i < MAX_TRACKS; i++) { if (trackList) { trackList[i] = ""; trackList[i].reserve(0); } }  // 先清空内容（字符串堆内存归还）
  freeSDBuffers();                        // 释放 trackList / scanFoundPaths 数组，归还 ~12KB 对象内存
  sdScanInProgress = false; sdScanComplete = false;  // 重置扫描状态
  sdScanProgress = 0; sdScanStatus = "";  // 重置扫描进度和状态

  safeStopAudioSystem();                  // 安全停止音频系统
  unsigned long cleanupStart = millis();  // 记录清理开始时间
  while (audioSystemStopping && (millis() - cleanupStart < 3000)) {  // 等待音频系统停止
    delay(30); esp_task_wdt_reset();      // 延时并重置看门狗
  }
  if (audioSystemStopping) audioSystemStopping = false;  // 强制设置停止标志

  cleanupFromSDToNetworkStream();         // 从SD清理到网络流
  reconnectWiFiAfterSDPlayback();         // SD播放后重连WiFi
  safeRestartAudioSystem();                // 安全重启音频系统
  digitalWrite(I2S_SD_MODE, 1);           // 开启I2S
  resetScrollState();                     // 重置滚动状态

  aggressiveMemoryCleanup();               // 激进内存清理
  delay(100);                             // 延时100ms
  ESP.getFreeHeap();                       // 获取空闲堆内存

  // 和从播放器选择界面进入网络播放器一样的流程
  menuState = MENU_WIFI_CONNECTING;       // 进入WiFi连接状态
  menuStartTime = millis();               // 记录开始时间
  stationConnectAnimationAngle = random(0, 12) * 30;  // 随机动画角度
  strcpy(stationString, "正在扫描WiFi...");  // 设置显示信息
  strcpy(infoString, "请稍候");

  sdExitingInProgress = false;            // 退出完成
}

#endif // SD_PLAYER_H  // 结束SD_PLAYER_H头文件保护