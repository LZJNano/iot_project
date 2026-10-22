/* mqtt_client.c — libmosquitto 0.15 封装
 *
 * API 适配说明 (libmosquitto 0.15 是非常老的版本,API 跟 1.x/2.x 不一样):
 *   - mosquitto_new(id, obj)                          // 2 个参数
 *   - mosquitto_connect(mosq, host, port, keepalive, clean_session)
 *   - 没有 mosquitto_strerror,  直接用 strerror(rc) 或自己写
 *   - 没有 mosquitto_loop_start,  自己起 pthread 调 mosquitto_loop(mosq, 100)
 *   - publish payload 类型是 const void *, 长度是 int
 */
#include "mqtt_client.h"
#include "config.h"
#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <mosquitto.h>

static struct mosquitto *g_mosq = NULL;
static pthread_t        g_loop_tid;
static int              g_loop_running = 0;
static volatile int     g_connected = 0;

/* 当前 broker host,可由 mqtt_client_set_host 覆盖 */
static char g_host[64] = MQTT_HOST;

/* 前向声明:控制指令处理函数(在 main.c 实现) */
extern void on_ctrl_message(const char *payload, int payload_len);

/* libmosquitto 0.15 的回调签名只有 (void *obj, ...) 没有 struct mosquitto* */
static void on_connect(void *obj, int rc)
{
    (void)obj;
    if (rc == 0) {
        LOG_I("MQTT connected to %s:%d", g_host, MQTT_PORT);
        g_connected = 1;
        /* 订阅控制主题,qos=1 */
        if (mosquitto_subscribe(g_mosq, NULL, TOPIC_DEV_CTRL, 1) != 0) {
            LOG_E("subscribe %s failed", TOPIC_DEV_CTRL);
        } else {
            LOG_I("subscribed to %s", TOPIC_DEV_CTRL);
        }
    } else {
        LOG_E("MQTT connect callback rc=%d", rc);
        g_connected = 0;
    }
}

static void on_disconnect(void *obj)
{
    (void)obj;
    LOG_W("MQTT disconnected, will auto-reconnect");
    g_connected = 0;
}

static void on_message(void *obj, const struct mosquitto_message *msg)
{
    (void)obj;
    if (!msg || !msg->topic || !msg->payload) return;

    if (strcmp(msg->topic, TOPIC_DEV_CTRL) == 0) {
        char *payload = malloc(msg->payloadlen + 1);
        if (!payload) return;
        memcpy(payload, msg->payload, msg->payloadlen);
        payload[msg->payloadlen] = '\0';
        LOG_I("RX %s: %s", TOPIC_DEV_CTRL, payload);
        on_ctrl_message(payload, msg->payloadlen);
        free(payload);
    } else {
        LOG_W("RX on unexpected topic %s, ignored", msg->topic);
    }
}

/* 后台线程:不停调 mosquitto_loop 处理收发/心跳/重连 */
static void *loop_thread_fn(void *arg)
{
    (void)arg;
    LOG_I("MQTT loop thread started");
    while (g_loop_running) {
        int rc = mosquitto_loop(g_mosq, 100);  /* 100ms timeout */
        if (rc != 0 && rc != MOSQ_ERR_SUCCESS) {
            /* 连接错误,sleep 一下避免 100% CPU */
            LOG_W("mosquitto_loop rc=%d, sleep %ds", rc, MQTT_RECONNECT_DELAY_SEC);
            sleep(MQTT_RECONNECT_DELAY_SEC);
        }
    }
    LOG_I("MQTT loop thread exiting");
    return NULL;
}

void mqtt_client_set_host(const char *host)
{
    if (!host) return;
    snprintf(g_host, sizeof(g_host), "%s", host);
}

const char *mqtt_client_host(void)
{
    return g_host;
}

int mqtt_client_start(void)
{
    mosquitto_lib_init();

    /* 0.15 API: mosquitto_new(id, obj) */
    g_mosq = mosquitto_new(MQTT_CLIENT_ID, NULL);
    if (!g_mosq) {
        LOG_E("mosquitto_new failed (out of memory?)");
        mosquitto_lib_cleanup();
        return -1;
    }

    mosquitto_connect_callback_set(g_mosq, on_connect);
    mosquitto_disconnect_callback_set(g_mosq, on_disconnect);
    mosquitto_message_callback_set(g_mosq, on_message);

    /* 0.15 API: 多一个 clean_session 参数 */
    int rc = mosquitto_connect(g_mosq, g_host, MQTT_PORT,
                               MQTT_KEEPALIVE, MQTT_CLEAN_SESSION);
    if (rc != MOSQ_ERR_SUCCESS) {
        LOG_E("mosquitto_connect %s:%d failed: rc=%d",
              g_host, MQTT_PORT, rc);
        mosquitto_destroy(g_mosq);
        g_mosq = NULL;
        mosquitto_lib_cleanup();
        return -1;
    }

    /* 起后台线程代替 mosquitto_loop_start (0.15 没有) */
    g_loop_running = 1;
    if (pthread_create(&g_loop_tid, NULL, loop_thread_fn, NULL) != 0) {
        LOG_E("pthread_create loop_thread failed");
        g_loop_running = 0;
        mosquitto_destroy(g_mosq);
        g_mosq = NULL;
        mosquitto_lib_cleanup();
        return -1;
    }

    LOG_I("MQTT client started, connecting to %s:%d ...", g_host, MQTT_PORT);
    return 0;
}

void mqtt_client_stop(void)
{
    if (g_loop_running) {
        g_loop_running = 0;
        pthread_join(g_loop_tid, NULL);
    }
    if (g_mosq) {
        mosquitto_disconnect(g_mosq);
        mosquitto_destroy(g_mosq);
        g_mosq = NULL;
    }
    mosquitto_lib_cleanup();
    g_connected = 0;
}

int mqtt_client_publish(const char *topic, const char *payload,
                        int payload_len, int qos)
{
    if (!g_mosq) {
        LOG_W("publish called before mqtt_client_start");
        return -1;
    }
    int rc = mosquitto_publish(g_mosq, NULL, topic,
                               payload_len, (const void *)payload, qos, false);
    if (rc != MOSQ_ERR_SUCCESS) {
        LOG_E("publish %s failed: rc=%d", topic, rc);
        return -1;
    }
    return 0;
}

bool mqtt_client_is_connected(void)
{
    return g_connected != 0;
}
