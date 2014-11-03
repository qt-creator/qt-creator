/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com
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
      m_project(0)
{
    if (testCases.size() != 0) {
        m_testCaseCount = testCases.size();
    }
}

TestConfiguration::~TestConfiguration()
{
    m_testCases.clear();
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

void TestConfiguration::setEnvironment(const Utils::Environment &env)
{
    m_environment = env;
}

void TestConfiguration::setProject(ProjectExplorer::Project *project)
{
    m_project = project;
}

} // namespace Internal
} // namespace Autotest
