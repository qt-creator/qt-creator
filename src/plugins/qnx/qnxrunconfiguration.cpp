// Copyright (C) 2016 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qnxrunconfiguration.h"

#include "qnxconstants.h"
#include "qnxtr.h"

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/deployablefile.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>

#include <remotelinux/remotelinuxenvironmentaspect.h>

#include <qtsupport/qtoutputformatter.h>

using namespace ProjectExplorer;
using namespace RemoteLinux;
using namespace Utils;

namespace Qnx::Internal {

class QnxRunConfiguration final : public ProjectExplorer::RunConfiguration
{
public:
    QnxRunConfiguration(ProjectExplorer::Target *target, Utils::Id id);
};

QnxRunConfiguration::QnxRunConfiguration(Target *target, Id id)
    : RunConfiguration(target, id)
{
    auto exeAspect = addAspect<ExecutableAspect>(target, ExecutableAspect::RunDevice);
    exeAspect->setLabelText(Tr::tr("Executable on device:"));
    exeAspect->setPlaceHolderText(Tr::tr("Remote path not set"));
    exeAspect->makeOverridable("RemoteLinux.RunConfig.AlternateRemoteExecutable",
                               "RemoteLinux.RunConfig.UseAlternateRemoteExecutable");
    exeAspect->setHistoryCompleter("RemoteLinux.AlternateExecutable.History");

    auto symbolsAspect = addAspect<SymbolFileAspect>();
    symbolsAspect->setLabelText(Tr::tr("Executable on host:"));
    symbolsAspect->setDisplayStyle(SymbolFileAspect::LabelDisplay);

    auto envAspect = addAspect<RemoteLinuxEnvironmentAspect>(target);

    addAspect<ArgumentsAspect>(macroExpander());
    addAspect<WorkingDirectoryAspect>(macroExpander(), envAspect);
    addAspect<TerminalAspect>();

    auto libAspect = addAspect<StringAspect>();
    libAspect->setSettingsKey("Qt4ProjectManager.QnxRunConfiguration.QtLibPath");
    libAspect->setLabelText(Tr::tr("Path to Qt libraries on device"));
    libAspect->setDisplayStyle(StringAspect::LineEditDisplay);

    setUpdater([this, target, exeAspect, symbolsAspect] {
        const BuildTargetInfo bti = buildTargetInfo();
        const FilePath localExecutable = bti.targetFilePath;
        const DeployableFile depFile = target->deploymentData()
            .deployableForLocalFile(localExecutable);
        exeAspect->setExecutable(FilePath::fromString(depFile.remoteFilePath()));
        symbolsAspect->setFilePath(localExecutable);
    });

    setRunnableModifier([libAspect](Runnable &r) {
        QString libPath = libAspect->value();
        if (!libPath.isEmpty()) {
            r.environment.appendOrSet("LD_LIBRARY_PATH", libPath + "/lib:$LD_LIBRARY_PATH");
            r.environment.appendOrSet("QML_IMPORT_PATH", libPath + "/imports:$QML_IMPORT_PATH");
            r.environment.appendOrSet("QML2_IMPORT_PATH", libPath + "/qml:$QML2_IMPORT_PATH");
            r.environment.appendOrSet("QT_PLUGIN_PATH", libPath + "/plugins:$QT_PLUGIN_PATH");
            r.environment.set("QT_QPA_FONTDIR", libPath + "/lib/fonts");
        }
    });

    connect(target, &Target::buildSystemUpdated, this, &RunConfiguration::update);
}

// QnxRunConfigurationFactory

QnxRunConfigurationFactory::QnxRunConfigurationFactory()
{
    registerRunConfiguration<QnxRunConfiguration>(Constants::QNX_RUNCONFIG_ID);
    addSupportedTargetDeviceType(Constants::QNX_QNX_OS_TYPE);
}

} // Qnx::Internal
