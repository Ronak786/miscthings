#-------------------------------------------------
#
# Project created by QtCreator 2018-06-25T09:07:57
#
#-------------------------------------------------

QT       -= gui

TARGET = pkglib
TEMPLATE = lib

DEFINES += PKGLIB_LIBRARY

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
    pkghandle.cpp \
    pkginfo.cpp \
    pkgutil.cpp \
    sigutil.cpp

HEADERS += \
        pkglib_global.h \ 
    debug.h \
    pkghandle.h \
    pkginfo.h \
    pkgutil.h \
    sigutil.h

QMAKE_INCDIR += /opt/openssl/include

unix {
    target.path = /usr/lib
    INSTALLS += target
}
