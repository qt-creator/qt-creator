/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://www.qtsoftware.com/contact.
**
**************************************************************************/
#ifndef LAUNCHER_H
#define LAUNCHER_H

#include "trkutils.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QQueue>
#include <QtCore/QTimer>
#include <QtCore/QDateTime>

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QLocalServer>
#include <QtNetwork/QLocalSocket>

#if USE_NATIVE
#include <windows.h>
#endif

namespace trk {

class Adapter : public QObject
{
    Q_OBJECT

public:
    Adapter();
    ~Adapter();
    void setTrkServerName(const QString &name) { m_trkServerName = name; }
    void setFileName(const QString &name) { m_fileName = name; }
    void setCopyFileName(const QString &srcName, const QString &dstName) { m_copySrcFileName = srcName; m_copyDstFileName = dstName; }
    void setInstallFileName(const QString &name) { m_installFileName = name; }
    bool startServer();

signals:
    void copyingStarted();
    void installingStarted();
    void startingApplication();
    void applicationRunning(uint pid);
    void finished();

public slots:
    void terminate();

private:
    //
    // TRK
    //
    typedef void (Adapter::*TrkCallBack)(const TrkResult &);

    struct TrkMessage
    {
        TrkMessage() { code = token = 0; callBack = 0; }
        byte code;
        byte token;
        QByteArray data;
        QVariant cookie;
        TrkCallBack callBack;
    };

    bool openTrkPort(const QString &port); // or server name for local server
    void sendTrkMessage(byte code,
        TrkCallBack callBack = 0,
        const QByteArray &data = QByteArray(),
        const QVariant &cookie = QVariant());
    // adds message to 'send' queue
    void queueTrkMessage(const TrkMessage &msg);
    void tryTrkWrite();
    void tryTrkRead();
    // actually writes a message to the device
    void trkWrite(const TrkMessage &msg);
    // convienience messages
    void sendTrkInitialPing();
    void sendTrkAck(byte token);

    // kill process and breakpoints
    void cleanUp();

    void timerEvent(QTimerEvent *ev);
    byte nextTrkWriteToken();

    void handleFileCreation(const TrkResult &result);
    void handleFileCreated(const TrkResult &result);
    void handleInstallPackageFinished(const TrkResult &result);
    void handleCpuType(const TrkResult &result);
    void handleCreateProcess(const TrkResult &result);
    void handleWaitForFinished(const TrkResult &result);
    void handleStop(const TrkResult &result);
    void handleSupportMask(const TrkResult &result);
    void waitForTrkFinished(const TrkResult &data);

    void handleAndReportCreateProcess(const TrkResult &result);
    void handleResult(const TrkResult &data);

    void copyFileToRemote();
    void installRemotePackageSilently(const QString &filename);
    void installAndRun();
    void startInferiorIfNeeded();

#if USE_NATIVE
    HANDLE m_hdevice;
#else
    QLocalSocket *m_trkDevice;
#endif

    QString m_trkServerName;
    QByteArray m_trkReadBuffer;

    unsigned char m_trkWriteToken;
    QQueue<TrkMessage> m_trkWriteQueue;
    QHash<byte, TrkMessage> m_writtenTrkMessages;
    QByteArray m_trkReadQueue;
    bool m_trkWriteBusy;

    void logMessage(const QString &msg);
    // Debuggee state
    Session m_session; // global-ish data (process id, target information)

    int m_timerId;
    QString m_fileName;
    QString m_copySrcFileName;
    QString m_copyDstFileName;
    QString m_installFileName;
};

} // namespace Trk

#endif // LAUNCHER_H
