# Copyright (c) 2012 Oliver Lau <oliver@von-und-fuer-lau.de>
# All rights reserved.

QT += core gui opengl multimedia

TARGET = Bineqt
TEMPLATE = app

INCLUDEPATH += "C:/Program Files/Microsoft SDKs/Kinect/v1.5/inc" \
    "C:/Program Files (x86)/Microsoft SDKs/Kinect/v1.5/inc"

QMAKE_CXXFLAGS += /openmp

LIBS += /LIBPATH:"C:/Program Files/Microsoft SDKs/Kinect/v1.5/lib/x86" \
    /LIBPATH:"C:/Program Files (x86)/Microsoft SDKs/Kinect/v1.5/lib/amd64" \
    Kinect10.lib \
    OleAut32.lib \
    Dmoguids.lib

SOURCES += main.cpp\
    mainwindow.cpp \
    nuithread.cpp \
    nui.cpp \
    depthimagewidget.cpp \
    stereogramwidget.cpp \
    stereogramsaveform.cpp \
    depthdata.cpp

HEADERS += mainwindow.h \
    nuithread.h \
    nui.h \
    depthimagewidget.h \
    stereogramwidget.h \
    stereogramsaveform.h \
    depthdata.h

FORMS += mainwindow.ui \
    stereogramsaveform.ui

RESOURCES += \
    Textures.qrc \
    Images.qrc

OTHER_FILES += \
    Bineqt.rc

RC_FILE = Bineqt.rc






















