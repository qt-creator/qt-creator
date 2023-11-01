// Copyright (C) 2018 Jochen Seemann
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "conaninstallstep.h"

#include "conanconstants.h"
#include "conansettings.h"
#include "conantr.h"

#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/gnumakeparser.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/task.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/projectmanager.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Conan::Internal {

static FilePath conanFilePath(Project *project, const FilePath &defaultFilePath = {})
{
    const FilePath projectDirectory = project->projectDirectory();
    // conanfile.py takes precedence over conanfile.txt when "conan install dir" is invoked
    const FilePath conanPy = projectDirectory / "conanfile.py";
    if (conanPy.exists())
        return conanPy;
    const FilePath conanTxt = projectDirectory / "conanfile.txt";
    if (conanTxt.exists())
        return conanTxt;
    return defaultFilePath;
}

static void connectTarget(Project *project, Target *target)
{
    if (!conanFilePath(project).isEmpty()) {
        const QList<BuildConfiguration *> buildConfigurations = target->buildConfigurations();
        for (BuildConfiguration *buildConfiguration : buildConfigurations)
            buildConfiguration->buildSteps()->insertStep(0, Constants::INSTALL_STEP);
    }
    QObject::connect(target, &Target::addedBuildConfiguration,
                     target, [project] (BuildConfiguration *buildConfiguration) {
        if (!conanFilePath(project).isEmpty())
            buildConfiguration->buildSteps()->insertStep(0, Constants::INSTALL_STEP);
    });
}

// ConanInstallStep

class ConanInstallStep final : public AbstractProcessStep
{
public:
    ConanInstallStep(BuildStepList *bsl, Id id);

private:
    bool init() final;
    void setupOutputFormatter(OutputFormatter *formatter) final;

    FilePathAspect conanFile{this};
    StringAspect additionalArguments{this};
    BoolAspect buildMissing{this};
};

ConanInstallStep::ConanInstallStep(BuildStepList *bsl, Id id)
    : AbstractProcessStep(bsl, id)
{
    setUseEnglishOutput();
    setDisplayName(Tr::tr("Conan install"));

    conanFile.setSettingsKey("ConanPackageManager.InstallStep.ConanFile");
    conanFile.setValue(conanFilePath(project(), project()->projectDirectory() / "conanfile.txt"));
    conanFile.setLabelText(Tr::tr("Conan file:"));
    conanFile.setToolTip(Tr::tr("Enter location of conanfile.txt or conanfile.py."));
    conanFile.setExpectedKind(PathChooser::File);

    additionalArguments.setSettingsKey("ConanPackageManager.InstallStep.AdditionalArguments");
    additionalArguments.setLabelText(Tr::tr("Additional arguments:"));
    additionalArguments.setDisplayStyle(StringAspect::LineEditDisplay);

    buildMissing.setSettingsKey("ConanPackageManager.InstallStep.BuildMissing");
    buildMissing.setLabel("Build missing:", BoolAspect::LabelPlacement::InExtraLabel);
    buildMissing.setDefaultValue(true);
    buildMissing.setValue(true);

    setCommandLineProvider([this] {
        BuildConfiguration::BuildType bt = buildConfiguration()->buildType();
        const QString buildType = bt == BuildConfiguration::Release ? QString("Release")
                                                                    : QString("Debug");

        CommandLine cmd(settings().conanFilePath());
        cmd.addArgs({"install", "-s", "build_type=" + buildType});
        if (buildMissing())
            cmd.addArg("--build=missing");
        cmd.addArg(conanFile().path());
        cmd.addArgs(additionalArguments(), CommandLine::Raw);
        return cmd;
    });

    setSummaryUpdater([this]() -> QString {
        QList<ToolChain *> tcList = ToolChainKitAspect::toolChains(target()->kit());
        if (tcList.isEmpty())
            return "<b>" + ToolChainKitAspect::msgNoToolChainInTarget() + "</b>";
        ProcessParameters param;
        setupProcessParameters(&param);
        return param.summary(displayName());
    });

    connect(ProjectManager::instance(), &ProjectManager::projectAdded, this, [](Project * project) {
        connect(project, &Project::addedTarget, project, [project] (Target *target) {
            connectTarget(project, target);
        });
    });
}

bool ConanInstallStep::init()
{
    if (!AbstractProcessStep::init())
        return false;

    const QList<ToolChain *> tcList = ToolChainKitAspect::toolChains(target()->kit());
    if (tcList.isEmpty()) {
        emit addTask(Task::compilerMissingTask());
        emitFaultyConfigurationMessage();
        return false;
    }

    return true;
}

void ConanInstallStep::setupOutputFormatter(OutputFormatter *formatter)
{
    formatter->addLineParser(new GnuMakeParser());
    formatter->addLineParsers(kit()->createOutputParsers());
    formatter->addSearchDir(processParameters()->effectiveWorkingDirectory());
    AbstractProcessStep::setupOutputFormatter(formatter);
}

// ConanInstallStepFactory

ConanInstallStepFactory::ConanInstallStepFactory()
{
    registerStep<ConanInstallStep>(Constants::INSTALL_STEP);
    setDisplayName(Tr::tr("Run conan install"));
}

} // Conan::Internal
