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

#include "cmakerunconfiguration.h"

#include "cmakeprojectconstants.h"

#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtoutputformatter.h>

#include <projectexplorer/localenvironmentaspect.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>

using namespace ProjectExplorer;

namespace CMakeProjectManager {
namespace Internal {

CMakeRunConfiguration::CMakeRunConfiguration(Target *target, Core::Id id)
    : RunConfiguration(target, id)
{
    // Workaround for QTCREATORBUG-19354:
    auto cmakeRunEnvironmentModifier = [target](Utils::Environment &env) {
        if (!Utils::HostOsInfo::isWindowsHost())
            return;

        const Kit *k = target->kit();
        const QtSupport::BaseQtVersion *qt = QtSupport::QtKitInformation::qtVersion(k);
        if (qt)
            env.prependOrSetPath(qt->qmakeProperty("QT_INSTALL_BINS"));
    };
    auto envAspect = addAspect<LocalEnvironmentAspect>(target, cmakeRunEnvironmentModifier);

    addAspect<ExecutableAspect>();
    addAspect<ArgumentsAspect>();
    addAspect<WorkingDirectoryAspect>(envAspect);
    addAspect<TerminalAspect>();

    connect(target->project(), &Project::parsingFinished,
            this, &CMakeRunConfiguration::updateTargetInformation);

    if (QtSupport::QtKitInformation::qtVersion(target->kit()))
        setOutputFormatter<QtSupport::QtOutputFormatter>();
}

void CMakeRunConfiguration::doAdditionalSetup(const RunConfigurationCreationInfo &info)
{
    Q_UNUSED(info);
    updateTargetInformation();
}

bool CMakeRunConfiguration::isBuildTargetValid() const
{
    return Utils::anyOf(target()->applicationTargets().list, [this](const BuildTargetInfo &bti) {
        return bti.buildKey == buildKey();
    });
}

void CMakeRunConfiguration::updateEnabledState()
{
    if (!isBuildTargetValid())
        setEnabled(false);
    else
        RunConfiguration::updateEnabledState();
}

QString CMakeRunConfiguration::disabledReason() const
{
    if (!isBuildTargetValid())
        return tr("The project no longer builds the target associated with this run configuration.");
    return RunConfiguration::disabledReason();
}

void CMakeRunConfiguration::updateTargetInformation()
{
    BuildTargetInfo bti = target()->applicationTargets().buildTargetInfo(buildKey());
    aspect<ExecutableAspect>()->setExecutable(bti.targetFilePath);
    aspect<WorkingDirectoryAspect>()->setDefaultWorkingDirectory(bti.workingDirectory);
    aspect<LocalEnvironmentAspect>()->buildEnvironmentHasChanged();

    auto terminalAspect = aspect<TerminalAspect>();
    if (!terminalAspect->isUserSet())
        terminalAspect->setUseTerminal(bti.usesTerminal);
}

// Factory
CMakeRunConfigurationFactory::CMakeRunConfigurationFactory()
{
    registerRunConfiguration<CMakeRunConfiguration>("CMakeProjectManager.CMakeRunConfiguration.");
    addSupportedProjectType(CMakeProjectManager::Constants::CMAKEPROJECT_ID);
    addSupportedTargetDeviceType(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);

    addRunWorkerFactory<SimpleTargetRunner>(ProjectExplorer::Constants::NORMAL_RUN_MODE);
}

} // Internal
} // CMakeProjectManager
