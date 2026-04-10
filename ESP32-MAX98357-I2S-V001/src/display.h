#ifndef DISPLAY_H  // 防止头文件重复包含
#define DISPLAY_H  // 定义显示头文件宏

#include "globals.h"  // 包含全局变量头文件
#include "battery.h"  // 包含电池头文件

// ---- 滚动文本（从左边开始显示，超宽则自动滚动） ----
void displayScrollableText(const char* text, int yPos) {
  static int offset = 0;
  static unsigned long lastScrollUpdate = 0;
  static char lastText[MAX_INFO_LENGTH] = "";
  const int scrollInterval = 300;

  // 文字内容变化时重置offset，防止旧offset超出新文字范围导致渲染异常
  if (strncmp(text, lastText, MAX_INFO_LENGTH) != 0) {
    offset = 0;
    lastScrollUpdate = 0;
    strncpy(lastText, text, MAX_INFO_LENGTH - 1);
    lastText[MAX_INFO_LENGTH - 1] = '\0';
  }

  u8g2.setFont(u8g2_font_wqy12_t_gb2312);
  int textPixelWidth = u8g2.getUTF8Width(text);

  if (textPixelWidth > 128) {
    unsigned long now = millis();
    if (now - lastScrollUpdate > scrollInterval) {
      offset++;
      if (offset > textPixelWidth - 128) offset = 0;
      lastScrollUpdate = now;
    }
    u8g2.setDrawColor(1);
    u8g2.setCursor(-offset, yPos);
    u8g2.print(text);
  } else {
    u8g2.setDrawColor(1);
    u8g2.setCursor(0, yPos);
    u8g2.print(text);
  }
}

// ---- 滚动文本（居中显示，超宽则从左边界开始滚动） ----
void displayScrollableTextCentered(const char* text, int yPos) {
  static int offset = 0;
  static unsigned long lastScrollUpdate = 0;
  static char lastText[MAX_INFO_LENGTH] = "";
  const int scrollInterval = 300;

  // 文字内容变化时重置offset
  if (strncmp(text, lastText, MAX_INFO_LENGTH) != 0) {
    offset = 0;
    lastScrollUpdate = 0;
    strncpy(lastText, text, MAX_INFO_LENGTH - 1);
    lastText[MAX_INFO_LENGTH - 1] = '\0';
  }

  u8g2.setFont(u8g2_font_wqy12_t_gb2312);
  int textPixelWidth = u8g2.getUTF8Width(text);
  int screenWidth = 128;

  if (textPixelWidth > screenWidth) {
    unsigned long now = millis();
    if (now - lastScrollUpdate > scrollInterval) {
      offset++;
      if (offset > textPixelWidth - screenWidth + 16) offset = 0;
      lastScrollUpdate = now;
    }
    u8g2.setDrawColor(1);
    u8g2.setClipWindow(0, yPos - 12, 127, yPos + 2);
    u8g2.setCursor(-offset, yPos);
    u8g2.print(text);
    u8g2.setMaxClipWindow();
  } else {
    u8g2.setDrawColor(1);
    int startX = (screenWidth - textPixelWidth) / 2;
    u8g2.setCursor(startX, yPos);
    u8g2.print(text);
  }
}

// ---- 反白滚动文本（用于错误信息） ----  // 反白滚动文本函数注释
void displayInvertedScrollableText(const char* text, int yPos, int startX, int width) {  // 显示反白滚动文本
  static int errorOffset = 0;  // 错误文本偏移量
  static unsigned long lastErrorScrollUpdate = 0;  // 上次更新时间
  const int scrollInterval = 100;  // 滚动间隔（毫秒，更快）

  u8g2.setFont(u8g2_font_wqy12_t_gb2312);  // 设置中文字体
  int textPixelWidth = u8g2.getUTF8Width(text);  // 获取文本宽度
  int maxOffset = textPixelWidth + 64;  // 最大偏移量

  if (textPixelWidth > 0) {  // 如果有文本
    unsigned long now = millis();  // 获取当前时间
    if (now - lastErrorScrollUpdate > scrollInterval) {  // 达到滚动间隔
      errorOffset++;  // 增加偏移
      if (errorOffset > maxOffset) errorOffset = 0;  // 超过则重置
      lastErrorScrollUpdate = now;  // 更新时间
    }
    int cursorX = 128 - errorOffset;  // 计算光标X位置（从右向左移动）
    u8g2.setDrawColor(1);  // 设置白色
    u8g2.drawBox(startX, yPos - 10, width, 12);  // 绘制背景框
    u8g2.setMaxClipWindow();  // 设置最大裁剪窗口
    u8g2.setClipWindow(startX, yPos - 10, 128, yPos + 2);  // 设置文本裁剪区域
    u8g2.setDrawColor(0);  // 设置黑色（反白）
    u8g2.setCursor(cursorX, yPos);  // 设置光标位置
    u8g2.print(text);  // 打印文本
    u8g2.setMaxClipWindow();  // 恢复最大裁剪窗口
    u8g2.setDrawColor(1);  // 恢复白色
  }
}

// ---- WiFi信号图标（扇形弧） ----  // WiFi图标绘制函数注释
void drawWifiIcon(int x, int y) {  // 绘制WiFi信号强度图标
  int wifiLevel = 0;  // WiFi信号等级
  if (WiFi.isConnected()) {  // 如果WiFi已连接
    int rssi = WiFi.RSSI();  // 获取信号强度
    if      (rssi >= -60) wifiLevel = 3;  // 强信号（-60dBm以上）
    else if (rssi >= -70) wifiLevel = 2;  // 中等信号
    else if (rssi >= -80) wifiLevel = 1;  // 弱信号
    else                  wifiLevel = 0;  // 无信号
  }
  int cx = x + 6, cy = y + 6;  // 计算中心点
  u8g2.drawDisc((uint8_t)cx, (uint8_t)(cy + 3), 1, U8G2_DRAW_ALL);  // 绘制中心点
  if (wifiLevel >= 1) u8g2.drawEllipse((uint8_t)cx, (uint8_t)(cy+1), 3, 1, U8G2_DRAW_UPPER_RIGHT|U8G2_DRAW_UPPER_LEFT);  // 第1格信号
  if (wifiLevel >= 2) u8g2.drawEllipse((uint8_t)cx, (uint8_t)(cy-1), 5, 2, U8G2_DRAW_UPPER_RIGHT|U8G2_DRAW_UPPER_LEFT);  // 第2格信号
  if (wifiLevel >= 3) u8g2.drawEllipse((uint8_t)cx, (uint8_t)(cy-3), 7, 3, U8G2_DRAW_UPPER_RIGHT|U8G2_DRAW_UPPER_LEFT);  // 第3格信号
}

// ---- 音频质量信号图标（3格柱状） ----  // 音频质量图标函数注释
void drawAudioQualityIcon(int x, int y) {  // 绘制音频质量指示图标
  if (audioQualityStatus == "Buffer Low") {  // 缓冲区低
    u8g2.drawBox(x, y + 6, 2, 2);  // 只显示1格
  } else if (audioQualityStatus == "Buffer Full") {  // 缓冲区满
    u8g2.drawBox(x,     y + 6, 2, 2);  // 显示2格
    u8g2.drawBox(x + 3, y + 3, 2, 5);  // 第二格
  } else {  // 正常状态
    u8g2.drawBox(x,     y + 6, 2, 2);  // 第1格
    u8g2.drawBox(x + 3, y + 3, 2, 5);  // 第2格
    u8g2.drawBox(x + 6, y,     2, 8);  // 第3格（满格）
  }
}

#endif // DISPLAY_H  // 结束头文件保护
