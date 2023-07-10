// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qdbmakedefaultappstep.h"

#include "qdbconstants.h"
#include "qdbtr.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>

#include <remotelinux/abstractremotelinuxdeploystep.h>

#include <utils/commandline.h>
#include <utils/process.h>

using namespace ProjectExplorer;
using namespace Tasking;
using namespace Utils;

namespace Qdb::Internal {

class QdbMakeDefaultAppStep final : public RemoteLinux::AbstractRemoteLinuxDeployStep
{
public:
    QdbMakeDefaultAppStep(BuildStepList *bsl, Id id)
        : AbstractRemoteLinuxDeployStep(bsl, id)
    {
        selection.setSettingsKey("QdbMakeDefaultDeployStep.MakeDefault");
        selection.addOption(Tr::tr("Set this application to start by default"));
        selection.addOption(Tr::tr("Reset default application"));

        setInternalInitializer([this] { return isDeploymentPossible(); });
    }

private:
    GroupItem deployRecipe() final
    {
        const auto setupHandler = [this](Process &process) {
            QString remoteExe;
            if (RunConfiguration *rc = target()->activeRunConfiguration()) {
                if (auto exeAspect = rc->aspect<ExecutableAspect>())
                    remoteExe = exeAspect->executable().nativePath();
            }
            CommandLine cmd{deviceConfiguration()->filePath(Constants::AppcontrollerFilepath)};
            if (selection() == 0 && !remoteExe.isEmpty())
                cmd.addArgs({"--make-default", remoteExe});
            else
                cmd.addArg("--remove-default");
            process.setCommand(cmd);
            Process *proc = &process;
            connect(proc, &Process::readyReadStandardError, this, [this, proc] {
                handleStdErrData(proc->readAllStandardError());
            });
        };
        const auto doneHandler = [this](const Process &) {
            if (selection() == 0)
                addProgressMessage(Tr::tr("Application set as the default one."));
            else
                addProgressMessage(Tr::tr("Reset the default application."));
        };
        const auto errorHandler = [this](const Process &process) {
            addErrorMessage(Tr::tr("Remote process failed: %1").arg(process.errorString()));
        };
        return ProcessTask(setupHandler, doneHandler, errorHandler);
    }

    SelectionAspect selection{this};
};

// QdbMakeDefaultAppStepFactory

QdbMakeDefaultAppStepFactory::QdbMakeDefaultAppStepFactory()
{
    registerStep<QdbMakeDefaultAppStep>(Constants::QdbMakeDefaultAppStepId);
    setDisplayName(Tr::tr("Change default application"));
    setSupportedDeviceType(Qdb::Constants::QdbLinuxOsType);
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY);
}

} // Qdb::Internal
