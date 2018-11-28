#-------------------------------------------------
#
# Project created by QtCreator 2018-05-30T14:43:42
#
#-------------------------------------------------

#QT       += core gui network multimedia multimediawidgets
QT       += core gui network serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = KivviLauncher
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
    device/devicebattery.cpp \
    device/devicemobilesignal.cpp \
    device/deviceqrcode.cpp \
    device/devicesysteminfo.cpp \
    device/devicesystemtime.cpp \
    device/devicewifisignal.cpp \
    widget/widgetstatusbar.cpp \
    system_setting/dialogpassword.cpp \
    system_setting/dialogsystemsetting.cpp \
    system_setting/dialogfactoryreset.cpp \
    system_setting/dialogclocksetting.cpp \
    system_setting/dialogsysteminfo.cpp \
    device/devicewifi.cpp \
    system_setting/dialogsystemupdate.cpp \
    device_test/dialogdevicetest.cpp \
    device_test/dialogkeypadtest.cpp \
    device_test/dialogqrcodetest.cpp \
    device_test/dialoglcdtest.cpp \
    device_test/dialogbatterytest.cpp \
    device_test/dialogcameratest.cpp \
    device_test/dialogwlantest.cpp \
    device_test/dialoggpstest.cpp \
    device_test/dialogledtest.cpp \
    device_test/dialogltetest.cpp \
    device_test/dialogbttest.cpp \
    device/devicegps.cpp \
    device/deviceuart.cpp \
    device/deviceled.cpp \
    device_test/dialogaudiotest.cpp \
    device_test/dialograser.cpp \
    device_test/dialogcamera.cpp \
    device_test/worker.cpp \
    device_test/passdialog.cpp \
    device_test/mylistwidget.cpp

HEADERS += \
        mainwindow.h \
    device/devicebattery.h \
    device/devicemobilesignal.h \
    device/deviceqrcode.h \
    device/devicesysteminfo.h \
    device/devicesystemtime.h \
    device/devicewifisignal.h \
    widget/widgetstatusbar.h \
    system_setting/dialogpassword.h \
    system_setting/dialogsystemsetting.h \
    system_setting/dialogfactoryreset.h \
    system_setting/dialogclocksetting.h \
    system_setting/dialogsysteminfo.h \
    device/devicewifi.h \
    system_setting/dialogsystemupdate.h \
    device_test/dialogdevicetest.h \
    device_test/dialogkeypadtest.h \
    device_test/dialogqrcodetest.h \
    device_test/dialoglcdtest.h \
    device_test/dialogbatterytest.h \
    device_test/dialogcameratest.h \
    device_test/dialogwlantest.h \
    device_test/dialoggpstest.h \
    device_test/dialogledtest.h \
    device_test/dialogltetest.h \
    device_test/dialogbttest.h \
    device/devicegps.h \
    device/deviceuart.h \
    device/deviceled.h \
    device_test/dialogaudiotest.h \
    device_test/dialograser.h \
    device_test/dialogcamera.h \
    third_part/include/Decoder.h \
    third_part/include/NDCode_Image_Recognition.h \
    device_test/worker.h \
    device_test/passdialog.h \
    device_test/mylistwidget.h

FORMS += \
        mainwindow.ui \
    widget/widgetstatusbar.ui \
    system_setting/dialogpassword.ui \
    system_setting/dialogsystemsetting.ui \
    system_setting/dialogfactoryreset.ui \
    system_setting/dialogclocksetting.ui \
    system_setting/dialogsysteminfo.ui \
    system_setting/dialogsystemupdate.ui \
    device_test/dialogdevicetest.ui \
    device_test/dialogkeypadtest.ui \
    device_test/dialogqrcodetest.ui \
    device_test/dialoglcdtest.ui \
    device_test/dialogbatterytest.ui \
    device_test/dialogcameratest.ui \
    device_test/dialogwlantest.ui \
    device_test/dialoggpstest.ui \
    device_test/dialogledtest.ui \
    device_test/dialogltetest.ui \
    device_test/dialogbttest.ui \
    device_test/dialogaudiotest.ui \
    device_test/dialograser.ui \
    device_test/dialogcamera.ui \
    device_test/passdialog.ui

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/third_part/lib/release/ -lqrencode
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/third_part/lib/debug/ -lqrencode
else:unix: LIBS += -L$$PWD/third_part/lib/ -lqrencode

INCLUDEPATH += $$PWD/third_part/include
DEPENDPATH += $$PWD/third_part/include

DISTFILES +=

RESOURCES += \
    resources.qrc

#win32:CONFIG(release, debug|release): LIBS += -L$$PWD/third_part/lib/release/ -liputil
#else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/third_part/lib/debug/ -liputil
#else:unix: LIBS += -L$$PWD/third_part/lib/ -liputil

INCLUDEPATH += $$PWD/third_part/include
DEPENDPATH += $$PWD/third_part/include

unix {
INCLUDEPATH += /opt/opencvarm/include
LIBS += -L/opt/opencvarm/lib -lopencv_world -L$$PWD -lndcode_decoder -lndcode_hexagon_recognition
LIBS += -L/opt/openssl/lib/ -lcrypto
}
