// Copyright (C) 2016 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qnxdeviceprocesssignaloperation.h"

namespace Qnx::Internal {

QnxDeviceProcessSignalOperation::QnxDeviceProcessSignalOperation(
        const ProjectExplorer::IDeviceConstPtr &device)
    : RemoteLinux::RemoteLinuxSignalOperation(device)
{
}

static QString signalProcessByNameQnxCommandLine(const QString &filePath, int sig)
{
    QString executable = filePath;
    return QString::fromLatin1("for PID in $(ps -f -o pid,comm | grep %1 | awk '/%1/ {print $1}'); "
        "do "
            "kill -%2 $PID; "
        "done").arg(executable.replace(QLatin1String("/"), QLatin1String("\\/"))).arg(sig);
}

QString QnxDeviceProcessSignalOperation::killProcessByNameCommandLine(
        const QString &filePath) const
{
    return QString::fromLatin1("%1; %2").arg(signalProcessByNameQnxCommandLine(filePath, 15),
                                           signalProcessByNameQnxCommandLine(filePath, 9));
}

QString QnxDeviceProcessSignalOperation::interruptProcessByNameCommandLine(
        const QString &filePath) const
{
    return signalProcessByNameQnxCommandLine(filePath, 2);
}

} // Qnx::Internal
