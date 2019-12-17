#ifndef OPENGLWIDGET_H
#define OPENGLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QDebug>
#include <QMouseEvent>

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

    QVector<RenderLayer *> layers;



private:
    QOpenGLBuffer m_vertex;
    QOpenGLVertexArrayObject m_object;
    QOpenGLShaderProgram *m_program;

    const float * vertexArray;
    QRectF * boundary;
    int arraySize;
    int * pointIndex = nullptr;

    QVector4D pressVec4;

    QMatrix4x4 projection;

    QMatrix4x4 camera;
    QVector3D cameraPos;
    QVector3D cameraTarget;

    QMatrix4x4 perspective;
    QMatrix4x4 moveMatrix;

    QVector4D cursorToView(QMouseEvent * event);
    QVector4D cursorToView(QWheelEvent * event);

    QVector4D viewToWorld(QVector4D point);


};


#endif // OPENGLWIDGET_H
