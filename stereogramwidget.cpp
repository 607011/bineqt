// Copyright (c) 2011-2012 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.

#include <QPainter>
#include <QtCore/QtDebug>
#include <QTime>
#include <QtConcurrentMap>
#include <QtConcurrentRun>
#include <QDateTime>
#include <QRegExp>

#define _USE_MATH_DEFINES
#include <qmath.h>

#include "stereogramwidget.h"
#include "nui.h"


static const float NUI_IMAGE_DEPTH_RANGE = NUI_IMAGE_DEPTH_MAXIMUM - NUI_IMAGE_DEPTH_MINIMUM;


StereogramWidget::StereogramWidget(QWidget* parent)
    : QWidget(parent)
    , mZScale(1.0f / 256.0f)
    , mMu(1.0f / 3.0f)
    , mEyeDist(2.64f) /* inch */
    , mResolution(94) /* dpi */
    , mTextureMode(TileTexture)
    , mNearClipping(NUI_IMAGE_DEPTH_MINIMUM)
    , mFarClipping(NUI_IMAGE_DEPTH_MAXIMUM)
{
    setMinimumSize(640, 480);
    QSizePolicy sizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    sizePolicy.setHeightForWidth(true);
    setSizePolicy(sizePolicy);
    setAcceptDrops(true);
    qsrand(QDateTime::currentDateTime().toTime_t());
}


inline void StereogramWidget::makeSameArray(DepthData::DepthDataType& sameArr, DepthData::DepthDataType::const_iterator pDepth, float xDepthStep)
{
    for (int x = 0; x < sameArr.size(); ++x)
        sameArr[x] = x;
    const float E = mEyeDist * mResolution;
    const float ft = 2.0f / (mZScale * mMu * E);
    float depx = 0;
    int xdo = 0;
    int xd = 0;
    SameArrayType::const_iterator p = pDepth;
    for (int x = 0; x < sameArr.size(); ++x) {
        const int Zorg = *p;
        const float Z = Zorg * mZScale;
        const int s = qRound(E * (1 - mMu * Z) / (2 - mMu * Z));
        int left = x - s / 2;
        int right = x + s / 2;
        if (left >= 0 && right < sameArr.size()) {
            int t = 1;
            int zt;
            bool visible;
            do {
                zt = Zorg + qRound((2 - mMu * Z) * t * ft);
                const int ts = qRound(t * xDepthStep);
                SameArrayType::const_iterator ph = p - ts;
                visible = *ph < zt;
                if (visible) {
                    ph = p + ts;
                    visible = *ph < zt;
                }
                ++t;
            }
            while (visible && mZScale > zt);
            if (visible) {
                int l = sameArr[left];
                while (l != left && l != right) {
                    if (l < right) {
                        left = l;
                        l = sameArr[left];
                    }
                    else {
                        sameArr[left] = right;
                        left = right;
                        l = sameArr[left];
                        right = l;
                    }
                }
                sameArr[left] = right;
            }
        }
        depx = depx + xDepthStep;
        xd = qRound(depx);
        p += xd - xdo;
        xdo = xd;
    }
}


void StereogramWidget::paintEvent(QPaintEvent*)
{
    calcStereogram();
    if (mStereogram.isNull())
        return;
    QPainter painter(this);
    const QImage& stereogram = mStereogram.scaled(size(), Qt::KeepAspectRatio);
    const float aspectRatioStereogram = (float)stereogram.width() / stereogram.height();
    const float aspectRatioWidget = (float)width() / height();
    int x0 = 0, y0 = 0;
    if (aspectRatioStereogram < aspectRatioWidget)
        x0 = (width() - stereogram.width()) / 2;
    else
        y0 = (height() - stereogram.height()) / 2;
    painter.drawImage(x0, y0, stereogram);
}


void StereogramWidget::setFrame(const QImage& frame)
{
    mFrame = frame;
    update();
}


inline void StereogramWidget::makeTexture(void)
{
    if (!mTexture.isNull() || !mOriginalDepthData.isValid() || !mRequestedStereogramSize.isValid())
        return;
    switch (mTextureMode) {
    case TileTexture:
    {
        if (mPreliminaryTexture.isNull())
            return;
        mTexture = QImage(mPreliminaryTexture.width(), mRequestedStereogramSize.height(), QImage::Format_RGB32);
        QPainter painter(&mTexture);
        const int numTiles = 1 + mRequestedStereogramSize.height() / mPreliminaryTexture.height();
        for (int i = 0; i < numTiles; ++i)
            painter.drawImage(0, i * mPreliminaryTexture.height(), mPreliminaryTexture);
        break;
    }
    case StretchTexture:
    {
        if (mPreliminaryTexture.isNull())
            return;
        mTexture = mPreliminaryTexture.scaled(mRequestedStereogramSize);
        break;
    }
    case RandomColor:
    {
        const int N = mRequestedStereogramSize.width() * mRequestedStereogramSize.height();
        mTexture = QImage(mRequestedStereogramSize, QImage::Format_RGB32);
        QRgb* d = reinterpret_cast<QRgb*>(mTexture.bits());
        for (int i = 0; i < N; ++i)
            *d++ = qRgb(qrand() % 255, qrand() % 255, qrand() % 255);
        break;
    }
    case RandomBlackAndWhite:
    {
        const int N = mRequestedStereogramSize.width() * mRequestedStereogramSize.height();
        mTexture = QImage(mRequestedStereogramSize, QImage::Format_RGB32);
        QRgb* d = reinterpret_cast<QRgb*>(mTexture.bits());
        for (int i = 0; i < N; ++i) {
            const int b = (qrand() > RAND_MAX/2)? 0 : 255;
            *d++ = qRgb(b, b, b);
        }
        break;
    }
    default:
    {
        qWarning() << "invalid texture mode: " << mTextureMode;
        break;
    }
    }
}


