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

#include "smtp.h"

Smtp::Smtp(const QString& serverName, quint16 serverPort, const QString& username, const QString& password, const QString& from, const QStringList& to, const QString& subject, const QString& body)
{
    mServer = serverName; // your server name
    mPort = serverPort;
    mUser = username.toLocal8Bit().toBase64(); // your SMTP username
    mPassword = password.toLocal8Bit().toBase64(); // your SMTP password
    mX = 1;
    mSocket = new QTcpSocket(this);
    connect(mSocket, SIGNAL(readyRead()), this, SLOT(readyRead()));
    connect(mSocket, SIGNAL(connected()), this, SLOT(connected()));
    connect(mSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(errorReceived(QAbstractSocket::SocketError)));
    connect(mSocket, SIGNAL(stateChanged( QAbstractSocket::SocketState)), this, SLOT(stateChanged(QAbstractSocket::SocketState)));
    connect(mSocket, SIGNAL(disconnected()), this, SLOT(disconnected()));
    for (int i = 0; i < to.count(); ++i)
        mMsg.append("To: " + to.at(i) + "\n");
    mMsg.append("From: " + mFrom + "\n");
    mMsg.append("Subject: " + subject + "\n");
    mMsg.append("Mime-Version: 1.0;\n");
    mMsg.append("Content-Type: text/html; charset=\"utf8\";\n");
    mMsg.append("Content-Transfer-Encoding: 8bit;\n");
    mMsg.append("\n");
    qDebug() << "body is: " << body;
    mMsg.append(body);
    mMsg.replace(QString::fromLatin1("\n"), QString::fromLatin1("\r\n"));
    mMsg.replace(QString::fromLatin1("\r\n.\r\n"), QString::fromLatin1("\r\n..\r\n"));
    mFrom = from;
    mRcpt = to.at(0);
    mRecipients = to;
    mState = Init;
    mSocket->connectToHost(mServer, mPort);
    if (mSocket->waitForConnected(30000))
        qDebug("connected");
    mTextStream = new QTextStream(mSocket);
}


Smtp::~Smtp()
{
    qDebug() << "Destroying";
    finished();
    delete mTextStream;
    delete mSocket;
}

void Smtp::stateChanged(QAbstractSocket::SocketState socketState)
{

    qDebug() <<"stateChanged: " << socketState;

}

void Smtp::errorReceived(QAbstractSocket::SocketError socketError)
{
    qDebug() << " error:" <<socketError;

}

void Smtp::disconnected()
{
    qDebug() << "Disconnected: "  << mSocket->errorString();
}

void Smtp::connected()
{
    qDebug() << "Connected: "  << mSocket->errorString();
}

void Smtp::readyRead()
{
    QString rLine;
    do
    {
        rLine = mSocket->readLine();
        mResponse += rLine;
        qDebug() << "Response is: " << mResponse;
    }
    while (mSocket->canReadLine() && rLine[3] != ' ');

    rLine.truncate(3);

    if (mState == Init && rLine[0] == '2')
    {
        qDebug() << "HELO there";
        *mTextStream << "HELO there\r\n";
        mTextStream->flush();

        mState = Auth;
    }
    else if (mState == Auth && rLine[0] == '2')
    {
        // Trying AUTH
        qDebug() << "Auth";
        *mTextStream << "AUTH LOGIN" << "\r\n";
        mTextStream->flush();
        mState = User;
    }
    else if (mState == User && rLine[0] == '3')
    {
        //Trying User
        qDebug() << "Username";
        *mTextStream << mUser << "\r\n";
        mTextStream->flush();

        mState = Pass;
    }
    else if (mState == Pass && rLine[0] == '3')
    {
        //Trying pass
        qDebug() << "Pass";
        *mTextStream << mPassword << "\r\n";
        mTextStream->flush();

        mState = Mail;
    }
    else if (mState == Mail && rLine[0] == '2')
    {
        qDebug() << "Mail from";
        *mTextStream << "MAIL FROM: " << mFrom << "\r\n";
        mTextStream->flush();
        mState = Rcpt;
    }
    else if (mState == Rcpt && rLine[0] == '2')
    {

        qDebug() << "RCPT TO ";
        *mTextStream << "RCPT TO: " << mRcpt << "\r\n";
        mTextStream->flush();
        if(mRecipients.isEmpty() || mX == mRecipients.count() )
        {
            mState = Data;
        }
        else
        {
            if(mX < mRecipients.count())
            {
                mRcpt = mRecipients.at(mX);
                mX++;
                mState = Rcpt;
            }
        }
    }
    else if (mState == Data && rLine[0] == '2')
    {
        qDebug() << "Data";
        *mTextStream << "DATA\r\n";
        mTextStream->flush();
        mState = Body;
    }
    else if (mState == Body && rLine[0] == '3')
    {
        qDebug() << "Body state";
        *mTextStream << mMsg << "\r\n.\r\n";
        mTextStream->flush();
        mState = Quit;
    }
    else if (mState == Quit && rLine[0] == '2')
    {
        qDebug() << "Quit";
        *mTextStream << "QUIT\r\n";
        mTextStream->flush();
        // here, we just close.
        mState = Close;
        emit status( tr( "Message sent" ) );
    }
    else if (mState == Close)
    {
        qDebug() << "State == close";
        deleteLater();
        return;
    }
    else
    {
        //something has broken
        mState = Close;
    }
    mResponse = "";
}
