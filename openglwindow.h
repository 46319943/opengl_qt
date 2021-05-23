#ifndef OPENGLWINDOW_H
#define OPENGLWINDOW_H

#include <QDir>
#include <QFileDialog>
#include <QMainWindow>

namespace Ui {
class OpenGLWindow;
}

class OpenGLWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit OpenGLWindow(QWidget *parent = 0);

  ~OpenGLWindow();

  void displaySelection();

 private slots:

  void on_pushButton_clicked();

  void on_checkBox_toggled(bool checked);

  void on_checkBox_2_toggled(bool checked);

  void on_checkBox_3_toggled(bool checked);

  void on_pushButton_2_clicked();

  void on_pushButton_3_clicked();

  void on_pushButton_4_clicked();

  void on_checkBox_4_toggled(bool checked);

 private:
  Ui::OpenGLWindow *ui;
};

#endif  // OPENGLWINDOW_H
