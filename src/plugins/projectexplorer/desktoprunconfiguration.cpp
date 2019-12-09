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

#include "desktoprunconfiguration.h"

#include "localenvironmentaspect.h"
#include "project.h"
#include "runconfigurationaspects.h"
#include "target.h"

#include <cmakeprojectmanager/cmakeprojectconstants.h>
#include <qbsprojectmanager/qbsprojectmanagerconstants.h>
#include <qmakeprojectmanager/qmakeprojectmanagerconstants.h>

#include <utils/fileutils.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <QFileInfo>

using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

DesktopRunConfiguration::DesktopRunConfiguration(Target *target, Core::Id id, Kind kind)
    : RunConfiguration(target, id), m_kind(kind)
{
    auto envAspect = addAspect<LocalEnvironmentAspect>(target);

    addAspect<ExecutableAspect>();
    addAspect<ArgumentsAspect>();
    addAspect<WorkingDirectoryAspect>();
    addAspect<TerminalAspect>();

    auto libAspect = addAspect<UseLibraryPathsAspect>();
    connect(libAspect, &UseLibraryPathsAspect::changed,
            envAspect, &EnvironmentAspect::environmentChanged);

    if (HostOsInfo::isMacHost()) {
        auto dyldAspect = addAspect<UseDyldSuffixAspect>();
        connect(dyldAspect, &UseLibraryPathsAspect::changed,
                envAspect, &EnvironmentAspect::environmentChanged);
        envAspect->addModifier([dyldAspect](Environment &env) {
            if (dyldAspect->value())
                env.set(QLatin1String("DYLD_IMAGE_SUFFIX"), QLatin1String("_debug"));
        });
    }

    envAspect->addModifier([this, libAspect](Environment &env) {
        BuildTargetInfo bti = buildTargetInfo();
        if (bti.runEnvModifier)
            bti.runEnvModifier(env, libAspect->value());
    });

    if (kind == Qbs) {

        connect(project(), &Project::parsingFinished,
                envAspect, &EnvironmentAspect::environmentChanged);

        connect(target, &Target::deploymentDataChanged,
                this, &DesktopRunConfiguration::updateTargetInformation);
        connect(target, &Target::applicationTargetsChanged,
                this, &DesktopRunConfiguration::updateTargetInformation);
        // Handles device changes, etc.
        connect(target, &Target::kitChanged,
                this, &DesktopRunConfiguration::updateTargetInformation);

    } else if (m_kind == CMake) {

        libAspect->setVisible(false);

    }

    connect(target->project(), &Project::parsingFinished,
            this, &DesktopRunConfiguration::updateTargetInformation);
}

void DesktopRunConfiguration::updateTargetInformation()
{
    BuildTargetInfo bti = buildTargetInfo();

    auto terminalAspect = aspect<TerminalAspect>();
    terminalAspect->setUseTerminalHint(bti.usesTerminal);

    if (m_kind == Qmake) {

        FilePath profile = FilePath::fromString(buildKey());
        if (profile.isEmpty())
            setDefaultDisplayName(tr("Qt Run Configuration"));
        else
            setDefaultDisplayName(profile.toFileInfo().completeBaseName());

        aspect<EnvironmentAspect>()->environmentChanged();

        auto wda = aspect<WorkingDirectoryAspect>();
        wda->setDefaultWorkingDirectory(bti.workingDirectory);
        if (wda->pathChooser())
            wda->pathChooser()->setBaseFileName(target()->project()->projectDirectory());

        aspect<ExecutableAspect>()->setExecutable(bti.targetFilePath);

    }  else if (m_kind == Qbs) {

        setDefaultDisplayName(bti.displayName);
        const FilePath executable = executableToRun(bti);

        aspect<ExecutableAspect>()->setExecutable(executable);

        if (!executable.isEmpty()) {
            QString defaultWorkingDir = QFileInfo(executable.toString()).absolutePath();
            if (!defaultWorkingDir.isEmpty()) {
                auto wdAspect = aspect<WorkingDirectoryAspect>();
                wdAspect->setDefaultWorkingDirectory(FilePath::fromString(defaultWorkingDir));
            }
        }

        emit enabledChanged();

    } else if (m_kind == CMake) {

        aspect<ExecutableAspect>()->setExecutable(bti.targetFilePath);
        aspect<WorkingDirectoryAspect>()->setDefaultWorkingDirectory(bti.workingDirectory);
        aspect<LocalEnvironmentAspect>()->environmentChanged();

    }
}

