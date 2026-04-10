//=============================================================================
// OLED 任务头文件
// 功能: 包含所有屏幕渲染逻辑：开机动画、播放器选择、SD播放器、时钟、PONG游戏等
//=============================================================================

#ifndef OLED_TASK_H
#define OLED_TASK_H

#include <Arduino.h> // 引入 Arduino 核心库
#include <U8g2lib.h> // 引入 U8g2 OLED 显示库
#include <WiFi.h> // 引入 WiFi 库
#include <math.h> // 引入数学库

#include "config.h" // 引入配置文件
#include "globals.h" // 引入全局变量头文件
#include "boot_animation.h" // 引入开机动画头文件
#include "battery.h" // 引入电池头文件
#include "display.h" // 引入显示头文件
#include "sd_player.h" // 引入 SD 卡播放器头文件
#include "ota.h" // 引入 OTA 升级头文件
#include "menu.h" // 引入菜单头文件
#include "stations.h" // 引入电台头文件
#include "navidrome.h" // 引入Navidrome播放器头文件

//============================================================================= // 分隔符
// displayMenu() - 处理所有菜单状态的显示 // 函数说明
//============================================================================= // 分隔符
void displayMenu() { // 处理所有菜单状态显示的主函数
  u8g2.clearBuffer(); // 清除 OLED 缓冲区
  u8g2.setFont(u8g2_font_wqy12_t_gb2312); // 设置中文字体

  // ==================================================================== // 播放器选择界面分隔符
  // 播放器选择界面  v3 // 版本说明
  // // 注释行
  // 屏幕分区（128×64）： // 屏幕分区说明
  //   标题栏  y: 0 ~ 13   （高14px） // 标题栏区域
  //   内容区  y: 15 ~ 63  （高49px） // 内容区区域
  //     左侧缩略区  x:  0 ~ 13  （宽14px）—— 上/下家小图标 // 左侧缩略区
  //     中部图标区  x: 14 ~ 78  （宽65px）—— 选中大图标 // 中部图标区
  //     分割线      x: 79 // 分割线位置
  //     右侧信息区  x: 80 ~127  （宽48px）—— 功能名+说明 // 右侧信息区
  // // 注释行
  // 走马灯：每槽宽65px，中心X=46（中区中点），图标最大52×44px // 走马灯参数
  // 左侧区永远展示"前一个"图标缩略（10×10），右侧区永远展示"后一个" // 缩略图说明
  // ==================================================================== // 播放器选择界面分隔符
  if (menuState == MENU_PLAYER_SELECT) { // 如果是播放器选择菜单状态

    // ──────────────────────────────────────────────────────────────── // 动画状态分隔符
    // 动画状态（static 帧间保持） // 说明
    // ──────────────────────────────────────────────────────────────── // 动画状态分隔符
    static int   animTick      = 0;      // 通用节拍（每80ms +1） // 变量说明
    static unsigned long lastAnimMs = 0; // 上次动画时间记录
    // 走马灯（x16定点，每槽65px） // 走马灯说明
    static int   scrollOff     = 0; // 滚动偏移量
    static int   scrollTgt     = 0; // 滚动目标值
    static int   lastIdx       = -1; // 上一个索引值
    // 右侧文字滑入（x16定点，0=就位，正值=向下偏移） // 文字滑入说明
    static int   txtSlide      = 0; // 文字滑动偏移
    static int   txtSlideTgt   = 0; // 文字滑动目标

    unsigned long now = millis(); // 获取当前毫秒数
    if (now - lastAnimMs > 80) { animTick++; lastAnimMs = now; } // 每80ms更新节拍

    menuIndex = constrain(menuIndex, 0, 4); // 限制菜单索引在0-4之间（含音乐闹钟）
    if (menuIndex != lastIdx) { // 如果菜单索引改变
      scrollTgt   = menuIndex * (65 << 4); // 设置滚动目标
      txtSlide    = 50 << 4;   // 文字从下方50px弹入 // 文字从下方弹入
      txtSlideTgt = 0; // 目标为0
      lastIdx     = menuIndex; // 更新上一个索引
    }
    { int d=scrollTgt  -scrollOff; if(d){int s=d/4; if(!s)s=(d>0)?1:-1; scrollOff +=s;} } // 滚动平滑动画
    { int d=txtSlideTgt-txtSlide;  if(d){int s=d/3; if(!s)s=(d>0)?1:-1; txtSlide  +=s;} } // 文字滑动平滑动画

    int scrollPx = scrollOff >> 4; // 滚动像素值
    int txtOffY  = txtSlide  >> 4; // 文字Y轴偏移像素值

    // ──────────────────────────────────────────────────────────────── // 布局常量分隔符
    // 布局常量 // 说明
    // ──────────────────────────────────────────────────────────────── // 布局常量分隔符
    const int TY      = 15;    // 内容区顶部 // Y坐标
    const int BY      = 63;    // 内容区底部 // Y坐标
    const int MID_CX  = 46;    // 中区中心 X（x:14~78 → 中心46） // X坐标
    const int MID_CY  = 39;    // 中区垂直中心（y:15~63 → 中心39） // Y坐标
    const int SLOT_W  = 65;    // 每槽宽度 // 宽度常量
    const int BIG_W   = 52;    // 大图标宽 // 宽度常量
    const int BIG_H   = 44;    // 大图标高 // 高度常量
    const int SML     = 10;    // 小缩略图尺寸 // 尺寸常量
    const int DIV_X   = 79;    // 分割线X // X坐标
    const int R_X0    = 81;    // 右区起始X // X坐标

    // ──────────────────────────────────────────────────────────────── // 标题栏分隔符
    // 标题栏 // 说明
    // ──────────────────────────────────────────────────────────────── // 标题栏分隔符
    u8g2.setFont(u8g2_font_wqy12_t_gb2312); // 设置中文字体
    u8g2.setCursor(2, 12); // 设置光标位置
    u8g2.print("选择模式"); // 打印标题
    // 右上角：5个指示点（选中=实心，未选中=空心）
    for (int i = 0; i < 5; i++) {
      int dx = 90 + i * 7, dy = 3; // 5个点，间距7px
      if (i == menuIndex) u8g2.drawBox(dx, dy, 6, 6);
      else                u8g2.drawFrame(dx, dy, 6, 6);
    }
    u8g2.drawHLine(0, 13, 128); // 绘制水平分割线

    // ──────────────────────────────────────────────────────────────── // 辅助宏分隔符
    // 辅助宏：绘制小缩略图（10×10，线框风格） // 宏说明
    // 参数：iconId, 左上角 (tx, ty) // 参数说明
    // ──────────────────────────────────────────────────────────────── // 辅助宏分隔符
    // （用 do-while(0) 块模拟 inline，避免 lambda 兼容问题） // 实现说明
#define DRAW_THUMB(id, tx, ty) do { \
  if ((id) == 0) { /* SD卡 */ \
    u8g2.drawHLine(tx,   ty,   7);  u8g2.drawLine(tx+7,ty,tx+9,ty+2); \
    u8g2.drawVLine(tx+9, ty+2, 8);  u8g2.drawHLine(tx, ty+10,10); \
    u8g2.drawVLine(tx,   ty,   11); u8g2.drawFrame(tx+1,ty+1,7,4); \
    for(int _f=0;_f<3;_f++) u8g2.drawBox(tx+1+_f*3, ty+7, 2, 3); \
  } else if ((id) == 1) { /* WiFi */ \
    int _cx=(tx)+5,_cy=(ty)+9; \
    u8g2.drawEllipse(_cx,_cy,4,3,U8G2_DRAW_UPPER_RIGHT|U8G2_DRAW_UPPER_LEFT); \
    u8g2.drawEllipse(_cx,_cy,7,5,U8G2_DRAW_UPPER_RIGHT|U8G2_DRAW_UPPER_LEFT); \
    u8g2.drawDisc(_cx,_cy,1); \
  } else if ((id) == 2) { /* 云+箭头 OTA */ \
    int _cx=(tx)+5,_cy=(ty)+4; \
    u8g2.drawCircle(_cx-2,_cy+2,3,U8G2_DRAW_UPPER_LEFT|U8G2_DRAW_UPPER_RIGHT); \
    u8g2.drawCircle(_cx+2,_cy+2,3,U8G2_DRAW_UPPER_LEFT|U8G2_DRAW_UPPER_RIGHT); \
    u8g2.drawCircle(_cx,  _cy-1,4,U8G2_DRAW_UPPER_LEFT|U8G2_DRAW_UPPER_RIGHT); \
    u8g2.drawHLine(_cx-5,_cy+5,11); \
    u8g2.drawVLine(_cx,  _cy+5, 4); \
    u8g2.drawLine(_cx-2,_cy+7,_cx,_cy+9); u8g2.drawLine(_cx+2,_cy+7,_cx,_cy+9); \
  } else if ((id) == 3) { /* 音符 Navidrome */ \
    int _nx=(tx)+1,_ny=(ty)+2; \
    u8g2.drawVLine(_nx,   _ny,   7); u8g2.drawBox(_nx-1,_ny+7,3,2); \
    u8g2.drawVLine(_nx+4, _ny+2, 7); u8g2.drawBox(_nx+3,_ny+9,3,2); \
    u8g2.drawHLine(_nx,   _ny,   5); u8g2.drawHLine(_nx,_ny+1,5); \
  } else { /* 闹钟 alarm */ \
    int _ax=(tx)+2,_ay=(ty)+1; \
    u8g2.drawCircle(_ax+3,_ay+5,4,U8G2_DRAW_ALL); \
    u8g2.drawVLine(_ax+3,_ay+1,3); \
    u8g2.drawHLine(_ax+3,_ay+5,3); \
    u8g2.drawPixel(_ax,  _ay);  u8g2.drawPixel(_ax+7,_ay); \
  } \
} while(0)

    // ──────────────────────────────────────────────────────────────── // 左侧缩略区分隔符
    // 左侧缩略区（x:0~13）：展示"前一个"图标 // 说明
    // ──────────────────────────────────────────────────────────────── // 左侧缩略区分隔符
    { // 代码块
      int prevId = (menuIndex + 4) % 5; // 计算前一个选项ID（5项循环）
      int ty = MID_CY - SML / 2; // 计算Y坐标
      DRAW_THUMB(prevId, 2, ty); // 绘制缩略图
      // 向右的小箭头（提示可向左翻） // 箭头说明
      u8g2.drawLine(2, BY - 3, 5, BY - 6); // 绘制箭头
      u8g2.drawLine(2, BY - 3, 5, BY); // 绘制箭头
    }

    // ──────────────────────────────────────────────────────────────── // 中部分隔符
    // 中部大图标区（x:14~78，裁剪绘制） // 说明
    // 走马灯：slot -1 ~ 5 循环遍历（5个选项）
    // ──────────────────────────────────────────────────────────────── // 中部分隔符
    u8g2.setClipWindow(14, TY, DIV_X - 1, BY); // 设置裁剪窗口

    for (int slot = -1; slot <= 5; slot++) { // 遍历槽位（5个选项）
      int id  = ((slot % 5) + 5) % 5; // 计算选项ID（0-4循环）
      int scx = slot * SLOT_W + MID_CX - scrollPx; // 计算图标X坐标

      if (scx < -20 || scx > 100) continue; // 跳过屏幕外图标

      bool sel = (id == menuIndex) && (scx > 20) && (scx < 72); // 判断是否选中
      int  iw  = sel ? BIG_W : 14; // 选中时使用大宽度
      int  ih  = sel ? BIG_H : 14; // 选中时使用大高度
      int  ix  = scx - iw / 2; // 计算图标X
      int  iy  = MID_CY - ih / 2; // 计算图标Y

      // 选中图标：正弦轻浮 ±1px // 动画说明
      if (sel) iy += (int)(sinf(animTick * 0.35f) * 1.5f); // 添加浮动效果

      // ── 图标0：MP3 / SD卡 ────────────────────────────────────── // SD卡图标说明
      // 设计：充满中区的SD卡正视图 // 设计说明
      //   卡宽36px，卡高44px，右上切角9px // 尺寸说明
      //   上部芯片区（圆角框+十字纹路） // 芯片区说明
      //   下部5根金手指（均匀排布） // 金手指说明
      //   左下角：双音符（跳动） // 音符说明
      if (id == 0) { // 如果是SD卡图标
        if (sel) { // 选中时绘制大图标
          int sx = ix + 8, sy = iy; // 绘制起点
          int sw = 36, sh = 44, cut = 9; // SD卡尺寸
          // 卡身轮廓 // 轮廓绘制
          u8g2.drawHLine(sx,          sy,          sw - cut); // 上边
          u8g2.drawLine (sx+sw-cut,   sy,          sx+sw, sy+cut); // 右上切角
          u8g2.drawVLine(sx+sw,       sy+cut,      sh - cut); // 右边
          u8g2.drawHLine(sx,          sy+sh,       sw + 1); // 下边
          u8g2.drawVLine(sx,          sy,          sh + 1); // 左边
          u8g2.drawLine (sx+sw-cut+1, sy+1,        sx+sw-1, sy+cut-1); // 内部切角
          // 芯片区（圆角框，上方1/3处） // 芯片区绘制
          u8g2.drawRFrame(sx+3, sy+4, sw-6, 14, 2); // 绘制芯片框
          // 芯片内十字纹路 // 纹路绘制
          u8g2.drawHLine(sx+5,    sy+11, sw-10); // 水平线
          u8g2.drawVLine(sx+sw/2, sy+6,  10); // 垂直线
          // 4个芯片引脚孔（圆角框内四角） // 引脚孔绘制
          u8g2.drawPixel(sx+5,  sy+6);  u8g2.drawPixel(sx+sw-6, sy+6); // 上排
          u8g2.drawPixel(sx+5,  sy+16); u8g2.drawPixel(sx+sw-6, sy+16); // 下排
          // 金手指：5根，底部，充满卡宽 // 金手指绘制
          int fY = sy + sh - 10; // Y坐标
          for (int f = 0; f < 5; f++) { // 循环绘制5根
            u8g2.drawBox(sx + 2 + f * 6, fY, 5, 8); // 绘制金手指
          }
          // 双音符（卡面左下，随节拍上下跳动） // 双音符绘制
          int nj  = (int)(sinf(animTick * 0.5f) * 2.5f); // 计算跳动偏移
          int nx  = sx + 2; // X坐标
          int ny  = sy + 21 + nj; // Y坐标
          ny = constrain(ny, sy+18, sy+26); // 限制范围
          // 音符1 // 音符1绘制
          u8g2.drawVLine(nx,    ny,    9); // 符杆
          u8g2.drawBox  (nx-2,  ny+9,  4, 3); // 符头
          u8g2.drawLine (nx,    ny,    nx+3, ny+3); // 连接线
          u8g2.drawLine (nx,    ny+1,  nx+3, ny+4); // 连接线
          // 音符2 // 音符2绘制
          u8g2.drawVLine(nx+6,  ny+2,  9); // 符杆
          u8g2.drawBox  (nx+4,  ny+11, 4, 3); // 符头
          u8g2.drawLine (nx+6,  ny+2,  nx+9, ny+5); // 连接线
          u8g2.drawLine (nx+6,  ny+3,  nx+9, ny+6); // 连接线
          // 连梁 // 连梁绘制
          u8g2.drawHLine(nx,    ny,    7); // 第一行
          u8g2.drawHLine(nx,    ny+1,  7); // 第二行
        } else { // 未选中时绘制缩略图
          DRAW_THUMB(0, ix+2, iy+2); // 调用宏
        }

      // ── 图标1：网络播放器 ──────────────────────────────────────── // WiFi图标说明
      // 位图：耳机轮廓 + 音频波形柱（ICON_WIFI_BIG 52×44，定义于 icons.h） // 图标来源
      } else if (id == 1) { // 如果是WiFi图标
        if (sel) { // 选中时
          u8g2.drawXBMP(ix, iy, 52, 44, ICON_WIFI_BIG); // 绘制大图标
        } else { // 未选中时
          DRAW_THUMB(1, ix+2, iy+2); // 绘制缩略图
        }

      // ── 图标2：OTA 固件升级 ───────────────────────────────────── // OTA图标说明
      } else if (id == 2) { // 如果是OTA图标
        if (sel) { // 选中时
          u8g2.drawXBMP(ix, iy, 52, 44, ICON_OTA_BIG); // 绘制大图标
        } else { // 未选中时
          DRAW_THUMB(2, ix+2, iy+2); // 绘制缩略图
        }

      // ── 图标3：Navidrome 音乐服务器 ─────────────────────────── // Navidrome图标
      } else if (id == 3) { // 如果是Navidrome图标
        if (sel) { // 选中时绘制大图标（音符+服务器轮廓）
          // 外框：服务器/NAS造型（圆角矩形叠加）
          int sx = ix + 6, sy = iy + 2;
          u8g2.drawRFrame(sx, sy,      40, 12, 2); // 第一层机架
          u8g2.drawRFrame(sx, sy + 14, 40, 12, 2); // 第二层机架
          u8g2.drawRFrame(sx, sy + 28, 40, 12, 2); // 第三层机架
          // 每层右侧状态指示灯
          u8g2.drawDisc(sx+34, sy+6,  2); // 指示灯1
          u8g2.drawDisc(sx+34, sy+20, 2); // 指示灯2
          u8g2.drawDisc(sx+34, sy+34, 2); // 指示灯3
          // 每层左侧装饰线（模拟硬盘）
          u8g2.drawHLine(sx+4, sy+5,  18); // 装饰线1
          u8g2.drawHLine(sx+4, sy+19, 18); // 装饰线2
          u8g2.drawHLine(sx+4, sy+33, 18); // 装饰线3
          // 顶部音符符号（跳动动画）
          int _nj = (int)(sinf(animTick * 0.5f) * 2.0f);
          int _nx = ix + 1, _ny = sy + 6 + _nj;
          _ny = constrain(_ny, sy+3, sy+10);
          u8g2.drawVLine(_nx,   _ny,   6); u8g2.drawBox(_nx-1, _ny+6, 3, 2);
          u8g2.drawVLine(_nx+4, _ny+2, 6); u8g2.drawBox(_nx+3, _ny+8, 3, 2);
          u8g2.drawHLine(_nx,   _ny,   5); u8g2.drawHLine(_nx, _ny+1, 5);
        } else { // 未选中时绘制缩略图
          DRAW_THUMB(3, ix+2, iy+2); // 绘制缩略图
        }

      // ── 图标4：音乐闹钟 ─────────────────────────────────────────
      } else if (id == 4) {
        if (sel) {
          // 大钟表：圆形表盘 + 时针/分针 + 铃铛耳
          int cx = ix + 26, cy = iy + 24, r = 18;
          u8g2.drawCircle(cx, cy, r, U8G2_DRAW_ALL);       // 表盘外圈
          u8g2.drawCircle(cx, cy, r-2, U8G2_DRAW_ALL);     // 内圈（加粗感）
          // 铃铛两耳
          u8g2.drawCircle(cx - r + 1, cy - r + 3, 4, U8G2_DRAW_UPPER_LEFT|U8G2_DRAW_UPPER_RIGHT);
          u8g2.drawCircle(cx + r - 1, cy - r + 3, 4, U8G2_DRAW_UPPER_LEFT|U8G2_DRAW_UPPER_RIGHT);
          // 12个刻度点
          for (int t = 0; t < 12; t++) {
            float a = t * 30.0f * PI / 180.0f - PI / 2.0f;
            int tx2 = cx + (int)((r-4) * cosf(a));
            int ty2 = cy + (int)((r-4) * sinf(a));
            u8g2.drawPixel(tx2, ty2);
          }
          // 显示闹钟设定时间：时针和分针
          float hAngle = (alarmHour % 12 + alarmMinute / 60.0f) * 30.0f * PI / 180.0f - PI / 2.0f;
          float mAngle = alarmMinute * 6.0f * PI / 180.0f - PI / 2.0f;
          u8g2.drawLine(cx, cy, cx + (int)(10 * cosf(hAngle)), cy + (int)(10 * sinf(hAngle))); // 时针
          u8g2.drawLine(cx, cy, cx + (int)(14 * cosf(mAngle)), cy + (int)(14 * sinf(mAngle))); // 分针
          u8g2.drawDisc(cx, cy, 2);  // 中心点
          // 闹钟状态：启用时显示小铃铛符号，禁用时显示×
          if (alarmEnabled) {
            u8g2.drawLine(cx-3, cy+r-5, cx, cy+r-2);
            u8g2.drawLine(cx,   cy+r-2, cx+3, cy+r-5);
          } else {
            u8g2.drawLine(cx-3, cy+r-6, cx+3, cy+r-6);  // 横线表示关闭
          }
        } else {
          DRAW_THUMB(4, ix+2, iy+2);
        }
      }
    }  // end for slot // 遍历结束

    // 清理裁剪 & 边缘遮罩 // 清理说明
    u8g2.setMaxClipWindow(); // 恢复裁剪窗口
    u8g2.setDrawColor(0); // 设置绘制颜色为黑色
    u8g2.drawBox(14, TY, 3, BY - TY + 1);        // 左边渐隐（中区入口） // 左边遮罩
    u8g2.drawBox(DIV_X - 3, TY, 3, BY - TY + 1); // 右边渐隐（中区出口） // 右边遮罩
    u8g2.setDrawColor(1); // 恢复绘制颜色为白色

    // ──────────────────────────────────────────────────────────────── // 分割线分隔符
    // 分割线 // 说明
    // ──────────────────────────────────────────────────────────────── // 分割线分隔符
    u8g2.drawVLine(DIV_X, TY, BY - TY + 1); // 绘制垂直分割线

    // ──────────────────────────────────────────────────────────────── // 右侧信息区分隔符
    // 右侧信息区（x:81~127，裁剪，含 txtOffY 滑入动画） // 说明
    // 布局： // 布局说明
    //   主标签（反白，12px字体）  y: 26+off // 主标签位置
    //   副说明行1（12px）         y: 40+off // 副说明1位置
    //   副说明行2（12px）         y: 53+off // 副说明2位置
    // ──────────────────────────────────────────────────────────────── // 右侧信息区分隔符
    u8g2.setClipWindow(R_X0, TY, 127, BY); // 设置裁剪窗口

    { // 文本显示代码块
      static const char* mainL[5] = { "MP3",  "网络",  "OTA",  "音乐",  "闹钟"  }; // 主标签数组（5选项）
      static const char* sub1 [5] = { "SD卡", "WiFi", "云端", "服务器","定时"  }; // 副说明1数组
      static const char* sub2 [5] = { "播放", "串流", "升级", "串流",  "音乐"  }; // 副说明2数组
      static const char* sub3 [5] = { "本地", "音频", "固件", "Navi",  "闹钟"  }; // 副说明3数组

      u8g2.setFont(u8g2_font_wqy12_t_gb2312); // 设置中文字体

      int y0 = 27 + txtOffY;   // 主标签基线 // Y坐标
      int y1 = 40 + txtOffY;   // 副行1 // Y坐标
      int y2 = 53 + txtOffY;   // 副行2 // Y坐标

      // 主标签：反白块填充 // 主标签绘制
      u8g2.drawBox(R_X0 - 1, y0 - 12, 47, 14); // 绘制背景块
      u8g2.setDrawColor(0); // 设置反色
      { // 文本内容代码块
        int mw = u8g2.getUTF8Width(mainL[menuIndex]); // 获取宽度
        u8g2.setCursor(R_X0 + (45 - mw) / 2, y0); // 设置光标
        u8g2.print(mainL[menuIndex]); // 打印文本
      }
      u8g2.setDrawColor(1); // 恢复正常颜色

      // 副说明：两行小字（同一字体，12px，右区居中） // 副说明绘制
      { // 副说明1代码块
        int w1 = u8g2.getUTF8Width(sub1[menuIndex]); // 获取宽度
        u8g2.setCursor(R_X0 + (45 - w1) / 2, y1); // 设置光标
        u8g2.print(sub1[menuIndex]); // 打印文本
      }
      { // 副说明2代码块
        int w2 = u8g2.getUTF8Width(sub2[menuIndex]); // 获取宽度
        u8g2.setCursor(R_X0 + (45 - w2) / 2, y2); // 设置光标
        u8g2.print(sub2[menuIndex]); // 打印文本
      }
    }

    u8g2.setMaxClipWindow(); // 恢复最大裁剪窗口

#undef DRAW_THUMB // 取消宏定义

  } // 播放器选择界面结束

  // ---- 主菜单 ---- // 主菜单状态处理
  else if (menuState == MENU_MAIN) { // 如果是主菜单状态
    u8g2.setCursor(0, 15); u8g2.print("主菜单"); // 打印主菜单标题
    unsigned long elapsedTime = (millis() - menuStartTime) / 1000; // 计算已过时间（秒）
    int remainingTime = 10 - elapsedTime; // 计算剩余时间
    if (remainingTime > 0) { // 如果还有剩余时间
      char countdownStr[8]; snprintf(countdownStr, sizeof(countdownStr), "%d秒", remainingTime); // 格式化倒计时字符串
      u8g2.setCursor(108, 15); u8g2.print(countdownStr); // 打印倒计时
    }
    u8g2.drawHLine(0, 17, 128); // 绘制水平分割线
    const char* mainMenuItems[8] = {"1.选择电台", "2.SD卡播放器", "3.Navidrome", "4.音乐闹钟", "5.PONG游戏", "6.WiFi设置", "7.恢复出厂设置", "8.内存分配"}; // 主菜单选项数组
    const int mainMenuCount = 8; // 主菜单选项数量
    int startIdx = constrain(menuIndex - 1, 0, mainMenuCount - 3); // 计算起始索引
    for (int i = 0; i < 3; i++) { // 遍历显示3个选项
      int idx = startIdx + i; // 计算当前选项索引
      if (idx < mainMenuCount) { // 如果索引有效
        if (idx == menuIndex) { // 如果是选中项
          u8g2.setDrawColor(1); u8g2.drawBox(0, 30+i*15-10, 128, 12); u8g2.setDrawColor(0); // 绘制反白背景
          u8g2.setCursor(2, 30+i*15); u8g2.print(mainMenuItems[idx]); // 打印选中项
          u8g2.setDrawColor(1); // 恢复正常颜色
        } else { // 如果不是选中项
          u8g2.setCursor(2, 30+i*15); u8g2.print(mainMenuItems[idx]); // 打印普通项
        }
      }
    }
  }

  // ---- 内存分配信息 ---- // 内存信息菜单状态处理
  else if (menuState == MENU_MEMORY_INFO) { // 如果是内存信息菜单状态
    u8g2.setCursor(0, 15); u8g2.print("内存分配"); // 打印标题
    u8g2.drawHLine(0, 17, 128); // 绘制水平分割线
    MemoryInfo memInfo = getMemoryInfo(); // 获取内存信息
    u8g2.setFont(u8g2_font_ncenB08_tr); // 设置英文字体
    char line[32]; // 缓冲字符串
    snprintf(line, sizeof(line), "Total:%luKB Free:%luKB", memInfo.totalHeap/1024, memInfo.freeHeap/1024); // 格式化总内存和空闲内存
    u8g2.setCursor(2, 30); u8g2.print(line); // 打印内存信息
    snprintf(line, sizeof(line), "WiFi:%luKB SSL:%luKB", memInfo.wifiMemory, memInfo.sslMemory); // 格式化WiFi和SSL内存
    u8g2.setCursor(2, 42); u8g2.print(line); // 打印内存信息
    snprintf(line, sizeof(line), "Audio:%luKB", memInfo.audioMemory); // 格式化音频内存
    u8g2.setCursor(2, 54); u8g2.print(line); // 打印内存信息
  }

  // ---- SD播放器控制菜单 ---- // SD卡控制菜单状态处理
  else if (menuState == MENU_SD_CONTROL) { // 如果是SD卡控制菜单状态
    u8g2.setCursor(0, 15); u8g2.print("播放器控制"); // 打印标题
    u8g2.drawHLine(0, 17, 128); // 绘制水平分割线
    const char* controlItems[8] = {"1.下一首", "2.上一首", "3.播放/暂停", "4.播放模式", "5.PONG游戏", "6.切换效果图", "7.刷新曲库", "8.退出"}; // 控制选项数组（含刷新曲库）
    int controlCount = 8; // 控制选项数量
    int startIdx = constrain(menuIndex - 1, 0, controlCount - 3); // 计算起始索引
    for (int i = 0; i < 3; i++) { // 遍历显示3个选项
      int idx = startIdx + i; // 计算当前选项索引
      if (idx < controlCount) { // 如果索引有效
        if (idx == menuIndex) { // 如果是选中项
          u8g2.setDrawColor(1); u8g2.drawBox(0, 30+i*15-10, 128, 12); u8g2.setDrawColor(0); // 绘制反白背景
          u8g2.setCursor(2, 30+i*15); u8g2.print(controlItems[idx]); // 打印选中项
          u8g2.setDrawColor(1); // 恢复正常颜色
        } else { // 如果不是选中项
          u8g2.setCursor(2, 30+i*15); u8g2.print(controlItems[idx]); // 打印普通项
        }
      }
    }
    unsigned long elapsedTime2 = (millis() - menuStartTime) / 1000; // 计算已过时间
    int remaining2 = 10 - elapsedTime2; // 计算剩余时间
    if (remaining2 > 0) { // 如果还有剩余时间
      char buf[8]; snprintf(buf, sizeof(buf), "%d秒", remaining2); // 格式化倒计时
      u8g2.setCursor(100, 62); u8g2.print(buf); // 打印倒计时
    }
  }

  // ---- 播放模式菜单 ---- // 播放模式菜单状态处理
  else if (menuState == MENU_PLAY_MODE) { // 如果是播放模式菜单状态
    u8g2.setCursor(0, 15); u8g2.print("播放模式"); // 打印标题
    u8g2.drawHLine(0, 17, 128); // 绘制水平分割线
    const char* modeItems[3] = {"1.单曲循环", "2.顺序播放", "3.随机播放"}; // 播放模式选项数组
    for (int i = 0; i < 3; i++) { // 遍历显示3个选项
      if (i == menuIndex) { // 如果是选中项
        u8g2.setDrawColor(1); u8g2.drawBox(0, 30+i*15-10, 128, 12); u8g2.setDrawColor(0); // 绘制反白背景
        u8g2.setCursor(2, 30+i*15); u8g2.print(modeItems[i]); // 打印选中项
        u8g2.setDrawColor(1); // 恢复正常颜色
      } else { // 如果不是选中项
        u8g2.setCursor(2, 30+i*15); u8g2.print(modeItems[i]); // 打印普通项
      }
    }
  }

  // ---- 电台选择菜单 ---- // 电台选择菜单状态处理
  else if (menuState == MENU_STATION_SELECT) { // 如果是电台选择菜单状态
    u8g2.setCursor(0, 15); u8g2.print("电台选择"); // 打印标题
    u8g2.drawHLine(0, 17, 128); // 绘制水平分割线
    int validCount = getValidStationCount(); // 获取有效电台数量
    int startIdx = constrain(menuIndex > 1 ? menuIndex - 1 : 0, 0, validCount - 1); // 计算起始索引
    if (validCount > 3 && startIdx > validCount - 3) startIdx = validCount - 3; // 调整起始索引
    for (int i = 0; i < 3; i++) { // 遍历显示3个选项
      int idx = startIdx + i; // 计算当前选项索引
      if (idx < validCount) { // 如果索引有效
        int realIdx = getValidStationIndex(idx); // 获取真实电台索引
        if (idx == menuIndex) { // 如果是选中项
          u8g2.setDrawColor(1); u8g2.drawBox(0, 30+i*15-10, 128, 12); u8g2.setDrawColor(0); // 绘制反白背景
          u8g2.setCursor(2, 30+i*15); u8g2.print(stationNames[realIdx]); // 打印选中电台名
          u8g2.setDrawColor(1); // 恢复正常颜色
        } else { // 如果不是选中项
          u8g2.setCursor(2, 30+i*15); u8g2.print(stationNames[realIdx]); // 打印电台名
        }
      }
    }
  }

  // ---- 动画效果选择菜单 ---- // 动画效果选择菜单状态处理
  else if (menuState == MENU_SD_ANIMATION_SELECT) { // 如果是动画选择菜单状态
    u8g2.setCursor(0, 15); u8g2.print("选择动画效果"); // 打印标题
    uint32_t animTime = millis(); // 获取当前动画时间
    int previewCenterX = 113, previewCenterY = 12; // 预览动画中心坐标

    // 预览缩略图 // 预览缩略图绘制
    if (menuIndex == 0) { // 第一种动画：九宫格
      int gridOriginX = 107, gridOriginY = 6, gridSize = 2; // 参数
      int gridPos[8][2] = {{0,0},{1,0},{2,0},{2,1},{2,2},{1,2},{0,2},{0,1}}; // 网格位置
      int activeGrid = (animTime / 800) % 8; // 计算活动网格
      for (int i = 0; i < 8; i++) { // 遍历绘制
        int bx = gridOriginX + gridPos[i][0]*(gridSize+1), by = gridOriginY + gridPos[i][1]*(gridSize+1); // 计算位置
        if (i == activeGrid) u8g2.drawFrame(bx, by, gridSize+2, gridSize+2); // 绘制活动网格边框
        else u8g2.drawBox(bx+1, by+1, gridSize, gridSize); // 绘制普通网格实心
      }
    } else if (menuIndex == 1) { // 第二种动画：圆环
      int radius = 4, dotCount = 6, activeDot = (animTime/500)%dotCount; // 参数
      float scrollOffset = (animTime/500.0)*30.0*PI/180.0; // 滚动偏移角度
      for (int i = 0; i < dotCount; i++) { // 遍历绘制
        float angle = (i*360.0/dotCount)*PI/180.0 - PI/2.0 + scrollOffset; // 计算每个点角度
        int dotX = previewCenterX + radius*cos(angle), dotY = previewCenterY + radius*sin(angle); // 计算点坐标
        if (i == activeDot) u8g2.drawDisc(dotX, dotY, 2); else u8g2.drawPixel(dotX, dotY); // 绘制点
      }
    } else if (menuIndex == 2) { // 第三种动画：五分割圆环
      int radius = 5, segCount = 5, activeSeg = (animTime/1000)%segCount; // 参数
      float segAngle = 360.0/segCount; // 每段角度
      for (int i = 0; i < segCount; i++) { // 遍历绘制
        float midAngle = (i*segAngle+segAngle/2-90)*PI/180.0; // 计算每段中间角度
        int segX = previewCenterX+radius*cos(midAngle), segY = previewCenterY+radius*sin(midAngle); // 计算段坐标
        if (i == activeSeg) u8g2.drawFrame(segX-2, segY-2, 4, 4); else u8g2.drawBox(segX-1, segY-1, 2, 2); // 绘制段
      }
    } else if (menuIndex == 3) { // 第四种动画：三角形切换
      bool isHollow = (animTime/1000)%2 == 1; // 判断是否空心
      int cx = previewCenterX, cy = previewCenterY; // 中心坐标
      if (isHollow) { u8g2.drawLine(cx-3,cy-4,cx-3,cy+4); u8g2.drawLine(cx-3,cy-4,cx+3,cy); u8g2.drawLine(cx+3,cy,cx-3,cy+4); } // 绘制空心三角形
      else { u8g2.drawLine(cx-2,cy-3,cx-2,cy+3); u8g2.drawLine(cx-2,cy-3,cx+2,cy); u8g2.drawLine(cx+2,cy,cx-2,cy+3); } // 绘制实心三角形
    } else if (menuIndex == 4) { // 第五种动画：旋转圆弧
      int radius = 5, angle = (animTime*180/1000)%360; // 参数
      for (int i = 0; i < 20; i++) { // 绘制20个弧线
        float arcAngle = (angle+i*3)*PI/180.0; // 计算弧线角度
        for (int r = radius-1; r <= radius+1; r++) u8g2.drawPixel(previewCenterX+r*cos(arcAngle), previewCenterY+r*sin(arcAngle)); // 绘制像素点
      }
    } else if (menuIndex == 5) { // 第六种动画：音频指示器
      int bx0 = 107, by0 = 6, bw = 2, bmaxH = 8, bminH = 3, bgap = 2; // 参数
      for (int i = 0; i < 3; i++) { // 绘制3个条形
        float phase = (animTime/300.0)+(i*2.0); // 计算相位
        int h = bminH + (bmaxH-bminH)*(0.5+0.5*sin(phase)); // 计算条形高度
        u8g2.drawBox(bx0+i*(bw+bgap), by0+(bmaxH-h), bw, h); // 绘制条形
      }
    } else if (menuIndex == 6) { // 第七种动画：正弦频谱
      int ox = 108, oy = 10, ww = 16, wh = 6, pc = 8; // 参数
      for (int i = 0; i < pc; i++) { // 绘制8个频谱点
        float phase = (animTime/200.0)-(i*0.5); // 计算相位
        int y = oy+wh/2+wh/2*sin(phase), x = ox+(i*ww/pc); // 计算点坐标
        u8g2.drawPixel(x, y); // 绘制点
      }
    } else if (menuIndex == 7) { // 第八种动画：爱心跳动
      float scale = 0.3+0.15*sin(animTime*PI/500); int hs = 4*scale; // 参数
      u8g2.drawPixel(previewCenterX-hs/2, previewCenterY-hs/2); // 绘制爱心上半
      u8g2.drawPixel(previewCenterX+hs/2, previewCenterY-hs/2); // 绘制爱心右半
      u8g2.drawPixel(previewCenterX, previewCenterY+hs/2); // 绘制爱心下半
    } else if (menuIndex == 8) { // 第九种动画：方块滚动
      int ox = 107, oy = 8, bc = 4; // 参数
      float pos = fmod(animTime/200.0, (bc-1)*2.0); // 位置计算
      if (pos > bc-1) pos = (bc-1)*2.0-pos; // 往返计算
      int activeIdx = int(pos+0.5); // 活动索引
      for (int i = 0; i < bc; i++) { // 绘制4个方块
        if (i == activeIdx) u8g2.drawFrame(ox+i*5-1, oy-1, 5, 5); // 绘制活动方块边框
        else u8g2.drawBox(ox+i*5+1, oy+1, 3, 3); // 绘制普通方块实心
      }
    } else if (menuIndex == 9) { // 第十种动画：日月旋转（预览，大小比例改善）
      float angle = animTime*0.002; int br=4,sr=1,orb=br+sr+3;
      u8g2.drawDisc(previewCenterX, previewCenterY, br); // 大圆
      u8g2.drawDisc(previewCenterX+(int)(orb*cos(angle)), previewCenterY+(int)(orb*sin(angle)), sr); // 小圆轨道
    } else if (menuIndex == 10) { // 第十一种动画：斜线下雨（预览）
      { int rx=107,ry=6,rw=12,rh=12; int lens2[]={3,5,4,6,3}; int xoff2[]={0,2,5,8,10};
        for (int i=0;i<5;i++) { float off2=fmod(animTime*0.05f+i*2.5f,(float)(rh+lens2[i]));
          int sx2=rx+xoff2[i]-(int)(off2*0.5f),sy2=ry+(int)off2-lens2[i];
          int ex2=sx2+(int)(lens2[i]*0.5f),ey2=sy2+lens2[i];
          if (ey2>ry&&sy2<ry+rh&&ex2>rx&&sx2<rx+rw) u8g2.drawLine(sx2,sy2,ex2,ey2); } }
    } else if (menuIndex == 11) { // 第十二种动画：气泡上升（预览）
      { int bx2=107,by2=18,bh2=12; int szs2[]={1,2,1,2,1}; int xp2[]={1,3,6,9,11};
        bool hw2[]={false,true,false,true,false};
        for (int i=0;i<5;i++) { float of2=fmod(animTime*0.04f+i*2.3f,(float)(bh2+szs2[i]*2));
          int px2=bx2+xp2[i],py2=by2-(int)of2;
          if (py2+szs2[i]>=by2-bh2&&py2-szs2[i]<=by2) {
            if (hw2[i]) u8g2.drawCircle(px2,py2,szs2[i],U8G2_DRAW_ALL); else u8g2.drawDisc(px2,py2,szs2[i]); } } }
    }

    u8g2.setFont(u8g2_font_wqy12_t_gb2312); // 设置中文字体
    u8g2.drawHLine(0, 17, 128); // 绘制水平分割线
    const char* animItems[12] = {"1.九宫格","2.圆环","3.五分割圆环","4.三角形切换","5.旋转圆弧","6.音频指示器","7.正弦频谱","8.爱心跳动","9.方块滚动","10.日月旋转","11.斜线下雨","12.气泡上升"}; // 动画选项数组
    const int animCount = 12; // 动画选项数量
    int startIdx = constrain(menuIndex-1, 0, animCount-3); // 计算起始索引
    for (int i = 0; i < 3; i++) { // 遍历显示3个选项
      int idx = startIdx + i; // 计算当前选项索引
      if (idx < animCount) { // 如果索引有效
        if (idx == menuIndex) { // 如果是选中项
          u8g2.setDrawColor(1); u8g2.drawBox(0, 30+i*15-10, 128, 12); u8g2.setDrawColor(0); // 绘制反白背景
          u8g2.setCursor(2, 30+i*15); u8g2.print(animItems[idx]); // 打印选中项
          u8g2.setDrawColor(1); // 恢复正常颜色
        } else { // 如果不是选中项
          u8g2.setCursor(2, 30+i*15); u8g2.print(animItems[idx]); // 打印普通项
        }
      }
    }
  }

  // ---- 恢复出厂设置 ---- // 恢复出厂设置菜单状态处理
  else if (menuState == MENU_FACTORY_RESET) { // 如果是恢复出厂设置菜单状态
    u8g2.setCursor(0, 20); u8g2.print("正在初始化设置..."); // 打印初始化提示
    u8g2.setFont(u8g2_font_ncenB08_tr); // 设置英文字体
    u8g2.drawStr(0, 40, "请稍候，恢复默认电台"); // 打印等待提示
    static int dotCount = 0; // 点计数变量
    char dotStr[5] = {0}; // 点字符串缓冲
    for (int i = 0; i < dotCount % 4; i++) dotStr[i] = '.'; // 填充点字符
    u8g2.drawStr(0, 60, dotStr); // 打印点
    dotCount++; // 增加点计数
    u8g2.sendBuffer(); // 发送缓冲区
    return; // 返回
  }

  // ---- OTA检查界面 ---- // OTA检查界面状态处理
  else if (menuState == MENU_OTA_CHECK) { // 如果是OTA检查菜单状态
    u8g2.setCursor(0, 15); u8g2.print("OTA升级"); // 打印标题
    u8g2.drawHLine(0, 17, 128); // 绘制水平分割线
    u8g2.setCursor(0, 30); u8g2.print("当前版本:"); u8g2.setCursor(70, 30); u8g2.print(currentVersion); // 打印当前版本

    if (!otaCheckCompleted) { // 如果OTA检查未完成
      u8g2.setCursor(0, 45); // 设置光标位置
      if (WiFi.status() == WL_CONNECTED) { // 如果WiFi已连接
        u8g2.print("检查升级中..."); // 打印检查提示
        // 滚动方块动画 // 动画绘制
        otaAnimAngle = (otaAnimAngle + 10) % 360; // 更新动画角度
        int step = otaAnimAngle / 30; // 计算步骤
        int highlight = (step <= 5) ? step : (10 - step); // 计算高亮索引
        if (highlight < 0) highlight = 0; // 修正负值
        for (int i = 0; i < 6; i++) { // 绘制6个方块
          int baseX = 30 + i * (8+6); // 计算基础X坐标
          int size = (i == highlight) ? 12 : 8; // 高亮时使用大尺寸
          int offset = (12-8)/2; // 偏移量
          int drawX = baseX - (i==highlight ? offset : 0); // 计算绘制X坐标
          int drawY = 55 - (i==highlight ? offset : 0); // 计算绘制Y坐标
          if (i == highlight) u8g2.drawFrame(drawX, drawY, size, size); // 绘制高亮方框
          else u8g2.drawBox(drawX, drawY, size, size); // 绘制普通方块
        }
      } else { // 如果WiFi未连接
        u8g2.print("WiFi未连接!"); // 打印未连接提示
        u8g2.setCursor(0, 58); u8g2.print("请先连接WiFi"); // 打印提示
      }
    } else { // 如果OTA检查已完成
      u8g2.setCursor(0, 45); // 设置光标位置
      if (otaCheckResult == 1) { u8g2.print("发现v"); u8g2.print(serverVersionStr); } // 发现新版本
      else if (otaCheckResult == 2) u8g2.print("已是最新版本"); // 已是最新
      else u8g2.print("检查失败!"); // 检查失败

      u8g2.setCursor(0, 58); // 设置光标位置
      if (menuIndex == 0) { // 如果选择返回
        u8g2.setDrawColor(1); u8g2.drawBox(0, 50, 128, 12); u8g2.setDrawColor(0); // 绘制反白背景
        u8g2.print("< 返回"); u8g2.setDrawColor(1); // 打印返回选项
      } else { // 如果未选中
        u8g2.print("< 返回"); // 打印返回选项
      }
    }
  }

  // ---- OTA确认界面 ---- // OTA确认界面状态处理
  else if (menuState == MENU_OTA_CONFIRM) { // 如果是OTA确认菜单状态
    u8g2.setCursor(0, 15); u8g2.print("发现新版本"); // 打印标题
    u8g2.drawHLine(0, 17, 128); // 绘制水平分割线
    u8g2.setCursor(0, 32); u8g2.print("新版本:"); u8g2.setCursor(56, 32); u8g2.print(serverVersionStr); // 打印新版本号
    u8g2.setCursor(0, 48); u8g2.print("是否升级?"); // 打印确认提示

    if (menuIndex == 0) { // 如果选择升级
      u8g2.setDrawColor(1); u8g2.drawBox(0, 54, 60, 12); u8g2.setDrawColor(0); // 绘制反白背景
      u8g2.setCursor(4, 62); u8g2.print("1.升级"); // 打印升级选项
      u8g2.setDrawColor(1); u8g2.setCursor(70, 62); u8g2.print("2.返回"); // 打印返回选项
    } else { // 如果选择返回
      u8g2.setDrawColor(1); u8g2.drawBox(64, 54, 64, 12); u8g2.setDrawColor(0); // 绘制反白背景
      u8g2.setCursor(4, 62); u8g2.print("1.升级"); // 打印升级选项
      u8g2.setCursor(68, 62); u8g2.print("2.返回"); // 打印返回选项
      u8g2.setDrawColor(1); // 恢复正常颜色
    }
  }

  // ---- Navidrome播放器主界面 ---- // Navidrome播放器显示
  else if (menuState == MENU_NAVI_PLAYER) {
    // 若刚进入且处于NAVI_CONNECTING状态，执行实际启动（避免阻塞OLED任务）
    static bool naviStarted = false;
    if (naviState == NAVI_CONNECTING && !naviStarted) {
      naviStarted = true;
      // 渲染一帧"连接中"提示
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_wqy12_t_gb2312);
      u8g2.setCursor(0, 20); u8g2.print("Navidrome");
      u8g2.setCursor(0, 38); u8g2.print("连接服务器中...");
      u8g2.sendBuffer();
      // !! 不在此处调用 naviStart() !!
      // naviStart() 内部做 HTTPS/TLS 请求，需要 >20KB 栈空间
      // 在 OLED Task（仅 6KB 栈）中调用会触发 Stack canary watchpoint 崩溃
      // 改为设置标志位，由 loop()（主任务栈充足）负责执行
      // 同时立即将 naviState 改为 NAVI_LOADING，防止本任务循环中重复触发
      naviState = NAVI_LOADING;       // ← 防止重复触发的关键
      naviStartRequested = true;
      naviStarted = false;
      return;  // 本帧到此结束
    }

    // ── 标题栏 ──
    u8g2.setFont(u8g2_font_wqy12_t_gb2312);
    u8g2.setCursor(0, 12); u8g2.print("Navidrome");
    // ── WiFi/电池图标（右上） ──
    drawWifiIcon(90, 0);
    drawBatteryIcon(110, 3);
    u8g2.drawHLine(0, 14, 128);  // 分割线

    // ── 加载中状态：队列填充期间显示动画，避免误显示"未知错误" ──
    // 必须在错误判断之前处理，否则 naviSongCount==0 会误触发错误界面
    if (naviState == NAVI_LOADING || naviState == NAVI_CONNECTING) {
      static unsigned long _naviLoadAnimMs = 0;
      static int           _naviLoadAnimDot = 0;
      unsigned long now = millis();
      if (now - _naviLoadAnimMs > 400) {
        _naviLoadAnimMs = now;
        _naviLoadAnimDot = (_naviLoadAnimDot + 1) % 4;
      }
      // 旋转指示器（右上角）
      const int cx = 120, cy = 8, r = 4;
      int angle = (_naviLoadAnimDot * 90);
      for (int i = 0; i < 4; i++) {
        float a = (angle + i * 90) * PI / 180.0f;
        int px = cx + (int)(r * cos(a));
        int py = cy + (int)(r * sin(a));
        if (i == 0) u8g2.drawDisc(px, py, 2); else u8g2.drawPixel(px, py);
      }
      // 主体提示文字
      u8g2.setCursor(0, 32); u8g2.print("正在加载歌曲");
      // 点点动画
      char dots[5] = "   ";
      for (int i = 0; i < _naviLoadAnimDot; i++) dots[i] = '.';
      u8g2.print(dots);
      // 队列进度（_naviQCount 在 navidrome.h 里是 static，此处直接引用）
      char qStr[24];
      snprintf(qStr, sizeof(qStr), "队列: %d / 5", _naviQCount);
      u8g2.setCursor(0, 48); u8g2.print(qStr);
      u8g2.sendBuffer();
      return;
    }

    if (naviState == NAVI_ERROR || naviSongCount == 0) {
      // ── 错误/未配置状态：显示提示和设备IP，引导用户去Web页面配置 ──
      String msg = naviStatusMsg;
      if (msg.length() == 0) msg = "连接失败，请重试";

      if (!naviConfigValid) {
        // 未配置：显示引导信息 + 设备IP
        u8g2.setCursor(0, 28); u8g2.print("请在浏览器访问:");
        // 优先显示局域网IP，其次显示AP IP
        char ipStr[20];
        if (WiFi.status() == WL_CONNECTED) {
          IPAddress ip = WiFi.localIP();
          snprintf(ipStr, sizeof(ipStr), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
        } else {
          IPAddress ap = WiFi.softAPIP();
          snprintf(ipStr, sizeof(ipStr), "%d.%d.%d.%d", ap[0], ap[1], ap[2], ap[3]);
        }
        // IP地址用较大字体突出显示
        u8g2.setFont(u8g2_font_7x14_tf);
        int ipW = u8g2.getStrWidth(ipStr);
        u8g2.setCursor((128 - ipW) / 2, 46);  // 居中显示
        u8g2.print(ipStr);
        u8g2.setFont(u8g2_font_wqy12_t_gb2312);
        u8g2.setCursor(0, 62); u8g2.print("填写Navidrome账号");
      } else {
        // 已配置但连接失败：显示错误原因和操作选项（3个按钮）
        u8g2.setCursor(0, 28); u8g2.print("错误:");
        u8g2.setCursor(0, 44); u8g2.print(msg.substring(0, 20).c_str());
        u8g2.drawHLine(0, 50, 128);
        if (menuIndex == 0) {
          u8g2.setDrawColor(1);
          u8g2.drawBox(0, 52, 40, 11);
          u8g2.setDrawColor(0);
          u8g2.setCursor(4, 61); u8g2.print("重试");
          u8g2.setDrawColor(1);
          u8g2.setCursor(44, 61); u8g2.print("配置");
          u8g2.setCursor(84, 61); u8g2.print("返回");
        } else if (menuIndex == 1) {
          u8g2.setCursor(4, 61); u8g2.print("重试");
          u8g2.setDrawColor(1);
          u8g2.drawBox(40, 52, 40, 11);
          u8g2.setDrawColor(0);
          u8g2.setCursor(44, 61); u8g2.print("配置");
          u8g2.setDrawColor(1);
          u8g2.setCursor(84, 61); u8g2.print("返回");
        } else {
          u8g2.setCursor(4, 61); u8g2.print("重试");
          u8g2.setCursor(44, 61); u8g2.print("配置");
          u8g2.setDrawColor(1);
          u8g2.drawBox(80, 52, 46, 11);
          u8g2.setDrawColor(0);
          u8g2.setCursor(84, 61); u8g2.print("返回");
          u8g2.setDrawColor(1);
        }
      }
    } else {
      // ── 正常播放状态 ──
      NaviSong& song = naviSongList[naviCurrentIdx];

      // 歌曲标题（第1行，居中可滚动）
      displayScrollableTextCentered(song.title.c_str(), 27);

      // 歌手名（第2行，居中显示）
      u8g2.setFont(u8g2_font_wqy12_t_gb2312);
      String artistShort = song.artist.substring(0, 18);
      int artistWidth = u8g2.getUTF8Width(artistShort.c_str());
      int artistX = (128 - artistWidth) / 2;
      u8g2.setCursor(artistX, 41);
      u8g2.print(artistShort.c_str());

      // ── 播放进度条（左侧，从0开始，宽度98像素） ──
      int barY = 46;
      u8g2.drawFrame(0, barY, 98, 6);  // 进度条外框
      if (naviCurrentDuration > 0) {
        int fillW = (int)((float)naviCurrentPlaySec / naviCurrentDuration * 96);
        fillW = constrain(fillW, 0, 96);
        if (fillW > 0) u8g2.drawBox(1, barY + 1, fillW, 4);  // 从左向右填充
      }

      // ── 动画效果显示（右侧位置） ──
      if (naviState != NAVI_PAUSED) {
        uint32_t at = millis();
        if (sdPlayerAnimType == 0) { // 九宫格
          int gox = 108, goy = 44, gs = 3, gbs = 5, gg = 1;
          int gp[8][2] = {{0,0},{1,0},{2,0},{2,1},{2,2},{1,2},{0,2},{0,1}};
          int ag = (at/800)%8;
          for (int i = 0; i < 8; i++) {
            int bx = gox+gp[i][0]*(gs+gg), by = goy+gp[i][1]*(gs+gg);
            if (i==ag) { int off=(gbs-gs)/2; u8g2.drawFrame(bx-off,by-off,gbs,gbs); } else u8g2.drawBox(bx,by,gs,gs);
          }
        } else if (sdPlayerAnimType == 1) { // 圆环
          int cx=108,cy=48,r=4,dc=6,ad=(at/500)%dc; float so=(at/500.0)*30.0*PI/180.0;
          for (int i=0;i<dc;i++) { float a=(i*360.0/dc)*PI/180.0-PI/2.0+so; int dx=cx+r*cos(a),dy=cy+r*sin(a); if(i==ad) u8g2.drawDisc(dx,dy,2); else u8g2.drawPixel(dx,dy); }
        } else if (sdPlayerAnimType == 2) { // 五分割圆环
          int cx=108,cy=48,r=5,sc=5,as2=(at/1000)%sc; float sa=360.0/sc;
          for (int i=0;i<sc;i++) { float ma=(i*sa+sa/2-90)*PI/180.0; int sx=cx+r*cos(ma),sy=cy+r*sin(ma); if(i==as2) u8g2.drawFrame(sx-2,sy-2,4,4); else u8g2.drawBox(sx-1,sy-1,2,2); }
        } else if (sdPlayerAnimType == 3) { // 三角形切换
          int cx=108,cy=48; bool iH=(at/1000)%2==1;
          if(iH){u8g2.drawLine(cx-3,cy-4,cx-3,cy+4);u8g2.drawLine(cx-3,cy-4,cx+3,cy);u8g2.drawLine(cx+3,cy,cx-3,cy+4);}
          else{u8g2.drawLine(cx-2,cy-3,cx-2,cy+3);u8g2.drawLine(cx-2,cy-3,cx+2,cy);u8g2.drawLine(cx+2,cy,cx-2,cy+3);}
        } else if (sdPlayerAnimType == 4) { // 旋转圆弧
          int cx=108,cy=48,r=5; int angle=(at*180/1000)%360;
          for (int i=0;i<20;i++) { float aa=(angle+i*3)*PI/180.0; for (int rr=r-1;rr<=r+1;rr++) u8g2.drawPixel(cx+rr*cos(aa),cy+rr*sin(aa)); }
        } else if (sdPlayerAnimType == 5) { // 音频指示器
          int bx0=108,by0=44,bw=2,bmx=7,bmn=3,bgap=2;
          for (int i=0;i<3;i++) { float ph=(at/300.0)+(i*2.0); int h=bmn+(bmx-bmn)*(0.5+0.5*sin(ph)); u8g2.drawBox(bx0+i*(bw+bgap),by0+(bmx-h),bw,h); }
        } else if (sdPlayerAnimType == 6) { // 正弦频谱
          int ox=108,oy=48,ww=16,wh=6,pc=8;
          for (int i=0;i<pc;i++) { float ph=(at/200.0)-(i*0.5); float amp=sin(ph)*0.7+sin(ph*2.3)*0.3; int y=oy+wh/2+(wh/2-1)*amp, x=ox+(i*ww/pc); u8g2.drawPixel(x,y); }
        } else if (sdPlayerAnimType == 7) { // 爱心跳动
          int hx=108,hy=48; float sf=0.5+0.5*sin(at*PI/500.0); int hs=2+int(3*sf);
          for (int dy=-hs;dy<=hs;dy++) for (int dx=-hs;dx<=hs;dx++) { int cx1=dx+hs/2,cx2=dx-hs/2,cy2=dy; float d1=sqrt(float(cx1*cx1)+float((cy2+hs/3)*(cy2+hs/3))); float d2=sqrt(float(cx2*cx2)+float((cy2+hs/3)*(cy2+hs/3))); bool inTri=(dy>0)&&(abs(dx)<(hs-dy*hs/(hs+1))); if (d1<hs||d2<hs||inTri) u8g2.drawPixel(hx+dx,hy+dy); }
        } else if (sdPlayerAnimType == 8) { // 方块滚动
          int ox=108,oy=46,bc=3,bgap=5; float pos=fmod(at/200.0,(bc-1)*2.0); if (pos>bc-1) pos=(bc-1)*2.0-pos; int ai=int(pos+0.5);
          for (int i=0;i<bc;i++) { int bx=ox+i*bgap,by=oy; if(i==ai) u8g2.drawFrame(bx-1,by-1,5,5); else u8g2.drawBox(bx+1,by+1,3,3); }
        } else if (sdPlayerAnimType == 9) { // 日月旋转：大圆居中，小圆宽轨道绕行
          int cx=116,cy=48,br=5,sr=2,orb=br+sr+5; float angle=at*0.002;
          u8g2.drawDisc(cx,cy,br);
          u8g2.drawDisc(cx+(int)(orb*cos(angle)),cy+(int)(orb*sin(angle)),sr);
        } else if (sdPlayerAnimType == 10) { // 斜线下雨：多条不同斜线从左上向右下落
          { int rx=108,ry=38,rw=20,rh=20;
            int lens[]={5,8,6,9,7,5,8}; int xoff[]={0,3,6,9,12,15,2};
            for (int i=0;i<7;i++) { float spd=0.04f+i*0.008f;
              float off=fmod(at*spd+i*3.1f,(float)(rh+lens[i]));
              int sx=rx+xoff[i]-(int)(off*0.5f),sy=ry+(int)off-lens[i];
              int ex=sx+(int)(lens[i]*0.5f),ey=sy+lens[i];
              if (ey>ry&&sy<ry+rh&&ex>rx&&sx<rx+rw) u8g2.drawLine(sx,sy,ex,ey); } }
        } else if (sdPlayerAnimType == 11) { // 气泡上升：实心空心大小混合气泡上浮
          { int bx=108,by=58,bw=20,bh=20;
            int szs[]={1,3,2,4,1,2,3}; int xpos[]={1,4,8,12,16,10,3};
            bool hollow[]={false,true,false,true,false,true,false};
            for (int i=0;i<7;i++) { float spd=0.03f+i*0.007f;
              float off=fmod(at*spd+i*2.8f,(float)(bh+szs[i]*2));
              int px=bx+xpos[i],py=by-(int)off;
              if (py+szs[i]>=by-bh&&py-szs[i]<=by) {
                if (hollow[i]) u8g2.drawCircle(px,py,szs[i],U8G2_DRAW_ALL);
                else u8g2.drawDisc(px,py,szs[i]); } } }
        }
      } else {
        // 暂停显示两竖线（右侧）
        u8g2.drawBox(102, 48, 2, 8);
        u8g2.drawBox(106, 48, 2, 8);
      }

      // ── 时间显示 ──
      char timeStr[16];
      unsigned long cur = naviCurrentPlaySec;
      int dur = naviCurrentDuration;
      snprintf(timeStr, sizeof(timeStr), "%02lu:%02lu/%02d:%02d",
               cur / 60, cur % 60, dur / 60, dur % 60);
      u8g2.setFont(u8g2_font_5x7_tf);  // 小字体
      u8g2.setCursor(0, 62);
      u8g2.print(timeStr);


    }
  }

  // ---- Navidrome动画选择菜单 ---- // 与SD动画选择共用预览逻辑，返回Navidrome播放器
  else if (menuState == MENU_NAVI_ANIMATION_SELECT) {
    u8g2.setFont(u8g2_font_wqy12_t_gb2312);
    u8g2.setCursor(0, 15); u8g2.print("选择动画效果"); // 打印标题
    uint32_t animTime = millis(); // 获取当前动画时间
    int previewCenterX = 113, previewCenterY = 12; // 预览动画中心坐标

    // 预览缩略图（与SD动画选择完全相同的预览逻辑）
    if (menuIndex == 0) {
      int gridOriginX = 107, gridOriginY = 6, gridSize = 2;
      int gridPos[8][2] = {{0,0},{1,0},{2,0},{2,1},{2,2},{1,2},{0,2},{0,1}};
      int activeGrid = (animTime / 800) % 8;
      for (int i = 0; i < 8; i++) {
        int bx = gridOriginX + gridPos[i][0]*(gridSize+1), by = gridOriginY + gridPos[i][1]*(gridSize+1);
        if (i == activeGrid) u8g2.drawFrame(bx, by, gridSize+2, gridSize+2);
        else u8g2.drawBox(bx+1, by+1, gridSize, gridSize);
      }
    } else if (menuIndex == 1) {
      int radius = 4, dotCount = 6, activeDot = (animTime/500)%dotCount;
      float scrollOffset = (animTime/500.0)*30.0*PI/180.0;
      for (int i = 0; i < dotCount; i++) {
        float angle = (i*360.0/dotCount)*PI/180.0 - PI/2.0 + scrollOffset;
        int dotX = previewCenterX + radius*cos(angle), dotY = previewCenterY + radius*sin(angle);
        if (i == activeDot) u8g2.drawDisc(dotX, dotY, 2); else u8g2.drawPixel(dotX, dotY);
      }
    } else if (menuIndex == 2) {
      int radius = 5, segCount = 5, activeSeg = (animTime/1000)%segCount;
      float segAngle = 360.0/segCount;
      for (int i = 0; i < segCount; i++) {
        float midAngle = (i*segAngle+segAngle/2-90)*PI/180.0;
        int segX = previewCenterX+radius*cos(midAngle), segY = previewCenterY+radius*sin(midAngle);
        if (i == activeSeg) u8g2.drawFrame(segX-2, segY-2, 4, 4); else u8g2.drawBox(segX-1, segY-1, 2, 2);
      }
    } else if (menuIndex == 3) {
      bool isHollow = (animTime/1000)%2 == 1;
      int cx = previewCenterX, cy = previewCenterY;
      if (isHollow) { u8g2.drawLine(cx-3,cy-4,cx-3,cy+4); u8g2.drawLine(cx-3,cy-4,cx+3,cy); u8g2.drawLine(cx+3,cy,cx-3,cy+4); }
      else { u8g2.drawLine(cx-2,cy-3,cx-2,cy+3); u8g2.drawLine(cx-2,cy-3,cx+2,cy); u8g2.drawLine(cx+2,cy,cx-2,cy+3); }
    } else if (menuIndex == 4) {
      int radius = 5, angle = (animTime*180/1000)%360;
      for (int i = 0; i < 20; i++) {
        float arcAngle = (angle+i*3)*PI/180.0;
        for (int r = radius-1; r <= radius+1; r++) u8g2.drawPixel(previewCenterX+r*cos(arcAngle), previewCenterY+r*sin(arcAngle));
      }
    } else if (menuIndex == 5) {
      int bx0 = 107, by0 = 6, bw = 2, bmaxH = 8, bminH = 3, bgap = 2;
      for (int i = 0; i < 3; i++) {
        float phase = (animTime/300.0)+(i*2.0);
        int h = bminH + (bmaxH-bminH)*(0.5+0.5*sin(phase));
        u8g2.drawBox(bx0+i*(bw+bgap), by0+(bmaxH-h), bw, h);
      }
    } else if (menuIndex == 6) {
      int ox = 108, oy = 10, ww = 16, wh = 6, pc = 8;
      for (int i = 0; i < pc; i++) {
        float phase = (animTime/200.0)-(i*0.5);
        int y = oy+wh/2+wh/2*sin(phase), x = ox+(i*ww/pc);
        u8g2.drawPixel(x, y);
      }
    } else if (menuIndex == 7) {
      float scale = 0.3+0.15*sin(animTime*PI/500); int hs = 4*scale;
      u8g2.drawPixel(previewCenterX-hs/2, previewCenterY-hs/2);
      u8g2.drawPixel(previewCenterX+hs/2, previewCenterY-hs/2);
      u8g2.drawPixel(previewCenterX, previewCenterY+hs/2);
    } else if (menuIndex == 8) {
      int ox = 107, oy = 8, bc = 4;
      float pos = fmod(animTime/200.0, (bc-1)*2.0);
      if (pos > bc-1) pos = (bc-1)*2.0-pos;
      int activeIdx = int(pos+0.5);
      for (int i = 0; i < bc; i++) {
        if (i == activeIdx) u8g2.drawFrame(ox+i*5-1, oy-1, 5, 5);
        else u8g2.drawBox(ox+i*5+1, oy+1, 3, 3);
      }
    } else if (menuIndex == 9) {
      float angle = animTime*0.002; int br=4,sr=1,orb=br+sr+3;
      u8g2.drawDisc(previewCenterX, previewCenterY, br);
      u8g2.drawDisc(previewCenterX+(int)(orb*cos(angle)), previewCenterY+(int)(orb*sin(angle)), sr);
    } else if (menuIndex == 10) {
      { int rx=107,ry=6,rw=12,rh=12; int lens2[]={3,5,4,6,3}; int xoff2[]={0,2,5,8,10};
        for (int i=0;i<5;i++) { float off2=fmod(animTime*0.05f+i*2.5f,(float)(rh+lens2[i]));
          int sx2=rx+xoff2[i]-(int)(off2*0.5f),sy2=ry+(int)off2-lens2[i];
          int ex2=sx2+(int)(lens2[i]*0.5f),ey2=sy2+lens2[i];
          if (ey2>ry&&sy2<ry+rh&&ex2>rx&&sx2<rx+rw) u8g2.drawLine(sx2,sy2,ex2,ey2); } }
    } else if (menuIndex == 11) {
      { int bx2=107,by2=18,bh2=12; int szs2[]={1,2,1,2,1}; int xp2[]={1,3,6,9,11};
        bool hw2[]={false,true,false,true,false};
        for (int i=0;i<5;i++) { float of2=fmod(animTime*0.04f+i*2.3f,(float)(bh2+szs2[i]*2));
          int px2=bx2+xp2[i],py2=by2-(int)of2;
          if (py2+szs2[i]>=by2-bh2&&py2-szs2[i]<=by2) {
            if (hw2[i]) u8g2.drawCircle(px2,py2,szs2[i],U8G2_DRAW_ALL); else u8g2.drawDisc(px2,py2,szs2[i]); } } }
    }

    u8g2.drawHLine(0, 17, 128); // 绘制水平分割线
    const char* animItems[12] = {"1.九宫格","2.圆环","3.五分割圆环","4.三角形切换","5.旋转圆弧","6.音频指示器","7.正弦频谱","8.爱心跳动","9.方块滚动","10.日月旋转","11.斜线下雨","12.气泡上升"};
    const int animCount = 12;
    int startIdx = constrain(menuIndex-1, 0, animCount-3);
    for (int i = 0; i < 3; i++) {
      int idx = startIdx + i;
      if (idx < animCount) {
        if (idx == menuIndex) {
          u8g2.setDrawColor(1); u8g2.drawBox(0, 30+i*15-10, 128, 12); u8g2.setDrawColor(0);
          u8g2.setCursor(2, 30+i*15); u8g2.print(animItems[idx]);
          u8g2.setDrawColor(1);
        } else {
          u8g2.setCursor(2, 30+i*15); u8g2.print(animItems[idx]);
        }
      }
    }
  }

  // ---- Navidrome控制菜单 ---- // 9选项控制菜单
  else if (menuState == MENU_NAVI_CONTROL) {
    u8g2.setFont(u8g2_font_wqy12_t_gb2312);
    u8g2.setCursor(0, 12); u8g2.print("Navidrome控制");
    u8g2.drawHLine(0, 14, 128);

    const char* naviMenuItems[9] = {
      "1.下一首", "2.上一首",
      (naviState == NAVI_PAUSED) ? "3.继续播放" : "3.暂停",
      "4.刷新歌单",
      "5.音乐闹钟", "6.PONG游戏", "7.内存分配",
      "8.切换动画", "9.退出"
    };

    // 计算起始显示项：当前选中项之前的项目不显示
    int startItem = 0;
    if (menuIndex >= 4) startItem = menuIndex - 3;  // 只显示4个选项

    // 显示4行菜单项：y=25, 37, 49, 61
    for (int i = 0; i < 4; i++) {
      int itemIdx = startItem + i;
      if (itemIdx >= 9) break;
      
      int yPos = 25 + i * 12;
      
      if (itemIdx == menuIndex) {
        u8g2.drawBox(0, yPos - 10, 128, 12);
        u8g2.setDrawColor(0);
      }
      u8g2.setCursor(4, yPos);
      u8g2.print(naviMenuItems[itemIdx]);
      u8g2.setDrawColor(1);
    }
  }


  // ---- OTA升级进度界面 ---- // OTA升级进度界面状态处理
  else if (menuState == MENU_OTA_UPGRADING) { // 如果是OTA升级菜单状态
    u8g2.setCursor(0, 15); u8g2.print("升级中..."); // 打印标题
    u8g2.drawHLine(0, 17, 128); // 绘制水平分割线
    u8g2.setCursor(0, 32); u8g2.print("请稍候，请不要关断电源"); // 打印提示
    u8g2.drawFrame(10, 45, 108, 12); // 绘制进度条边框
    int fillW = map(otaProgress, 0, 100, 0, 106); // 映射进度到宽度
    if (fillW > 0) u8g2.drawBox(11, 46, fillW, 10); // 绘制进度条填充
    char percentStr[10]; snprintf(percentStr, sizeof(percentStr), "%d%%", otaProgress); // 格式化百分比字符串
    u8g2.setCursor(50, 62); u8g2.print(percentStr); // 打印百分比
  }

  u8g2.sendBuffer(); // 发送缓冲区到OLED
}

