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
#include "testrunner.h"

#include "autotestconstants.h"
#include "autotestplugin.h"
#include "testresultspane.h"
#include "testsettings.h"
#include "testxmloutputreader.h"

#include <QDebug> // REMOVE

#include <coreplugin/progressmanager/futureprogress.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <projectexplorer/buildmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorersettings.h>

#include <utils/multitask.h>

#include <QFuture>
#include <QFutureInterface>
#include <QTime>

namespace Autotest {
namespace Internal {

static TestRunner *m_instance = 0;

void emitTestResultCreated(const TestResult &testResult)
{
    emit m_instance->testResultCreated(testResult);
}

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
    m_building(false),
    m_executingTests(false)
{
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

void performTestRun(QFutureInterface<void> &futureInterface, const QList<TestConfiguration *> selectedTests, const int timeout, const QString metricsOption, TestRunner* testRunner)
{
    int testCaseCount = 0;
    foreach (const TestConfiguration *config, selectedTests)
        testCaseCount += config->testCaseCount();

    QProcess testProcess;
    testProcess.setReadChannelMode(QProcess::MergedChannels);
    testProcess.setReadChannel(QProcess::StandardOutput);
    QObject::connect(testRunner, &TestRunner::requestStopTestRun, &testProcess, [&] () {
            futureInterface.cancel(); // this kills the process if that is still in the running loop
    });

    TestXmlOutputReader xmlReader(&testProcess);
    QObject::connect(&xmlReader, &TestXmlOutputReader::increaseProgress, [&] () {
        futureInterface.setProgressValue(futureInterface.progressValue() + 1);
    });
    QObject::connect(&xmlReader, &TestXmlOutputReader::testResultCreated, &emitTestResultCreated);

    QObject::connect(&testProcess, &QProcess::readyRead, &xmlReader, &TestXmlOutputReader::processOutput);

    futureInterface.setProgressRange(0, testCaseCount);
    futureInterface.setProgressValue(0);

    foreach (const TestConfiguration *testConfiguration, selectedTests) {
        if (futureInterface.isCanceled())
            break;

        QProcessEnvironment environment = testConfiguration->environment().toProcessEnvironment();
        QString commandFilePath = executableFilePath(testConfiguration->targetFile(), environment);
        if (commandFilePath.isEmpty()) {
            emitTestResultCreated(FaultyTestResult(Result::MESSAGE_FATAL,
                QObject::tr("*** Could not find command '%1' ***").arg(testConfiguration->targetFile())));
            continue;
        }

        QStringList argumentList(QLatin1String("-xml"));
        if (!metricsOption.isEmpty())
            argumentList << metricsOption;
        if (testConfiguration->testCases().count())
            argumentList << testConfiguration->testCases();
        testProcess.setArguments(argumentList);

        testProcess.setWorkingDirectory(testConfiguration->workingDirectory());
        testProcess.setProcessEnvironment(environment);
        testProcess.setProgram(commandFilePath);
        testProcess.start();

        bool ok = testProcess.waitForStarted();
        QTime executionTimer;
        executionTimer.start();
        if (ok) {
            while (testProcess.state() == QProcess::Running && executionTimer.elapsed() < timeout) {
                if (futureInterface.isCanceled()) {
                    testProcess.kill();
                    testProcess.waitForFinished();
                    emitTestResultCreated(FaultyTestResult(Result::MESSAGE_FATAL,
                                                           QObject::tr("*** Test Run canceled by user ***")));
                }
                qApp->processEvents();
            }
        }

        if (executionTimer.elapsed() >= timeout) {
            if (testProcess.state() != QProcess::NotRunning) {
                testProcess.kill();
                testProcess.waitForFinished();
                emitTestResultCreated(FaultyTestResult(Result::MESSAGE_FATAL, QObject::tr(
                    "*** Test Case canceled due to timeout ***\nMaybe raise the timeout?")));
            }
        }
    }
    futureInterface.setProgressValue(testCaseCount);
}

void TestRunner::runTests()
{
    // clear old log and output pane
    TestResultsPane::instance()->clearContents();

    // handle faulty test configurations
    QList<TestConfiguration *> toBeRemoved;
    foreach (TestConfiguration *config, m_selectedTests)
        if (!config->project()) {
            toBeRemoved.append(config);
            TestResultsPane::instance()->addTestResult(FaultyTestResult(Result::MESSAGE_WARN,
                tr("*** Project is null for '%1' - removing from Test Run ***\n"
                "This might be the case for a faulty environment or similar."
                ).arg(config->displayName())));
        }
    foreach (TestConfiguration *config, toBeRemoved) {
        m_selectedTests.removeOne(config);
        delete config;
    }

    if (m_selectedTests.empty()) {
        TestResultsPane::instance()->addTestResult(FaultyTestResult(Result::MESSAGE_WARN,
            tr("*** No tests selected - canceling Test Run ***")));
        return;
    }

    ProjectExplorer::Project *project = m_selectedTests.at(0)->project();
    if (!project) {
        TestResultsPane::instance()->addTestResult(FaultyTestResult(Result::MESSAGE_WARN,
            tr("*** Project is null - canceling Test Run ***\n"
            "Actually only Desktop kits are supported - make sure the "
            "current active kit is a Desktop kit.")));
        return;
    }

    ProjectExplorer::Internal::ProjectExplorerSettings projectExplorerSettings =
        ProjectExplorer::ProjectExplorerPlugin::projectExplorerSettings();
    if (projectExplorerSettings.buildBeforeDeploy) {
        if (!project->hasActiveBuildSettings()) {
            TestResultsPane::instance()->addTestResult(FaultyTestResult(Result::MESSAGE_FATAL,
                tr("*** Project is not configured - canceling Test Run ***")));
            return;
        }
        buildProject(project);
        while (m_building) {
            qApp->processEvents();
        }

        if (!m_buildSucceeded) {
            TestResultsPane::instance()->addTestResult(FaultyTestResult(Result::MESSAGE_FATAL,
                tr("*** Build failed - canceling Test Run ***")));
            return;
        }
    }

    m_executingTests = true;
    connect(this, &TestRunner::testResultCreated,
            TestResultsPane::instance(), &TestResultsPane::addTestResult,
            Qt::QueuedConnection);

    const QSharedPointer<TestSettings> settings = AutotestPlugin::instance()->settings();
    const int timeout = settings->timeout;
    const QString metricsOption = TestSettings::metricsTypeToOption(settings->metrics);

    emit testRunStarted();

    QFuture<void> future = QtConcurrent::run(&performTestRun, m_selectedTests, timeout, metricsOption, this);
    Core::FutureProgress *progress = Core::ProgressManager::addTask(future, tr("Running Tests"),
                                                                    Autotest::Constants::TASK_INDEX);
    connect(progress, &Core::FutureProgress::finished,
            TestRunner::instance(), &TestRunner::onFinished);
}

void TestRunner::buildProject(ProjectExplorer::Project *project)
{
    m_building = true;
    m_buildSucceeded = false;
    ProjectExplorer::BuildManager *buildManager = static_cast<ProjectExplorer::BuildManager *>(
                ProjectExplorer::BuildManager::instance());
    connect(buildManager, &ProjectExplorer::BuildManager::buildQueueFinished,
            this, &TestRunner::buildFinished);
    ProjectExplorer::ProjectExplorerPlugin::buildProject(project);
}

void TestRunner::buildFinished(bool success)
{
    ProjectExplorer::BuildManager *buildManager = static_cast<ProjectExplorer::BuildManager *>(
                ProjectExplorer::BuildManager::instance());
    disconnect(buildManager, &ProjectExplorer::BuildManager::buildQueueFinished,
               this, &TestRunner::buildFinished);
    m_building = false;
    m_buildSucceeded = success;
}

void TestRunner::onFinished()
{
    disconnect(this, &TestRunner::testResultCreated,
               TestResultsPane::instance(), &TestResultsPane::addTestResult);

    m_executingTests = false;
    emit testRunFinished();
}

} // namespace Internal
} // namespace Autotest
