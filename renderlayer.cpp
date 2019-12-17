#include "renderlayer.h"
#include "ui_openglwindow.h"

RenderLayer::RenderLayer(OGRLayer * layer, int indexGrid):
    layer(layer),indexGrid(indexGrid),gridCount(indexGrid * indexGrid)
{
    // 1.创建索引，并进行相关初始化

    // 创建索引网格
    gridArray = new QVector<OGRFeature*> [indexGrid * indexGrid];
    envelopeArray = new OGREnvelope [indexGrid * indexGrid];
    // 获取图层MBR
    layer->GetExtent(&boundary);
    boundaryQ.setCoords(boundary.MinX,boundary.MinY,boundary.MaxX,boundary.MaxY);
    qDebug() << boundaryQ;
    qDebug() << "Boundary Y:" << boundary.MinY << boundary.MaxY;
    dx = (boundary.MaxX - boundary.MinX) / indexGrid;
    dy = (boundary.MaxY - boundary.MinY) / indexGrid;
    // 创建网格对应的外接矩形，用来和要素判断相交情况
    for(int i = 0; i < indexGrid * indexGrid ; i++){
        float left = (i % 5) * dx + boundary.MinX;
        float right = ((i % 5) + 1) * dx + boundary.MinX;
        float bottom = (i / 5) * dy + boundary.MinY;
        float top = ((i / 5) + 1) * dy + boundary.MinY;
        envelopeArray[i].Merge(left,top);
        envelopeArray[i].Merge(right,bottom);
        qDebug() << left << right << bottom << top;
    }

    // 如果是面的话，记录当前面点的数量
    int polygonPointCount = 0;

    // 2.读取要素

    layer->ResetReading();
    OGRFeature *feature;
    OGREnvelope envelope;
    while( (feature = layer->GetNextFeature()) != NULL ){

        // 1.将要素根据外接矩形存入索引网格数组

        feature->GetGeometryRef()->getEnvelope(&envelope);
        for(int i = 0; i < gridCount; i++){
            if(envelope.Intersects(envelopeArray[i])){
                gridArray[i].append(feature);
            }
        }

        // 2.获取要素的顶点

        // 判断要素类型
//        qDebug() << feature->GetGeometryRef()->getGeometryName();
        int featureType = feature->GetGeometryRef()->getGeometryType();
        if(featureType == wkbPoint){
            OGRPoint * point = (OGRPoint *)feature->GetGeometryRef();
            vertexVector.append(point->getX());
            vertexVector.append(point->getY());
        }
        else if(featureType == wkbPolygon){
            geoType = 1;
            OGRPolygon * polygon = (OGRPolygon *)feature->GetGeometryRef();
            OGRLinearRing * ring = polygon->getExteriorRing();
            vertexIndex.append(polygonPointCount);
            vertexIndex.append(ring->getNumPoints());
            polygonPointCount += ring->getNumPoints();
            for(int i = 0; i < ring->getNumPoints(); i++){
                vertexVector.append(ring->getX(i));
                vertexVector.append(ring->getY(i));
            }
        }
        else if(featureType == wkbMultiPolygon){
            geoType = 1;
            OGRMultiPolygon * mulPolygon = (OGRMultiPolygon *)feature->GetGeometryRef();
            for(int i = 0; i < mulPolygon->getNumGeometries(); i++){
                OGRPolygon * polygon = (OGRPolygon *)mulPolygon->getGeometryRef(i);
                OGRLinearRing * ring = polygon->getExteriorRing();
                vertexIndex.append(polygonPointCount);
                vertexIndex.append(ring->getNumPoints());
                polygonPointCount += ring->getNumPoints();
                // 两层循环的循环变量注意命名！！！
                for(int j = 0; j < ring->getNumPoints(); j++){
                    vertexVector.append(ring->getX(j));
                    vertexVector.append(ring->getY(j));
                }
            }
        }

    }

}

