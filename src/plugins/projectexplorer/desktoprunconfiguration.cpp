// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "desktoprunconfiguration.h"

#include "buildsystem.h"
#include "deploymentdata.h"
#include "projectexplorerconstants.h"
#include "projectexplorertr.h"
#include "runconfigurationaspects.h"
#include "runcontrol.h"
#include "target.h"

#include <cmakeprojectmanager/cmakeprojectconstants.h>
#include <qbsprojectmanager/qbsprojectmanagerconstants.h>
#include <qmakeprojectmanager/qmakeprojectmanagerconstants.h>

using namespace Utils;
using namespace ProjectExplorer::Constants;

namespace ProjectExplorer::Internal {

class DesktopRunConfiguration : public RunConfiguration
{
public:
    DesktopRunConfiguration(BuildConfiguration *bc, Id id)
        : RunConfiguration(bc, id)
    {
        environment.setSupportForBuildEnvironment(bc);

        executable.setDeviceSelector(kit(), ExecutableAspect::RunDevice);

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
        enableCategoriesFilterAspect.setEnabled(kit()->supportsQtCategoryFilter());

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
    }

private:
    void updateTargetInformation();

    FilePath executableToRun(const BuildTargetInfo &targetInfo) const;

    LauncherAspect launcher{this};
    EnvironmentAspect environment{this};
    ExecutableAspect executable{this};
    ArgumentsAspect arguments{this};
    WorkingDirectoryAspect workingDir{this};
    TerminalAspect terminal{this};
    UseDyldSuffixAspect useDyldSuffix{this};
    UseLibraryPathsAspect useLibraryPaths{this};
    RunAsRootAspect runAsRoot{this};
    EnableCategoriesFilterAspect enableCategoriesFilterAspect{this};
};

void DesktopRunConfiguration::updateTargetInformation()
{
    QTC_ASSERT(buildSystem(), return);

    BuildTargetInfo bti = buildTargetInfo();

    setDefaultDisplayName(bti.displayName);
    auto terminalAspect = aspect<TerminalAspect>();
    terminalAspect->setUseTerminalHint(!bti.targetFilePath.isLocal() ? false : bti.usesTerminal);
    terminalAspect->setEnabled(bti.targetFilePath.isLocal());
    auto launcherAspect = aspect<LauncherAspect>();
    launcherAspect->setVisible(false);

    auto wda = aspect<WorkingDirectoryAspect>();
    if (!bti.workingDirectory.isEmpty())
        wda->setDefaultWorkingDirectory(bti.workingDirectory);

    const FilePath executable = executableToRun(bti);
    aspect<ExecutableAspect>()->setExecutable(executable);

    const QStringList argumentsList = bti.additionalData.toMap()["arguments"].toStringList();
    if (!argumentsList.isEmpty())
        aspect<ArgumentsAspect>()->setArguments(
            ProcessArgs::joinArgs(argumentsList, bti.targetFilePath.osType()));

    if (bti.launchers.size() > 0) {
        launcherAspect->setVisible(true);
        // Use start program by default, if defined (see toBuildTarget() for details)
        launcherAspect->setDefaultLauncher(bti.launchers.last());
        launcherAspect->updateLaunchers(bti.launchers);
    }

    emit aspect<EnvironmentAspect>()->environmentChanged();
}

FilePath DesktopRunConfiguration::executableToRun(const BuildTargetInfo &targetInfo) const
{
    const FilePath appInBuildDir = targetInfo.targetFilePath;
    const DeploymentData deploymentData = buildSystem()->deploymentData();
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

class DesktopRunConfigurationFactory final : public RunConfigurationFactory
{
public:
    DesktopRunConfigurationFactory(const Utils::Id &runConfigId, const Utils::Id &projectTypeId)
    {
        registerRunConfiguration<DesktopRunConfiguration>(runConfigId);
        addSupportedProjectType(projectTypeId);
        setExecutionTypeId(STDPROCESS_EXECUTION_TYPE_ID);
    }
};

void setupDesktopRunConfigurations()
{
    static DesktopRunConfigurationFactory theQmakeRunConfigFactory
        (Constants::QMAKE_RUNCONFIG_ID, QmakeProjectManager::Constants::QMAKEPROJECT_ID);
    static DesktopRunConfigurationFactory theQbsRunConfigFactory
        (Constants::QBS_RUNCONFIG_ID, QbsProjectManager::Constants::PROJECT_ID);
    static DesktopRunConfigurationFactory theCmakeRunConfigFactory
        (Constants::CMAKE_RUNCONFIG_ID, CMakeProjectManager::Constants::CMAKE_PROJECT_ID);
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
