#ifndef NAVIDROME_H
#define NAVIDROME_H

#include "globals.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <MD5Builder.h>  // ESP32 Arduino 内置，用于 token 认证

extern void handleRoot();
extern void handleConfig();
extern void handleSaveRestart();
extern void handleStationsList();
extern void handleStationUpdate();
extern void handleStationDelete();
extern void handleStationAdd();
extern void handleWifiList();
extern void handleWifiAdd();
extern void handleWifiDelete();
extern void handleNaviConfig();
extern void handleNaviPing();

// ===================================================================
// Navidrome / Subsonic API 播放器  v3
//
// 架构改进（解决播放中 TLS/RSA 内存不足问题）：
//   1. 启动时内存充裕，一次性预取 NAVI_QUEUE_SIZE(5) 首歌曲元数据
//      填满队列，TLS 握手全部在解码器启动前完成
//   2. 播放中仅在「曲末 NAVI_PREFETCH_BEFORE_END_SEC 秒内」才尝试
//      补充队列（方案一），而不是在 60% 处就抢内存
//   3. 每次补充失败后设置 NAVI_PREFETCH_COOLDOWN_MS 冷却时间，
//      最多重试 NAVI_PREFETCH_MAX_RETRY 次（方案四），杜绝无限循环
//   4. 切歌时（naviNextSong）先停止解码器释放约 68KB，再补满队列
//      TLS 握手永远在解码器不运行时完成，根本解决 57KB 内存不足
//   5. 曲目结束回调（naviOnTrackEnd）强制重置限流计数作为最后兜底
// ===================================================================

#define ND_KEY_SERVER   "nd_server"
#define ND_KEY_USER     "nd_user"
#define ND_KEY_PASS     "nd_pass"
#define ND_DEFAULT_SERVER "https://"  // 默认Navidrome服务器地址
#define ND_CLIENT_NAME  "ESP32Radio"
#define ND_API_VERSION  "1.16.1"

// ---- 队列参数 ----
#define NAVI_QUEUE_SIZE              5       // 元数据队列容量（仅存 NaviSong，不占音频内存）
// TLS 握手实际需要约 30-40KB 连续内存（mbedTLS peak）。
// WiFi 驱动在线时 maxAllocHeap 固定约 45KB，用 maxAllocHeap 做守卫会永远触发。
// 改用 getFreeHeap() 守卫：低于 50KB 才是真正的内存耗尽，正常 WiFi+TLS 场景
// freeHeap 约 86KB，远高于此阈值，不会误拦。
#define NAVI_MIN_HEAP_FOR_TLS    50000U
                                             // X509证书解析在83KB时仍失败，需110KB才安全
#define NAVI_PREFETCH_BEFORE_END_SEC  30     // 距曲末多少秒内才允许补充队列（方案一）
#define NAVI_PREFETCH_COOLDOWN_MS  60000UL  // 失败后冷却时间（方案四）
#define NAVI_PREFETCH_MAX_RETRY       2     // 单曲生命周期内最大重试次数（方案四）

// ---- 队列存储 ----
static NaviSong  _naviQueue[NAVI_QUEUE_SIZE];  // 环形队列：只存元数据
static int       _naviQHead  = 0;              // 队头索引（下一首要播放的槽）
static int       _naviQCount = 0;              // 当前有效条目数

// ---- 限流状态（方案四）----
static int           _naviRetryCount    = 0;   // 本曲已重试次数
static unsigned long _naviLastRetryTime = 0;   // 上次重试的 millis()

// ---- 连接失败重试状态（方案A）----
static NaviSong  _naviCurrentSong;             // 当前正在播放的歌曲（用于重试）
static int       _naviPlayRetryCount = 0;      // 播放失败重试次数
static int       _naviPlayRetryIdx = 0;        // 当前重试播放的歌曲索引
#define NAVI_PLAY_RETRY_MAX 2                  // 播放失败最大重试次数
#define NAVI_PLAY_RETRY_DELAY_MS 2000          // 重试前等待时间（毫秒）

// ---- 假连接释放内存标志 ----
// 为 true 时表示正在用 http://127.0.0.1/ 触发 buffers freed
// audio_info() 回调中需检查此标志，避免将假连接误判为真实连接并触发 aggressiveMemoryCleanup
extern volatile bool naviReleasingMemory;  // 定义在 main.cpp
void aggressiveMemoryCleanup();            // 定义在 audio_manager.h，此处前向声明

// ===================================================================
// NVS 读写
// ===================================================================