bool DesktopRunConfiguration::fromMap(const QVariantMap &map)
{
    if (!RunConfiguration::fromMap(map))
        return false;

    updateTargetInformation();

    return true;
}

void DesktopRunConfiguration::doAdditionalSetup(const RunConfigurationCreationInfo &info)
{
    if (m_kind == Qbs)
        setDefaultDisplayName(info.displayName);

    updateTargetInformation();
}

Utils::FilePath DesktopRunConfiguration::executableToRun(const BuildTargetInfo &targetInfo) const
{
    const FilePath appInBuildDir = targetInfo.targetFilePath;
    const DeploymentData deploymentData = target()->deploymentData();
    if (deploymentData.localInstallRoot().isEmpty())
        return appInBuildDir;

    const QString deployedAppFilePath = deploymentData
            .deployableForLocalFile(appInBuildDir).remoteFilePath();
    if (deployedAppFilePath.isEmpty())
        return appInBuildDir;

    const FilePath appInLocalInstallDir = deploymentData.localInstallRoot() + deployedAppFilePath;
    return appInLocalInstallDir.exists() ? appInLocalInstallDir : appInBuildDir;
}

bool DesktopRunConfiguration::isBuildTargetValid() const
{
    return Utils::anyOf(target()->applicationTargets(), [this](const BuildTargetInfo &bti) {
        return bti.buildKey == buildKey();
    });
}

void DesktopRunConfiguration::updateEnabledState()
{
    if (m_kind == CMake && !isBuildTargetValid())
        setEnabled(false);
    else
        RunConfiguration::updateEnabledState();
}

QString DesktopRunConfiguration::disabledReason() const
{
    if (m_kind == CMake && !isBuildTargetValid())
        return tr("The project no longer builds the target associated with this run configuration.");
    return RunConfiguration::disabledReason();
}

// Factory

class DesktopQmakeRunConfiguration : public DesktopRunConfiguration
{
public:
    DesktopQmakeRunConfiguration(Target *target, Core::Id id)
        : DesktopRunConfiguration(target, id, Qmake)
    {}
};

class QbsRunConfiguration : public DesktopRunConfiguration
{
public:
    QbsRunConfiguration(Target *target, Core::Id id)
        : DesktopRunConfiguration(target, id, Qbs)
    {}
};

class CMakeRunConfiguration : public DesktopRunConfiguration
{
public:
    CMakeRunConfiguration(Target *target, Core::Id id)
        : DesktopRunConfiguration(target, id, CMake)
    {}
};

const char QMAKE_RUNCONFIG_ID[] = "Qt4ProjectManager.Qt4RunConfiguration:";
const char QBS_RUNCONFIG_ID[]   = "Qbs.RunConfiguration:";
const char CMAKE_RUNCONFIG_ID[] = "CMakeProjectManager.CMakeRunConfiguration.";

CMakeRunConfigurationFactory::CMakeRunConfigurationFactory()
{
    registerRunConfiguration<CMakeRunConfiguration>(CMAKE_RUNCONFIG_ID);
    addSupportedProjectType(CMakeProjectManager::Constants::CMAKEPROJECT_ID);
    addSupportedTargetDeviceType(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);
}

QbsRunConfigurationFactory::QbsRunConfigurationFactory()
{
    registerRunConfiguration<QbsRunConfiguration>(QBS_RUNCONFIG_ID);
    addSupportedProjectType(QbsProjectManager::Constants::PROJECT_ID);
    addSupportedTargetDeviceType(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);
}

DesktopQmakeRunConfigurationFactory::DesktopQmakeRunConfigurationFactory()
{
    registerRunConfiguration<DesktopQmakeRunConfiguration>(QMAKE_RUNCONFIG_ID);
    addSupportedProjectType(QmakeProjectManager::Constants::QMAKEPROJECT_ID);
    addSupportedTargetDeviceType(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);
}

} // namespace Internal
} // namespace ProjectExplorer