//============================================================================= // 辅助函数分隔符
// 辅助函数：绘制6方块左右往返动画（连接/退出等待状态通用） // 函数说明
//============================================================================= // 辅助函数分隔符
static void drawBouncingBlocks(int centerX, int centerY, int angle) { // 绘制弹跳方块动画函数
  int step = angle / 30; // 计算步骤
  int highlight = (step <= 5) ? step : (10 - step); // 计算高亮索引
  if (highlight < 0) highlight = 0; // 修正负值
  const int normalSize = 10, bigSize = 14, gap = 6; // 尺寸常量
  for (int i = 0; i < 6; i++) { // 循环绘制6个方块
    int baseX = centerX - 3*(normalSize+gap) + i*(normalSize+gap); // 计算每个方块的X坐标
    int size = (i == highlight) ? bigSize : normalSize; // 高亮时使用大尺寸
    int offset = (bigSize - normalSize) / 2; // 偏移量
    int drawX = baseX - (i==highlight ? offset : 0); // 计算绘制X坐标
    int drawY = centerY - (i==highlight ? offset : 0); // 计算绘制Y坐标
    if (i == highlight) u8g2.drawFrame(drawX, drawY, size, size); // 绘制高亮方框
    else u8g2.drawBox(drawX, drawY, size, size); // 绘制普通方块
  }
}

