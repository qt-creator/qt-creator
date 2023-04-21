// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "mcubuildstep.h"
#include "qmlprojectmanagertr.h"

#include <projectexplorer/buildstep.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/target.h>
#include <projectexplorer/task.h>
#include <projectexplorer/taskhub.h>

#include <cmakeprojectmanager/cmakekitinformation.h>

#include <mcusupport/mculegacyconstants.h>
#include <mcusupport/mcusupportconstants.h>
#include <qtsupport/qtsupportconstants.h>

#include <utils/aspects.h>
#include <utils/filepath.h>

#include <QVersionNumber>

namespace QmlProjectManager {

const Utils::Id DeployMcuProcessStep::id = "QmlProject.Mcu.DeployStep";

void DeployMcuProcessStep::showError(const QString &text)
{
    ProjectExplorer::DeploymentTask task(ProjectExplorer::Task::Error, text);
    ProjectExplorer::TaskHub::addTask(task);
}

// TODO:
// - Grabbing *a* kit might not be the best todo.
//   Would be better to specify a specific version of Qt4MCU in the qmlproject file.
//   Currently we use the kit with the greatest version number.
//
// - Do not compare to *legacy* constants.
//   Sounds like they will stop existing at some point.
//   Also: Find Constant for QUL_PLATFORM

DeployMcuProcessStep::DeployMcuProcessStep(ProjectExplorer::BuildStepList *bc, Utils::Id id)
    : AbstractProcessStep(bc, id)
    , m_tmpDir()
{
    if (!buildSystem()) {
        showError(Tr::tr("Failed to find valid build system"));
        return;
    }

    if (!m_tmpDir.isValid()) {
        showError(Tr::tr("Failed to create valid build directory"));
        return;
    }

    ProjectExplorer::Kit *kit = MCUBuildStepFactory::findMostRecentQulKit();
    if (!kit)
        return;

    QString root = findKitInformation(kit, McuSupport::Internal::Legacy::Constants::QUL_CMAKE_VAR);
    auto rootPath = Utils::FilePath::fromString(root);

    auto cmd = addAspect<Utils::StringAspect>();
    cmd->setSettingsKey("QmlProject.Mcu.ProcessStep.Command");
    cmd->setDisplayStyle(Utils::StringAspect::PathChooserDisplay);
    cmd->setExpectedKind(Utils::PathChooser::Command);
    cmd->setLabelText(Tr::tr("Command:"));
    cmd->setFilePath(rootPath.pathAppended("/bin/qmlprojectexporter"));

    const char *importPathConstant = QtSupport::Constants::KIT_QML_IMPORT_PATH;
    Utils::FilePath projectDir = buildSystem()->projectDirectory();
    Utils::FilePath qulIncludeDir = Utils::FilePath::fromVariant(kit->value(importPathConstant));
    QStringList includeDirs {
        Utils::ProcessArgs::quoteArg(qulIncludeDir.toString()),
        Utils::ProcessArgs::quoteArg(qulIncludeDir.pathAppended("Timeline").toString())
    };

    const char *toolChainConstant = McuSupport::Internal::Constants::KIT_MCUTARGET_TOOLCHAIN_KEY;
    QStringList arguments = {
        Utils::ProcessArgs::quoteArg(buildSystem()->projectFilePath().toString()),
        "--platform", findKitInformation(kit, "QUL_PLATFORM"),
        "--toolchain", kit->value(toolChainConstant).toString(),
        "--include-dirs", includeDirs.join(","),
    };

    auto args = addAspect<Utils::StringAspect>();
    args->setSettingsKey("QmlProject.Mcu.ProcessStep.Arguments");
    args->setDisplayStyle(Utils::StringAspect::LineEditDisplay);
    args->setLabelText(Tr::tr("Arguments:"));
    args->setValue(Utils::ProcessArgs::joinArgs(arguments));

    auto outDir = addAspect<Utils::StringAspect>();
    outDir->setSettingsKey("QmlProject.Mcu.ProcessStep.BuildDirectory");
    outDir->setDisplayStyle(Utils::StringAspect::PathChooserDisplay);
    outDir->setExpectedKind(Utils::PathChooser::Directory);
    outDir->setLabelText(Tr::tr("Build directory:"));
    outDir->setPlaceHolderText(m_tmpDir.path());

    setCommandLineProvider([this, cmd, args, outDir]() -> Utils::CommandLine {
        auto directory = outDir->value();
        if (directory.isEmpty())
            directory = m_tmpDir.path();

        Utils::CommandLine cmdLine(cmd->filePath());
        cmdLine.addArgs(args->value(), Utils::CommandLine::Raw);
        cmdLine.addArg("--outdir");
        cmdLine.addArg(directory);
        return cmdLine;
    });
}

QString DeployMcuProcessStep::findKitInformation(ProjectExplorer::Kit *kit, const QString &key)
{
    // This is (kind of) stolen from mcukitmanager.cpp. Might make sense to unify.
    using namespace CMakeProjectManager;
    const auto config = CMakeConfigurationKitAspect::configuration(kit).toList();
    const auto keyName = key.toUtf8();
    for (const CMakeProjectManager::CMakeConfigItem &configItem : config) {
        if (configItem.key == keyName)
            return QString::fromUtf8(configItem.value);
    }
    return {};
}

MCUBuildStepFactory::MCUBuildStepFactory()
    : BuildStepFactory()
{
    setDisplayName(Tr::tr("Qt4MCU Deploy Step"));
    registerStep<DeployMcuProcessStep>(DeployMcuProcessStep::id);
}

void MCUBuildStepFactory::attachToTarget(ProjectExplorer::Target *target)
{
    if (!target)
        return;

    ProjectExplorer::DeployConfiguration *deployConfiguration = target->activeDeployConfiguration();
    ProjectExplorer::BuildStepList *stepList = deployConfiguration->stepList();
    if (stepList->contains(DeployMcuProcessStep::id))
        return;

    if (!findMostRecentQulKit()) {
        DeployMcuProcessStep::showError(Tr::tr("Failed to find valid Qt4MCU kit"));
        return;
    }

    for (BuildStepFactory *factory : BuildStepFactory::allBuildStepFactories()) {
        if (factory->stepId() == DeployMcuProcessStep::id) {
            ProjectExplorer::BuildStep *deployConfig = factory->create(stepList);
            stepList->appendStep(deployConfig);
        }
    }
}

ProjectExplorer::Kit *MCUBuildStepFactory::findMostRecentQulKit()
{
    // Stolen from mcukitmanager.cpp
    auto kitQulVersion = [](const ProjectExplorer::Kit *kit) -> QVersionNumber {
        const char *sdkVersion = McuSupport::Internal::Constants::KIT_MCUTARGET_SDKVERSION_KEY;
        return QVersionNumber::fromString(kit->value(sdkVersion).toString());
    };

    ProjectExplorer::Kit *mcuKit = nullptr;
    for (auto availableKit : ProjectExplorer::KitManager::kits()) {
        if (!availableKit)
            continue;

        auto qulVersion = kitQulVersion(availableKit);
        if (qulVersion.isNull())
            continue;

        if (!mcuKit)
            mcuKit = availableKit;

        if (qulVersion > kitQulVersion(mcuKit))
            mcuKit = availableKit;
    }
    return mcuKit;
}

} // namespace QmlProjectManager
