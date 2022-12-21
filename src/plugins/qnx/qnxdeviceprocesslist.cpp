// Copyright (C) 2016 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qnxdeviceprocesslist.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/processinfo.h>

#include <QRegularExpression>
#include <QStringList>

using namespace Utils;

namespace Qnx::Internal {

QnxDeviceProcessList::QnxDeviceProcessList(
        const ProjectExplorer::IDevice::ConstPtr &device, QObject *parent)
    : ProjectExplorer::SshDeviceProcessList(device, parent)
{
}

QString QnxDeviceProcessList::listProcessesCommandLine() const
{
    return QLatin1String("pidin -F '%a %A {/%n}'");
}

QList<ProcessInfo> QnxDeviceProcessList::buildProcessList(const QString &listProcessesReply) const
{
    QList<ProcessInfo> processes;
    QStringList lines = listProcessesReply.split(QLatin1Char('\n'));
    if (lines.isEmpty())
        return processes;

    lines.pop_front(); // drop headers
    const QRegularExpression re("\\s*(\\d+)\\s+(.*){(.*)}");

    for (const QString &line : std::as_const(lines)) {
        const QRegularExpressionMatch match = re.match(line);
        if (match.hasMatch()) {
            const QStringList captures = match.capturedTexts();
            if (captures.size() == 4) {
                const int pid = captures[1].toInt();
                const QString args = captures[2];
                const QString exe = captures[3];
                ProcessInfo deviceProcess;
                deviceProcess.processId = pid;
                deviceProcess.executable = exe.trimmed();
                deviceProcess.commandLine = args.trimmed();
                processes.append(deviceProcess);
            }
        }
    }

    return Utils::sorted(std::move(processes));
}

} // Qnx::Internal
