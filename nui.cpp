// Copyright (c) 2011-2012 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.


#include <QtCore>
#include <QtCore/QString>
#include <QtCore/QDebug>
#include <QtGui/QMessageBox>

#include "nui.h"


NUI* NUI::mInstance = NULL;


NUI* NUI::instance(void)
{
    if (mInstance == NULL)
        mInstance = new NUI;
    return mInstance;
}


NUI::NUI(QObject* parent)
    : QObject(parent)
    , mNearClipping(NUI_IMAGE_DEPTH_MINIMUM)
    , mFarClipping(NUI_IMAGE_DEPTH_MAXIMUM)
    , mDepthData(NULL)
{
    mResult = NuiInitialize(NUI_INITIALIZE_FLAG_USES_DEPTH | NUI_INITIALIZE_FLAG_USES_COLOR);
    if (FAILED(mResult))
        QMessageBox::critical(NULL, QObject::tr("NuiInitialize() fehlgeschlagen"), QObject::tr("Kinect angeschlossen?"));
    calcDepthScale();
}


NUI::~NUI()
{
    NuiShutdown();
}


int NUI::tilt(void)
{
    long angle = BadAngle;
    mResult = NuiCameraElevationGetAngle(&angle);
    if (FAILED(mResult))
        QMessageBox::critical(NULL, QObject::tr("NuiCameraElevationGetAngle() fehlgeschlagen"), QObject::tr("Kinect angeschlossen?"));
    return (int)angle;
}


void NUI::setTilt(int angle)
{
    mResult = NuiCameraElevationSetAngle((long)angle);
    if (FAILED(mResult))
        qDebug() << QObject::tr("NuiCameraElevationSetAngle() fehlgeschlagen");
}


void NUI::enableTracking(HANDLE event)
{
    mResult = NuiSkeletonTrackingEnable(event, 0);
    if (FAILED(mResult))
        QMessageBox::critical(NULL, QObject::tr("NuiSkeletonTrackingEnable() fehlgeschlagen"), QObject::tr("Kinect angeschlossen?"));
}


void NUI::disableTracking(void)
{
    mResult = NuiSkeletonTrackingDisable();
    if (FAILED(mResult))
        QMessageBox::critical(NULL, QObject::tr("NuiSkeletonTrackingDisable() fehlgeschlagen"), QObject::tr("Kinect angeschlossen?"));
}


void NUI::calcDepthScale(void)
{
    mDepthScale = 256.0f / (mFarClipping - mNearClipping);
}


void NUI::setNearClipping(int nearClipping)
{
    mNearClipping = nearClipping;
    calcDepthScale();
}


void NUI::setFarClipping(int farClipping)
{
    mFarClipping = farClipping;
    calcDepthScale();
}



HANDLE NUI::openStream(NUI_IMAGE_TYPE eImageType, NUI_IMAGE_RESOLUTION eResolution, HANDLE event)
{
    HANDLE stream;
    mResult = NuiImageStreamOpen(eImageType, eResolution, 0, 2, event, &stream);
    if (FAILED(mResult))
        QMessageBox::critical(NULL, QObject::tr("NuiImageStreamOpen() fehlgeschlagen"), QObject::tr("Kinect angeschlossen?"));
    return stream;
}


inline QRgb NUI::depthToRGB(quint16 s)
{
    const int person = s & 7;
    const int depth = (s >> 7) & 0x00ff;
    int b = (depth < mNearClipping || depth > mFarClipping)
            ? 0
            : (int)(mDepthScale * (depth - mNearClipping));
    switch (person) {
    case 0: // no person
        return qRgb(b, b, b);
    case 1: // person 1
        return qRgb(b, 0, 0);
    case 2: // person 2
        return qRgb(0, b, 0);
    case 3: // person 3
        return qRgb(0, 0, b);
    case 4: // person 4
        return qRgb(b, b, 0);
    case 5: // person 5
        return qRgb(0, b, b);
    case 6: // person 6
        return qRgb(b/4, b, b);
    case 7: // person 7
        return qRgb(b, b/2, b/2);
    }
    return 0; // just to please the compiler
}


