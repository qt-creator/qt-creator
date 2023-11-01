// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "testconfiguration.h"

#include "itestframework.h"
#include "testrunconfiguration.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>

#include <utils/algorithm.h>

#include <QLoggingCategory>

static Q_LOGGING_CATEGORY(LOG, "qtc.autotest.testconfiguration", QtWarningMsg)

using namespace ProjectExplorer;
using namespace Utils;

namespace Autotest {

ITestConfiguration::ITestConfiguration(ITestBase *testBase)
    : m_testBase(testBase)
{
}

void ITestConfiguration::setWorkingDirectory(const FilePath &workingDirectory)
{
    m_runnable.workingDirectory = workingDirectory;
}

FilePath ITestConfiguration::workingDirectory() const
{
    if (!m_runnable.workingDirectory.isEmpty()) {
        if (m_runnable.workingDirectory.isDir()) // ensure wanted working dir does exist
            return m_runnable.workingDirectory.absoluteFilePath();
    }

    const FilePath executable = executableFilePath();
    return executable.isEmpty() ? executable : executable.absolutePath();
}

bool ITestConfiguration::hasExecutable() const
{
    return !m_runnable.command.isEmpty();
}

FilePath ITestConfiguration::executableFilePath() const
{
    if (!hasExecutable())
        return {};

    const Environment env = m_runnable.environment.appliedToEnvironment(
        m_runnable.command.executable().deviceEnvironment());

    return m_runnable.command.executable().searchInDirectories(env.path());
}

Environment ITestConfiguration::filteredEnvironment(const Environment &original) const
{
    return original;
}

TestConfiguration::TestConfiguration(ITestFramework *framework)
    : ITestConfiguration(framework)
{
}

TestConfiguration::~TestConfiguration()
{
    m_testCases.clear();
}

static bool isLocal(Target *target)
{
    Kit *kit = target ? target->kit() : nullptr;
    return DeviceTypeKitAspect::deviceTypeId(kit) == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE;
}

static FilePath ensureExeEnding(const FilePath &file)
{
    if (!HostOsInfo::isWindowsHost() || file.isEmpty() || file.toString().toLower().endsWith(".exe"))
        return file;
    return file.withExecutableSuffix();
}

void TestConfiguration::completeTestInformation(RunConfiguration *rc,
                                                TestRunMode runMode)
{
    QTC_ASSERT(rc, return);
    QTC_ASSERT(project(), return);

    if (hasExecutable()) {
        qCDebug(LOG) << "Executable has been set already - not completing configuration again.";
        return;
    }
    Project *startupProject = ProjectManager::startupProject();
    if (!startupProject || startupProject != project())
        return;

    Target *target = startupProject->activeTarget();
    if (!target)
        return;

    if (!target->runConfigurations().contains(rc))
        return;

    m_runnable = rc->runnable();
    setDisplayName(rc->displayName());

    BuildTargetInfo targetInfo = rc->buildTargetInfo();
    if (!targetInfo.targetFilePath.isEmpty())
        m_runnable.command.setExecutable(ensureExeEnding(targetInfo.targetFilePath));

    FilePath buildBase;
    if (auto buildConfig = target->activeBuildConfiguration()) {
        buildBase = buildConfig->buildDirectory();
        const QString projBase = startupProject->projectDirectory().toString();
        if (m_projectFile.startsWith(projBase))
            m_buildDir = (buildBase / m_projectFile.toString().mid(projBase.length())).absolutePath();
    }
    if (runMode == TestRunMode::Debug || runMode == TestRunMode::DebugWithoutDeploy)
        m_runConfig = new Internal::TestRunConfiguration(rc->target(), this);
}

void TestConfiguration::completeTestInformation(TestRunMode runMode)
{
    QTC_ASSERT(!m_projectFile.isEmpty(), return);
    QTC_ASSERT(!m_buildTargets.isEmpty(), return);
    QTC_ASSERT(project(), return);

    if (m_origRunConfig) {
        qCDebug(LOG) << "Using run configuration specified by user or found by first call";
        completeTestInformation(m_origRunConfig, runMode);
        if (hasExecutable()) {
            qCDebug(LOG) << "Completed.\nCommand:" << m_runnable.command.toUserOutput()
                         << "\nWorking directory:" << m_runnable.workingDirectory;
            return;
        }
        qCDebug(LOG) << "Failed to complete - using 'normal' way.";
    }
    Project *startupProject = ProjectManager::startupProject();
    if (!startupProject || startupProject != project()) {
        setProject(nullptr);
        return;
    }

    Target *target = startupProject->activeTarget();
    if (!target)
        return;
    qCDebug(LOG) << "ActiveTargetName\n    " << target->displayName();
    if (const auto kit = target->kit())
        qCDebug(LOG) << "SupportedPlatforms\n    " << kit->supportedPlatforms();

    const QSet<QString> buildSystemTargets = m_buildTargets;
    qCDebug(LOG) << "BuildSystemTargets\n    " << buildSystemTargets;
    const QList<BuildTargetInfo> buildTargets
            = Utils::filtered(target->buildSystem()->applicationTargets(),
                              [&buildSystemTargets](const BuildTargetInfo &bti) {
        return buildSystemTargets.contains(bti.buildKey);
    });
    if (buildTargets.size() > 1 )  // there are multiple executables with the same build target
        return;                    // let the user decide which one to run

    const BuildTargetInfo targetInfo = buildTargets.size() ? buildTargets.first()
                                                           : BuildTargetInfo();

    // we might end up with an empty targetFilePath - e.g. when having a library we just link to
    // there would be no BuildTargetInfo that could match
    if (targetInfo.targetFilePath.isEmpty()) {
        qCDebug(LOG) << "BuildTargetInfos";
        // if there is only one build target just use it (but be honest that we're deducing)
        m_deducedConfiguration = true;
        m_deducedFrom = targetInfo.buildKey;
    }

    const FilePath localExecutable = ensureExeEnding(targetInfo.targetFilePath);
    if (localExecutable.isEmpty())
        return;

    FilePath buildBase;
    if (auto buildConfig = target->activeBuildConfiguration()) {
        buildBase = buildConfig->buildDirectory();
        const QString projBase = startupProject->projectDirectory().toString();
        if (m_projectFile.startsWith(projBase))
            m_buildDir = (buildBase / m_projectFile.toString().mid(projBase.length())).absolutePath();
    }

    // deployment information should get taken into account, but it pretty much seems as if
    // each build system uses it differently
    const DeploymentData &deployData = target->deploymentData();
    const DeployableFile deploy = deployData.deployableForLocalFile(localExecutable);
    // we might have a deployable executable
    const FilePath deployedExecutable = ensureExeEnding((deploy.isValid() && deploy.isExecutable())
            ? FilePath::fromString(QDir::cleanPath(deploy.remoteFilePath())) : localExecutable);

    qCDebug(LOG) << " LocalExecutable" << localExecutable;
    qCDebug(LOG) << " DeployedExecutable" << deployedExecutable;
    qCDebug(LOG) << "Iterating run configurations - prefer active over others";
    QList<RunConfiguration *> runConfigurations = target->runConfigurations();
    runConfigurations.removeOne(target->activeRunConfiguration());
    runConfigurations.prepend(target->activeRunConfiguration());
    for (RunConfiguration *runConfig : std::as_const(runConfigurations)) {
        qCDebug(LOG) << "RunConfiguration" << runConfig->id();
        if (!isLocal(target)) { // TODO add device support
            qCDebug(LOG) << " Skipped as not being local";
            continue;
        }

        const ProcessRunData runnable = runConfig->runnable();
        // not the best approach - but depending on the build system and whether the executables
        // are going to get installed or not we have to soften the condition...
        const FilePath currentExecutable = ensureExeEnding(runnable.command.executable());
        const QString currentBST = runConfig->buildKey();
        qCDebug(LOG) << " CurrentExecutable" << currentExecutable;
        qCDebug(LOG) << " BST of RunConfig" << currentBST;
        if ((localExecutable == currentExecutable)
                || (deployedExecutable == currentExecutable)
                || (buildSystemTargets.contains(currentBST))) {
            qCDebug(LOG) << "  Using this RunConfig.";
            m_origRunConfig = runConfig;
            m_runnable = runnable;
            m_runnable.command.setExecutable(currentExecutable);
            setDisplayName(runConfig->displayName());
            if (runMode == TestRunMode::Debug || runMode == TestRunMode::DebugWithoutDeploy)
                m_runConfig = new Internal::TestRunConfiguration(target, this);
            break;
        }
    }

    // RunConfiguration for this target could be explicitly removed or not created at all
    // or we might have end up using the (wrong) path of a locally installed executable
    // for this case try the original executable path of the BuildTargetInfo (the executable
    // before installation) to have at least something to execute
    if (!hasExecutable() && !localExecutable.isEmpty())
        m_runnable.command.setExecutable(localExecutable);
    if (displayName().isEmpty() && hasExecutable()) {
        qCDebug(LOG) << "   Fallback";
        // we failed to find a valid runconfiguration - but we've got the executable already
        if (auto rc = target->activeRunConfiguration()) {
            if (isLocal(target)) { // FIXME for now only Desktop support
                const ProcessRunData runnable = rc->runnable();
                m_runnable.environment = runnable.environment;
                m_deducedConfiguration = true;
                m_deducedFrom = rc->displayName();
                if (runMode == TestRunMode::Debug)
                    m_runConfig = new Internal::TestRunConfiguration(rc->target(), this);
            } else {
                qCDebug(LOG) << "not using the fallback as the current active run configuration "
                                "appears to be non-Desktop";
            }
        }
    }

    if (displayName().isEmpty()) // happens e.g. when deducing the TestConfiguration or error
        setDisplayName(*buildSystemTargets.begin());
}

/**
 * @brief sets the test cases for this test configuration.
 *
 * Watch out for special handling of test configurations, because this method also
 * updates the test case count to the current size of \a testCases.
 *
 * @param testCases list of names of the test functions / test cases
 */
void TestConfiguration::setTestCases(const QStringList &testCases)
{
    m_testCases.clear();
    m_testCases << testCases;
    setTestCaseCount(m_testCases.size());
}

void TestConfiguration::setProjectFile(const FilePath &projectFile)
{
    m_projectFile = projectFile;
}

void TestConfiguration::setBuildDirectory(const FilePath &buildDirectory)
{
    m_buildDir = buildDirectory;
}

void TestConfiguration::setInternalTarget(const QString &target)
{
    m_buildTargets.clear();
    m_buildTargets.insert(target);
}

void TestConfiguration::setInternalTargets(const QSet<QString> &targets)
{
    m_buildTargets = targets;
}

void TestConfiguration::setOriginalRunConfiguration(RunConfiguration *runConfig)
{
    m_origRunConfig = runConfig;
}

bool DebuggableTestConfiguration::isDebugRunMode() const
{
    return m_runMode == TestRunMode::Debug || m_runMode == TestRunMode::DebugWithoutDeploy;
}

ITestFramework *TestConfiguration::framework() const
{
    return static_cast<ITestFramework *>(testBase());
}

} // namespace Autotest
