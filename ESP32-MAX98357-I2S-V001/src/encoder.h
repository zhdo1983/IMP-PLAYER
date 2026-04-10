#ifndef ENCODER_H
#define ENCODER_H

#include "globals.h"

// ============================================================
// 旋转编码器 —— 状态机方式（彻底解决每转一格跳2步的问题）
//
// 原理：
//   标准机械编码器每转一个"物理格"，CLK 和 DT 会各产生一次
//   完整的方波脉冲，对应 4 个状态转换：
//     顺时针一格：00→10→11→01→00
//     逆时针一格：00→01→11→10→00
//   用 2 位状态（CLK<<1 | DT）跑完整的 4 步转换表，
//   只在累积满 4 步（一个完整物理格）时才输出 +1 或 -1，
//   任何中间噪声/抖动都不会被计数，从根本上避免跳步。
// ============================================================

// 完整 4 步状态转换表
// 索引 = (lastState<<2) | currState（4 位，0-15）
// 值：+1 = 顺时针一步，-1 = 逆时针一步，0 = 无效/噪声
static const int8_t ENCODER_TABLE[16] PROGMEM = {
//  curr: 00  01  10  11
         0,  -1,  1,  0,   // last=00
         1,   0,  0, -1,   // last=01
        -1,   0,  0,  1,   // last=10
         0,   1, -1,  0    // last=11
};

// 累积步数：凑满 ±4 步才输出一个计数（= 一个物理格）
static volatile int8_t encoderAccum = 0;

void IRAM_ATTR updateEncoder() {
    static uint8_t lastState = 0;  // 上次 (CLK<<1 | DT) 状态

    // 同时读取两个引脚，减少时间差
    uint8_t clk = (uint8_t)digitalRead(ENCODER_CLK);
    uint8_t dt  = (uint8_t)digitalRead(ENCODER_DT);
    uint8_t currState = (clk << 1) | dt;

    if (currState == lastState) return;  // 状态未变，忽略

    // 查表获取本次转换的方向步进
    int8_t step = pgm_read_byte(&ENCODER_TABLE[(lastState << 2) | currState]);
    lastState = currState;

    if (step == 0) return;  // 无效转换（噪声），忽略

    encoderAccum += step;

    // 累积满 4 步 = 完成一个物理格，输出一次计数
    if (encoderAccum >= 4) {
        encoderAccum = 0;
        encoderPos++;
        encoderDelta = 1;
        encoderChanged = true;
        lastEncoderTime = millis();
    } else if (encoderAccum <= -4) {
        encoderAccum = 0;
        encoderPos--;
        encoderDelta = -1;
        encoderChanged = true;
        lastEncoderTime = millis();
    }
}

// 按钮中断（300ms 去抖动）
void IRAM_ATTR handleButtonPress() {
    if (millis() - lastButtonPress > 300) {
        buttonPressed = true;
        lastButtonPress = millis();
    }
}

// 初始化编码器引脚和中断
void setupEncoder() {
    pinMode(ENCODER_CLK, INPUT_PULLUP);
    pinMode(ENCODER_DT,  INPUT_PULLUP);
    pinMode(ENCODER_SW,  INPUT_PULLUP);
    lastButtonPress = millis();
    // CLK 和 DT 都注册 CHANGE 中断，让状态机能跟踪所有边沿
    attachInterrupt(digitalPinToInterrupt(ENCODER_CLK), updateEncoder, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENCODER_DT),  updateEncoder, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENCODER_SW),  handleButtonPress, FALLING);
}

#endif // ENCODER_H