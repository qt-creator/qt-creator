// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "mcubuildstep.h"

#include "mcukitmanager.h"
#include "mculegacyconstants.h"
#include "mcusupportconstants.h"

#include <coreplugin/icore.h>

#include <cmakeprojectmanager/cmakekitaspect.h>

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

#include <QMessageBox>
#include <QTemporaryDir>
#include <QVersionNumber>

using namespace Utils;

namespace McuSupport::Internal {
const Id DeployMcuProcessStep::id = "QmlProject.Mcu.DeployStep";

void DeployMcuProcessStep::showError(const QString &text)
{
    ProjectExplorer::DeploymentTask task(ProjectExplorer::Task::Error, text);
    ProjectExplorer::TaskHub::addTask(task);
}

DeployMcuProcessStep::DeployMcuProcessStep(ProjectExplorer::BuildStepList *bc, Id id)
    : AbstractProcessStep(bc, id)
    , m_tmpDir()
{
    if (!buildSystem()) {
        showError(QmlProjectManager::Tr::tr("Cannot find a valid build system."));
        return;
    }

    if (!m_tmpDir.isValid()) {
        showError(QmlProjectManager::Tr::tr("Cannot create a valid build directory."));
        return;
    }

    ProjectExplorer::Kit *kit = MCUBuildStepFactory::findMostRecentQulKit();
    if (!kit)
        return;

    const QString root = findKitInformation(kit, Internal::Legacy::Constants::QUL_CMAKE_VAR);
    const FilePath rootPath = FilePath::fromString(root);

    cmd.setSettingsKey("QmlProject.Mcu.ProcessStep.Command");
    cmd.setExpectedKind(PathChooser::Command);
    cmd.setLabelText(QmlProjectManager::Tr::tr("Command:"));
    cmd.setValue(rootPath.pathAppended("/bin/qmlprojectexporter"));

    const Id toolChainConstant = Internal::Constants::KIT_MCUTARGET_TOOLCHAIN_KEY;
    arguments
        = {ProcessArgs::quoteArg(buildSystem()->projectFilePath().path()),
           "--platform",
           findKitInformation(kit, "QUL_PLATFORM"),
           "--toolchain",
           kit->value(toolChainConstant).toString()};

    args.setSettingsKey("QmlProject.Mcu.ProcessStep.Arguments");
    args.setDisplayStyle(StringAspect::LineEditDisplay);
    args.setLabelText(QmlProjectManager::Tr::tr("Arguments:"));
    args.setValue(ProcessArgs::joinArgs(arguments));
    updateIncludeDirArgs();

    outDir.setSettingsKey("QmlProject.Mcu.ProcessStep.BuildDirectory");
    outDir.setExpectedKind(PathChooser::Directory);
    outDir.setLabelText(QmlProjectManager::Tr::tr("Build directory:"));
    outDir.setPlaceHolderText(m_tmpDir.path());

    setCommandLineProvider([this] {
        QString directory = outDir().path();
        if (directory.isEmpty())
            directory = m_tmpDir.path();

        CommandLine cmdLine(cmd());
        cmdLine.addArgs(args(), CommandLine::Raw);
        cmdLine.addArg("--outdir");
        cmdLine.addArg(directory);
        return cmdLine;
    });

    connect(this, &BuildStep::addOutput, this, [](const QString &str, OutputFormat fmt) {
        if (fmt == OutputFormat::ErrorMessage)
            showError(str);
    });
}

// Workaround for QDS-13763, when UL-10456 is completed this can be removed with the next LTS
void DeployMcuProcessStep::updateIncludeDirArgs()
{
    ProjectExplorer::Kit *kit = MCUBuildStepFactory::findMostRecentQulKit();
    if (!kit)
        return;

    // Remove the old include dirs (if any)
    bool removeMode = false;
    arguments.erase(
        std::remove_if(
            arguments.begin(),
            arguments.end(),
            [&](const QString &s) {
                if (s == "--include-dirs")
                    return removeMode = true;
                if (removeMode && s.startsWith("--"))
                    return removeMode = false;
                return removeMode;
            }),
        arguments.end());

    const Id importPathConstant = QtSupport::Constants::KIT_QML_IMPORT_PATH;
    const FilePath qulIncludeDir = FilePath::fromVariant(kit->value(importPathConstant));

    QStringList includeDirs = {ProcessArgs::quoteArg(qulIncludeDir.path())};
    QStringList subDirs
        = {"Controls",
           "ControlsTemplates",
           "Extras",
           "Layers",
           "Layouts",
           "Profiling",
           "SafeRenderer",
           "Shapes",
           "StudioComponents",
           "Timeline",
           "VirtualKeyboard"};
    std::transform(
        subDirs.begin(), subDirs.end(), std::back_inserter(includeDirs), [&](const QString &subDir) {
            return ProcessArgs::quoteArg(qulIncludeDir.pathAppended(subDir).path());
        });
    arguments.append({"--include-dirs", includeDirs.join(",")});
    args.setValue(ProcessArgs::joinArgs(arguments));
};

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

void MCUBuildStepFactory::updateDeployStep(ProjectExplorer::BuildConfiguration *bc, bool enabled)
{
    if (!bc)
        return;

    ProjectExplorer::DeployConfiguration *deployConfiguration = bc->activeDeployConfiguration();

    // Return if the kit is currupted or is an MCU kit (unsupported in Design Studio)
    if (!deployConfiguration
        || (bc->kit() && bc->kit()->hasValue(Constants::KIT_MCUTARGET_KITVERSION_KEY))) {
        // This branch is called multiple times when selecting the run configuration
        // Show the message only once when the kit changes (avoid repitition)
        static ProjectExplorer::Kit *previousSelectedKit = nullptr;
        if (previousSelectedKit && previousSelectedKit == bc->kit())
            return;
        previousSelectedKit = bc->kit();

        //TODO use DeployMcuProcessStep::showError instead when the Issues panel
        // supports poping up on new entries
        QMessageBox::warning(
            Core::ICore::dialogParent(),
            QmlProjectManager::Tr::tr("The Selected Kit Is Not Supported"),
            QmlProjectManager::Tr::tr(
                "You cannot use the selected kit to preview Qt for MCUs applications."));
        return;
    }

    ProjectExplorer::BuildStepList *stepList = deployConfiguration->stepList();
    ProjectExplorer::BuildStep *step = stepList->firstStepWithId(DeployMcuProcessStep::id);
    if (!step && enabled) {
        if (findMostRecentQulKit()) {
            stepList->appendStep(DeployMcuProcessStep::id);
        } else {
            DeployMcuProcessStep::showError(
                QmlProjectManager::Tr::tr("Cannot find a valid Qt for MCUs kit."));
        }
    } else {
        if (!step)
            return;
        auto mcuStep = qobject_cast<DeployMcuProcessStep *>(step);
        if (mcuStep)
            mcuStep->updateIncludeDirArgs();
        step->setStepEnabled(enabled);
    }
}

MCUBuildStepFactory::MCUBuildStepFactory()
{
    setDisplayName(QmlProjectManager::Tr::tr("Qt for MCUs Deploy Step"));
    registerStep<DeployMcuProcessStep>(DeployMcuProcessStep::id);
}

} // namespace McuSupport::Internal
