#include "openglwidget.h"

OpenGLWidget::OpenGLWidget(QWidget *parent) : QOpenGLWidget() { this->show(); }

void OpenGLWidget::initializeGL() {
  initializeOpenGLFunctions();
  makeCurrent();
  glClearColor(.2f, .2f, .2f, 1.0f);

  // 创建Shader
  m_program = new QOpenGLShaderProgram();
  m_program->addShaderFromSourceFile(QOpenGLShader::Vertex,
                                     "../shaders/simple.vert");
  m_program->addShaderFromSourceFile(QOpenGLShader::Fragment,
                                     "../shaders/simple.frag");
  m_program->link();
  m_program->bind();

  // 创建渲染图层
  RenderLayer *renderLayer = new RenderLayer("../tests/data/qu.shp");
  layers.append(renderLayer);
  //    layers.append(new RenderLayer("D:/Document/Qt/ConsoleDemo/school.shp"));

  // 设置投影区域
  boundary = &(renderLayer->boundaryQ);
  //    qDebug() << "boundary:" << *boundary;

  glEnable(GL_PROGRAM_POINT_SIZE);

  // 使用线框模式，可以看看每个单独的三角形样子
  //    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

  cameraPos = QVector3D(0, 0, 1);
  cameraTarget = QVector3D(0, 0, 0);
  worldUp = QVector3D(0, 1, 0);
}

void OpenGLWidget::paintGL() {
  if (layers.size() == 0) {
    return;
  }
  // 1. 清空上一次，以及一些初始化
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  m_program->bind();
  m_program->setUniformValue(m_program->uniformLocation("ourColor"), 0.0f, 1.0f,
                             0.0f, 1.0f);

  // 调整纵横比
  QRectF adjustBoundary(*boundary);
  float widgetRatio = float(this->width()) / this->height();
  float boundaryRatio = boundary->width() / boundary->height();

  //    qDebug() << "widgetRatio" << widgetRatio << "boundaryRatio" <<
  //    boundaryRatio;

  if (boundaryRatio < widgetRatio) {
    float width = boundary->height() * widgetRatio;
    float dx = width - boundary->width();
    adjustBoundary.adjust(-dx / 2, 0, dx / 2, 0);
    //        qDebug() << "boundary width" << boundary->width() << "boundary
    //        height" << boundary->height(); qDebug() << "width" << width <<
    //        "dx" << dx ;
  } else {
    float height = boundary->width() / widgetRatio;
    float dy = height - boundary->height();
    adjustBoundary.adjust(0, -dy / 2, 0, dy / 2);
  }

  // 2. 设置变换矩阵

  // 设置投影，将执行范围映射到标准坐标系
  projection.setToIdentity();
  // QRect的上下坐标反过来
  // 近、远端是指从原点往z负轴看。但这里为了保证z不变，设为1,-1，对xy没有影响，即乘1
  // 如果为-1,1，将z乘-1
  /*
  By convention, OpenGL is a right-handed system.
  Note that in normalized device coordinates OpenGL actually uses a left-handed
  system (the projection matrix switches the handedness).

  example for ortho and perspective projection:
  // note that we're translating the scene in the reverse direction of where we
  want to move view = glm::translate(view, glm::vec3(0.0f, 0.0f, -3.0f));

  glm::ortho(0.0f, 800.0f, 0.0f, 600.0f, 0.1f, 100.0f);
  glm::mat4 proj = glm::perspective(glm::radians(45.0f),
  (float)width/(float)height, 0.1f, 100.0f);
  */
  projection.ortho(adjustBoundary.left(), adjustBoundary.right(),
                   adjustBoundary.top(), adjustBoundary.bottom(), 1, -1);
  //    qDebug() << "projection Matrix" << projection;

  if (gameMode || displayMode) {
    glEnable(GL_DEPTH_TEST);

    // out = (x/w,y/w,z/w)
    perspective.setToIdentity();
    // 注意Qt中一般返回的是int，但是计算需要float！
    perspective.perspective(45.f, 1, .1f, 100.f);
    //            qDebug() << "perspective Matrix" << perspective;

    front.setX(cos(qDegreesToRadians(yaw)) * cos(qDegreesToRadians(pitch)));
    front.setY(sin(qDegreesToRadians(pitch)));
    front.setZ(sin(qDegreesToRadians(yaw)) * cos(qDegreesToRadians(pitch)));
    front.normalize();
    // Also re-calculate the Right and Up vector
    right = QVector3D::crossProduct(front, worldUp.normalized()).normalized();
    up = QVector3D::crossProduct(right, front).normalized();
    // Normalize the vectors, because their length gets closer to 0 the more you
    // look up or down which results in slower movement.

    //    qDebug() << "cameraPos" << cameraPos;
    camera.setToIdentity();
    camera.lookAt(cameraPos, cameraPos + front, up);

    if (displayMode) {
      camera.setToIdentity();
      int time = QDateTime::currentMSecsSinceEpoch();
      cameraPos = QVector3D(sin(time * displaySpeed / 1000.f) * 3, rotateHeight,
                            cos(time * displaySpeed / 1000.f) * 3);
      camera.lookAt(cameraPos, cameraTarget, QVector3D(0.0f, 1.0f, 0.0f));
    }

  } else {
    glDisable(GL_DEPTH_TEST);
    perspective.setToIdentity();
    camera.setToIdentity();
  }

  // 设置变换矩阵
  m_program->setUniformValue(m_program->uniformLocation("projection"),
                             projection);
  m_program->setUniformValue(m_program->uniformLocation("move"), moveMatrix);
  m_program->setUniformValue(m_program->uniformLocation("perspective"),
                             perspective);
  m_program->setUniformValue(m_program->uniformLocation("camera"), camera);
  m_program->setUniformValue(m_program->uniformLocation("rotate"), rotateVec);

  // 3. 渲染图层
  for (RenderLayer *layer : layers) {
    m_program->setUniformValue(m_program->uniformLocation("layerHeight"),
                               layer->layerHeight);
    layer->renderInit(m_program);
    layer->render();
  }

  this->update();
}