RenderLayer::RenderLayer(const char * str, int indexGrid)
{
    GDALAllRegister();
    GDALDataset *poDS;
    // 中文乱码
    CPLSetConfigOption("SHAPE_ENCODING","");
    poDS = (GDALDataset*) GDALOpenEx( str, GDAL_OF_VECTOR, NULL, NULL, NULL );
    if( poDS == NULL )
    {
        printf( "Open failed.\n" );
        exit( 1 );
    }

    // placement new
    new (this) RenderLayer(poDS->GetLayer(0),indexGrid);

    triangulate();

}
void RenderLayer::renderInit(QOpenGLShaderProgram *shader){
    // 如果已经进行初始化，直接返回
    if(this->shader != nullptr){
        return;
    }
    this->shader = shader;

    vaoFirst.create();
    vaoFirst.bind();

    vboFirst.create();
    vboFirst.bind();
    vboFirst.setUsagePattern(QOpenGLBuffer::StaticDraw);
    vboFirst.allocate(vertexVector.constData() , vertexVector.size() * sizeof(float));
    shader->enableAttributeArray(0);
    shader->setAttributeBuffer(0, GL_FLOAT, 0, 2, 2 * sizeof(float));

    if(geoType == 2){
        vaoSecond.create();
        vaoSecond.bind();

        vboSecond.create();
        vboSecond.bind();
        vboSecond.setUsagePattern(QOpenGLBuffer::StaticDraw);
        vboSecond.allocate(triResult.constData() , triResult.size() * sizeof(float));
        shader->enableAttributeArray(0);
        shader->setAttributeBuffer(0, GL_FLOAT, 0, 2, 2 * sizeof(float));
    }

    // second 和 select看清楚。最好改命名，规范一下。
    vaoSelect.create();
    vaoSelect.bind();

    vboSelect.create();
    vboSelect.bind();
    vboSelect.setUsagePattern(QOpenGLBuffer::DynamicDraw);
//    vboSelect.allocate(selectVertex.constData() , selectVertex.size() * sizeof(float));
    shader->enableAttributeArray(0);
    shader->setAttributeBuffer(0, GL_FLOAT, 0, 2, 2 * sizeof(float));

}

void RenderLayer::render(){
    // 如果没有进行初始化，不渲染，返回
    if(shader == nullptr){
        return;
    }
    QOpenGLFunctions *glFuncs = QOpenGLContext::currentContext()->functions();

    // 1. 画面

    if(geoType == 2){

        // 先画面，再画线，不然被覆盖
        vaoSecond.bind();
        shader->setUniformValue(shader->uniformLocation("ourColor")
                                ,.0f, .8f, .0f, 1.0f);
        for(int i = 0; i < triIndex.size() / 2; i++){
            glFuncs->glDrawArrays(GL_TRIANGLE_STRIP,triIndex.at(2 * i),triIndex.at(2 * i + 1));
        }

    }

    // 2. 画点、线

    vaoFirst.bind();
    shader->setUniformValue(shader->uniformLocation("ourColor")
                            ,.8f, .8f, .8f, 1.0f);

    if(geoType == 0){
        glFuncs->glDrawArrays(GL_POINTS,0,vertexVector.size() / 2);
    }
    else if(geoType == 1 || geoType == 2){
        glFuncs->glLineWidth(lineWidth);
        for(int i = 0; i < vertexIndex.size() / 2; i++){
            glFuncs->glDrawArrays(GL_LINE_STRIP,vertexIndex.at(2 * i),vertexIndex.at(2 * i + 1));

        }
    }

    // 3. 画选择部分

    if(selectVertex.size() == 0){
        return;
    }

    vaoSelect.bind();
    // 需要绑定VBO才能重新分配内存！！！
    vboSelect.bind();
    vboSelect.allocate(selectVertex.constData() , selectVertex.size() * sizeof(float));

    shader->setUniformValue(shader->uniformLocation("ourColor")
                            ,.0f, .5f, .1f, 1.0f);

    if(geoType == 0){
        glFuncs->glDrawArrays(GL_POINTS,0,selectVertex.size() / 2);
    }
    else if(geoType == 1 || geoType == 2)
    {
        glFuncs->glLineWidth(lineWidth);
        for(int i = 0; i < selectVertexIndex.size() / 2; i++){
            glFuncs->glDrawArrays(GL_LINE_STRIP,selectVertexIndex.at(2 * i),selectVertexIndex.at(2 * i + 1));
        }
    }
}

