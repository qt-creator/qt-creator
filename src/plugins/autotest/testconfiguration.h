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
#include <projectexplorer/runcontrol.h>
#include <utils/environment.h>

#include <QFutureInterface>
#include <QPointer>
#include <QStringList>

QT_BEGIN_NAMESPACE
class QProcess;
QT_END_NAMESPACE

namespace Autotest {
namespace Internal {
class TestRunConfiguration;
} // namespace Internal

class ITestBase;
class ITestFramework;
class TestOutputReader;
class TestResult;
enum class TestRunMode;

using TestResultPtr = QSharedPointer<TestResult>;

class ITestConfiguration
{
public:
    explicit ITestConfiguration(ITestBase *testBase);
    virtual ~ITestConfiguration() = default;

    void setEnvironment(const Utils::Environment &env) { m_runnable.environment = env; }
    Utils::Environment environment() const { return m_runnable.environment; }
    void setWorkingDirectory(const Utils::FilePath &workingDirectory);
    Utils::FilePath workingDirectory() const;
    bool hasExecutable() const;
    Utils::FilePath executableFilePath() const;

    virtual TestOutputReader *outputReader(const QFutureInterface<TestResultPtr> &fi,
                                           QProcess *app) const = 0;
    virtual Utils::Environment filteredEnvironment(const Utils::Environment &original) const;

    ITestBase *testBase() const { return m_testBase; }
    void setProject(ProjectExplorer::Project *project) { m_project = project; }
    void setDisplayName(const QString &displayName) { m_displayName = displayName; }
    QString displayName() const { return m_displayName; }
    void setTestCaseCount(int count) { m_testCaseCount = count; }
    int testCaseCount() const { return m_testCaseCount; }
    ProjectExplorer::Project *project() const { return m_project.data(); }
    ProjectExplorer::Runnable runnable() const { return m_runnable; }

protected:
    ProjectExplorer::Runnable m_runnable;

private:
    ITestBase *m_testBase = nullptr;
    QPointer<ProjectExplorer::Project> m_project;
    QString m_displayName;
    int m_testCaseCount = 0;
};

class TestConfiguration : public ITestConfiguration
{
public:
    explicit TestConfiguration(ITestFramework *framework);
    ~TestConfiguration() override;

    void completeTestInformation(TestRunMode runMode);
    void completeTestInformation(ProjectExplorer::RunConfiguration *rc, TestRunMode runMode);

    void setTestCases(const QStringList &testCases);
    void setExecutableFile(const QString &executableFile);
    void setProjectFile(const Utils::FilePath &projectFile);
    void setBuildDirectory(const Utils::FilePath &buildDirectory);
    void setInternalTarget(const QString &target);
    void setInternalTargets(const QSet<QString> &targets);
    void setOriginalRunConfiguration(ProjectExplorer::RunConfiguration *runConfig);

    ITestFramework *framework() const;
    QStringList testCases() const { return m_testCases; }
    Utils::FilePath buildDirectory() const { return m_buildDir; }
    Utils::FilePath projectFile() const { return m_projectFile; }
    QSet<QString> internalTargets() const { return m_buildTargets; }
    ProjectExplorer::RunConfiguration *originalRunConfiguration() const { return m_origRunConfig; }
    Internal::TestRunConfiguration *runConfiguration() const { return m_runConfig; }
    bool isDeduced() const { return m_deducedConfiguration; }
    QString runConfigDisplayName() const { return m_deducedConfiguration ? m_deducedFrom
                                                                         : displayName(); }

    virtual QStringList argumentsForTestRunner(QStringList *omitted = nullptr) const = 0;

private:
    QStringList m_testCases;
    Utils::FilePath m_projectFile;
    Utils::FilePath m_buildDir;
    QString m_deducedFrom;
    bool m_deducedConfiguration = false;
    Internal::TestRunConfiguration *m_runConfig = nullptr;
    QSet<QString> m_buildTargets;
    ProjectExplorer::RunConfiguration *m_origRunConfig = nullptr;
};

class DebuggableTestConfiguration : public TestConfiguration
{
public:
    explicit DebuggableTestConfiguration(ITestFramework *framework, TestRunMode runMode = TestRunMode::Run)
        : TestConfiguration(framework), m_runMode(runMode) {}

    void setRunMode(TestRunMode mode) { m_runMode = mode; }
    TestRunMode runMode() const { return m_runMode; }
    bool isDebugRunMode() const;
    void setMixedDebugging(bool enable) { m_mixedDebugging = enable; }
    bool mixedDebugging() const { return m_mixedDebugging; }
private:
    TestRunMode m_runMode;
    bool m_mixedDebugging = false;
};

class TestToolConfiguration : public ITestConfiguration
{
public:
    explicit TestToolConfiguration(ITestBase *testBase) : ITestConfiguration(testBase) {}
    Utils::CommandLine commandLine() const { return m_commandLine; }
    void setCommandLine(const Utils::CommandLine &cmdline) { m_commandLine = cmdline; }

private:
    Utils::CommandLine m_commandLine;
};

} // namespace Autotest
