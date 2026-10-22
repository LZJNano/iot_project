/* dht11.c — DHT11 温湿度读取
 *
 * 设备节点: /dev/dht11
 *
 * 数据格式(read 返回 4 字节,从 6818B dht11.c 看驱动返回 data+1,即):
 *   buf[0] = 湿度小数,  buf[1] = 温度整数
 *   buf[2] = 温度小数,  buf[3] = 校验和 (0..3 之间的部分和 — 暂时忽略,只用 buf[1])
 * 华清远见 6818B 出厂 dht11 驱动 read() 返回的正是 data[1..4](data[0] 是湿度整数,被跳过了)。
 *
 * 注意: read() 是阻塞的,驱动内部完成单线协议时序,一次读取约 100~200ms,5s 周期够用。
 *
 * 自动 mknod:实验箱出厂 rootfs 的 /dev 是空目录,没有 devtmpfs 自动创建设备节点。
 * 所以本程序在 open 失败时主动 mknod /dev/dht11(主 804,次 0,这是 6818B 驱动写死的)。
 * 需要 root 权限;rootfs 启动后 init 通常是 root,这个调用能成功。
 */
#include "dht11.h"
#include "config.h"
#include "logger.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

/* 6818B 驱动写死的主次设备号 */
#define DHT11_MAJOR 804
#define DHT11_MINOR 0

static int g_fd = -1;

static void try_mknod(void)
{
    /* 如果已经存在就当无事发生 */
    struct stat st;
    if (stat(DHT11_DEV_PATH, &st) == 0) {
        return;
    }
    /* mknod 需要 root,失败就交给上层 LOG */
    if (mknod(DHT11_DEV_PATH, S_IFCHR | 0666,
              makedev(DHT11_MAJOR, DHT11_MINOR)) != 0) {
        LOG_W("mknod %s c %d %d failed: %s",
              DHT11_DEV_PATH, DHT11_MAJOR, DHT11_MINOR, strerror(errno));
    } else {
        LOG_I("mknod %s c %d %d ok", DHT11_DEV_PATH, DHT11_MAJOR, DHT11_MINOR);
    }
}

int dht11_init(void)
{
    g_fd = open(DHT11_DEV_PATH, O_RDWR);
    if (g_fd < 0) {
        LOG_W("open %s failed: %s — try mknod and retry",
              DHT11_DEV_PATH, strerror(errno));
        try_mknod();
        g_fd = open(DHT11_DEV_PATH, O_RDWR);
    }
    if (g_fd < 0) {
        LOG_E("open %s still failed: %s (driver loaded? probe ok?)",
              DHT11_DEV_PATH, strerror(errno));
        return -1;
    }
    LOG_I("DHT11: opened %s", DHT11_DEV_PATH);
    return 0;
}

void dht11_release(void)
{
    if (g_fd >= 0) {
        close(g_fd);
        g_fd = -1;
    }
}

int dht11_read(int *temp_int, int *temp_dec,
               int *humi_int, int *humi_dec)
{
    if (g_fd < 0) {
        LOG_W("dht11_read called before init");
        return -1;
    }
    unsigned char buf[4] = {0};
    ssize_t n = read(g_fd, buf, sizeof(buf));
    if (n != sizeof(buf)) {
        LOG_E("DHT11 read failed: %s (n=%zd)", strerror(errno), n);
        return -1;
    }
    /* 6818B 驱动返回的是 data[1..4]:湿度小数/温度整数/温度小数/校验和 */
    *humi_dec = buf[0];
    *temp_int = buf[1];
    *temp_dec = buf[2];
    /* 拿不到湿度整数(被驱动跳过了),先用 0 占位;板子上湿度整数通常就是 50~70 那种量级 */
    *humi_int = 0;
    return 0;
}
