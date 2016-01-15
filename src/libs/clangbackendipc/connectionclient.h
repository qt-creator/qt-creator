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

#ifndef CLANGBACKEND_CONNECTIONCLIENT_H
#define CLANGBACKEND_CONNECTIONCLIENT_H

#include "ipcserverproxy.h"
#include "lineprefixer.h"

#include <QLocalSocket>
#include <QProcessEnvironment>

#include <memory>

QT_BEGIN_NAMESPACE
class QProcess;
QT_END_NAMESPACE

class Utf8String;
class Utf8StringVector;

namespace ClangBackEnd {

class FileContainer;

class CMBIPC_EXPORT ConnectionClient : public QObject
{
    Q_OBJECT

public:
    ConnectionClient(IpcClientInterface *client);
    ~ConnectionClient();

    bool connectToServer();
    bool disconnectFromServer();
    bool isConnected() const;

    void sendEndMessage();

    void resetProcessAliveTimer();
    void setProcessAliveTimerInterval(int processTimerInterval);

    const QString &processPath() const;
    void setProcessPath(const QString &processPath);

    void startProcess();
    void restartProcess();
    void restartProcessIfTimerIsNotResettedAndSocketIsEmpty();
    void finishProcess();
    bool isProcessIsRunning() const;

    bool waitForEcho();

    IpcServerProxy &serverProxy();

    QProcess *processForTestOnly() const;

signals:
    void processRestarted();

private:
    bool connectToLocalSocket();
    void endProcess();
    void terminateProcess();
    void killProcess();
    void printLocalSocketError(QLocalSocket::LocalSocketError socketError);
    void printStandardOutput();
    void printStandardError();

    QProcess *process() const;
    void connectProcessFinished() const;
    void disconnectProcessFinished() const;
    void connectStandardOutputAndError() const;

    void ensureMessageIsWritten();

    QProcessEnvironment processEnvironment() const;

private:
    mutable std::unique_ptr<QProcess> process_;
    QLocalSocket localSocket;
    IpcServerProxy serverProxy_;
    QTimer processAliveTimer;
    QString processPath_;
    bool isAliveTimerResetted;

    LinePrefixer stdErrPrefixer;
    LinePrefixer stdOutPrefixer;
};

} // namespace ClangBackEnd

#endif // CLANGBACKEND_CONNECTIONCLIENT_H
