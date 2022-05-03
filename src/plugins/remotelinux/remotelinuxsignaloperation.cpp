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

#include "remotelinuxsignaloperation.h"

#include <utils/commandline.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

using namespace RemoteLinux;
using namespace ProjectExplorer;
using namespace Utils;

RemoteLinuxSignalOperation::RemoteLinuxSignalOperation(
        const IDeviceConstPtr &device)
    : m_device(device)
{}

RemoteLinuxSignalOperation::~RemoteLinuxSignalOperation() = default;

static QString signalProcessGroupByPidCommandLine(qint64 pid, int signal)
{
    return QString::fromLatin1("kill -%1 -%2 %2").arg(signal).arg(pid);
}

void RemoteLinuxSignalOperation::run(const QString &command)
{
    QTC_ASSERT(!m_process, return);
    m_process.reset(new QtcProcess);
    connect(m_process.get(), &QtcProcess::done, this, &RemoteLinuxSignalOperation::runnerDone);

    m_process->setCommand({m_device->mapToGlobalPath("/bin/sh"), {"-c", command}});
    m_process->start();
}

static QString signalProcessGroupByNameCommandLine(const QString &filePath, int signal)
{
    return QString::fromLatin1(
                "cd /proc; for pid in `ls -d [0123456789]*`; "
                "do "
                "if [ \"`readlink /proc/$pid/exe`\" = \"%1\" ]; then "
                "    kill -%2 -$pid $pid;"
                "fi; "
                "done").arg(filePath).arg(signal);
}

QString RemoteLinuxSignalOperation::killProcessByNameCommandLine(const QString &filePath) const
{
    return QString::fromLatin1("%1; %2").arg(signalProcessGroupByNameCommandLine(filePath, 15),
                                             signalProcessGroupByNameCommandLine(filePath, 9));
}

QString RemoteLinuxSignalOperation::interruptProcessByNameCommandLine(const QString &filePath) const
{
    return signalProcessGroupByNameCommandLine(filePath, 2);
}

void RemoteLinuxSignalOperation::killProcess(qint64 pid)
{
    run(QString::fromLatin1("%1; sleep 1; %2 && %3")
        .arg(signalProcessGroupByPidCommandLine(pid, 15),
             signalProcessGroupByPidCommandLine(pid, 0),
             signalProcessGroupByPidCommandLine(pid, 9)));
}

void RemoteLinuxSignalOperation::killProcess(const QString &filePath)
{
    run(killProcessByNameCommandLine(filePath));
}

void RemoteLinuxSignalOperation::interruptProcess(qint64 pid)
{
    run(signalProcessGroupByPidCommandLine(pid, 2));
}

void RemoteLinuxSignalOperation::interruptProcess(const QString &filePath)
{
    run(interruptProcessByNameCommandLine(filePath));
}

void RemoteLinuxSignalOperation::runnerDone()
{
    m_errorMessage.clear();
    if (m_process->exitStatus() != QProcess::NormalExit) {
        m_errorMessage = m_process->errorString();
    } else if (m_process->exitCode() != 0) {
        m_errorMessage = tr("Exit code is %1. stderr:").arg(m_process->exitCode())
                + QLatin1Char(' ')
                + QString::fromLatin1(m_process->readAllStandardError());
    }
    m_process.release()->deleteLater();
    emit finished(m_errorMessage);
}
