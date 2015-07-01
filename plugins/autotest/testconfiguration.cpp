/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at
** http://www.qt.io/contact-us
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
****************************************************************************/

#include "testconfiguration.h"

#include <cpptools/cppmodelmanager.h>

#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/environmentaspect.h>
#include <projectexplorer/localapplicationrunconfiguration.h>
#include <projectexplorer/project.h>
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
      m_guessedConfiguration(false)
{
    if (testCases.size() != 0)
        m_testCaseCount = testCases.size();
}

TestConfiguration::~TestConfiguration()
{
    m_testCases.clear();
}

void basicProjectInformation(Project *project, const QString &mainFilePath, QString *proFile,
                             QString *displayName, Project **targetProject)
{
    CppTools::CppModelManager *cppMM = CppTools::CppModelManager::instance();
    QList<CppTools::ProjectPart::Ptr> projParts = cppMM->projectInfo(project).projectParts();

    foreach (const CppTools::ProjectPart::Ptr &part, projParts) {
        foreach (const CppTools::ProjectFile currentFile, part->files) {
            if (currentFile.path == mainFilePath) {
                *proFile = part->projectFile;
                *displayName = part->displayName;
                *targetProject = part->project;
                return;
            }
        }
    }
}

void extractEnvironmentInformation(LocalApplicationRunConfiguration *localRunConfiguration,
                                   QString *workDir, Utils::Environment *env)
{
    *workDir = Utils::FileUtils::normalizePathName(localRunConfiguration->workingDirectory());
    if (auto environmentAspect = localRunConfiguration->extraAspect<EnvironmentAspect>())
        *env = environmentAspect->environment();
}

void TestConfiguration::completeTestInformation()
{
    QTC_ASSERT(!m_mainFilePath.isEmpty(), return);

    typedef LocalApplicationRunConfiguration LocalRunConfig;

    Project *project = SessionManager::startupProject();
    if (!project)
        return;

    QString targetFile;
    QString targetName;
    QString workDir;
    QString proFile;
    QString displayName;
    Project *targetProject = 0;
    Utils::Environment env;
    bool hasDesktopTarget = false;
    bool guessedRunConfiguration = false;
    setProject(0);

    basicProjectInformation(project, m_mainFilePath, &proFile, &displayName, &targetProject);

    Target *target = project->activeTarget();
    if (!target)
        return;

    BuildTargetInfoList appTargets = target->applicationTargets();
    foreach (const BuildTargetInfo &bti, appTargets.list) {
        // some project manager store line/column information as well inside ProjectPart
        if (bti.isValid() && proFile.startsWith(bti.projectFilePath.toString())) {
            targetFile = Utils::HostOsInfo::withExecutableSuffix(bti.targetFilePath.toString());
            targetName = bti.targetName;
            break;
        }
    }

    QList<RunConfiguration *> rcs = target->runConfigurations();
    foreach (RunConfiguration *rc, rcs) {
        auto config = qobject_cast<LocalRunConfig *>(rc);
        if (config && config->executable() == targetFile) {
            extractEnvironmentInformation(config, &workDir, &env);
            hasDesktopTarget = true;
            break;
        }
    }

    // if we could not figure out the run configuration
    // try to use the run configuration of the parent project
    if (!hasDesktopTarget && targetProject && !targetFile.isEmpty()) {
        if (auto config = qobject_cast<LocalRunConfig *>(target->activeRunConfiguration())) {
            extractEnvironmentInformation(config, &workDir, &env);
            hasDesktopTarget = true;
            guessedRunConfiguration = true;
        }
    }

    setProFile(proFile);
    setDisplayName(displayName);

    if (hasDesktopTarget) {
        setTargetFile(targetFile);
        setTargetName(targetName);
        setWorkingDirectory(workDir);
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

void TestConfiguration::setMainFilePath(const QString &mainFile)
{
    m_mainFilePath = mainFile;
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

} // namespace Internal
} // namespace Autotest
