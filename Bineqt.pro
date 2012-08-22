# Copyright (c) 2012 Oliver Lau <oliver@von-und-fuer-lau.de>
# All rights reserved.

QT += core gui opengl multimedia network qt3support

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
    depthdata.cpp \
    smtp/src/smtpclient.cpp \
    smtp/src/quotedprintable.cpp \
    smtp/src/mimetext.cpp \
    smtp/src/mimepart.cpp \
    smtp/src/mimemultipart.cpp \
    smtp/src/mimemessage.cpp \
    smtp/src/mimeinlinefile.cpp \
    smtp/src/mimehtml.cpp \
    smtp/src/mimefile.cpp \
    smtp/src/mimecontentformatter.cpp \
    smtp/src/mimeattachment.cpp \
    smtp/src/emailaddress.cpp \
    mailaddressdialog.cpp

HEADERS += mainwindow.h \
    nuithread.h \
    nui.h \
    depthimagewidget.h \
    stereogramwidget.h \
    stereogramsaveform.h \
    depthdata.h \
    smtp/src/SmtpMime \
    smtp/src/smtpclient.h \
    smtp/src/quotedprintable.h \
    smtp/src/mimetext.h \
    smtp/src/mimepart.h \
    smtp/src/mimemultipart.h \
    smtp/src/mimemessage.h \
    smtp/src/mimeinlinefile.h \
    smtp/src/mimehtml.h \
    smtp/src/mimefile.h \
    smtp/src/mimecontentformatter.h \
    smtp/src/mimeattachment.h \
    smtp/src/emailaddress.h \
    mailaddressdialog.h

FORMS += mainwindow.ui \
    stereogramsaveform.ui \
    mailaddressdialog.ui

RESOURCES += \
    Textures.qrc \
    Images.qrc

OTHER_FILES += \
    Bineqt.rc \
    ifa.ini

RC_FILE = Bineqt.rc






































