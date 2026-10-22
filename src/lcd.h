/* lcd.h — LCD framebuffer 接口
 *
 * 基于 fs6818b_test/LCD/lcd.h 改造:
 *   - 去掉了 main()
 *   - 增加了 GREEN 颜色宏(readme 要求 lcd_green 指令)
 *   - 增加了便利函数 lcd_fill_screen_str(),根据字符串名字选颜色
 */
#ifndef LCD_H
#define LCD_H

#include "config.h"

typedef unsigned long color_t;

/* 颜色常量 (RGB888,32bpp framebuffer 用) */
#define BLACK   0x000000UL
#define WHITE   0xFFFFFFUL
#define RED     0xFF0000UL
#define GREEN   0x00FF00UL  /* 新增:readme 要求 lcd_green */
#define BLUE    0x0000FFUL
#define YELLOW  0xFFFF00UL
#define BROWN   0x802A2AUL
#define GRAY    0x00FFFFUL  /* 青色,原代码叫 GRAY */
#define PURPLE  0xA020F0UL
#define ORANGE  0xFF6100UL

/* 初始化 framebuffer;失败时 LOG_E 后返回 -1,成功返回 0 */
int lcd_init(void);

/* 释放 framebuffer 资源 */
void lcd_release(void);

/* 整屏刷一种颜色 */
void lcd_fill_screen(color_t color);

/* 根据名字("red"/"green"/"blue"/"white"/"black")选颜色并刷屏
 * 返回 0 成功,-1 名字不认识
 */
int lcd_fill_named(const char *name);

#endif /* LCD_H */
