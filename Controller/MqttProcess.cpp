// MqttProcess.cpp
#include "MqttProcess.h"
#include <QStringList>
#include <QByteArray>

MqttProcess::MqttProcess(QObject *parent)
    : QObject(parent), m_port(1883), m_sub(0)
{
}

void MqttProcess::setBroker(const QString &host, int port)
{
    m_host = host;
    m_port = port;
}

void MqttProcess::setBaseTopic(const QString &baseTopic)
{
    m_baseTopic = baseTopic;
    if (m_baseTopic.endsWith('/'))
        m_baseTopic.chop(1);
}

void MqttProcess::publish(const QString &subTopic, const QString &payload)
{
    QProcess *p = new QProcess(this);
    QStringList args;
    args << "-h" << m_host
         << "-p" << QString::number(m_port)
         << "-t" << (m_baseTopic + "/" + subTopic)
         << "-m" << payload;
    p->start("mosquitto_pub", args);
    // 不阻塞,fire-and-forget,完成后自动 delete
    connect(p, SIGNAL(finished(int, QProcess::ExitStatus)),
            p, SLOT(deleteLater()));
}

void MqttProcess::startSubscribe()
{
    if (m_sub) {
        stopSubscribe();
    }
    m_sub = new QProcess(this);
    connect(m_sub, SIGNAL(readyReadStandardOutput()), this, SLOT(onReadyRead()));
    connect(m_sub, SIGNAL(error(QProcess::ProcessError)),
            this, SLOT(onError(QProcess::ProcessError)));
    connect(m_sub, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(onFinished(int, QProcess::ExitStatus)));

    QStringList args;
    args << "-h" << m_host
         << "-p" << QString::number(m_port)
         << "-v"                // 打印 topic
         << "-t" << (m_baseTopic + "/#");
    m_sub->start("mosquitto_sub", args);
    emit subscribeStarted();
}

void MqttProcess::stopSubscribe()
{
    if (m_sub) {
        m_sub->terminate();
        if (!m_sub->waitForFinished(1000))
            m_sub->kill();
        delete m_sub;
        m_sub = 0;
        emit subscribeStoped();
    }
}

void MqttProcess::onReadyRead()
{
    if (!m_sub) return;
    QByteArray data = m_sub->readAllStandardOutput();
    QList<QByteArray> lines = data.split('\n');
    for (int i = 0; i < lines.size(); ++i) {
        QByteArray line = lines[i].trimmed();
        if (line.isEmpty()) continue;
        // mosquitto_sub -v 输出格式: "topic payload"
        int sp = line.indexOf(' ');
        QString topic, payload;
        if (sp < 0) {
            topic = QString::fromUtf8(line);
            payload = QString();
        } else {
            topic = QString::fromUtf8(line.left(sp));
            payload = QString::fromUtf8(line.mid(sp + 1));
        }
        QString prefix = m_baseTopic + "/";
        QString sub = topic.startsWith(prefix)
                      ? topic.mid(prefix.length())
                      : topic;
        emit messageReceived(sub, payload);
    }
}

void MqttProcess::onError(QProcess::ProcessError err)
{
    Q_UNUSED(err);
    if (m_sub) {
        emit subscribeError(QString::fromUtf8(m_sub->errorString().toUtf8()));
    }
}

void MqttProcess::onFinished(int exitCode, QProcess::ExitStatus status)
{
    Q_UNUSED(status);
    emit subscribeError(QString("mosquitto_sub exited, code=%1").arg(exitCode));
}