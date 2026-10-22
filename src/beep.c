/* beep.c — 蜂鸣器应用层封装 */
#include "beep.h"
#include "logger.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define BEEP_DEV_PATH "/dev/beep"
static int g_fd = -1;

int beep_init(void)
{
    g_fd = open(BEEP_DEV_PATH, O_RDWR);
    if (g_fd < 0) {
        LOG_W("open %s failed: %s (insmod beep.ko + mknod?)",
              BEEP_DEV_PATH, strerror(errno));
        return -1;
    }
    LOG_I("beep: opened %s", BEEP_DEV_PATH);
    return 0;
}

void beep_release(void)
{
    if (g_fd >= 0) {
        /* 关闭前先 off 一下,免得蜂鸣器一直响 */
        ioctl(g_fd, BEEP_OFF, 0);
        close(g_fd);
        g_fd = -1;
    }
}

int beep_on(void)
{
    if (g_fd < 0) return -1;
    if (ioctl(g_fd, BEEP_ON, 0) != 0) {
        LOG_E("beep ON failed: %s", strerror(errno));
        return -1;
    }
    LOG_I("beep ON");
    return 0;
}

int beep_off(void)
{
    if (g_fd < 0) return -1;
    if (ioctl(g_fd, BEEP_OFF, 0) != 0) {
        LOG_E("beep OFF failed: %s", strerror(errno));
        return -1;
    }
    LOG_I("beep OFF");
    return 0;
}

int beep_set_freq(int hz)
{
    if (g_fd < 0) return -1;
    if (hz < 0)   hz = 0;
    if (hz > 100) hz = 100;
    if (ioctl(g_fd, BEEP_SET_FREQ, (unsigned long)&hz) != 0) {
        LOG_E("beep SET_FREQ %d failed: %s", hz, strerror(errno));
        return -1;
    }
    LOG_I("beep freq -> %d", hz);
    return 0;
}
