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

// DesktopQmakeRunConfiguration

DesktopQmakeRunConfiguration::DesktopQmakeRunConfiguration(Target *target, Core::Id id)
    : RunConfiguration(target, id)
{
    auto envAspect = addAspect<LocalEnvironmentAspect>(target);
    envAspect->addModifier([this](Environment &env) {
        BuildTargetInfo bti = buildTargetInfo();
        if (bti.runEnvModifier)
            bti.runEnvModifier(env, aspect<UseLibraryPathsAspect>()->value());
    });

    addAspect<ExecutableAspect>();
    addAspect<ArgumentsAspect>();
    addAspect<WorkingDirectoryAspect>();
    addAspect<TerminalAspect>();

    setOutputFormatter<QtSupport::QtOutputFormatter>();

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

    connect(target->project(), &Project::parsingFinished,
            this, &DesktopQmakeRunConfiguration::updateTargetInformation);
}

void DesktopQmakeRunConfiguration::updateTargetInformation()
{
    setDefaultDisplayName(defaultDisplayName());
    aspect<EnvironmentAspect>()->environmentChanged();

    BuildTargetInfo bti = buildTargetInfo();

    auto wda = aspect<WorkingDirectoryAspect>();
    wda->setDefaultWorkingDirectory(bti.workingDirectory);
    if (wda->pathChooser())
        wda->pathChooser()->setBaseFileName(target()->project()->projectDirectory());

    auto terminalAspect = aspect<TerminalAspect>();
    terminalAspect->setUseTerminalHint(bti.usesTerminal);

    aspect<ExecutableAspect>()->setExecutable(bti.targetFilePath);
}

bool DesktopQmakeRunConfiguration::fromMap(const QVariantMap &map)
{
    if (!RunConfiguration::fromMap(map))
        return false;
    updateTargetInformation();
    return true;
}

void DesktopQmakeRunConfiguration::doAdditionalSetup(const RunConfigurationCreationInfo &)
{
    updateTargetInformation();
}

FilePath DesktopQmakeRunConfiguration::proFilePath() const
{
    return FilePath::fromString(buildKey());
}

QString DesktopQmakeRunConfiguration::defaultDisplayName()
{
    FilePath profile = proFilePath();
    if (!profile.isEmpty())
        return profile.toFileInfo().completeBaseName();
    return tr("Qt Run Configuration");
}


// Qbs

QbsRunConfiguration::QbsRunConfiguration(Target *target, Core::Id id)
    : RunConfiguration(target, id)
{
    auto envAspect = addAspect<LocalEnvironmentAspect>(target);
    envAspect->addModifier([this](Environment &env) {
        bool usingLibraryPaths = aspect<UseLibraryPathsAspect>()->value();

        BuildTargetInfo bti = buildTargetInfo();
        if (bti.runEnvModifier)
            bti.runEnvModifier(env, usingLibraryPaths);
    });

    addAspect<ExecutableAspect>();
    addAspect<ArgumentsAspect>();
    addAspect<WorkingDirectoryAspect>();
    addAspect<TerminalAspect>();

    setOutputFormatter<QtSupport::QtOutputFormatter>();

    auto libAspect = addAspect<UseLibraryPathsAspect>();
    connect(libAspect, &UseLibraryPathsAspect::changed,
            envAspect, &EnvironmentAspect::environmentChanged);
    if (HostOsInfo::isMacHost()) {
        auto dyldAspect = addAspect<UseDyldSuffixAspect>();
        connect(dyldAspect, &UseDyldSuffixAspect::changed,
                envAspect, &EnvironmentAspect::environmentChanged);
        envAspect->addModifier([dyldAspect](Environment &env) {
            if (dyldAspect->value())
                env.set("DYLD_IMAGE_SUFFIX", "_debug");
        });
    }

    connect(project(), &Project::parsingFinished,
            envAspect, &EnvironmentAspect::environmentChanged);

    connect(target, &Target::deploymentDataChanged,
            this, &QbsRunConfiguration::updateTargetInformation);
    connect(target, &Target::applicationTargetsChanged,
            this, &QbsRunConfiguration::updateTargetInformation);
    // Handles device changes, etc.
    connect(target, &Target::kitChanged,
            this, &QbsRunConfiguration::updateTargetInformation);

    connect(target->project(), &Project::parsingFinished,
            this, &QbsRunConfiguration::updateTargetInformation);
}

QVariantMap QbsRunConfiguration::toMap() const
{
    return RunConfiguration::toMap();
}

bool QbsRunConfiguration::fromMap(const QVariantMap &map)
{
    if (!RunConfiguration::fromMap(map))
        return false;

    updateTargetInformation();
    return true;
}

void QbsRunConfiguration::doAdditionalSetup(const RunConfigurationCreationInfo &info)
{
    setDefaultDisplayName(info.displayName);
    updateTargetInformation();
}

Utils::FilePath QbsRunConfiguration::executableToRun(const BuildTargetInfo &targetInfo) const
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

void QbsRunConfiguration::updateTargetInformation()
{
    BuildTargetInfo bti = buildTargetInfo();
    setDefaultDisplayName(bti.displayName);
    const FilePath executable = executableToRun(bti);
    auto terminalAspect = aspect<TerminalAspect>();
    terminalAspect->setUseTerminalHint(bti.usesTerminal);

    aspect<ExecutableAspect>()->setExecutable(executable);

    if (!executable.isEmpty()) {
        QString defaultWorkingDir = QFileInfo(executable.toString()).absolutePath();
        if (!defaultWorkingDir.isEmpty()) {
            auto wdAspect = aspect<WorkingDirectoryAspect>();
            wdAspect->setDefaultWorkingDirectory(FilePath::fromString(defaultWorkingDir));
        }
    }

    emit enabledChanged();
}

// CMakeRunConfiguration

CMakeRunConfiguration::CMakeRunConfiguration(Target *target, Core::Id id)
    : RunConfiguration(target, id)
{
    auto envAspect = addAspect<LocalEnvironmentAspect>(target);

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

    addAspect<ExecutableAspect>();
    addAspect<ArgumentsAspect>();
    addAspect<WorkingDirectoryAspect>();
    addAspect<TerminalAspect>();

    connect(target->project(), &Project::parsingFinished,
            this, &CMakeRunConfiguration::updateTargetInformation);

    if (QtSupport::QtKitAspect::qtVersion(target->kit()))
        setOutputFormatter<QtSupport::QtOutputFormatter>();
}

void CMakeRunConfiguration::doAdditionalSetup(const RunConfigurationCreationInfo &info)
{
    Q_UNUSED(info)
    updateTargetInformation();
}

bool CMakeRunConfiguration::isBuildTargetValid() const
{
    return Utils::anyOf(target()->applicationTargets(), [this](const BuildTargetInfo &bti) {
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
    BuildTargetInfo bti = buildTargetInfo();
    aspect<ExecutableAspect>()->setExecutable(bti.targetFilePath);
    aspect<WorkingDirectoryAspect>()->setDefaultWorkingDirectory(bti.workingDirectory);
    aspect<LocalEnvironmentAspect>()->environmentChanged();

    auto terminalAspect = aspect<TerminalAspect>();
    terminalAspect->setUseTerminalHint(bti.usesTerminal);
}

// Factory

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
