// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "customcommanddeploystep.h"

#include "abstractremotelinuxdeploystep.h"
#include "remotelinux_constants.h"
#include "remotelinuxtr.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfigurationaspects.h>

#include <utils/process.h>

using namespace ProjectExplorer;
using namespace Tasking;
using namespace Utils;

namespace RemoteLinux::Internal {

class CustomCommandDeployStep : public AbstractRemoteLinuxDeployStep
{
public:
    CustomCommandDeployStep(BuildStepList *bsl, Id id)
        : AbstractRemoteLinuxDeployStep(bsl, id)
    {
        commandLine.setSettingsKey("RemoteLinuxCustomCommandDeploymentStep.CommandLine");
        commandLine.setLabelText(Tr::tr("Command line:"));
        commandLine.setDisplayStyle(StringAspect::LineEditDisplay);
        commandLine.setHistoryCompleter("RemoteLinuxCustomCommandDeploymentStep.History");

        setInternalInitializer([this] { return isDeploymentPossible(); });

        addMacroExpander();
    }

    expected_str<void> isDeploymentPossible() const final;

private:
    GroupItem deployRecipe() final;

    StringAspect commandLine{this};
};

expected_str<void> CustomCommandDeployStep::isDeploymentPossible() const
{
    if (commandLine().isEmpty())
        return make_unexpected(Tr::tr("No command line given."));

    return AbstractRemoteLinuxDeployStep::isDeploymentPossible();
}

GroupItem CustomCommandDeployStep::deployRecipe()
{
    const auto setupHandler = [this](Process &process) {
        addProgressMessage(Tr::tr("Starting remote command \"%1\"...").arg(commandLine()));
        process.setCommand({deviceConfiguration()->filePath("/bin/sh"),
                                 {"-c", commandLine()}});
        Process *proc = &process;
        connect(proc, &Process::readyReadStandardOutput, this, [this, proc] {
            handleStdOutData(proc->readAllStandardOutput());
        });
        connect(proc, &Process::readyReadStandardError, this, [this, proc] {
            handleStdErrData(proc->readAllStandardError());
        });
    };
    const auto doneHandler = [this](const Process &) {
        addProgressMessage(Tr::tr("Remote command finished successfully."));
    };
    const auto errorHandler = [this](const Process &process) {
        if (process.error() != QProcess::UnknownError
                || process.exitStatus() != QProcess::NormalExit) {
            addErrorMessage(Tr::tr("Remote process failed: %1").arg(process.errorString()));
        } else if (process.exitCode() != 0) {
            addErrorMessage(Tr::tr("Remote process finished with exit code %1.")
                .arg(process.exitCode()));
        }
    };
    return ProcessTask(setupHandler, doneHandler, errorHandler);
}


// CustomCommandDeployStepFactory

CustomCommandDeployStepFactory::CustomCommandDeployStepFactory()
{
    registerStep<CustomCommandDeployStep>(Constants::CustomCommandDeployStepId);
    setDisplayName(Tr::tr("Run custom remote command"));
    setSupportedConfiguration(RemoteLinux::Constants::DeployToGenericLinux);
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY);
}

} // RemoteLinux::Internal
