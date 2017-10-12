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

#include "testconfiguration.h"
#include "testoutputreader.h"
#include "testrunconfiguration.h"

#include <cpptools/cppmodelmanager.h>
#include <cpptools/projectinfo.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/environmentaspect.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/runnables.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>

#include <QLoggingCategory>

static Q_LOGGING_CATEGORY(LOG, "qtc.autotest.testconfiguration")

using namespace ProjectExplorer;

namespace Autotest {
namespace Internal {

TestConfiguration::TestConfiguration()
{
}

TestConfiguration::~TestConfiguration()
{
    m_testCases.clear();
}

static bool isLocal(RunConfiguration *runConfiguration)
{
    Target *target = runConfiguration ? runConfiguration->target() : 0;
    Kit *kit = target ? target->kit() : 0;
    return DeviceTypeKitInformation::deviceTypeId(kit) == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE;
}

static QString ensureExeEnding(const QString& file)
{
    if (!Utils::HostOsInfo::isWindowsHost() || file.isEmpty() || file.toLower().endsWith(".exe"))
        return file;
    return Utils::HostOsInfo::withExecutableSuffix(file);
}

void TestConfiguration::completeTestInformation(ProjectExplorer::RunConfiguration *rc,
                                                TestRunMode runMode)
{
    QTC_ASSERT(rc, return);
    if (hasExecutable()) {
        qCDebug(LOG) << "Executable has been set already - not completing configuration again.";
        return;
    }
    Project *project = SessionManager::startupProject();
    if (!project)
        return;

    Target *target = project->activeTarget();
    if (!target)
        return;

    if (!Utils::findOr(target->runConfigurations(), nullptr,
                       [&rc] (RunConfiguration *config) { return rc == config; })) {
        return;
    }

    Runnable runnable = rc->runnable();
    if (!runnable.is<StandardRunnable>())
        return;
    m_runnable = runnable.as<StandardRunnable>();
    m_displayName = rc->displayName();
    m_project = rc->project();

    const QString buildSystemTarget = rc->buildSystemTarget();
    BuildTargetInfo targetInfo
            = Utils::findOrDefault(target->applicationTargets().list,
                                   [&buildSystemTarget] (const BuildTargetInfo &bti) {
        return bti.targetName == buildSystemTarget;
    });
    if (!targetInfo.targetFilePath.isEmpty())
        m_runnable.executable = ensureExeEnding(targetInfo.targetFilePath.toString());

    QString buildBase;
    if (auto buildConfig = target->activeBuildConfiguration()) {
        buildBase = buildConfig->buildDirectory().toString();
        const QString projBase = m_project->projectDirectory().toString();
        if (m_projectFile.startsWith(projBase))
            m_buildDir = QFileInfo(buildBase + m_projectFile.mid(projBase.length())).absolutePath();
    }
    if (runMode == TestRunMode::Debug || runMode == TestRunMode::DebugWithoutDeploy)
        m_runConfig = new TestRunConfiguration(rc->target(), this);
}

void TestConfiguration::completeTestInformation(TestRunMode runMode)
{
    QTC_ASSERT(!m_projectFile.isEmpty(), return);
    QTC_ASSERT(!m_buildTargets.isEmpty(), return);

    if (m_origRunConfig) {
        qCDebug(LOG) << "Using run configuration specified by user or found by first call";
        completeTestInformation(m_origRunConfig, runMode);
        if (hasExecutable()) {
            qCDebug(LOG) << "Completed.\nRunnable:" << m_runnable.executable
                         << "\nArgs:" << m_runnable.commandLineArguments
                         << "\nWorking directory:" << m_runnable.workingDirectory;
            return;
        }
        qCDebug(LOG) << "Failed to complete - using 'normal' way.";
    }
    Project *project = SessionManager::startupProject();
    if (!project)
        return;

    Target *target = project->activeTarget();
    if (!target)
        return;
    qCDebug(LOG) << "ActiveTargetName\n    " << target->displayName();
    if (const auto kit = target->kit())
        qCDebug(LOG) << "SupportedPlatforms\n    " << kit->supportedPlatforms();

    const QSet<QString> buildSystemTargets = m_buildTargets;
    qCDebug(LOG) << "BuildSystemTargets\n    " << buildSystemTargets;
    BuildTargetInfo targetInfo
            = Utils::findOrDefault(target->applicationTargets().list,
                                   [&buildSystemTargets] (const BuildTargetInfo &bti) {
        return Utils::anyOf(buildSystemTargets, [&bti](const QString &b) {
            const QStringList targWithProjectFile = b.split('|');
            if (targWithProjectFile.size() != 2) // some build targets might miss the project file
                return false;
            return !bti.targetFilePath.isEmpty() && targWithProjectFile.at(0) == bti.targetName
                    && targWithProjectFile.at(1).startsWith(bti.projectFilePath.toString());
        });
    });
    // we might end up with an empty targetFilePath - e.g. when having a library we just link to
    // there would be no BuildTargetInfo that could match
    if (targetInfo.targetFilePath.isEmpty()) {
        qCDebug(LOG) << "BuildTargetInfos";
        const QList<BuildTargetInfo> buildTargets = target->applicationTargets().list;
        // if there is only one build target just use it (but be honest that we're guessing)
        if (buildTargets.size() == 1) {
            targetInfo = buildTargets.first();
            m_guessedConfiguration = true;
            m_guessedFrom = targetInfo.targetName;
        }
    }

    const QString localExecutable = ensureExeEnding(targetInfo.targetFilePath.toString());
    if (localExecutable.isEmpty())
        return;

    QString buildBase;
    if (auto buildConfig = target->activeBuildConfiguration()) {
        buildBase = buildConfig->buildDirectory().toString();
        const QString projBase = project->projectDirectory().toString();
        if (m_projectFile.startsWith(projBase))
            m_buildDir = QFileInfo(buildBase + m_projectFile.mid(projBase.length())).absolutePath();
    }

    // deployment information should get taken into account, but it pretty much seems as if
    // each build system uses it differently
    const DeploymentData &deployData = target->deploymentData();
    const DeployableFile deploy = deployData.deployableForLocalFile(localExecutable);
    // we might have a deployable executable
    const QString deployedExecutable = ensureExeEnding((deploy.isValid() && deploy.isExecutable())
            ? QDir::cleanPath(deploy.remoteFilePath()) : localExecutable);

    qCDebug(LOG) << " LocalExecutable" << localExecutable;
    qCDebug(LOG) << " DeployedExecutable" << deployedExecutable;
    qCDebug(LOG) << "Iterating run configurations";
    for (RunConfiguration *runConfig : target->runConfigurations()) {
        qCDebug(LOG) << "RunConfiguration" << runConfig->id();
        if (!isLocal(runConfig)) { // TODO add device support
            qCDebug(LOG) << " Skipped as not being local";
            continue;
        }

        Runnable runnable = runConfig->runnable();
        if (!runnable.is<StandardRunnable>()) {
            qCDebug(LOG) << " Skipped as not being a StandardRunnable";
            continue;
        }
        StandardRunnable stdRunnable = runnable.as<StandardRunnable>();
        // not the best approach - but depending on the build system and whether the executables
        // are going to get installed or not we have to soften the condition...
        const QString &currentExecutable = ensureExeEnding(stdRunnable.executable);
        const QString currentBST = runConfig->buildSystemTarget() + '|';
        qCDebug(LOG) << " CurrentExecutable" << currentExecutable;
        qCDebug(LOG) << " BST of RunConfig" << currentBST;
        const bool isQbs = runConfig->id().toString().startsWith("Qbs.RunConfiguration:"); // BAD!
        if ((localExecutable == currentExecutable)
                || (deployedExecutable == currentExecutable)
                || (isQbs && Utils::anyOf(buildSystemTargets, [currentBST] (const QString &b) {
                                              return b.startsWith(currentBST);
                                          }))) {
            qCDebug(LOG) << "  Using this RunConfig.";
            m_origRunConfig = runConfig;
            m_runnable = stdRunnable;
            m_runnable.executable = currentExecutable;
            m_displayName = runConfig->displayName();
            m_project = project;
            if (runMode == TestRunMode::Debug || runMode == TestRunMode::DebugWithoutDeploy)
                m_runConfig = new TestRunConfiguration(runConfig->target(), this);
            break;
        }
    }

    // RunConfiguration for this target could be explicitly removed or not created at all
    // or we might have end up using the (wrong) path of a locally installed executable
    // for this case try the original executable path of the BuildTargetInfo (the executable
    // before installation) to have at least something to execute
    if (!hasExecutable() && !localExecutable.isEmpty())
        m_runnable.executable = localExecutable;
    if (m_displayName.isEmpty() && hasExecutable()) {
        qCDebug(LOG) << "   Fallback";
        // we failed to find a valid runconfiguration - but we've got the executable already
        if (auto rc = target->activeRunConfiguration()) {
            if (isLocal(rc)) { // FIXME for now only Desktop support
                Runnable runnable = rc->runnable();
                if (runnable.is<StandardRunnable>()) {
                    StandardRunnable stdRunnable = runnable.as<StandardRunnable>();
                    m_runnable.environment = stdRunnable.environment;
                    m_project = project;
                    m_guessedConfiguration = true;
                    m_guessedFrom = rc->displayName();
                    if (runMode == TestRunMode::Debug)
                        m_runConfig = new TestRunConfiguration(rc->target(), this);
                }
            } else {
                qCDebug(LOG) << "not using the fallback as the current active run configuration "
                                "appears to be non-Desktop";
            }
        }
    }

    if (m_displayName.isEmpty()) // happens e.g. when guessing the TestConfiguration or error
        m_displayName = (*buildSystemTargets.begin()).split('|').first();
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
    m_testCaseCount = m_testCases.size();
}

void TestConfiguration::setTestCaseCount(int count)
{
    m_testCaseCount = count;
}

void TestConfiguration::setExecutableFile(const QString &executableFile)
{
    m_runnable.executable = executableFile;
}

void TestConfiguration::setProjectFile(const QString &projectFile)
{
    m_projectFile = projectFile;
}

void TestConfiguration::setWorkingDirectory(const QString &workingDirectory)
{
    m_runnable.workingDirectory = workingDirectory;
}

void TestConfiguration::setBuildDirectory(const QString &buildDirectory)
{
    m_buildDir = buildDirectory;
}

void TestConfiguration::setDisplayName(const QString &displayName)
{
    m_displayName = displayName;
}

void TestConfiguration::setEnvironment(const Utils::Environment &env)
{
    m_runnable.environment = env;
}

void TestConfiguration::setProject(Project *project)
{
    m_project = project;
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

QString TestConfiguration::executableFilePath() const
{
    if (!hasExecutable())
        return QString();

    QFileInfo commandFileInfo(m_runnable.executable);
    if (commandFileInfo.isExecutable() && commandFileInfo.path() != ".") {
        return commandFileInfo.absoluteFilePath();
    } else if (commandFileInfo.path() == "."){
        QString fullCommandFileName = m_runnable.executable;
        // TODO: check if we can use searchInPath() from Utils::Environment
        const QStringList &pathList = m_runnable.environment.toProcessEnvironment().value("PATH")
                .split(Utils::HostOsInfo::pathListSeparator());

        foreach (const QString &path, pathList) {
            QString filePath(path + QDir::separator() + fullCommandFileName);
            if (QFileInfo(filePath).isExecutable())
                return commandFileInfo.absoluteFilePath();
        }
    }
    return QString();
}

QString TestConfiguration::workingDirectory() const
{
    if (!m_runnable.workingDirectory.isEmpty()) {
        const QFileInfo info(m_runnable.workingDirectory);
        if (info.isDir()) // ensure wanted working dir does exist
            return info.absoluteFilePath();
    }

    const QString executable = executableFilePath();
    return executable.isEmpty() ? executable : QFileInfo(executable).absolutePath();
}

bool DebuggableTestConfiguration::isDebugRunMode() const
{
    return m_runMode == TestRunMode::Debug || m_runMode == TestRunMode::DebugWithoutDeploy;
}

bool TestConfiguration::hasExecutable() const
{
    return !m_runnable.executable.isEmpty();
}

} // namespace Internal
} // namespace Autotest
