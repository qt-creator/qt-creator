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

#include "remotelinuxcheckforfreediskspaceservice.h"

#include <ssh/sshremoteprocessrunner.h>
#include <utils/fileutils.h>

#include <QScopeGuard>

namespace RemoteLinux {
namespace Internal {
class RemoteLinuxCheckForFreeDiskSpaceServicePrivate
{
public:
    QString pathToCheck;
    quint64 requiredSpaceInBytes = 0;
};
} // namespace Internal

RemoteLinuxCheckForFreeDiskSpaceService::RemoteLinuxCheckForFreeDiskSpaceService(QObject *parent)
        : AbstractRemoteLinuxDeployService(parent),
          d(new Internal::RemoteLinuxCheckForFreeDiskSpaceServicePrivate)
{
}

RemoteLinuxCheckForFreeDiskSpaceService::~RemoteLinuxCheckForFreeDiskSpaceService()
{
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

void RemoteLinuxCheckForFreeDiskSpaceService::deployAndFinish()
{
    auto cleanup = qScopeGuard([this] { setFinished(); });
    const Utils::FilePath path
            = deviceConfiguration()->mapToGlobalPath(Utils::FilePath::fromString(d->pathToCheck));
    const qint64 freeSpace = path.bytesAvailable();
    if (freeSpace < 0) {
        emit errorMessage(tr("Can't get the info about the free disk space for \"%1\"")
                .arg(path.toUserOutput()));
        return;
    }

    const qint64 mb = 1024 * 1024;
    const qint64 freeSpaceMB = freeSpace / mb;
    const qint64 requiredSpaceMB = d->requiredSpaceInBytes / mb;

    if (freeSpaceMB < requiredSpaceMB) {
        emit errorMessage(tr("The remote file system has only %n megabytes of free space, "
                "but %1 megabytes are required.", nullptr, freeSpaceMB)
                          .arg(requiredSpaceMB));
        return;
    }

    emit progressMessage(tr("The remote file system has %n megabytes of free space, going ahead.",
                            nullptr, freeSpaceMB));
}

CheckResult RemoteLinuxCheckForFreeDiskSpaceService::isDeploymentPossible() const
{
    if (!d->pathToCheck.startsWith(QLatin1Char('/'))) {
        return CheckResult::failure(
           tr("Cannot check for free disk space: \"%1\" is not an absolute path.")
                    .arg(d->pathToCheck));
    }

    return AbstractRemoteLinuxDeployService::isDeploymentPossible();
}

} // namespace RemoteLinux
