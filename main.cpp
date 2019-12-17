﻿#include "mainwindow.h"
#include "openglwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    MainWindow w;
    w.setWindowTitle("Demo");
    w.show();

    OpenGLWindow openglWindow;
    openglWindow.show();

    return a.exec();
}
