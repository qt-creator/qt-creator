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

#include "linuxdeviceprocess.h"

#include <projectexplorer/runcontrol.h>

#include <utils/environment.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux {

const QByteArray pidMarker = "__qtc";

LinuxDeviceProcess::LinuxDeviceProcess(const QSharedPointer<const ProjectExplorer::IDevice> &device,
                                       QObject *parent)
    : ProjectExplorer::SshDeviceProcess(device, parent)
{
    connect(this, &DeviceProcess::finished, this, [this]() {
        m_processId = 0;
    });
    connect(this, &DeviceProcess::started, this, [this]() {
        m_pidParsed = false;
        m_output.clear();
    });
}

void LinuxDeviceProcess::setRcFilesToSource(const QStringList &filePaths)
{
    m_rcFilesToSource = filePaths;
}

QByteArray LinuxDeviceProcess::readAllStandardOutput()
{
    QByteArray output = SshDeviceProcess::readAllStandardOutput();
    if (m_pidParsed || runInTerminal())
        return output;

    m_output.append(output);
    static const QByteArray endMarker = pidMarker + '\n';
    const int endMarkerOffset = m_output.indexOf(endMarker);
    if (endMarkerOffset == -1)
        return {};
    const int startMarkerOffset = m_output.indexOf(pidMarker);
    if (startMarkerOffset == endMarkerOffset) // Only theoretically possible.
        return {};
    const int pidStart = startMarkerOffset + pidMarker.length();
    const QByteArray pidString = m_output.mid(pidStart, endMarkerOffset - pidStart);
    m_pidParsed = true;
    m_processId = pidString.toLongLong();

    // We don't want to show output from e.g. /etc/profile.
    const QByteArray actualOutput = m_output.mid(endMarkerOffset + endMarker.length());
    m_output.clear();
    return actualOutput;
}

qint64 LinuxDeviceProcess::processId() const
{
    return m_processId < 0 ? 0 : m_processId;
}

QString LinuxDeviceProcess::fullCommandLine(const Runnable &runnable) const
{
    CommandLine cmd;

    for (const QString &filePath : rcFilesToSource()) {
        cmd.addArgs({"test", "-f", filePath});
        cmd.addArgs("&&", CommandLine::Raw);
        cmd.addArgs({".", filePath});
        cmd.addArgs(";", CommandLine::Raw);
    }

    if (!runnable.workingDirectory.isEmpty()) {
        cmd.addArgs({"cd", runnable.workingDirectory});
        cmd.addArgs("&&", CommandLine::Raw);
    }

    if (!runInTerminal())
        cmd.addArgs(QString("echo ") + pidMarker + "$$" + pidMarker + " && ", CommandLine::Raw);

    const Environment &env = runnable.environment;
    for (auto it = env.constBegin(); it != env.constEnd(); ++it)
        cmd.addArgs(env.key(it) + "='" + env.expandedValueForKey(env.key(it)) + '\'', CommandLine::Raw);

    if (!runInTerminal())
        cmd.addArg("exec");

    cmd.addArg(runnable.executable.toString());
    cmd.addArgs(runnable.commandLineArguments, CommandLine::Raw);

    return cmd.arguments();
}

const QStringList LinuxDeviceProcess::rcFilesToSource() const
{
    if (!m_rcFilesToSource.isEmpty())
        return m_rcFilesToSource;
    return {"/etc/profile", "$HOME/.profile"};
}

} // namespace RemoteLinux
