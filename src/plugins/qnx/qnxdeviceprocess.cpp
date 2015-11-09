/**************************************************************************
**
** Copyright (C) 2012 - 2014 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
** Contact: KDAB (info@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qnxdeviceprocess.h"

#include <projectexplorer/devicesupport/sshdeviceprocess.h>
#include <utils/qtcprocess.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Qnx {
namespace Internal {

static int pidFileCounter = 0;

QnxDeviceProcess::QnxDeviceProcess(const QSharedPointer<const IDevice> &device, QObject *parent)
    : SshDeviceProcess(device, parent)
{
    setEnvironment(Environment(OsTypeLinux));
    m_pidFile = QString::fromLatin1("/var/run/qtc.%1.pid").arg(++pidFileCounter);
}

QString QnxDeviceProcess::fullCommandLine() const
{
    QStringList args = arguments();
    args.prepend(executable());
    QString cmd = QtcProcess::Arguments::createUnixArgs(args).toString();

    QString fullCommandLine = QLatin1String(
        "test -f /etc/profile && . /etc/profile ; "
        "test -f $HOME/profile && . $HOME/profile ; "
    );

    if (!m_workingDir.isEmpty())
        fullCommandLine += QString::fromLatin1("cd %1 ; ").arg(QtcProcess::quoteArg(m_workingDir));

    for (auto it = environment().constBegin(); it != environment().constEnd(); ++it)
        fullCommandLine += QString::fromLatin1("%1='%2' ").arg(it.key()).arg(it.value());

    fullCommandLine += QString::fromLatin1("%1 & echo $! > %2").arg(cmd).arg(m_pidFile);

    return fullCommandLine;
}

void QnxDeviceProcess::doSignal(int sig)
{
    auto signaler = new SshDeviceProcess(device(), this);
    QString cmd = QString::fromLatin1("kill -%2 `cat %1`").arg(m_pidFile).arg(sig);
    connect(signaler, &SshDeviceProcess::finished, signaler, &QObject::deleteLater);
    signaler->start(cmd, QStringList());
}

} // namespace Internal
} // namespace Qnx
