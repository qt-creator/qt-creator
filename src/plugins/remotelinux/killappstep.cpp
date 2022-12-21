// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    bool isDeploymentNecessary() const final { return !m_remoteExecutable.isEmpty(); }
    Group deployRecipe() final;

    FilePath m_remoteExecutable;
};

Group KillAppService::deployRecipe()
{
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
    return Group { Killer(setupHandler, doneHandler, errorHandler) };
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
