// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "killappstep.h"

#include "abstractremotelinuxdeploystep.h"
#include "remotelinux_constants.h"
#include "remotelinuxtr.h"

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>

#include <utils/processinterface.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace QtTaskTree;
using namespace Utils;

namespace RemoteLinux::Internal {

class KillAppStep : public AbstractRemoteLinuxDeployStep
{
public:
    KillAppStep(BuildStepList *bsl, Id id) : AbstractRemoteLinuxDeployStep(bsl, id)
    {
        setWidgetExpandedByDefault(false);

        setInternalInitializer([this]() -> Result<> {
            BuildConfiguration * const bc = buildConfiguration();
            QTC_ASSERT(bc, return make_unexpected(QString()));
            RunConfiguration * const rc = bc->activeRunConfiguration();
            m_remoteExecutable =  rc ? rc->runnable().command.executable() : FilePath();
            return {};
        });
    }

private:
    GroupItem deployRecipe() final;

    FilePath m_remoteExecutable;
};

GroupItem KillAppStep::deployRecipe()
{
    if (m_remoteExecutable.isEmpty())
        return QSyncTask([this] { addSkipDeploymentMessage(); });

    const IDevice::ConstPtr device = DeviceManager::deviceForPath(m_remoteExecutable);
    if (!device) {
        return QSyncTask([this] {
            addProgressMessage(Tr::tr("No device for the path: \"%1\".")
                               .arg(m_remoteExecutable.toUserOutput()));
        });
    }

    const SignalOperationData data{.mode = SignalOperationMode::KillByPath,
                                   .filePath = m_remoteExecutable};
    const Storage<Utils::Result<>> resultStorage;

    const auto onSetup = [this] {
        addProgressMessage(Tr::tr("Trying to kill \"%1\" on remote device...")
                               .arg(m_remoteExecutable.path()));
    };
    const auto onDone = [this, resultStorage] {
        const QString message = *resultStorage ? Tr::tr("Remote application killed.")
            : resultStorage->error();
        addProgressMessage(message);
        return DoneResult::Success;
    };

    return Group {
        resultStorage,
        onGroupSetup(onSetup),
        device->signalOperationRecipe(data, resultStorage),
        onGroupDone(onDone)
    };
}

class KillAppStepFactory final : public BuildStepFactory
{
public:
    KillAppStepFactory()
    {
        registerStep<KillAppStep>(Constants::KillAppStepId);
        setDisplayName(Tr::tr("Kill current application instance"));
        setSupportedConfiguration(RemoteLinux::Constants::DeployToGenericLinux);
        setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY);
    }
};

void setupKillAppStep()
{
    static KillAppStepFactory theKillAppStepFactory;
}

} // RemoteLinux::Internal
