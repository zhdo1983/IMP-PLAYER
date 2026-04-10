#ifndef BATTERY_H  // 防止头文件重复包含
#define BATTERY_H  // 定义电池头文件宏

#include "globals.h"  // 包含全局变量头文件

// 绘制电池图标（12x6像素，显示在屏幕角落）  // 电池图标绘制函数注释
void drawBatteryIcon(int x, int y) {  // 在指定位置绘制电池图标
  u8g2.drawFrame(x, y, 10, 6);      // 绘制电池主体外框（10x6像素）
  u8g2.drawBox(x + 10, y + 2, 2, 2); // 绘制电池正极（右侧凸起）

  if (batteryLevelPercent >= 70) {  // 电量大于等于70%
    u8g2.drawBox(x + 1, y + 1, 2, 4);  // 绘制第1格
    u8g2.drawBox(x + 4, y + 1, 2, 4);  // 绘制第2格
    u8g2.drawBox(x + 7, y + 1, 2, 4);  // 绘制第3格（满格）
  } else if (batteryLevelPercent >= 30) {  // 电量30%-70%
    u8g2.drawBox(x + 1, y + 1, 2, 4);  // 绘制第1格
    u8g2.drawBox(x + 4, y + 1, 2, 4);  // 绘制第2格
  } else {  // 电量低于30%
    u8g2.drawBox(x + 1, y + 1, 2, 4);  // 仅绘制第1格（低电量警告）
  }
}

// 检测电池电量（滑动平均，每次call读一次ADC）  // 电池电量检测函数注释
void checkBatteryLevel() {  // 检测并更新电池电量百分比
  static uint16_t adcReadings[5] = {0};  // ADC读数缓存数组（用于滑动平均）
  static uint8_t readingIndex = 0;  // 当前读取位置索引
  static uint8_t numReadings = 0;  // 当前有效读数数量

  int adcValue = analogRead(BAT_DAC);  // 读取电池ADC值
  adcReadings[readingIndex] = adcValue;  // 存入缓存数组
  readingIndex = (readingIndex + 1) % 5;  // 移动到下一个位置（循环）
  if (numReadings < 5) numReadings++;  // 累计有效读数

  if (numReadings >= 5) {  // 已有5个有效读数，可以计算平均值
    uint32_t sum = 0;  // 求和变量
    for (int i = 0; i < 5; i++) sum += adcReadings[i];  // 累加所有读数
    uint16_t avgAdc = sum / 5;  // 计算平均值

    // 低于3.0V阈值，进入深度睡眠  // 低电压保护
    if (avgAdc < 1860) {  // ADC值低于1860（约3.0V）
      Serial.println("电池电量严重不足，设备将进入休眠模式。");  // 打印警告信息
      u8g2.clearBuffer();  // 清空显示缓冲区
      u8g2.setFont(u8g2_font_wqy12_t_gb2312);  // 设置中文字体
      u8g2.drawStr(0, 20, "电池电量低");  // 显示低电量提示
      u8g2.drawStr(0, 40, "即将休眠...");  // 显示即将休眠提示
      u8g2.sendBuffer();  // 发送缓冲区到显示
      delay(3000);  // 等待3秒
      digitalWrite(I2S_SD_MODE, HIGH);  // 关闭功放
      delay(100);  // 等待稳定
      esp_deep_sleep_start();  // 进入深度睡眠模式
    }

    // 根据ADC值计算电量百分比  // 电量等级判断
    if      (avgAdc >= 2350) batteryLevelPercent = 100;  // >=4.2V时满电
    else if (avgAdc >= 2236) batteryLevelPercent = 70;  // >=4.0V时70%
    else if (avgAdc >= 2100) batteryLevelPercent = 30;  // >=3.75V时30%
    else                     batteryLevelPercent = 10;  // 低于3.75V时10%

    // 打印电池信息到串口  // 调试输出
    Serial.print("Battery ADC: "); Serial.print(adcValue);  // 打印原始ADC值
    Serial.print(", Avg: ");       Serial.print(avgAdc);  // 打印平均值
    Serial.print(", Level: ");     Serial.print(batteryLevelPercent);  // 打印电量百分比
    Serial.println("%");  // 打印百分号并换行
  }
}

#endif // BATTERY_H  // 结束头文件保护