//============================================================================= // oledTask函数分隔符
// oledTask() - FreeRTOS任务，运行在 Core 1 // 函数说明
//============================================================================= // oledTask函数分隔符
void oledTask(void* pvParam) { // OLED任务主函数
  u8g2.begin(); // 初始化OLED
  u8g2.enableUTF8Print(); // 启用UTF8打印

  // PONG游戏常量 // 游戏常量说明
  const int PADDLE_HEIGHT = 8; // 挡板高度
  const int PADDLE_WIDTH  = 2; // 挡板宽度
  const int BALL_SIZE     = 2; // 球大小

  for (;;) { // 无限循环
    //------------------------------------------------------------------ // 开机动画分隔符
    // 1. 开机动画 // 说明
    //------------------------------------------------------------------ // 开机动画分隔符
    if (bootScreenActive && !skipBootAnimation) { // 如果开机画面激活且未跳过动画
      unsigned long elapsed = millis() - bootScreenStartTime; // 计算已过时间
      int frame = (elapsed / BOOT_ANIMATION_DURATION) % BOOT_ANIMATION_FRAMES; // 计算当前帧
      drawBootAnimationFrame(u8g2, frame, elapsed); // 绘制开机动画帧
      if (elapsed > 5000) { // 如果已超过5秒
        bootScreenActive  = false; // 关闭开机画面
        audioSystemReady  = true; // 标记音频系统就绪
        menuState         = MENU_PLAYER_SELECT; // 切换到播放器选择菜单
        menuIndex         = 0; // 重置菜单索引
        menuStartTime     = millis(); // 记录菜单开始时间
      }
      vTaskDelay(50); // 延迟50毫秒
      continue; // 继续循环
    }

    //------------------------------------------------------------------ // 音量显示分隔符
    // 2. 音量显示（所有菜单下都可触发） // 说明
    //------------------------------------------------------------------ // 音量显示分隔符
    if (showingVolume) { // 如果显示音量
      u8g2.clearBuffer(); // 清除缓冲区
      u8g2.setFont(u8g2_font_wqy12_t_gb2312); // 设置中文字体
      char volStr[30]; snprintf(volStr, sizeof(volStr), "音量: %d/21", encoderPos); // 格式化音量字符串
      u8g2.setCursor(60, 27); u8g2.print(volStr); // 打印音量

      int barX = 30, barY = 22, barHeight = 35, barWidth = 12; // 音量条参数
      u8g2.drawDisc(barX, barY, barWidth/2); // 绘制音量条上圆弧
      u8g2.drawDisc(barX, barY+barHeight, barWidth/2); // 绘制音量条下圆弧
      u8g2.drawBox(barX-barWidth/2, barY, barWidth, barHeight); // 绘制音量条主体
      u8g2.setDrawColor(0); // 设置反色
      int innerHeight = map(encoderPos, 0, 21, 0, barHeight); // 映射音量到填充高度
      if (innerHeight > 0) { // 如果有填充
        int innerY = barY + barHeight - innerHeight; // 计算填充Y坐标
        u8g2.drawDisc(barX, barY+barHeight, barWidth/2-1); // 绘制填充下圆弧
        u8g2.drawBox(barX-barWidth/2+1, innerY, barWidth-2, innerHeight); // 绘制填充主体
      }
      u8g2.setDrawColor(1); // 恢复正常颜色
      const char* level = (encoderPos < 7) ? "轻柔" : (encoderPos < 14) ? "刚好" : "聋啦"; // 根据音量选择描述
      u8g2.setCursor(60, 50); u8g2.print(level); // 打印音量描述
      u8g2.sendBuffer(); // 发送缓冲区
      vTaskDelay(200); // 延迟200毫秒
      continue; // 继续循环
    }

    //------------------------------------------------------------------ // OTA版本检查分隔符
    // 3. OTA版本检查（后台触发checkOTAVersion） // 说明
    //------------------------------------------------------------------ // OTA版本检查分隔符
    if (menuState == MENU_OTA_CHECK && !otaCheckCompleted) { // 如果在OTA检查菜单且检查未完成
      if (millis() - otaCheckStartTime > 500) checkOTAVersion(); // 超过500毫秒则检查OTA版本
      displayMenu(); // 显示菜单
      vTaskDelay(100); // 延迟100毫秒
      continue; // 继续循环
    }

    //------------------------------------------------------------------ // SD播放器主界面分隔符
    // 4. SD播放器主界面 // 说明
    //------------------------------------------------------------------ // SD播放器主界面分隔符
    if (menuState == MENU_SD_PLAYER) { // 如果在SD卡播放器菜单状态
      // 播放三角形动画变量 // 动画变量说明
      static int playIconOffset = 0, playIconDir = 1; // 偏移量和方向
      static unsigned long lastPlayIconAnim = 0; // 上次动画时间
      const int playIconOffsetMax = 5, playIconAnimInterval = 80; // 最大偏移和动画间隔
      if (!sdPaused) { // 如果未暂停
        unsigned long now = millis(); // 获取当前时间
        if (now - lastPlayIconAnim > playIconAnimInterval) { // 如果超过动画间隔
          playIconOffset += playIconDir; // 更新偏移
          if (playIconOffset >= playIconOffsetMax) { playIconOffset = playIconOffsetMax; playIconDir = -1; } // 达到最大则反向
          else if (playIconOffset <= 0) { playIconOffset = 0; playIconDir = 1; } // 达到最小则正向
          lastPlayIconAnim = now; // 更新上次动画时间
        }
      } else { playIconOffset = 0; playIconDir = 1; } // 暂停时重置

      u8g2.clearBuffer(); // 清除缓冲区
      u8g2.setFont(u8g2_font_wqy12_t_gb2312); // 设置中文字体
      u8g2.setCursor(5, 15); u8g2.print("SD卡播放器"); // 打印标题
      u8g2.setFont(u8g2_font_ncenB08_tr); // 设置英文字体
      u8g2.drawHLine(0, 17, 128); // 绘制水平分割线

      // 音频质量图标（右上角） // 音频质量图标绘制
      { // 图标代码块
        int iconX = 120, iconY = 7; // 图标坐标
        if (audioQualityStatus == "Buffer Low") { // 如果缓冲区低
          u8g2.drawBox(iconX, iconY+6, 2, 2); // 绘制最低级图标
        } else if (audioQualityStatus == "Buffer Full") { // 如果缓冲区满
          u8g2.drawBox(iconX, iconY+6, 2, 2); u8g2.drawBox(iconX+3, iconY+3, 2, 5); // 绘制中级图标
        } else { // 如果缓冲区正常
          u8g2.drawBox(iconX, iconY+6, 2, 2); u8g2.drawBox(iconX+3, iconY+3, 2, 5); u8g2.drawBox(iconX+6, iconY, 2, 8); // 绘制高级图标
        }
      }
      drawBatteryIcon(100, 4); // 绘制电池图标

      if (totalTracks == 0) { // 如果没有曲目
        u8g2.setFont(u8g2_font_wqy12_t_gb2312); // 设置中文字体
        u8g2.setCursor(10, 35); u8g2.print("未找到MP3文件"); // 打印提示
        u8g2.setCursor(10, 50); u8g2.print("按下按钮重新扫描"); // 打印提示
      } else { // 如果有曲目
        // 曲目信息 // 曲目信息绘制
        u8g2.setFont(u8g2_font_wqy12_t_gb2312); // 设置中文字体
        u8g2.setCursor(0, 30); u8g2.print("曲目："); // 打印曲目标题
        u8g2.setFont(u8g2_font_ncenB08_tr); // 设置英文字体
        u8g2.setCursor(35, 30); u8g2.print(String(currentTrackIndex+1)); // 打印当前曲目号
        u8g2.setCursor(55, 30); u8g2.print("/"); // 打印分隔符
        u8g2.setCursor(60, 30); u8g2.print(String(totalTracks)); // 打印总曲目数

        // 歌曲名滚动 // 歌曲名滚动绘制
        String displayName = getDisplayTrackName(currentTrackName); // 获取显示歌曲名
        displayScrollableText(displayName.c_str(), 45); // 显示滚动文本

        // 时间显示 // 时间显示绘制
        u8g2.setFont(u8g2_font_ncenB08_tr); // 设置英文字体
        if (sdPlaying && trackStartTime > 0) { // 如果正在播放
          unsigned long elapsedSec = 0; // 已播放秒数
          if (!sdPaused) elapsedSec = (millis() - trackStartTime) / 1000; // 计算已播放秒数

          String currentTimeStr = formatTime(elapsedSec); // 格式化当前时间
          String totalTimeStr; // 总时间字符串
          if (timeInfoAvailable && totalPlayTime > 0) totalTimeStr = formatTime(totalPlayTime); // 格式化总时间
          else if (totalPlayTime > 0) totalTimeStr = "~" + formatTime(totalPlayTime); // 估算总时间
          else totalTimeStr = "--:--"; // 无法获取总时间

          int ctw = u8g2.getStrWidth(currentTimeStr.c_str()); // 获取当前时间宽度
          int sw  = u8g2.getStrWidth("/"); // 获取分隔符宽度
          int ttw = u8g2.getStrWidth(totalTimeStr.c_str()); // 获取总时间宽度
          u8g2.drawStr(0, 60, currentTimeStr.c_str()); // 绘制当前时间
          u8g2.drawStr(ctw, 60, "/"); // 绘制分隔符
          u8g2.setDrawColor(1); u8g2.drawBox(ctw+sw, 52, ttw+2, 10); // 绘制背景块
          u8g2.setDrawColor(0); u8g2.drawStr(ctw+sw+1, 60, totalTimeStr.c_str()); // 绘制总时间
          u8g2.setDrawColor(1); // 恢复正常颜色
        } else { // 如果未在播放
          u8g2.drawStr(0, 60, sdPaused ? "PAUSED" : "PLAYING"); // 绘制播放状态
        }

        // 音量条 // 音量条绘制
        u8g2.drawFrame(70, 54, 24, 4); // 绘制音量条边框
        int volPos = map(encoderPos, 0, 21, 0, 23); // 映射音量到位置
        if (volPos > 0) u8g2.drawBox(70, 54, volPos, 4); // 绘制音量填充
        int dX = 70 + volPos; // 计算音量调节图标X坐标
        u8g2.drawLine(dX, 51, dX-3, 54); u8g2.drawLine(dX-3, 54, dX, 57); // 绘制左箭头
        u8g2.drawLine(dX, 57, dX+3, 54); u8g2.drawLine(dX+3, 54, dX, 51); // 绘制右箭头

        // 播放状态/动画图标 // 播放状态/动画图标绘制
        if (sdPaused) { // 如果暂停
          u8g2.drawBox(107, 49, 3, 12); u8g2.drawBox(113, 49, 3, 12); // 绘制暂停图标
        } else { // 如果正在播放
          // 12种动画类型（与原代码完全一致） // 12种动画类型说明
          uint32_t at = millis(); // 获取动画时间
          if (sdPlayerAnimType == 0) { // 第一种动画：九宫格
            int gox = 107, goy = 49, gs = 3, gbs = 5, gg = 1; // 参数
            int gp[8][2] = {{0,0},{1,0},{2,0},{2,1},{2,2},{1,2},{0,2},{0,1}}; // 网格位置
            int ag = (at/800)%8; // 计算活动网格
            for (int i = 0; i < 8; i++) { // 遍历绘制
              int bx = gox+gp[i][0]*(gs+gg), by = goy+gp[i][1]*(gs+gg); // 计算位置
              if (i==ag) { int off=(gbs-gs)/2; u8g2.drawFrame(bx-off,by-off,gbs,gbs); } else u8g2.drawBox(bx,by,gs,gs); // 绘制
            }
          } else if (sdPlayerAnimType == 1) { // 第二种动画：圆环
            int cx=113,cy=55,r=5,dc=6,ad=(at/500)%dc; float so=(at/500.0)*30.0*PI/180.0; // 参数
            for (int i=0;i<dc;i++) { float a=(i*360.0/dc)*PI/180.0-PI/2.0+so; int dx=cx+r*cos(a),dy=cy+r*sin(a); if(i==ad) u8g2.drawDisc(dx,dy,2); else u8g2.drawPixel(dx,dy); } // 绘制
          } else if (sdPlayerAnimType == 2) { // 第三种动画：五分割圆环
            int cx=113,cy=55,r=6,sc=5,as2=(at/1000)%sc; float sa=360.0/sc; // 参数
            for (int i=0;i<sc;i++) { float ma=(i*sa+sa/2-90)*PI/180.0; int sx=cx+r*cos(ma),sy=cy+r*sin(ma); if(i==as2) u8g2.drawFrame(sx-2,sy-2,5,5); else u8g2.drawBox(sx-1,sy-1,3,3); } // 绘制
          } else if (sdPlayerAnimType == 3) { // 第四种动画：三角形切换
            int cx=113,cy=55; bool iH=(at/1000)%2==1; // 参数
            if (iH) { u8g2.drawLine(cx-4,cy-5,cx-4,cy+5); u8g2.drawLine(cx-4,cy-5,cx+4,cy); u8g2.drawLine(cx+4,cy,cx-4,cy+5); } // 绘制空心三角形
            else   { u8g2.drawLine(cx-3,cy-4,cx-3,cy+4); u8g2.drawLine(cx-3,cy-4,cx+3,cy); u8g2.drawLine(cx+3,cy,cx-3,cy+4); } // 绘制实心三角形
          } else if (sdPlayerAnimType == 4) { // 第五种动画：旋转圆弧
            int cx=113,cy=55,r=6; int angle=(at*180/1000)%360; // 参数
            for (int i=0;i<30;i++) { float aa=(angle+i*3)*PI/180.0; for (int rr=r-1;rr<=r+1;rr++) u8g2.drawPixel(cx+rr*cos(aa),cy+rr*sin(aa)); } // 绘制
          } else if (sdPlayerAnimType == 5) { // 第六种动画：音频指示器
            int bx0=108,by0=52,bw=3,bmx=10,bmn=4,bgap=3; // 参数
            for (int i=0;i<3;i++) { float ph=(at/300.0)+(i*2.0); int h=bmn+(bmx-bmn)*(0.5+0.5*sin(ph)); u8g2.drawBox(bx0+i*(bw+bgap),by0+(bmx-h),bw,h); } // 绘制
          } else if (sdPlayerAnimType == 6) { // 第七种动画：正弦频谱
            int ox=108,oy=57,ww=18,wh=8,pc=12; // 参数
            for (int i=0;i<pc;i++) { // 遍历绘制
              float ph=(at/150.0)-(i*0.6); float amp=sin(ph)*0.7+sin(ph*2.3)*0.3; // 计算相位和振幅
              int y=oy+wh/2+(wh/2-1)*amp, x=ox+(i*ww/pc); // 计算坐标
              u8g2.drawPixel(x,y); // 绘制点
              if (i>0) { int px=ox+((i-1)*ww/pc); float pp=(at/150.0)-((i-1)*0.6); float pa=sin(pp)*0.7+sin(pp*2.3)*0.3; int py=oy+wh/2+(wh/2-1)*pa; u8g2.drawLine(px,py,x,y); } // 绘制连接线
            }
          } else if (sdPlayerAnimType == 7) { // 第八种动画：爱心跳动
            int hx=113,hy=57; float sf=0.5+0.5*sin(at*PI/500.0); int hs=3+int(5*sf); // 参数
            for (int dy=-hs;dy<=hs;dy++) for (int dx=-hs;dx<=hs;dx++) { // 遍历绘制
              int cx1=dx+hs/2,cx2=dx-hs/2,cy2=dy; // 坐标计算
              float d1=sqrt(float(cx1*cx1)+float((cy2+hs/3)*(cy2+hs/3))); // 距离计算
              float d2=sqrt(float(cx2*cx2)+float((cy2+hs/3)*(cy2+hs/3))); // 距离计算
              bool inTri=(dy>0)&&(abs(dx)<(hs-dy*hs/(hs+1))); // 三角形判断
              if (d1<hs||d2<hs||inTri) u8g2.drawPixel(hx+dx,hy+dy); // 绘制爱心
            }
          } else if (sdPlayerAnimType == 8) { // 第九种动画：方块滚动
            int ox=108,oy=55,bc=4,bgap=7; // 参数
            float pos=fmod(at/180.0,(bc-1)*2.0); // 位置计算
            if (pos>bc-1) pos=(bc-1)*2.0-pos; // 往返计算
            int ai=int(pos+0.5); // 活动索引
            for (int i=0;i<bc;i++) { int bx=ox+i*bgap,by=oy; if(i==ai) u8g2.drawFrame(bx-1,by-1,5,5); else u8g2.drawBox(bx+1,by+1,3,3); } // 绘制
          } else if (sdPlayerAnimType == 9) { // 第十种动画：日月旋转（大小比例改善，轨道更宽）
            int cx=116,cy=55,br=5,sr=2,orb=br+sr+5; float angle=at*0.002; // 大圆r=5，小圆r=2，轨道半径12
            u8g2.drawDisc(cx,cy,br); // 太阳大圆
            u8g2.drawDisc(cx+(int)(orb*cos(angle)),cy+(int)(orb*sin(angle)),sr); // 月亮绕轨道
          } else if (sdPlayerAnimType == 10) { // 第十一种动画：斜线下雨（多条从左上向右下落）
            { int rx=107,ry=43,rw=20,rh=20; // 雨区20x20px
              int lens[]={5,9,6,10,7,5,8}; int xoff[]={0,3,6,10,13,16,2};
              for (int i=0;i<7;i++) { float spd=0.04f+i*0.008f;
                float off=fmod(at*spd+i*3.1f,(float)(rh+lens[i]));
                int sx=rx+xoff[i]-(int)(off*0.5f),sy=ry+(int)off-lens[i];
                int ex=sx+(int)(lens[i]*0.5f),ey=sy+lens[i];
                if (ey>ry&&sy<ry+rh&&ex>rx&&sx<rx+rw) u8g2.drawLine(sx,sy,ex,ey); } }
          } else if (sdPlayerAnimType == 11) { // 第十二种动画：气泡上升（实心空心大小混合）
            { int bx=107,by=63,bw=20,bh=20; // 气泡区20x20px
              int szs[]={1,3,2,4,1,2,3}; int xpos[]={1,4,8,12,16,10,3};
              bool hollow[]={false,true,false,true,false,true,false};
              for (int i=0;i<7;i++) { float spd=0.03f+i*0.007f;
                float off=fmod(at*spd+i*2.8f,(float)(bh+szs[i]*2));
                int px=bx+xpos[i],py=by-(int)off;
                if (py+szs[i]>=by-bh&&py-szs[i]<=by) {
                  if (hollow[i]) u8g2.drawCircle(px,py,szs[i],U8G2_DRAW_ALL);
                  else u8g2.drawDisc(px,py,szs[i]); } } }
          }
        }

        // 播放模式标签（反白） // 播放模式标签绘制
        u8g2.setFont(u8g2_font_6x10_tr); // 设置小字体
        switch (currentPlayMode) { // 根据播放模式显示
          case PLAY_MODE_SINGLE: // 单曲循环模式
            u8g2.setDrawColor(1); u8g2.drawBox(107,19,21,12); u8g2.setDrawColor(0); u8g2.setCursor(110,29); u8g2.print("<1>"); u8g2.setDrawColor(1); break;
          case PLAY_MODE_SEQUENCE: // 顺序播放模式
            u8g2.setDrawColor(1); u8g2.drawBox(87,19,41,12); u8g2.setDrawColor(0); u8g2.setCursor(90,29); u8g2.print("SEQ->>"); u8g2.setDrawColor(1); break;
          case PLAY_MODE_RANDOM: // 随机播放模式
            u8g2.setDrawColor(1); u8g2.drawBox(87,19,41,12); u8g2.setDrawColor(0); u8g2.setCursor(90,29); u8g2.print("RND-><"); u8g2.setDrawColor(1); break;
        }
      }
      u8g2.sendBuffer(); // 发送缓冲区
      vTaskDelay(200); // 延迟200毫秒
      continue; // 继续循环
    }

    //------------------------------------------------------------------ // SD扫描加载界面分隔符
    // 5. SD扫描加载界面 // 说明
    //------------------------------------------------------------------ // SD扫描加载界面分隔符
    if (menuState == MENU_SD_LOADING) { // 如果在SD卡加载菜单状态
      u8g2.clearBuffer(); // 清除缓冲区
      u8g2.setFont(u8g2_font_wqy12_t_gb2312); // 设置中文字体
      u8g2.setCursor(0, 15); u8g2.print("SD卡扫描中"); // 打印标题
      u8g2.setFont(u8g2_font_ncenB08_tr); // 设置英文字体
      u8g2.drawHLine(0, 17, 128); // 绘制水平分割线

      u8g2.setFont(u8g2_font_wqy12_t_gb2312); // 设置中文字体
      u8g2.setCursor(0, 30); u8g2.print(sdScanStatus); // 打印扫描状态

      u8g2.drawFrame(0, 40, 100, 8); // 绘制进度条边框
      int pw = map(sdScanProgress, 0, 100, 0, 108); // 映射进度到宽度
      u8g2.drawBox(1, 41, pw, 6); // 绘制进度条填充
      char progressStr[10]; snprintf(progressStr, sizeof(progressStr), "%d%%", sdScanProgress); // 格式化进度字符串
      u8g2.drawStr(105, 47, progressStr); // 打印进度百分比

      if (sdScanInProgress && !sdScanComplete) { // 如果扫描进行中且未完成
        char scanInfo[32]; snprintf(scanInfo, sizeof(scanInfo), "Chk:%d/Find:%dMP3", sdCheckedFiles, totalTracks); // 格式化扫描信息
        u8g2.setFont(u8g2_font_ncenB08_tr); u8g2.setCursor(0, 60); u8g2.print(scanInfo); // 打印扫描信息
      } else if (sdScanComplete && totalTracks > 0) { // 如果扫描完成且有曲目
        char trackStr[20]; snprintf(trackStr, sizeof(trackStr), "Found %d MP3", totalTracks); // 格式化曲目数
        u8g2.setFont(u8g2_font_wqy12_t_gb2312); u8g2.setCursor(0, 60); u8g2.print(trackStr); // 打印找到的曲目数
      }

      // 扫描完成，自动进入播放器 // 自动进入说明
      if (sdScanComplete) { // 如果扫描完成
        safeRestartAudioSystem(); // 安全重启音频系统
        digitalWrite(I2S_SD_MODE, 1); // 设置I2S SD模式
        delay(10); // 延迟10毫秒
        if (totalTracks > 0) { // 如果有曲目
          int randomIndex = (millis() % totalTracks); // 随机选择曲目
          if (randomIndex == 0) randomIndex = 1; // 避免索引为0
          playTrack(randomIndex); // 播放曲目
          menuState = MENU_SD_PLAYER; // 切换到SD卡播放器菜单
          loadSDAnimationType(); // 加载动画类型
          encoderPos = constrain(encoderPos, 0, 21); // 限制音量范围
          sdPlaying = true; sdPaused = false; // 标记正在播放
        } else { // 如果没有曲目
          menuState = MENU_SD_PLAYER; // 切换到SD卡播放器菜单
          loadSDAnimationType(); // 加载动画类型
          encoderPos = constrain(encoderPos, 0, 21); // 限制音量范围
          sdPlaying = false; sdPaused = false; // 标记未播放
        }
        menuStartTime = millis(); // 记录菜单开始时间
      }

      u8g2.sendBuffer(); // 发送缓冲区
      vTaskDelay(200); // 延迟200毫秒
      continue; // 继续循环
    }

    //------------------------------------------------------------------ // PONG游戏分隔符
    // 6. PONG 游戏 // 说明
    //------------------------------------------------------------------ // PONG游戏分隔符
    //------------------------------------------------------------------ // PONG游戏分隔符
    if (menuState == MENU_ANIMATION) { // 如果在动画/游戏菜单状态
      u8g2.clearBuffer(); // 清除缓冲区

      if (encoderChanged) { playerMode = true; encoderChanged = false; } // 如果编码器改变，切换到玩家模式
      if (playerMode) { rightPaddleY = constrain(map(encoderPos, 0, 21, 0, 56), 0, 56); } // 玩家模式下编码器控制右挡板

      if (++gameTick % 2 == 0) { // 每2帧更新一次球位置
        ballX += ballDX; ballY += ballDY; // 更新球位置
        if (ballY <= 0 || ballY >= 62) ballDY = -ballDY; // 球碰上下边界反弹

        if (ballX<=2 && ballX>=0 && ballY>=leftPaddleY && ballY<=leftPaddleY+PADDLE_HEIGHT) { // 球碰左挡板
          ballDX=-ballDX; ballX=2; ballDY=map(ballY-leftPaddleY,0,PADDLE_HEIGHT,-3,3); // 反弹并改变角度
        }
        if (ballX>=124 && ballX<=126 && ballY>=rightPaddleY && ballY<=rightPaddleY+PADDLE_HEIGHT) { // 球碰右挡板
          ballDX=-ballDX; ballX=124; ballDY=map(ballY-rightPaddleY,0,PADDLE_HEIGHT,-3,3); // 反弹并改变角度
        }
        if (ballX < 0) { rightScore++; ballX=64; ballY=random(16,48); ballDX=random(2,4); ballDY=random(-3,4); if(!ballDY) ballDY=random(0,2)*2-1; } // 球出左边界，右方得分
        else if (ballX > 128) { leftScore++; ballX=64; ballY=random(16,48); ballDX=-random(2,4); ballDY=random(-3,4); if(!ballDY) ballDY=random(0,2)*2-1; } // 球出右边界，左方得分

        // AI控制左侧（始终） // AI控制说明
        if (ballY > leftPaddleY+PADDLE_HEIGHT/2) leftPaddleY+=2; else if (ballY < leftPaddleY+PADDLE_HEIGHT/2) leftPaddleY-=2; // AI跟踪球
        leftPaddleY = constrain(leftPaddleY, 0, 56); // 限制挡板位置
        if (!playerMode) { // 如果不是玩家模式
          if (ballY > rightPaddleY+PADDLE_HEIGHT/2) rightPaddleY+=2; else if (ballY < rightPaddleY+PADDLE_HEIGHT/2) rightPaddleY-=2; // AI跟踪球
          rightPaddleY = constrain(rightPaddleY, 0, 56); // 限制挡板位置
        }
      }

      // 绘制 // 绘制说明
      for (int y = 0; y < 64; y += 4) u8g2.drawBox(63, y, 2, 2); // 绘制中线
      u8g2.drawBox(0, leftPaddleY, PADDLE_WIDTH, PADDLE_HEIGHT); // 绘制左挡板
      u8g2.drawBox(126, rightPaddleY, PADDLE_WIDTH, PADDLE_HEIGHT); // 绘制右挡板
      u8g2.drawDisc(ballX, ballY, BALL_SIZE); // 绘制球
      u8g2.setFont(u8g2_font_ncenB08_tr); // 设置英文字体
      char scoreStr[10]; snprintf(scoreStr, sizeof(scoreStr), "%d:%d", leftScore, rightScore); // 格式化比分字符串
      int sw = u8g2.getStrWidth(scoreStr); // 获取比分宽度
      u8g2.drawStr((128-sw)/2, 10, scoreStr); // 绘制比分
      u8g2.drawStr(5, 62, playerMode ? "Player Mode" : "Auto Mode"); // 绘制模式提示

      u8g2.sendBuffer(); // 发送缓冲区
      vTaskDelay(50); // 延迟50毫秒
      continue; // 继续循环
    }

    //------------------------------------------------------------------ // 时钟界面分隔符
    // 7. 时钟界面 // 说明
    //------------------------------------------------------------------ // 时钟界面分隔符
    if (menuState == MENU_CLOCK) { // 如果在时钟菜单状态
      u8g2.clearBuffer(); // 清除缓冲区
      struct tm timeinfo; // 时间信息结构体
      static unsigned long ntpStart = 0; // NTP同步开始时间
      static bool ntpSynced = false; // NTP同步标志
      if (!ntpSynced) ntpStart = millis(); // 如果未同步，记录开始时间

      // 非阻塞获取时间（timeout=0），避免oledTask被阻塞5秒导致死机
      static struct tm clockCachedTime;
      static bool clockTimeCached = false;
      bool clockGotTime = getLocalTime(&timeinfo, 0);
      if (clockGotTime) {
        clockCachedTime = timeinfo;
        clockTimeCached = true;
        ntpSynced = true;
      } else if (clockTimeCached) {
        timeinfo = clockCachedTime;
        clockGotTime = true;
      }

      if (!clockGotTime) { // 如果获取本地时间失败
        u8g2.setFont(u8g2_font_ncenB08_tr); // 设置英文字体
        u8g2.drawStr(10, 30, "NTP Syncing..."); // 绘制同步提示
        if (millis()-ntpStart > 5000) u8g2.drawStr(10, 50, "NTP Failed!"); // 5秒后显示失败
        ntpSynced = false; // 标记未同步
      } else { // 如果获取时间成功
        ntpSynced = true; // 标记已同步
        clockGridAnimationPos = timeinfo.tm_sec % 8; // 更新时钟动画位置
        int clockAnimType = preferences.getInt("sdAnimType", 0); // 获取动画类型
        uint32_t at = millis(); // 获取动画时间

        // 左上角动画 // 时钟动画绘制
        if (clockAnimType == 0) { // 九宫格动画
          int gox=2,goy=2,gs=4,gbs=6,gg=1; // 参数
          int gp[8][2]={{0,0},{1,0},{2,0},{2,1},{2,2},{1,2},{0,2},{0,1}}; // 网格位置
          for (int i=0;i<8;i++) { int bx=gox+gp[i][0]*(gs+gg),by=goy+gp[i][1]*(gs+gg); if(i==clockGridAnimationPos){int off=(gbs-gs)/2;u8g2.drawFrame(bx-off,by-off,gbs,gbs);}else u8g2.drawBox(bx,by,gs,gs); } // 绘制九宫格
        } else if (clockAnimType == 1) { // 圆环动画
          int cx=8,cy=7,r=4,dc=6,ad=(at/500)%dc; float so=(at/500.0)*30.0*PI/180.0; // 参数
          for (int i=0;i<dc;i++) { float a=(i*360.0/dc)*PI/180.0-PI/2.0+so; int dx=cx+r*cos(a),dy=cy+r*sin(a); if(i==ad) u8g2.drawDisc(dx,dy,2); else u8g2.drawPixel(dx,dy); } // 绘制圆环
        } else if (clockAnimType == 2) { // 五分割圆环动画
          int cx=8,cy=7,r=5,sc=5,as2=(at/1000)%sc; float sa=360.0/sc; // 参数
          for (int i=0;i<sc;i++) { float ma=(i*sa+sa/2-90)*PI/180.0; int sx=cx+r*cos(ma),sy=cy+r*sin(ma); if(i==as2) u8g2.drawFrame(sx-2,sy-2,4,4); else u8g2.drawBox(sx-1,sy-1,2,2); } // 绘制五分割圆环
        } else if (clockAnimType == 3) { // 三角形切换动画
          int cx=8,cy=7; bool iH=(at/1000)%2==1; // 参数
          if(iH){u8g2.drawLine(cx-3,cy-4,cx-3,cy+4);u8g2.drawLine(cx-3,cy-4,cx+3,cy);u8g2.drawLine(cx+3,cy,cx-3,cy+4);} // 绘制空心三角形
          else{u8g2.drawLine(cx-2,cy-3,cx-2,cy+3);u8g2.drawLine(cx-2,cy-3,cx+2,cy);u8g2.drawLine(cx+2,cy,cx-2,cy+3);} // 绘制实心三角形
        } else if (clockAnimType == 4) { // 旋转圆弧动画
          int cx=8,cy=7,r=5; int angle=(at*180/1000)%360; // 参数
          for (int i=0;i<20;i++) { float aa=(angle+i*3)*PI/180.0; for (int rr=r-1;rr<=r+1;rr++) u8g2.drawPixel(cx+rr*cos(aa),cy+rr*sin(aa)); } // 绘制旋转圆弧
        } else if (clockAnimType == 5) { // 音频指示器动画
          int bx0=2,by0=2,bw=2,bmx=8,bmn=3,bgap=2; // 参数
          for (int i=0;i<3;i++) { float ph=(at/300.0)+(i*2.0); int h=bmn+(bmx-bmn)*(0.5+0.5*sin(ph)); u8g2.drawBox(bx0+i*(bw+bgap),by0+(bmx-h),bw,h); } // 绘制音频指示器
        }

        // 时间 // 时间显示绘制
        u8g2.setFont(u8g2_font_wqy12_t_gb2312); // 设置中文字体
        const char* title = "网络时钟"; // 标题文本
        int16_t tw = u8g2.getUTF8Width(title); // 获取标题宽度
        u8g2.setCursor((128-tw)/2, 15); u8g2.print(title); // 绘制标题
        u8g2.drawHLine(0, 16, 128); // 绘制分割线
        char timeStr[16]; snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec); // 格式化时间字符串
        u8g2.setFont(u8g2_font_fub20_tr); // 设置大字体
        int16_t tsw = u8g2.getStrWidth(timeStr); // 获取时间宽度
        u8g2.drawStr((128-tsw)/2, 38, timeStr); // 绘制时间

        char dateStr[20]; snprintf(dateStr, sizeof(dateStr), "%04d-%02d-%02d", 1900+timeinfo.tm_year, timeinfo.tm_mon+1, timeinfo.tm_mday); // 格式化日期字符串
        u8g2.setFont(u8g2_font_ncenB08_tr); // 设置英文字体
        u8g2.drawStr(10, 55, dateStr); // 绘制日期

        const char* wdays_cn[] = {"星期日","星期一","星期二","星期三","星期四","星期五","星期六"}; // 星期数组
        u8g2.setFont(u8g2_font_wqy12_t_gb2312); // 设置中文字体
        u8g2.setCursor(90, 55); u8g2.print(wdays_cn[timeinfo.tm_wday]); // 绘制星期

        // WiFi信号图标 // WiFi信号图标绘制
        { // 图标代码块
          int cx = 112+6, cy = 4+6, wifiLevel = 0; // 图标坐标和等级
          if (WiFi.isConnected()) { // 如果WiFi已连接
            int rssi = WiFi.RSSI(); // 获取信号强度
            if (rssi>=-60) wifiLevel=3; else if (rssi>=-70) wifiLevel=2; else if (rssi>=-80) wifiLevel=1; // 根据信号强度确定等级
          }
          u8g2.drawDisc(cx, cy+3, 1, U8G2_DRAW_ALL); // 绘制中心点
          if (wifiLevel>=1) u8g2.drawEllipse(cx,cy+1,3,1,U8G2_DRAW_UPPER_RIGHT|U8G2_DRAW_UPPER_LEFT); // 绘制第一层弧
          if (wifiLevel>=2) u8g2.drawEllipse(cx,cy-1,5,2,U8G2_DRAW_UPPER_RIGHT|U8G2_DRAW_UPPER_LEFT); // 绘制第二层弧
          if (wifiLevel>=3) u8g2.drawEllipse(cx,cy-3,7,3,U8G2_DRAW_UPPER_RIGHT|U8G2_DRAW_UPPER_LEFT); // 绘制第三层弧
        }
        drawBatteryIcon(95, 4); // 绘制电池图标
      }
      u8g2.sendBuffer(); // 发送缓冲区
      vTaskDelay(300); // 延迟300毫秒
      continue; // 继续循环
    }

    //------------------------------------------------------------------ // SD退出分隔符
    // 8. SD退出/WiFi配置退出过渡动画 // 说明
    //------------------------------------------------------------------ // SD退出分隔符
    if (menuState == MENU_SD_EXITING) {
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_wqy12_t_gb2312);
      u8g2.setCursor(0, 15);
      if (wifiConfigExiting)        u8g2.print("正在启动WiFi配置");
      else if (configPortalExiting) u8g2.print("正在退出WiFi配置");
      else                          u8g2.print("正在退出SD播放器");

      sdExitAnimationAngle = (sdExitAnimationAngle + 30) % 360;
      drawBouncingBlocks(64, 30, sdExitAnimationAngle);

      u8g2.setCursor(0, 62);
      if (wifiConfigExiting)        u8g2.print("正在释放资源...");
      else if (configPortalExiting) u8g2.print("正在保存并重启...");
      else                          u8g2.print("正在清理资源...");

      u8g2.sendBuffer();
      vTaskDelay(150);
      continue;
    }

    //------------------------------------------------------------------ // 电台切换过渡界面分隔符
    // 9. 电台切换过渡界面（新增）
    //------------------------------------------------------------------ // 电台切换过渡界面分隔符
    if (menuState == MENU_STATION_SWITCHING) { // 如果在电台切换过渡状态
      u8g2.clearBuffer(); // 清除缓冲区
      u8g2.setFont(u8g2_font_wqy12_t_gb2312); // 设置中文字体
      u8g2.setCursor(0, 12);
      u8g2.print("电台切换中"); // 打印标题
      u8g2.setFont(u8g2_font_ncenB08_tr); // 设置英文字体

      // 显示电台名称
      u8g2.setFont(u8g2_font_wqy12_t_gb2312); // 设置中文字体
      u8g2.setCursor(0, 28);
      u8g2.print(connectingStationName.c_str()); // 打印电台名称

      // 弹跳方块动画
      drawBouncingBlocks(64, 42, stationConnectAnimationAngle); // 绘制弹跳方块动画

      // 显示当前状态
      u8g2.setFont(u8g2_font_wqy12_t_gb2312); // 设置中文字体
      u8g2.setCursor(0, 62);
      u8g2.print(infoString); // 打印当前状态信息

      // 如果是重启模式，显示倒计时
      if (stationConnectRetryCount == 255) { // 如果是重启模式
        unsigned long elapsed = millis() - stationConnectStartTime; // 计算已过时间
        if (elapsed >= 2000 && elapsed < 2500) { // 如果在2-2.5秒之间
          u8g2.print(" (3秒后重启)"); // 显示倒计时
        } else if (elapsed >= 2500 && elapsed < 3000) { // 如果在2.5-3秒之间
          u8g2.print(" (2秒后重启)"); // 显示倒计时
        } else if (elapsed >= 3000 && elapsed < 3500) { // 如果在3-3.5秒之间
          u8g2.print(" (1秒后重启)"); // 显示倒计时
        }
      }

      u8g2.sendBuffer(); // 发送缓冲区
      stationConnectAnimationAngle = (stationConnectAnimationAngle + 30) % 360; // 更新动画角度
      vTaskDelay(150); // 延迟150毫秒
      continue; // 继续循环
    }

    //------------------------------------------------------------------ // 流媒体连接等待界面分隔符
    // 10. 流媒体连接等待界面 // 说明
    //------------------------------------------------------------------ // 流媒体连接等待界面分隔符
    if (menuState == MENU_STATION_CONNECTING) { // 如果在电台连接菜单状态
      u8g2.clearBuffer(); // 清除缓冲区
      u8g2.setFont(u8g2_font_wqy12_t_gb2312); // 设置中文字体
      u8g2.setCursor(2, 12); u8g2.print("正在连接"); // 打印连接提示
      u8g2.setFont(u8g2_font_ncenB08_tr); // 设置英文字体

      drawBouncingBlocks(64, 38, stationConnectAnimationAngle); // 绘制弹跳方块动画

      u8g2.setFont(u8g2_font_ncenB08_tr); // 设置英文字体
      u8g2.drawStr(0, 62, infoString); // 打印信息字符串

      u8g2.sendBuffer(); // 发送缓冲区
      stationConnectAnimationAngle = (stationConnectAnimationAngle + 30) % 360; // 更新动画角度
      vTaskDelay(150); // 延迟150毫秒
      continue; // 继续循环
    }

    //------------------------------------------------------------------ // IMP播放器WiFi启动过渡界面分隔符
    // 10. IMP播放器WiFi启动过渡界面 // 说明
    //------------------------------------------------------------------ // IMP播放器WiFi启动过渡界面分隔符
    if (menuState == MENU_WIFI_CONNECTING) { // 如果在WiFi连接菜单状态
      u8g2.clearBuffer(); // 清除缓冲区
      u8g2.setFont(u8g2_font_wqy12_t_gb2312); // 设置中文字体

      unsigned long elapsed = millis() - menuStartTime; // 计算已等待时间

      // ---- 判断当前连接阶段 ----
      bool timedOut  = (elapsed > 30000);                    // 超过30秒视为超时
      bool connected = (WiFi.status() == WL_CONNECTED);      // 是否已连接
      bool scanning  = (elapsed < 15000 && !connected);      // 扫描阶段（通常<15秒）

      if (timedOut && !connected) {
        // ---- 连接失败界面 ----
        u8g2.setCursor(2, 12); u8g2.print("无法连接WiFi"); // 标题

        // 感叹号警告图标（手绘）
        u8g2.drawCircle(64, 36, 12);                         // 外圆
        u8g2.drawBox(62, 27, 4, 10);                         // 叹号竖杠
        u8g2.drawBox(62, 40, 4,  4);                         // 叹号下圆点

        u8g2.setCursor(0, 58); u8g2.print("密码错误?正在进入配置"); // 底部提示
      } else {
        // ---- 正常连接中界面 ----
        // 根据阶段显示不同标题
        if (scanning) {
          u8g2.setCursor(2, 12); u8g2.print("正在扫描WiFi"); // 扫描阶段标题
          // 扫描进度（假设扫描约15秒）
          int scanProgress = min(100, (int)((elapsed * 100) / 15000));
          char progressStr[16];
          snprintf(progressStr, sizeof(progressStr), "%d%%", scanProgress);
          u8g2.setCursor(100, 12); u8g2.print(progressStr);
        } else {
          u8g2.setCursor(2, 12); u8g2.print("正在连接WiFi"); // 连接阶段标题
        }

        drawBouncingBlocks(64, 38, stationConnectAnimationAngle); // 绘制弹跳方块动画

        u8g2.setFont(u8g2_font_wqy12_t_gb2312); // 设置中文字体
        u8g2.setCursor(0, 62); // 设置光标位置

        if (connected) { // 如果WiFi已连接
          u8g2.print("WIFI已连接!"); // 打印连接成功提示
        } else {
          // 显示等待进度点（每0.5秒增加一个点）
          int dotCount = (elapsed / 500) % 4;
          if (scanning) {
            u8g2.print("正在搜索AP...");
          } else {
            char waitStr[16] = "请稍候";
            for (int d = 0; d < dotCount; d++) strcat(waitStr, ".");
            u8g2.print(waitStr);
          }
        }
      }

      u8g2.sendBuffer(); // 发送缓冲区

      if (connected) { // 如果WiFi已连接（pollWiFiConnection会处理后续，这里只更新动画）
        // 短暂播放动画后转场 // 动画后转场说明
        for (int i = 0; i < 8; i++) { // 播放8帧动画
          stationConnectAnimationAngle = (stationConnectAnimationAngle + 30) % 360; // 更新动画角度
          u8g2.clearBuffer(); // 清除缓冲区
          u8g2.setFont(u8g2_font_wqy12_t_gb2312); // 设置中文字体
          u8g2.setCursor(2, 12); u8g2.print("正在连接WiFi"); // 打印提示
          drawBouncingBlocks(64, 38, stationConnectAnimationAngle); // 绘制弹跳方块
          u8g2.setCursor(0, 62); u8g2.print("WIFI已连接!"); // 打印连接成功
          u8g2.sendBuffer(); // 发送缓冲区
          vTaskDelay(150);
        }
        // pollWiFiConnection() 在 loop() 里检测到连接成功后会切换 menuState，此处不重复处理
      }

      stationConnectAnimationAngle = (stationConnectAnimationAngle + 30) % 360; // 更新动画角度
      vTaskDelay(150); // 延迟150毫秒
      continue; // 继续循环
    }

    //------------------------------------------------------------------ // 恢复出厂设置分隔符
    // 11. 恢复出厂设置进行中 // 说明
    //------------------------------------------------------------------ // 恢复出厂设置分隔符
    if (menuState == MENU_FACTORY_RESET) { // 如果在恢复出厂设置菜单状态
      static bool resetCompleted = false; // 恢复完成标志
      if (!resetCompleted) { // 如果恢复尚未完成
        u8g2.clearBuffer(); // 清除缓冲区
        u8g2.setFont(u8g2_font_wqy12_t_gb2312); // 设置中文字体
        u8g2.setCursor(0, 20); u8g2.print("正在恢复出厂设置..."); // 打印初始化提示
        u8g2.setFont(u8g2_font_ncenB08_tr); // 设置英文字体
        u8g2.drawStr(0, 40, "请稍候..."); // 打印等待提示
        static int dotCount = 0; // 点计数变量
        char dotStr[5] = {0}; // 点字符串缓冲
        for (int i = 0; i < dotCount % 4; i++) dotStr[i] = '.'; // 填充点字符
        u8g2.drawStr(0, 60, dotStr); // 打印点
        dotCount++; // 增加点计数
        u8g2.sendBuffer(); // 发送缓冲区
        
        factoryResetAll(); // 执行完全恢复出厂设置
        resetCompleted = true; // 标记恢复完成
        vTaskDelay(500); // 延迟500毫秒
      } else { // 如果恢复已完成
        u8g2.clearBuffer(); // 清除缓冲区
        u8g2.setFont(u8g2_font_wqy12_t_gb2312); // 设置中文字体
        u8g2.setCursor(0, 25); u8g2.print("恢复出厂设置完成"); // 打印完成提示
        u8g2.setFont(u8g2_font_ncenB08_tr); // 设置英文字体
        u8g2.drawStr(0, 50, "Rebooting..."); // 打印重启提示
        u8g2.sendBuffer(); // 发送缓冲区
        vTaskDelay(1500); // 延迟1.5秒让用户看到
        ESP.restart(); // 重启设备
      }
      continue; // 继续循环
    }

    //------------------------------------------------------------------ // 其他菜单分隔符
    // 12. 其他菜单状态 -> displayMenu() // 说明
    //------------------------------------------------------------------ // 其他菜单分隔符
    if (menuState != MENU_NONE        &&
        menuState != MENU_ALARM_CLOCK &&
        menuState != MENU_ALARM_SUBMENU &&
        menuState != MENU_ALARM_SET_TIME &&
        menuState != MENU_ALARM_SET_SOURCE) {
      displayMenu(); // 显示菜单
      vTaskDelay(200); // 延迟200毫秒
      continue; // 继续循环
    }

    //------------------------------------------------------------------ // 正常IMP播放器界面分隔符
    // 13. 正常IMP播放器界面（MENU_NONE） // 说明
    //------------------------------------------------------------------ // 正常IMP播放器界面分隔符

    u8g2.clearBuffer(); // 清除缓冲区
    u8g2.setFont(u8g2_font_ncenB08_tr); // 设置英文字体

    if (configMode) { // 如果是配置模式
      // WiFi配置模式 // 配置模式显示说明
      u8g2.setFont(u8g2_font_wqy12_t_gb2312); // 设置中文字体
      u8g2.setCursor(0, 15); u8g2.print("WiFi配置模式"); // 打印配置模式标题
      u8g2.setFont(u8g2_font_ncenB08_tr); // 设置英文字体
      char ipStr[20]; IPAddress apIP = WiFi.softAPIP(); // 获取AP IP地址
      snprintf(ipStr, sizeof(ipStr), "IP: %d.%d.%d.%d", apIP[0], apIP[1], apIP[2], apIP[3]); // 格式化IP字符串
      u8g2.drawStr(0, 30, ipStr); // 绘制IP地址
      u8g2.drawStr(0, 45, "SSID:"); u8g2.drawStr(40, 45, AP_SSID); // 绘制SSID
      u8g2.drawStr(0, 60, "No Password"); // 绘制密码提示
    } else { // 如果是正常模式
      // IMP播放器正常显示 // 正常播放器显示说明
      u8g2.setCursor(5, LED_LAN_1); // 设置光标位置
      u8g2.setFont(u8g2_font_wqy12_t_gb2312); // 设置中文字体
      u8g2.print("IMP播放器"); // 打印播放器标题
      u8g2.setFont(u8g2_font_ncenB08_tr); // 设置英文字体
      u8g2.drawHLine(0, LED_LAN_1+2, 128); // 绘制水平分割线

      // 音频质量图标 // 音频质量图标说明
      { // 图标代码块
        int iconX = 120, iconY = LED_LAN_1-8; // 图标坐标
        if (audioQualityStatus == "Buffer Low") { u8g2.drawBox(iconX,iconY+6,2,2); } // 缓冲区低
        else if (audioQualityStatus == "Buffer Full") { u8g2.drawBox(iconX,iconY+6,2,2); u8g2.drawBox(iconX+3,iconY+3,2,5); } // 缓冲区满
        else { u8g2.drawBox(iconX,iconY+6,2,2); u8g2.drawBox(iconX+3,iconY+3,2,5); u8g2.drawBox(iconX+6,iconY,2,8); } // 缓冲区正常
      }

      // WiFi信号图标 // WiFi信号图标说明
      { // 图标代码块
        int cx=102+6, cy=4+6, wifiLevel=0;
        if (WiFi.isConnected()) { int rssi=WiFi.RSSI(); if(rssi>=-60) wifiLevel=3; else if(rssi>=-70) wifiLevel=2; else if(rssi>=-80) wifiLevel=1; }
        u8g2.drawDisc(cx,cy+3,1,U8G2_DRAW_ALL);
        if (wifiLevel>=1) u8g2.drawEllipse(cx,cy+1,3,1,U8G2_DRAW_UPPER_RIGHT|U8G2_DRAW_UPPER_LEFT);
        if (wifiLevel>=2) u8g2.drawEllipse(cx,cy-1,5,2,U8G2_DRAW_UPPER_RIGHT|U8G2_DRAW_UPPER_LEFT);
        if (wifiLevel>=3) u8g2.drawEllipse(cx,cy-3,7,3,U8G2_DRAW_UPPER_RIGHT|U8G2_DRAW_UPPER_LEFT);
      }
      drawBatteryIcon(85, 4);

      // 电台名称（第2行）
      u8g2.setCursor(0, LED_LAN_2);
      displayScrollableText(stationString, 30);
      if (strlen(errorInfoString) > 0) displayInvertedScrollableText(errorInfoString, LED_LAN_2, 64, 64);

      // 流信息（第3行）
      if (strlen(infoString) > 0) {
        u8g2.setCursor(0, LED_LAN_3);
        displayScrollableText(infoString, 45);
      } else {
        u8g2.setCursor(0, LED_LAN_3); u8g2.print("<No data>");
      }

      // CPU使用率（第4行，反白） // CPU使用率显示说明
      char cpuStr[20]; snprintf(cpuStr, sizeof(cpuStr), "CPU: %.1f%%", cpuUsage); // 格式化CPU字符串
      int cpuTextWidth = u8g2.getStrWidth(cpuStr); // 获取CPU文本宽度
      u8g2.setDrawColor(1); u8g2.drawBox(0, 53, cpuTextWidth+4, 11); // 绘制反白背景
      u8g2.setDrawColor(0); u8g2.drawStr(2, 63, cpuStr); // 绘制CPU使用率
      u8g2.setDrawColor(1); // 恢复正常颜色

      // 缓冲区占用率 // 缓冲区占用率显示说明
      u8g2.drawStr(75, 62, "Buf:"); // 绘制缓冲区标签
      int bufFilled = audio.inBufferFilled(), bufFree = audio.inBufferFree(), bufTotal = bufFilled + bufFree; // 获取缓冲区信息
      int bufPercent = (bufTotal > 0) ? (bufFilled * 100 / bufTotal) : 0; // 计算占用百分比
      char percentStr[5]; snprintf(percentStr, sizeof(percentStr), "%d%%", bufPercent); // 格式化百分比字符串
      u8g2.drawStr(100, 62, percentStr); // 绘制百分比
    }

    // ==================================================================
    // 音乐闹钟时钟界面
    // ==================================================================
    if (menuState == MENU_ALARM_CLOCK) {
      u8g2.clearBuffer();

      // 非阻塞获取时间（timeout=0）：失败时沿用上次成功的缓存值，避免阻塞oledTask
      static struct tm cachedTime;
      static bool timeCached = false;
      struct tm timeinfo;
      bool gotTime = getLocalTime(&timeinfo, 0);  // timeout=0，立即返回，不阻塞
      if (gotTime) {
        cachedTime  = timeinfo;
        timeCached  = true;
      } else if (timeCached) {
        timeinfo = cachedTime;  // 使用上次成功的时间（不会显示跳变，但秒数不走）
        gotTime  = true;
      }

      // 标题栏（向左移动，给右侧WiFi/电池图标留出空间）
      u8g2.setFont(u8g2_font_wqy12_t_gb2312);
      const char* alarmTitle = alarmEnabled ? "音乐闹钟 ●" : "音乐闹钟 ○";
      u8g2.setCursor(2, 12);
      u8g2.print(alarmTitle);

      // WiFi信号图标（右上角，向上移动2像素）
      {
        int cx=101, cy=8, wifiLevel=0;
        if (WiFi.isConnected()) { int rssi=WiFi.RSSI(); if(rssi>=-60) wifiLevel=3; else if(rssi>=-70) wifiLevel=2; else if(rssi>=-80) wifiLevel=1; }
        u8g2.drawDisc(cx,cy+3,1,U8G2_DRAW_ALL);
        if (wifiLevel>=1) u8g2.drawEllipse(cx,cy+1,3,1,U8G2_DRAW_UPPER_RIGHT|U8G2_DRAW_UPPER_LEFT);
        if (wifiLevel>=2) u8g2.drawEllipse(cx,cy-1,5,2,U8G2_DRAW_UPPER_RIGHT|U8G2_DRAW_UPPER_LEFT);
        if (wifiLevel>=3) u8g2.drawEllipse(cx,cy-3,7,3,U8G2_DRAW_UPPER_RIGHT|U8G2_DRAW_UPPER_LEFT);
      }

      // 电池图标（WiFi右侧）
      drawBatteryIcon(113, 4);

      u8g2.drawHLine(0, 13, 128);

      if (!gotTime) {
        u8g2.setFont(u8g2_font_ncenB08_tr);
        u8g2.drawStr(10, 35, "NTP Syncing...");
      } else {
        // 当前时间（大字）
        char timeStr[9];
        snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d",
                 timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        u8g2.setFont(u8g2_font_fub20_tr);
        int16_t tsw = u8g2.getStrWidth(timeStr);
        u8g2.drawStr((128 - tsw) / 2, 40, timeStr);

        // 日期
        char dateStr[12];
        snprintf(dateStr, sizeof(dateStr), "%02d/%02d",
                 timeinfo.tm_mon + 1, timeinfo.tm_mday);
        u8g2.setFont(u8g2_font_ncenB08_tr);
        u8g2.drawStr(4, 55, dateStr);
      }

      // 闹钟设定时间（右下角）
      char almStr[12];
      snprintf(almStr, sizeof(almStr), "%s%02d:%02d",
               alarmEnabled ? "AL " : "-- ", alarmHour, alarmMinute);
      u8g2.setFont(u8g2_font_ncenB08_tr);
      u8g2.drawStr(72, 52, almStr);

      // 底部：居中绘制一个"齿轮+向下箭头"图标，暗示按钮可进入设置
      // 齿轮：小圆 + 8个外齿
      {
        int cx = 64, cy = 60, r = 3;
        u8g2.drawCircle(cx, cy, r, U8G2_DRAW_ALL);
        u8g2.drawDisc(cx, cy, 1);
        // 8个齿（上下左右+对角）
        for (int t = 0; t < 8; t++) {
          float a = t * 45.0f * PI / 180.0f;
          int x1 = cx + (int)((r + 1) * cosf(a));
          int y1 = cy + (int)((r + 1) * sinf(a));
          int x2 = cx + (int)((r + 2) * cosf(a));
          int y2 = cy + (int)((r + 2) * sinf(a));
          u8g2.drawLine(x1, y1, x2, y2);
        }
        // 齿轮两侧各一个向下箭头（▼），提示按下按钮
        // 左箭头
        u8g2.drawLine(cx - 12, cy - 2, cx - 9, cy + 2);
        u8g2.drawLine(cx - 9,  cy + 2, cx - 6, cy - 2);
        // 右箭头
        u8g2.drawLine(cx + 6,  cy - 2, cx + 9, cy + 2);
        u8g2.drawLine(cx + 9,  cy + 2, cx + 12,cy - 2);
      }

      u8g2.sendBuffer();
      vTaskDelay(500);  // 音乐时钟界面：1秒刷新一次（省电）
      continue;
    }

    // ==================================================================
    // 闹钟子菜单
    // ==================================================================
    if (menuState == MENU_ALARM_SUBMENU) {
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_wqy12_t_gb2312);
      // 标题
      u8g2.setCursor(2, 12); u8g2.print("闹钟设置");
      u8g2.drawHLine(0, 13, 128);

      const char* items[5] = { "设定闹钟时间", "音乐来源", "每天重复", "返回闹钟", "返回选择界面" };
      for (int i = 0; i < 5; i++) {
        int y = 28 + i * 10;
        if (i == menuIndex) {
          u8g2.drawBox(0, y - 9, 128, 11);
          u8g2.setDrawColor(0);
        }
        u8g2.setCursor(4, y); u8g2.print(items[i]);
        // 右侧附加信息
        if (i == 0) {
          char t[8]; snprintf(t, sizeof(t), "%02d:%02d", alarmHour, alarmMinute);
          int tw = u8g2.getStrWidth(t);
          u8g2.setCursor(124 - tw, y); u8g2.print(t);
        } else if (i == 1) {
          const char* sn = alarmSourceName(alarmSource);
          int sw = u8g2.getUTF8Width(sn);
          u8g2.setCursor(124 - sw, y); u8g2.print(sn);
        } else if (i == 2) {
          // 重复开关：用 [ON] / [  ] 图标显示
          const char* rp = alarmRepeat ? "[ON]" : "[  ]";
          int rw = u8g2.getStrWidth(rp);
          u8g2.setCursor(124 - rw, y); u8g2.print(rp);
        }
        u8g2.setDrawColor(1);
      }
      u8g2.sendBuffer();
      vTaskDelay(100);
      continue;
    }

    // ==================================================================
    // 闹钟时间设置界面
    // ==================================================================
    if (menuState == MENU_ALARM_SET_TIME) {
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_wqy12_t_gb2312);
      u8g2.setCursor(2, 12); u8g2.print("设定闹钟时间");
      u8g2.drawHLine(0, 13, 128);

      // 大字显示编辑时间
      char hStr[3], mStr[3];
      snprintf(hStr, sizeof(hStr), "%02d", alarmEditHour);
      snprintf(mStr, sizeof(mStr), "%02d", alarmEditMin);

      u8g2.setFont(u8g2_font_fub20_tr);
      // 小时（编辑中时反白）
      if (alarmEditField == 0) {
        u8g2.drawBox(18, 18, 30, 22);
        u8g2.setDrawColor(0);
      }
      u8g2.drawStr(22, 38, hStr);
      u8g2.setDrawColor(1);

      u8g2.drawStr(50, 38, ":");

      // 分钟（编辑中时反白）
      if (alarmEditField == 1) {
        u8g2.drawBox(60, 18, 30, 22);
        u8g2.setDrawColor(0);
      }
      u8g2.drawStr(64, 38, mStr);
      u8g2.setDrawColor(1);

      // 操作提示
      u8g2.setFont(u8g2_font_wqy12_t_gb2312);
      if (alarmEditField == 0)      { u8g2.setCursor(4, 56); u8g2.print("旋转调小时，按钮确认"); }
      else                          { u8g2.setCursor(4, 56); u8g2.print("旋转调分钟，按钮保存"); }

      u8g2.sendBuffer();
      vTaskDelay(100);
      continue;
    }

    // ==================================================================
    // 闹钟音乐来源选择
    // ==================================================================
    if (menuState == MENU_ALARM_SET_SOURCE) {
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_wqy12_t_gb2312);
      u8g2.setCursor(2, 12); u8g2.print("选择音乐来源");
      u8g2.drawHLine(0, 13, 128);

      const char* srcs[3] = { "MP3播放器", "IMP播放器", "Navidrome" };
      for (int i = 0; i < 3; i++) {
        int y = 28 + i * 14;
        if (i == menuIndex) {
          u8g2.drawBox(0, y - 11, 128, 13);
          u8g2.setDrawColor(0);
        }
        // 当前已保存来源加★
        if (i == alarmSource) {
          u8g2.setCursor(4, y); u8g2.print("★");
          u8g2.setCursor(20, y); u8g2.print(srcs[i]);
        } else {
          u8g2.setCursor(4, y); u8g2.print(srcs[i]);
        }
        u8g2.setDrawColor(1);
      }
      u8g2.sendBuffer();
      vTaskDelay(100);
      continue;
    }

    u8g2.sendBuffer(); // 发送缓冲区到OLED
    vTaskDelay(200); // 延迟200毫秒
  }
}

#endif // OLED_TASK_H // 结束头文件保护