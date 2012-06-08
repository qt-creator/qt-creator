/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/
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