void loadNaviConfig() {
  naviServer = preferences.getString(ND_KEY_SERVER, ND_DEFAULT_SERVER);  // NVS为空时使用默认地址
  naviUser   = preferences.getString(ND_KEY_USER,   "");
  naviPass   = preferences.getString(ND_KEY_PASS,   "");

  // 修复：NVS 中可能保存了旧的 http:// 地址，自动升级为 https://
  if (naviServer.startsWith("http://")) {
    String hostPart = naviServer.substring(7);
    if (hostPart.indexOf(":443") >= 0 || hostPart.indexOf(":4433") >= 0) {
      naviServer = "https://" + hostPart;
      // 同步更新 NVS，避免下次仍读到旧值
      preferences.putString(ND_KEY_SERVER, naviServer);
      #if DEBUG_NAVI_VERBOSE
      Serial.println("[Navi] loadNaviConfig: auto-upgraded NVS server to HTTPS: " + naviServer);
      #endif
    }
  }

  naviConfigValid = (naviServer.length() > 0 && naviUser.length() > 0);
  Serial.println("[Navi] Config loaded.");
}

void saveNaviConfig(const String& server, const String& user, const String& pass) {
  // 正规化：将 http:// 前缀的地址（含 HTTPS 端口 4433/443）自动升级为 https://
  String fixedServer = server;
  if (fixedServer.startsWith("http://")) {
    String hostPart = fixedServer.substring(7);
    if (hostPart.indexOf(":443") >= 0 || hostPart.indexOf(":4433") >= 0) {
      fixedServer = "https://" + hostPart;
      #if DEBUG_NAVI_VERBOSE
      Serial.println("[Navi] saveNaviConfig: auto-upgraded to HTTPS: " + fixedServer);
      #endif
    }
  }
  preferences.putString(ND_KEY_SERVER, fixedServer);
  preferences.putString(ND_KEY_USER,   user);
  preferences.putString(ND_KEY_PASS,   pass);
  naviServer = fixedServer;
  naviUser   = user;
  naviPass   = pass;
  naviConfigValid = (fixedServer.length() > 0 && user.length() > 0);
  Serial.println("[Navi] Config saved to NVS.");
}

// ===================================================================
// URL 构造
// ===================================================================

static void _naviParseServer(const String& server, String& scheme, String& host) {
  if (server.startsWith("https://")) {
    scheme = "https://";
    host   = server.substring(8);
  } else if (server.startsWith("http://")) {
    scheme = "http://";
    host   = server.substring(7);
  } else {
    scheme = "http://";
    host   = server;
  }
  while (host.endsWith("/")) host.remove(host.length() - 1);
}

// ===================================================================
// MD5-salt Token 认证（Subsonic API spec §3.1）
//
// 规范：
//   salt = 随机字母数字字符串（每次调用重新生成，防重放）
//   token = MD5( password + salt )
//   请求参数：u=<user>&t=<token>&s=<salt>&v=...&c=...&f=json
//
// 优点：
//   · 密码从不出现在 URL / HTTP 日志 / 串口输出中
//   · 每次请求 salt 不同，重放攻击无效
//   · Navidrome / Airsonic / Subsonic 均支持此方式
// ===================================================================

// 生成6位随机 salt（字母+数字）
static String _naviGenSalt() {
  const char charset[] = "abcdefghijklmnopqrstuvwxyz0123456789";
  String salt = "";
  // 使用 esp_random() 提供真随机源（基于硬件RNG）
  for (int i = 0; i < 6; i++) {
    salt += charset[esp_random() % (sizeof(charset) - 1)];
  }
  return salt;
}

// 计算 MD5(password + salt)，返回32位小写十六进制字符串
static String _naviCalcToken(const String& password, const String& salt) {
  MD5Builder md5;
  md5.begin();
  md5.add(password + salt);  // MD5Builder::add() 接受 String
  md5.calculate();
  return md5.toString();     // 返回32位小写十六进制
}

// 构造认证参数字符串（每次调用生成新 salt，密码不出现在结果中）
String naviAuthParams() {
  String salt  = _naviGenSalt();
  String token = _naviCalcToken(naviPass, salt);

  String p = "?u=" + naviUser;
  p += "&t=" + token;   // MD5(password+salt)，非明文
  p += "&s=" + salt;    // 随机盐值
  p += "&v=" + String(ND_API_VERSION);
  p += "&c=" + String(ND_CLIENT_NAME);
  p += "&f=json";
  return p;
}

String naviApiUrl(const String& endpoint, const String& extraParams = "") {
  String scheme, host;
  _naviParseServer(naviServer, scheme, host);
  return scheme + host + "/rest/" + endpoint + naviAuthParams() + extraParams;
}

