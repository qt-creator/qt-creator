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

#include "maemodeploymentmounter.h"

#include "maemoglobal.h"
#include "maemoremotemounter.h"

#include <projectexplorer/target.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qtsupport/qtprofileinformation.h>
#include <remotelinux/linuxdeviceconfiguration.h>
#include <remotelinux/remotelinuxusedportsgatherer.h>
#include <utils/qtcassert.h>
#include <ssh/sshconnection.h>

using namespace Qt4ProjectManager;
using namespace RemoteLinux;
using namespace QSsh;

namespace Madde {
namespace Internal {

MaemoDeploymentMounter::MaemoDeploymentMounter(QObject *parent)
    : QObject(parent),
      m_state(Inactive),
      m_mounter(new MaemoRemoteMounter(this)),
      m_portsGatherer(new RemoteLinuxUsedPortsGatherer(this))
{
    connect(m_mounter, SIGNAL(error(QString)), SLOT(handleMountError(QString)));
    connect(m_mounter, SIGNAL(mounted()), SLOT(handleMounted()));
    connect(m_mounter, SIGNAL(unmounted()), SLOT(handleUnmounted()));
    connect(m_mounter, SIGNAL(reportProgress(QString)),
        SIGNAL(reportProgress(QString)));
    connect(m_mounter, SIGNAL(debugOutput(QString)),
        SIGNAL(debugOutput(QString)));

    connect(m_portsGatherer, SIGNAL(error(QString)),
        SLOT(handlePortsGathererError(QString)));
    connect(m_portsGatherer, SIGNAL(portListReady()),
        SLOT(handlePortListReady()));
}

MaemoDeploymentMounter::~MaemoDeploymentMounter() {}

void MaemoDeploymentMounter::setupMounts(SshConnection *connection,
    const LinuxDeviceConfiguration::ConstPtr &devConf,
    const QList<MaemoMountSpecification> &mountSpecs,
    const Qt4BuildConfiguration *bc)
{
    QTC_ASSERT(m_state == Inactive, return);

    m_mountSpecs = mountSpecs;
    m_connection = connection;
    m_devConf = devConf;
    m_mounter->setConnection(m_connection, m_devConf);
    m_buildConfig = bc;
    connect(m_connection, SIGNAL(error(QSsh::SshError)), SLOT(handleConnectionError()));
    setState(UnmountingOldDirs);
    unmount();
}

void MaemoDeploymentMounter::tearDownMounts()
{
    QTC_ASSERT(m_state == Mounted, return);

    setState(UnmountingCurrentMounts);
    unmount();
}

void MaemoDeploymentMounter::setupMounter()
{
    QTC_ASSERT(m_state == UnmountingOldDirs, return);

    setState(UnmountingCurrentDirs);

    m_mounter->resetMountSpecifications();
    m_mounter->setBuildConfiguration(m_buildConfig);
    foreach (const MaemoMountSpecification &mountSpec, m_mountSpecs)
        m_mounter->addMountSpecification(mountSpec, true);
    unmount();
}

void MaemoDeploymentMounter::unmount()
{
    QTC_ASSERT(m_state == UnmountingOldDirs || m_state == UnmountingCurrentDirs
        || m_state == UnmountingCurrentMounts, return);

    if (m_mounter->hasValidMountSpecifications())
        m_mounter->unmount();
    else
        handleUnmounted();
}

void MaemoDeploymentMounter::handleMounted()
{
    QTC_ASSERT(m_state == Mounting || m_state == Inactive, return);

    if (m_state == Inactive)
        return;

    setState(Mounted);
    emit setupDone();
}

void MaemoDeploymentMounter::handleUnmounted()
{
    QTC_ASSERT(m_state == UnmountingOldDirs || m_state == UnmountingCurrentDirs
        || m_state == UnmountingCurrentMounts || m_state == Inactive, return);

    switch (m_state) {
    case UnmountingOldDirs:
        setupMounter();
        break;
    case UnmountingCurrentDirs:
        setState(GatheringPorts);
        m_portsGatherer->start(m_devConf);
        break;
    case UnmountingCurrentMounts:
        setState(Inactive);
        emit tearDownDone();
        break;
    case Inactive:
    default:
        break;
    }
}

void MaemoDeploymentMounter::handlePortsGathererError(const QString &errorMsg)
{
    QTC_ASSERT(m_state == GatheringPorts || m_state == Inactive, return);

    if (m_state == Inactive)
        return;

    setState(Inactive);
    m_mounter->resetMountSpecifications();
    emit error(errorMsg);
}

void MaemoDeploymentMounter::handlePortListReady()
{
    QTC_ASSERT(m_state == GatheringPorts || m_state == Inactive, return);

    if (m_state == Inactive)
        return;

    setState(Mounting);
    m_freePorts = MaemoGlobal::freePorts(m_devConf, QtSupport::QtProfileInformation::qtVersion(m_buildConfig->target()->profile()));
    m_mounter->mount(&m_freePorts, m_portsGatherer);
}

void MaemoDeploymentMounter::handleMountError(const QString &errorMsg)
{
    QTC_ASSERT(m_state == UnmountingOldDirs || m_state == UnmountingCurrentDirs
        || m_state == UnmountingCurrentMounts || m_state == Mounting || m_state == Mounted
        || m_state == Inactive, return);

    if (m_state == Inactive)
        return;

    setState(Inactive);
    emit error(errorMsg);
}

void MaemoDeploymentMounter::handleConnectionError()
{
    if (m_state == Inactive)
        return;

    setState(Inactive);
    emit error(tr("Connection failed: %1").arg(m_connection->errorString()));
}

void MaemoDeploymentMounter::setState(State newState)
{
    if (m_state == newState)
        return;
    if (newState == Inactive && m_connection) {
        disconnect(m_connection, 0, this, 0);
        m_connection = 0;
    }
    m_state = newState;
}

} // namespace Internal
} // namespace Madde
