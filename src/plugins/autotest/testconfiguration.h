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

#pragma once

#include "autotestconstants.h"

#include <projectexplorer/project.h>
#include <utils/environment.h>

#include <QFutureInterface>
#include <QObject>
#include <QPointer>
#include <QStringList>

QT_BEGIN_NAMESPACE
class QProcess;
QT_END_NAMESPACE

namespace Autotest {
namespace Internal {

class TestOutputReader;
class TestResult;
class TestRunConfiguration;

using TestResultPtr = QSharedPointer<TestResult>;

class TestConfiguration

{
public:
    explicit TestConfiguration();
    virtual ~TestConfiguration();

    void completeTestInformation(int runMode);

    void setTestCases(const QStringList &testCases);
    void setTestCaseCount(int count);
    void setExecutableFile(const QString &executableFile);
    void setProjectFile(const QString &projectFile);
    void setWorkingDirectory(const QString &workingDirectory);
    void setBuildDirectory(const QString &buildDirectory);
    void setDisplayName(const QString &displayName);
    void setEnvironment(const Utils::Environment &env);
    void setProject(ProjectExplorer::Project *project);
    void setGuessedConfiguration(bool guessed);

    QStringList testCases() const { return m_testCases; }
    int testCaseCount() const { return m_testCaseCount; }
    QString executableFilePath() const;
    QString workingDirectory() const;
    QString buildDirectory() const { return m_buildDir; }
    QString displayName() const { return m_displayName; }
    Utils::Environment environment() const { return m_environment; }
    ProjectExplorer::Project *project() const { return m_project.data(); }
    TestRunConfiguration *runConfiguration() const { return m_runConfig; }
    bool guessedConfiguration() const { return m_guessedConfiguration; }

    virtual TestOutputReader *outputReader(const QFutureInterface<TestResultPtr> &fi,
                                           QProcess *app) const = 0;
    virtual QStringList argumentsForTestRunner() const = 0;

private:
    QStringList m_testCases;
    int m_testCaseCount = 0;
    QString m_projectFile;
    QString m_executableFile;
    QString m_workingDir;
    QString m_buildDir;
    QString m_displayName;
    Utils::Environment m_environment;
    QPointer<ProjectExplorer::Project> m_project;
    bool m_guessedConfiguration = false;
    TestRunConfiguration *m_runConfig = 0;
};

class DebuggableTestConfiguration : public TestConfiguration
{
public:
    enum RunMode
    {
        Run,
        Debug
    };

    explicit DebuggableTestConfiguration(RunMode runMode = Run) : m_runMode(runMode) {}
    ~DebuggableTestConfiguration() {}

    void setRunMode(RunMode mode) { m_runMode = mode; }
    RunMode runMode() const { return m_runMode; }


private:
    RunMode m_runMode;
};

} // namespace Internal
} // namespace Autotest
