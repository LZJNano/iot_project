/* led.c — LED sysfs 操作
 *
 * 接口: /sys/class/leds/led{1..8}/brightness
 * 操作: write(fd, "0", 1) 亮, write(fd, "1", 1) 灭
 *         (FS6818B 板子上 LED 逻辑是反的:brightness=0 灯亮,=1 灯灭)
 *
 * 必须在 insmod /lib/modules/3.4.39-farsight/extra/fs6818_led.ko 之后调用
 */
#include "led.h"
#include "logger.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

static int g_led_fd[LED_NUMS] = { -1, -1, -1, -1 };

static int open_one(int idx)
{
    char path[64];
    snprintf(path, sizeof(path), "%s%d%s",
             LED_SYS_PREFIX, idx, LED_BRIGHT_SUFFIX);
    int fd = open(path, O_WRONLY);
    if (fd < 0) {
        LOG_W("open %s failed: %s (LED %d not present?)",
              path, strerror(errno), idx);
    }
    return fd;
}

int led_init(void)
{
    int i;
    int ok = 0;
    for (i = 0; i < LED_NUMS; i++) {
        g_led_fd[i] = open_one(i + 1);  /* idx 从 1 开始 */
        if (g_led_fd[i] >= 0) ok++;
    }
    if (ok == 0) {
        LOG_W("no LED sysfs nodes opened, continuing without LED");
        return 0;   /* 不视为致命错误,允许程序继续运行 */
    }
    LOG_I("LED: opened %d/%d sysfs nodes", ok, LED_NUMS);
    return 0;
}

void led_release(void)
{
    int i;
    for (i = 0; i < LED_NUMS; i++) {
        if (g_led_fd[i] >= 0) {
            close(g_led_fd[i]);
            g_led_fd[i] = -1;
        }
    }
}

int led_set_one(int idx, int on)
{
    if (idx < 1 || idx > LED_NUMS) {
        LOG_W("led idx %d out of range [1,%d]", idx, LED_NUMS);
        return -1;
    }
    if (g_led_fd[idx - 1] < 0) {
        /* 尝试重新打开 */
        g_led_fd[idx - 1] = open_one(idx);
        if (g_led_fd[idx - 1] < 0) return -1;
    }
    const char *v = on ? "0" : "1";   /* 板子反相:on=1 时写 0 才亮 */
    if (write(g_led_fd[idx - 1], v, 1) != 1) {
        LOG_E("write LED %d failed: %s", idx, strerror(errno));
        return -1;
    }
    LOG_I("LED %d -> %s", idx, on ? "ON" : "OFF");
    return 0;
}

int led_set_all(int on)
{
    int i, fail = 0;
    for (i = 1; i <= LED_NUMS; i++) {
        if (led_set_one(i, on) < 0) fail++;
    }
    return fail ? -1 : 0;
}
