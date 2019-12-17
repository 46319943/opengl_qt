#include "openglwindow.h"
#include "ui_openglwindow.h"

// 引入GDAL头文件
#include "ogrsf_frmts.h"

float * gdalDemo();

OpenGLWindow::OpenGLWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::OpenGLWindow)
{
    ui->setupUi(this);
    RenderLayer::window = this->ui;

//    RenderLayer l("");

//    ui->openGLWidget->vertexArray = gdalDemo();
}

OpenGLWindow::~OpenGLWindow()
{
    delete ui;
}

void OpenGLWindow::on_pushButton_clicked()
{
    QRectF rect(100,300,5,15);
    qDebug() << "top:" << rect.top() << "bottom:" << rect.bottom();
    qDebug() << rect;
}

void OpenGLWindow::mousePressEvent(QMouseEvent *event){
    QString str;
    for(RenderLayer * layer : ui->openGLWidget->layers){
        str += layer->queryString;
    }
    ui->textEdit->setText(str);
}

void OpenGLWindow::on_checkBox_toggled(bool checked)
{
    ui->openGLWidget->displayMode = checked;
}

void OpenGLWindow::on_checkBox_2_toggled(bool checked)
{
    ui->openGLWidget->gameMode = checked;
}

void OpenGLWindow::on_checkBox_3_toggled(bool checked)
{
    ui->openGLWidget->rotate(checked);
    ui->openGLWidget->rotateHeight = ui->lineEdit->text().toFloat();
}
