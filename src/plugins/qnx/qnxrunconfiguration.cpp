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

#include <utils/processinterface.h>

using namespace ProjectExplorer;
using namespace RemoteLinux;
using namespace Utils;

namespace Qnx::Internal {

class QnxRunConfiguration final : public RunConfiguration
{
public:
    QnxRunConfiguration(Target *target, Id id)
        : RunConfiguration(target, id)
    {
        executable.setDeviceSelector(target, ExecutableAspect::RunDevice);
        executable.setLabelText(Tr::tr("Executable on device:"));
        executable.setPlaceHolderText(Tr::tr("Remote path not set"));
        executable.makeOverridable("RemoteLinux.RunConfig.AlternateRemoteExecutable",
                                   "RemoteLinux.RunConfig.UseAlternateRemoteExecutable");
        executable.setHistoryCompleter("RemoteLinux.AlternateExecutable.History");

        symbolFile.setLabelText(Tr::tr("Executable on host:"));

        environment.setDeviceSelector(target, EnvironmentAspect::RunDevice);

        arguments.setMacroExpander(macroExpander());

        workingDir.setMacroExpander(macroExpander());
        workingDir.setEnvironment(&environment);

        qtLibraries.setSettingsKey("Qt4ProjectManager.QnxRunConfiguration.QtLibPath");
        qtLibraries.setLabelText(Tr::tr("Path to Qt libraries on device"));
        qtLibraries.setDisplayStyle(StringAspect::LineEditDisplay);

        setUpdater([this, target] {
            const BuildTargetInfo bti = buildTargetInfo();
            const FilePath localExecutable = bti.targetFilePath;
            const DeployableFile depFile = target->deploymentData()
                                               .deployableForLocalFile(localExecutable);
            executable.setExecutable(FilePath::fromString(depFile.remoteFilePath()));
            symbolFile.setValue(localExecutable);
        });

        setRunnableModifier([this](ProcessRunData &r) {
            QString libPath = qtLibraries();
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

    ExecutableAspect executable{this};
    SymbolFileAspect symbolFile{this};
    RemoteLinuxEnvironmentAspect environment{this};
    ArgumentsAspect arguments{this};
    WorkingDirectoryAspect workingDir{this};
    TerminalAspect terminal{this};
    StringAspect qtLibraries{this};
};

// QnxRunConfigurationFactory

QnxRunConfigurationFactory::QnxRunConfigurationFactory()
{
    registerRunConfiguration<QnxRunConfiguration>(Constants::QNX_RUNCONFIG_ID);
    addSupportedTargetDeviceType(Constants::QNX_QNX_OS_TYPE);
}

} // Qnx::Internal