// ===================================================================
// HTTP/HTTPS GET
// ===================================================================

String naviHttpGet(const String& url) {
  HTTPClient http;
  http.setTimeout(8000);
  http.setReuse(false);

  WiFiClientSecure secureClient;
  secureClient.setInsecure();
  secureClient.setTimeout(8);

  String fixedUrl = url;
  if (fixedUrl.startsWith("http://") && !fixedUrl.startsWith("https://")) {
    fixedUrl = "https://" + fixedUrl.substring(7);
  }

  http.begin(secureClient, fixedUrl);

  esp_task_wdt_reset();
  int code = http.GET();
  esp_task_wdt_reset();

  String result = "";
  if (code == HTTP_CODE_OK) {
    result = http.getString();
    esp_task_wdt_reset();
    #if DEBUG_NAVI_VERBOSE
    Serial.printf("[Navi] Response %d bytes, heap: %u KB\n",
                  result.length(), ESP.getFreeHeap() / 1024);
    #endif
  } else {
    #if DEBUG_NAVI_VERBOSE
    Serial.printf("[Navi] HTTP error: %d\n", code);
    #endif
  }

  http.end();
  return result;
}

// ===================================================================
// 核心：取 1 首随机歌曲元数据
// ===================================================================

bool naviFetchOneSong(NaviSong& song) {
  uint32_t freeHeap = ESP.getFreeHeap();
  if (freeHeap < NAVI_MIN_HEAP_FOR_TLS) {
    #if DEBUG_NAVI_VERBOSE
    Serial.printf("[Navi] Skip fetch: heap too low (%u KB < %u KB)\n",
                  freeHeap / 1024, NAVI_MIN_HEAP_FOR_TLS / 1024);
    #endif
    return false;
  }

  String url = naviApiUrl("getRandomSongs", "&size=1");
  // 仅打印域名部分，避免 token/salt 出现在串口日志
  #if DEBUG_NAVI_VERBOSE
  Serial.printf("[Navi] Fetching 1 song from %s ...\n", naviServer.c_str());
  #endif

  String response = naviHttpGet(url);
  if (response.length() == 0) {
    #if DEBUG_NAVI_VERBOSE
    Serial.println("[Navi] Empty response");
    #endif
    return false;
  }

  #if DEBUG_NAVI_VERBOSE
  Serial.print("[Navi] Preview: "); Serial.println(response.substring(0, 150));
  #endif

  StaticJsonDocument<3072> doc;
  DeserializationError err = deserializeJson(doc, response);
  response = "";  // 立即释放响应体

  #if DEBUG_NAVI_VERBOSE
  if (err) {
    Serial.print("[Navi] JSON error: "); Serial.println(err.c_str());
    return false;
  }

  const char* status = doc["subsonic-response"]["status"] | "failed";
  if (strcmp(status, "ok") != 0) {
    const char* errMsg = doc["subsonic-response"]["error"]["message"] | "unknown";
    Serial.print("[Navi] API error: "); Serial.println(errMsg);
    return false;
  }
  #else
  if (err) return false;
  if (strcmp(doc["subsonic-response"]["status"] | "failed", "ok") != 0) return false;
  #endif

  JsonArray songs = doc["subsonic-response"]["randomSongs"]["song"].as<JsonArray>();
  if (songs.size() == 0) {
    #if DEBUG_NAVI_VERBOSE
    Serial.println("[Navi] No songs returned");
    #endif
    return false;
  }

  JsonObject s  = songs[0];
  song.id       = s["id"].as<String>();
  song.title    = s["title"].as<String>();
  song.artist   = s["artist"].as<String>();
  song.album    = s["album"].as<String>();
  song.duration = s["duration"].as<int>();

  #if DEBUG_NAVI_VERBOSE
  Serial.printf("[Navi] Got: %s - %s (%ds)\n",
                song.artist.c_str(), song.title.c_str(), song.duration);
  #endif
  return true;
}

// ===================================================================
// 队列操作
// ===================================================================

// 返回队列中已有的有效条目数
static inline int _naviQueueCount() { return _naviQCount; }

// 向队列尾部追加一首歌（失败时不修改队列）
static bool _naviQueuePush(const NaviSong& song) {
  if (_naviQCount >= NAVI_QUEUE_SIZE) return false;
  int tail = (_naviQHead + _naviQCount) % NAVI_QUEUE_SIZE;
  _naviQueue[tail] = song;
  _naviQCount++;
  return true;
}

