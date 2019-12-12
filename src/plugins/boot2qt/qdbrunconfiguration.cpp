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

FullCommandLineAspect::FullCommandLineAspect(RunConfiguration *rc)
{
    setLabelText(QdbRunConfiguration::tr("Full command line:"));

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

// QdbRunConfiguration

QdbRunConfiguration::QdbRunConfiguration(Target *target, Core::Id id)
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

    connect(target, &Target::deploymentDataChanged,
            this, &QdbRunConfiguration::updateTargetInformation);
    connect(target, &Target::applicationTargetsChanged,
            this, &QdbRunConfiguration::updateTargetInformation);
    connect(target, &Target::kitChanged,
            this, &QdbRunConfiguration::updateTargetInformation);
    connect(target->project(), &Project::parsingFinished,
            this, &QdbRunConfiguration::updateTargetInformation);

    setDefaultDisplayName(tr("Run on Boot2Qt Device"));
}

ProjectExplorer::RunConfiguration::ConfigurationState QdbRunConfiguration::ensureConfigured(QString *errorMessage)
{
    QString remoteExecutable = aspect<ExecutableAspect>()->executable().toString();
    if (remoteExecutable.isEmpty()) {
        if (errorMessage) {
            *errorMessage = tr("The remote executable must be set "
                               "in order to run on a Boot2Qt device.");
        }
        return UnConfigured;
    }
    return Configured;
}

void QdbRunConfiguration::doAdditionalSetup(const RunConfigurationCreationInfo &)
{
    updateTargetInformation();
}

void QdbRunConfiguration::updateTargetInformation()
{
    const BuildTargetInfo bti = buildTargetInfo();
    const FilePath localExecutable = bti.targetFilePath;
    const DeployableFile depFile = target()->deploymentData().deployableForLocalFile(localExecutable);

    aspect<ExecutableAspect>()->setExecutable(FilePath::fromString(depFile.remoteFilePath()));
    aspect<SymbolFileAspect>()->setFilePath(localExecutable);
}

QString QdbRunConfiguration::defaultDisplayName() const
{
    return RunConfigurationFactory::decoratedTargetName(buildKey(), target());
}

// QdbRunConfigurationFactory

QdbRunConfigurationFactory::QdbRunConfigurationFactory()
{
    registerRunConfiguration<QdbRunConfiguration>(Qdb::Constants::QdbRunConfigurationPrefix);
    addSupportedTargetDeviceType(Constants::QdbLinuxOsType);
}

} // namespace Internal
} // namespace Qdb
