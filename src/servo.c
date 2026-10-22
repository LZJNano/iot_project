/* servo.c — 舵机应用层封装
 *
 * 设备节点: /dev/servo(由 6818M4/servo_driver.c 加载后 probe 创建设备节点)
 *           如果驱动没装 / 设备节点没建,程序只是 LOG_W 不崩
 *
 * 如果 rootfs 的 /dev 没有 devtmpfs 自动建,可能需要:
 *   cat /proc/devices | grep servo   (通常 misc 设备主设备号是 10)
 *   mknod /dev/servo c 10 <minor>    (minor 从 /proc/misc 找)
 */
#include "servo.h"
#include "logger.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define SERVO_DEV_PATH "/dev/servo"
static int g_fd = -1;

int servo_init(void)
{
    g_fd = open(SERVO_DEV_PATH, O_RDWR);
    if (g_fd < 0) {
        LOG_W("open %s failed: %s (insmod servo.ko + mknod?)",
              SERVO_DEV_PATH, strerror(errno));
        return -1;
    }
    LOG_I("servo: opened %s", SERVO_DEV_PATH);
    return 0;
}

void servo_release(void)
{
    if (g_fd >= 0) {
        close(g_fd);
        g_fd = -1;
    }
}

int servo_set_angle(int angle)
{
    if (g_fd < 0) return -1;
    if (angle < 0)   angle = 0;
    if (angle > 180) angle = 180;
    if (ioctl(g_fd, SERVO_SET_ANGLE, (unsigned long)angle) != 0) {
        LOG_E("servo SET_ANGLE %d failed: %s", angle, strerror(errno));
        return -1;
    }
    LOG_I("servo -> %d°", angle);
    return 0;
}

int servo_goto(int slot)
{
    if (g_fd < 0) return -1;
    unsigned int cmd;
    const char *label;
    switch (slot) {
        case 0:  cmd = SERVO_ROTATE_0;   label = "0°";   break;
        case 1:  cmd = SERVO_ROTATE_45;  label = "45°";  break;
        case 2:  cmd = SERVO_ROTATE_90;  label = "90°";  break;
        case 3:  cmd = SERVO_ROTATE_135; label = "135°"; break;
        case 4:  cmd = SERVO_ROTATE_180; label = "180°"; break;
        default:
            LOG_W("servo slot %d out of range [0..4]", slot);
            return -1;
    }
    if (ioctl(g_fd, cmd, 0) != 0) {
        LOG_E("servo ROTATE_%s failed: %s", label, strerror(errno));
        return -1;
    }
    LOG_I("servo -> %s", label);
    return 0;
}
