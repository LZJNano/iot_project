// main.cpp - IoT 课设控制端入口
// 兼容 Qt 4.8 / Ubuntu 12.04 / Windows
#include <QtGui/QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("IoT Controller");
    app.setOrganizationName("iot_course");

    MainWindow w;
    w.show();

    return app.exec();
}