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

#include <utils/qtcassert.h>

#include <QFormLayout>

using namespace ProjectExplorer;

namespace CMakeProjectManager {
namespace Internal {

const char CMAKE_RC_PREFIX[] = "CMakeProjectManager.CMakeRunConfiguration.";
const char TITLE_KEY[] = "CMakeProjectManager.CMakeRunConfiguation.Title";

// Configuration widget
class CMakeRunConfigurationWidget : public QWidget
{
public:
    CMakeRunConfigurationWidget(RunConfiguration *rc)
    {
        auto fl = new QFormLayout(this);

        rc->extraAspect<ExecutableAspect>()->addToMainConfigurationWidget(this, fl);
        rc->extraAspect<ArgumentsAspect>()->addToMainConfigurationWidget(this, fl);
        rc->extraAspect<WorkingDirectoryAspect>()->addToMainConfigurationWidget(this, fl);
        rc->extraAspect<TerminalAspect>()->addToMainConfigurationWidget(this, fl);
    }
};

CMakeRunConfiguration::CMakeRunConfiguration(Target *target)
    : RunConfiguration(target, CMAKE_RC_PREFIX)
{
    // Workaround for QTCREATORBUG-19354:
    auto cmakeRunEnvironmentModifier = [](RunConfiguration *rc, Utils::Environment &env) {
        if (!Utils::HostOsInfo::isWindowsHost() || !rc)
            return;

        const Kit *k = rc->target()->kit();
        const QtSupport::BaseQtVersion *qt = QtSupport::QtKitInformation::qtVersion(k);
        if (qt)
            env.prependOrSetPath(qt->qmakeProperty("QT_INSTALL_BINS"));
    };
    addExtraAspect(new LocalEnvironmentAspect(this, cmakeRunEnvironmentModifier));
    addExtraAspect(new ExecutableAspect(this));
    addExtraAspect(new ArgumentsAspect(this, "CMakeProjectManager.CMakeRunConfiguration.Arguments"));
    addExtraAspect(new TerminalAspect(this, "CMakeProjectManager.CMakeRunConfiguration.UseTerminal"));
    addExtraAspect(new WorkingDirectoryAspect(this, "CMakeProjectManager.CMakeRunConfiguration.UserWorkingDirectory"));

    connect(target->project(), &Project::parsingFinished,
            this, &CMakeRunConfiguration::updateTargetInformation);
}

Runnable CMakeRunConfiguration::runnable() const
{
    StandardRunnable r;
    r.executable = extraAspect<ExecutableAspect>()->executable().toString();
    r.commandLineArguments = extraAspect<ArgumentsAspect>()->arguments();
    r.workingDirectory = extraAspect<WorkingDirectoryAspect>()->workingDirectory().toString();
    r.environment = extraAspect<LocalEnvironmentAspect>()->environment();
    r.runMode = extraAspect<TerminalAspect>()->runMode();
    return r;
}

QVariantMap CMakeRunConfiguration::toMap() const
{
    QVariantMap map(RunConfiguration::toMap());
    map.insert(QLatin1String(TITLE_KEY), m_title);
    return map;
}

bool CMakeRunConfiguration::fromMap(const QVariantMap &map)
{
    RunConfiguration::fromMap(map);
    m_title = map.value(QLatin1String(TITLE_KEY)).toString();
    return true;
}

void CMakeRunConfiguration::doAdditionalSetup(const RunConfigurationCreationInfo &info)
{
    m_title = info.displayName;
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

QWidget *CMakeRunConfiguration::createConfigurationWidget()
{
    return wrapWidget(new CMakeRunConfigurationWidget(this));
}

QString CMakeRunConfiguration::disabledReason() const
{
    if (!isBuildTargetValid())
        return tr("The project no longer builds the target associated with this run configuration.");
    return RunConfiguration::disabledReason();
}

Utils::OutputFormatter *CMakeRunConfiguration::createOutputFormatter() const
{
    if (QtSupport::QtKitInformation::qtVersion(target()->kit()))
        return new QtSupport::QtOutputFormatter(target()->project());
    return RunConfiguration::createOutputFormatter();
}

void CMakeRunConfiguration::updateTargetInformation()
{
    setDefaultDisplayName(m_title);

    BuildTargetInfo bti = target()->applicationTargets().buildTargetInfo(buildKey());
    extraAspect<ExecutableAspect>()->setExecutable(bti.targetFilePath);
    extraAspect<WorkingDirectoryAspect>()->setDefaultWorkingDirectory(bti.workingDirectory);
    extraAspect<LocalEnvironmentAspect>()->buildEnvironmentHasChanged();

    auto terminalAspect = extraAspect<TerminalAspect>();
    if (!terminalAspect->isUserSet())
        terminalAspect->setUseTerminal(bti.usesTerminal);
}

// Factory
CMakeRunConfigurationFactory::CMakeRunConfigurationFactory()
{
    registerRunConfiguration<CMakeRunConfiguration>(CMAKE_RC_PREFIX);
    addSupportedProjectType(CMakeProjectManager::Constants::CMAKEPROJECT_ID);
    addSupportedTargetDeviceType(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);
}

} // Internal
} // CMakeProjectManager
