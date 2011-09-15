/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/
#include "sshkeydeployer.h"

#include <utils/ssh/sshremoteprocessrunner.h>
#include <utils/fileutils.h>

#include <QtCore/QFile>
#include <QtCore/QSharedPointer>

using namespace Utils;

namespace RemoteLinux {
namespace Internal {

class SshKeyDeployerPrivate
{
public:
    SshRemoteProcessRunner::Ptr deployProcess;
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
    d->deployProcess = SshRemoteProcessRunner::create(sshParams);

    Utils::FileReader reader;
    if (!reader.fetch(keyFilePath)) {
        emit error(tr("Public key error: %1").arg(reader.errorString()));
        return;
    }

    connect(d->deployProcess.data(), SIGNAL(connectionError(Utils::SshError)), this,
        SLOT(handleConnectionFailure()));
    connect(d->deployProcess.data(), SIGNAL(processClosed(int)), this,
        SLOT(handleKeyUploadFinished(int)));
    const QByteArray command = "test -d .ssh "
        "|| mkdir .ssh && chmod 0700 .ssh && echo '"
        + reader.data() + "' >> .ssh/authorized_keys && chmod 0600 .ssh/authorized_keys";
    d->deployProcess->run(command);
}

void SshKeyDeployer::handleConnectionFailure()
{
    if (!d->deployProcess)
        return;
    const QString errorMsg = d->deployProcess->connection()->errorString();
    cleanup();
    emit error(tr("Connection failed: %1").arg(errorMsg));
}

void SshKeyDeployer::handleKeyUploadFinished(int exitStatus)
{
    Q_ASSERT(exitStatus == SshRemoteProcess::FailedToStart
        || exitStatus == SshRemoteProcess::KilledBySignal
        || exitStatus == SshRemoteProcess::ExitedNormally);

    if (!d->deployProcess)
        return;

    const int exitCode = d->deployProcess->process()->exitCode();
    const QString errorMsg = d->deployProcess->process()->errorString();
    cleanup();
    if (exitStatus == SshRemoteProcess::ExitedNormally && exitCode == 0)
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
    if (d->deployProcess) {
        disconnect(d->deployProcess.data(), 0, this, 0);
        d->deployProcess.clear();
    }
}

} // namespace RemoteLinux
