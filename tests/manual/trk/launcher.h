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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/
#ifndef LAUNCHER_H
#define LAUNCHER_H

#include <QtCore/QObject>
#include <QtCore/QVariant>

namespace trk {

struct TrkResult;
struct TrkMessage;
struct LauncherPrivate;

class Launcher : public QObject
{
    Q_OBJECT
public:
    typedef void (Launcher::*TrkCallBack)(const TrkResult &);

    Launcher();
    ~Launcher();
    void setTrkServerName(const QString &name);
    void setFileName(const QString &name);
    void setCopyFileName(const QString &srcName, const QString &dstName);
    void setInstallFileName(const QString &name);
    bool startServer(QString *errorMessage);
    void setVerbose(int v);

signals:
    void copyingStarted();
    void installingStarted();
    void startingApplication();
    void applicationRunning(uint pid);
    void finished();
    void applicationOutputReceived(const QString &output);

public slots:
    void terminate();

private:
    bool openTrkPort(const QString &port, QString *errorMessage); // or server name for local server
    void sendTrkMessage(unsigned char code,
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
    void sendTrkAck(unsigned char token);

    // kill process and breakpoints
    void cleanUp();

    void timerEvent(QTimerEvent *ev);
    unsigned char nextTrkWriteToken();

    void handleFileCreation(const TrkResult &result);
    void handleFileCreated(const TrkResult &result);
    void handleInstallPackageFinished(const TrkResult &result);
    void handleCpuType(const TrkResult &result);
    void handleCreateProcess(const TrkResult &result);
    void handleWaitForFinished(const TrkResult &result);
    void handleStop(const TrkResult &result);
    void handleSupportMask(const TrkResult &result);
    void handleTrkVersion(const TrkResult &result);
    void waitForTrkFinished(const TrkResult &data);

    void handleAndReportCreateProcess(const TrkResult &result);
    void handleResult(const TrkResult &data);

    void copyFileToRemote();
    void installRemotePackageSilently(const QString &filename);
    void installAndRun();
    void startInferiorIfNeeded();

    void logMessage(const QString &msg);

    LauncherPrivate *d;
};

} // namespace Trk

#endif // LAUNCHER_H
