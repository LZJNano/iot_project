/* mqtt_client.h — libmosquitto 客户端封装
 *
 * 依赖: libmosquitto (Ubuntu 包 libmosquitto0-dev)
 * 模式: 异步,自己起 pthread 调 mosquitto_loop(mosq, 100)
 *       (libmosquitto 0.15 没有 mosquitto_loop_start)
 */
#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <stdbool.h>
/* 提供 bool 给旧版 GCC 4.5;若编译环境已定义则忽略 */
#ifndef __bool_true_false_are_defined
typedef int bool;
#define true 1
#define false 0
#endif

/* 在启动前可覆盖默认 broker host (来自 config.h 的 MQTT_HOST)
 * 用法: ./mqtt_iot 192.168.127.100   (命令行参数)
 */
void mqtt_client_set_host(const char *host);

/* 初始化 libmosquitto 库,创建 mosq 客户端,设置回调,连接到 broker,订阅控制主题
 * 返回 0 成功,-1 失败
 */
int mqtt_client_start(void);

/* 停止:断开、释放 mosq,清理 lib */
void mqtt_client_stop(void);

/* 发布一条消息到指定主题
 * payload 二进制安全,允许 \0;qos=0/1, retain=false
 * 返回 0 成功,-1 失败
 */
int mqtt_client_publish(const char *topic, const char *payload,
                        int payload_len, int qos);

/* 查询当前实际使用的 broker host (默认或命令行覆盖后的) */
const char *mqtt_client_host(void);

/* 查询是否已连上 broker */
bool mqtt_client_is_connected(void);

#endif /* MQTT_CLIENT_H */