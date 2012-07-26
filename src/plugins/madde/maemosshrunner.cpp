/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
#include "maemosshrunner.h"

#include "maemoqemumanager.h"
#include "maemoremotemounter.h"
#include "maemoremotemountsmodel.h"
#include "maemorunconfiguration.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtprofileinformation.h>
#include <utils/qtcassert.h>
#include <ssh/sshconnection.h>

using namespace ProjectExplorer;
using namespace QSsh;
using namespace RemoteLinux;

namespace Madde {
namespace Internal {

MaemoSshRunner::MaemoSshRunner(QObject *parent, MaemoRunConfiguration *runConfig)
    : AbstractRemoteLinuxApplicationRunner(runConfig, parent),
      m_mounter(new MaemoRemoteMounter(this)),
      m_mountSpecs(runConfig->remoteMounts()->mountSpecs()),
      m_mountState(InactiveMountState)
{
    const BuildConfiguration * const bc = runConfig->target()->activeBuildConfiguration();
    Profile *profile  = bc ? bc->target()->profile() : 0;
    m_qtId = QtSupport::QtProfileInformation::qtVersionId(profile);
    m_mounter->setProfile(profile);
    connect(m_mounter, SIGNAL(mounted()), this, SLOT(handleMounted()));
    connect(m_mounter, SIGNAL(unmounted()), this, SLOT(handleUnmounted()));
    connect(m_mounter, SIGNAL(error(QString)), this,
        SLOT(handleMounterError(QString)));
    connect(m_mounter, SIGNAL(reportProgress(QString)), this,
        SIGNAL(reportProgress(QString)));
    connect(m_mounter, SIGNAL(debugOutput(QString)), this,
        SIGNAL(mountDebugOutput(QString)));
}

bool MaemoSshRunner::canRun(QString &whyNot) const
{
    if (!AbstractRemoteLinuxApplicationRunner::canRun(whyNot))
        return false;

    if (devConfig()->machineType() == IDevice::Emulator
            && !MaemoQemuManager::instance().qemuIsRunning()) {
        MaemoQemuRuntime rt;
        if (MaemoQemuManager::instance().runtimeForQtVersion(m_qtId, &rt)) {
            MaemoQemuManager::instance().startRuntime();
            whyNot = tr("Qemu was not running. It has now been started up for you, but it will "
                "take a bit of time until it is ready. Please try again then.");
        } else {
            whyNot = tr("You want to run on Qemu, but it is not enabled for this Qt version.");
        }
        return false;
    }

    return true;
}

void MaemoSshRunner::doDeviceSetup()
{
    QTC_ASSERT(m_mountState == InactiveMountState, return);

    handleDeviceSetupDone(true);
}

void MaemoSshRunner::doAdditionalInitialCleanup()
{
    QTC_ASSERT(m_mountState == InactiveMountState, return);

    m_mounter->setConnection(connection(), devConfig());
    m_mounter->resetMountSpecifications();
    for (int i = 0; i < m_mountSpecs.count(); ++i)
        m_mounter->addMountSpecification(m_mountSpecs.at(i), false);
    m_mountState = InitialUnmounting;
    unmount();
}

void MaemoSshRunner::doAdditionalInitializations()
{
    mount();
}

void MaemoSshRunner::doPostRunCleanup()
{
    QTC_ASSERT(m_mountState == Mounted, return);

    m_mountState = PostRunUnmounting;
    unmount();
}

void MaemoSshRunner::handleUnmounted()
{
    QTC_ASSERT(m_mountState == InitialUnmounting || m_mountState == PostRunUnmounting, return);

    switch (m_mountState) {
    case InitialUnmounting:
        m_mountState = InactiveMountState;
        handleInitialCleanupDone(true);
        break;
    case PostRunUnmounting:
        m_mountState = InactiveMountState;
        handlePostRunCleanupDone();
        break;
    default:
        break;
    }
    m_mountState = InactiveMountState;
}

void MaemoSshRunner::doAdditionalConnectionErrorHandling()
{
    m_mountState = InactiveMountState;
}

void MaemoSshRunner::handleMounted()
{
    QTC_ASSERT(m_mountState == Mounting, return);

    if (m_mountState == Mounting) {
        m_mountState = Mounted;
        handleInitializationsDone(true);
    }
}

void MaemoSshRunner::handleMounterError(const QString &errorMsg)
{
    QTC_ASSERT(m_mountState == InitialUnmounting || m_mountState == Mounting
        || m_mountState == PostRunUnmounting, return);

    const MountState oldMountState = m_mountState;
    m_mountState = InactiveMountState;
    emit error(errorMsg);
    switch (oldMountState) {
    case InitialUnmounting:
        handleInitialCleanupDone(false);
        break;
    case Mounting:
        handleInitializationsDone(false);
        break;
    case PostRunUnmounting:
        handlePostRunCleanupDone();
        break;
    default:
        break;
    }
}

void MaemoSshRunner::mount()
{
    m_mountState = Mounting;
    if (m_mounter->hasValidMountSpecifications()) {
        emit reportProgress(tr("Mounting host directories..."));
        m_mounter->mount(freePorts(), usedPortsGatherer());
    } else {
        handleMounted();
    }
}

void MaemoSshRunner::unmount()
{
    QTC_ASSERT(m_mountState == InitialUnmounting || m_mountState == PostRunUnmounting, return);

    if (m_mounter->hasValidMountSpecifications()) {
        QString message;
        switch (m_mountState) {
        case InitialUnmounting:
            message = tr("Potentially unmounting left-over host directory mounts...");
            break;
        case PostRunUnmounting:
            message = tr("Unmounting host directories...");
            break;
        default:
            break;
        }
        emit reportProgress(message);
        m_mounter->unmount();
    } else {
        handleUnmounted();
    }
}

} // namespace Internal
} // namespace Madde

