// MqttProcess.h - 用 QProcess 包装 mosquitto_pub / mosquitto_sub
// 故意不依赖 libmosquitto C 库,Windows/Ubuntu 通用,Ubuntu 12.04 上
// `apt install mosquitto-clients` 即可,演示机器上已经有 broker 工具。
#ifndef MQTT_PROCESS_H
#define MQTT_PROCESS_H

#include <QObject>
#include <QProcess>
#include <QString>

class MqttProcess : public QObject
{
    Q_OBJECT
public:
    explicit MqttProcess(QObject *parent = 0);

    // 设置 broker host 和基础 topic
    void setBroker(const QString &host, int port);
    void setBaseTopic(const QString &baseTopic);

    // 发布控制命令(topic 是相对 baseTopic 的子路径,例如 "led/1")
    void publish(const QString &subTopic, const QString &payload);

    // 订阅 -t <baseTopic>/# 形式,start() 后所有收到的消息 emit messageReceived(topic, payload)
    void startSubscribe();
    void stopSubscribe();

signals:
    // subTopic 是相对 baseTopic 的子路径(去掉 baseTopic/ 前缀),便于上层路由
    void messageReceived(const QString &subTopic, const QString &payload);
    void subscribeError(const QString &err);
    void subscribeStarted();
    void subscribeStoped();

private slots:
    void onReadyRead();
    void onError(QProcess::ProcessError err);
    void onFinished(int exitCode, QProcess::ExitStatus status);

private:
    QString m_host;
    int     m_port;
    QString m_baseTopic;
    QProcess *m_sub;
};

#endif // MQTT_PROCESS_H