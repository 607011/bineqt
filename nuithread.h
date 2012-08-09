// Copyright (c) 2011-2012 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.
// $Id: nuithread.h 487ecbee7e01 2012/06/01 14:34:02 Oliver Lau <oliver@von-und-fuer-lau.de> $

#ifndef __NUITHREAD_H_
#define __NUITHREAD_H_

#include <Windows.h>

#include <QThread>

#include "nui.h"


class NUIThread : public QThread
{
    Q_OBJECT
public:
    explicit NUIThread(QObject* parent = NULL);
    ~NUIThread();

    void halt(void);

    QSize depthFrameSize(void) const { return QSize(640, 480); }
    QSize videoFrameSize(void) const { return QSize(640, 480); }

public slots:
    void setFrozen(bool frozen = true);

protected:
    void run(void);

private:
    NUI* mNUI;

    HANDLE mEvNuiProcessStop;
    HANDLE mNextDepthFrameEvent;
    HANDLE mNextVideoFrameEvent;

    HANDLE mVideoStreamHandle;
    HANDLE mDepthStreamHandle;

    bool mFrozen;

signals:
    void videoFrameReady(const QImage&);
    void depthFrameReady(const QImage&);
    void depthDataReady(const quint16*, const QSize&);

};

#endif // __NUITHREAD_H_
