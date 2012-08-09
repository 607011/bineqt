// Copyright (c) 2012 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.
// $Id: depthimagewidget.h 9a383954a478 2012/08/08 14:55:58 Oliver Lau <oliver@von-und-fuer-lau.de> $

#ifndef __DEPTHIMAGEWIDGET_H_
#define __DEPTHIMAGEWIDGET_H_

#include <QWidget>
#include <QImage>
#include <QTime>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QList>
#include <QUrl>
#include <QPoint>

#include "nui.h"

class DepthImageWidget : public QWidget
{
    Q_OBJECT

public:
    static const int WIDTH = 640;
    static const int HEIGHT = 480;
    static const QRgb COLOR_NO_DEPTH = 0xfffb96ffu;
    static const QRgb COLOR_TOO_FAR = 0xff8cffdcu;
    static const QRgb COLOR_NEAR_CLIPPING = 0xffe6be00u;
    static const QRgb COLOR_FAR_CLIPPING = 0xff0d0a79u;

    explicit DepthImageWidget(QWidget* parent = NULL);
    QSize minimumSizeHint(void) const { return QSize(WIDTH, HEIGHT); }
    QSize sizeHint(void) const { return QSize(WIDTH, HEIGHT); }
    virtual int heightForWidth(int w) const { return HEIGHT * w / WIDTH; }
    const QImage& depthImage(void) const { return mDepthFrame; }
    void resetSelection(void);


protected:
    void paintEvent(QPaintEvent*);
    void mouseMoveEvent(QMouseEvent*);
    void mousePressEvent(QMouseEvent*);
    void mouseReleaseEvent(QMouseEvent*);
    void mouseDoubleClickEvent(QMouseEvent*);
    void dragEnterEvent(QDragEnterEvent*);
    void dragLeaveEvent(QDragLeaveEvent*);
    void dropEvent(QDropEvent*);

private:
    QImage mWebcamFrame;
    QImage mDepthFrame;
    int mNearClipping;
    int mFarClipping;
    int mFrameCount;
    QTime mLastFrameTime;
    int mCurrentFrameRate;
    qreal mOverlayFrameOpacity;
    bool mLeftButtonPressed;
    QPoint mMouseStartPos;
    QPoint mMouseCurrentPos;
    bool mFitFrameIntoDepthFrame;
    const quint16* mDepthData;

signals:
    void selectionChanged(QImage);
    void stopStreaming(void);
    void depthDataReady(const quint16*, const QSize&);
    void clippingChanged(int, int);

public slots:
    void setFrame(const QImage&);
    void setDepthData(const quint16* data, const QSize& frameSize);
    void setNearClipping(int);
    void setFarClipping(int);
    void setOverlayFrameOpacity(int);
    void setFitFrameIntoDepthFrame(bool doFit);

};

#endif // __DEPTHIMAGEWIDGET_H_
