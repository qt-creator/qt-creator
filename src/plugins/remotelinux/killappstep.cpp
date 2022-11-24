// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

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
using namespace Utils::Tasking;

namespace RemoteLinux::Internal {

class KillAppService : public AbstractRemoteLinuxDeployService
{
public:
    void setRemoteExecutable(const FilePath &filePath) { m_remoteExecutable = filePath; }

private:
    bool isDeploymentNecessary() const override { return !m_remoteExecutable.isEmpty(); }

    void doDeploy() override;
    void stopDeployment() override;

    FilePath m_remoteExecutable;
    std::unique_ptr<TaskTree> m_taskTree;
};

void KillAppService::doDeploy()
{
    QTC_ASSERT(!m_taskTree, return);

    const auto setupHandler = [this](DeviceProcessKiller &killer) {
        killer.setProcessPath(m_remoteExecutable);
        emit progressMessage(Tr::tr("Trying to kill \"%1\" on remote device...")
                             .arg(m_remoteExecutable.path()));
    };
    const auto doneHandler = [this](const DeviceProcessKiller &) {
        emit progressMessage(Tr::tr("Remote application killed."));
    };
    const auto errorHandler = [this](const DeviceProcessKiller &) {
        emit progressMessage(Tr::tr("Failed to kill remote application. "
                                    "Assuming it was not running."));
    };

    const auto endHandler = [this] {
        m_taskTree.release()->deleteLater();
        stopDeployment();
    };

    const Group root {
        Killer(setupHandler, doneHandler, errorHandler),
        OnGroupDone(endHandler),
        OnGroupError(endHandler)
    };
    m_taskTree.reset(new TaskTree(root));
    m_taskTree->start();
}

void KillAppService::stopDeployment()
{
    m_taskTree.reset();
    handleDeploymentDone();
}

class KillAppStep : public AbstractRemoteLinuxDeployStep
{
public:
    KillAppStep(BuildStepList *bsl, Id id) : AbstractRemoteLinuxDeployStep(bsl, id)
    {
        auto service = new Internal::KillAppService;
        setDeployService(service);

        setWidgetExpandedByDefault(false);

        setInternalInitializer([this, service] {
            Target * const theTarget = target();
            QTC_ASSERT(theTarget, return CheckResult::failure());
            RunConfiguration * const rc = theTarget->activeRunConfiguration();
            const FilePath remoteExe = rc ? rc->runnable().command.executable() : FilePath();
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
