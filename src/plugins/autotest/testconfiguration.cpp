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
#include "testrunner.h"

#include <cpptools/cppmodelmanager.h>
#include <cpptools/projectinfo.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/environmentaspect.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/runnables.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>

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

void TestConfiguration::completeTestInformation(int runMode)
{
    QTC_ASSERT(!m_projectFile.isEmpty(), return);
    QTC_ASSERT(!m_buildTargets.isEmpty(), return);

    Project *project = SessionManager::startupProject();
    if (!project)
        return;

    Target *target = project->activeTarget();
    if (!target)
        return;

    const QSet<QString> buildSystemTargets = m_buildTargets;
    const BuildTargetInfo targetInfo
            = Utils::findOrDefault(target->applicationTargets().list,
                                   [&buildSystemTargets] (const BuildTargetInfo &bti) {
        return Utils::anyOf(buildSystemTargets, [&bti](const QString &b) {
            const QStringList targWithProjectFile = b.split('|');
            if (targWithProjectFile.size() != 2) // some build targets might miss the project file
                return false;
            return targWithProjectFile.at(0) == bti.targetName
                    && targWithProjectFile.at(1).startsWith(bti.projectFilePath.toString());
        });
    });
    QString executable = targetInfo.targetFilePath.toString(); // empty if BTI default created
    if (Utils::HostOsInfo::isWindowsHost() && !executable.isEmpty()
            && !executable.toLower().endsWith(".exe")) {
        executable = Utils::HostOsInfo::withExecutableSuffix(executable);
    }

    for (RunConfiguration *runConfig : target->runConfigurations()) {
        if (!isLocal(runConfig)) // TODO add device support
            continue;

        const QString bst = runConfig->buildSystemTarget() + '|' + m_projectFile;
        if (buildSystemTargets.contains(bst)) {
            Runnable runnable = runConfig->runnable();
            if (!runnable.is<StandardRunnable>())
                continue;
            StandardRunnable stdRunnable = runnable.as<StandardRunnable>();
            // TODO this might pick up the wrong executable
            m_executableFile = stdRunnable.executable;
            if (Utils::HostOsInfo::isWindowsHost() && !m_executableFile.isEmpty()
                    && !m_executableFile.toLower().endsWith(".exe")) {
                m_executableFile = Utils::HostOsInfo::withExecutableSuffix(m_executableFile);
            }
            m_displayName = runConfig->displayName();
            m_workingDir = Utils::FileUtils::normalizePathName(stdRunnable.workingDirectory);
            m_environment = stdRunnable.environment;
            m_project = project;
            if (runMode == TestRunner::Debug)
                m_runConfig = new TestRunConfiguration(runConfig->target(), this);
            if (m_executableFile == executable) // we can find a better runConfig if no match
                break;
        }
    }
    // RunConfiguration for this target could be explicitly removed or not created at all
    if (m_displayName.isEmpty() && !executable.isEmpty()) {
        // we failed to find a valid runconfiguration - but we've got the executable already
        if (auto rc = target->activeRunConfiguration()) {
            if (isLocal(rc)) { // FIXME for now only Desktop support
                Runnable runnable = rc->runnable();
                if (runnable.is<StandardRunnable>()) {
                    StandardRunnable stdRunnable = runnable.as<StandardRunnable>();
                    m_environment = stdRunnable.environment;
                    m_executableFile = executable;
                    m_project = project;
                    m_guessedConfiguration = true;
                    m_guessedFrom = rc->displayName();
                    if (runMode == TestRunner::Debug)
                        m_runConfig = new TestRunConfiguration(rc->target(), this);
                }
            }
        }
    }

    if (auto buildConfig = target->activeBuildConfiguration()) {
        const QString buildBase = buildConfig->buildDirectory().toString();
        const QString projBase = project->projectDirectory().toString();
        if (m_projectFile.startsWith(projBase))
            m_buildDir = QFileInfo(buildBase + m_projectFile.mid(projBase.length())).absolutePath();
    }

    if (m_displayName.isEmpty()) // happens e.g. when guessing the TestConfiguration or error
        m_displayName = buildSystemTargets.isEmpty() ? "unknown" : (*buildSystemTargets.begin()).split('|').first();
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
    m_executableFile = executableFile;
}

void TestConfiguration::setProjectFile(const QString &projectFile)
{
    m_projectFile = projectFile;
}

void TestConfiguration::setWorkingDirectory(const QString &workingDirectory)
{
    m_workingDir = workingDirectory;
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
    m_environment = env;
}

void TestConfiguration::setProject(Project *project)
{
    m_project = project;
}

void TestConfiguration::setInternalTargets(const QSet<QString> &targets)
{
    m_buildTargets = targets;
}

QString TestConfiguration::executableFilePath() const
{
    if (m_executableFile.isEmpty())
        return QString();

    QFileInfo commandFileInfo(m_executableFile);
    if (commandFileInfo.isExecutable() && commandFileInfo.path() != ".") {
        return commandFileInfo.absoluteFilePath();
    } else if (commandFileInfo.path() == "."){
        QString fullCommandFileName = m_executableFile;
        // TODO: check if we can use searchInPath() from Utils::Environment
        const QStringList &pathList = m_environment.toProcessEnvironment().value("PATH").split(
                    Utils::HostOsInfo::pathListSeparator());

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
    if (!m_workingDir.isEmpty()) {
        const QFileInfo info(m_workingDir);
        if (info.isDir()) // ensure wanted working dir does exist
            return info.absoluteFilePath();
    }

    const QString executable = executableFilePath();
    return executable.isEmpty() ? executable : QFileInfo(executable).absolutePath();
}

} // namespace Internal
} // namespace Autotest
