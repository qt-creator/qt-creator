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

#include "../nimconstants.h"

#include <projectexplorer/localenvironmentaspect.h>
#include <projectexplorer/runconfigurationaspects.h>

#include <utils/environment.h>

#include <QDir>
#include <QFileInfo>

using namespace ProjectExplorer;
using namespace Utils;

namespace Nim {

NimRunConfiguration::NimRunConfiguration(Target *target, Core::Id id)
    : RunConfiguration(target, id)
{
    auto envAspect = addAspect<LocalEnvironmentAspect>
            (target, LocalEnvironmentAspect::BaseEnvironmentModifier());

    addAspect<ExecutableAspect>();
    addAspect<ArgumentsAspect>();
    addAspect<WorkingDirectoryAspect>(envAspect);
    addAspect<TerminalAspect>();

    setDisplayName(tr("Current Build Target"));
    setDefaultDisplayName(tr("Current Build Target"));

    // Connect target signals
    connect(target, &Target::activeBuildConfigurationChanged,
            this, &NimRunConfiguration::updateConfiguration);
    updateConfiguration();
}

void NimRunConfiguration::updateConfiguration()
{
    auto buildConfiguration = qobject_cast<NimBuildConfiguration *>(activeBuildConfiguration());
    QTC_ASSERT(buildConfiguration, return);
    setActiveBuildConfiguration(buildConfiguration);
    const QFileInfo outFileInfo = buildConfiguration->outFilePath().toFileInfo();
    aspect<ExecutableAspect>()->setExecutable(FileName::fromString(outFileInfo.absoluteFilePath()));
    const QString workingDirectory = outFileInfo.absoluteDir().absolutePath();
    aspect<WorkingDirectoryAspect>()->setDefaultWorkingDirectory(FileName::fromString(workingDirectory));
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

// NimRunConfigurationFactory

NimRunConfigurationFactory::NimRunConfigurationFactory() : FixedRunConfigurationFactory(QString())
{
    registerRunConfiguration<NimRunConfiguration>("Nim.NimRunConfiguration");
    addSupportedProjectType(Constants::C_NIMPROJECT_ID);
    addRunWorkerFactory<SimpleTargetRunner>(ProjectExplorer::Constants::NORMAL_RUN_MODE);
}

} // Nim