void RenderLayer::triangulate(){
    if(geoType != 1){
        return;
    }

    int totalVertex = 0;
    for(int i = 0; i < vertexIndex.size() / 2; i++){

        int startIndex = vertexIndex.at(2 * i);
        int vertexNum = vertexIndex.at(2 * i + 1);

        gpc_vertex * vertexArray = new gpc_vertex[vertexNum];
        Vector2dVector a;
        for(
            int j = 0;
            j < vertexNum;
            j++){
            vertexArray[j].x = vertexVector.at(2 * (j + startIndex));
            vertexArray[j].y = vertexVector.at(1 + 2 * (j + startIndex));
        }

        gpc_vertex_list gpcVertexList = {vertexNum,vertexArray};
        gpc_polygon gpcPolygon = {1,0,&gpcVertexList};

        gpc_tristrip result;
        gpc_polygon_to_tristrip(&gpcPolygon,&result);


        for(int i = 0 ; i < result.num_strips ; i++){
            gpc_vertex_list verList = result.strip[i];
            for(int j = 0 ; j < verList.num_vertices ; j++){
                gpc_vertex ver = verList.vertex[j];
                triResult.append(ver.x);
                triResult.append(ver.y);
            }
            triIndex.append(totalVertex);
            triIndex.append(verList.num_vertices);
            totalVertex += verList.num_vertices;
        }

        qDebug() << "The" << i << "triangulation finished" << "triResult size" << triResult.size();
    }

    geoType = 2;

}

void RenderLayer::selectFeature(float x, float y){
    selectVertex.clear();
    selectVertexIndex.clear();

    OGREnvelope envelope;
    envelope.Merge(x,y);
    qDebug() << "intersects?" << envelope.Intersects(boundary);
    if(envelope.Intersects(boundary)){
        int xLoc = (x - boundary.MinX) / dx ;
        int yLoc = (y - boundary.MinY) / dy ;
        int loc = xLoc + yLoc * indexGrid;
        qDebug() << "select location" << loc;
        QVector<OGRFeature*> grid = gridArray[loc];

        int polygonPointCount = 0;
        QString qstr;
        for(OGRFeature * feature : grid){
            // 如果field不存在，可能报ERROR 1:index -1 错误。Debug模式错误消失
            int fieldIndex = feature->GetFieldIndex("NAME");
            if(fieldIndex != -1){
                const char * str = feature->GetFieldAsString(fieldIndex);
                qstr += QString().fromLocal8Bit(str);
            }

            int featureType = feature->GetGeometryRef()->getGeometryType();
            if(featureType == wkbPoint){
                OGRPoint * point = (OGRPoint *)feature->GetGeometryRef();
                selectVertex.append(point->getX());
                selectVertex.append(point->getY());
            }
            else if(featureType == wkbPolygon){
                OGRPolygon * polygon = (OGRPolygon *)feature->GetGeometryRef();
                OGRLinearRing * ring = polygon->getExteriorRing();
                selectVertexIndex.append(polygonPointCount);
                selectVertexIndex.append(ring->getNumPoints());
                polygonPointCount += ring->getNumPoints();
                for(int i = 0; i < ring->getNumPoints(); i++){
                    selectVertex.append(ring->getX(i));
                    selectVertex.append(ring->getY(i));
                }
            }
            else if(featureType == wkbMultiPolygon){
                OGRMultiPolygon * mulPolygon = (OGRMultiPolygon *)feature->GetGeometryRef();
                for(int i = 0; i < mulPolygon->getNumGeometries(); i++){
                    OGRPolygon * polygon = (OGRPolygon *)mulPolygon->getGeometryRef(i);
                    OGRLinearRing * ring = polygon->getExteriorRing();
                    selectVertexIndex.append(polygonPointCount);
                    selectVertexIndex.append(ring->getNumPoints());
                    polygonPointCount += ring->getNumPoints();
                    // 两层循环的循环变量注意命名！！！
                    for(int j = 0; j < ring->getNumPoints(); j++){
                        selectVertex.append(ring->getX(j));
                        selectVertex.append(ring->getY(j));
                    }
                }
            }
        }

        //        this->window->textEdit->setText(QString(qstr));
        queryString = qstr;
    }

}

float RenderLayer::randF(){
    return qrand() % 100 / 100.f;
}

// 注意：static 成员变量的内存既不是在声明类时分配，也不是在创建对象时分配，而是在（类外）初始化时分配。
// 反过来说，没有在类外初始化的 static 成员变量不能使用。
Ui_OpenGLWindow * RenderLayer::window = nullptr;