// 从队列头部取出一首歌（调用前需确认 _naviQCount > 0）
static NaviSong _naviQueuePop() {
  NaviSong s = _naviQueue[_naviQHead];
  _naviQHead = (_naviQHead + 1) % NAVI_QUEUE_SIZE;
  _naviQCount--;
  return s;
}

// 填满队列：趁内存充裕一次性取若干首（每次请求后稍等让内存稳定）
// 已有条目不会重复请求
static void _naviFillQueue() {
  #if DEBUG_NAVI_VERBOSE
  Serial.printf("[Navi] Filling queue (%d/%d), heap: %u KB\n",
                _naviQCount, NAVI_QUEUE_SIZE, ESP.getFreeHeap() / 1024);
  #endif

  while (_naviQCount < NAVI_QUEUE_SIZE) {
    // 每次请求之前都再检查内存，避免多次握手导致内存不足
    if (ESP.getFreeHeap() < NAVI_MIN_HEAP_FOR_TLS) {
      #if DEBUG_NAVI_VERBOSE
      Serial.printf("[Navi] Fill stopped: heap too low (%u KB)\n",
                    ESP.getFreeHeap() / 1024);
      #endif
      break;
    }

    NaviSong song;
    bool ok = naviFetchOneSong(song);
    esp_task_wdt_reset();

    if (ok) {
      _naviQueuePush(song);
      #if DEBUG_NAVI_VERBOSE
      Serial.printf("[Navi] Queue[%d/%d]: %s\n",
                    _naviQCount, NAVI_QUEUE_SIZE, song.title.c_str());
      #endif
    } else {
      #if DEBUG_NAVI_VERBOSE
      Serial.println("[Navi] Fill: fetch failed, stopping");
      #endif
      break;  // 失败就停止，不要继续消耗内存
    }

    // 多次请求之间稍作间隔，让堆碎片整理
    if (_naviQCount < NAVI_QUEUE_SIZE) delay(200);
    esp_task_wdt_reset();
  }

  #if DEBUG_NAVI_VERBOSE
  Serial.printf("[Navi] Queue filled: %d/%d, heap: %u KB\n",
                _naviQCount, NAVI_QUEUE_SIZE, ESP.getFreeHeap() / 1024);
  #endif
}

// ===================================================================
// 流媒体 URL 构造
// ===================================================================

String naviGetStreamUrl(const String& songId) {
  String scheme, host;
  _naviParseServer(naviServer, scheme, host);
  // 修复：如果服务器以 http:// 保存但实际需要 TLS，强制升级为 https://
  // 判断依据：host 包含端口 443 或 4433，或 scheme 本身已是 https://
  if (scheme == "http://") {
    if (host.endsWith(":443") || host.endsWith(":4433") ||
        host.indexOf(":443/") >= 0 || host.indexOf(":4433/") >= 0) {
      scheme = "https://";
      #if DEBUG_NAVI_VERBOSE
      Serial.println("[Navi] StreamUrl: auto-upgraded to HTTPS");
      #endif
    }
  }

  // stream 端点同样使用 MD5-salt token 认证，密码不出现在 URL 中
  String salt  = _naviGenSalt();
  String token = _naviCalcToken(naviPass, salt);

  String url = scheme + host;
  url += "/rest/stream?id=" + songId;
  url += "&u="  + naviUser;
  url += "&t="  + token;   // MD5(password+salt)
  url += "&s="  + salt;    // 随机盐值
  url += "&v="  + String(ND_API_VERSION);
  url += "&c="  + String(ND_CLIENT_NAME);
  url += "&format=mp3";
  url += "&maxBitRate=96";
  // 日志中隐去 token/salt，防止通过日志反推密码
  #if DEBUG_NAVI_VERBOSE
  Serial.printf("[Navi] Stream: %s/rest/stream?id=%s&u=%s&t=****&s=****\n",
                (scheme + host).c_str(), songId.c_str(), naviUser.c_str());
  #endif
  return url;
}

// ===================================================================
// 播放指定歌曲对象（内部实现）
// ===================================================================

