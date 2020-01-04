#include "renderlayer.h"
#include "openglwindow.h"
#include "ui_openglwindow.h"

RenderLayer::~RenderLayer(){
    for(OGRFeature * feature : features){
        delete feature;
    }
    delete[] gridArray;
    delete layer;
}

RenderLayer::RenderLayer(OGRLayer * layer, int indexGrid):
    layer(layer),indexGrid(indexGrid),gridCount(indexGrid * indexGrid)
{
    // 1.创建索引，并进行相关初始化

    // 创建索引网格
    gridArray = new QVector<OGRFeature*> [indexGrid * indexGrid];
    envelopeArray = new OGREnvelope [indexGrid * indexGrid];
    // 获取图层MBR
    layer->GetExtent(&boundary);

    // 将边界四边各延伸1/100，解决点落在网格边界的BUG
    float boundaryWidth = boundary.MaxX - boundary.MinX;
    float boundaryHeight = boundary.MaxY - boundary.MinY;
    boundary.Merge(boundary.MinX - boundaryWidth / 100, boundary.MinY - boundaryHeight / 100);
    boundary.Merge(boundary.MaxX + boundaryWidth / 100, boundary.MaxY + boundaryHeight / 100);

    boundaryQ.setCoords(boundary.MinX,boundary.MinY,boundary.MaxX,boundary.MaxY);
    qDebug() << "boundaryQ" << boundaryQ;
    dx = (boundary.MaxX - boundary.MinX) / indexGrid;
    dy = (boundary.MaxY - boundary.MinY)/ indexGrid;

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

    // 2.读取要素

    layer->ResetReading();
    OGRFeature *feature;
    OGREnvelope envelope;
    while( (feature = layer->GetNextFeature()) != NULL ){

        rindex.Insert(feature);

        // 1.将要素根据外接矩形存入索引网格数组

        feature->GetGeometryRef()->getEnvelope(&envelope);
        for(int i = 0; i < gridCount; i++){
            if(envelope.Intersects(envelopeArray[i])){
                gridArray[i].append(feature);
            }
        }
        // 同时将所有的要素也存起来，避免再次通过layer创建新的对象
        features.append(feature);

        // 2.获取要素的顶点
        geoType = appendFeature(feature,vertexVector,vertexIndex);

    }

    if(geoType == 0){
        layerHeight = 0.1f;
    }

    RTreeVertex = rindex.Skeleton();

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

    vaoOrigin.create();
    vaoOrigin.bind();

    vboOrigin.create();
    vboOrigin.bind();
    vboOrigin.setUsagePattern(QOpenGLBuffer::StaticDraw);
    vboOrigin.allocate(vertexVector.constData() , vertexVector.size() * sizeof(float));
    shader->enableAttributeArray(0);
    shader->setAttributeBuffer(0, GL_FLOAT, 0, 2, 2 * sizeof(float));

    if(geoType == 2){
        vaoTriangular.create();
        vaoTriangular.bind();

        vboTriangular.create();
        vboTriangular.bind();
        vboTriangular.setUsagePattern(QOpenGLBuffer::StaticDraw);
        vboTriangular.allocate(triResult.constData() , triResult.size() * sizeof(float));
        shader->enableAttributeArray(0);
        shader->setAttributeBuffer(0, GL_FLOAT, 0, 2, 2 * sizeof(float));
    }

    // second 和 select看清楚。最好改命名，规范一下。
    vaoSelect.create();
    vaoSelect.bind();

    vboSelect.create();
    vboSelect.bind();
    vboSelect.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    shader->enableAttributeArray(0);
    shader->setAttributeBuffer(0, GL_FLOAT, 0, 2, 2 * sizeof(float));

    vaoDensity.create();
    vaoDensity.bind();

    vboDensity.create();
    vboDensity.bind();
    vboDensity.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    shader->enableAttributeArray(0);
    shader->setAttributeBuffer(0, GL_FLOAT, 0, 3, 3 * sizeof(float));

    vaoRTree.create();
    vaoRTree.bind();

    vboRTree.create();
    vboRTree.bind();
    vboRTree.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    vboRTree.allocate(RTreeVertex.constData(), RTreeVertex.size() * sizeof(float));
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
        vaoTriangular.bind();
        shader->setUniformValue(shader->uniformLocation("ourColor")
                                ,.0f, .8f, .0f, 1.0f);
        for(int i = 0; i < triIndex.size() / 2; i++){
            glFuncs->glDrawArrays(GL_TRIANGLE_STRIP,triIndex.at(2 * i),triIndex.at(2 * i + 1));
        }

    }

    // 2. 画点、线

    vaoOrigin.bind();
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
    // 在透视下启用景深后，被前面的覆盖看不到。当不画原始点时看得到。

    if(selectVertex.size() != 0){
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

    // 4. 画点密度

    if(densityVertex.size() != 0){
        vaoDensity.bind();
        vboDensity.bind();
        vboDensity.allocate(densityVertex.constData(), densityVertex.size() * sizeof(float));
        glFuncs->glDrawArrays(GL_POINTS,0,densityVertex.size()/3);
    }

    // 5.RTree
    for(int i = 0; i < RTreeVertex.count() / 2 / 4; i++){
        vaoRTree.bind();
        vaoRTree.bind();
        glFuncs->glDrawArrays(GL_LINE_LOOP, i*4, 4);
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
void RenderLayer::selectFeature(float x1, float y1){
    selectFeature(x1,y1,x1,y1);
}

void RenderLayer::selectFeature(float x1, float y1, float x2, float y2){
    selectVertex.clear();
    selectVertexIndex.clear();

    OGREnvelope envelope;
    envelope.Merge(x1,y1);
    envelope.Merge(x2,y2);
    qDebug() << "intersects?" << envelope.Intersects(boundary);
    if(envelope.Intersects(boundary)){
        int xLoc = ((x1+x2)/2 - boundary.MinX) / dx ;
        int yLoc = ((y1+y2)/2 - boundary.MinY) / dy ;
        int loc = xLoc + yLoc * indexGrid;
        qDebug() << "select location" << loc;
        QVector<OGRFeature*> grid = gridArray[loc];

        QString qstr;
        for(int i = 0; i < layer->GetLayerDefn()->GetFieldCount(); i++){
            const char * str = layer->GetLayerDefn()->GetFieldDefn(i)->GetNameRef();
            qstr += QString().fromLocal8Bit(str);
            qstr += " ";
        }
        qstr += "\n";

        OGREnvelope featureEnvelope;
        for(OGRFeature * feature : grid){
            // 判断是否和鼠标范围相交
            feature->GetGeometryRef()->getEnvelope(&featureEnvelope);
            if(concreteSelect && !envelope.Intersects(featureEnvelope)){
                continue;
            }

            // 如果field不存在，可能报ERROR 1:index -1 错误。Debug模式错误消失
            //            int fieldIndex = feature->GetFieldIndex("NAME");
            for(int i = 0; i < feature->GetFieldCount(); i++){
                feature->GetFieldDefnRef(0)->GetNameRef();
                const char * str = feature->GetFieldAsString(i);
                qstr += QString().fromLocal8Bit(str);
                qstr += " ";
            }
            qstr += "\n";

            appendFeature(feature, selectVertex, selectVertexIndex);
        }

        queryString = qstr;
        this->window->displaySelection();

        // R树测试
        QVector<OGRFeature*> rresult;
        QVector<RTreeNode*> nodes;
        rindex.root->Search(envelope,rresult,nodes);

        RTreeVertex.clear();
        for(RTreeNode * node : nodes){
//            OGREnvelope * nodeEnvelope = &(node->envelope);

            RTreeVertex.append(node->envelope.MinX);
            RTreeVertex.append(node->envelope.MinY);

            RTreeVertex.append(node->envelope.MinX);
            RTreeVertex.append(node->envelope.MaxY);

            RTreeVertex.append(node->envelope.MaxX);
            RTreeVertex.append(node->envelope.MaxY);

            RTreeVertex.append(node->envelope.MaxX);
            RTreeVertex.append(node->envelope.MinY);
        }
        vboRTree.bind();
        vboRTree.allocate(RTreeVertex.constData(),sizeof(float) * RTreeVertex.size());

    }

}

void RenderLayer::calculateDensity(float radius, float resolution){
    if(geoType != 0){
        return;
    }
    densityVertex.clear();
    QVector<float> densityData;
    //    OGRSpatialReference  wgs84;
    //    wgs84.SetWellKnownGeogCS("WGS84");
    // 首先获取边界距离，进行拓展

    // 最短边像素数量
    float minPiexl = resolution;
    // 每个像素距离
    float disPiexl;
    if(boundaryQ.width() > boundaryQ.height()){
        disPiexl = (boundaryQ.height() + 2*radius ) / minPiexl;
    }
    else{
        disPiexl = (boundaryQ.width() + 2*radius ) / minPiexl;
    }
    // 对应方向像素数量。注意这里应该是int！
    // 10 < 10 和 10 < 10.1 ，真假不同
    int widthPiexl = (boundaryQ.width() + 2*radius ) / disPiexl;
    int heightPiexl = (boundaryQ.height() + 2*radius ) / disPiexl;

    // 注意进行初始化！
    float maxDensity = 0;
    // 计算每一点的估计值
    for(int i = 0; i < heightPiexl; i++){
        for(int j = 0; j < widthPiexl; j++){
            float pointX = boundary.MinX - radius + disPiexl * j;
            float pointY = boundary.MinY - radius + disPiexl * i;
            float density = 0;
            OGRPoint pointR(pointX,pointY);
            //            pointR.assignSpatialReference(&wgs84);
            for(int k = 0; k < features.size(); k++){
                OGRPoint * pointF = (OGRPoint*)features.at(k)->GetGeometryRef();
                //                pointF->assignSpatialReference(&wgs84);
                //                float distance = pointR.Distance(pointF);
                float distance = sqrt(
                            pow((pointR.getX() - pointF->getX()),2)
                            + pow((pointR.getY() - pointF->getY()),2)
                            );
                if(distance > radius){
                    continue;
                }
                density += (3 / M_PI) * pow(1 - pow((distance / radius),2) ,2);
            }
            density = density / (radius * radius) ;

            // 保存最大值
            if(density > maxDensity){
                maxDensity = density;
            }
            // 画点时不画值为0的点
            if(density != 0){
                densityVertex.append(pointX);
                densityVertex.append(pointY);
                densityVertex.append(density);
            }
            // 画图数据
            densityData.append(density);

        }
        qDebug() << "density:" << i << densityVertex.size();
    }

    qDebug() << maxDensity;

    // 计算从下到上，图片从上到下，需要反转
    QVector<float> reverse;
    for(int i = 0; i < heightPiexl; i++){
        for(int j = 0; j < widthPiexl; j++){
            reverse.append(densityData.at(
                               (heightPiexl-i-1) * widthPiexl + j
                               ));
        }
    }
    // 导出tiff
    exportDensity(boundary.MinX - radius,
                  boundary.MaxY + radius,
                  disPiexl,disPiexl,
                  widthPiexl,heightPiexl,
                  reverse);

    // 变到0-1
    for(int i = 0; i < densityVertex.size() / 3; i++){
        densityVertex[i*3 + 2] = densityVertex.at(i*3 + 2) / maxDensity;
    }
}

void RenderLayer::exportDensity(float x,float y,float dx,float dy,int xCount,int yCount,QVector<float> data){
    // 定义地理信息
    double adfGeoTransform[6] = { x, dx, 0, y, 0, -dy };
    // 定义投影信息
    char *pszSRS_WKT = NULL;
    this->layer->GetSpatialRef()->exportToWkt(&pszSRS_WKT);
    // 获取Driver
    const char *pszFormat = "GTiff";
    GDALDriver *poDriver;
//    char **papszMetadata;
    poDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
    if( poDriver == NULL ){
        exit( 1 );
    }
    // 创建文件
    GDALDataset *poDstDS;
    char **papszOptions = NULL;
    poDstDS = poDriver->Create("output.tiff",xCount+100,yCount,1,GDT_Float32,papszOptions);
    // 设置元数据
    poDstDS->SetGeoTransform( adfGeoTransform );
    poDstDS->SetProjection(pszSRS_WKT);
    // 写入数据
    GDALRasterBand * band = poDstDS->GetBands()[0];
    band->RasterIO( GF_Write, 0, 0, xCount, yCount,
                    (void *)data.constData(), xCount, yCount, GDT_Float32, 0, 0 );
    // 关闭文件
    GDALClose( (GDALDatasetH) poDstDS );
}

int RenderLayer::appendFeature(OGRFeature * feature, QVector<float> & vertexVector, QVector<int> & vertexIndex){
    int featureType = feature->GetGeometryRef()->getGeometryType();
    if(featureType == wkbPoint){
        OGRPoint * point = (OGRPoint *)feature->GetGeometryRef();
        vertexVector.append(point->getX());
        vertexVector.append(point->getY());
        return 0;
    }
    else if(featureType == wkbPolygon){
        OGRPolygon * polygon = (OGRPolygon *)feature->GetGeometryRef();
        OGRLinearRing * ring = polygon->getExteriorRing();

        if(vertexIndex.size() == 0){
            vertexIndex.append(0);
        }
        else{
            int pointCount = vertexIndex.at(vertexIndex.size()-2) + vertexIndex.at(vertexIndex.size()-1);
            vertexIndex.append(pointCount);
        }
        vertexIndex.append(ring->getNumPoints());

        for(int i = 0; i < ring->getNumPoints(); i++){
            vertexVector.append(ring->getX(i));
            vertexVector.append(ring->getY(i));
        }

        return 1;
    }
    else if(featureType == wkbMultiPolygon){
        OGRMultiPolygon * mulPolygon = (OGRMultiPolygon *)feature->GetGeometryRef();
        for(int i = 0; i < mulPolygon->getNumGeometries(); i++){
            OGRPolygon * polygon = (OGRPolygon *)mulPolygon->getGeometryRef(i);
            OGRLinearRing * ring = polygon->getExteriorRing();

            if(vertexIndex.size() == 0){
                vertexIndex.append(0);
            }
            else{
                int pointCount = vertexIndex.at(vertexIndex.size()-2) + vertexIndex.at(vertexIndex.size()-1);
                vertexIndex.append(pointCount);
            }
            vertexIndex.append(ring->getNumPoints());

            // 两层循环的循环变量注意命名！！！
            for(int j = 0; j < ring->getNumPoints(); j++){
                vertexVector.append(ring->getX(j));
                vertexVector.append(ring->getY(j));
            }
        }
        return 1;
    }
}

float RenderLayer::randF(){
    return qrand() % 100 / 100.f;
}

// 注意：static 成员变量的内存既不是在声明类时分配，也不是在创建对象时分配，而是在（类外）初始化时分配。
// 反过来说，没有在类外初始化的 static 成员变量不能使用。
Ui_OpenGLWindow * RenderLayer::windowUi = nullptr;
OpenGLWindow * RenderLayer::window = nullptr;
