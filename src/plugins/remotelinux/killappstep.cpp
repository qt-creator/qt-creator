/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "killappstep.h"

#include "abstractremotelinuxdeploystep.h"
#include "abstractremotelinuxdeployservice.h"
#include "remotelinux_constants.h"
#include "remotelinuxtr.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/target.h>

#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux::Internal {

class KillAppService : public AbstractRemoteLinuxDeployService
{
public:
    ~KillAppService() override;

    void setRemoteExecutable(const QString &filePath);

private:
    void handleStdErr();
    void handleProcessFinished();

    bool isDeploymentNecessary() const override;

    void doDeploy() override;
    void stopDeployment() override;

    void handleSignalOpFinished(const QString &errorMessage);
    void cleanup();
    void finishDeployment();

    QString m_remoteExecutable;
    DeviceProcessSignalOperation::Ptr m_signalOperation;
};

KillAppService::~KillAppService()
{
    cleanup();
}

void KillAppService::setRemoteExecutable(const QString &filePath)
{
    m_remoteExecutable = filePath;
}

bool KillAppService::isDeploymentNecessary() const
{
    return !m_remoteExecutable.isEmpty();
}

void KillAppService::doDeploy()
{
    m_signalOperation = deviceConfiguration()->signalOperation();
    if (!m_signalOperation) {
        handleDeploymentDone();
        return;
    }
    connect(m_signalOperation.data(), &DeviceProcessSignalOperation::finished,
            this, &KillAppService::handleSignalOpFinished);
    emit progressMessage(Tr::tr("Trying to kill \"%1\" on remote device...").arg(m_remoteExecutable));
    m_signalOperation->killProcess(m_remoteExecutable);
}

void KillAppService::cleanup()
{
    if (m_signalOperation) {
        disconnect(m_signalOperation.data(), nullptr, this, nullptr);
        m_signalOperation.clear();
    }
}

void KillAppService::finishDeployment()
{
    cleanup();
    handleDeploymentDone();
}

void KillAppService::stopDeployment()
{
    finishDeployment();
}

void KillAppService::handleSignalOpFinished(const QString &errorMessage)
{
    if (errorMessage.isEmpty())
        emit progressMessage(Tr::tr("Remote application killed."));
    else
        emit progressMessage(Tr::tr("Failed to kill remote application. Assuming it was not running."));
    finishDeployment();
}

class KillAppStep : public AbstractRemoteLinuxDeployStep
{
public:
    KillAppStep(BuildStepList *bsl, Id id) : AbstractRemoteLinuxDeployStep(bsl, id)
    {
        auto service = createDeployService<Internal::KillAppService>();

        setWidgetExpandedByDefault(false);

        setInternalInitializer([this, service] {
            Target * const theTarget = target();
            QTC_ASSERT(theTarget, return CheckResult::failure());
            RunConfiguration * const rc = theTarget->activeRunConfiguration();
            const QString remoteExe = rc ? rc->runnable().command.executable().toString() : QString();
            service->setRemoteExecutable(remoteExe);
            return CheckResult::success();
        });
    }
};

KillAppStepFactory::KillAppStepFactory()
{
    registerStep<KillAppStep>(Constants::KillAppStepId);
    setDisplayName(Tr::tr("Kill current application instance"));
    setSupportedConfiguration(RemoteLinux::Constants::DeployToGenericLinux);
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY);
}

} // RemoteLinux::Internal
