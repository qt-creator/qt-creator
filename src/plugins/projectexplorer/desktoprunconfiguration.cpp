// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "desktoprunconfiguration.h"

#include "buildsystem.h"
#include "deploymentdata.h"
#include "devicesupport/devicekitaspects.h"
#include "devicesupport/idevice.h"
#include "projectexplorerconstants.h"
#include "runconfigurationaspects.h"
#include "runcontrol.h"
#include "target.h"

#include <cmakeprojectmanager/cmakeprojectconstants.h>
#include <qbsprojectmanager/qbsprojectmanagerconstants.h>
#include <qmakeprojectmanager/qmakeprojectmanagerconstants.h>
#include <gnprojectmanager/gnpluginconstants.h>

#ifdef WITH_TESTS
#include <QTest>
#endif

using namespace Utils;
using namespace ProjectExplorer::Constants;

namespace ProjectExplorer::Internal {

/*!
    Shows the arguments \a fromTarget of the build target in \a arguments,
    where \a fromLastTarget is what the target of the previous update put
    there.

    A target that names no arguments clears the field only if it still holds
    what the previous target put there; arguments the user typed are kept.
    The two are told apart by the text as it was typed, so that a macro
    expanding to the previous target's arguments is not taken for them.
*/
static void applyTargetArguments(ArgumentsAspect *arguments,
                                 const QString &fromTarget,
                                 const QString &fromLastTarget)
{
    if (!fromTarget.isEmpty())
        arguments->setArguments(fromTarget);
    else if (arguments->unexpandedArguments() == fromLastTarget)
        arguments->setArguments({});
}

class DesktopRunConfiguration : public RunConfiguration
{
public:
    DesktopRunConfiguration(BuildConfiguration *bc, Id id)
        : RunConfiguration(bc, id)
    {
        // Run configurations kept in sync match their aspects by id, and
        // aspects without one all match each other.
        argumentsFromTarget.setId("ArgumentsFromTarget");
        argumentsFromTarget.setSettingsKey("RunConfiguration.ArgumentsFromTarget");

        environment.setSupportForBuildEnvironment(bc);

        executable.setDeviceSelector(kit(), ExecutableAspect::RunDevice);

        workingDir.setEnvironment(&environment);

        connect(&useLibraryPaths, &UseLibraryPathsAspect::changed,
                &environment, &EnvironmentAspect::environmentChanged);

        useDyldSuffix.applyTo(environment);

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
                        env.addItem(std::make_tuple("_QTC_" + i.name, i.value, true));
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

    // What the build target put into the arguments field on the last update,
    // so that a later target naming none can clear it again. Saved with the
    // run configuration, like the field itself.
    TypedAspect<QString> argumentsFromTarget{this};
    LauncherAspect launcher{this};
    EnvironmentAspect environment{this};
    ExecutableAspect executable{this};
    ArgumentsAspect arguments{this};
    WorkingDirectoryAspect workingDir{this};
    RunAsAspect runAs{this};
    TerminalAspect terminal{this};
    UseDyldSuffixAspect useDyldSuffix{this};
    UseLibraryPathsAspect useLibraryPaths{this};
    EnableCategoriesFilterAspect enableCategoriesFilterAspect{this};
};

void DesktopRunConfiguration::updateTargetInformation()
{
    QTC_ASSERT(buildSystem(), return);

    BuildTargetInfo bti = buildTargetInfo();

    setDefaultDisplayName(bti.displayName);
    auto terminalAspect = aspect<TerminalAspect>();
    terminalAspect->setUseTerminalHint(bti.usesTerminal && bti.targetFilePath.isLocal());
    terminalAspect->setEnabled(bti.targetFilePath.isLocal());
    auto launcherAspect = aspect<LauncherAspect>();
    launcherAspect->setVisible(false);

    auto wda = aspect<WorkingDirectoryAspect>();
    if (!bti.workingDirectory.isEmpty())
        wda->setDefaultWorkingDirectory(bti.workingDirectory);

    const FilePath executable = executableToRun(bti);
    aspect<ExecutableAspect>()->setExecutable(executable);

    const QStringList argumentsList = bti.additionalData.toMap()["arguments"].toStringList();
    const QString targetArguments
        = ProcessArgs::joinArgs(argumentsList, bti.targetFilePath.osType());
    applyTargetArguments(aspect<ArgumentsAspect>(), targetArguments, argumentsFromTarget());
    argumentsFromTarget.setValue(targetArguments);

    if (bti.launchers.isEmpty()) {
        // Otherwise the launcher of the previous target would run this one.
        launcherAspect->updateLaunchers({});
    } else {
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

    const IDeviceConstPtr runDevice = RunDeviceKitAspect::device(kit());
    if (runDevice && !runDevice->rootPath().isLocal())
        return runDevice->filePath(deployedAppFilePath);

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
    static DesktopRunConfigurationFactory theGnRunConfigFactory(Constants::GN_RUNCONFIG_ID,
                                                                GNProjectManager::Constants::
                                                                    GN_PROJECT_ID);
}

#ifdef WITH_TESTS
class DesktopRunConfigurationTest final : public QObject
{
    Q_OBJECT

private slots:
    void testApplyTargetArguments()
    {
        const QString ofTheTarget = "--build . --target hello";
        const QString ofTheUser = "--verbose input.txt";

        ArgumentsAspect arguments;

        // The arguments of the target are shown.
        applyTargetArguments(&arguments, ofTheTarget, {});
        QCOMPARE(arguments.unexpandedArguments(), ofTheTarget);

        // A target naming none clears what the previous one put there.
        applyTargetArguments(&arguments, {}, ofTheTarget);
        QCOMPARE(arguments.unexpandedArguments(), QString());

        // Arguments the user typed are kept, whatever the previous target
        // named.
        arguments.setArguments(ofTheUser);
        applyTargetArguments(&arguments, {}, ofTheTarget);
        QCOMPARE(arguments.unexpandedArguments(), ofTheUser);
        applyTargetArguments(&arguments, {}, {});
        QCOMPARE(arguments.unexpandedArguments(), ofTheUser);
    }

    // Arguments the user typed may hold macros, which the field keeps as
    // typed. Writing their expansion back would freeze them as of that day.
    void testKeepsTheMacrosOfTheUser()
    {
        const QString ofTheUser = "%{Env:HOME}/input.txt";

        ArgumentsAspect arguments;
        arguments.setArguments(ofTheUser);
        // The test says nothing unless the macro expands to something else.
        QVERIFY(arguments.arguments() != arguments.unexpandedArguments());

        applyTargetArguments(&arguments, {}, {});
        QCOMPARE(arguments.unexpandedArguments(), ofTheUser);

        // Also when a target put something there before, and even when the
        // expansion is exactly that: the comparison is on the text as typed.
        applyTargetArguments(&arguments, {}, "--build . --target hello");
        QCOMPARE(arguments.unexpandedArguments(), ofTheUser);
        applyTargetArguments(&arguments, {}, arguments.arguments());
        QCOMPARE(arguments.unexpandedArguments(), ofTheUser);
    }
};

QObject *createDesktopRunConfigurationTest()
{
    return new DesktopRunConfigurationTest;
}
#endif

void setupDesktopRunWorker()
{
    static ProcessRunnerFactory theDesktopRunWorkerFactory({
        Constants::CMAKE_RUNCONFIG_ID,
        Constants::QBS_RUNCONFIG_ID,
        Constants::QMAKE_RUNCONFIG_ID,
        Constants::GN_RUNCONFIG_ID
    });
}

} // ProjectExplorer::Internal

#ifdef WITH_TESTS
#include <desktoprunconfiguration.moc>
#endif
