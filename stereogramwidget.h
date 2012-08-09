// Copyright (c) 2011-2012 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.

#ifndef __VIDEOWIDGET_H_
#define __VIDEOWIDGET_H_

#include <QWidget>
#include <QImage>
#include <QVector>
#include <QPaintEvent>
#include <QHideEvent>
#include <QMouseEvent>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QList>
#include <QUrl>

#include "nui.h"
#include "depthdata.h"

class StereogramWidget : public QWidget
{
    Q_OBJECT
public:
    explicit StereogramWidget(QWidget* parent = NULL);
    QSize minimumSizeHint(void) const { return QSize(640, 480); }
    QSize sizeHint(void) const { return QSize(640, 480); }
    virtual int heightForWidth(int w) const { return 480 * w / 640; }

    QImage stereogram(void) const { return mStereogram; }
    void setRequestedStereogramSize(const QSize& requestedSize);
    const QSize& requestedStereogramSize(void) const { return mRequestedStereogramSize; }
    QImage stereogram(const QSize& requestedSize);

    typedef QVector<int> SameArrayType;

    const DepthData& originalDepthData(void) const { return mOriginalDepthData; }
    const DepthData& scaledDepthData(void) const { return mScaledDepthData; }

    enum TextureMode {
        TileTexture,
        StretchTexture,
        RandomColor,
        RandomBlackAndWhite
    };

protected:
    void paintEvent(QPaintEvent*);
    void hideEvent(QHideEvent*);
    void mouseDoubleClickEvent(QMouseEvent*);
    void dragEnterEvent(QDragEnterEvent*);
    void dragLeaveEvent(QDragLeaveEvent*);
    void dropEvent(QDropEvent*);

private:
    static const int MAX_SIRD_SIZE = 20480;
    QImage mFrame;
    QImage mTexture;
    QImage mPreliminaryTexture;
    QImage mStereogram;
    float mZScale;
    float mMu;
    float mEyeDist; /* inch */
    int mResolution; /* DPI */
    bool mSwapFrontBack;
    TextureMode mTextureMode;
    DepthData mOriginalDepthData;
    DepthData mScaledDepthData;
    QSize mRequestedStereogramSize;
    int mNearClipping;
    int mFarClipping;

private: // methods
    void makeSameArray(DepthData::DepthDataType& sameArr, DepthData::DepthDataType::const_iterator pDepth, float xDepthStep);
    void calcStereogram(void);
    void invalidateTexture(void) { mTexture = QImage(); }
    void makeTexture(void);

public slots:
    void setFrame(const QImage&);
    void setTexture(const QImage&);
    void setClipping(int, int);
    void setNearClipping(int);
    void setFarClipping(int);
    void setEyeDistance(float /* inch */);
    void setMu(int);
    void setResolution(int resolution);
    void setFrontBackSwap(bool doSwap);
    void setTextureMode(TextureMode);
    void setDepthData(const quint16*, const QSize&);

signals:
    void attaching(void);
    void detaching(void);
    void textureModeChanged(TextureMode);
};

#endif // __VIDEOWIDGET_H_
