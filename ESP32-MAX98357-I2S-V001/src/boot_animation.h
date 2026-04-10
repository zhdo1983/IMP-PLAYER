//=============================================================================
// 开机动画头文件
// 功能: ESP32网络播放器开机动画显示
// 更加现代感和音乐感的动画效果
//=============================================================================

#ifndef BOOT_ANIMATION_H
#define BOOT_ANIMATION_H

// 引入Arduino核心库和U8G2显示库
#include <Arduino.h>
#include <U8g2lib.h>
#include <math.h>

//=============================================================================
// 动画参数定义
//=============================================================================

// 动画总帧数
#define BOOT_ANIMATION_FRAMES 20

// 每帧动画持续时间（毫秒）
#define BOOT_ANIMATION_DURATION 150

//=============================================================================
// 函数声明
//=============================================================================

void drawBootAnimationFrame(U8G2& u8g2, int frame, unsigned long elapsedTime);

//=============================================================================
// 动画帧绘制函数实现
void drawBootAnimationFrame(U8G2& u8g2, int frame, unsigned long elapsedTime) {
    u8g2.clearBuffer();
    
    int centerX = 64;
    int centerY = 32;
    
    //=========================================================================
    // 音频波形效果 - 从中心向两侧扩散的动态波形
    //=========================================================================
    
    // 计算波形数量（基于帧数递增）
    int waveCount = min(frame + 3, 12);
    
    for (int i = 0; i < waveCount; i++) {
        // 计算每条波动的位置和高度
        float offset = i * 3;
        // 波动高度随时间变化
        int height = 8 + sin((elapsedTime / 100.0) + i * 0.5) * 6 + i * 2;
        height = constrain(height, 4, 20);
        
        // 计算颜色深度（越靠近中心越亮）
        int alpha = map(abs(i - waveCount/2), 0, waveCount/2, 10, 4);
        
        // 绘制上下对称的波动条
        int barWidth = 4;
        int barX = centerX - waveCount * barWidth / 2 + i * barWidth;
        
        // 上半部分
        u8g2.drawBox(barX, centerY - height, barWidth - 1, height);
        // 下半部分
        u8g2.drawBox(barX, centerY + 1, barWidth - 1, height);
    }
    
    //=========================================================================
    // 中心动态圆环 - 模拟音乐节奏
    //=========================================================================
    
    // 外圈 - 顺时针旋转
    int outerRingPhase = (elapsedTime / 20) % 360;
    for (int i = 0; i < 8; i++) {
        float angle = (outerRingPhase + i * 45) * PI / 180.0;
        int dotX = centerX + 28 * cos(angle);
        int dotY = centerY + 28 * sin(angle);
        int dotSize = (i == 0) ? 2 : 1;
        if (dotSize == 2) {
            u8g2.drawDisc(dotX, dotY, 2);
        } else {
            u8g2.drawPixel(dotX, dotY);
        }
    }
    
    // 内圈 - 逆时针旋转
    int innerRingPhase = (elapsedTime / 25) % 360;
    for (int i = 0; i < 6; i++) {
        float angle = (innerRingPhase - i * 60) * PI / 180.0;
        int dotX = centerX + 18 * cos(angle);
        int dotY = centerY + 18 * sin(angle);
        u8g2.drawPixel(dotX, dotY);
    }
    
    //=========================================================================
    // 进度条 - 底部简约风格
    //=========================================================================
    
    // 进度计算
    float progress = (float)frame / BOOT_ANIMATION_FRAMES;
    int barWidth = 100;
    int barHeight = 3;
    int barX = (128 - barWidth) / 2;
    int barY = 58;
    
    // 背景条（暗淡）
    u8g2.drawBox(barX, barY, barWidth, barHeight);
    
    // 进度条（明亮）- 带圆角效果
    int fillWidth = progress * barWidth;
    u8g2.drawBox(barX, barY, fillWidth, barHeight);
    
    // 进度点
    if (fillWidth > 4) {
        u8g2.drawDisc(barX + fillWidth, barY + 1, 2);
    }
    
    //=========================================================================
    // 文字 - 简约风格
    //=========================================================================
    
    // 根据进度淡入文字
    if (frame > 2) {
        u8g2.setFont(u8g2_font_wqy12_t_gb2312);
        const char* title = "网络收音机";
        int16_t titleWidth = u8g2.getUTF8Width(title);
        u8g2.setCursor((128 - titleWidth) / 2, 12);
        u8g2.print(title);
    }
    
    //=========================================================================
    // 发送缓冲区
    //=========================================================================
    
    u8g2.sendBuffer();
}

// 头文件保护结束
#endif
