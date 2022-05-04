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

#include "remotelinuxcustomcommanddeployservice.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <utils/qtcprocess.h>

using namespace Utils;

namespace RemoteLinux {
namespace Internal {

class RemoteLinuxCustomCommandDeployservicePrivate
{
public:
    QString m_commandLine;
    QtcProcess m_process;
};

RemoteLinuxCustomCommandDeployService::RemoteLinuxCustomCommandDeployService(QObject *parent)
    : AbstractRemoteLinuxDeployService(parent), d(new RemoteLinuxCustomCommandDeployservicePrivate)
{
    connect(&d->m_process, &QtcProcess::readyReadStandardOutput, this, [this] {
        emit stdOutData(QString::fromUtf8(d->m_process.readAllStandardOutput()));
    });
    connect(&d->m_process, &QtcProcess::readyReadStandardError, this, [this] {
        emit stdErrData(QString::fromUtf8(d->m_process.readAllStandardError()));
    });
    connect(&d->m_process, &QtcProcess::done, this, [this] {
        if (d->m_process.error() != QProcess::UnknownError
                || d->m_process.exitStatus() != QProcess::NormalExit) {
            emit errorMessage(tr("Remote process failed: %1").arg(d->m_process.errorString()));
        } else if (d->m_process.exitCode() != 0) {
            emit errorMessage(tr("Remote process finished with exit code %1.")
                .arg(d->m_process.exitCode()));
        } else {
            emit progressMessage(tr("Remote command finished successfully."));
        }
        stopDeployment();
    });
}

RemoteLinuxCustomCommandDeployService::~RemoteLinuxCustomCommandDeployService() = default;

void RemoteLinuxCustomCommandDeployService::setCommandLine(const QString &commandLine)
{
    d->m_commandLine = commandLine;
}

CheckResult RemoteLinuxCustomCommandDeployService::isDeploymentPossible() const
{
    if (d->m_commandLine.isEmpty())
        return CheckResult::failure(tr("No command line given."));

    return AbstractRemoteLinuxDeployService::isDeploymentPossible();
}

void RemoteLinuxCustomCommandDeployService::doDeploy()
{
    emit progressMessage(tr("Starting remote command \"%1\"...").arg(d->m_commandLine));
    d->m_process.setCommand({deviceConfiguration()->mapToGlobalPath("/bin/sh"),
                             {"-c", d->m_commandLine}});
    d->m_process.start();
}

void RemoteLinuxCustomCommandDeployService::stopDeployment()
{
    d->m_process.close();
    handleDeploymentDone();
}

} // namespace Internal
} // namespace RemoteLinux
