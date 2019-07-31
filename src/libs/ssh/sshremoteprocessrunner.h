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

#include "sshconnection.h"
#include "sshremoteprocess.h"

namespace QSsh {
namespace Internal { class SshRemoteProcessRunnerPrivate; }

class QSSH_EXPORT SshRemoteProcessRunner : public QObject
{
    Q_OBJECT

public:
    SshRemoteProcessRunner(QObject *parent = nullptr);
    ~SshRemoteProcessRunner();

    void run(const QString &command, const SshConnectionParameters &sshParams);
    void runInTerminal(const QString &command, const SshConnectionParameters &sshParams);
    QString command() const;

    QString lastConnectionErrorString() const;

    bool isProcessRunning() const;
    void writeDataToProcess(const QByteArray &data);
    void cancel();
    SshRemoteProcess::ExitStatus processExitStatus() const;
    int processExitCode() const;
    QString processErrorString() const;
    QByteArray readAllStandardOutput();
    QByteArray readAllStandardError();

signals:
    void connectionError();
    void processStarted();
    void readyReadStandardOutput();
    void readyReadStandardError();
    void processClosed(const QString &error);

private:
    void handleConnected();
    void handleConnectionError();
    void handleDisconnected();
    void handleProcessStarted();
    void handleProcessFinished(const QString &error);
    void handleStdout();
    void handleStderr();
    void runInternal(const QString &command, const QSsh::SshConnectionParameters &sshParams);
    void setState(int newState);

    Internal::SshRemoteProcessRunnerPrivate * const d;
};

} // namespace QSsh