const QImage& NUI::getStreamDepthFrame(HANDLE stream)
{
    const NUI_IMAGE_FRAME* pImageFrame = NULL;
    HRESULT hr = NuiImageStreamGetNextFrame(stream, 0, &pImageFrame);
    if (FAILED(hr))
        return mDepthFrame;
    INuiFrameTexture* const pTexture = pImageFrame->pFrameTexture;
    NUI_LOCKED_RECT LockedRect;
    pTexture->LockRect(0, &LockedRect, NULL, 0);
    NUI_SURFACE_DESC SurfaceDesc;
    pTexture->GetLevelDesc(0, &SurfaceDesc);
    mDepthFrame = QImage(SurfaceDesc.Width, SurfaceDesc.Height, QImage::Format_RGB32);
    if (LockedRect.Pitch != 0) {
        const quint16* buf = reinterpret_cast<const quint16*>(LockedRect.pBits);
        for (UINT y = 0; y < SurfaceDesc.Height; ++y) {
            QRgb* scanLine = reinterpret_cast<QRgb*>(mDepthFrame.scanLine(y));
            for (int x = 0; x < (int)SurfaceDesc.Width; ++x)
                *scanLine++ = depthToRGB(*buf++);
        }
    }
    NuiImageStreamReleaseFrame(stream, pImageFrame);
    return mDepthFrame;
}


const quint16* NUI::getStreamDepthData(HANDLE stream, QSize& frameSize) const
{
    const quint16* result = NULL;
    const NUI_IMAGE_FRAME* pImageFrame = NULL;
    HRESULT hr = NuiImageStreamGetNextFrame(stream, 0, &pImageFrame);
    if (FAILED(hr))
        return NULL;
    INuiFrameTexture* const pTexture = pImageFrame->pFrameTexture;
    NUI_LOCKED_RECT LockedRect;
    pTexture->LockRect(0, &LockedRect, NULL, 0);
    if (LockedRect.Pitch != 0) {
        NUI_SURFACE_DESC SurfaceDesc;
        pTexture->GetLevelDesc(0, &SurfaceDesc);
        frameSize = QSize(SurfaceDesc.Width, SurfaceDesc.Height);
        result = reinterpret_cast<const quint16*>(LockedRect.pBits);
    }
    NuiImageStreamReleaseFrame(stream, pImageFrame);
    return result;
}


const QImage& NUI::getStreamVideoFrame(HANDLE stream)
{
    const NUI_IMAGE_FRAME* pImageFrame = NULL;
    HRESULT hr = NuiImageStreamGetNextFrame(stream, 0, &pImageFrame);
    if (FAILED(hr))
        return mFrame;
#if 0
    INuiFrameTexture* pTexture = pImageFrame->pFrameTexture;
    NUI_LOCKED_RECT LockedRect;
    pTexture->LockRect(0, &LockedRect, NULL, 0);
    if (FAILED(hr))
        return QImage();
    NUI_SURFACE_DESC SurfaceDesc;
    pTexture->GetLevelDesc(0, &SurfaceDesc);
    QImage frame = (LockedRect.Pitch > 0) ?
                   QImage(reinterpret_cast<const uchar*>(LockedRect.pBits), SurfaceDesc.Width, SurfaceDesc.Height, QImage::Format_RGB32):
                   QImage();
#else
    // etwas schlankere Fassung, die die Ausmaße des Frames
    // auf 640 x 480 Pixel festlegt und annimmt, dass der Framebuffer
    // mit entsprechend vielen Bytes gefüllt ist
    INuiFrameTexture* pTexture = pImageFrame->pFrameTexture;
    NUI_LOCKED_RECT LockedRect;
    hr = pTexture->LockRect(0, &LockedRect, NULL, 0);
    if (FAILED(hr))
        return mFrame;
        mFrame = QImage((const uchar*)LockedRect.pBits, 640, 480, QImage::Format_RGB32);
#endif
    NuiImageStreamReleaseFrame(stream, pImageFrame);
    return mFrame;
}
