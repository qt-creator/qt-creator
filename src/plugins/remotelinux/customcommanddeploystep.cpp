// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "customcommanddeploystep.h"

#include "abstractremotelinuxdeployservice.h"
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

class CustomCommandDeployService : public AbstractRemoteLinuxDeployService
{
public:
    void setCommandLine(const QString &commandLine);
    CheckResult isDeploymentPossible() const final;

protected:
    Group deployRecipe() final;

private:
    bool isDeploymentNecessary() const final { return true; }

    QString m_commandLine;
};

void CustomCommandDeployService::setCommandLine(const QString &commandLine)
{
    m_commandLine = commandLine;
}

CheckResult CustomCommandDeployService::isDeploymentPossible() const
{
    if (m_commandLine.isEmpty())
        return CheckResult::failure(Tr::tr("No command line given."));

    return AbstractRemoteLinuxDeployService::isDeploymentPossible();
}

Group CustomCommandDeployService::deployRecipe()
{
    const auto setupHandler = [this](QtcProcess &process) {
        emit progressMessage(Tr::tr("Starting remote command \"%1\"...").arg(m_commandLine));
        process.setCommand({deviceConfiguration()->filePath("/bin/sh"),
                                 {"-c", m_commandLine}});
        QtcProcess *proc = &process;
        connect(proc, &QtcProcess::readyReadStandardOutput, this, [this, proc] {
            emit stdOutData(proc->readAllStandardOutput());
        });
        connect(proc, &QtcProcess::readyReadStandardError, this, [this, proc] {
            emit stdErrData(proc->readAllStandardError());
        });
    };
    const auto doneHandler = [this](const QtcProcess &) {
        emit progressMessage(Tr::tr("Remote command finished successfully."));
    };
    const auto errorHandler = [this](const QtcProcess &process) {
        if (process.error() != QProcess::UnknownError
                || process.exitStatus() != QProcess::NormalExit) {
            emit errorMessage(Tr::tr("Remote process failed: %1").arg(process.errorString()));
        } else if (process.exitCode() != 0) {
            emit errorMessage(Tr::tr("Remote process finished with exit code %1.")
                .arg(process.exitCode()));
        }
    };
    return Group { Process(setupHandler, doneHandler, errorHandler) };
}

class CustomCommandDeployStep : public AbstractRemoteLinuxDeployStep
{
public:
    CustomCommandDeployStep(BuildStepList *bsl, Id id)
        : AbstractRemoteLinuxDeployStep(bsl, id)
    {
        auto service = new CustomCommandDeployService;
        setDeployService(service);

        auto commandLine = addAspect<StringAspect>();
        commandLine->setSettingsKey("RemoteLinuxCustomCommandDeploymentStep.CommandLine");
        commandLine->setLabelText(Tr::tr("Command line:"));
        commandLine->setDisplayStyle(StringAspect::LineEditDisplay);
        commandLine->setHistoryCompleter("RemoteLinuxCustomCommandDeploymentStep.History");

        setInternalInitializer([service, commandLine] {
            service->setCommandLine(commandLine->value().trimmed());
            return service->isDeploymentPossible();
        });

        addMacroExpander();
    }
};


// CustomCommandDeployStepFactory

CustomCommandDeployStepFactory::CustomCommandDeployStepFactory()
{
    registerStep<CustomCommandDeployStep>(Constants::CustomCommandDeployStepId);
    setDisplayName(Tr::tr("Run custom remote command"));
    setSupportedConfiguration(RemoteLinux::Constants::DeployToGenericLinux);
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY);
}

} // RemoteLinux::Internal
