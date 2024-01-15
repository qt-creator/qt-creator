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
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>
#include <projectexplorer/kitaspects.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace AppManager::Internal {

#define SETTINGSPREFIX "ApplicationManagerPlugin.Deploy.InstallPackageStep."

const char ArgumentsDefault[] = "install-package -a";

class AppManagerInstallPackageStep final : public AbstractProcessStep
{
public:
    AppManagerInstallPackageStep(BuildStepList *bsl, Id id);

protected:
    bool init() final;

private:
    AppManagerControllerAspect controller{this};
    ProjectExplorer::ArgumentsAspect arguments{this};
    FilePathAspect packageFile{this};
};

AppManagerInstallPackageStep::AppManagerInstallPackageStep(BuildStepList *bsl, Id id)
    : AbstractProcessStep(bsl, id)
{
    setDisplayName(Tr::tr("Install Application Manager package"));

    arguments.setSettingsKey(SETTINGSPREFIX "Arguments");

    packageFile.setSettingsKey(SETTINGSPREFIX "FileName");
    packageFile.setLabelText(Tr::tr("Package file:"));

    const auto updateAspects = [this]  {
        const TargetInformation targetInformation(target());

        controller.setValue(FilePath::fromString(getToolFilePath(Constants::APPMAN_CONTROLLER,
                                                                 target()->kit(),
                                                                 targetInformation.device)));
        controller.setDefaultValue(controller.value());
        arguments.setArguments(ArgumentsDefault);
        arguments.setResetter([] { return QLatin1String(ArgumentsDefault); });

        packageFile.setValue(targetInformation.packageFile.absoluteFilePath());

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
