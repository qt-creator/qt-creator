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

#include "remotelinuxcustomrunconfiguration.h"

#include "remotelinux_constants.h"
#include "remotelinuxenvironmentaspect.h"

#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtoutputformatter.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux {
namespace Internal {

RemoteLinuxCustomRunConfiguration::RemoteLinuxCustomRunConfiguration(Target *target, Core::Id id)
    : RunConfiguration(target, id)
{
    auto exeAspect = new ExecutableAspect(this);
    exeAspect->setSettingsKey("RemoteLinux.CustomRunConfig.RemoteExecutable");
    exeAspect->setLabelText(tr("Remote executable:"));
    exeAspect->setExecutablePathStyle(OsTypeLinux);
    exeAspect->setDisplayStyle(BaseStringAspect::LineEditDisplay);
    exeAspect->setHistoryCompleter("RemoteLinux.CustomExecutable.History");
    exeAspect->setExpectedKind(PathChooser::Any);
    addExtraAspect(exeAspect);

    auto symbolsAspect = new SymbolFileAspect(this);
    symbolsAspect->setSettingsKey("RemoteLinux.CustomRunConfig.LocalExecutable");
    symbolsAspect->setLabelText(tr("Local executable:"));
    symbolsAspect->setDisplayStyle(SymbolFileAspect::PathChooserDisplay);
    addExtraAspect(symbolsAspect);

    addExtraAspect(new ArgumentsAspect(this, "RemoteLinux.CustomRunConfig.Arguments"));
    addExtraAspect(new WorkingDirectoryAspect(this, "RemoteLinux.CustomRunConfig.WorkingDirectory"));
    addExtraAspect(new RemoteLinuxEnvironmentAspect(this));

    setDefaultDisplayName(runConfigDefaultDisplayName());
    setOutputFormatter<QtSupport::QtOutputFormatter>();
}

bool RemoteLinuxCustomRunConfiguration::isConfigured() const
{
    return !extraAspect<ExecutableAspect>()->executable().isEmpty();
}

RunConfiguration::ConfigurationState
RemoteLinuxCustomRunConfiguration::ensureConfigured(QString *errorMessage)
{
    if (!isConfigured()) {
        if (errorMessage) {
            *errorMessage = tr("The remote executable must be set "
                               "in order to run a custom remote run configuration.");
        }
        return UnConfigured;
    }
    return Configured;
}

Core::Id RemoteLinuxCustomRunConfiguration::runConfigId()
{
    return "RemoteLinux.CustomRunConfig";
}

QString RemoteLinuxCustomRunConfiguration::runConfigDefaultDisplayName()
{
    QString remoteExecutable = extraAspect<ExecutableAspect>()->executable().toString();
    QString display = remoteExecutable.isEmpty()
            ? tr("Custom Executable") : tr("Run \"%1\"").arg(remoteExecutable);
    return  RunConfigurationFactory::decoratedTargetName(display, target());
}

// RemoteLinuxCustomRunConfigurationFactory

RemoteLinuxCustomRunConfigurationFactory::RemoteLinuxCustomRunConfigurationFactory()
    : FixedRunConfigurationFactory(RemoteLinuxCustomRunConfiguration::tr("Custom Executable"), true)
{
    registerRunConfiguration<RemoteLinuxCustomRunConfiguration>
            (RemoteLinuxCustomRunConfiguration::runConfigId());
    addSupportedTargetDeviceType(RemoteLinux::Constants::GenericLinuxOsType);
}

} // namespace Internal
} // namespace RemoteLinux
