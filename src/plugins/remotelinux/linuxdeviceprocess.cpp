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

#include <projectexplorer/runnables.h>

#include <utils/environment.h>
#include <utils/qtcprocess.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux {

static QString quote(const QString &s) { return Utils::QtcProcess::quoteArgUnix(s); }

LinuxDeviceProcess::LinuxDeviceProcess(const QSharedPointer<const ProjectExplorer::IDevice> &device,
        QObject *parent)
    : ProjectExplorer::SshDeviceProcess(device, parent), m_processId(0)
{
    connect(this, &DeviceProcess::finished, this, [this]() {
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
    if (m_processId != 0)
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
    return m_processId;
}

QString LinuxDeviceProcess::fullCommandLine(const StandardRunnable &runnable) const
{
    const Environment env = runnable.environment;

    QString fullCommandLine;
    foreach (const QString &filePath, rcFilesToSource())
        fullCommandLine += QString::fromLatin1("test -f %1 && . %1;").arg(filePath);
    if (!runnable.workingDirectory.isEmpty()) {
        fullCommandLine.append(QLatin1String("cd ")).append(quote(runnable.workingDirectory))
                .append(QLatin1String(" && "));
    }
    QString envString;
    for (auto it = env.constBegin(); it != env.constEnd(); ++it) {
        if (!envString.isEmpty())
            envString += QLatin1Char(' ');
        envString.append(env.key(it)).append(QLatin1String("='")).append(env.value(it))
                .append(QLatin1Char('\''));
    }
    fullCommandLine.append("echo $$ && ");
    if (!envString.isEmpty())
        fullCommandLine.append(envString);
    fullCommandLine.append(" exec ");
    fullCommandLine.append(quote(runnable.executable));
    if (!runnable.commandLineArguments.isEmpty()) {
        fullCommandLine.append(QLatin1Char(' '));
        fullCommandLine.append(runnable.commandLineArguments);
    }
    return fullCommandLine;
}

QStringList LinuxDeviceProcess::rcFilesToSource() const
{
    if (!m_rcFilesToSource.isEmpty())
        return m_rcFilesToSource;
    return QStringList() << QLatin1String("/etc/profile") << QLatin1String("$HOME/.profile");
}

} // namespace RemoteLinux