void _naviPlaySongObj(const NaviSong& song, int idx) {
  naviCurrentIdx = idx;

  // 保存当前歌曲信息（用于连接失败重试）
  _naviCurrentSong = song;
  _naviPlayRetryIdx = idx;
  _naviPlayRetryCount = 0;

  strncpy(stationString, song.title.c_str(), MAX_INFO_LENGTH - 1);
  stationString[MAX_INFO_LENGTH - 1] = '\0';
  String infoLine = song.artist + " - " + song.album;
  strncpy(infoString, infoLine.c_str(), MAX_INFO_LENGTH - 1);
  infoString[MAX_INFO_LENGTH - 1] = '\0';

  naviTrackStartTime  = 0;   // 等 stream ready 回调再计时，避免缓冲等待被计入
  naviCurrentDuration = song.duration;
  naviCurrentPlaySec  = 0;
  naviPlaying         = true;
  naviState           = NAVI_PLAYING;

  String streamUrl = naviGetStreamUrl(song.id);
  #if DEBUG_NAVI_VERBOSE
  Serial.printf("[Navi] Playing [%d]: %s\n", idx, song.title.c_str());
  #endif

  audio.stopSong();
  delay(50);
  digitalWrite(I2S_SD_MODE, 1);
  audio.setVolume(encoderPos);
  audio.connecttohost(streamUrl.c_str());
}

// ===================================================================
// 曲末队列补充（方案一 + 方案四）
// 仅在以下条件全部满足时才发起一次 HTTPS 请求：
//   a. 队列未满
//   b. 剩余播放时间 <= NAVI_PREFETCH_BEFORE_END_SEC 秒
//   c. 堆内存 >= NAVI_MIN_HEAP_FOR_TLS（守卫在 naviFetchOneSong 内）
//   d. 未处于冷却期
//   e. 未超过最大重试次数
// ===================================================================

static void _naviMaybeRefillQueue() {
  // 队列已满，无需补充
  if (_naviQCount >= NAVI_QUEUE_SIZE) return;

  // 计算剩余时间
  long remaining = (long)naviCurrentDuration - (long)naviCurrentPlaySec;

  // 时长未知时（duration == 0）不在播放中触发，由 naviOnTrackEnd 兜底
  if (naviCurrentDuration == 0) return;

  // 方案一：只在曲末窗口内才触发
  if (remaining > NAVI_PREFETCH_BEFORE_END_SEC) return;

  // 方案四：冷却期检查
  unsigned long now = millis();
  if (_naviLastRetryTime > 0 &&
      (now - _naviLastRetryTime) < NAVI_PREFETCH_COOLDOWN_MS) {
    return;  // 还在冷却期，跳过
  }

  // 方案四：最大重试次数检查
  if (_naviRetryCount >= NAVI_PREFETCH_MAX_RETRY) {
    // 已达上限，等 naviOnTrackEnd 重置计数后再试
    return;
  }

  #if DEBUG_NAVI_VERBOSE
  Serial.printf("[Navi] Refill: remaining=%lds, retry=%d/%d, heap=%uKB\n",
                remaining, _naviRetryCount, NAVI_PREFETCH_MAX_RETRY,
                ESP.getFreeHeap() / 1024);
  #endif

  _naviLastRetryTime = now;

  NaviSong song;
  if (naviFetchOneSong(song)) {
    _naviQueuePush(song);
    _naviRetryCount = 0;  // 成功后重置计数
    #if DEBUG_NAVI_VERBOSE
    Serial.printf("[Navi] Refill OK, queue=%d/%d\n", _naviQCount, NAVI_QUEUE_SIZE);
    #endif
  } else {
    _naviRetryCount++;
    #if DEBUG_NAVI_VERBOSE
    Serial.printf("[Navi] Refill failed (retry %d/%d), cooldown %lus\n",
                  _naviRetryCount, NAVI_PREFETCH_MAX_RETRY,
                  NAVI_PREFETCH_COOLDOWN_MS / 1000);
    #endif
  }
}

// ===================================================================
// 公共播放控制接口
// ===================================================================

