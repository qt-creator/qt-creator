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

#include <projectexplorer/project.h>

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
    if (testCases.size() != 0) {
        m_testCaseCount = testCases.size();
    }
}

TestConfiguration::~TestConfiguration()
{
    m_testCases.clear();
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

void TestConfiguration::setDisplayName(const QString &displayName)
{
    m_displayName = displayName;
}

void TestConfiguration::setEnvironment(const Utils::Environment &env)
{
    m_environment = env;
}

void TestConfiguration::setProject(ProjectExplorer::Project *project)
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
