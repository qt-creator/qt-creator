/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef MAEMODEPLOYMENTMOUNTER_H
#define MAEMODEPLOYMENTMOUNTER_H

#include "maemomountspecification.h"

#include <remotelinux/portlist.h>

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QSharedPointer>

namespace Utils { class SshConnection; }
namespace Qt4ProjectManager { class Qt4BuildConfiguration; }

namespace RemoteLinux {
class LinuxDeviceConfiguration;
class RemoteLinuxUsedPortsGatherer;
}

namespace Madde {
namespace Internal {
class MaemoRemoteMounter;

class MaemoDeploymentMounter : public QObject
{
    Q_OBJECT
public:
    explicit MaemoDeploymentMounter(QObject *parent = 0);
    ~MaemoDeploymentMounter();

    // Connection must be in connected state.
    void setupMounts(const QSharedPointer<Utils::SshConnection> &connection,
        const QSharedPointer<const RemoteLinux::LinuxDeviceConfiguration> &devConf,
        const QList<MaemoMountSpecification> &mountSpecs,
        const Qt4ProjectManager::Qt4BuildConfiguration *bc);
    void tearDownMounts();

signals:
    void debugOutput(const QString &output);
    void setupDone();
    void tearDownDone();
    void error(const QString &error);
    void reportProgress(const QString &message);

private slots:
    void handleMounted();
    void handleUnmounted();
    void handleMountError(const QString &errorMsg);
    void handlePortsGathererError(const QString &errorMsg);
    void handlePortListReady();
    void handleConnectionError();

private:
    enum State {
        Inactive, UnmountingOldDirs, UnmountingCurrentDirs, GatheringPorts,
        Mounting, Mounted, UnmountingCurrentMounts
    };

    void unmount();
    void setupMounter();
    void setState(State newState);

    State m_state;
    QSharedPointer<Utils::SshConnection> m_connection;
    QSharedPointer<const RemoteLinux::LinuxDeviceConfiguration> m_devConf;
    MaemoRemoteMounter * const m_mounter;
    RemoteLinux::RemoteLinuxUsedPortsGatherer * const m_portsGatherer;
    RemoteLinux::PortList m_freePorts;
    QList<MaemoMountSpecification> m_mountSpecs;
    const Qt4ProjectManager::Qt4BuildConfiguration *m_buildConfig;
};

} // namespace Internal
} // namespace Madde

#endif // MAEMODEPLOYMENTMOUNTER_H
