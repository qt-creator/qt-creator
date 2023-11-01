// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qdbrunconfiguration.h"

#include "qdbconstants.h"
#include "qdbtr.h"

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>

#include <remotelinux/remotelinuxenvironmentaspect.h>

#include <utils/commandline.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Qdb::Internal {

// QdbRunConfiguration

class QdbRunConfiguration : public RunConfiguration
{
public:
    QdbRunConfiguration(Target *target, Id id)
        : RunConfiguration(target, id)
    {
        setDefaultDisplayName(Tr::tr("Run on Boot2Qt Device"));

        executable.setDeviceSelector(target, ExecutableAspect::RunDevice);
        executable.setSettingsKey("QdbRunConfig.RemoteExecutable");
        executable.setLabelText(Tr::tr("Executable on device:"));
        executable.setPlaceHolderText(Tr::tr("Remote path not set"));
        executable.makeOverridable("QdbRunConfig.AlternateRemoteExecutable",
                                   "QdbRunCofig.UseAlternateRemoteExecutable");

        symbolFile.setSettingsKey("QdbRunConfig.LocalExecutable");
        symbolFile.setLabelText(Tr::tr("Executable on host:"));

        environment.setDeviceSelector(target, EnvironmentAspect::RunDevice);

        arguments.setMacroExpander(macroExpander());

        workingDir.setMacroExpander(macroExpander());
        workingDir.setEnvironment(&environment);

        fullCommand.setLabelText(Tr::tr("Full command line:"));

        setUpdater([this, target] {
            const BuildTargetInfo bti = buildTargetInfo();
            const FilePath localExecutable = bti.targetFilePath;
            const DeployableFile depFile = target->deploymentData().deployableForLocalFile(localExecutable);
            IDevice::ConstPtr dev = DeviceKitAspect::device(target->kit());
            QTC_ASSERT(dev, return);
            executable.setExecutable(dev->filePath(depFile.remoteFilePath()));
            symbolFile.setValue(localExecutable);
        });

        connect(target, &Target::buildSystemUpdated, this, &RunConfiguration::update);
        connect(target, &Target::deploymentDataChanged, this, &RunConfiguration::update);
        connect(target, &Target::kitChanged, this, &RunConfiguration::update);

        auto updateFullCommand = [this] {
            CommandLine plain{executable(), arguments(), CommandLine::Raw};
            CommandLine cmd;
            cmd.setExecutable(plain.executable().withNewPath(Constants::AppcontrollerFilepath));
            cmd.addCommandLineAsArgs(plain);
            fullCommand.setValue(cmd.toUserOutput());
        };

        connect(&arguments, &BaseAspect::changed, this, updateFullCommand);
        connect(&executable, &BaseAspect::changed, this, updateFullCommand);
        updateFullCommand();
    }

private:
    Tasks checkForIssues() const override
    {
        Tasks tasks;
        if (executable().isEmpty()) {
            tasks << BuildSystemTask(Task::Warning, Tr::tr("The remote executable must be set "
                                                           "in order to run on a Boot2Qt device."));
        }
        return tasks;
    }

    QString defaultDisplayName() const
    {
        return RunConfigurationFactory::decoratedTargetName(buildKey(), target());
    }

    ExecutableAspect executable{this};
    SymbolFileAspect symbolFile{this};
    RemoteLinux::RemoteLinuxEnvironmentAspect environment{this};
    ArgumentsAspect arguments{this};
    WorkingDirectoryAspect workingDir{this};
    StringAspect fullCommand{this};
};

// QdbRunConfigurationFactory

QdbRunConfigurationFactory::QdbRunConfigurationFactory()
{
    registerRunConfiguration<QdbRunConfiguration>("QdbLinuxRunConfiguration:");
    addSupportedTargetDeviceType(Constants::QdbLinuxOsType);
}

} // Qdb::Internal
