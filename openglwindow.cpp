#include "openglwindow.h"

#include "ui_openglwindow.h"

// 引入GDAL头文件
#include "ogrsf_frmts.h"

float *gdalDemo();

OpenGLWindow::OpenGLWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::OpenGLWindow) {
  ui->setupUi(this);
  RenderLayer::windowUi = this->ui;
  RenderLayer::window = this;
}

OpenGLWindow::~OpenGLWindow() { delete ui; }

void OpenGLWindow::on_pushButton_clicked() {
  QString curPath = QDir::currentPath();  //获取系统当前目录
  QString dlgTitle = "Open a File";       //对话框标题
  QString filter =
      "Shape File(*.shp);;Program File(*.h *.cpp);;txt "
      "File(*.txt);;All File(*.*)";  //文件过滤器
  QString aFileName =
      QFileDialog::getOpenFileName(this, dlgTitle, curPath, filter);
  qDebug() << "Chosen File Path: " << aFileName;
  // C++中不存在对象为null，对象一定是对应内存空间的。
  // Java中的对象为空，对应C++中指针为空，即不指向任何对象。
  if (aFileName.isEmpty()) {
    qDebug() << "Cancel File Choose";
    return;
  }
  RenderLayer *layer = new RenderLayer(aFileName.toStdString().data());
  ui->openGLWidget->layers.append(layer);
  ui->openGLWidget->boundary = &(layer->boundaryQ);
  ui->openGLWidget->update();
}

void OpenGLWindow::displaySelection() {
  ui->textEdit->setWindowFlag(Qt::Window);
  ui->textEdit->show();

  QString str;
  for (RenderLayer *layer : ui->openGLWidget->layers) {
    str += layer->queryString;
    str += "\n";
  }
  ui->textEdit->setText(str);
}

void OpenGLWindow::on_checkBox_toggled(bool checked) {
  ui->openGLWidget->displaySpeed = ui->lineEdit_3->text().toFloat();
  ui->openGLWidget->displayMode = checked;
}

void OpenGLWindow::on_checkBox_2_toggled(bool checked) {
  ui->openGLWidget->gameMode = checked;
}

void OpenGLWindow::on_checkBox_3_toggled(bool checked) {
  ui->openGLWidget->rotate(checked);
  ui->openGLWidget->rotateHeight = ui->lineEdit->text().toFloat();
}

void OpenGLWindow::on_pushButton_2_clicked() {
  for (RenderLayer *layer : ui->openGLWidget->layers) {
    layer->calculateDensity(ui->lineEdit_4->text().toFloat(),
                            ui->lineEdit_5->text().toFloat());
  }
}

void OpenGLWindow::on_pushButton_3_clicked() {
  for (RenderLayer *layer : ui->openGLWidget->layers) {
    if (layer->geoType == 0) {
      layer->layerHeight = ui->lineEdit_2->text().toFloat();
    }
  }
}

void OpenGLWindow::on_pushButton_4_clicked() {
  for (RenderLayer *layer : ui->openGLWidget->layers) {
    delete layer;
  }
  ui->openGLWidget->layers.clear();
}

void OpenGLWindow::on_checkBox_4_toggled(bool checked) {
  for (RenderLayer *layer : ui->openGLWidget->layers) {
    layer->concreteSelect = checked;
  }
}
