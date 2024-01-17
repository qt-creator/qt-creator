// Copyright (C) 2019 Luxoft Sweden AB
// Copyright (C) 2018 Pelagicore AG
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "appmanagercreatepackagestep.h"

#include "appmanagerconstants.h"
#include "appmanagerstringaspect.h"
#include "appmanagertargetinformation.h"
#include "appmanagertr.h"
#include "appmanagerutilities.h"

#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace AppManager::Internal {

#define SETTINGSPREFIX "ApplicationManagerPlugin.Deploy.CreatePackageStep."

const char ArgumentsDefault[] = "create-package --verbose --json";

class AppManagerCreatePackageStep final : public AbstractProcessStep
{
public:
    AppManagerCreatePackageStep(BuildStepList *bsl, Id id)
        : AbstractProcessStep(bsl, id)
    {
        setDisplayName(Tr::tr("Create Application Manager package"));

        packager.setSettingsKey(SETTINGSPREFIX "Executable");
        packager.setDefaultValue(getToolFilePath(Constants::APPMAN_PACKAGER,
                                                 kit(),
                                                 DeviceKitAspect::device(kit())));

        arguments.setSettingsKey(SETTINGSPREFIX "Arguments");
        arguments.setResetter([] { return QLatin1String(ArgumentsDefault); });
        arguments.resetArguments();

        sourceDirectory.setSettingsKey(SETTINGSPREFIX "SourceDirectory");
        sourceDirectory.setLabelText(Tr::tr("Source directory:"));
        sourceDirectory.setExpectedKind(Utils::PathChooser::ExistingDirectory);

        packageFile.setSettingsKey(SETTINGSPREFIX "FileName");
        packageFile.setLabelText(Tr::tr("Package file:"));
        packageFile.setExpectedKind(Utils::PathChooser::SaveFile);
    }

    bool init() final
    {
        if (!AbstractProcessStep::init())
            return false;

        const FilePath packagerPath = packager().isEmpty() ?
                                          FilePath::fromString(packager.defaultValue()) :
                                          packager();
        const QString packagerArguments = arguments();
        const FilePath sourceDirectoryPath = sourceDirectory().isEmpty() ?
                                                 FilePath::fromString(sourceDirectory.defaultValue()) :
                                                 sourceDirectory();
        const FilePath packageFilePath = packageFile().isEmpty() ?
                                             FilePath::fromString(packageFile.defaultValue()) :
                                             packageFile();


        CommandLine cmd(packagerPath);
        cmd.addArgs(packagerArguments, CommandLine::Raw);
        cmd.addArgs({packageFilePath.nativePath(), sourceDirectoryPath.nativePath()});
        processParameters()->setCommandLine(cmd);

        return true;
    }

private:
    AppManagerPackagerAspect packager{this};
    ProjectExplorer::ArgumentsAspect arguments{this};
    FilePathAspect sourceDirectory{this};
    FilePathAspect packageFile{this};
};

// Factory

class AppManagerCreatePackageStepFactory final : public BuildStepFactory
{
public:
    AppManagerCreatePackageStepFactory()
    {
        registerStep<AppManagerCreatePackageStep>(Constants::CREATE_PACKAGE_STEP_ID);
        setDisplayName(Tr::tr("Create Application Manager package"));
        setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY);
    }
};

void setupAppManagerCreatePackageStep()
{
    static AppManagerCreatePackageStepFactory theAppManagerCreatePackageStepFactory;
}

} // AppManager::Internal