// 播放下一首：从队列头取，队列空则同步补充
void naviNextSong() {
  #if DEBUG_NAVI_VERBOSE
  Serial.println("[Navi] naviNextSong called");
  #endif

  // ── 触发 Audio 库真正释放解码器内存 ────────────────────────────────
  // 关键发现（来自日志分析）：
  //   - audio.stopSong() 不释放解码器缓冲，堆维持在 ~56KB
  //   - audio.loop() 驱动等待 1500ms 也无效，堆依然不变
  //   - "buffers freed" 日志只出现在 connecttohost() 被调用时
  //     即：Audio 库在建立新连接时才释放上一连接的解码器内存
  //
  // 方案：先 stopSong()，再 connecttohost() 一个空白 HTTP URL，
  //       等待 "buffers freed" 触发（堆回到 ~128KB），然后停止这个假连接，
  //       此时内存充裕，再安全地做 HTTPS 元数据请求和真正的流连接。
  naviPlaying = false;
  naviState   = NAVI_LOADING;
  audio.stopSong();
  esp_task_wdt_reset();
  delay(100);

  // 用一个会立即失败的 HTTP 连接触发 buffers freed
  // 使用本机环回地址，连接会被拒绝，但 Audio 库仍会先执行内存释放
  #if DEBUG_NAVI_VERBOSE
  Serial.printf("[Navi] Pre-release heap: %uKB, triggering buffer free...\n",
                ESP.getFreeHeap() / 1024);
  #endif
  naviReleasingMemory = true;                    // 告知 audio_info 回调这是假连接
  audio.connecttohost("http://127.0.0.1/");      // 触发 buffers freed，连接必然失败

  // 等待 Audio 库完成 buffers freed（通常 <200ms）
  // 以堆内存回升超过 120KB 为判断依据，最长等 1000ms
  {
    uint32_t waitStart = millis();
    while (millis() - waitStart < 1000) {
      audio.loop();
      esp_task_wdt_reset();
      delay(20);
      if (ESP.getFreeHeap() >= 120000) break;  // 内存已充分释放
    }
  }
  audio.stopSong();   // 停止这个无效连接
  naviReleasingMemory = false;                   // 恢复正常模式
  esp_task_wdt_reset();

  // ── socket 完全关闭等待 ─────────────────────────────────────────────
  // stopSong() 后 TCP socket（fd=50）处于 TIME_WAIT 状态，
  // 若立即发起下一个真实连接，对端的 RST 包会被 Audio 库误收，
  // 产生 "Connection reset by peer" 警告（不影响播放但日志污染）。
  // 持续调用 audio.loop() 100ms，让 lwIP 完成 socket 回收。
  {
    uint32_t drainStart = millis();
    while (millis() - drainStart < 100) {
      audio.loop();
      esp_task_wdt_reset();
      delay(10);
    }
  }

  #if DEBUG_NAVI_VERBOSE
  Serial.printf("[Navi] Post-release heap: %uKB\n", ESP.getFreeHeap() / 1024);
  #endif

  // ★ 内存已充分释放，此时补满队列安全
  if (_naviQCount < NAVI_QUEUE_SIZE) {
    #if DEBUG_NAVI_VERBOSE
    Serial.printf("[Navi] Post-stop refill (queue=%d/%d, heap=%uKB)\n",
                  _naviQCount, NAVI_QUEUE_SIZE, ESP.getFreeHeap() / 1024);
    #endif
    _naviFillQueue();
  }

  NaviSong songToPlay;
  bool gotSong = false;

  if (_naviQCount > 0) {
    songToPlay = _naviQueuePop();
    gotSong    = true;
    #if DEBUG_NAVI_VERBOSE
    Serial.printf("[Navi] Using queued song (queue remaining: %d)\n", _naviQCount);
    #endif
  } else {
    // 填充仍然失败（网络问题），直接实时获取
    #if DEBUG_NAVI_VERBOSE
    Serial.println("[Navi] Fill failed, fetching directly...");
    #endif
    gotSong = naviFetchOneSong(songToPlay);
  }

  esp_task_wdt_reset();

  if (gotSong) {
    naviSongList[0] = songToPlay;
    naviSongCount   = 1;
    // 重置限流计数（新曲开始）
    _naviRetryCount    = 0;
    _naviLastRetryTime = 0;
    _naviPlaySongObj(songToPlay, 0);
  } else {
    naviStatusMsg = "获取歌曲失败";
    naviState     = NAVI_ERROR;
    strncpy(infoString, "获取歌曲失败，稍后重试", MAX_INFO_LENGTH - 1);
  }
}

// 播放上一首（随机模式下等同于下一首）
void naviPrevSong() {
  naviNextSong();
}

// 暂停/恢复
void naviTogglePause() {
  audio.pauseResume();
  if (naviState == NAVI_PLAYING) {
    naviState     = NAVI_PAUSED;
    naviStatusMsg = "已暂停";
  } else if (naviState == NAVI_PAUSED) {
    naviState     = NAVI_PLAYING;
    naviStatusMsg = "";
  }
}

// 刷新歌单（清空队列，重新开始）
void naviRefreshPlaylist() {
  _naviQHead      = 0;
  _naviQCount     = 0;
  _naviRetryCount    = 0;
  _naviLastRetryTime = 0;
  naviNextSong();
}

// 保留对外接口：naviPlaySong(idx)
void naviPlaySong(int idx) {
  naviNextSong();
}

// ===================================================================
// 初始化入口
// ===================================================================

void naviLoadRandomSongs() { /* 保留空实现，外部可能调用 */ }

