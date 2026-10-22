// MainWindow.cpp - IoT Controller main window (Qt 4.8, English UI)
#include "MainWindow.h"
#include "FarSightWidget.h"
#include <QtGui/QApplication>
#include <QSplitter>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QCloseEvent>
#include <QDateTime>
#include <QIntValidator>
#include <QSizePolicy>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_hostEdit(0), m_portEdit(0), m_topicEdit(0),
      m_connectBtn(0), m_disconnectBtn(0), m_statusLabel(0),
      m_tempLabel(0), m_humiLabel(0),
      m_led1OnBtn(0), m_led1OffBtn(0),
      m_led2OnBtn(0), m_led2OffBtn(0),
      m_led3OnBtn(0), m_led3OffBtn(0),
      m_led4OnBtn(0), m_led4OffBtn(0),
      m_ledAllOnBtn(0), m_ledAllOffBtn(0),
      m_servoCombo(0), m_servoBtn(0), m_beepBtn(0),
      m_farSight(0),
      m_logView(0),
      m_mqtt(0)
{
    buildUi();
    setConnected(false);
    resize(960, 540);
    setWindowTitle("IoT Controller");

    // Auto-connect 300ms after start (no manual click needed,
    // also enables headless/xvfb selftest).
    QTimer::singleShot(300, this, SLOT(onConnectClicked()));
}

MainWindow::~MainWindow()
{
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    if (m_mqtt) m_mqtt->stopSubscribe();
    e->accept();
}

