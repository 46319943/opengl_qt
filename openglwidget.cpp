﻿#include "openglwidget.h"

OpenGLWidget::OpenGLWidget(QWidget * parent):
    QOpenGLWidget()
{
    this->show();
}

void OpenGLWidget::initializeGL()
{

    initializeOpenGLFunctions();
    makeCurrent();
    glClearColor(.2f, .2f, .2f, 1.0f);

    // 创建Shader
    m_program = new QOpenGLShaderProgram();
    m_program->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/simple.vert");
    m_program->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/simple.frag");
    m_program->link();
    m_program->bind();

    // 创建渲染图层
    RenderLayer * renderLayer = new RenderLayer("D:/Document/Qt/shp/china.shp");
    layers.append(renderLayer);
    layers.append(new RenderLayer("D:/Document/Qt/ConsoleDemo/school.shp"));

    // 设置投影区域
    boundary = &(renderLayer->boundaryQ);
    qDebug() << "boundary:" << *boundary;

    glEnable(GL_PROGRAM_POINT_SIZE);

    // 使用线框模式，可以看看每个单独的三角形样子
//    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}

void OpenGLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);

    m_program->bind();
    m_program->setUniformValue(m_program->uniformLocation("ourColor"),0.0f, 1.0f, 0.0f, 1.0f);

    // 调整纵横比
    QRectF adjustBoundary(*boundary);
    float widgetRatio = float(this->width()) / this->height();
    float boundaryRatio = boundary->width() / boundary->height();

//    qDebug() << "widgetRatio" << widgetRatio << "boundaryRatio" << boundaryRatio;

    if(boundaryRatio < widgetRatio){
        float width = boundary->height() * widgetRatio;
        float dx = width - boundary->width();
        adjustBoundary.adjust(-dx/2,0,dx/2,0);
//        qDebug() << "boundary width" << boundary->width() << "boundary height" << boundary->height();
//        qDebug() << "width" << width << "dx" << dx ;
    }
    else{
        float height = boundary->width() / widgetRatio;
        float dy = height - boundary->height();
        adjustBoundary.adjust(0,-dy/2,0,dy/2);
    }

    // 设置投影，将执行范围映射到标准坐标系
    projection.setToIdentity();
    // QRect的上下坐标反过来
    // 近、远端是指从原点往z负轴看。但这里为了保证z不变，设为1,-1，对xy没有影响，即乘1
    // 如果为-1,1，将z乘-1
    projection.ortho(adjustBoundary.left(),adjustBoundary.right(),adjustBoundary.top(),adjustBoundary.bottom(),1,-1);
    qDebug() << "projection Matrix" << projection;

    // out = (x/w,y/w,z/w)
    perspective.setToIdentity();
    perspective.perspective(45.f,this->width() / this->height(),.1f,100.f);
    qDebug() << "perspective Matrix" << perspective;

    // 设置变换矩阵
    m_program->setUniformValue(m_program->uniformLocation("projection"),projection);
    m_program->setUniformValue(m_program->uniformLocation("move"),moveMatrix);
    m_program->setUniformValue(m_program->uniformLocation("perspective"),perspective);


    // 渲染图层
    int i = 0;
    for(RenderLayer * layer : layers){
        layer->renderInit(m_program);
        layer->render();
    }

}


void OpenGLWidget::mousePressEvent(QMouseEvent *event){
    pressVec4 = this->cursorToView(event);

    this->viewToWorld(pressVec4);
}
void OpenGLWidget::mouseReleaseEvent(QMouseEvent *event){
    QVector4D releaseVec4 = this->cursorToView(event);

    QVector4D worldVec = this->viewToWorld(releaseVec4);

    QVector3D moveVec = (releaseVec4 - pressVec4).toVector3D();

    QMatrix4x4 temp;
    temp.translate(moveVec);
    moveMatrix = temp * moveMatrix;

    for(RenderLayer * layer : layers){
        layer->selectFeature(worldVec.x(),worldVec.y());
    }

    this->update();
}

void OpenGLWidget::wheelEvent(QWheelEvent *event){
    // 用触摸板移动，可能只移动x，导致y为0
    if(event->angleDelta().y() == 0){
        return;
    }

    QVector4D wheelVec4 = this->cursorToView(event);

//    qDebug() << "wheel event" << wheelVec4 << event->angleDelta();

    QMatrix4x4 temp;

    temp.setToIdentity();
    temp.translate(-wheelVec4.toVector3D());
    moveMatrix = temp * moveMatrix;

    temp.setToIdentity();
    // angleDelta().y()的值，是随滚动速度而变化的，会大于360，可以视为正负无穷。
    // 当y=-360时，运算的scale值变为0，<-360时，上下颠倒
    // 因此做限制
    float scaleValue = 1 + 0.3 * event->angleDelta().y() / abs(event->angleDelta().y());
    // 只缩放x,y
    temp.scale(scaleValue,scaleValue);
    moveMatrix = temp * moveMatrix;

    temp.setToIdentity();
    temp.translate(wheelVec4.toVector3D());
    moveMatrix = temp * moveMatrix;

    this->update();
}

QVector4D OpenGLWidget::cursorToView(QMouseEvent * event){
    float x = float(event->x()) / this->width() * 2 - 1;
    float y = float(event->y()) / this->height() * 2 - 1;
    y = - y;
    return QVector4D(x,y,0,1);
}

QVector4D OpenGLWidget::cursorToView(QWheelEvent * event){
    float x = float(event->x()) / this->width() * 2 - 1;
    float y = float(event->y()) / this->height() * 2 - 1;
    y = - y;
    return QVector4D(x,y,0,1);
}

QVector4D OpenGLWidget::viewToWorld(QVector4D point){
    QVector4D result = projection.inverted() * moveMatrix.inverted() * point;
    qDebug() << result;
    return result;
}