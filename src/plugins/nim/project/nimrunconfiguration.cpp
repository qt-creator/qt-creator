/****************************************************************************
**
** Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
** Contact: http://www.qt.io/licensing
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

#include "nimrunconfiguration.h"
#include "nimbuildconfiguration.h"
#include "nimrunconfigurationwidget.h"

#include "../nimconstants.h"

#include <projectexplorer/runnables.h>
#include <projectexplorer/localenvironmentaspect.h>
#include <projectexplorer/runconfigurationaspects.h>

#include <utils/environment.h>

#include <QDir>
#include <QFileInfo>
#include <QVariantMap>

using namespace ProjectExplorer;
using namespace Utils;

namespace Nim {

NimRunConfiguration::NimRunConfiguration(Target *target)
    : RunConfiguration(target)
    , m_workingDirectoryAspect(new WorkingDirectoryAspect(this, Nim::Constants::C_NIMRUNCONFIGURATION_WORKINGDIRECTORYASPECT_ID))
    , m_argumentAspect(new ArgumentsAspect(this, Nim::Constants::C_NIMRUNCONFIGURATION_ARGUMENTASPECT_ID))
    , m_terminalAspect(new TerminalAspect(this, Nim::Constants::C_NIMRUNCONFIGURATION_TERMINALASPECT_ID))
    , m_localEnvironmentAspect(new LocalEnvironmentAspect(this, LocalEnvironmentAspect::BaseEnvironmentModifier()))
{
    m_terminalAspect->setRunMode(ApplicationLauncher::Gui);

    addExtraAspect(m_argumentAspect);
    addExtraAspect(m_terminalAspect);
    addExtraAspect(m_localEnvironmentAspect);

    setDisplayName(tr(Constants::C_NIMRUNCONFIGURATION_DISPLAY));
    setDefaultDisplayName(tr(Constants::C_NIMRUNCONFIGURATION_DEFAULT_DISPLAY));

    // Connect target signals
    connect(target, &Target::activeBuildConfigurationChanged,
            this, &NimRunConfiguration::updateConfiguration);
    updateConfiguration();
}

QWidget *NimRunConfiguration::createConfigurationWidget()
{
    return new NimRunConfigurationWidget(this);
}

Runnable NimRunConfiguration::runnable() const
{
    StandardRunnable result;
    result.runMode = m_terminalAspect->runMode();
    result.executable = m_executable;
    result.commandLineArguments = m_argumentAspect->arguments();
    result.workingDirectory = m_workingDirectoryAspect->workingDirectory().toString();
    result.environment = m_localEnvironmentAspect->environment();
    return result;
}


QVariantMap NimRunConfiguration::toMap() const
{
    auto result = RunConfiguration::toMap();
    result[Constants::C_NIMRUNCONFIGURATION_EXECUTABLE_KEY] = m_executable;
    return result;
}

bool NimRunConfiguration::fromMap(const QVariantMap &map)
{
    bool result = RunConfiguration::fromMap(map);
    if (!result)
        return result;
    m_executable = map[Constants::C_NIMRUNCONFIGURATION_EXECUTABLE_KEY].toString();
    return true;
}

void NimRunConfiguration::setExecutable(const QString &executable)
{
    if (m_executable == executable)
        return;
    m_executable = executable;
    emit executableChanged(executable);
}

void NimRunConfiguration::setWorkingDirectory(const QString &workingDirectory)
{
    m_workingDirectoryAspect->setDefaultWorkingDirectory(FileName::fromString(workingDirectory));
}

void NimRunConfiguration::updateConfiguration()
{
    auto buildConfiguration = qobject_cast<NimBuildConfiguration *>(activeBuildConfiguration());
    QTC_ASSERT(buildConfiguration, return);
    setActiveBuildConfiguration(buildConfiguration);
    const QFileInfo outFileInfo = buildConfiguration->outFilePath().toFileInfo();
    setExecutable(outFileInfo.absoluteFilePath());
    setWorkingDirectory(outFileInfo.absoluteDir().absolutePath());
}

void NimRunConfiguration::setActiveBuildConfiguration(NimBuildConfiguration *activeBuildConfiguration)
{
    if (m_buildConfiguration == activeBuildConfiguration)
        return;

    if (m_buildConfiguration) {
        disconnect(m_buildConfiguration, &NimBuildConfiguration::buildDirectoryChanged,
                   this, &NimRunConfiguration::updateConfiguration);
        disconnect(m_buildConfiguration, &NimBuildConfiguration::outFilePathChanged,
                   this, &NimRunConfiguration::updateConfiguration);
    }

    m_buildConfiguration = activeBuildConfiguration;

    if (m_buildConfiguration) {
        connect(m_buildConfiguration, &NimBuildConfiguration::buildDirectoryChanged,
                this, &NimRunConfiguration::updateConfiguration);
        connect(m_buildConfiguration, &NimBuildConfiguration::outFilePathChanged,
                this, &NimRunConfiguration::updateConfiguration);
    }
}

}
