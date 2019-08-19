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

LinuxDeviceProcess::LinuxDeviceProcess(const QSharedPointer<const ProjectExplorer::IDevice> &device,
                                       QObject *parent)
    : ProjectExplorer::SshDeviceProcess(device, parent), m_processId(0)
{
    connect(this, &DeviceProcess::finished, this, [this]() {
        m_processId = -1;
    });
    connect(this, &DeviceProcess::started, this, [this]() {
        m_processId = 0;
    });
}

void LinuxDeviceProcess::setRcFilesToSource(const QStringList &filePaths)
{
    m_rcFilesToSource = filePaths;
}

QByteArray LinuxDeviceProcess::readAllStandardOutput()
{
    QByteArray output = SshDeviceProcess::readAllStandardOutput();
    if (m_processId != 0 || runInTerminal())
        return output;

    m_processIdString.append(output);
    int cut = m_processIdString.indexOf('\n');
    if (cut != -1) {
        m_processId = m_processIdString.left(cut).toLongLong();
        output = m_processIdString.mid(cut + 1);
        m_processIdString.clear();
        return output;
    }
    return QByteArray();
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
        cmd.addArgs("echo $$ && ", CommandLine::Raw);

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
