// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qdbrunconfiguration.h"

#include "qdbconstants.h"

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>

#include <remotelinux/remotelinuxenvironmentaspect.h>

#include <utils/commandline.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Qdb {
namespace Internal {

// FullCommandLineAspect

class FullCommandLineAspect : public StringAspect
{
    Q_DECLARE_TR_FUNCTIONS(Qdb::Internal::QdbRunConfiguration);

public:
    explicit FullCommandLineAspect(RunConfiguration *rc)
    {
        setLabelText(tr("Full command line:"));

        auto exeAspect = rc->aspect<ExecutableAspect>();
        auto argumentsAspect = rc->aspect<ArgumentsAspect>();

        auto updateCommandLine = [this, exeAspect, argumentsAspect] {
            CommandLine plain{exeAspect->executable(), argumentsAspect->arguments(), CommandLine::Raw};
            CommandLine cmd;
            cmd.setExecutable(plain.executable().withNewPath(Constants::AppcontrollerFilepath));
            cmd.addCommandLineAsArgs(plain);
            setValue(cmd.toUserOutput());
        };

        connect(argumentsAspect, &ArgumentsAspect::changed, this, updateCommandLine);
        connect(exeAspect, &ExecutableAspect::changed, this, updateCommandLine);
        updateCommandLine();
    }
};


// QdbRunConfiguration

class QdbRunConfiguration : public RunConfiguration
{
    Q_DECLARE_TR_FUNCTIONS(Qdb::Internal::QdbRunConfiguration);

public:
    QdbRunConfiguration(Target *target, Utils::Id id);

private:
    Tasks checkForIssues() const override;
    QString defaultDisplayName() const;
};

QdbRunConfiguration::QdbRunConfiguration(Target *target, Id id)
    : RunConfiguration(target, id)
{
    auto exeAspect = addAspect<ExecutableAspect>(target, ExecutableAspect::RunDevice);
    exeAspect->setSettingsKey("QdbRunConfig.RemoteExecutable");
    exeAspect->setLabelText(tr("Executable on device:"));
    exeAspect->setPlaceHolderText(tr("Remote path not set"));
    exeAspect->makeOverridable("QdbRunConfig.AlternateRemoteExecutable",
                               "QdbRunCofig.UseAlternateRemoteExecutable");

    auto symbolsAspect = addAspect<SymbolFileAspect>();
    symbolsAspect->setSettingsKey("QdbRunConfig.LocalExecutable");
    symbolsAspect->setLabelText(tr("Executable on host:"));
    symbolsAspect->setDisplayStyle(SymbolFileAspect::LabelDisplay);

    auto envAspect = addAspect<RemoteLinux::RemoteLinuxEnvironmentAspect>(target);

    addAspect<ArgumentsAspect>(macroExpander());
    addAspect<WorkingDirectoryAspect>(macroExpander(), envAspect);
    addAspect<FullCommandLineAspect>(this);

    setUpdater([this, target, exeAspect, symbolsAspect] {
        const BuildTargetInfo bti = buildTargetInfo();
        const FilePath localExecutable = bti.targetFilePath;
        const DeployableFile depFile = target->deploymentData().deployableForLocalFile(localExecutable);
        IDevice::ConstPtr dev = DeviceKitAspect::device(target->kit());
        QTC_ASSERT(dev, return);
        exeAspect->setExecutable(dev->filePath(depFile.remoteFilePath()));
        symbolsAspect->setFilePath(localExecutable);
    });

    connect(target, &Target::buildSystemUpdated, this, &RunConfiguration::update);
    connect(target, &Target::deploymentDataChanged, this, &RunConfiguration::update);
    connect(target, &Target::kitChanged, this, &RunConfiguration::update);

    setDefaultDisplayName(tr("Run on Boot2Qt Device"));
}

Tasks QdbRunConfiguration::checkForIssues() const
{
    Tasks tasks;
    if (aspect<ExecutableAspect>()->executable().toString().isEmpty()) {
        tasks << BuildSystemTask(Task::Warning, tr("The remote executable must be set "
                                                   "in order to run on a Boot2Qt device."));
    }
    return tasks;
}

QString QdbRunConfiguration::defaultDisplayName() const
{
    return RunConfigurationFactory::decoratedTargetName(buildKey(), target());
}

// QdbRunConfigurationFactory

QdbRunConfigurationFactory::QdbRunConfigurationFactory()
{
    registerRunConfiguration<QdbRunConfiguration>("QdbLinuxRunConfiguration:");
    addSupportedTargetDeviceType(Constants::QdbLinuxOsType);
}

} // namespace Internal
} // namespace Qdb