void MainWindow::buildUi()
{
    // ---- Splitter: left = controls, right = reserved for future use ----
    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);

    // ---- Left pane: existing control widgets ----
    QWidget     *leftPane = new QWidget(splitter);
    QVBoxLayout *root     = new QVBoxLayout(leftPane);

    // ---- Connection ----
    QGroupBox *connBox = new QGroupBox("MQTT Broker", leftPane);
    QGridLayout *connLay = new QGridLayout(connBox);
    connLay->addWidget(new QLabel("Host:"), 0, 0);
    m_hostEdit = new QLineEdit("127.0.0.1", connBox);
    connLay->addWidget(m_hostEdit, 0, 1);
    connLay->addWidget(new QLabel("Port:"), 0, 2);
    m_portEdit = new QLineEdit("1883", connBox);
    m_portEdit->setValidator(new QIntValidator(1, 65535, m_portEdit));
    connLay->addWidget(m_portEdit, 0, 3);
    connLay->addWidget(new QLabel("Base Topic:"), 1, 0);
    m_topicEdit = new QLineEdit("s5p6818", connBox);
    connLay->addWidget(m_topicEdit, 1, 1, 1, 3);
    m_connectBtn    = new QPushButton("Connect", connBox);
    m_disconnectBtn = new QPushButton("Disconnect", connBox);
    connLay->addWidget(m_connectBtn,    0, 4);
    connLay->addWidget(m_disconnectBtn, 1, 4);
    m_statusLabel = new QLabel("Status: disconnected", connBox);
    connLay->addWidget(m_statusLabel, 2, 0, 1, 5);
    root->addWidget(connBox);

    // ---- Sensor ----
    QGroupBox *statBox = new QGroupBox("Sensor (subscribed)", leftPane);
    QHBoxLayout *statLay = new QHBoxLayout(statBox);
    m_tempLabel = new QLabel("Temp: --", statBox);
    m_tempLabel->setStyleSheet("font-size:18pt; color:#c00;");
    m_humiLabel = new QLabel("Hum:  --", statBox);
    m_humiLabel->setStyleSheet("font-size:18pt; color:#06c;");
    statLay->addWidget(m_tempLabel);
    statLay->addWidget(m_humiLabel);
    root->addWidget(statBox);

    // ---- LED ----
    // Layout (per user's request):
    //   Row 0: LED 1 [On][Off]   LED 2 [On][Off]
    //   Row 1: LED 3 [On][Off]   LED 4 [On][Off]
    //   Row 2: All    [On][Off]
    QGroupBox *ledBox = new QGroupBox("LED Control", leftPane);
    QGridLayout *ledLay = new QGridLayout(ledBox);

    struct Leds { int idx; QPushButton **on; QPushButton **off; } leds[] = {
        {1, &m_led1OnBtn, &m_led1OffBtn},
        {2, &m_led2OnBtn, &m_led2OffBtn},
        {3, &m_led3OnBtn, &m_led3OffBtn},
        {4, &m_led4OnBtn, &m_led4OffBtn},
    };
    for (int i = 0; i < 4; ++i) {
        int row      = i / 2;
        int colBase  = (i % 2) * 3;  // 0 or 3
        QLabel *lbl  = new QLabel(QString("LED %1").arg(leds[i].idx), ledBox);
        *leds[i].on  = new QPushButton("Off", ledBox);
        *leds[i].off = new QPushButton("On",  ledBox);
        (*leds[i].on )->setProperty("ledIdx", leds[i].idx);
        (*leds[i].off)->setProperty("ledIdx", leds[i].idx);
        ledLay->addWidget(lbl,             row, colBase + 0);
        ledLay->addWidget(*leds[i].on,     row, colBase + 1);
        ledLay->addWidget(*leds[i].off,    row, colBase + 2);
        connect(*leds[i].on,  SIGNAL(clicked()), this, SLOT(onLedOnClicked()));
        connect(*leds[i].off, SIGNAL(clicked()), this, SLOT(onLedOffClicked()));
    }
    // All row
    QLabel *allLbl   = new QLabel("All", ledBox);
    m_ledAllOnBtn    = new QPushButton("Off", ledBox);
    m_ledAllOffBtn   = new QPushButton("On",  ledBox);
    ledLay->addWidget(allLbl,          2, 0);
    ledLay->addWidget(m_ledAllOnBtn,   2, 1);
    ledLay->addWidget(m_ledAllOffBtn,  2, 2);
    connect(m_ledAllOnBtn,  SIGNAL(clicked()), this, SLOT(onLedAllOnClicked()));
    connect(m_ledAllOffBtn, SIGNAL(clicked()), this, SLOT(onLedAllOffClicked()));

    root->addWidget(ledBox);

    // ---- Servo + Buzzer ----
    QGroupBox *sbBox = new QGroupBox("Servo / Buzzer", leftPane);
    QHBoxLayout *sbLay = new QHBoxLayout(sbBox);
    m_servoCombo = new QComboBox(sbBox);
    m_servoCombo->addItem("0 deg (left)",   0);
    m_servoCombo->addItem("45 deg",         45);
    m_servoCombo->addItem("90 deg (mid)",   90);
    m_servoCombo->addItem("135 deg",        135);
    m_servoCombo->addItem("180 deg (right)",180);
    m_servoBtn = new QPushButton("Servo Turn", sbBox);
    m_beepBtn  = new QPushButton("Buzzer Beep", sbBox);
    sbLay->addWidget(new QLabel("Servo angle:"));
    sbLay->addWidget(m_servoCombo);
    sbLay->addWidget(m_servoBtn);
    sbLay->addWidget(m_beepBtn);
    root->addWidget(sbBox);

    // ---- LCD ----
    // 6 color buttons: red / green / blue / white / black / yellow.
    // Each fills the whole LCD screen with the named color (no text support).
    QGroupBox   *lcdBox = new QGroupBox("LCD Display (fill color)", leftPane);
    QGridLayout *lcdLay = new QGridLayout(lcdBox);
    struct LcdBtn { const char *label; const char *color; const char *bg; };
    LcdBtn btns[] = {
        {"Red",    "red",    "#c02828"},
        {"Green",  "green",  "#28a028"},
        {"Blue",   "blue",   "#2850c8"},
        {"White",  "white",  "#eeeeee"},
        {"Black",  "black",  "#202020"},
        {"Yellow", "yellow", "#d8c020"},
    };
    m_lcdBtns.clear();
    for (int i = 0; i < 6; ++i) {
        QPushButton *b = new QPushButton(btns[i].label, lcdBox);
        // White text on dark buttons, dark text on the light ones (white/yellow)
        QString fg = (QString(btns[i].color) == "white"
                      || QString(btns[i].color) == "yellow")
                     ? "#202020" : "#ffffff";
        b->setStyleSheet(QString("background-color:%1;color:%2;min-width:80px;")
                           .arg(btns[i].bg).arg(fg));
        b->setProperty("lcdColor", QString(btns[i].color));
        lcdLay->addWidget(b, i / 3, i % 3);
        m_lcdBtns.append(b);
        connect(b, SIGNAL(clicked()), this, SLOT(onLcdColorClicked()));
    }
    root->addWidget(lcdBox);

    // ---- Log ----
    QGroupBox *logBox = new QGroupBox("Log / RX messages", leftPane);
    QVBoxLayout *logLay = new QVBoxLayout(logBox);
    m_logView = new QTextEdit(logBox);
    m_logView->setReadOnly(true);
    logLay->addWidget(m_logView);
    root->addWidget(logBox);

    splitter->addWidget(leftPane);

    // ---- Right pane: width fixed 500, vertical flexible (so the FarSightWidget
    // stays vertically centered in the whole window when it is resized tall).
    // The FarSightWidget itself stays 500x406 and non-stretchable.
    QWidget   *rightPane = new QWidget(splitter);
    rightPane->setMinimumSize(QSize(500, 406));
    rightPane->setMaximumSize(QSize(500, QWIDGETSIZE_MAX));
    rightPane->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);

    QVBoxLayout *rightLay = new QVBoxLayout(rightPane);
    rightLay->setContentsMargins(0, 0, 0, 0);
    rightLay->setSpacing(0);
    rightLay->addStretch(1);

    FarSightWidget *fsw = new FarSightWidget(rightPane);
    // stretch=0 keeps fsw at its sizeHint; the surrounding stretches push it
    // to the vertical middle of rightPane (and therefore of the splitter).
    rightLay->addWidget(fsw, 0, Qt::AlignCenter);
    rightLay->addStretch(1);
    m_farSight = fsw;

    splitter->addWidget(rightPane);

    // Initial split ratio: left flexible, right locked at 500x406.
    // FarSightWidget / QSizePolicy::Fixed makes the right side non-stretchable
    // and non-shrinkable; the left pane takes whatever remains.
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 0);
    splitter->setSizes(QList<int>() << 640 << 500);
    splitter->setMinimumWidth(380);

    setCentralWidget(splitter);

    // ---- Signals ----
    connect(m_connectBtn,    SIGNAL(clicked()), this, SLOT(onConnectClicked()));
    connect(m_disconnectBtn, SIGNAL(clicked()), this, SLOT(onDisconnectClicked()));

    connect(m_led1OnBtn,  SIGNAL(clicked()), this, SLOT(onLedOnClicked()));
    connect(m_led1OffBtn, SIGNAL(clicked()), this, SLOT(onLedOffClicked()));
    connect(m_led2OnBtn,  SIGNAL(clicked()), this, SLOT(onLedOnClicked()));
    connect(m_led2OffBtn, SIGNAL(clicked()), this, SLOT(onLedOffClicked()));
    connect(m_servoBtn,   SIGNAL(clicked()), this, SLOT(onServoClicked()));
    connect(m_beepBtn,    SIGNAL(clicked()), this, SLOT(onBeepClicked()));
    // 6 LCD color buttons are wired inside the LCD section above (onLcdColorClicked)
}

