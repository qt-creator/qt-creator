// Copyright (C) 2018 Jochen Seemann
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "conanconstants.h"
#include "conaninstallstep.h"
#include "conanplugin.h"
#include "conansettings.h"

#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/gnumakeparser.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/task.h>
#include <projectexplorer/toolchain.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace ConanPackageManager {
namespace Internal {

// ConanInstallStep

class ConanInstallStep final : public AbstractProcessStep
{
    Q_DECLARE_TR_FUNCTIONS(ConanPackageManager::Internal::ConanInstallStep)

public:
    ConanInstallStep(BuildStepList *bsl, Id id);

private:
    bool init() final;
    void setupOutputFormatter(OutputFormatter *formatter) final;
};

ConanInstallStep::ConanInstallStep(BuildStepList *bsl, Id id)
    : AbstractProcessStep(bsl, id)
{
    setUseEnglishOutput();
    setDisplayName(ConanInstallStep::tr("Conan install"));

    auto conanFile = addAspect<StringAspect>();
    conanFile->setSettingsKey("ConanPackageManager.InstallStep.ConanFile");
    conanFile->setFilePath(ConanPlugin::conanFilePath(project(),
                           project()->projectDirectory() / "conanfile.txt"));
    conanFile->setLabelText(tr("Conan file:"));
    conanFile->setToolTip(tr("Enter location of conanfile.txt or conanfile.py."));
    conanFile->setDisplayStyle(StringAspect::PathChooserDisplay);
    conanFile->setExpectedKind(PathChooser::File);

    auto additionalArguments = addAspect<StringAspect>();
    additionalArguments->setSettingsKey("ConanPackageManager.InstallStep.AdditionalArguments");
    additionalArguments->setLabelText(tr("Additional arguments:"));
    additionalArguments->setDisplayStyle(StringAspect::LineEditDisplay);

    auto buildMissing = addAspect<BoolAspect>();
    buildMissing->setSettingsKey("ConanPackageManager.InstallStep.BuildMissing");
    buildMissing->setLabel("Build missing:", BoolAspect::LabelPlacement::InExtraLabel);
    buildMissing->setDefaultValue(true);
    buildMissing->setValue(true);

    setCommandLineProvider([=] {
        BuildConfiguration::BuildType bt = buildConfiguration()->buildType();
        const QString buildType = bt == BuildConfiguration::Release ? QString("Release")
                                                                    : QString("Debug");

        CommandLine cmd(ConanPlugin::conanSettings()->conanFilePath.filePath());
        cmd.addArgs({"install", "-s", "build_type=" + buildType});
        if (buildMissing->value())
            cmd.addArg("--build=missing");
        cmd.addArg(conanFile->value());
        cmd.addArgs(additionalArguments->value(), CommandLine::Raw);
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
    setDisplayName(ConanInstallStep::tr("Run conan install"));
}

} // Internal
} // ConanPackageManager
