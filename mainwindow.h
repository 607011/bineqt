// Copyright (c) 2012 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.

#ifndef __MAINWINDOW_H_
#define __MAINWINDOW_H_

#include <QMainWindow>
#include <QCloseEvent>
#include <QImage>

#include "depthimagewidget.h"
#include "stereogramwidget.h"
#include "nuithread.h"

#define IFA_MODE

#ifdef IFA_MODE
// #define IFA_SEND_MAIL
#endif

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = NULL);
    ~MainWindow();

    static const QString Company;
    static const QString AppName;
    static const QString AppVersion;

protected:
    void closeEvent(QCloseEvent*);

private: // methods
    void saveAppSettings(void);
    void restoreAppSettings(void);
    void loadTexture(const QString&);

private slots:
    void openTexture(void);
    void saveDepthImage(void);
    void saveStereogram(void);
    void printStereogram(void);
    void freezeToggled(bool running = false);
    void setEyeDistance(int /* mm */);
    void setTilt(int);
    void setSelection(const QImage&);
    void placeStereogramWidget(void);
    void attachStereogramWidget(void);
    void detachStereogramWidget(void);
    void fitFrameIntoDepthFrame(bool);
    void modeChanged(int);
    void stereogramSizeChanged(int);

private:
    Ui::MainWindow *ui;
    DepthImageWidget* mDepthWidget;
    StereogramWidget* mStereogramWidget;
    NUIThread mNUIThread;
    QString mTextureFileName;
    QImage mTexture;
    bool mDepthFrameFrozen;
    QSize mSavedStereogramSize;

#ifdef IFA_MODE
    int mFileSequenceNumber;
#ifdef IFA_SEND_MAIL
    QString mSmtpServer;
    quint16 mSmtpPort;
    QString mSmtpUser;
    QString mSmtpPass;
#endif // IFA_SEND_MAIL
#endif // IFA_MODE
};

#endif // __MAINWINDOW_H_
