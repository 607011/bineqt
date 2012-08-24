// Copyright (c) 2012 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.

#include <QSizePolicy>
#include <QPainter>
#include <QTime>
#include <QtCore/QtDebug>
#include "depthimagewidget.h"
#include "globalsettings.h"

#ifdef _OPENMP
#include <omp.h>
#else
inline int omp_get_thread_num(void) { return 0; }
inline int omp_get_num_threads(void) { return 1; }
inline int omp_get_max_threads(void) { return 1; }
#endif


DepthImageWidget::DepthImageWidget(QWidget* parent)
    : QWidget(parent)
    , mNearClipping(NUI_IMAGE_DEPTH_MINIMUM)
    , mFarClipping(NUI_IMAGE_DEPTH_MAXIMUM)
    , mFrameCount(0)
    , mCurrentFrameRate(0)
    , mOverlayFrameOpacity(0)
    , mLeftButtonPressed(false)
    , mFitFrameIntoDepthFrame(false)
    , mDepthData(NULL)
{
    setMinimumSize(WIDTH, HEIGHT);
    QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    sizePolicy.setHeightForWidth(true);
    setSizePolicy(sizePolicy);
    setAcceptDrops(true);
    mLastFrameTime.start();
}


void DepthImageWidget::paintEvent(QPaintEvent*)
{
    if (mDepthFrame.isNull() || mDepthData == NULL)
        return;
    QPainter painter(this);

    if (mFitFrameIntoDepthFrame) {
        QImage depthFrame = QImage(mDepthFrame.size(), QImage::Format_RGB32);
        depthFrame.fill(qRgb(0, 0, 0));
#pragma omp parallel
        {
            LONG x, y;
            const int tileHeight = mDepthFrame.height() / omp_get_num_threads();
            const int firstLine = omp_get_thread_num() * tileHeight;
            const int lastLine = firstLine + tileHeight;
            for (int depthY = firstLine; depthY < lastLine; ++depthY) {
                const quint16* depthData = mDepthData + depthY * mDepthFrame.width();
                for (int depthX = 0; depthX < mDepthFrame.width(); ++depthX) {
                    const USHORT depthZ = *depthData++;
                    const HRESULT rc = NuiImageGetColorPixelCoordinatesFromDepthPixelAtResolution(
                                NUI_IMAGE_RESOLUTION_640x480,
                                NUI_IMAGE_RESOLUTION_640x480,
                                NULL,
                                depthX,
                                depthY,
                                depthZ,
                                &x,
                                &y);
                    if (rc == S_OK && x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT)
                        depthFrame.setPixel(x, y, mDepthFrame.pixel(depthX, depthY));
                }
            }
        }
        painter.drawImage(0, 0, depthFrame);
    }
    else {
        painter.drawImage(0, 0, mDepthFrame);
    }

    if (!mWebcamFrame.isNull()) {
        const qreal opacity = painter.opacity();
        painter.setOpacity(mOverlayFrameOpacity);
        painter.drawImage(0, 0, mWebcamFrame);
        painter.setOpacity(opacity);
        const int elapsed = mLastFrameTime.elapsed();
        ++mFrameCount;
        static const int MS_INTERVAL = 1000;
        if (elapsed > MS_INTERVAL) {
            mCurrentFrameRate = MS_INTERVAL * mFrameCount / elapsed;
            mLastFrameTime.start();
            mFrameCount = 0;
        }
        painter.setPen(Qt::green);
        painter.drawText(12, 16, tr("%1 fps").arg(mCurrentFrameRate, 3, 10, QChar(' ')));

        if (!mMouseStartPos.isNull() && !mMouseCurrentPos.isNull()) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(QBrush(mLeftButtonPressed?  QColor(250, 30, 10, 160) : QColor(250, 170, 0, 140)));
            painter.drawRect(mMouseStartPos.x(), 0, mMouseCurrentPos.x() - mMouseStartPos.x(), height());
        }
    }
}


void DepthImageWidget::setOverlayFrameOpacity(int opacity)
{
    mOverlayFrameOpacity = (qreal)opacity / 100;
    update();
}


void DepthImageWidget::setFrame(const QImage& frame)
{
    mWebcamFrame = frame;
    update();
}


void DepthImageWidget::setFitFrameIntoDepthFrame(bool doFit)
{
    mFitFrameIntoDepthFrame = doFit;
    update();
}


