#-------------------------------------------------
#
# Project created by QtCreator 2015-09-01T13:16:51
#
#-------------------------------------------------

QT       += core gui
QT       += sql
LIBS     += -L$$PWD/curl/lib -llibcurl
LIBS     += -L$$PWD/libvlc_lib -llibvlc -llibvlccore
INCLUDEPATH +=$$PWD/libvlc_include
DEPENDPATH  +=$$PWD/libvlc_include
INCLUDEPATH +=$$PWD/include
DEPENDPATH  +=$$PWD/include
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = DoorsInfo
TEMPLATE = app

CONFIG += c++11
CONFIG += warn_off
OBJECTS_DIR += obj
UI_DIR += forms
RCC_DIR += rcc
MOC_DIR += moc
DESTDIR += bin

SOURCES += main.cpp\
        DMachine.cpp \
    tablemodel.cpp \
    yqcdelegate.cpp \
    curlupload.cpp \
    mysqlquery.cpp \
    src/Audio.cpp \
    src/Common.cpp \
    src/ControlAudio.cpp \
    src/ControlVideo.cpp \
    src/Enums.cpp \
    src/Error.cpp \
    src/Instance.cpp \
    src/Media.cpp \
    src/MediaList.cpp \
    src/MediaListPlayer.cpp \
    src/MediaPlayer.cpp \
    src/MetaManager.cpp \
    src/Video.cpp \
    src/VideoDelegate.cpp \
    src/WidgetSeek.cpp \
    src/WidgetVideo.cpp \
    src/WidgetVolumeSlider.cpp

HEADERS  += DMachine.h \
    tablemodel.h \
    yqcdelegate.h \
    curlupload.h \
    mysqlquery.h \
    include/Audio.h \
    include/Common.h \
    include/Config.h \
    include/ControlAudio.h \
    include/ControlVideo.h \
    include/Enums.h \
    include/Error.h \
    include/Instance.h \
    include/Media.h \
    include/MediaList.h \
    include/MediaListPlayer.h \
    include/MediaPlayer.h \
    include/MetaManager.h \
    include/SharedExportCore.h \
    include/SharedExportWidgets.h \
    include/Video.h \
    include/VideoDelegate.h \
    include/WidgetSeek.h \
    include/WidgetVideo.h \
    include/WidgetVolumeSlider.h

FORMS    += DMachine.ui

RESOURCES += \
    resources.qrc
