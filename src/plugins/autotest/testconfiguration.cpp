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

#include <cpptools/cppmodelmanager.h>
#include <cpptools/projectinfo.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/environmentaspect.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runnables.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>

using namespace ProjectExplorer;

namespace Autotest {
namespace Internal {

TestConfiguration::TestConfiguration(const QString &testClass, const QStringList &testCases,
                                     int testCaseCount, QObject *parent)
    : QObject(parent),
      m_testClass(testClass),
      m_testCases(testCases),
      m_testCaseCount(testCaseCount),
      m_unnamedOnly(false),
      m_project(0),
      m_guessedConfiguration(false),
      m_type(TestTypeQt)
{
    if (testCases.size() != 0)
        m_testCaseCount = testCases.size();
}

TestConfiguration::~TestConfiguration()
{
    m_testCases.clear();
}

void completeBasicProjectInformation(Project *project, const QString &proFile, QString *displayName,
                             Project **targetProject)
{
    CppTools::CppModelManager *cppMM = CppTools::CppModelManager::instance();
    QList<CppTools::ProjectPart::Ptr> projParts = cppMM->projectInfo(project).projectParts();

    if (displayName->isEmpty()) {
        foreach (const CppTools::ProjectPart::Ptr &part, projParts) {
            if (part->projectFile == proFile) {
                *displayName = part->displayName;
                *targetProject = part->project;
                return;
            }
        }
    } else { // for CMake based projects we've got the displayname already
        foreach (const CppTools::ProjectPart::Ptr &part, projParts) {
            if (part->displayName == *displayName) {
                *targetProject = part->project;
                return;
            }
        }
    }
}

static bool isLocal(RunConfiguration *runConfiguration)
{
    Target *target = runConfiguration ? runConfiguration->target() : 0;
    Kit *kit = target ? target->kit() : 0;
    return DeviceTypeKitInformation::deviceTypeId(kit) == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE;
}

void TestConfiguration::completeTestInformation()
{
    QTC_ASSERT(!m_proFile.isEmpty(), return);

    Project *project = SessionManager::startupProject();
    if (!project)
        return;

    QString targetFile;
    QString targetName;
    QString workDir;
    QString displayName = m_displayName;
    QString buildDir;
    Project *targetProject = 0;
    Utils::Environment env;
    bool hasDesktopTarget = false;
    bool guessedRunConfiguration = false;
    setProject(0);

    completeBasicProjectInformation(project, m_proFile, &displayName, &targetProject);

    Target *target = project->activeTarget();
    if (!target)
        return;

    BuildTargetInfoList appTargets = target->applicationTargets();
    if (m_displayName.isEmpty()) {
        foreach (const BuildTargetInfo &bti, appTargets.list) {
            // some project manager store line/column information as well inside ProjectPart
            if (bti.isValid() && m_proFile.startsWith(bti.projectFilePath.toString())) {
                targetFile = bti.targetFilePath.toString();
                if (Utils::HostOsInfo::isWindowsHost()
                        && !targetFile.toLower().endsWith(QLatin1String(".exe"))) {
                    targetFile = Utils::HostOsInfo::withExecutableSuffix(targetFile);
                }
                targetName = bti.targetName;
                break;
            }
        }
    } else { // CMake based projects have no specific pro file, but target name matches displayname
        foreach (const BuildTargetInfo &bti, appTargets.list) {
            if (bti.isValid() && m_displayName == bti.targetName) {
                // for CMake base projects targetFilePath has executable suffix already
                targetFile = bti.targetFilePath.toString();
                targetName = m_displayName;
                break;
            }
        }
    }

    if (targetProject) {
        if (auto buildConfig = target->activeBuildConfiguration()) {
            const QString buildBase = buildConfig->buildDirectory().toString();
            const QString projBase = targetProject->projectDirectory().toString();
            if (m_proFile.startsWith(projBase))
                buildDir = QFileInfo(buildBase + m_proFile.mid(projBase.length())).absolutePath();
        }
    }

    QList<RunConfiguration *> rcs = target->runConfigurations();
    foreach (RunConfiguration *rc, rcs) {
        Runnable runnable = rc->runnable();
        if (isLocal(rc) && runnable.is<StandardRunnable>()) {
            StandardRunnable stdRunnable = runnable.as<StandardRunnable>();
            if (stdRunnable.executable == targetFile) {
                workDir = Utils::FileUtils::normalizePathName(stdRunnable.workingDirectory);
                env = stdRunnable.environment;
                hasDesktopTarget = true;
                break;
            }
        }
    }

    // if we could not figure out the run configuration
    // try to use the run configuration of the parent project
    if (!hasDesktopTarget && targetProject && !targetFile.isEmpty()) {
        if (auto rc = target->activeRunConfiguration()) {
            Runnable runnable = rc->runnable();
            if (isLocal(rc) && runnable.is<StandardRunnable>()) {
                StandardRunnable stdRunnable = runnable.as<StandardRunnable>();
                workDir = Utils::FileUtils::normalizePathName(stdRunnable.workingDirectory);
                env = stdRunnable.environment;
                hasDesktopTarget = true;
                guessedRunConfiguration = true;
            }
        }
    }

    setDisplayName(displayName);

    if (hasDesktopTarget) {
        setTargetFile(targetFile);
        setTargetName(targetName);
        setWorkingDirectory(workDir);
        setBuildDirectory(buildDir);
        setEnvironment(env);
        setProject(project);
        setGuessedConfiguration(guessedRunConfiguration);
    }
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

void TestConfiguration::setTargetFile(const QString &targetFile)
{
    m_targetFile = targetFile;
}

void TestConfiguration::setTargetName(const QString &targetName)
{
    m_targetName = targetName;
}

void TestConfiguration::setProFile(const QString &proFile)
{
    m_proFile = proFile;
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

void TestConfiguration::setUnnamedOnly(bool unnamedOnly)
{
    m_unnamedOnly = unnamedOnly;
}

void TestConfiguration::setGuessedConfiguration(bool guessed)
{
    m_guessedConfiguration = guessed;
}

void TestConfiguration::setTestType(TestType type)
{
    m_type = type;
}

} // namespace Internal
} // namespace Autotest
