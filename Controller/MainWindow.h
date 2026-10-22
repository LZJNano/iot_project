// MainWindow.h - IoT 控制端主窗口
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui/QMainWindow>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QGroupBox>
#include <QTextEdit>
#include <QComboBox>
#include <QList>
#include "MqttProcess.h"
#include "FarSightWidget.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *e);

private slots:
    void onConnectClicked();
    void onDisconnectClicked();
    void onMqttMessage(const QString &subTopic, const QString &payload);
    void onSubscribeError(const QString &err);
    void onLedOnClicked();
    void onLedOffClicked();
    void onServoClicked();
    void onBeepClicked();
    void onLcdColorClicked();
    void onLedAllOnClicked();
    void onLedAllOffClicked();

private:
    void buildUi();
    void setConnected(bool ok);
    void log(const QString &line);

    // 控件
    QLineEdit   *m_hostEdit;
    QLineEdit   *m_portEdit;
    QLineEdit   *m_topicEdit;
    QPushButton *m_connectBtn;
    QPushButton *m_disconnectBtn;
    QLabel      *m_statusLabel;

    QLabel      *m_tempLabel;     // 温度显示
    QLabel      *m_humiLabel;     // 湿度显示

    QPushButton *m_led1OnBtn;
    QPushButton *m_led1OffBtn;
    QPushButton *m_led2OnBtn;
    QPushButton *m_led2OffBtn;
    QPushButton *m_led3OnBtn;
    QPushButton *m_led3OffBtn;
    QPushButton *m_led4OnBtn;
    QPushButton *m_led4OffBtn;
    QPushButton *m_ledAllOnBtn;
    QPushButton *m_ledAllOffBtn;
    QComboBox   *m_servoCombo;
    QPushButton *m_servoBtn;
    QPushButton *m_beepBtn;
    QList<QPushButton *> m_lcdBtns;   // 6 color buttons (see buildUi)
    FarSightWidget       *m_farSight;  // right-pane image + LCD overlay

    QTextEdit   *m_logView;

    // 业务
    MqttProcess *m_mqtt;
    QString      m_lastTemp;
    QString      m_lastHumi;
};

#endif // MAINWINDOW_H