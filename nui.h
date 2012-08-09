// Copyright (c) 2011 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.
// $Id: nui.h a207395837b2 2012/06/11 15:19:01 Oliver Lau <oliver@von-und-fuer-lau.de> $

#ifndef __NUI_H_
#define __NUI_H_

#define static_assert(_0, _1)

#include <Windows.h>
#include <Propsys.h>
#include <NuiApi.h>

#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtCore/QTime>
#include <QtCore/QSize>
#include <QtCore/QtDebug>
#include <QtGui/QColor>
#include <QtGui/QImage>

#ifndef __OUT
#define __OUT
#endif

#ifndef __IN
#define __IN
#endif

class NUI : public QObject // Singleton
{
    Q_OBJECT
public:
    static NUI* instance(void);
    static const int FrameInterval = 33; // one frame per 33 ms
    static const int BadAngle = INT_MAX;

    int tilt(void);

    void enableTracking(HANDLE event = NULL);
    void disableTracking(void);

    HANDLE openStream(NUI_IMAGE_TYPE eImageType, NUI_IMAGE_RESOLUTION eResolution, HANDLE event);
    const QImage& getStreamVideoFrame(HANDLE stream);
    const QImage& getStreamDepthFrame(HANDLE stream);
    const quint16* getStreamDepthData(HANDLE stream, __OUT QSize& width) const;

    inline HRESULT resultCode(void) const { return mResult; }

    const QImage& getLastVideoFrame(void) const { return mFrame; }
    const QImage& getLastDepthFrame(void) const { return mDepthFrame; }
    const quint16* getLastDepthData(void) const { return mDepthData; }
    const QSize& getLastDepthFrameSize(void) const { return mDepthFrameSize; }
    const QSize& getLastFrameSize(void) const { return mFrameSize; }

public slots:
    void setNearClipping(int);
    void setFarClipping(int);
    void setTilt(int);

private:

    explicit NUI(QObject* parent = NULL);
    ~NUI();

    void calcDepthScale(void);

    QRgb depthToRGB(quint16);

    static NUI* mInstance;

    HRESULT mResult;
    int mNearClipping;
    int mFarClipping;
    float mDepthScale;

    QSize mDepthFrameSize;
    const quint16* mDepthData;
    QImage mDepthFrame;
    QSize mFrameSize;
    QImage mFrame;

    class Watchdog {
    public:
        ~Watchdog() {
            if (NUI::mInstance)
                delete NUI::mInstance;
        }
    };

    static Watchdog mWatchdog;
    friend class Watchdog;
};


#endif // __NUI_H_
