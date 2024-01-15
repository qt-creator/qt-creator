// Copyright (C) 2019 Luxoft Sweden AB
// Copyright (C) 2018 Pelagicore AG
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "appmanagerremoteinstallpackagestep.h"

#include "appmanagerstringaspect.h"
#include "appmanagerconstants.h"
#include "appmanagertargetinformation.h"
#include "appmanagertr.h"
#include "appmanagerutilities.h"

#include <remotelinux/abstractremotelinuxdeploystep.h>

#include <projectexplorer/buildstep.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/runconfigurationaspects.h>

#include <utils/process.h>

using namespace ProjectExplorer;
using namespace Utils;
using namespace Tasking;

namespace AppManager::Internal {

#define SETTINGSPREFIX "ApplicationManagerPlugin.Deploy.RemoteInstallPackageStep."

const char ArgumentsDefault[] = "install-package -a";

class AppManagerRemoteInstallPackageStep final : public RemoteLinux::AbstractRemoteLinuxDeployStep
{
public:
    AppManagerRemoteInstallPackageStep(BuildStepList *bsl, Id id);

private:
    GroupItem deployRecipe() final;

private:
    AppManagerControllerAspect controller{this};
    ProjectExplorer::ArgumentsAspect arguments{this};
    FilePathAspect packageFile{this};
};

AppManagerRemoteInstallPackageStep::AppManagerRemoteInstallPackageStep(BuildStepList *bsl, Id id)
    : AbstractRemoteLinuxDeployStep(bsl, id)
{
    setDisplayName(tr("Remote Install Application Manager package"));

    arguments.setSettingsKey(SETTINGSPREFIX "Arguments");

    packageFile.setSettingsKey(SETTINGSPREFIX "FileName");
    packageFile.setLabelText(Tr::tr("Package file:"));

    setInternalInitializer([this] { return isDeploymentPossible(); });

    const auto updateAspects = [this]  {
        const TargetInformation targetInformation(target());

        controller.setValue(FilePath::fromString(getToolFilePath(Constants::APPMAN_CONTROLLER,
                                                                 target()->kit(),
                                                                 targetInformation.device)));
        controller.setDefaultValue(controller.value());
        arguments.setArguments(ArgumentsDefault);
        arguments.setResetter([](){ return QLatin1String(ArgumentsDefault); });

        packageFile.setValue(targetInformation.packageFile.absoluteFilePath());
        packageFile.setDefaultValue(packageFile.value());

        setEnabled(!targetInformation.isBuiltin);
    };

    connect(target(), &Target::activeRunConfigurationChanged, this, updateAspects);
    connect(target(), &Target::activeDeployConfigurationChanged, this, updateAspects);
    connect(target(), &Target::parsingFinished, this, updateAspects);
    connect(target(), &Target::runConfigurationsUpdated, this, updateAspects);
    connect(project(), &Project::displayNameChanged, this, updateAspects);
    updateAspects();
}

GroupItem AppManagerRemoteInstallPackageStep::deployRecipe()
{
    const TargetInformation targetInformation(target());

    const FilePath controllerPath = controller().isEmpty() ?
                                        FilePath::fromString(controller.defaultValue()) :
                                        controller();
    const QString controllerArguments = arguments();
    const FilePath packageFilePath = packageFile().isEmpty() ?
                                         FilePath::fromString(packageFile.defaultValue()) :
                                         packageFile();

    const auto setupHandler = [=](Process &process) {
        CommandLine remoteCmd(controllerPath);
        remoteCmd.addArgs(controllerArguments, CommandLine::Raw);
        remoteCmd.addArg(packageFilePath.nativePath());

        CommandLine cmd(deviceConfiguration()->filePath("/bin/sh"));
        cmd.addArg("-c");
        cmd.addCommandLineAsSingleArg(remoteCmd);

        addProgressMessage(Tr::tr("Starting remote command \"%1\"...").arg(cmd.toUserOutput()));
        process.setCommand(cmd);
        Process *proc = &process;
        connect(proc, &Process::readyReadStandardOutput, this, [this, proc] {
            handleStdOutData(proc->readAllStandardOutput());
        });
        connect(proc, &Process::readyReadStandardError, this, [this, proc] {
            handleStdErrData(proc->readAllStandardError());
        });
    };
    const auto doneHandler = [this](const Process &process, DoneWith result) {
        if (result == DoneWith::Success) {
            addProgressMessage(tr("Remote command finished successfully."));
        } else {
            if (process.error() != QProcess::UnknownError
                || process.exitStatus() != QProcess::NormalExit) {
                addErrorMessage(Tr::tr("Remote process failed: %1").arg(process.errorString()));
            } else if (process.exitCode() != 0) {
                addErrorMessage(Tr::tr("Remote process finished with exit code %1.")
                                    .arg(process.exitCode()));
            }
        }
    };

    return ProcessTask(setupHandler, doneHandler);
}

// Factory

class AppManagerRemoteInstallPackageStepFactory final : public BuildStepFactory
{
public:
    AppManagerRemoteInstallPackageStepFactory()
    {
        registerStep<AppManagerRemoteInstallPackageStep>(Constants::REMOTE_INSTALL_PACKAGE_STEP_ID);
        setDisplayName(Tr::tr("Remote Install Application Manager package"));
        setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY);
    }
};

void setupAppManagerRemoteInstallPackageStep()
{
    static AppManagerRemoteInstallPackageStepFactory theAppManagerRemoteInstallPackageStepFactory;
}

} // namespace AppManager::Internal
