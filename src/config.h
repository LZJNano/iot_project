/* config.h — 全局常量配置
 *
 * 把所有"魔法值"集中到这里,方便日后调整。
 * 实际使用方: mqtt_iot.c / mqtt_client.c / dht11.c / peripherals.c
 */
#ifndef CONFIG_H
#define CONFIG_H

/* ===== MQTT 服务器 =====
 * MQTT_HOST 默认是实验室 Ubuntu VM 桥接后的 IP(与实验箱同子网)。
 * 桥接模式:192.168.100.192
 * NAT 模式:192.168.127.100
 * 可通过命令行参数覆盖: ./mqtt_iot <broker_ip>
 * (在 main.c 里覆盖 g_mqtt_host 静态变量)
 */
#ifndef MQTT_HOST
#define MQTT_HOST        "192.168.100.192" /* Ubuntu VM 桥接后 IP,Mosquitto broker */
#endif
#define MQTT_PORT        1883
#define MQTT_KEEPALIVE   60                /* 秒 */
#define MQTT_CLIENT_ID   "fs6818_mqtt_iot" /* client id,broker 用作区分 */
#define MQTT_CLEAN_SESSION 1

/* ===== 主题 ===== */
#define TOPIC_ENV_DATA   "s5p6818/env/data"     /* 上行: 温湿度 */
#define TOPIC_DEV_CTRL   "s5p6818/device/ctrl"  /* 下行: 控制指令 */
#define TOPIC_DEV_STATUS "s5p6818/device/status"/* (可选) 上行: 状态 */

/* ===== DHT11 采集 ===== */
#define DHT11_DEV_PATH   "/dev/dht11"
#define DHT11_PERIOD_SEC 5                    /* 采集周期,秒 */

/* ===== LED (sysfs) =====
 * 实测 FS_EMB6818 实验箱只有 led1~led4 (4 个),不是 8 个。
 * led5~led8 的 sysfs 节点在板上根本不存在。
 */
#define LED_NUMS         4
#define LED_SYS_PREFIX   "/sys/class/leds/led"
#define LED_BRIGHT_SUFFIX "/brightness"

/* ===== 蜂鸣器 (PWM sysfs,本项目暂不接控制,函数留空) ===== */
#define BUZZER_PATH      "/sys/devices/platform/pwm/pwm.2"

/* ===== LCD (framebuffer) ===== */
#define LCD_FB_PATH      "/dev/fb0"

/* ===== 重连 ===== */
#define MQTT_RECONNECT_DELAY_SEC 3

#endif /* CONFIG_H */
