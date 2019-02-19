/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "clangcodemodelserverproxy.h"
#include "lineprefixer.h"
#include "processcreator.h"

#include <QLocalServer>
#include <QLocalSocket>
#include <QScopedPointer>
#include <QTemporaryDir>

#include <future>
#include <memory>

QT_BEGIN_NAMESPACE
class QProcess;
QT_END_NAMESPACE

class Utf8String;
class Utf8StringVector;

namespace ClangBackEnd {

class FileContainer;

class CLANGSUPPORT_EXPORT ConnectionClient : public QObject
{
    Q_OBJECT

public:
    ConnectionClient(const QString &connectionName);

    ConnectionClient(const ConnectionClient &) = delete;
    ConnectionClient &operator=(const ConnectionClient &) = delete;

    void startProcessAndConnectToServerAsynchronously();
    void disconnectFromServer();
    bool isConnected() const;

    void sendEndMessage();

    void resetProcessAliveTimer();
    void setProcessAliveTimerInterval(int processTimerInterval);

    void setProcessPath(const QString &processPath);

    void restartProcessAsynchronously();
    void restartProcessIfTimerIsNotResettedAndSocketIsEmpty();
    void finishProcess();
    bool isProcessRunning();

    bool waitForEcho();
    bool waitForConnected();

    QProcess *processForTestOnly();

signals:
    void connectedToLocalSocket();
    void disconnectedFromLocalSocket();
    void processFinished();

protected:
    ~ConnectionClient();

protected:
    QIODevice *ioDevice();
    const QTemporaryDir &temporaryDirectory() const;
    LinePrefixer &stdErrPrefixer();
    LinePrefixer &stdOutPrefixer();

    virtual void sendEndCommand() = 0;
    virtual void resetState() = 0;
    virtual QString outputName() const = 0;

    QString connectionName() const;
    bool event(QEvent* event);

    virtual void newConnectedServer(QLocalSocket *localSocket) = 0;

private:
    static bool isProcessRunning(QProcess *process);
    void finishProcess(QProcessUniquePointer &&process);
    void endProcess(QProcess *process);
    void terminateProcess(QProcess *process);
    void killProcess(QProcess *process);
    void finishConnection();
    void printLocalSocketError(QLocalSocket::LocalSocketError socketError);
    void printStandardOutput();
    void printStandardError();
    void initializeProcess(QProcess *process);

    void resetTemporaryDirectory();

    void connectLocalSocketDisconnected();
    void disconnectLocalSocketDisconnected();
    void connectStandardOutputAndError(QProcess *process) const;
    void connectLocalSocketError() const;
    void connectAliveTimer();
    void connectNewConnection();
    void handleNewConnection();
    void getProcessFromFuture();
    void listenForConnections();

    void ensureMessageIsWritten();

protected:
    ProcessCreator m_processCreator;

private:
    LinePrefixer m_stdErrPrefixer;
    LinePrefixer m_stdOutPrefixer;
    QLocalServer m_localServer;
    std::future<QProcessUniquePointer> m_processFuture;
    mutable QProcessUniquePointer m_process;
    QLocalSocket *m_localSocket = nullptr;
    QTimer m_processAliveTimer;
    QString m_connectionName;
    bool m_isAliveTimerResetted = false;
    bool m_processIsStarting = false;
};

} // namespace ClangBackEnd
