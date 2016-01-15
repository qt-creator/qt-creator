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

#include <utils/environment.h>
#include <utils/qtcprocess.h>

namespace RemoteLinux {

static QString quote(const QString &s) { return Utils::QtcProcess::quoteArgUnix(s); }

LinuxDeviceProcess::LinuxDeviceProcess(const QSharedPointer<const ProjectExplorer::IDevice> &device,
        QObject *parent)
    : ProjectExplorer::SshDeviceProcess(device, parent)
{
    setEnvironment(Utils::Environment(Utils::OsTypeLinux));
}

void LinuxDeviceProcess::setRcFilesToSource(const QStringList &filePaths)
{
    m_rcFilesToSource = filePaths;
}

void LinuxDeviceProcess::setWorkingDirectory(const QString &directory)
{
    m_workingDir = directory;
}

QString LinuxDeviceProcess::fullCommandLine() const
{
    QString fullCommandLine;
    foreach (const QString &filePath, rcFilesToSource())
        fullCommandLine += QString::fromLatin1("test -f %1 && . %1;").arg(filePath);
    if (!m_workingDir.isEmpty()) {
        fullCommandLine.append(QLatin1String("cd ")).append(quote(m_workingDir))
                .append(QLatin1String(" && "));
    }
    QString envString;
    for (auto it = environment().constBegin(); it != environment().constEnd(); ++it) {
        if (!envString.isEmpty())
            envString += QLatin1Char(' ');
        envString.append(it.key()).append(QLatin1String("='")).append(it.value())
                .append(QLatin1Char('\''));
    }
    if (!envString.isEmpty())
        fullCommandLine.append(QLatin1Char(' ')).append(envString);
    if (!fullCommandLine.isEmpty())
        fullCommandLine += QLatin1Char(' ');
    fullCommandLine.append(quote(executable()));
    if (!arguments().isEmpty()) {
        fullCommandLine.append(QLatin1Char(' '));
        fullCommandLine.append(Utils::QtcProcess::joinArgs(arguments(), Utils::OsTypeLinux));
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
