// Copyright (C) 2019 Luxoft Sweden AB
// Copyright (C) 2018 Pelagicore AG
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "appmanagerinstallpackagestep.h"

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

#include <utils/qtcprocess.h>

using namespace ProjectExplorer;
using namespace Utils;
using namespace Tasking;

namespace AppManager::Internal {

#define SETTINGSPREFIX "ApplicationManagerPlugin.Deploy.InstallPackageStep."

const char ArgumentsDefault[] = "install-package --acknowledge";

class AppManagerInstallPackageStep final : public RemoteLinux::AbstractRemoteLinuxDeployStep
{
public:
    AppManagerInstallPackageStep(BuildStepList *bsl, Id id);

private:
    GroupItem deployRecipe() final;

private:
    AppManagerCustomizeAspect customizeStep{this};
    AppManagerControllerAspect controller{this};
    ProjectExplorer::ArgumentsAspect arguments{this};
    FilePathAspect packageFile{this};
};

AppManagerInstallPackageStep::AppManagerInstallPackageStep(BuildStepList *bsl, Id id)
    : AbstractRemoteLinuxDeployStep(bsl, id)
{
    setDisplayName(Tr::tr("Install Application Manager package"));

    controller.setDefaultPathValue(getToolFilePath(Constants::APPMAN_CONTROLLER,
                                                   target()->kit(),
                                                   DeviceKitAspect::device(target()->kit())));

    arguments.setSettingsKey(SETTINGSPREFIX "Arguments");
    arguments.setResetter([] { return QLatin1String(ArgumentsDefault); });
    arguments.resetArguments();

    packageFile.setSettingsKey(SETTINGSPREFIX "FileName");
    packageFile.setLabelText(Tr::tr("Package file:"));
    packageFile.setEnabler(&customizeStep);

    setInternalInitializer([this] { return isDeploymentPossible(); });

    const auto updateAspects = [this]  {
        if (customizeStep.value())
            return;

        const TargetInformation targetInformation(target());

        IDeviceConstPtr device = DeviceKitAspect::device(kit());
        if (device && device->type() == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE) {
            packageFile.setDefaultPathValue(targetInformation.packageFilePath);
        } else {
            const Utils::FilePath packageFilePath = targetInformation.runDirectory.pathAppended(
                targetInformation.packageFilePath.fileName());
            packageFile.setDefaultPathValue(packageFilePath);
        }

        setStepEnabled(!targetInformation.isBuiltin);
    };

    connect(target(), &Target::activeRunConfigurationChanged, this, updateAspects);
    connect(target(), &Target::activeDeployConfigurationChanged, this, updateAspects);
    connect(target(), &Target::parsingFinished, this, updateAspects);
    connect(target(), &Target::runConfigurationsUpdated, this, updateAspects);
    connect(project(), &Project::displayNameChanged, this, updateAspects);
    connect(&customizeStep, &BaseAspect::changed, this, updateAspects);
    updateAspects();
}

GroupItem AppManagerInstallPackageStep::deployRecipe()
{
    const TargetInformation targetInformation(target());

    const FilePath controllerPath = controller().isEmpty() ?
                                        FilePath::fromString(controller.defaultValue()) :
                                        controller();
    const QString controllerArguments = arguments();
    const FilePath packageFilePath = packageFile().isEmpty() ?
                                         FilePath::fromString(packageFile.defaultValue()) :
                                         packageFile();
    CommandLine cmd(controllerPath);
    cmd.addArgs(controllerArguments, CommandLine::Raw);
    cmd.addArg(packageFilePath.nativePath());

    const auto setupHandler = [this, cmd](Process &process) {
        addProgressMessage(Tr::tr("Starting command \"%1\".").arg(cmd.displayName()));

        process.setCommand(cmd);
        // Prevent the write channel to be closed, otherwise the appman-controller will exit
        process.setProcessMode(ProcessMode::Writer);
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
            addProgressMessage(Tr::tr("Command finished successfully."));
        } else {
            if (process.error() != QProcess::UnknownError
                || process.exitStatus() != QProcess::NormalExit) {
                addErrorMessage(Tr::tr("Process failed: %1").arg(process.errorString()));
            } else if (process.exitCode() != 0) {
                addErrorMessage(Tr::tr("Process finished with exit code %1.")
                                    .arg(process.exitCode()));
            }
        }
    };

    return ProcessTask(setupHandler, doneHandler);
}

// Factory

class AppManagerInstallPackageStepFactory final : public BuildStepFactory
{
public:
    AppManagerInstallPackageStepFactory()
    {
        registerStep<AppManagerInstallPackageStep>(Constants::INSTALL_PACKAGE_STEP_ID);
        setDisplayName(Tr::tr("Install Application Manager package"));
        setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY);
    }
};

void setupAppManagerInstallPackageStep()
{
    static AppManagerInstallPackageStepFactory theAppManagerInstallPackageStepFactory;
}

} // namespace AppManager::Internal
