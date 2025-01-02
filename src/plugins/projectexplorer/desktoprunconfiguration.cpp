// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "desktoprunconfiguration.h"

#include "deploymentdata.h"
#include "projectexplorerconstants.h"
#include "projectexplorertr.h"
#include "runconfigurationaspects.h"
#include "runcontrol.h"
#include "target.h"

#include <cmakeprojectmanager/cmakeprojectconstants.h>
#include <docker/dockerconstants.h>
#include <qbsprojectmanager/qbsprojectmanagerconstants.h>
#include <qmakeprojectmanager/qmakeprojectmanagerconstants.h>

using namespace Utils;
using namespace ProjectExplorer::Constants;

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
            if (bti.runEnvModifier) {
                Environment old = env;
                bti.runEnvModifier(env, useLibraryPaths());
                const EnvironmentItems diff = old.diff(env, true);
                for (const EnvironmentItem &i : diff) {
                    switch (i.operation) {
                    case EnvironmentItem::SetEnabled:
                    case EnvironmentItem::Prepend:
                    case EnvironmentItem::Append:
                        env.addItem(std::make_tuple("_QTC_" + i.name, i.value));
                        break;
                    default:
                        break;
                    }
                }
            }
        });

        setUpdater([this] { updateTargetInformation(); });

        connect(target, &Target::buildSystemUpdated, this, &RunConfiguration::update);
    }

private:
    void updateTargetInformation();

    FilePath executableToRun(const BuildTargetInfo &targetInfo) const;

    const Kind m_kind;
    LauncherAspect launcher{this};
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
    terminalAspect->setUseTerminalHint(!bti.targetFilePath.isLocal() ? false : bti.usesTerminal);
    terminalAspect->setEnabled(bti.targetFilePath.isLocal());
    auto launcherAspect = aspect<LauncherAspect>();
    launcherAspect->setVisible(false);

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

        if (bti.launchers.size() > 0) {
            launcherAspect->setVisible(true);
            // Use start program by default, if defined (see toBuildTarget() for details)
            launcherAspect->setDefaultLauncher(bti.launchers.last());
            launcherAspect->updateLaunchers(bti.launchers);
        }
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

// Factories

// FIXME: These three would not be needed if registerRunConfiguration took parameter pack args

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

class CMakeRunConfigurationFactory final : public RunConfigurationFactory
{
public:
    CMakeRunConfigurationFactory()
    {
        registerRunConfiguration<CMakeRunConfiguration>(Constants::CMAKE_RUNCONFIG_ID);
        addSupportedProjectType(CMakeProjectManager::Constants::CMAKE_PROJECT_ID);
        addSupportedTargetDeviceType(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);
        addSupportedTargetDeviceType(Docker::Constants::DOCKER_DEVICE_TYPE);
    }
};

class QbsRunConfigurationFactory final : public RunConfigurationFactory
{
public:
    QbsRunConfigurationFactory()
    {
        registerRunConfiguration<QbsRunConfiguration>(Constants::QBS_RUNCONFIG_ID);
        addSupportedProjectType(QbsProjectManager::Constants::PROJECT_ID);
        addSupportedTargetDeviceType(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);
        addSupportedTargetDeviceType(Docker::Constants::DOCKER_DEVICE_TYPE);
    }
};

class DesktopQmakeRunConfigurationFactory final : public RunConfigurationFactory
{
public:
    DesktopQmakeRunConfigurationFactory()
    {
        registerRunConfiguration<DesktopQmakeRunConfiguration>(Constants::QMAKE_RUNCONFIG_ID);
        addSupportedProjectType(QmakeProjectManager::Constants::QMAKEPROJECT_ID);
        addSupportedTargetDeviceType(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);
        addSupportedTargetDeviceType(Docker::Constants::DOCKER_DEVICE_TYPE);
    }
};

void setupDesktopRunConfigurations()
{
    static DesktopQmakeRunConfigurationFactory theQmakeRunConfigFactory;
    static QbsRunConfigurationFactory theQbsRunConfigFactory;
    static CMakeRunConfigurationFactory theCmakeRunConfigFactory;
}

void setupDesktopRunWorker()
{
    static ProcessRunnerFactory theDesktopRunWorkerFactory({
        Constants::CMAKE_RUNCONFIG_ID,
        Constants::QBS_RUNCONFIG_ID,
        Constants::QMAKE_RUNCONFIG_ID
    });
}

} // ProjectExplorer::Internal