void MainWindow::setConnected(bool ok)
{
    m_connectBtn->setEnabled(!ok);
    m_disconnectBtn->setEnabled(ok);
    m_statusLabel->setText(ok ? "Status: connected" : "Status: disconnected");
}

void MainWindow::log(const QString &line)
{
    QString ts = QDateTime::currentDateTime().toString("HH:mm:ss");
    m_logView->append(QString("[%1] %2").arg(ts).arg(line));
    // Mirror to stderr for headless/xvfb observation.
    QByteArray tsBa   = ts.toLocal8Bit();
    QByteArray lineBa = line.toLocal8Bit();
    fprintf(stderr, "[%s] %s\n", tsBa.constData(), lineBa.constData());
    fflush(stderr);
}

// ---- slots ----

void MainWindow::onConnectClicked()
{
    if (m_mqtt) {
        delete m_mqtt;
        m_mqtt = 0;
    }
    m_mqtt = new MqttProcess(this);
    m_mqtt->setBroker(m_hostEdit->text(), m_portEdit->text().toInt());
    m_mqtt->setBaseTopic(m_topicEdit->text());

    connect(m_mqtt, SIGNAL(messageReceived(QString, QString)),
            this,   SLOT(onMqttMessage(QString, QString)));
    connect(m_mqtt, SIGNAL(subscribeError(QString)),
            this,   SLOT(onSubscribeError(QString)));

    m_mqtt->startSubscribe();
    setConnected(true);
    log(QString("subscribing %1/# from broker %2:%3")
        .arg(m_topicEdit->text()).arg(m_hostEdit->text()).arg(m_portEdit->text()));
    log("topics: sub <base>/env/data (JSON hum/temp), pub <base>/device/ctrl (cmd string)");
}

