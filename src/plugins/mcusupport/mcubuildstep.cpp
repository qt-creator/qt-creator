// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "mcubuildstep.h"

#include "mcukitmanager.h"
#include "mculegacyconstants.h"
#include "mcusupportconstants.h"

#include <cmakeprojectmanager/cmakekitinformation.h>

#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/task.h>
#include <projectexplorer/taskhub.h>

#include <qmlprojectmanager/qmlprojectmanagertr.h>

#include <qtsupport/qtsupportconstants.h>

#include <utils/aspects.h>

#include <QTemporaryDir>
#include <QVersionNumber>

namespace McuSupport::Internal {

class DeployMcuProcessStep : public ProjectExplorer::AbstractProcessStep
{
public:
    static const Utils::Id id;
    static void showError(const QString &text);

    DeployMcuProcessStep(ProjectExplorer::BuildStepList *bc, Utils::Id id);

private:
    QString findKitInformation(ProjectExplorer::Kit *kit, const QString &key);
    QTemporaryDir m_tmpDir;
};

const Utils::Id DeployMcuProcessStep::id = "QmlProject.Mcu.DeployStep";

void DeployMcuProcessStep::showError(const QString &text)
{
    ProjectExplorer::DeploymentTask task(ProjectExplorer::Task::Error, text);
    ProjectExplorer::TaskHub::addTask(task);
}

DeployMcuProcessStep::DeployMcuProcessStep(ProjectExplorer::BuildStepList *bc, Utils::Id id)
    : AbstractProcessStep(bc, id)
    , m_tmpDir()
{
    if (!buildSystem()) {
        showError(QmlProjectManager::Tr::tr("Failed to find valid build system"));
        return;
    }

    if (!m_tmpDir.isValid()) {
        showError(QmlProjectManager::Tr::tr("Failed to create valid build directory"));
        return;
    }

    ProjectExplorer::Kit *kit = MCUBuildStepFactory::findMostRecentQulKit();
    if (!kit)
        return;

    QString root = findKitInformation(kit, Internal::Legacy::Constants::QUL_CMAKE_VAR);
    auto rootPath = Utils::FilePath::fromString(root);

    auto cmd = addAspect<Utils::FilePathAspect>();
    cmd->setSettingsKey("QmlProject.Mcu.ProcessStep.Command");
    cmd->setExpectedKind(Utils::PathChooser::Command);
    cmd->setLabelText(QmlProjectManager::Tr::tr("Command:"));
    cmd->setFilePath(rootPath.pathAppended("/bin/qmlprojectexporter"));

    const char *importPathConstant = QtSupport::Constants::KIT_QML_IMPORT_PATH;
    Utils::FilePath projectDir = buildSystem()->projectDirectory();
    Utils::FilePath qulIncludeDir = Utils::FilePath::fromVariant(kit->value(importPathConstant));
    QStringList includeDirs {
        Utils::ProcessArgs::quoteArg(qulIncludeDir.toString()),
        Utils::ProcessArgs::quoteArg(qulIncludeDir.pathAppended("Timeline").toString())
    };

    const char *toolChainConstant = Internal::Constants::KIT_MCUTARGET_TOOLCHAIN_KEY;
    QStringList arguments = {
        Utils::ProcessArgs::quoteArg(buildSystem()->projectFilePath().toString()),
        "--platform", findKitInformation(kit, "QUL_PLATFORM"),
        "--toolchain", kit->value(toolChainConstant).toString(),
        "--include-dirs", includeDirs.join(","),
    };

    auto args = addAspect<Utils::StringAspect>();
    args->setSettingsKey("QmlProject.Mcu.ProcessStep.Arguments");
    args->setDisplayStyle(Utils::StringAspect::LineEditDisplay);
    args->setLabelText(QmlProjectManager::Tr::tr("Arguments:"));
    args->setValue(Utils::ProcessArgs::joinArgs(arguments));

    auto outDir = addAspect<Utils::FilePathAspect>();
    outDir->setSettingsKey("QmlProject.Mcu.ProcessStep.BuildDirectory");
    outDir->setExpectedKind(Utils::PathChooser::Directory);
    outDir->setLabelText(QmlProjectManager::Tr::tr("Build directory:"));
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

    if(target())
        target()->setNamedSettings("IS_MCU_QDS_PROJECT", true);
}

QString DeployMcuProcessStep::findKitInformation(ProjectExplorer::Kit *kit, const QString &key)
{
    using namespace CMakeProjectManager;
    const auto config = CMakeConfigurationKitAspect::configuration(kit).toList();
    const auto keyName = key.toUtf8();
    for (const CMakeProjectManager::CMakeConfigItem &configItem : config) {
        if (configItem.key == keyName)
            return QString::fromUtf8(configItem.value);
    }
    return {};
}

ProjectExplorer::Kit *MCUBuildStepFactory::findMostRecentQulKit()
{
    ProjectExplorer::Kit *mcuKit = nullptr;
    for (auto availableKit : ProjectExplorer::KitManager::kits()) {
        if (!availableKit)
            continue;

        auto qulVersion = McuKitManager::kitQulVersion(availableKit);
        if (qulVersion.isNull())
            continue;

        if (!mcuKit)
            mcuKit = availableKit;

        if (qulVersion > McuKitManager::kitQulVersion(mcuKit))
            mcuKit = availableKit;
    }
    return mcuKit;
}

void MCUBuildStepFactory::updateDeployStep(ProjectExplorer::Target *target, bool enabled)
{
    if (!target)
        return;

    ProjectExplorer::DeployConfiguration *deployConfiguration = target->activeDeployConfiguration();
    ProjectExplorer::BuildStepList *stepList = deployConfiguration->stepList();
    ProjectExplorer::BuildStep *step = stepList->firstStepWithId(DeployMcuProcessStep::id);
    if (!step && enabled) {
        if (findMostRecentQulKit()) {
            stepList->appendStep(DeployMcuProcessStep::id);
        } else {
            DeployMcuProcessStep::showError(
                QmlProjectManager::Tr::tr("Failed to find valid Qt for MCUs kit"));
        }
    } else {
        if (!step)
            return;
        step->setEnabled(enabled);
    }
}


MCUBuildStepFactory::MCUBuildStepFactory()
{
    setDisplayName(QmlProjectManager::Tr::tr("Qt for MCUs Deploy Step"));
    registerStep<DeployMcuProcessStep>(DeployMcuProcessStep::id);
}

} // namespace McuSupport::Internal