void OpenGLWidget::mousePressEvent(QMouseEvent *event) {
  pressVec4 = this->cursorToView(event);
  lastVec4 = this->cursorToView(event);
  this->viewToWorld(pressVec4);
}

void OpenGLWidget::mouseReleaseEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    QVector4D releaseVec4 = this->cursorToView(event);
    QVector3D moveVec = (releaseVec4 - pressVec4).toVector3D();

    QMatrix4x4 temp;
    temp.translate(moveVec);
    moveMatrix = temp * moveMatrix;

    if (releaseVec4 == pressVec4) {
      QVector4D leftTop, rightBottom;
      cursorToViewBoundary(event, &leftTop, &rightBottom);
      leftTop = this->viewToWorld(leftTop);
      rightBottom = this->viewToWorld(rightBottom);
      QVector4D worldVec = this->viewToWorld(releaseVec4);
      for (RenderLayer *layer : layers) {
        //                layer->selectFeature(worldVec.x(),worldVec.y());
        layer->selectFeature(leftTop.x(), leftTop.y(), rightBottom.x(),
                             rightBottom.y());
      }
    }
  }

  this->update();
}

void OpenGLWidget::mouseMoveEvent(QMouseEvent *event) {
  if (event->buttons() & Qt::RightButton) {
    QVector4D releaseVec4 = this->cursorToView(event);
    QVector3D moveVec = (releaseVec4 - lastVec4).toVector3D();
    lastVec4 = releaseVec4;
    pitch += moveVec.y() * 20;
    yaw += moveVec.x() * 20;

    if (pitch >= 89) {
      pitch = 89;
    } else if (pitch <= -89) {
      pitch = -89;
    }

    this->update();
  }
}

void OpenGLWidget::wheelEvent(QWheelEvent *event) {
  // 用触摸板移动，可能只移动x，导致y为0
  if (event->angleDelta().y() == 0) {
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
  float scaleValue =
      1 + 0.3 * event->angleDelta().y() / abs(event->angleDelta().y());
  // 只缩放x,y
  temp.scale(scaleValue, scaleValue);
  moveMatrix = temp * moveMatrix;

  temp.setToIdentity();
  temp.translate(wheelVec4.toVector3D());
  moveMatrix = temp * moveMatrix;

  this->update();
}

void OpenGLWidget::keyPressEvent(QKeyEvent *event) {
  int key = event->key();
  if (key == Qt::Key_W) {
    cameraPos += front * 0.1;
  } else if (key == Qt::Key_S) {
    cameraPos -= front * 0.1;
  } else if (key == Qt::Key_A) {
    cameraPos -= right * 0.1;
  } else if (key == Qt::Key_D) {
    cameraPos += right * 0.1;
  } else if (key == Qt::Key_Q) {
    cameraPos -= worldUp * 0.1;
  } else if (key == Qt::Key_E) {
    cameraPos += worldUp * 0.1;
  }

  this->update();
}

QVector4D OpenGLWidget::cursorToView(QMouseEvent *event) {
  float x = float(event->x()) / this->width() * 2 - 1;
  float y = float(event->y()) / this->height() * 2 - 1;
  y = -y;
  return QVector4D(x, y, 0, 1);
}

void OpenGLWidget::cursorToViewBoundary(QMouseEvent *event, QVector4D *leftTop,
                                        QVector4D *rightBottom, float offset) {
  float x1 = float(event->x() - offset) / this->width() * 2 - 1;
  float y1 = float(event->y() - offset) / this->height() * 2 - 1;
  y1 = -y1;

  float x2 = float(event->x() + offset) / this->width() * 2 - 1;
  float y2 = float(event->y() + offset) / this->height() * 2 - 1;
  y2 = -y2;

  *leftTop = QVector4D(x1, y1, 0, 1);
  *rightBottom = QVector4D(x2, y2, 0, 1);
}

QVector4D OpenGLWidget::cursorToView(QWheelEvent *event) {
  float x = float(event->x()) / this->width() * 2 - 1;
  float y = float(event->y()) / this->height() * 2 - 1;
  y = -y;
  return QVector4D(x, y, 0, 1);
}

QVector4D OpenGLWidget::viewToWorld(QVector4D point) {
  QVector4D result = projection.inverted() * moveMatrix.inverted() * point;
  qDebug() << result;
  return result;
}

void OpenGLWidget::rotate(bool checked) {
  if (checked) {
    QMatrix4x4 temp;
    temp.translate(0, -1, 0);
    temp.rotate(90, 1, 0, 0);
    rotateVec = temp;
    cameraTarget = QVector3D(0, -1, 0);
  } else {
    rotateVec.setToIdentity();
    cameraTarget = QVector3D(0, 0, 0);
  }
}
