// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "customcommanddeploystep.h"

#include "abstractremotelinuxdeploystep.h"
#include "remotelinux_constants.h"
#include "remotelinuxtr.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfigurationaspects.h>

#include <utils/qtcprocess.h>

using namespace ProjectExplorer;
using namespace Utils;
using namespace Utils::Tasking;

namespace RemoteLinux::Internal {

class CustomCommandDeployStep : public AbstractRemoteLinuxDeployStep
{
public:
    CustomCommandDeployStep(BuildStepList *bsl, Id id)
        : AbstractRemoteLinuxDeployStep(bsl, id)
    {
        auto commandLine = addAspect<StringAspect>();
        commandLine->setSettingsKey("RemoteLinuxCustomCommandDeploymentStep.CommandLine");
        commandLine->setLabelText(Tr::tr("Command line:"));
        commandLine->setDisplayStyle(StringAspect::LineEditDisplay);
        commandLine->setHistoryCompleter("RemoteLinuxCustomCommandDeploymentStep.History");

        setInternalInitializer([this, commandLine] {
            m_commandLine = commandLine->value().trimmed();
            return isDeploymentPossible();
        });

        addMacroExpander();
    }

    CheckResult isDeploymentPossible() const final;

private:
    Group deployRecipe() final;

    QString m_commandLine;
};

CheckResult CustomCommandDeployStep::isDeploymentPossible() const
{
    if (m_commandLine.isEmpty())
        return CheckResult::failure(Tr::tr("No command line given."));

    return AbstractRemoteLinuxDeployStep::isDeploymentPossible();
}

Group CustomCommandDeployStep::deployRecipe()
{
    const auto setupHandler = [this](QtcProcess &process) {
        addProgressMessage(Tr::tr("Starting remote command \"%1\"...").arg(m_commandLine));
        process.setCommand({deviceConfiguration()->filePath("/bin/sh"),
                                 {"-c", m_commandLine}});
        QtcProcess *proc = &process;
        connect(proc, &QtcProcess::readyReadStandardOutput, this, [this, proc] {
            handleStdOutData(proc->readAllStandardOutput());
        });
        connect(proc, &QtcProcess::readyReadStandardError, this, [this, proc] {
            handleStdErrData(proc->readAllStandardError());
        });
    };
    const auto doneHandler = [this](const QtcProcess &) {
        addProgressMessage(Tr::tr("Remote command finished successfully."));
    };
    const auto errorHandler = [this](const QtcProcess &process) {
        if (process.error() != QProcess::UnknownError
                || process.exitStatus() != QProcess::NormalExit) {
            addErrorMessage(Tr::tr("Remote process failed: %1").arg(process.errorString()));
        } else if (process.exitCode() != 0) {
            addErrorMessage(Tr::tr("Remote process finished with exit code %1.")
                .arg(process.exitCode()));
        }
    };
    return Group { Process(setupHandler, doneHandler, errorHandler) };
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
