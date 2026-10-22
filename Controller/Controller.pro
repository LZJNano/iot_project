# ----------------------------------------------------
# Controller - IoT 课设控制端 (Qt 4.8, 兼容 Ubuntu 12.04)
# ----------------------------------------------------
QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET   = Controller
TEMPLATE = app

SOURCES += main.cpp \
           MainWindow.cpp \
           MqttProcess.cpp \
           FarSightWidget.cpp

HEADERS += MainWindow.h \
           MqttProcess.h \
           FarSightWidget.h

FORMS   +=

RESOURCES +=

# 不要 warnings 当 error(-Werror),旧编译器友好
QMAKE_CXXFLAGS += -fno-strict-aliasing

# Windows 下部署时把 Qt 核心 dll 拷到 bin
win32 {
    CONFIG += console
}

INSTALLS += target