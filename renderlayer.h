#ifndef RENDERLAYER_H
#define RENDERLAYER_H

#include "ogrsf_frmts.h"
#include "ogr_core.h"
#include <QDebug>
#include <QRectF>

//#include "ui_openglwindow.h"

#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QMatrix4x4>
#include <QtMath>

#include "gpc.h"
#include "rtreeindex.h"

// 头文件的互相引用，声明位置导致的问题。需要前置声明，并在CPP文件中包含定义。
// 在ui_openglwindow.h头文件中，Ui_OpenGLWindow出现在了类定义之前，所以要先声明
class Ui_OpenGLWindow;
class OpenGLWindow;
class RenderLayer
{
public:
    static Ui_OpenGLWindow * windowUi;
    static OpenGLWindow * window;

    RenderLayer(const char * str, int indexGrid = 5);
    ~RenderLayer();
    // 外接矩形
    OGREnvelope boundary;
    QRectF boundaryQ;

    // 依赖的图层
    OGRLayer * layer;
    QVector<OGRFeature *> features;

    // 建立网格索引的密度
    int indexGrid;
    // 网格索引数量，indexGrid^2
    int gridCount;
    // 每个网格，保存着要素的引用
    QVector<OGRFeature*> * gridArray;
    // 外接矩形数组，用来判断要素落在哪个网格内
    OGREnvelope * envelopeArray;

    // 顶点数组
    QVector<float> vertexVector;
    // 画面时，面的分隔数组
    QVector<int> vertexIndex;

    // 要素类型
    int geoType = 0;

    QOpenGLBuffer vboOrigin;
    QOpenGLVertexArrayObject vaoOrigin;

    QOpenGLBuffer vboTriangular;
    QOpenGLVertexArrayObject vaoTriangular;

    QOpenGLBuffer vboSelect;
    QOpenGLVertexArrayObject vaoSelect;

    QOpenGLBuffer vboDensity;
    QOpenGLVertexArrayObject vaoDensity;

    QOpenGLBuffer vboRTree;
    QOpenGLVertexArrayObject vaoRTree;

    QOpenGLShaderProgram * shader = nullptr;

    void renderInit(QOpenGLShaderProgram * shader);
    void render();

    float lineWidth = 2.f;

    void selectFeature(float x,float y);
    void selectFeature(float x,float y, float x2, float y2);
    QString queryString;
    bool concreteSelect = false;

    void calculateDensity(float radius = 0.1, float resolution = 300);
    void exportDensity(float x,float y,float dx,float dy,int xCount,int yCount,QVector<float> data);

    float layerHeight = 0;

private:
    RenderLayer(OGRLayer * layer, int indexGrid = 5);

    int appendFeature(OGRFeature * feature, QVector<float> & vertexVector, QVector<int> & vertexIndex);

    // 三角化
    void triangulate();
    // 三角化结果
    QVector<float> triResult;
    QVector<int> triIndex;

    // 选择要素
    QVector<float> selectVertex;
    QVector<int> selectVertexIndex;

    QVector<float> densityVertex;

    QVector<float> RTreeVertex;

    RTreeIndex rindex;

    // 取随机数
    float randF();

    float dx;
    float dy;

};

#endif // RENDERLAYER_H
