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

#include "testrunner.h"

#include "autotestconstants.h"
#include "autotestplugin.h"
#include "testresultspane.h"
#include "testsettings.h"
#include "testoutputreader.h"

#include <coreplugin/progressmanager/futureprogress.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <projectexplorer/buildmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorersettings.h>

#include <utils/runextensions.h>

#include <QFuture>
#include <QFutureInterface>
#include <QTime>

namespace Autotest {
namespace Internal {

static TestRunner *m_instance = 0;

static QString executableFilePath(const QString &command, const QProcessEnvironment &environment)
{
    if (command.isEmpty())
        return QString();

    QFileInfo commandFileInfo(command);
    if (commandFileInfo.isExecutable() && commandFileInfo.path() != QLatin1String(".")) {
        return commandFileInfo.absoluteFilePath();
    } else if (commandFileInfo.path() == QLatin1String(".")){
        QString fullCommandFileName = command;
    #ifdef Q_OS_WIN
        if (!command.endsWith(QLatin1String(".exe")))
            fullCommandFileName = command + QLatin1String(".exe");

        static const QString pathSeparator(QLatin1Char(';'));
    #else
        static const QString pathSeparator(QLatin1Char(':'));
    #endif
        QStringList pathList = environment.value(QLatin1String("PATH")).split(pathSeparator);

        foreach (const QString &path, pathList) {
            QString filePath(path + QDir::separator() + fullCommandFileName);
            if (QFileInfo(filePath).isExecutable())
                return commandFileInfo.absoluteFilePath();
        }
    }
    return QString();
}

TestRunner *TestRunner::instance()
{
    if (!m_instance)
        m_instance = new TestRunner;
    return m_instance;
}

TestRunner::TestRunner(QObject *parent) :
    QObject(parent),
    m_executingTests(false)
{
    connect(&m_futureWatcher, &QFutureWatcher<TestResultPtr>::resultReadyAt,
            this, [this](int index) { emit testResultReady(m_futureWatcher.resultAt(index)); });
    connect(&m_futureWatcher, &QFutureWatcher<TestResultPtr>::finished,
            this, &TestRunner::onFinished);
    connect(this, &TestRunner::requestStopTestRun,
            &m_futureWatcher, &QFutureWatcher<TestResultPtr>::cancel);
    connect(&m_futureWatcher, &QFutureWatcher<TestResultPtr>::canceled,
            this, [this]() { emit testResultReady(TestResultPtr(new FaultyTestResult(
                                                      Result::MessageFatal,
                                                      QObject::tr("Test run canceled by user."))));
    });
}

TestRunner::~TestRunner()
{
    qDeleteAll(m_selectedTests);
    m_selectedTests.clear();
    m_instance = 0;
}

void TestRunner::setSelectedTests(const QList<TestConfiguration *> &selected)
{
     qDeleteAll(m_selectedTests);
     m_selectedTests.clear();
     m_selectedTests = selected;
}

static void performTestRun(QFutureInterface<TestResultPtr> &futureInterface,
                           const QList<TestConfiguration *> selectedTests,
                           const TestSettings &settings)
{
    const int timeout = settings.timeout;
    const QString &metricsOption = TestSettings::metricsTypeToOption(settings.metrics);
    QEventLoop eventLoop;
    int testCaseCount = 0;
    foreach (TestConfiguration *config, selectedTests) {
        config->completeTestInformation();
        if (config->project()) {
            testCaseCount += config->testCaseCount();
        } else {
            futureInterface.reportResult(TestResultPtr(new FaultyTestResult(Result::MessageWarn,
                QObject::tr("Project is null for \"%1\". Removing from test run.\n"
                            "Check the test environment.").arg(config->displayName()))));
        }
    }

    QProcess testProcess;
    testProcess.setReadChannel(QProcess::StandardOutput);

    futureInterface.setProgressRange(0, testCaseCount);
    futureInterface.setProgressValue(0);

    foreach (const TestConfiguration *testConfiguration, selectedTests) {
        QScopedPointer<TestOutputReader> outputReader;
        switch (testConfiguration->testType()) {
        case TestTypeQt:
            outputReader.reset(new QtTestOutputReader(futureInterface, &testProcess,
                                                      testConfiguration->buildDirectory()));
            break;
        case TestTypeGTest:
            outputReader.reset(new GTestOutputReader(futureInterface, &testProcess,
                                                     testConfiguration->buildDirectory()));
            break;
        }
        if (futureInterface.isCanceled())
            break;

        if (!testConfiguration->project())
            continue;

        QProcessEnvironment environment = testConfiguration->environment().toProcessEnvironment();
        QString commandFilePath = executableFilePath(testConfiguration->targetFile(), environment);
        if (commandFilePath.isEmpty()) {
            futureInterface.reportResult(TestResultPtr(new FaultyTestResult(Result::MessageFatal,
                QObject::tr("Could not find command \"%1\". (%2)")
                                                   .arg(testConfiguration->targetFile())
                                                   .arg(testConfiguration->displayName()))));
            continue;
        }

        if (testConfiguration->testType() == TestTypeQt) {
            QStringList argumentList(QLatin1String("-xml"));
            if (!metricsOption.isEmpty())
                argumentList << metricsOption;
            if (testConfiguration->testCases().count())
                argumentList << testConfiguration->testCases();
            testProcess.setArguments(argumentList);
        } else { // TestTypeGTest
            QStringList argumentList;
            const QStringList &testSets = testConfiguration->testCases();
            if (testSets.size()) {
                argumentList << QLatin1String("--gtest_filter=")
                                + testSets.join(QLatin1Char(':'));
            }
            if (settings.gtestRunDisabled)
                argumentList << QLatin1String("--gtest_also_run_disabled_tests");
            if (settings.gtestRepeat)
                argumentList << QString::fromLatin1("--gtest_repeat=%1").arg(settings.gtestIterations);
            if (settings.gtestShuffle) {
                argumentList << QLatin1String("--gtest_shuffle");
                argumentList << QString::fromLatin1("--gtest_random_seed=%1").arg(settings.gtestSeed);
            }
            testProcess.setArguments(argumentList);
        }

        testProcess.setWorkingDirectory(testConfiguration->workingDirectory());
        if (Utils::HostOsInfo::isWindowsHost())
            environment.insert(QLatin1String("QT_LOGGING_TO_CONSOLE"), QLatin1String("1"));
        testProcess.setProcessEnvironment(environment);
        testProcess.setProgram(commandFilePath);
        testProcess.start();

        bool ok = testProcess.waitForStarted();
        QTime executionTimer;
        executionTimer.start();
        bool canceledByTimeout = false;
        if (ok) {
            while (testProcess.state() == QProcess::Running) {
                if (executionTimer.elapsed() >= timeout) {
                    canceledByTimeout = true;
                    break;
                }
                if (futureInterface.isCanceled()) {
                    testProcess.kill();
                    testProcess.waitForFinished();
                    return;
                }
                eventLoop.processEvents();
            }
        }
        if (testProcess.exitStatus() == QProcess::CrashExit) {
            futureInterface.reportResult(TestResultPtr(new FaultyTestResult(Result::MessageFatal,
                QString::fromLatin1("Test for project \"%1\" crashed.").arg(testConfiguration->displayName()))));
        }

        if (canceledByTimeout) {
            if (testProcess.state() != QProcess::NotRunning) {
                testProcess.kill();
                testProcess.waitForFinished();
            }
            futureInterface.reportResult(TestResultPtr(
                    new FaultyTestResult(Result::MessageFatal, QObject::tr(
                    "Test case canceled due to timeout. \nMaybe raise the timeout?"))));
        }
    }
    futureInterface.setProgressValue(testCaseCount);
}

void TestRunner::prepareToRunTests()
{
    ProjectExplorer::Internal::ProjectExplorerSettings projectExplorerSettings =
        ProjectExplorer::ProjectExplorerPlugin::projectExplorerSettings();
    if (projectExplorerSettings.buildBeforeDeploy && !projectExplorerSettings.saveBeforeBuild) {
        if (!ProjectExplorer::ProjectExplorerPlugin::saveModifiedFiles())
            return;
    }

    const bool omitRunConfigWarnings = AutotestPlugin::instance()->settings()->omitRunConfigWarn;

    m_executingTests = true;
    emit testRunStarted();

    // clear old log and output pane
    TestResultsPane::instance()->clearContents();

    foreach (TestConfiguration *config, m_selectedTests) {
        if (!omitRunConfigWarnings && config->guessedConfiguration()) {
            emit testResultReady(TestResultPtr(new FaultyTestResult(Result::MessageWarn,
                tr("Project's run configuration was guessed for \"%1\".\n"
                "This might cause trouble during execution.").arg(config->displayName()))));
        }
    }

    if (m_selectedTests.empty()) {
        emit testResultReady(TestResultPtr(new FaultyTestResult(Result::MessageWarn,
            tr("No tests selected. Canceling test run."))));
        onFinished();
        return;
    }

    ProjectExplorer::Project *project = m_selectedTests.at(0)->project();
    if (!project) {
        emit testResultReady(TestResultPtr(new FaultyTestResult(Result::MessageWarn,
            tr("Project is null. Canceling test run.\n"
            "Only desktop kits are supported. Make sure the "
            "currently active kit is a desktop kit."))));
        onFinished();
        return;
    }

    if (!projectExplorerSettings.buildBeforeDeploy) {
        runTests();
    } else {
        if (project->hasActiveBuildSettings()) {
            buildProject(project);
        } else {
            emit testResultReady(TestResultPtr(new FaultyTestResult(Result::MessageFatal,
                tr("Project is not configured. Canceling test run."))));
            onFinished();
            return;
        }
    }
}

void TestRunner::runTests()
{
    QFuture<TestResultPtr> future = Utils::runAsync(&performTestRun, m_selectedTests,
                                                    *AutotestPlugin::instance()->settings());
    m_futureWatcher.setFuture(future);
    Core::ProgressManager::addTask(future, tr("Running Tests"), Autotest::Constants::TASK_INDEX);
}

void TestRunner::buildProject(ProjectExplorer::Project *project)
{
    ProjectExplorer::BuildManager *buildManager = ProjectExplorer::BuildManager::instance();
    m_buildConnect = connect(this, &TestRunner::requestStopTestRun,
                             buildManager, &ProjectExplorer::BuildManager::cancel);
    connect(buildManager, &ProjectExplorer::BuildManager::buildQueueFinished,
            this, &TestRunner::buildFinished);
    ProjectExplorer::ProjectExplorerPlugin::buildProject(project);
}

void TestRunner::buildFinished(bool success)
{
    disconnect(m_buildConnect);
    ProjectExplorer::BuildManager *buildManager = ProjectExplorer::BuildManager::instance();
    disconnect(buildManager, &ProjectExplorer::BuildManager::buildQueueFinished,
               this, &TestRunner::buildFinished);

    if (success) {
        runTests();
    } else {
        emit testResultReady(TestResultPtr(new FaultyTestResult(Result::MessageFatal,
                                                  tr("Build failed. Canceling test run."))));
        onFinished();
    }
}

void TestRunner::onFinished()
{
    m_executingTests = false;
    emit testRunFinished();
}

} // namespace Internal
} // namespace Autotest
