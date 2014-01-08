/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QTC_SSHDEVICEPROCESS_H
#define QTC_SSHDEVICEPROCESS_H

#include "deviceprocess.h"

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT SshDeviceProcess : public DeviceProcess
{
    Q_OBJECT
public:
    SshDeviceProcess(const QSharedPointer<const IDevice> &device, QObject *parent = 0);
    ~SshDeviceProcess();

    void start(const QString &executable, const QStringList &arguments);
    void interrupt();
    void terminate();
    void kill();

    QString executable() const;
    QStringList arguments() const;
    QProcess::ProcessState state() const;
    QProcess::ExitStatus exitStatus() const;
    int exitCode() const;
    QString errorString() const;

    Utils::Environment environment() const;
    void setEnvironment(const Utils::Environment &env);

    void setWorkingDirectory(const QString & /* directory */) { } // No such thing in the RFC.

    QByteArray readAllStandardOutput();
    QByteArray readAllStandardError();

    qint64 write(const QByteArray &data);

    // Default is "false" due to OpenSSH not implementing this feature for some reason.
    void setSshServerSupportsSignals(bool signalsSupported);

private slots:
    void handleConnected();
    void handleConnectionError();
    void handleDisconnected();
    void handleProcessStarted();
    void handleProcessFinished(int exitStatus);
    void handleStdout();
    void handleStderr();
    void handleKillOperationFinished(const QString &errorMessage);
    void handleKillOperationTimeout();

private:
    virtual QString fullCommandLine() const;

    class SshDeviceProcessPrivate;
    friend class SshDeviceProcessPrivate;
    SshDeviceProcessPrivate * const d;
};

} // namespace ProjectExplorer

#endif // Include guard.
