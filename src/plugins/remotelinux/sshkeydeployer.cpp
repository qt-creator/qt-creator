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

#include "sshkeydeployer.h"

#include <ssh/sshremoteprocessrunner.h>
#include <utils/fileutils.h>

using namespace QSsh;

namespace RemoteLinux {
namespace Internal {

class SshKeyDeployerPrivate
{
public:
    SshRemoteProcessRunner deployProcess;
};

} // namespace Internal

SshKeyDeployer::SshKeyDeployer(QObject *parent)
    : QObject(parent), d(new Internal::SshKeyDeployerPrivate)
{
}

SshKeyDeployer::~SshKeyDeployer()
{
    cleanup();
    delete d;
}

void SshKeyDeployer::deployPublicKey(const SshConnectionParameters &sshParams,
    const QString &keyFilePath)
{
    cleanup();

    Utils::FileReader reader;
    if (!reader.fetch(keyFilePath)) {
        emit error(tr("Public key error: %1").arg(reader.errorString()));
        return;
    }

    connect(&d->deployProcess, &SshRemoteProcessRunner::connectionError,
            this, &SshKeyDeployer::handleConnectionFailure);
    connect(&d->deployProcess, &SshRemoteProcessRunner::processClosed,
            this, &SshKeyDeployer::handleKeyUploadFinished);
    const QByteArray command = "test -d .ssh "
        "|| mkdir .ssh && chmod 0700 .ssh && echo '"
        + reader.data() + "' >> .ssh/authorized_keys && chmod 0600 .ssh/authorized_keys";
    d->deployProcess.run(command, sshParams);
}

void SshKeyDeployer::handleConnectionFailure()
{
    cleanup();
    emit error(tr("Connection failed: %1").arg(d->deployProcess.lastConnectionErrorString()));
}

void SshKeyDeployer::handleKeyUploadFinished(int exitStatus)
{
    Q_ASSERT(exitStatus == SshRemoteProcess::FailedToStart
        || exitStatus == SshRemoteProcess::CrashExit
        || exitStatus == SshRemoteProcess::NormalExit);

    const int exitCode = d->deployProcess.processExitCode();
    const QString errorMsg = d->deployProcess.processErrorString();
    cleanup();
    if (exitStatus == SshRemoteProcess::NormalExit && exitCode == 0)
        emit finishedSuccessfully();
    else
        emit error(tr("Key deployment failed: %1.").arg(errorMsg));
}

void SshKeyDeployer::stopDeployment()
{
    cleanup();
}

void SshKeyDeployer::cleanup()
{
    disconnect(&d->deployProcess, 0, this, 0);
}

} // namespace RemoteLinux
