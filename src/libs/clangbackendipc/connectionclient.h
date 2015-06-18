/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CLANGBACKEND_CONNECTIONCLIENT_H
#define CLANGBACKEND_CONNECTIONCLIENT_H

#include "ipcserverproxy.h"
#include "lineprefixer.h"

#include <QLocalSocket>

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

    void sendEndCommand();

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

    void ensureCommandIsWritten();

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
