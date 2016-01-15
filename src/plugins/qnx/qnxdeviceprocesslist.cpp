/****************************************************************************
**
** Copyright (C) 2016 BlackBerry Limited. All rights reserved.
** Contact: BlackBerry Limited (blackberry-qt@qnx.com), KDAB (info@kdab.com)
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

#include "qnxdeviceprocesslist.h"

#include <utils/algorithm.h>

#include <QRegExp>
#include <QStringList>

using namespace Qnx;
using namespace Qnx::Internal;

QnxDeviceProcessList::QnxDeviceProcessList(
        const ProjectExplorer::IDevice::ConstPtr &device, QObject *parent)
    : ProjectExplorer::SshDeviceProcessList(device, parent)
{
}

QString QnxDeviceProcessList::listProcessesCommandLine() const
{
    return QLatin1String("pidin -F \"%a %A '/%n'\"");
}

QList<ProjectExplorer::DeviceProcessItem> QnxDeviceProcessList::buildProcessList(
        const QString &listProcessesReply) const
{
    QList<ProjectExplorer::DeviceProcessItem> processes;
    QStringList lines = listProcessesReply.split(QLatin1Char('\n'));
    if (lines.isEmpty())
        return processes;

    lines.pop_front(); // drop headers
    QRegExp re(QLatin1String("\\s*(\\d+)\\s+(.*)'(.*)'"));

    foreach (const QString& line, lines) {
        if (re.exactMatch(line)) {
            const QStringList captures = re.capturedTexts();
            if (captures.size() == 4) {
                const int pid = captures[1].toInt();
                const QString args = captures[2];
                const QString exe = captures[3];
                ProjectExplorer::DeviceProcessItem deviceProcess;
                deviceProcess.pid = pid;
                deviceProcess.exe = exe.trimmed();
                deviceProcess.cmdLine = args.trimmed();
                processes.append(deviceProcess);
            }
        }
    }

    Utils::sort(processes);
    return processes;
}
