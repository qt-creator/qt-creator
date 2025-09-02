// Copyright (C) 2019 Luxoft Sweden AB
// Copyright (C) 2018 Pelagicore AG
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "appmanagerdeploypackagestep.h"

#include "appmanagerconstants.h"
#include "appmanagerstringaspect.h"
#include "appmanagertargetinformation.h"
#include "appmanagertr.h"

#include <projectexplorer/buildstep.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>

#include <utils/filestreamer.h>

using namespace ProjectExplorer;
using namespace Tasking;
using namespace Utils;

namespace AppManager::Internal {

#define SETTINGSPREFIX "ApplicationManagerPlugin.Deploy.DeployPackageStep."

class AppManagerDeployPackageStep : public BuildStep
{
public:
    AppManagerDeployPackageStep(BuildStepList *bsl, Id id)
        : BuildStep(bsl, id)
    {
        setDisplayName(Tr::tr("Deploy Application Manager package"));

        packageFilePath.setSettingsKey(SETTINGSPREFIX "FilePath");
        packageFilePath.setLabelText(Tr::tr("Package file:"));
        packageFilePath.setEnabler(&customizeStep);

        targetDirectory.setSettingsKey(SETTINGSPREFIX "TargetDirectory");
        targetDirectory.setLabelText(Tr::tr("Target directory:"));
        targetDirectory.setEnabler(&customizeStep);

        const auto updateAspects = [this] {
            if (customizeStep.value())
                return;

            const TargetInformation targetInformation(buildConfiguration());

            packageFilePath.setValue(targetInformation.packageFilePath);
            packageFilePath.setDefaultValue(packageFilePath.value());

            targetDirectory.setValue(targetInformation.runDirectory);
            targetDirectory.setDefaultValue(targetDirectory.value());

            setStepEnabled(!targetInformation.isBuiltin);
        };

        connect(buildConfiguration(), &BuildConfiguration::activeRunConfigurationChanged,
                this, updateAspects);
        connect(buildConfiguration(), &BuildConfiguration::activeDeployConfigurationChanged,
                this, updateAspects);
        connect(buildSystem(), &BuildSystem::parsingFinished, this, updateAspects);
        connect(buildConfiguration(), &BuildConfiguration::runConfigurationsUpdated,
                this, updateAspects);
        connect(project(), &Project::displayNameChanged, this, updateAspects);
        connect(&customizeStep, &BaseAspect::changed, this, updateAspects);

        updateAspects();
    }

private:
    bool init() final
    {
        return TargetInformation(buildConfiguration()).isValid();
    }

    GroupItem runRecipe() final
    {
        const auto onSetup = [this](FileStreamer &streamer) {
            const FilePath source = packageFilePath().isEmpty() ?
                                        FilePath::fromString(packageFilePath.defaultValue()) :
                                        packageFilePath();
            const FilePath targetDir = targetDirectory().isEmpty() ?
                                           FilePath::fromString(targetDirectory.defaultValue()) :
                                           targetDirectory();
            const FilePath target = RunDeviceKitAspect::device(kit())->filePath(targetDir.path())
                                        .pathAppended(source.fileName());
            streamer.setSource(source);
            streamer.setDestination(target);
            emit addOutput("Starting uploading", OutputFormat::NormalMessage);
        };
        const auto onDone = [this](DoneWith result) {
            if (result == DoneWith::Success)
                emit addOutput(Tr::tr("Uploading finished."), OutputFormat::NormalMessage);
            else
                emit addOutput(Tr::tr("Uploading failed."), OutputFormat::ErrorMessage);
        };
        return FileStreamerTask(onSetup, onDone);
    }

    AppManagerCustomizeAspect customizeStep{this};
    FilePathAspect packageFilePath{this};
    FilePathAspect targetDirectory{this};
};

// Factory

class AppManagerDeployPackageStepFactory final : public BuildStepFactory
{
public:
    AppManagerDeployPackageStepFactory()
    {
        registerStep<AppManagerDeployPackageStep>(Constants::DEPLOY_PACKAGE_STEP_ID);
        setDisplayName(Tr::tr("Deploy Application Manager package"));
        setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY);
    }
};

void setupAppManagerDeployPackageStep()
{
   static AppManagerDeployPackageStepFactory theAppManagerDeployPackageStepFactory;
}

} // namespace AppManager::Internal
