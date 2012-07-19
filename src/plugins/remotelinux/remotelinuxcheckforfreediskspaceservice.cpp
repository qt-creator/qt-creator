/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
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
**
**************************************************************************/
#include "remotelinuxcheckforfreediskspaceservice.h"

#include "linuxdeviceconfiguration.h"

#include <ssh/sshremoteprocessrunner.h>

#include <QByteArray>
#include <QString>

namespace RemoteLinux {
namespace Internal {
class RemoteLinuxCheckForFreeDiskSpaceServicePrivate
{
public:
    QString pathToCheck;
    quint64 requiredSpaceInBytes;
    QSsh::SshRemoteProcessRunner *processRunner;
};
} // namespace Internal

RemoteLinuxCheckForFreeDiskSpaceService::RemoteLinuxCheckForFreeDiskSpaceService(QObject *parent)
        : AbstractRemoteLinuxDeployService(parent),
          d(new Internal::RemoteLinuxCheckForFreeDiskSpaceServicePrivate)
{
    d->processRunner = 0;
    d->requiredSpaceInBytes = 0;
}

RemoteLinuxCheckForFreeDiskSpaceService::~RemoteLinuxCheckForFreeDiskSpaceService()
{
    cleanup();
    delete d;
}

void RemoteLinuxCheckForFreeDiskSpaceService::setPathToCheck(const QString &path)
{
    d->pathToCheck = path;
}

void RemoteLinuxCheckForFreeDiskSpaceService::setRequiredSpaceInBytes(quint64 sizeInBytes)
{
    d->requiredSpaceInBytes = sizeInBytes;
}

void RemoteLinuxCheckForFreeDiskSpaceService::handleStdErr()
{
    emit stdErrData(QString::fromUtf8(d->processRunner->readAllStandardError()));
}

void RemoteLinuxCheckForFreeDiskSpaceService::handleProcessFinished()
{
    switch (d->processRunner->processExitStatus()) {
    case QSsh::SshRemoteProcess::FailedToStart:
        emit errorMessage(tr("Remote process failed to start."));
        stopDeployment();
        return;
    case QSsh::SshRemoteProcess::CrashExit:
        emit errorMessage(tr("Remote process crashed."));
        stopDeployment();
        return;
    case QSsh::SshRemoteProcess::NormalExit:
        break;
    }

    bool isNumber;
    QByteArray processOutput = d->processRunner->readAllStandardOutput();
    processOutput.chop(1); // newline
    quint64 freeSpace = processOutput.toULongLong(&isNumber);
    if (!isNumber) {
        emit errorMessage(tr("Unexpected output from remote process: '%1'.")
                .arg(QString::fromUtf8(processOutput)));
        stopDeployment();
        return;
    }

    freeSpace *= 1024;
    if (freeSpace < d->requiredSpaceInBytes) {
        emit errorMessage(tr("The remote file system has only %1 bytes of free space, "
                "but %2 bytes are required.").arg(freeSpace).arg(d->requiredSpaceInBytes));
        stopDeployment();
        return;
    }

    emit progressMessage(tr("The remote file system has %1 bytes of free space, going ahead.")
            .arg(freeSpace));
    stopDeployment();
}

bool RemoteLinuxCheckForFreeDiskSpaceService::isDeploymentPossible(QString *whyNot) const
{
    if (!AbstractRemoteLinuxDeployService::isDeploymentPossible(whyNot))
        return false;
    if (!d->pathToCheck.startsWith(QLatin1Char('/'))) {
        if (whyNot) {
            *whyNot = tr("Cannot check for free disk space: '%1' is not an absolute path.")
                    .arg(d->pathToCheck);
        }
        return false;
    }
    return true;
}

void RemoteLinuxCheckForFreeDiskSpaceService::doDeploy()
{
    d->processRunner = new QSsh::SshRemoteProcessRunner;
    connect(d->processRunner, SIGNAL(processClosed(int)), SLOT(handleProcessFinished()));
    connect(d->processRunner, SIGNAL(readyReadStandardError()), SLOT(handleStdErr()));
    const QString command = QString::fromLocal8Bit("df -k -P %1 |tail -n 1 |sed 's/  */ /g' "
            "|cut -d ' ' -f 4").arg(d->pathToCheck);
    d->processRunner->run(command.toUtf8(), deviceConfiguration()->sshParameters());
}

void RemoteLinuxCheckForFreeDiskSpaceService::stopDeployment()
{
    cleanup();
    handleDeploymentDone();
}

void RemoteLinuxCheckForFreeDiskSpaceService::cleanup()
{
    if (d->processRunner) {
        disconnect(d->processRunner, 0, this, 0);
        d->processRunner->cancel();
        delete d->processRunner;
        d->processRunner = 0;
    }
}

} // namespace RemoteLinux
