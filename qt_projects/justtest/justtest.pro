#-------------------------------------------------
#
# Project created by QtCreator 2018-06-29T13:05:13
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = justtest
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        main.cpp \
        mainwindow.cpp \
    customkey.cpp \
    mthread.cpp \
    worker.cpp

HEADERS += \
        mainwindow.h \
    customkey.h \
    mthread.h \
    worker.h \
    M3F20xm.h \
    Decoder.h \
    NDCode_Image_Recognition.h

FORMS += \
        mainwindow.ui

win32 {
INCLUDEPATH += $$PWD\include
#LIBS += $$PWD\libopencv_core300.dll $$PWD\libopencv_highgui300.dll $$PWD\libopencv_imgcodecs300.dll \
#            $$PWD\libopencv_imgproc300.dll  $$PWD\libopencv_video300.dll $$PWD\libopencv_videoio300.dll
LIBS += $$PWD\M3F20xm.lib $$PWD\opencv_ts300.lib $$PWD\opencv_world300.lib # this is for msvc
#LIBS += -L$$PWD -lM3F20xm -lopencv_ts300 -lopencv_world300 # this is for mingw32
}

unix {
INCLUDEPATH += /opt/opencvarm/include
LIBS += -L/opt/opencvarm/lib -lopencv_world -L$$PWD -lndcode_decoder -lndcode_hexagon_recognition
LIBS += -L/opt/openssl/lib/ -lcrypto
}
