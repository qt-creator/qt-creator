/****************************************************************************
**
** Copyright (C) 2016 BlackBerry Limited. All rights reserved.
** Contact: KDAB (info@kdab.com)
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

#include "qnxdeviceprocess.h"

#include <projectexplorer/devicesupport/sshdeviceprocess.h>
#include <projectexplorer/runnables.h>
#include <utils/qtcprocess.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Qnx {
namespace Internal {

static int pidFileCounter = 0;

QnxDeviceProcess::QnxDeviceProcess(const QSharedPointer<const IDevice> &device, QObject *parent)
    : SshDeviceProcess(device, parent)
{
    m_pidFile = QString::fromLatin1("/var/run/qtc.%1.pid").arg(++pidFileCounter);
}

QString QnxDeviceProcess::fullCommandLine(const StandardRunnable &runnable) const
{
    QStringList args = QtcProcess::splitArgs(runnable.commandLineArguments);
    args.prepend(runnable.executable);
    QString cmd = QtcProcess::Arguments::createUnixArgs(args).toString();

    QString fullCommandLine = QLatin1String(
        "test -f /etc/profile && . /etc/profile ; "
        "test -f $HOME/profile && . $HOME/profile ; "
    );

    if (!runnable.workingDirectory.isEmpty())
        fullCommandLine += QString::fromLatin1("cd %1 ; ").arg(QtcProcess::quoteArg(runnable.workingDirectory));

    const Environment env = runnable.environment;
    for (auto it = env.constBegin(); it != env.constEnd(); ++it)
        fullCommandLine += QString::fromLatin1("%1='%2' ").arg(env.key(it)).arg(env.value(it));

    fullCommandLine += QString::fromLatin1("%1 & echo $! > %2").arg(cmd).arg(m_pidFile);

    return fullCommandLine;
}

void QnxDeviceProcess::doSignal(int sig)
{
    auto signaler = new SshDeviceProcess(device(), this);
    StandardRunnable r;
    r.executable = QString::fromLatin1("kill -%2 `cat %1`").arg(m_pidFile).arg(sig);
    connect(signaler, &SshDeviceProcess::finished, signaler, &QObject::deleteLater);
    signaler->start(r);
}

} // namespace Internal
} // namespace Qnx
