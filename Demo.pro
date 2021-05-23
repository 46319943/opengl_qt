#-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -
#

#Project created by QtCreator 2019 - 11 - 21T18 : 20 : 50
#

#-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -

QT += core gui

    greaterThan(QT_MAJOR_VERSION, 4)
    : QT += widgets

          TARGET = Demo TEMPLATE = app

#The following define makes your compiler emit warnings if you use
#any feature of Qt which as been marked as deprecated(the exact warnings
#depend on your compiler).Please consult the documentation of the
#deprecated API in order to know how to port your code away from it.
              DEFINES += QT_DEPRECATED_WARNINGS

#You can also make your code fail to compile if you use deprecated APIs.
#In order to do so, uncomment the following line.
#You can also select to disable deprecated APIs only up to a certain version \
    of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE = \
    0x060000 #disables all the APIs deprecated before Qt 6.0.0

                  SOURCES +=
      main.cpp mainwindow.cpp openglwindow.cpp openglwidget.cpp renderlayer
          .cpp gpc.cpp main.cpp rtreeindex.cpp utils.cpp

              HEADERS +=
      mainwindow.h../ build - Demo - Desktop_Qt_5_9_0_MSVC2015_32bit -
      Debug / ui_mainwindow.h openglwindow.h openglwidget.h renderlayer.h gpc
                  .h gpc.h rtreeindex.h utils.h

                      FORMS += mainwindow.ui openglwindow.ui
#添加第三方库头文件
                                   INCLUDEPATH += D :

    SoftwareInstall gdal

        include
#添加第三方库静态库
            LIBS += D :\SoftwareInstall\gdal\lib\gdal_i.lib

#glPolygonMode调用支持
                LIBS += -lopengl32 CONFIG += c++ 11 LIBS += -lopengl32 LIBS +=
    -lglu32
#LIBS += -lglut32

        RESOURCES += resources.qrc
