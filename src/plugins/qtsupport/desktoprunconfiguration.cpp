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

#include "qtkitinformation.h"
#include "qtoutputformatter.h"

#include <projectexplorer/localenvironmentaspect.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>

#include <cmakeprojectmanager/cmakeprojectconstants.h>
#include <qbsprojectmanager/qbsprojectmanagerconstants.h>
#include <qmakeprojectmanager/qmakeprojectmanagerconstants.h>

#include <utils/fileutils.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <QFileInfo>

using namespace ProjectExplorer;
using namespace Utils;

namespace QtSupport {
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

    if (m_kind == Qmake) {

        envAspect->addModifier([this](Environment &env) {
            BuildTargetInfo bti = buildTargetInfo();
            if (bti.runEnvModifier)
                bti.runEnvModifier(env, aspect<UseLibraryPathsAspect>()->value());
        });

    } else if (kind == Qbs) {

        envAspect->addModifier([this](Environment &env) {
            bool usingLibraryPaths = aspect<UseLibraryPathsAspect>()->value();

            BuildTargetInfo bti = buildTargetInfo();
            if (bti.runEnvModifier)
                bti.runEnvModifier(env, usingLibraryPaths);
        });

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

        // Workaround for QTCREATORBUG-19354:
        if (HostOsInfo::isWindowsHost()) {
            envAspect->addModifier([target](Environment &env) {
                const Kit *k = target->kit();
                if (const QtSupport::BaseQtVersion *qt = QtSupport::QtKitAspect::qtVersion(k)) {
                    const QString installBinPath = qt->qmakeProperty("QT_INSTALL_BINS");
                    env.prependOrSetPath(installBinPath);
                }
            });
        }

    }

    setOutputFormatter<QtSupport::QtOutputFormatter>();

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

    if (m_kind == Qmake || m_kind == Qbs)
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
    if (target()->deploymentData().localInstallRoot().isEmpty())
        return appInBuildDir;
    const QString deployedAppFilePath = target()->deploymentData()
            .deployableForLocalFile(appInBuildDir.toString()).remoteFilePath();
    if (deployedAppFilePath.isEmpty())
        return appInBuildDir;
    const FilePath appInLocalInstallDir = target()->deploymentData().localInstallRoot()
            + deployedAppFilePath;
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

DesktopQmakeRunConfiguration::DesktopQmakeRunConfiguration(Target *target, Core::Id id)
    : DesktopRunConfiguration(target, id, Qmake)
{}

QbsRunConfiguration::QbsRunConfiguration(Target *target, Core::Id id)
    : DesktopRunConfiguration(target, id, Qbs)
{}

CMakeRunConfiguration::CMakeRunConfiguration(Target *target, Core::Id id)
    : DesktopRunConfiguration(target, id, CMake)
{}

CMakeRunConfigurationFactory::CMakeRunConfigurationFactory()
{
    registerRunConfiguration<CMakeRunConfiguration>("CMakeProjectManager.CMakeRunConfiguration.");
    addSupportedProjectType(CMakeProjectManager::Constants::CMAKEPROJECT_ID);
    addSupportedTargetDeviceType(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);
}

QbsRunConfigurationFactory::QbsRunConfigurationFactory()
{
    registerRunConfiguration<QbsRunConfiguration>("Qbs.RunConfiguration:");
    addSupportedProjectType(QbsProjectManager::Constants::PROJECT_ID);
    addSupportedTargetDeviceType(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);
}

DesktopQmakeRunConfigurationFactory::DesktopQmakeRunConfigurationFactory()
{
    registerRunConfiguration<DesktopQmakeRunConfiguration>("Qt4ProjectManager.Qt4RunConfiguration:");
    addSupportedProjectType(QmakeProjectManager::Constants::QMAKEPROJECT_ID);
    addSupportedTargetDeviceType(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);
}

} // namespace Internal
} // namespace ProjectExplorer
