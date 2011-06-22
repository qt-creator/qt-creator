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

#include "maemodeploymentmounter.h"

#include "maemoglobal.h"
#include "maemoremotemounter.h"
#include "maemousedportsgatherer.h"

#include <qt4projectmanager/qt4buildconfiguration.h>
#include <utils/ssh/sshconnection.h>

#define ASSERT_STATE(state) ASSERT_STATE_GENERIC(State, state, m_state)

using namespace Qt4ProjectManager;
using namespace Utils;

namespace RemoteLinux {
namespace Internal {

MaemoDeploymentMounter::MaemoDeploymentMounter(QObject *parent)
    : QObject(parent),
      m_state(Inactive),
      m_mounter(new MaemoRemoteMounter(this)),
      m_portsGatherer(new MaemoUsedPortsGatherer(this))
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

void MaemoDeploymentMounter::setupMounts(const SshConnection::Ptr &connection,
    const LinuxDeviceConfiguration::ConstPtr &devConf,
    const QList<MaemoMountSpecification> &mountSpecs,
    const Qt4BuildConfiguration *bc)
{
    ASSERT_STATE(Inactive);

    m_mountSpecs = mountSpecs;
    m_connection = connection;
    m_devConf = devConf;
    m_mounter->setConnection(m_connection, m_devConf);
    m_buildConfig = bc;
    connect(m_connection.data(), SIGNAL(error(Utils::SshError)),
        SLOT(handleConnectionError()));
    setState(UnmountingOldDirs);
    unmount();
}

void MaemoDeploymentMounter::tearDownMounts()
{
    ASSERT_STATE(Mounted);

    setState(UnmountingCurrentMounts);
    unmount();
}

void MaemoDeploymentMounter::setupMounter()
{
    ASSERT_STATE(UnmountingOldDirs);
    setState(UnmountingCurrentDirs);

    m_mounter->resetMountSpecifications();
    m_mounter->setBuildConfiguration(m_buildConfig);
    foreach (const MaemoMountSpecification &mountSpec, m_mountSpecs)
        m_mounter->addMountSpecification(mountSpec, true);
    unmount();
}

void MaemoDeploymentMounter::unmount()
{
    ASSERT_STATE(QList<State>() << UnmountingOldDirs << UnmountingCurrentDirs
        << UnmountingCurrentMounts);

    if (m_mounter->hasValidMountSpecifications())
        m_mounter->unmount();
    else
        handleUnmounted();
}

void MaemoDeploymentMounter::handleMounted()
{
    ASSERT_STATE(QList<State>() << Mounting << Inactive);

    if (m_state == Inactive)
        return;

    setState(Mounted);
    emit setupDone();
}

void MaemoDeploymentMounter::handleUnmounted()
{
    ASSERT_STATE(QList<State>() << UnmountingOldDirs << UnmountingCurrentDirs
                 << UnmountingCurrentMounts << Inactive);

    switch (m_state) {
    case UnmountingOldDirs:
        setupMounter();
        break;
    case UnmountingCurrentDirs:
        setState(GatheringPorts);
        m_portsGatherer->start(m_connection, m_devConf);
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
    ASSERT_STATE(QList<State>() << GatheringPorts << Inactive);
    if (m_state == Inactive)
        return;

    setState(Inactive);
    m_mounter->resetMountSpecifications();
    emit error(errorMsg);
}

void MaemoDeploymentMounter::handlePortListReady()
{
    ASSERT_STATE(QList<State>() << GatheringPorts << Inactive);
    if (m_state == Inactive)
        return;

    setState(Mounting);
    m_freePorts = MaemoGlobal::freePorts(m_devConf, m_buildConfig->qtVersion());
    m_mounter->mount(&m_freePorts, m_portsGatherer);
}

void MaemoDeploymentMounter::handleMountError(const QString &errorMsg)
{
    ASSERT_STATE(QList<State>() << UnmountingOldDirs << UnmountingCurrentDirs
        << UnmountingCurrentMounts << Mounting << Mounted << Inactive);
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
        disconnect(m_connection.data(), 0, this, 0);
        m_connection.clear();
    }
    m_state = newState;
}

} // namespace Internal
} // namespace RemoteLinux
