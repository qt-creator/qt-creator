/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qdbrunconfiguration.h"

#include "qdbconstants.h"

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>

#include <remotelinux/remotelinuxenvironmentaspect.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Qdb {
namespace Internal {

// FullCommandLineAspect

class FullCommandLineAspect : public BaseStringAspect
{
    Q_DECLARE_TR_FUNCTIONS(Qdb::Internal::QdbRunConfiguration);

public:
    explicit FullCommandLineAspect(RunConfiguration *rc)
    {
        setLabelText(tr("Full command line:"));

        auto exeAspect = rc->aspect<ExecutableAspect>();
        auto argumentsAspect = rc->aspect<ArgumentsAspect>();

        auto updateCommandLine = [this, rc, exeAspect, argumentsAspect] {
            const QString usedExecutable = exeAspect->executable().toString();
            const QString args = argumentsAspect->arguments(rc->macroExpander());
            setValue(QString(Constants::AppcontrollerFilepath)
                     + ' ' + usedExecutable + ' ' + args);
        };

        connect(argumentsAspect, &ArgumentsAspect::argumentsChanged, this, updateCommandLine);
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

QdbRunConfiguration::QdbRunConfiguration(Target *target, Utils::Id id)
    : RunConfiguration(target, id)
{
    auto exeAspect = addAspect<ExecutableAspect>();
    exeAspect->setSettingsKey("QdbRunConfig.RemoteExecutable");
    exeAspect->setLabelText(tr("Executable on device:"));
    exeAspect->setExecutablePathStyle(OsTypeLinux);
    exeAspect->setPlaceHolderText(tr("Remote path not set"));
    exeAspect->makeOverridable("QdbRunConfig.AlternateRemoteExecutable",
                               "QdbRunCofig.UseAlternateRemoteExecutable");

    auto symbolsAspect = addAspect<SymbolFileAspect>();
    symbolsAspect->setSettingsKey("QdbRunConfig.LocalExecutable");
    symbolsAspect->setLabelText(tr("Executable on host:"));
    symbolsAspect->setDisplayStyle(SymbolFileAspect::LabelDisplay);

    addAspect<RemoteLinux::RemoteLinuxEnvironmentAspect>(target);
    addAspect<ArgumentsAspect>();
    addAspect<WorkingDirectoryAspect>();
    addAspect<FullCommandLineAspect>(this);

    setUpdater([this, target, exeAspect, symbolsAspect] {
        const BuildTargetInfo bti = buildTargetInfo();
        const FilePath localExecutable = bti.targetFilePath;
        const DeployableFile depFile = target->deploymentData().deployableForLocalFile(localExecutable);

        exeAspect->setExecutable(FilePath::fromString(depFile.remoteFilePath()));
        symbolsAspect->setFilePath(localExecutable);
    });

    connect(target, &Target::buildSystemUpdated, this, &RunConfiguration::update);

    setDefaultDisplayName(tr("Run on Boot2Qt Device"));
}

Tasks QdbRunConfiguration::checkForIssues() const
{
    Tasks tasks;
    if (aspect<ExecutableAspect>()->executable().toString().isEmpty()) {
        tasks << createConfigurationIssue(tr("The remote executable must be set "
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