void StereogramWidget::calcStereogram(void)
{
    makeTexture();
    mScaledDepthData = mOriginalDepthData.scaled(mRequestedStereogramSize);
    mStereogram = QImage(mRequestedStereogramSize, QImage::Format_RGB32);
    if (mTexture.isNull())
        return;
    for (int y = 0; y < mScaledDepthData.size().height(); ++y) {
        DepthData::DepthDataType sameArr(mStereogram.width());
        DepthData::DepthDataType::const_iterator data = y * mScaledDepthData.size().width() + mScaledDepthData.data().begin();
        makeSameArray(sameArr, data, mScaledDepthData.size().width() / mStereogram.width());
        QRgb* const dst = reinterpret_cast<QRgb*>(mStereogram.scanLine(y));
        const QRgb* const src = reinterpret_cast<QRgb*>(mTexture.scanLine(y));
        int x = mStereogram.width();
        while (x--) {
            const int same = sameArr[x];
            dst[x] = (same == x)
                    ? src[same % mTexture.width()]
                    : dst[same];
        }
    }
}


QImage StereogramWidget::stereogram(const QSize& requestedSize)
{
    if (requestedSize == mStereogram.size())
        return mStereogram;
    DepthData oldScaledDepthData = mScaledDepthData;
    QSize oldRequestedStereogramSize = mRequestedStereogramSize;
    mRequestedStereogramSize = requestedSize;
    invalidateTexture();
    calcStereogram();
    mRequestedStereogramSize = oldRequestedStereogramSize;
    mScaledDepthData = oldScaledDepthData;
    QImage scaledStereogram = mStereogram;
    invalidateTexture();
    calcStereogram();
    return scaledStereogram;
}


void StereogramWidget::setDepthData(const quint16* nuiData, const QSize& size)
{
    mOriginalDepthData = DepthData(nuiData, size, mNearClipping, mFarClipping);
    update();
}


void StereogramWidget::setRequestedStereogramSize(const QSize& requestedSize)
{
    mRequestedStereogramSize = requestedSize;
    invalidateTexture();
    update();
}


void StereogramWidget::setTextureMode(TextureMode textureMode)
{
    mTextureMode = textureMode;
    invalidateTexture();
    makeTexture();
    update();
}


void StereogramWidget::setTexture(const QImage& texture)
{
    mPreliminaryTexture = texture;
    invalidateTexture();
    update();
}


void StereogramWidget::setClipping(int nearClipping, int farClipping)
{
    mNearClipping = nearClipping;
    mFarClipping = farClipping;
    mOriginalDepthData.setClipping(mNearClipping, mFarClipping);
    update();
}


void StereogramWidget::setNearClipping(int nearClipping)
{
    mNearClipping = nearClipping;
    mOriginalDepthData.setNearClipping(nearClipping);
    update();
}


void StereogramWidget::setFarClipping(int farClipping)
{
    mFarClipping = farClipping;
    mOriginalDepthData.setFarClipping(farClipping);
    update();
}


void StereogramWidget::setEyeDistance(float eyeDistance)
{
    Q_ASSERT_X(eyeDistance > 0.0f, "StereogramWidget::setEyeDistance()", "eyeDistance muss größer als 0 sein");
    Q_ASSERT_X(eyeDistance < 5.0f, "StereogramWidget::setEyeDistance()", "eyeDistance muss kleiner als 5 sein");
    if (qFuzzyCompare(eyeDistance, 0))
        return;
    mEyeDist = eyeDistance;
    update();
}


void StereogramWidget::setMu(int mu)
{
    Q_ASSERT(mu >= 0);
    Q_ASSERT(mu <= 1000);
    mMu = (float)mu / 1000;
    update();
}


void StereogramWidget::setResolution(int resolution)
{
    Q_ASSERT(resolution > 0);
    mResolution = resolution;
    update();
}


void StereogramWidget::hideEvent(QHideEvent*)
{
    if ((windowType() & Qt::Window) == Qt::Window)
        emit attaching();
}


void StereogramWidget::mouseDoubleClickEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton) {
        if ((windowType() & Qt::Window) == Qt::Window)
            emit attaching();
        else
            emit detaching();
        e->accept();
    }
}


void StereogramWidget::dragEnterEvent(QDragEnterEvent* e)
{
    const QMimeData* d = e->mimeData();
    if (d->hasUrls()) {
        if (d->urls().first().toString().contains(QRegExp("\\.(png|jpg)$")))
            e->acceptProposedAction();
    }
}


void StereogramWidget::dragLeaveEvent(QDragLeaveEvent* e)
{
    e->accept();
}


void StereogramWidget::dropEvent(QDropEvent* e)
{
    const QMimeData* d = e->mimeData();
    if (d->hasUrls()) {
        QString file = d->urls().first().toString();
        if (file.contains(QRegExp("file://.*\\.(png|jpg)$"))) {
            QImage texture = QImage(file.remove("file:///"));
            if (!texture.isNull()) {
                setTexture(texture);
                e->acceptProposedAction();
            }
        }
    }
}
