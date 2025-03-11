// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qdbrunconfiguration.h"

#include "qdbconstants.h"
#include "qdbtr.h"

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/devicesupport/idevice.h>
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
    QdbRunConfiguration(BuildConfiguration *bc, Id id)
        : RunConfiguration(bc, id)
    {
        setDefaultDisplayName(Tr::tr("Run on Boot to Qt Device"));

        executable.setDeviceSelector(kit(), ExecutableAspect::RunDevice);
        executable.setSettingsKey("QdbRunConfig.RemoteExecutable");
        executable.setLabelText(Tr::tr("Executable on device:"));
        executable.setPlaceHolderText(Tr::tr("Remote path not set"));
        executable.makeOverridable("QdbRunConfig.AlternateRemoteExecutable",
                                   "QdbRunCofig.UseAlternateRemoteExecutable");

        symbolFile.setSettingsKey("QdbRunConfig.LocalExecutable");
        symbolFile.setLabelText(Tr::tr("Executable on host:"));

        environment.setDeviceSelector(kit(), EnvironmentAspect::RunDevice);

        workingDir.setEnvironment(&environment);

        fullCommand.setLabelText(Tr::tr("Full command line:"));

        setUpdater([this] {
            const BuildTargetInfo bti = buildTargetInfo();
            const FilePath localExecutable = bti.targetFilePath;
            const DeployableFile depFile = buildSystem()->deploymentData().deployableForLocalFile(
                localExecutable);
            IDevice::ConstPtr dev = RunDeviceKitAspect::device(kit());
            QTC_ASSERT(dev, return);
            executable.setExecutable(dev->filePath(depFile.remoteFilePath()));
            symbolFile.setValue(localExecutable);
        });

        auto updateFullCommand = [this] {
            CommandLine plain{executable(), arguments(), CommandLine::Raw};
            CommandLine cmd;
            cmd.setExecutable(plain.executable().withNewPath(Constants::AppcontrollerFilepath));
            cmd.addCommandLineAsArgs(plain);
            fullCommand.setValue(cmd.toUserOutput());
        };

        arguments.addOnChanged(this, updateFullCommand);
        executable.addOnChanged(this, updateFullCommand);
        updateFullCommand();
    }

private:
    Tasks checkForIssues() const override
    {
        Tasks tasks;
        if (executable().isEmpty()) {
            tasks << BuildSystemTask(Task::Warning, Tr::tr("The remote executable must be set "
                                                           "to run on a Boot to Qt device."));
        }
        return tasks;
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
    registerRunConfiguration<QdbRunConfiguration>(Constants::QdbRunConfigurationId);
    addSupportedTargetDeviceType(Constants::QdbLinuxOsType);
}

} // Qdb::Internal
