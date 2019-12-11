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

#include "nimblerunconfiguration.h"
#include "nimconstants.h"
#include "nimbleproject.h"

#include <projectexplorer/localenvironmentaspect.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/target.h>

#include <utils/algorithm.h>
#include <utils/environment.h>

#include <QStandardPaths>

using namespace Nim;
using namespace ProjectExplorer;

NimbleRunConfiguration::NimbleRunConfiguration(ProjectExplorer::Target *target, Core::Id id)
    : RunConfiguration(target, id)
{
    addAspect<LocalEnvironmentAspect>(target);
    addAspect<ExecutableAspect>();
    addAspect<ArgumentsAspect>();
    addAspect<WorkingDirectoryAspect>();
    addAspect<TerminalAspect>();

    setUpdater([this] {
        BuildTargetInfo bti = buildTargetInfo();
        setDisplayName(bti.displayName);
        setDefaultDisplayName(bti.displayName);
        aspect<ExecutableAspect>()->setExecutable(bti.targetFilePath);
        aspect<WorkingDirectoryAspect>()->setDefaultWorkingDirectory(bti.workingDirectory);
    });

    connect(target, &Target::buildSystemUpdated, this, &RunConfiguration::update);
    update();
}

bool NimbleRunConfiguration::isBuildTargetValid() const
{
    return Utils::anyOf(target()->applicationTargets(), [this](const BuildTargetInfo &bti) {
        return bti.buildKey == buildKey();
    });
}

QString NimbleRunConfiguration::disabledReason() const
{
    if (!isBuildTargetValid())
        return tr("The project no longer builds the target associated with this run configuration.");
    return RunConfiguration::disabledReason();
}

NimbleRunConfigurationFactory::NimbleRunConfigurationFactory()
    : RunConfigurationFactory()
{
    registerRunConfiguration<NimbleRunConfiguration>("Nim.NimbleRunConfiguration");
    addSupportedProjectType(Constants::C_NIMBLEPROJECT_ID);
    addSupportedTargetDeviceType(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);
}

QList<RunConfigurationCreationInfo> NimbleRunConfigurationFactory::availableCreators(Target *parent) const
{
    return RunConfigurationFactory::availableCreators(parent);
}

NimbleTestConfiguration::NimbleTestConfiguration(Target *target, Core::Id id)
    : RunConfiguration(target, id)
{
    addAspect<ExecutableAspect>()->setExecutable(Utils::FilePath::fromString(QStandardPaths::findExecutable("nimble")));
    addAspect<ArgumentsAspect>()->setArguments("test");
    addAspect<WorkingDirectoryAspect>()->setDefaultWorkingDirectory(project()->projectDirectory());
    addAspect<TerminalAspect>();

    setDisplayName(tr("Nimble Test"));
    setDefaultDisplayName(tr("Nimble Test"));
}

NimbleTestConfigurationFactory::NimbleTestConfigurationFactory()
    : FixedRunConfigurationFactory(QString())
{
    registerRunConfiguration<NimbleTestConfiguration>("Nim.NimbleTestConfiguration");
    addSupportedProjectType(Constants::C_NIMBLEPROJECT_ID);
}
