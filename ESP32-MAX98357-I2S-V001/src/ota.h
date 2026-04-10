#ifndef OTA_H  // 防止头文件重复包含
#define OTA_H  // 定义OTA头文件宏

#include "globals.h"  // 包含全局变量头文件
#include <HTTPClient.h>  // 包含HTTP客户端库
#include <WiFiClientSecure.h>  // 包含HTTPS客户端库
#include <Update.h>  // 包含OTA更新库

// ===================================================================
// OTA URL 的 NVS 加载与保存
// URL 优先从 NVS 读取；NVS 为空时回落到编译期默认值（config.h）。
// 用户可通过 Web 界面 /ota/config 修改并持久化到 NVS。
// ===================================================================

// 从 NVS 加载 OTA URL（在 setup() 早期调用一次）
void loadOtaUrls() {
  String ver = preferences.getString(OTA_NVS_KEY_VER_URL, "");
  String fw  = preferences.getString(OTA_NVS_KEY_FW_URL,  "");

  OTA_VERSION_URL  = (ver.length() > 0) ? ver : String(OTA_DEFAULT_VER_URL);
  OTA_FIRMWARE_URL = (fw.length()  > 0) ? fw  : String(OTA_DEFAULT_FW_URL);

  Serial.println(F("[OTA] URLs loaded."));
}

// 将新 OTA URL 保存到 NVS（由 handleOtaConfig() Web处理函数调用）
void saveOtaUrls(const String& verUrl, const String& fwUrl) {
  preferences.putString(OTA_NVS_KEY_VER_URL, verUrl);
  preferences.putString(OTA_NVS_KEY_FW_URL,  fwUrl);
  OTA_VERSION_URL  = verUrl;
  OTA_FIRMWARE_URL = fwUrl;
  Serial.println(F("[OTA] URLs saved to NVS."));
}

// 获取当前固件版本（从NVS或使用默认值）  // 获取固件版本函数注释
String getFirmwareVersion() {  // 获取当前固件版本字符串
  String version = preferences.getString(OTA_VERSION_KEY, "");  // 从NVS读取版本号
  if (version == "") {  // 如果NVS中无版本号
    version = DEFAULT_VERSION;  // 使用默认版本号
    preferences.putString(OTA_VERSION_KEY, version);  // 保存到NVS
    Serial.println("Version not found in NVS, set to default: " DEFAULT_VERSION);  // 打印信息
  }
  return version;  // 返回版本号
}

// 保存固件版本到NVS  // 保存版本函数注释
void saveFirmwareVersion(const String& version) {  // 保存固件版本到NVS
  preferences.putString(OTA_VERSION_KEY, version);  // 保存版本字符串
  Serial.print("Firmware version saved to NVS: "); Serial.println(version);  // 打印保存成功信息
}

// 将版本字符串转为数值（支持1.1和1.1.01格式）  // 版本号解析函数注释
float parseVersionNumber(const String& versionStr) {  // 将版本字符串转换为数值
  String str = versionStr;  // 复制字符串
  str.trim();  // 去除首尾空格
  int firstDot = str.indexOf('.');  // 查找第一个点号
  if (firstDot > 0) {  // 如果找到点号
    String major = str.substring(0, firstDot);  // 获取主版本号
    String rest  = str.substring(firstDot + 1);  // 获取剩余部分
    int secondDot = rest.indexOf('.');  // 查找第二个点号
    if (secondDot > 0) {  // 如果有第二个点号（格式如1.1.01）
      String minor = rest.substring(0, secondDot);  // 获取次版本号
      String sub   = rest.substring(secondDot + 1);  // 获取子版本号
      return major.toFloat() * 100 + minor.toFloat() * 10 + sub.toFloat();  // 计算数值
    } else {  // 格式如1.1
      return major.toFloat() * 100 + rest.toFloat() * 10;  // 计算数值
    }
  }
  return str.toFloat() * 100;  // 只有主版本号的情况
}

