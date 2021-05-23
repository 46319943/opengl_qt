#include "mainwindow.h"

#include "openglwindow.h"
#include "ui_mainwindow.h"

// 引入GDAL头文件
#include "ogrsf_frmts.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::on_pushButton_clicked() {
  // 1.打开文件管理器，选择文件

  QString curPath = QDir::currentPath();  //获取系统当前目录
  QString dlgTitle = "Open a File";       //对话框标题
  QString filter =
      "Program File(*.h *.cpp);;txt File(*.txt);;All File(*.*)";  //文件过滤器
  QString aFileName =
      QFileDialog::getOpenFileName(this, dlgTitle, curPath, filter);
  qDebug() << "Chosen File Path: " << aFileName;
  // C++中不存在对象为null，对象一定是对应内存空间的。
  // Java中的对象为空，对应C++中指针为空，即不指向任何对象。
  if (aFileName.isEmpty()) {
    qDebug() << "Cancel File Choose";
    return;
  }
  ui->label_file_input->setText(aFileName);
  // 设置标签宽度
  // 进行一次对象拷贝
  QRect rect = ui->label_file_input->geometry();
  rect.setWidth(aFileName.length() * 6);
  ui->label_file_input->setGeometry(rect);

  // 2.根据文件路径读取文件

  // 不能处理中文路径，比较麻烦，放弃
  std::string fileNameStr = aFileName.toStdString();
  const char *fileName = fileNameStr.data();
  // 二进制读写，对换行处理不同。
  // 这里影响不大，但从逻辑上，应该是二进制
  FILE *file = fopen(fileName, "rb");

  qDebug() << "Open File and Read";
  std::map<int, char> int_sort_map;
  char value;
  int index;
  QDebug debug = qDebug();
  // 最后一个数据单元会被读两遍：读一遍之后，才检测到结尾
  while (!feof(file)) {
    fread(&value, sizeof(value), 1, file);
    fread(&index, sizeof(index), 1, file);
    int_sort_map[index] = value;

    // QDebug输出问题？
    debug << int(value);
    debug << index << int(value);
  }
  fclose(file);

  // 将内容显示在文本框
  QString text;
  for (std::map<int, char>::iterator iter = int_sort_map.begin();
       iter != int_sort_map.end(); ++iter) {
    text += (*iter).second;
  }
  ui->textEdit->setPlainText(text);

  // 最后输出的全是\0
  //    debug = qDebug();
  //    debug << endl << endl;
  //    for (std::map<int, char>::iterator iter = int_sort_map.begin();
  //         iter != int_sort_map.end();
  //         ++iter) {
  //        debug << '[' << (*iter).first << ':' << int((*iter).second) << ']';
  //    }

  // 3.根据文件路径，写入相应的.txt文件

  aFileName += ".txt";
  file = fopen(aFileName.toStdString().data(), "w");
  for (std::map<int, char>::iterator iter = int_sort_map.begin();
       iter != int_sort_map.end(); ++iter) {
    fwrite(&(*iter).second, sizeof((*iter).second), 1, file);
  }
  fclose(file);
}

void MainWindow::on_actionClose_triggered() {}

void MainWindow::on_actionOpenGL_triggered() {}

void MainWindow::on_action_Shp_triggered() {
  // registers all format drivers built into GDAL/OGR.
  GDALAllRegister();
  // Datasources can be files, RDBMSes, directories full of files,
  // or even remote web services depending on the driver being used
  GDALDataset *poDS;
  // 中文乱码
  CPLSetConfigOption("SHAPE_ENCODING", "");

  poDS = (GDALDataset *)GDALOpenEx("D:/Document/Qt/ConsoleDemo/school.shp",
                                   GDAL_OF_VECTOR, NULL, NULL, NULL);
  if (poDS == NULL) {
    printf("Open failed.\n");
    exit(1);
  }
  // 数据集包含多个图层
  OGRLayer *poLayer;

  poLayer = poDS->GetLayer(0);
  // 左值引用和右值引用都不报错？
  for (auto &poFeature : poLayer) {
    for (auto &&oField : poFeature) {
      printf("%s,", oField.GetAsString());
    }
  }
}
