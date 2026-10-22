/* main.c — mqtt_iot 程序入口
 *
 * 启动流程:
 *   1. 信号处理 (SIGINT/SIGTERM → 优雅退出)
 *   2. 初始化外设 (LED, LCD, DHT11)
 *   3. 初始化 MQTT 客户端 (连 broker,订阅 ctrl 主题)
 *   4. 创建 DHT11 采集线程 (每 5s 读一次并 publish)
 *   5. 主线程 sleep,等信号
 *
 * 控制指令处理 (on_ctrl_message) 在本文件实现。
 */
#include "config.h"
#include "logger.h"
#include "mqtt_client.h"
#include "led.h"
#include "lcd.h"
#include "dht11.h"
#include "servo.h"
#include "beep.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

/* ===== 全局状态 ===== */
static volatile sig_atomic_t g_should_exit = 0;

/* ===== 信号处理 ===== */
static void on_signal(int sig)
{
    (void)sig;
    g_should_exit = 1;
}

/* ===== DHT11 采集线程 ===== */
static void *dht11_thread_fn(void *arg)
{
    (void)arg;
    LOG_I("dht11 thread started, period=%ds", DHT11_PERIOD_SEC);

    /* 启动时给个 1s 延迟,等 MQTT 连上 broker 再发 */
    sleep(1);

    while (!g_should_exit) {
        if (mqtt_client_is_connected()) {
            int ti, td, hi, hd;
            if (dht11_read(&ti, &td, &hi, &hd) == 0) {
                /* 拼 JSON: {"hum":60,"temperature":25} */
                char json[96];
                int n = snprintf(json, sizeof(json),
                                 "{\"hum\":%d,\"temperature\":%d}",
                                 hi, ti);
                if (n > 0 && n < (int)sizeof(json)) {
                    LOG_I("publish %s: %s", TOPIC_ENV_DATA, json);
                    mqtt_client_publish(TOPIC_ENV_DATA, json, n, 0);
                }
            } else {
                LOG_W("dht11_read failed, skip this cycle");
            }
        }
        /* 等 5 秒,期间响应 g_should_exit */
        for (int i = 0; i < DHT11_PERIOD_SEC * 10 && !g_should_exit; i++) {
            usleep(100 * 1000);
        }
    }
    LOG_I("dht11 thread exiting");
    return NULL;
}

/* ===== 控制指令处理 =====
 *
 * 协议 (来自 readme §6.2):
 *   led_on / led_off                            (全部 LED)
 *   led1_on / led1_off .. led4_on / led4_off   (FS_EMB6818 只有 4 个 LED)
 *   led5_on ~ led8_on: 静默拒绝 (板上不存在)
 *   buzzer_on / buzzer_off                      (本项目暂不接)
 *   lcd_red / lcd_green / lcd_blue / lcd_white / lcd_black
 */
void on_ctrl_message(const char *payload, int payload_len)
{
    if (!payload || payload_len <= 0) return;
    /* payload 是 null-terminated 字符串 (mqtt_client.c 已保证) */

    /* --- LED 全开/全关 --- */
    if (!strcmp(payload, "led_on"))  { led_set_all(1); return; }
    if (!strcmp(payload, "led_off")) { led_set_all(0); return; }

    /* --- LED 单个 led<N>_on/off --- */
    if (strncmp(payload, "led", 3) == 0) {
        int n, on;
        char action[8];
        if (sscanf(payload, "led%d_%s", &n, action) == 2) {
            if (n < 1 || n > LED_NUMS) {
                /* led5~8 板上不存在,不响应但也不 WARN,避免日志被刷爆 */
                return;
            }
            on = !strcmp(action, "on");
            led_set_one(n, on);
            return;
        }
    }

    /* --- 蜂鸣器 (实测可用,如果 /dev/beep 在) --- */
    if (!strcmp(payload, "buzzer_on") || !strcmp(payload, "beep_on")) {
        beep_on();
        return;
    }
    if (!strcmp(payload, "buzzer_off") || !strcmp(payload, "beep_off")) {
        beep_off();
        return;
    }

    /* --- 舵机 (预设角度) ---
     * 协议: servo_0 / servo_45 / servo_90 / servo_135 / servo_180
     * 也支持: servo_angle <N>  (任意角度,0~180)
     */
    if (strncmp(payload, "servo_", 6) == 0) {
        const char *arg = payload + 6;
        if (!strcmp(arg, "0"))   { servo_goto(0); return; }
        if (!strcmp(arg, "45"))  { servo_goto(1); return; }
        if (!strcmp(arg, "90"))  { servo_goto(2); return; }
        if (!strcmp(arg, "135")) { servo_goto(3); return; }
        if (!strcmp(arg, "180")) { servo_goto(4); return; }
        if (strncmp(arg, "angle", 5) == 0) {
            int a = -1;
            if (sscanf(arg + 5, "%d", &a) == 1 && a >= 0 && a <= 180) {
                servo_set_angle(a);
                return;
            }
        }
        LOG_W("unknown servo command: %s", payload);
        return;
    }

    /* --- LCD --- */
    if (strncmp(payload, "lcd_", 4) == 0) {
        const char *name = payload + 4;
        lcd_fill_named(name);
        return;
    }

    LOG_W("unknown command: %s", payload);
}

/* ===== main =====
 * 用法: ./mqtt_iot [broker_ip]
 *   broker_ip 留空则用 config.h 里的默认 (MQTT_HOST)
 */
int main(int argc, char *argv[])
{
    /* 命令行参数覆盖 broker IP */
    if (argc >= 2 && argv[1] && argv[1][0] != '\0') {
        mqtt_client_set_host(argv[1]);
    }

    /* 信号 */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_signal;
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    /* 忽略 SIGPIPE,MQTT 断线时 write 可能触发 */
    signal(SIGPIPE, SIG_IGN);

    LOG_I("===== mqtt_iot starting, broker=%s =====", mqtt_client_host());

    /* 外设初始化 (失败不退出,允许部分外设缺失) */
    if (led_init() != 0) {
        LOG_W("LED init failed, continuing without LED");
    }
    if (lcd_init() != 0) {
        LOG_W("LCD init failed, continuing without LCD");
    }
    if (dht11_init() != 0) {
        LOG_W("DHT11 init failed, will not publish data");
    }
    if (servo_init() != 0) {
        LOG_W("servo init failed, continuing without servo");
    }
    if (beep_init() != 0) {
        LOG_W("beep init failed, continuing without beep");
    }

    /* MQTT */
    if (mqtt_client_start() != 0) {
        LOG_E("MQTT start failed, exiting");
        return 1;
    }

    /* DHT11 采集线程 */
    pthread_t dht11_tid;
    if (pthread_create(&dht11_tid, NULL, dht11_thread_fn, NULL) != 0) {
        LOG_E("pthread_create dht11_thread failed: %s", strerror(errno));
    }

    /* 主线程等信号 */
    while (!g_should_exit) {
        sleep(1);
    }

    LOG_I("===== shutting down =====");
    pthread_join(dht11_tid, NULL);
    mqtt_client_stop();
    led_release();
    lcd_release();
    dht11_release();
    beep_release();
    servo_release();
    LOG_I("===== bye =====");
    return 0;
}