bool _naviStartLoad() {
  naviPlaying = false;
  audio.stopSong();
  esp_task_wdt_reset();
  delay(100);

  // 同 naviNextSong：用假连接触发 buffers freed，释放上一个解码器的内存
  // 启动时若之前没有播放过则堆本就充裕，等待会迅速结束
  #if DEBUG_NAVI_VERBOSE
  Serial.printf("[Navi] StartLoad pre-release heap: %uKB\n", ESP.getFreeHeap() / 1024);
  #endif
  naviReleasingMemory = true;                    // 告知 audio_info 回调这是假连接
  audio.connecttohost("http://127.0.0.1/");
  {
    uint32_t waitStart = millis();
    while (millis() - waitStart < 1000) {
      audio.loop();
      esp_task_wdt_reset();
      delay(20);
      if (ESP.getFreeHeap() >= 120000) break;
    }
  }
  audio.stopSong();
  naviReleasingMemory = false;                   // 恢复正常模式
  esp_task_wdt_reset();
  // socket 完全关闭等待（防止 RST 包污染后续真实连接日志）
  {
    uint32_t drainStart = millis();
    while (millis() - drainStart < 100) {
      audio.loop();
      esp_task_wdt_reset();
      delay(10);
    }
  }
  #if DEBUG_NAVI_VERBOSE
  Serial.printf("[Navi] StartLoad post-release heap: %uKB\n", ESP.getFreeHeap() / 1024);
  #endif

  // 重置队列和限流状态
  _naviQHead         = 0;
  _naviQCount        = 0;
  _naviRetryCount    = 0;
  _naviLastRetryTime = 0;

  // 启动时内存最充裕，一次性填满队列（5 首元数据）
  // TLS 握手全部在解码器启动前完成，根本上避免播放中握手
  _naviFillQueue();

  if (_naviQCount == 0) {
    naviStatusMsg = "服务器无响应";
    naviState     = NAVI_ERROR;
    return false;
  }

  // 从队列取第一首播放
  NaviSong firstSong = _naviQueuePop();
  esp_task_wdt_reset();

  naviSongList[0] = firstSong;
  naviSongCount   = 1;
  _naviPlaySongObj(firstSong, 0);
  return true;
}

void naviStart() {
  loadNaviConfig();

  if (!naviConfigValid) {
    naviStatusMsg = "未配置，请在Web页面设置";
    naviState     = NAVI_ERROR;
    strncpy(stationString, "Navidrome", MAX_INFO_LENGTH - 1);

    if (WiFi.status() == WL_CONNECTED) {
      IPAddress ip = WiFi.localIP();
      char ipMsg[MAX_INFO_LENGTH];
      snprintf(ipMsg, sizeof(ipMsg), "访问 %d.%d.%d.%d 配置", ip[0], ip[1], ip[2], ip[3]);
      strncpy(infoString, ipMsg, MAX_INFO_LENGTH - 1);
    } else {
      strncpy(infoString, "请先连接WiFi", MAX_INFO_LENGTH - 1);
    }
    return;
  }

  // 检查 WiFi 连接状态
  if (WiFi.status() != WL_CONNECTED) {
    naviState = NAVI_CONNECTING;
    strncpy(stationString, "Navidrome", MAX_INFO_LENGTH - 1);
    strncpy(infoString,    "等待WiFi...", MAX_INFO_LENGTH - 1);
    naviStartRequested = true;  // 标记需要重试
    return;
  }

  // WiFi.status() 返回 CONNECTED 后，尝试解析域名确保网络真正可用
  IPAddress naviIP;
  if (WiFi.hostByName("music.alex1984.asia", naviIP) != 1) {
    naviState = NAVI_CONNECTING;
    strncpy(stationString, "Navidrome", MAX_INFO_LENGTH - 1);
    strncpy(infoString,    "等待网络...", MAX_INFO_LENGTH - 1);
    naviStartRequested = true;  // 标记需要重试
    return;
  }

  strncpy(stationString, "Navidrome", MAX_INFO_LENGTH - 1);
  strncpy(infoString,    "连接服务器...", MAX_INFO_LENGTH - 1);

  _naviStartLoad();
}

// ===================================================================
// 播放结束回调（在 main.cpp 的 audio_eof_mp3 中调用）
// ===================================================================

void naviOnTrackEnd() {
  if (!naviPlaying) return;
  #if DEBUG_NAVI_VERBOSE
  Serial.println("[Navi] Track ended, playing next...");
  #endif

  // 重置限流计数，给新的一曲周期完整的重试机会
  _naviRetryCount    = 0;
  _naviLastRetryTime = 0;

  naviNextSong();
}

