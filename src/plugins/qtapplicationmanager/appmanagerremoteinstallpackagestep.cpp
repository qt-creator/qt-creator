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

#include <utils/process.h>

using namespace ProjectExplorer;
using namespace Utils;
using namespace Tasking;

namespace AppManager::Internal {

#define SETTINGSPREFIX "ApplicationManagerPlugin.Deploy.RemoteInstallPackageStep."

const char ArgumentsDefault[] = "install-package -a";

class AppManagerRemoteInstallPackageStep final : public RemoteLinux::AbstractRemoteLinuxDeployStep
{
    Q_DECLARE_TR_FUNCTIONS(AppManager::Internal::AppManagerInstallPackageStep)

public:
    AppManagerRemoteInstallPackageStep(BuildStepList *bsl, Id id);

private:
    GroupItem deployRecipe() final;

private:
    AppManagerFilePathAspect executable{this};
    AppManagerStringAspect arguments{this};
    AppManagerStringAspect packageFileName{this};
    AppManagerFilePathAspect packageDirectory{this};
};

AppManagerRemoteInstallPackageStep::AppManagerRemoteInstallPackageStep(BuildStepList *bsl, Id id)
    : AbstractRemoteLinuxDeployStep(bsl, id)
{
    setDisplayName(tr("Remote Install Application Manager package"));

    executable.setSettingsKey(SETTINGSPREFIX "Executable");
    executable.setHistoryCompleter(SETTINGSPREFIX "Executable.History");
    executable.setLabelText(tr("Executable:"));

    arguments.setSettingsKey(SETTINGSPREFIX "Arguments");
    arguments.setHistoryCompleter(SETTINGSPREFIX "Arguments.History");
    arguments.setDisplayStyle(StringAspect::LineEditDisplay);
    arguments.setLabelText(tr("Arguments:"));

    packageFileName.setSettingsKey(SETTINGSPREFIX "FileName");
    packageFileName.setHistoryCompleter(SETTINGSPREFIX "FileName.History");
    packageFileName.setDisplayStyle(StringAspect::LineEditDisplay);
    packageFileName.setLabelText(tr("File name:"));

    packageDirectory.setSettingsKey(SETTINGSPREFIX "Directory");
    packageDirectory.setHistoryCompleter(SETTINGSPREFIX "Directory.History");
    packageDirectory.setExpectedKind(PathChooser::Directory);
    packageDirectory.setLabelText(tr("Directory:"));

    setInternalInitializer([this] { return isDeploymentPossible(); });

    const auto updateAspects = [this]  {
        const TargetInformation targetInformation(target());

        executable.setPromptDialogFilter(getToolNameByDevice(Constants::APPMAN_CONTROLLER, targetInformation.device));
        executable.setButtonsVisible(!targetInformation.remote);
        executable.setExpectedKind(targetInformation.remote ? PathChooser::Command : PathChooser::ExistingCommand);
        executable.setPlaceHolderPath(getToolFilePath(Constants::APPMAN_CONTROLLER, target()->kit(), targetInformation.device));
        arguments.setPlaceHolderText(ArgumentsDefault);
        packageFileName.setPlaceHolderText(targetInformation.packageFile.fileName());
        auto device = DeviceKitAspect::device(target()->kit());
        bool remote = device && device->type() != ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE;
        QDir packageDirectoryPath = remote ? QDir(Constants::REMOTE_DEFAULT_TMP_PATH) : targetInformation.packageFile.absolutePath();
        packageDirectory.setPlaceHolderPath(packageDirectoryPath.absolutePath());
        packageDirectory.setButtonsVisible(!targetInformation.remote);

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

    const FilePath controller = executable.valueOrDefault(getToolFilePath(Constants::APPMAN_CONTROLLER, target()->kit(), targetInformation.device));
    const QString controllerArguments = arguments.valueOrDefault(ArgumentsDefault);
    const QString packageFile = packageFileName.valueOrDefault(targetInformation.packageFile.fileName());
    auto device = DeviceKitAspect::device(target()->kit());
    bool remote = device && device->type() != ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE;
    QDir packageDirectoryPath = remote ? QDir(Constants::REMOTE_DEFAULT_TMP_PATH) : targetInformation.packageFile.absolutePath();
    const FilePath packageDir = packageDirectory.valueOrDefault(packageDirectoryPath.absolutePath());

    const auto setupHandler = [=](Process &process) {
        CommandLine remoteCmd(controller);
        remoteCmd.addArgs(controllerArguments, CommandLine::Raw);
        remoteCmd.addArg(packageDir.toString() + '/' + packageFile);

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
