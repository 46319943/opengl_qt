#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QDir>
#include <QFileDialog>
#include <QString>
#include <QDebug>
#include <QTextCodec>
#include <QMessageBox>

#include <stdio.h>
#include <map>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_pushButton_clicked();

    void on_actionClose_triggered();

    void on_actionOpenGL_triggered();

    void on_action_Shp_triggered();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