// ===================================================================
// loop() 中调用：更新播放进度 + 曲末触发队列补充 + 连接失败重试
// ===================================================================

void naviUpdate() {
  if (naviState != NAVI_PLAYING) return;

  // 处理播放失败重试请求（方案A）
  if (naviPlayRetryRequested) {
    naviPlayRetryRequested = false;
    if (_naviPlayRetryCount < NAVI_PLAY_RETRY_MAX) {
      _naviPlayRetryCount++;
      #if DEBUG_NAVI_VERBOSE
      Serial.printf("[Navi] Retrying playback (%d/%d), waiting %dms...\n",
                    _naviPlayRetryCount, NAVI_PLAY_RETRY_MAX, NAVI_PLAY_RETRY_DELAY_MS);
      #endif
      naviState = NAVI_LOADING;
      strncpy(infoString, "连接重试中...", MAX_INFO_LENGTH - 1);
      // 延迟后重试同一首歌
      delay(NAVI_PLAY_RETRY_DELAY_MS);
      _naviPlaySongObj(_naviCurrentSong, _naviPlayRetryIdx);
      return;
    } else {
      // 重试次数用完，跳到下一首
      #if DEBUG_NAVI_VERBOSE
      Serial.println("[Navi] Retry exhausted, moving to next song...");
      #endif
      _naviPlayRetryCount = 0;
      naviNextSong();
      return;
    }
  }

  if (naviTrackStartTime == 0) return;

  naviCurrentPlaySec = (millis() - naviTrackStartTime) / 1000;

  // 曲末队列补充（方案一 + 方案四）
  // 注意：此处在解码器运行中调用，naviFetchOneSong 内部有堆内存守卫
  // 仅在剩余 <= NAVI_PREFETCH_BEFORE_END_SEC 秒时才发起请求
  _naviMaybeRefillQueue();

  // 超时保护：超过时长+3秒强制跳曲
  if (naviCurrentDuration > 0 &&
      naviCurrentPlaySec >= (unsigned long)(naviCurrentDuration + 3)) {
    #if DEBUG_NAVI_VERBOSE
    Serial.println("[Navi] Track timeout, auto-advancing...");
    #endif
    naviNextSong();
  }
}

// ===================================================================
// Web 服务器处理函数
// ===================================================================

void handleNaviConfig() {
  if (server.method() == HTTP_GET) {
    loadNaviConfig();
    // 不返回明文地址和账号，只告知前端各字段是否已有值
    String json = "{\"configured\":" + String(naviConfigValid ? "true" : "false")
                + ",\"hasServer\":" + String(naviServer.length() > 0 ? "true" : "false")
                + ",\"hasUser\":"   + String(naviUser.length()   > 0 ? "true" : "false")
                + "}";
    server.send(200, "application/json", json);
  } else {
    #if DEBUG_NAVI_VERBOSE
    Serial.println("[Navi] POST /navi/config received");
    #endif
    String newSrv = server.arg("server"); newSrv.trim();
    String newUsr = server.arg("user");   newUsr.trim();
    String newPwd = server.arg("pass");

    // 若前端未传 server，则保留 NVS 中已有的地址（含默认值）
    if (newSrv.length() == 0) {
      loadNaviConfig();
      newSrv = naviServer;
    }
    if (newSrv.length() == 0 || newUsr.length() == 0) {
      server.send(400, "application/json", "{\"error\":\"user不能为空\"}");
      return;
    }
    if (newPwd.length() == 0) newPwd = naviPass;

    saveNaviConfig(newSrv, newUsr, newPwd);
    server.send(200, "application/json", "{\"ok\":true}");
  }
}

void handleNaviPing() {
  loadNaviConfig();
  if (naviServer.length() == 0) {
    server.send(200, "application/json", "{\"ok\":false,\"msg\":\"未配置服务器\"}");
    return;
  }

  String url      = naviApiUrl("ping");
  #if DEBUG_NAVI_VERBOSE
  Serial.printf("[Navi] Ping %s ...\n", naviServer.c_str());
  #endif
  String response = naviHttpGet(url);

  if (response.length() == 0) {
    server.send(200, "application/json", "{\"ok\":false,\"msg\":\"无法连接服务器\"}");
    return;
  }

  bool ok = response.indexOf("\"status\":\"ok\"") >= 0;
  String result = ok ? "{\"ok\":true,\"msg\":\"连接成功\"}"
                     : "{\"ok\":false,\"msg\":\"认证失败或服务器错误\"}";
  server.send(200, "application/json", result);
}

#endif // NAVIDROME_H