// 检查服务器上的版本号  // OTA版本检查函数注释
void checkOTAVersion() {  // 检查是否有新版本可用
  if (otaCheckCompleted) return;  // 如果已经检查过则返回

  Serial.println("[OTA] checkOTAVersion called");  // 打印函数调用信息
  Serial.print("[OTA] WiFi status: "); Serial.println(WiFi.status());  // 打印WiFi状态
  Serial.print("[OTA] Version URL: "); Serial.println(OTA_VERSION_URL);  // 打印当前版本检查URL

  if (WiFi.status() != WL_CONNECTED) {  // 如果WiFi未连接
    Serial.println("[OTA] WiFi NOT connected");  // 打印未连接信息
    otaCheckResult = 3;  // 设置检查结果为失败
    otaCheckCompleted = true;  // 标记检查完成
    return;  // 返回
  }

  HTTPClient http;  // 创建HTTP客户端
  WiFiClientSecure client;  // 创建HTTPS客户端
  client.setInsecure();  // 设置不验证证书（自签名证书场景）
  http.begin(client, OTA_VERSION_URL);  // 开始连接版本检查URL
  int httpCode = http.GET();  // 发送GET请求
  Serial.print("[OTA] HTTP code: "); Serial.println(httpCode);  // 打印HTTP响应码

  if (httpCode == HTTP_CODE_OK) {  // 如果HTTP响应成功
    String payload = http.getString();  // 获取响应内容
    Serial.print("[OTA] Server response: "); Serial.println(payload);  // 打印响应内容

    int versionNumIndex = -1;  // 版本号起始位置
    for (int i = 0; i < (int)payload.length() - 2; i++) {  // 搜索版本号位置
      if (isdigit(payload.charAt(i)) && payload.charAt(i + 1) == '.') {  // 找到数字+点的模式
        versionNumIndex = i;  // 记录位置
        break;  // 退出循环
      }
    }

    if (versionNumIndex >= 0) {  // 如果找到版本号
      String versionStr = "";  // 版本号字符串
      for (int i = versionNumIndex; i < (int)payload.length() && i < versionNumIndex + 8; i++) {  // 提取版本号
        char c = payload.charAt(i);
        if (isdigit(c) || c == '.') versionStr += c;  // 只保留数字和点号
        else break;  // 遇到其他字符停止
      }
      versionStr.trim();  // 去除空格
      serverVersionStr = versionStr;  // 保存版本字符串
      serverVersionNum = parseVersionNumber(versionStr);  // 解析版本号数值

      Serial.print("[OTA] Server version: ");  Serial.println(serverVersionNum);  // 打印服务器版本
      Serial.print("[OTA] Current version: "); Serial.println(currentVersionNum);  // 打印当前版本

      if (serverVersionNum > currentVersionNum) {  // 如果服务器版本更新
        otaUpdateAvailable = true;  // 标记有可用更新
        otaCheckResult = 1;  // 设置结果为有新版本
        Serial.println("[OTA] New version available!");  // 打印提示
      } else {  // 如果已是最新
        otaUpdateAvailable = false;  // 标记无可用更新
        otaCheckResult = 2;  // 设置结果为已是最新
        Serial.println("[OTA] Already at latest version");  // 打印提示
      }
    } else {  // 无法解析版本号
      Serial.println("[OTA] Could not parse version");  // 打印错误
      otaCheckResult = 3;  // 设置结果为失败
    }
  } else {  // HTTP请求失败
    Serial.print("[OTA] HTTP error: "); Serial.println(httpCode);  // 打印错误码
    otaCheckResult = 3;  // 设置结果为失败
  }

  http.end();  // 关闭HTTP连接
  otaCheckCompleted = true;  // 标记检查完成

  if (otaCheckResult == 1) {  // 如果有新版本
    menuState = MENU_OTA_CONFIRM;  // 进入OTA确认菜单
    menuIndex = 0;  // 重置菜单索引
    menuStartTime = millis();  // 记录时间
  }
}

// 执行OTA升级  // OTA升级执行函数注释
void performOTAUpdate() {  // 执行固件在线升级
  if (WiFi.status() != WL_CONNECTED) {  // 如果WiFi未连接
    delay(2000);  // 等待2秒
    ESP.restart();  // 重启设备
    return;  // 返回
  }

  Serial.print("[OTA] Downloading firmware from: "); Serial.println(OTA_FIRMWARE_URL);

  HTTPClient http;  // 创建HTTP客户端
  WiFiClientSecure client;  // 创建HTTPS客户端
  client.setInsecure();  // 设置不验证证书（自签名证书场景）
  http.begin(client, OTA_FIRMWARE_URL);  // 连接固件下载URL
  int httpCode = http.GET();  // 发送GET请求

  if (httpCode != HTTP_CODE_OK) {  // 如果下载失败
    Serial.printf("[OTA] Download failed, HTTP %d\n", httpCode);
    http.end();  // 关闭连接
    delay(2000);  // 等待2秒
    ESP.restart();  // 重启设备
    return;  // 返回
  }

  int contentLength = http.getSize();  // 获取固件大小
  if (!Update.begin(contentLength)) {  // 如果开始更新失败
    http.end();  // 关闭连接
    delay(2000);  // 等待2秒
    ESP.restart();  // 重启设备
    return;  // 返回
  }

  WiFiClient* stream = http.getStreamPtr();  // 获取流指针
  size_t written = 0;  // 已写入字节数
  otaProgress = 0;  // 初始化进度

  while (http.connected() && (written < (size_t)contentLength)) {  // 循环下载并写入
    size_t available = stream->available();  // 获取可用字节数
    if (available) {  // 如果有数据
      uint8_t buffer[128];  // 读取缓冲区
      int bytesRead = stream->readBytes(buffer, min(available, sizeof(buffer)));  // 读取数据
      written += Update.write(buffer, bytesRead);  // 写入固件
      otaProgress = map(written, 0, contentLength, 0, 100);  // 计算进度百分比
    }
    delay(1);  // 短暂延时
  }

  http.end();  // 关闭HTTP连接

  if (Update.end(true)) {  // 如果更新完成
    saveFirmwareVersion(serverVersionStr);  // 保存新版本号
    delay(1000);  // 等待1秒
    ESP.restart();  // 重启设备
  } else {  // 如果更新失败
    delay(2000);  // 等待2秒
    ESP.restart();  // 重启设备
  }
}

#endif // OTA_H  // 结束头文件保护