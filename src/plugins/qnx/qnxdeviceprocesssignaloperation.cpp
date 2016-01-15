/****************************************************************************
**
** Copyright (C) 2016 BlackBerry Limited. All rights reserved.
** Contact: BlackBerry (qt@blackberry.com), KDAB (info@kdab.com)
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

#include "qnxdeviceprocesssignaloperation.h"

using namespace Qnx;
using namespace Qnx::Internal;

QnxDeviceProcessSignalOperation::QnxDeviceProcessSignalOperation(
        const QSsh::SshConnectionParameters &sshParameters)
    : RemoteLinux::RemoteLinuxSignalOperation(sshParameters)
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
