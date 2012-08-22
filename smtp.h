/*
    Copyright (c) 2012 Nicholas Smith

    Permission is hereby granted, free of charge, to any person obtaining a copy of
    this software and associated documentation files (the "Software"), to deal in
    the Software without restriction, including without limitation the rights to
    use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
    of the Software, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.

    Code taken from https://github.com/nicholassmith/Qt-SMTP
*/


#ifndef __SMTP_H_
#define __SMTP_H_

#include <QTcpSocket>
#include <QString>
#include <QTextStream>
#include <QDebug>
#include <QMessageBox>

class Smtp : public QObject
{
    Q_OBJECT


public:
    Smtp(const QString& serverName, quint16 serverPort, const QString& username, const QString& password, const QString& from, const QStringList& to, const QString& subject, const QString& body);
    ~Smtp();
    void initialise();

signals:
        void status(const QString&);
        void finished(void);

private slots:
        void stateChanged(QAbstractSocket::SocketState);
        void errorReceived(QAbstractSocket::SocketError);
        void disconnected(void);
        void connected(void);
        void readyRead(void);

private:
        QString mMsg;
        QTextStream *mTextStream;
        QTcpSocket *mSocket;
        QString mFrom;
        QString mRcpt;
        QString mResponse;
        QStringList mRecipients;
        QString mServer;
        quint16 mPort;
        QString mUser;
        QString mPassword;
        int mX;
        enum states {Auth, User, Pass, Rcpt, Mail, Data, Init, Body, Quit, Close};
        int mState;
};

#endif // __SMTP_H_
