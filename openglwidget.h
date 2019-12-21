#ifndef OPENGLWIDGET_H
#define OPENGLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QDebug>
#include <QMouseEvent>
#include <QTime>
#include <QDateTime>
#include <QtMath>

#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QMatrix4x4>


// 引入GDAL头文件
#include "ogrsf_frmts.h"
#include "renderlayer.h"

class OpenGLWidget : public QOpenGLWidget,
        protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit OpenGLWidget(QWidget * parent);

    void initializeGL() override;
    void paintGL() override;
    void mousePressEvent(QMouseEvent * event);
    void mouseReleaseEvent(QMouseEvent * event);
    void OpenGLWidget::wheelEvent(QWheelEvent *event);
    void OpenGLWidget::keyPressEvent(QKeyEvent *event);
    void OpenGLWidget::mouseMoveEvent(QMouseEvent *event);

    QVector<RenderLayer *> layers;
    QRectF * boundary;

    bool displayMode = false;
    float displaySpeed = 1;

    bool gameMode = false;

    void rotate(bool checked);
    float rotateHeight = 0;

private:
    QOpenGLBuffer m_vertex;
    QOpenGLVertexArrayObject m_object;
    QOpenGLShaderProgram *m_program;

    QVector4D pressVec4;
    QVector4D lastVec4;

    QMatrix4x4 projection;
    QMatrix4x4 rotateVec;

    QMatrix4x4 camera;
    QVector3D cameraPos;
    QVector3D cameraTarget;

    float pitch = .0f;
    float yaw = -90.0f;
    float span = 0;
    QVector3D worldUp;
    QVector3D front;
    QVector3D right;
    QVector3D up;

    QMatrix4x4 perspective;
    QMatrix4x4 moveMatrix;

    QVector4D cursorToView(QMouseEvent * event);
    void cursorToViewBoundary(QMouseEvent * event, QVector4D * leftTop, QVector4D * rightBottom, float offset = 2);
    QVector4D cursorToView(QWheelEvent * event);

    QVector4D viewToWorld(QVector4D point);


};


#endif // OPENGLWIDGET_H
