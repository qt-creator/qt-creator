// Copyright (C) 2019 Luxoft Sweden AB
// Copyright (C) 2018 Pelagicore AG
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "appmanagercreatepackagestep.h"

#include "appmanagerconstants.h"
#include "appmanagerstringaspect.h"
#include "appmanagertargetinformation.h"
#include "appmanagerutilities.h"

#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace AppManager {
namespace Internal {

#define SETTINGSPREFIX "ApplicationManagerPlugin.Deploy.CreatePackageStep."

const char ArgumentsDefault[] = "create-package --verbose --json";

class AppManagerCreatePackageStep final : public AbstractProcessStep
{
public:
    AppManagerCreatePackageStep(BuildStepList *bsl, Id id);

    bool init() final;

private:
    AppManagerFilePathAspect executable{this};
    AppManagerStringAspect arguments{this};
    AppManagerFilePathAspect sourceDirectory{this};
    AppManagerFilePathAspect buildDirectory{this};
    AppManagerStringAspect packageFileName{this};
};

AppManagerCreatePackageStep::AppManagerCreatePackageStep(BuildStepList *bsl, Id id)
    : AbstractProcessStep(bsl, id)
{
    setDisplayName(tr("Create Application Manager package"));

    executable.setSettingsKey(SETTINGSPREFIX "Executable");
    executable.setHistoryCompleter(SETTINGSPREFIX "Executable.History");
    executable.setExpectedKind(PathChooser::ExistingCommand);
    executable.setLabelText(tr("Executable:"));
    executable.setPromptDialogFilter(getToolNameByDevice(Constants::APPMAN_PACKAGER));

    arguments.setSettingsKey(SETTINGSPREFIX "Arguments");
    arguments.setHistoryCompleter(SETTINGSPREFIX "Arguments.History");
    arguments.setDisplayStyle(StringAspect::LineEditDisplay);
    arguments.setLabelText(tr("Arguments:"));

    sourceDirectory.setSettingsKey(SETTINGSPREFIX "SourceDirectory");
    sourceDirectory.setHistoryCompleter(SETTINGSPREFIX "SourceDirectory.History");
    sourceDirectory.setExpectedKind(PathChooser::Directory);
    sourceDirectory.setLabelText(tr("Source directory:"));

    buildDirectory.setSettingsKey(SETTINGSPREFIX "BuildDirectory");
    buildDirectory.setHistoryCompleter(SETTINGSPREFIX "BuildDirectory.History");
    buildDirectory.setExpectedKind(PathChooser::Directory);
    buildDirectory.setLabelText(tr("Build directory:"));

    packageFileName.setSettingsKey(SETTINGSPREFIX "FileName");
    packageFileName.setHistoryCompleter(SETTINGSPREFIX "FileName.History");
    packageFileName.setDisplayStyle(StringAspect::LineEditDisplay);
    packageFileName.setLabelText(tr("Package file name:"));

    const auto updateAspects = [this] {
        const auto targetInformation = TargetInformation(target());

        executable.setPlaceHolderPath(getToolFilePath(Constants::APPMAN_PACKAGER, target()->kit(), nullptr));
        arguments.setPlaceHolderText(ArgumentsDefault);
        sourceDirectory.setPlaceHolderPath(targetInformation.packageSourcesDirectory.absolutePath());
        buildDirectory.setPlaceHolderPath(targetInformation.buildDirectory.absolutePath());
        packageFileName.setPlaceHolderText(targetInformation.packageFile.fileName());
    };

    connect(target(), &Target::activeRunConfigurationChanged, this, updateAspects);
    connect(target(), &Target::activeDeployConfigurationChanged, this, updateAspects);
    connect(target(), &Target::parsingFinished, this, updateAspects);
    connect(project(), &Project::displayNameChanged, this, updateAspects);
    updateAspects();
}

bool AppManagerCreatePackageStep::init()
{
    if (!AbstractProcessStep::init())
        return false;

    const auto targetInformation = TargetInformation(target());
    if (!targetInformation.isValid())
        return false;

    const FilePath packager = executable.valueOrDefault(getToolFilePath(Constants::APPMAN_PACKAGER, target()->kit(), nullptr));
    const QString packagerArguments = arguments.valueOrDefault(ArgumentsDefault);
    const FilePath packageSourcesDirectory = sourceDirectory.valueOrDefault(targetInformation.packageSourcesDirectory.absolutePath());
    const FilePath packageDirectory = buildDirectory.valueOrDefault(targetInformation.buildDirectory.absolutePath());
    const QString packageFile = packageFileName.valueOrDefault(targetInformation.packageFile.fileName());

    CommandLine cmd(packager);
    cmd.addArgs(packagerArguments, CommandLine::Raw);
    cmd.addArgs({packageFile, packageSourcesDirectory.path()});
    processParameters()->setWorkingDirectory(packageDirectory);
    processParameters()->setCommandLine(cmd);

    return true;
}

// Factory

AppManagerCreatePackageStepFactory::AppManagerCreatePackageStepFactory()
{
    registerStep<AppManagerCreatePackageStep>(Constants::DEPLOY_PACKAGE_STEP_ID);
    setDisplayName(AppManagerCreatePackageStep::tr("Create Application Manager package"));
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY);
}

} // namespace Internal
} // namespace AppManager
