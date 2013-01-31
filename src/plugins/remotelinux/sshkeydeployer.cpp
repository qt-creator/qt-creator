/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#include "sshkeydeployer.h"

#include <ssh/sshremoteprocessrunner.h>
#include <utils/fileutils.h>

#include <QFile>
#include <QSharedPointer>

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

    connect(&d->deployProcess, SIGNAL(connectionError()), SLOT(handleConnectionFailure()));
    connect(&d->deployProcess, SIGNAL(processClosed(int)), SLOT(handleKeyUploadFinished(int)));
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
