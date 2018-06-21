/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "remotelinuxrunconfiguration.h"

#include "remotelinux_constants.h"
#include "remotelinuxenvironmentaspect.h"

#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtoutputformatter.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux {

RemoteLinuxRunConfiguration::RemoteLinuxRunConfiguration(Target *target, Core::Id id)
    : RunConfiguration(target, id)
{
    auto exeAspect = new ExecutableAspect(this);
    exeAspect->setLabelText(tr("Executable on device:"));
    exeAspect->setExecutablePathStyle(OsTypeLinux);
    exeAspect->setPlaceHolderText(tr("Remote path not set"));
    exeAspect->makeOverridable("RemoteLinux.RunConfig.AlternateRemoteExecutable",
                               "RemoteLinux.RunConfig.UseAlternateRemoteExecutable");
    exeAspect->setHistoryCompleter("RemoteLinux.AlternateExecutable.History");
    addExtraAspect(exeAspect);

    auto symbolsAspect = new SymbolFileAspect(this);
    symbolsAspect->setLabelText(tr("Executable on host:"));
    symbolsAspect->setDisplayStyle(SymbolFileAspect::LabelDisplay);
    addExtraAspect(symbolsAspect);

    auto argsAspect = new ArgumentsAspect(this);
    argsAspect->setSettingsKey("Qt4ProjectManager.MaemoRunConfiguration.Arguments");
    addExtraAspect(argsAspect);

    auto wdAspect = new WorkingDirectoryAspect(this);
    wdAspect->setSettingsKey("RemoteLinux.RunConfig.WorkingDirectory");
    addExtraAspect(wdAspect);

    addExtraAspect(new RemoteLinuxEnvironmentAspect(this));

    setOutputFormatter<QtSupport::QtOutputFormatter>();

    connect(target, &Target::deploymentDataChanged,
            this, &RemoteLinuxRunConfiguration::updateTargetInformation);
    connect(target, &Target::applicationTargetsChanged,
            this, &RemoteLinuxRunConfiguration::updateTargetInformation);
    connect(target->project(), &Project::parsingFinished,
            this, &RemoteLinuxRunConfiguration::updateTargetInformation);
    connect(target, &Target::kitChanged,
            this, &RemoteLinuxRunConfiguration::updateTargetInformation);
}

void RemoteLinuxRunConfiguration::doAdditionalSetup(const RunConfigurationCreationInfo &)
{
    setDefaultDisplayName(defaultDisplayName());
}

QString RemoteLinuxRunConfiguration::defaultDisplayName() const
{
    return RunConfigurationFactory::decoratedTargetName(buildKey(), target());
}

void RemoteLinuxRunConfiguration::updateTargetInformation()
{
    BuildTargetInfo bti = buildTargetInfo();
    QString localExecutable = bti.targetFilePath.toString();
    DeployableFile depFile = target()->deploymentData().deployableForLocalFile(localExecutable);

    extraAspect<ExecutableAspect>()->setExecutable(FileName::fromString(depFile.remoteFilePath()));
    extraAspect<SymbolFileAspect>()->setValue(localExecutable);

    emit enabledChanged();
}

const char *RemoteLinuxRunConfiguration::IdPrefix = "RemoteLinuxRunConfiguration:";


// RemoteLinuxRunConfigurationFactory

RemoteLinuxRunConfigurationFactory::RemoteLinuxRunConfigurationFactory()
{
    registerRunConfiguration<RemoteLinuxRunConfiguration>(RemoteLinuxRunConfiguration::IdPrefix);
    addSupportedTargetDeviceType(RemoteLinux::Constants::GenericLinuxOsType);
}

} // namespace RemoteLinux
