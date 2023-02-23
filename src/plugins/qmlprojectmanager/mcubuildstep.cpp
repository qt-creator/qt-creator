// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "mcubuildstep.h"

#include "projectexplorer/buildstep.h"
#include "projectexplorer/buildsystem.h"
#include "projectexplorer/buildsteplist.h"
#include "projectexplorer/deployconfiguration.h"
#include "projectexplorer/kit.h"
#include "projectexplorer/target.h"
#include "projectexplorer/kitmanager.h"

#include <coreplugin/messagebox.h>
#include <cmakeprojectmanager/cmakekitinformation.h>

#include "qtsupport/qtsupportconstants.h"
#include "mcusupport/mcusupportconstants.h"
#include "mcusupport/mculegacyconstants.h"

#include "utils/aspects.h"
#include "utils/filepath.h"

#include <QVersionNumber>

namespace QmlProjectManager {

const Utils::Id DeployMcuProcessStep::id = "QmlProject.Mcu.DeployStep";
const QString DeployMcuProcessStep::processCommandKey = "QmlProject.Mcu.ProcessStep.Command";
const QString DeployMcuProcessStep::processArgumentsKey = "QmlProject.Mcu.ProcessStep.Arguments";
const QString DeployMcuProcessStep::processWorkingDirectoryKey = "QmlProject.Mcu.ProcessStep.BuildDirectory";

void DeployMcuProcessStep::showError(const QString& text) {
    Core::AsynchronousMessageBox::critical(tr("Qt4MCU Deploy Step"), text);
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
    if (not buildSystem()) {
        showError(QObject::tr("Failed to find valid build system"));
        return;
    }

    if (not m_tmpDir.isValid()) {
        showError(QObject::tr("Failed to create valid build directory"));
        return;
    }

    auto fixPath = [](const QString& path) -> QString {
        return "\"" + QDir::toNativeSeparators(path) + "\"";
    };

    ProjectExplorer::Kit* kit = MCUBuildStepFactory::findMostRecentQulKit();
    if (not kit)
        return;

    QString root = findKitInformation(kit, McuSupport::Internal::Legacy::Constants::QUL_CMAKE_VAR);

    auto* cmd = addAspect<Utils::StringAspect>();
    cmd->setSettingsKey(processCommandKey);
    cmd->setDisplayStyle(Utils::StringAspect::PathChooserDisplay);
    cmd->setExpectedKind(Utils::PathChooser::Command);
    cmd->setLabelText(tr("Command:"));
    cmd->setValue(QDir::toNativeSeparators(root + "/bin/qmlprojectexporter"));

    const char* importPathConstant = QtSupport::Constants::KIT_QML_IMPORT_PATH;
    QString projectDir = buildSystem()->projectDirectory().toString();
    QString qulIncludeDir = kit->value(importPathConstant).toString( );
    QStringList includeDirs {
        fixPath(qulIncludeDir),
        fixPath(qulIncludeDir + "/Timeline"),
        fixPath(projectDir + "/imports")
    };

    const char* toolChainConstant = McuSupport::Internal::Constants::KIT_MCUTARGET_TOOLCHAIN_KEY;
    QStringList arguments = {
        fixPath(buildSystem()->projectFilePath().toString()),
        "--platform", findKitInformation(kit, "QUL_PLATFORM"),
        "--toolchain", kit->value(toolChainConstant).toString( ),
        "--include-dirs", includeDirs.join(","),
    };

    auto* args = addAspect<Utils::StringAspect>();
    args->setSettingsKey(processArgumentsKey);
    args->setDisplayStyle(Utils::StringAspect::LineEditDisplay);
    args->setLabelText(tr("Arguments:"));
    args->setValue(arguments.join(" "));

    auto* outDir = addAspect<Utils::StringAspect>();
    outDir->setSettingsKey(processWorkingDirectoryKey);
    outDir->setDisplayStyle(Utils::StringAspect::PathChooserDisplay);
    outDir->setExpectedKind(Utils::PathChooser::Directory);
    outDir->setLabelText(tr("Build directory:"));
    outDir->setPlaceHolderText(fixPath(m_tmpDir.path()));

    setCommandLineProvider([this, cmd, args, outDir, fixPath]() -> Utils::CommandLine {
        auto directory = outDir->value();
        if (directory.isEmpty())
            directory = fixPath(m_tmpDir.path());

        QString outArg = " --outdir " + directory;
        return {cmd->filePath(), args->value() + outArg, Utils::CommandLine::Raw};
    });
}

bool DeployMcuProcessStep::init()
{
    if (!AbstractProcessStep::init())
        return false;
    return true;
}

void DeployMcuProcessStep::doRun()
{
    AbstractProcessStep::doRun();
}

QString DeployMcuProcessStep::findKitInformation(ProjectExplorer::Kit* kit, const QString& key)
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
    setDisplayName("Qt4MCU Deploy Step");
    registerStep< DeployMcuProcessStep >(DeployMcuProcessStep::id);
}

void MCUBuildStepFactory::attachToTarget(ProjectExplorer::Target *target)
{
    if (not target)
        return;

    ProjectExplorer::DeployConfiguration* deployConfiguration = target->activeDeployConfiguration();
    ProjectExplorer::BuildStepList* stepList = deployConfiguration->stepList();
    if (stepList->contains(DeployMcuProcessStep::id))
        return;

    if (not findMostRecentQulKit()) {
        DeployMcuProcessStep::showError(QObject::tr("Failed to find valid Qt4MCU kit"));
        return;
    }

    for (BuildStepFactory *factory : BuildStepFactory::allBuildStepFactories()) {
        if (factory->stepId() == DeployMcuProcessStep::id) {
           ProjectExplorer::BuildStep* deployConfig = factory->create(stepList);
           stepList->appendStep(deployConfig);
        }
    }
}

ProjectExplorer::Kit* MCUBuildStepFactory::findMostRecentQulKit()
{
    // Stolen from mcukitmanager.cpp
    auto kitQulVersion = [](const ProjectExplorer::Kit *kit) -> QVersionNumber {
        const char* sdkVersion = McuSupport::Internal::Constants::KIT_MCUTARGET_SDKVERSION_KEY;
        return QVersionNumber::fromString(kit->value(sdkVersion).toString());
    };

    ProjectExplorer::Kit* kit = nullptr;
    for (auto k : ProjectExplorer::KitManager::kits())
    {
        auto qulVersion = kitQulVersion(k);
        if (qulVersion.isNull( ))
            continue;

        if (not kit)
            kit = k;

        if (qulVersion > kitQulVersion(kit))
            kit = k;
    }
    return kit;
}

} // namespace QmlProjectManager
