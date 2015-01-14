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
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef SSHREMOTEPROCESSRUNNER_H
#define SSHREMOTEPROCESSRUNNER_H

#include "sshconnection.h"
#include "sshremoteprocess.h"

namespace QSsh {
namespace Internal { class SshRemoteProcessRunnerPrivate; }

class QSSH_EXPORT SshRemoteProcessRunner : public QObject
{
    Q_OBJECT

public:
    SshRemoteProcessRunner(QObject *parent = 0);
    ~SshRemoteProcessRunner();

    void run(const QByteArray &command, const SshConnectionParameters &sshParams);
    void runInTerminal(const QByteArray &command, const SshPseudoTerminal &terminal,
        const SshConnectionParameters &sshParams);
    QByteArray command() const;

    QSsh::SshError lastConnectionError() const;
    QString lastConnectionErrorString() const;

    bool isProcessRunning() const;
    void writeDataToProcess(const QByteArray &data);
    void sendSignalToProcess(SshRemoteProcess::Signal signal); // No effect with OpenSSH server.
    void cancel(); // Does not stop remote process, just frees SSH-related process resources.
    SshRemoteProcess::ExitStatus processExitStatus() const;
    SshRemoteProcess::Signal processExitSignal() const;
    int processExitCode() const;
    QString processErrorString() const;
    QByteArray readAllStandardOutput();
    QByteArray readAllStandardError();

signals:
    void connectionError();
    void processStarted();
    void readyReadStandardOutput();
    void readyReadStandardError();
    void processClosed(int exitStatus); // values are of type SshRemoteProcess::ExitStatus

private slots:
    void handleConnected();
    void handleConnectionError(QSsh::SshError error);
    void handleDisconnected();
    void handleProcessStarted();
    void handleProcessFinished(int exitStatus);
    void handleStdout();
    void handleStderr();

private:
    void runInternal(const QByteArray &command, const QSsh::SshConnectionParameters &sshParams);
    void setState(int newState);

    Internal::SshRemoteProcessRunnerPrivate * const d;
};

} // namespace QSsh

#endif // SSHREMOTEPROCESSRUNNER_H