void DepthImageWidget::setDepthData(const quint16* data, const QSize& frameSize)
{
    if (data == NULL)
        return;
    mDepthData = data;
    mDepthFrame = QImage(frameSize, QImage::Format_RGB32);
    const int DEPTH_RANGE = mFarClipping - mNearClipping;
// #pragma omp parallel
    {
        const int tileHeight = mDepthFrame.height() / omp_get_num_threads();
        const int firstLine = omp_get_thread_num() * tileHeight;
        const int lastLine = firstLine + tileHeight;
        // qDebug() << omp_get_thread_num() << firstLine << lastLine << tileHeight;
        for (int y = firstLine; y < lastLine; ++y) {
            const quint16* aDepth = data + y * mDepthFrame.width();
            QRgb* aFrame = reinterpret_cast<QRgb*>(mDepthFrame.scanLine(y));
            for (int x = 0; x < mDepthFrame.width(); ++x) {
                int depth = *aDepth++;
                if (depth == NUI_IMAGE_DEPTH_TOO_FAR_VALUE) {
                    *aFrame++ = COLOR_TOO_FAR;
                }
                else if (depth == NUI_IMAGE_DEPTH_NO_VALUE) {
                    *aFrame++ = COLOR_NO_DEPTH;
                }
                else if (depth < mNearClipping) {
                    *aFrame++ = COLOR_NEAR_CLIPPING;
                }
                else if (depth > mFarClipping) {
                    *aFrame++ = COLOR_FAR_CLIPPING;
                }
                else {
                    depth = 256 - 256 * (depth - mNearClipping) / DEPTH_RANGE;
                    *aFrame++ = qRgb(depth, depth, depth);
                }
            }
        }
    }
    update();
}


void DepthImageWidget::setNearClipping(int nearClipping)
{
    mNearClipping = nearClipping;
    update();
}


void DepthImageWidget::setFarClipping(int farClipping)
{
    mFarClipping = farClipping;
    update();
}


void DepthImageWidget::resetSelection(void)
{
    mMouseStartPos = QPoint();
    mMouseCurrentPos = QPoint();
    update();
}


void DepthImageWidget::mouseMoveEvent(QMouseEvent* e)
{
    if (mLeftButtonPressed) {
        mMouseCurrentPos = e->pos();
        e->accept();
        update();
    }
}


void DepthImageWidget::mousePressEvent(QMouseEvent* e)
{
    mLeftButtonPressed = (e->button() == Qt::LeftButton);
    if (mLeftButtonPressed) {
        mMouseStartPos = e->pos();
        mMouseCurrentPos = e->pos();
        e->accept();
        update();
    }
}


void DepthImageWidget::mouseReleaseEvent(QMouseEvent* e)
{
    if (mLeftButtonPressed) {
        const int x0 = qMin(mMouseStartPos.x(), mMouseCurrentPos.x());
        const int width = qAbs(mMouseCurrentPos.x() - mMouseStartPos.x());
        QImage selection = mWebcamFrame.copy(x0, 0, width, mWebcamFrame.height());
        emit selectionChanged(selection);
        mLeftButtonPressed = false;
        e->accept();
        update();
    }
}


void DepthImageWidget::mouseDoubleClickEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton) {
        emit selectionChanged(QImage());
        mLeftButtonPressed = false;
        mMouseStartPos = QPoint();
        mMouseCurrentPos = QPoint();
        e->accept();
        update();
    }
}


void DepthImageWidget::dragEnterEvent(QDragEnterEvent* e)
{
    const QMimeData* d = e->mimeData();
    if (d->hasUrls()) {
        if (d->urls().first().toString().contains(QRegExp("\\.(png|jpg)$")))
            e->acceptProposedAction();
    }
}


void DepthImageWidget::dragLeaveEvent(QDragLeaveEvent* e)
{
    e->accept();
}


void DepthImageWidget::dropEvent(QDropEvent* e)
{
    const QMimeData* d = e->mimeData();
    if (d->hasUrls()) {
        QString file = d->urls().first().toString();
        if (file.contains(QRegExp("file://.*\\.(png|jpg)$"))) {
            QImage depthImage = QImage(file.remove("file:///"));
            if (!depthImage.isNull()) {
                emit stopStreaming();
                mFarClipping = 256;
                mNearClipping = 0;
                emit clippingChanged(mNearClipping, mFarClipping);
                quint16* depthData = new quint16[depthImage.width() * depthImage.height()];
                quint16* aDepth = depthData;
                for (int y = 0; y < depthImage.height(); ++y) {
                    const QRgb* src = reinterpret_cast<const QRgb*>(depthImage.constScanLine(y));
                    for (int x = 0; x < depthImage.width(); ++x) {
                        QRgb c = *src++;
                        switch (c) {
                        case COLOR_NO_DEPTH:
                        case COLOR_TOO_FAR:
                        case COLOR_FAR_CLIPPING:
                            c = 0;
                            break;
                        case COLOR_NEAR_CLIPPING:
                            c = 255;
                            break;
                        default:
                            // do nothing
                            break;
                        }
                        *aDepth++ = 256-(quint16)qGray(c);
                    }
                }
                setDepthData(depthData, depthImage.size());
                emit depthDataReady(depthData, depthImage.size());
                delete [] depthData;
                mWebcamFrame = QImage();
                resetSelection();
                e->acceptProposedAction();
            }
        }
    }
}
