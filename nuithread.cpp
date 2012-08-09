// Copyright (c) 2011-2012 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.
// $Id: nuithread.cpp 2cd7b967385f 2012/06/04 15:28:18 Oliver Lau <oliver@von-und-fuer-lau.de> $

#include "nuithread.h"


NUIThread::NUIThread(QObject* parent)
    : QThread(parent)
    , mFrozen(false)
{
    mNUI = NUI::instance();
    mEvNuiProcessStop = CreateEvent(NULL, FALSE, FALSE, NULL);
    mNextDepthFrameEvent = CreateEvent(NULL, TRUE,  FALSE, NULL);
    mNextVideoFrameEvent = CreateEvent(NULL, TRUE,  FALSE, NULL);
    mDepthStreamHandle = mNUI->openStream(NUI_IMAGE_TYPE_DEPTH , NUI_IMAGE_RESOLUTION_640x480, mNextDepthFrameEvent);
    mVideoStreamHandle = mNUI->openStream(NUI_IMAGE_TYPE_COLOR, NUI_IMAGE_RESOLUTION_640x480, mNextVideoFrameEvent);
    start();
}


NUIThread::~NUIThread()
{
    halt();
}


void NUIThread::halt(void)
{
    if (mEvNuiProcessStop) {
        SetEvent(mEvNuiProcessStop);
        wait();
        CloseHandle(mEvNuiProcessStop);
    }
}


void NUIThread::setFrozen(bool frozen)
{
    mFrozen = frozen;
}


void NUIThread::run(void)
{
    static const DWORD EventCount = 3;
    const HANDLE hEvent[EventCount] = {
        mEvNuiProcessStop,    // 0
        mNextDepthFrameEvent, // 1
        mNextVideoFrameEvent  // 2
    };
    forever {
        const DWORD eIdx = WaitForMultipleObjects(EventCount, hEvent, FALSE, 100);
        switch (eIdx) {
        case 1:
            if (!mFrozen) {
                QSize frameSize;
                const quint16* data = mNUI->getStreamDepthData(mDepthStreamHandle, frameSize);
                emit depthDataReady(data, frameSize);
            }
            ResetEvent(mNextDepthFrameEvent);
            break;
        case 2:
            if (!mFrozen) {
                const QImage& img = mNUI->getStreamVideoFrame(mVideoStreamHandle);
                emit videoFrameReady(img);
            }
            ResetEvent(mNextVideoFrameEvent);
            break;
        case 0: return;
        }
    }
}

