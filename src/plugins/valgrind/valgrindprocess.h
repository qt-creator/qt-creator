/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
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

#include <projectexplorer/applicationlauncher.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/runnables.h>

#include <ssh/sshremoteprocess.h>
#include <ssh/sshconnection.h>
#include <utils/osspecificaspects.h>
#include <utils/outputformat.h>

namespace Valgrind {

/**
 * Process for supplying local and remote valgrind runs
 */
class ValgrindProcess : public QObject
{
    Q_OBJECT

public:
    ValgrindProcess(const ProjectExplorer::IDevice::ConstPtr &device, QObject *parent);
    ~ValgrindProcess();

    bool isRunning() const;

    void setValgrindExecutable(const QString &valgrindExecutable);
    void setValgrindArguments(const QStringList &valgrindArguments);
    void setDebuggee(const ProjectExplorer::StandardRunnable &debuggee);

    void run(ProjectExplorer::ApplicationLauncher::Mode runMode);
    void close();

    QString errorString() const;
    QProcess::ProcessError processError() const;

    void setProcessChannelMode(QProcess::ProcessChannelMode mode);
    QString workingDirectory() const;

    ProjectExplorer::IDevice::ConstPtr device() const { return m_device; }

    qint64 pid() const;
    bool isLocal() const;

signals:
    void started();
    void finished(int, QProcess::ExitStatus);
    void error(QProcess::ProcessError);
    void processOutput(const QString &, Utils::OutputFormat format);

private:
    void handleRemoteStderr(const QByteArray &b);
    void handleRemoteStdout(const QByteArray &b);

    void closed(bool success);
    void localProcessStarted();
    void remoteProcessStarted();
    void findPIDOutputReceived(const QByteArray &out);

    QString argumentString(Utils::OsType osType) const;

    ProjectExplorer::StandardRunnable m_debuggee;
    ProjectExplorer::ApplicationLauncher m_valgrindProcess;
    qint64 m_pid = 0;
    ProjectExplorer::IDevice::ConstPtr m_device;

    ProjectExplorer::ApplicationLauncher m_findPID;

    QSsh::SshConnectionParameters m_params;
    QString m_valgrindExecutable;
    QStringList m_valgrindArguments;
};

} // namespace Valgrind