void MainWindow::onDisconnectClicked()
{
    if (m_mqtt) {
        m_mqtt->stopSubscribe();
        delete m_mqtt;
        m_mqtt = 0;
    }
    setConnected(false);
    log("disconnected");
}

void MainWindow::onMqttMessage(const QString &subTopic, const QString &payload)
{
    log(QString("RX %1 = %2").arg(subTopic).arg(payload));

    // Topics aligned with src/config.h on the device:
    //   sub: <base>/env/data  -> JSON {"hum":N,"temperature":N}
    //   pub: <base>/device/ctrl -> "led1_on" / "servo_90" etc.
    if (subTopic == "env/data") {
        QString p = payload;
        int ih = p.indexOf("\"hum\"");
        int it = p.indexOf("\"temperature\"");
        if (ih >= 0) {
            int colon = p.indexOf(':', ih);
            int comma = p.indexOf(',', colon);
            if (comma < 0) comma = p.indexOf('}', colon);
            if (colon > 0 && comma > colon) {
                QString hum = p.mid(colon + 1, comma - colon - 1).trimmed();
                m_lastHumi = hum;
                m_humiLabel->setText("Hum:  " + hum + " %");
            }
        }
        if (it >= 0) {
            int colon = p.indexOf(':', it);
            int comma = p.indexOf(',', colon);
            if (comma < 0) comma = p.indexOf('}', colon);
            if (colon > 0 && comma > colon) {
                QString tmp = p.mid(colon + 1, comma - colon - 1).trimmed();
                m_lastTemp = tmp;
                m_tempLabel->setText("Temp: " + tmp + " C");
            }
        }
    }
}

void MainWindow::onSubscribeError(const QString &err)
{
    log("ERR " + err);
}

void MainWindow::onLedOnClicked()
{
    if (!m_mqtt) return;
    int idx = sender()->property("ledIdx").toInt();
    if (idx < 1 || idx > 4) return;
    // Align with src/main.c::on_ctrl_message:
    //   led_on / led_off (all)  |  led<N>_on/off (single, 1..4)
    QString cmd = QString("led%1_on").arg(idx);
    m_mqtt->publish("device/ctrl", cmd);
    log("PUB device/ctrl = " + cmd);
}

void MainWindow::onLedOffClicked()
{
    if (!m_mqtt) return;
    int idx = sender()->property("ledIdx").toInt();
    if (idx < 1 || idx > 4) return;
    QString cmd = QString("led%1_off").arg(idx);
    m_mqtt->publish("device/ctrl", cmd);
    log("PUB device/ctrl = " + cmd);
}

void MainWindow::onLedAllOnClicked()
{
    if (!m_mqtt) return;
    m_mqtt->publish("device/ctrl", "led_on");
    log("PUB device/ctrl = led_on");
}

void MainWindow::onLedAllOffClicked()
{
    if (!m_mqtt) return;
    m_mqtt->publish("device/ctrl", "led_off");
    log("PUB device/ctrl = led_off");
}

void MainWindow::onServoClicked()
{
    if (!m_mqtt) return;
    int angle = m_servoCombo->itemData(m_servoCombo->currentIndex()).toInt();
    // Device protocol: servo_0 / servo_45 / servo_90 / servo_135 / servo_180
    QString cmd = QString("servo_%1").arg(angle);
    m_mqtt->publish("device/ctrl", cmd);
    log("PUB device/ctrl = " + cmd);
}

void MainWindow::onBeepClicked()
{
    if (!m_mqtt) return;
    // Device protocol: buzzer_on / beep_on
    m_mqtt->publish("device/ctrl", "buzzer_on");
    log("PUB device/ctrl = buzzer_on");
}

void MainWindow::onLcdColorClicked()
{
    if (!m_mqtt) return;
    QObject *src = sender();
    QString color = src->property("lcdColor").toString();
    if (color.isEmpty()) return;
    m_mqtt->publish("device/ctrl", "lcd_" + color);
    log("PUB device/ctrl = lcd_" + color);
    // Mirror the LCD color onto the right-pane image overlay so the GUI
    // reflects what the device's LCD screen will show.
    if (m_farSight) m_farSight->setLcdColor(color);
}