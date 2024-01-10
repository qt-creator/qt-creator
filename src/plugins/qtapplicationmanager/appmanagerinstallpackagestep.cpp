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

#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/kitaspects.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace AppManager::Internal {

#define SETTINGSPREFIX "ApplicationManagerPlugin.Deploy.InstallPackageStep."

const char ArgumentsDefault[] = "install-package -a";

class AppManagerInstallPackageStep final : public AbstractProcessStep
{
    Q_DECLARE_TR_FUNCTIONS(AppManager::Internal::AppManagerInstallPackageStep)

public:
    AppManagerInstallPackageStep(BuildStepList *bsl, Id id);

protected:
    bool init() final;

private:
    AppManagerFilePathAspect executable{this};
    AppManagerStringAspect arguments{this};
    AppManagerStringAspect packageFileName{this};
    AppManagerFilePathAspect packageDirectory{this};
};

AppManagerInstallPackageStep::AppManagerInstallPackageStep(BuildStepList *bsl, Id id)
    : AbstractProcessStep(bsl, id)
{
    setDisplayName(Tr::tr("Install Application Manager package"));

    executable.setSettingsKey(SETTINGSPREFIX "Executable");
    executable.setHistoryCompleter(SETTINGSPREFIX "Executable.History");
    executable.setLabelText(Tr::tr("Executable:"));

    arguments.setSettingsKey(SETTINGSPREFIX "Arguments");
    arguments.setHistoryCompleter(SETTINGSPREFIX "Arguments.History");
    arguments.setDisplayStyle(StringAspect::LineEditDisplay);
    arguments.setLabelText(Tr::tr("Arguments:"));

    packageFileName.setSettingsKey(SETTINGSPREFIX "FileName");
    packageFileName.setHistoryCompleter(SETTINGSPREFIX "FileName.History");
    packageFileName.setDisplayStyle(StringAspect::LineEditDisplay);
    packageFileName.setLabelText(Tr::tr("File name:"));

    packageDirectory.setSettingsKey(SETTINGSPREFIX "Directory");
    packageDirectory.setHistoryCompleter(SETTINGSPREFIX "Directory.History");
    packageDirectory.setExpectedKind(PathChooser::Directory);
    packageDirectory.setLabelText(Tr::tr("Directory:"));

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

bool AppManagerInstallPackageStep::init()
{
    if (!AbstractProcessStep::init())
        return false;

    const TargetInformation targetInformation(target());
    if (!targetInformation.isValid())
        return false;

    const FilePath controller = executable.valueOrDefault(getToolFilePath(Constants::APPMAN_CONTROLLER, target()->kit(), targetInformation.device));
    const QString controllerArguments = arguments.valueOrDefault(ArgumentsDefault);
    const QString packageFile = packageFileName.valueOrDefault(targetInformation.packageFile.fileName());
    auto device = DeviceKitAspect::device(target()->kit());
    bool remote = device && device->type() != ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE;
    QDir packageDirectoryPath = remote ? QDir(Constants::REMOTE_DEFAULT_TMP_PATH) : targetInformation.packageFile.absolutePath();
    const FilePath packageDir = packageDirectory.valueOrDefault(packageDirectoryPath.absolutePath());

    CommandLine cmd(targetInformation.device->filePath(controller.path()));
    cmd.addArgs(controllerArguments, CommandLine::Raw);
    cmd.addArg(packageFile);
    processParameters()->setWorkingDirectory(packageDir);
    processParameters()->setCommandLine(cmd);

    return true;
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
