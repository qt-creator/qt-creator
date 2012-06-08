/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/
#ifndef REMOTELINUXCHECKFORFREEDISKSPACESERVICE_H
#define REMOTELINUXCHECKFORFREEDISKSPACESERVICE_H

#include "abstractremotelinuxdeployservice.h"

namespace RemoteLinux {
namespace Internal { class RemoteLinuxCheckForFreeDiskSpaceServicePrivate; }

class REMOTELINUX_EXPORT RemoteLinuxCheckForFreeDiskSpaceService : public AbstractRemoteLinuxDeployService
{
    Q_OBJECT
public:
    RemoteLinuxCheckForFreeDiskSpaceService(QObject *parent = 0);
    ~RemoteLinuxCheckForFreeDiskSpaceService();

    void setPathToCheck(const QString &path);
    void setRequiredSpaceInBytes(quint64 sizeInBytes);

private slots:
    void handleStdErr();
    void handleProcessFinished();

private:
    bool isDeploymentNecessary() const { return true; }
    void doDeviceSetup() { handleDeviceSetupDone(true); }
    void stopDeviceSetup() { handleDeviceSetupDone(false); }

    bool isDeploymentPossible(QString *whyNot) const;
    void doDeploy();
    void stopDeployment();

    void cleanup();

    Internal::RemoteLinuxCheckForFreeDiskSpaceServicePrivate * const d;
};

} // namespace RemoteLinux

#endif // REMOTELINUXCHECKFORFREEDISKSPACESERVICE_H
