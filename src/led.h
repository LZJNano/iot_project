/* led.h — LED sysfs 接口 */
#ifndef LED_H
#define LED_H

#include "config.h"

/* 初始化:打开 8 个 LED 的 sysfs 节点,缓存 fd。
 * 返回 0 成功,-1 失败(可能是 fs6818_led.ko 没加载)
 */
int led_init(void);

/* 释放 fd */
void led_release(void);

/* 单个 LED 开关,idx 从 1 开始 */
int led_set_one(int idx, int on);

/* 全部 LED 开关 */
int led_set_all(int on);

#endif /* LED_H */
