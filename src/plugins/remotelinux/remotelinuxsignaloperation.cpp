// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "remotelinuxsignaloperation.h"

#include "remotelinuxtr.h"

#include <utils/commandline.h>
#include <utils/fileutils.h>
#include <utils/process.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux {

RemoteLinuxSignalOperation::RemoteLinuxSignalOperation(
        const IDeviceConstPtr &device)
    : m_device(device)
{}

RemoteLinuxSignalOperation::~RemoteLinuxSignalOperation() = default;

static QString signalProcessGroupByPidCommandLine(qint64 pid, int signal)
{
    return QString::fromLatin1("kill -%1 -%2").arg(signal).arg(pid);
}

void RemoteLinuxSignalOperation::run(const QString &command)
{
    QTC_ASSERT(!m_process, return);
    m_process.reset(new Process);
    connect(m_process.get(), &Process::done, this, &RemoteLinuxSignalOperation::runnerDone);

    m_process->setCommand({m_device->filePath("/bin/sh"), {"-c", command}});
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
    run(QString::fromLatin1("%1 && %2")
        .arg(signalProcessGroupByPidCommandLine(pid, 15),
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
        m_errorMessage = Tr::tr("Exit code is %1. stderr:").arg(m_process->exitCode())
                + QLatin1Char(' ')
                + QString::fromLatin1(m_process->readAllRawStandardError());
    }
    m_process.release()->deleteLater();
    emit finished(m_errorMessage);
}

} // RemoteLinux
