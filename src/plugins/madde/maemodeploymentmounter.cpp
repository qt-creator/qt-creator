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

#include "maemodeploymentmounter.h"

#include "maemoglobal.h"
#include "maemoremotemounter.h"

#include <projectexplorer/target.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/devicesupport/deviceusedportsgatherer.h>
#include <utils/qtcassert.h>
#include <ssh/sshconnection.h>

using namespace ProjectExplorer;
using namespace QSsh;

namespace Madde {
namespace Internal {

MaemoDeploymentMounter::MaemoDeploymentMounter(QObject *parent)
    : QObject(parent),
      m_state(Inactive),
      m_mounter(new MaemoRemoteMounter(this))
{
    connect(m_mounter, SIGNAL(error(QString)), SLOT(handleMountError(QString)));
    connect(m_mounter, SIGNAL(mounted()), SLOT(handleMounted()));
    connect(m_mounter, SIGNAL(unmounted()), SLOT(handleUnmounted()));
    connect(m_mounter, SIGNAL(reportProgress(QString)),
        SIGNAL(reportProgress(QString)));
    connect(m_mounter, SIGNAL(debugOutput(QString)),
        SIGNAL(debugOutput(QString)));
}

MaemoDeploymentMounter::~MaemoDeploymentMounter() {}

void MaemoDeploymentMounter::setupMounts(SshConnection *connection,
    const QList<MaemoMountSpecification> &mountSpecs,
    const Kit *k)
{
    QTC_ASSERT(m_state == Inactive, return);

    m_mountSpecs = mountSpecs;
    m_connection = connection;
    m_kit = k;
    m_devConf = DeviceKitInformation::device(k);
    m_mounter->setParameters(m_devConf, MaemoGlobal::maddeRoot(k));
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
        setState(Mounting);
        m_mounter->mount();
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
