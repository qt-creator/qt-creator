// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "desktoprunconfiguration.h"

#include "buildsystem.h"
#include "projectexplorerconstants.h"
#include "projectexplorertr.h"
#include "runconfigurationaspects.h"
#include "target.h"

#include <cmakeprojectmanager/cmakeprojectconstants.h>
#include <docker/dockerconstants.h>
#include <qbsprojectmanager/qbsprojectmanagerconstants.h>
#include <qmakeprojectmanager/qmakeprojectmanagerconstants.h>

using namespace Utils;

namespace ProjectExplorer::Internal {

class DesktopRunConfiguration : public RunConfiguration
{
protected:
    enum Kind { Qmake, Qbs, CMake }; // FIXME: Remove

    DesktopRunConfiguration(Target *target, Id id, Kind kind)
        : RunConfiguration(target, id), m_kind(kind)
    {
        environment.setSupportForBuildEnvironment(target);

        executable.setDeviceSelector(target, ExecutableAspect::RunDevice);

        arguments.setMacroExpander(macroExpander());

        workingDir.setMacroExpander(macroExpander());
        workingDir.setEnvironment(&environment);

        connect(&useLibraryPaths, &UseLibraryPathsAspect::changed,
                &environment, &EnvironmentAspect::environmentChanged);

        if (HostOsInfo::isMacHost()) {
            connect(&useDyldSuffix, &UseLibraryPathsAspect::changed,
                    &environment, &EnvironmentAspect::environmentChanged);
            environment.addModifier([this](Environment &env) {
                if (useDyldSuffix())
                    env.set(QLatin1String("DYLD_IMAGE_SUFFIX"), QLatin1String("_debug"));
            });
        } else {
            useDyldSuffix.setVisible(false);
        }

        runAsRoot.setVisible(HostOsInfo::isAnyUnixHost());

        environment.addModifier([this](Environment &env) {
            BuildTargetInfo bti = buildTargetInfo();
            if (bti.runEnvModifier)
                bti.runEnvModifier(env, useLibraryPaths());
        });

        setUpdater([this] { updateTargetInformation(); });

        connect(target, &Target::buildSystemUpdated, this, &RunConfiguration::update);
    }

private:
    void updateTargetInformation();

    FilePath executableToRun(const BuildTargetInfo &targetInfo) const;

    const Kind m_kind;
    EnvironmentAspect environment{this};
    ExecutableAspect executable{this};
    ArgumentsAspect arguments{this};
    WorkingDirectoryAspect workingDir{this};
    TerminalAspect terminal{this};
    UseDyldSuffixAspect useDyldSuffix{this};
    UseLibraryPathsAspect useLibraryPaths{this};
    RunAsRootAspect runAsRoot{this};
};

void DesktopRunConfiguration::updateTargetInformation()
{
    if (!activeBuildSystem())
        return;

    BuildTargetInfo bti = buildTargetInfo();

    auto terminalAspect = aspect<TerminalAspect>();
    terminalAspect->setUseTerminalHint(bti.targetFilePath.needsDevice() ? false : bti.usesTerminal);
    terminalAspect->setEnabled(!bti.targetFilePath.needsDevice());

    if (m_kind == Qmake) {

        FilePath profile = FilePath::fromString(buildKey());
        if (profile.isEmpty())
            setDefaultDisplayName(Tr::tr("Qt Run Configuration"));
        else
            setDefaultDisplayName(profile.completeBaseName());

        emit aspect<EnvironmentAspect>()->environmentChanged();

        auto wda = aspect<WorkingDirectoryAspect>();
        wda->setDefaultWorkingDirectory(bti.workingDirectory);

        aspect<ExecutableAspect>()->setExecutable(bti.targetFilePath);

    }  else if (m_kind == Qbs) {

        setDefaultDisplayName(bti.displayName);
        const FilePath executable = executableToRun(bti);

        aspect<ExecutableAspect>()->setExecutable(executable);

        if (!executable.isEmpty()) {
            const FilePath defaultWorkingDir = executable.absolutePath();
            if (!defaultWorkingDir.isEmpty())
                aspect<WorkingDirectoryAspect>()->setDefaultWorkingDirectory(defaultWorkingDir);
        }

    } else if (m_kind == CMake) {

        aspect<ExecutableAspect>()->setExecutable(bti.targetFilePath);
        aspect<WorkingDirectoryAspect>()->setDefaultWorkingDirectory(bti.workingDirectory);
        emit aspect<EnvironmentAspect>()->environmentChanged();

    }
}

FilePath DesktopRunConfiguration::executableToRun(const BuildTargetInfo &targetInfo) const
{
    const FilePath appInBuildDir = targetInfo.targetFilePath;
    const DeploymentData deploymentData = target()->deploymentData();
    if (deploymentData.localInstallRoot().isEmpty())
        return appInBuildDir;

    const QString deployedAppFilePath = deploymentData
            .deployableForLocalFile(appInBuildDir).remoteFilePath();
    if (deployedAppFilePath.isEmpty())
        return appInBuildDir;

    const FilePath appInLocalInstallDir = deploymentData.localInstallRoot() / deployedAppFilePath;
    return appInLocalInstallDir.exists() ? appInLocalInstallDir : appInBuildDir;
}

// Factory

class DesktopQmakeRunConfiguration final : public DesktopRunConfiguration
{
public:
    DesktopQmakeRunConfiguration(Target *target, Id id)
        : DesktopRunConfiguration(target, id, Qmake)
    {}
};

class QbsRunConfiguration final : public DesktopRunConfiguration
{
public:
    QbsRunConfiguration(Target *target, Id id)
        : DesktopRunConfiguration(target, id, Qbs)
    {}
};

class CMakeRunConfiguration final : public DesktopRunConfiguration
{
public:
    CMakeRunConfiguration(Target *target, Id id)
        : DesktopRunConfiguration(target, id, CMake)
    {}
};

const char QMAKE_RUNCONFIG_ID[] = "Qt4ProjectManager.Qt4RunConfiguration:";
const char QBS_RUNCONFIG_ID[]   = "Qbs.RunConfiguration:";
const char CMAKE_RUNCONFIG_ID[] = "CMakeProjectManager.CMakeRunConfiguration.";

CMakeRunConfigurationFactory::CMakeRunConfigurationFactory()
{
    registerRunConfiguration<CMakeRunConfiguration>(CMAKE_RUNCONFIG_ID);
    addSupportedProjectType(CMakeProjectManager::Constants::CMAKE_PROJECT_ID);
    addSupportedTargetDeviceType(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);
    addSupportedTargetDeviceType(Docker::Constants::DOCKER_DEVICE_TYPE);
}

QbsRunConfigurationFactory::QbsRunConfigurationFactory()
{
    registerRunConfiguration<QbsRunConfiguration>(QBS_RUNCONFIG_ID);
    addSupportedProjectType(QbsProjectManager::Constants::PROJECT_ID);
    addSupportedTargetDeviceType(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);
    addSupportedTargetDeviceType(Docker::Constants::DOCKER_DEVICE_TYPE);
}

DesktopQmakeRunConfigurationFactory::DesktopQmakeRunConfigurationFactory()
{
    registerRunConfiguration<DesktopQmakeRunConfiguration>(QMAKE_RUNCONFIG_ID);
    addSupportedProjectType(QmakeProjectManager::Constants::QMAKEPROJECT_ID);
    addSupportedTargetDeviceType(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);
    addSupportedTargetDeviceType(Docker::Constants::DOCKER_DEVICE_TYPE);
}

} // ProjectExplorer::Internal
