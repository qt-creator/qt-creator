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

#include "autotestconstants.h"
#include "autotestplugin.h"
#include "testresultspane.h"
#include "testrunner.h"
#include "testsettings.h"

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
static QFutureInterface<void> *m_currentFuture = 0;

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

/******************** XML line parser helper ********************/

static bool xmlStartsWith(const QString &code, const QString &start, QString &result)
{
    if (code.startsWith(start)) {
        result = code.mid(start.length());
        result = result.left(result.indexOf(QLatin1Char('"')));
        result = result.left(result.indexOf(QLatin1String("</")));
        return !result.isEmpty();
    }
    return false;
}

static bool xmlCData(const QString &code, const QString &start, QString &result)
{
    if (code.startsWith(start)) {
        int index = code.indexOf(QLatin1String("<![CDATA[")) + 9;
        result = code.mid(index, code.indexOf(QLatin1String("]]>"), index) - index);
        return !result.isEmpty();
    }
    return false;
}

static bool xmlExtractTypeFileLine(const QString &code, const QString &tagStart,
                                   Result::Type &result, QString &file, int &line)
{
    if (code.startsWith(tagStart)) {
        int start = code.indexOf(QLatin1String(" type=\"")) + 7;
        result = TestResult::resultFromString(
                    code.mid(start, code.indexOf(QLatin1Char('"'), start) - start));
        start = code.indexOf(QLatin1String(" file=\"")) + 7;
        file = code.mid(start, code.indexOf(QLatin1Char('"'), start) - start);
        start = code.indexOf(QLatin1String(" line=\"")) + 7;
        line = code.mid(start, code.indexOf(QLatin1Char('"'), start) - start).toInt();
        return true;
    }
    return false;
}

// adapted from qplaintestlogger.cpp
static QString formatResult(double value)
{
    if (value < 0 || value == NAN)
        return QLatin1String("NAN");
    if (value == 0)
        return QLatin1String("0");

    int significantDigits = 0;
    qreal divisor = 1;

    while (value / divisor >= 1) {
        divisor *= 10;
        ++significantDigits;
    }

    QString beforeDecimalPoint = QString::number(value, 'f', 0);
    QString afterDecimalPoint = QString::number(value, 'f', 20);
    afterDecimalPoint.remove(0, beforeDecimalPoint.count() + 1);

    const int beforeUse = qMin(beforeDecimalPoint.count(), significantDigits);
    const int beforeRemove = beforeDecimalPoint.count() - beforeUse;

    beforeDecimalPoint.chop(beforeRemove);
    for (int i = 0; i < beforeRemove; ++i)
        beforeDecimalPoint.append(QLatin1Char('0'));

    int afterUse = significantDigits - beforeUse;
    if (beforeDecimalPoint == QLatin1String("0") && !afterDecimalPoint.isEmpty()) {
        ++afterUse;
        int i = 0;
        while (i < afterDecimalPoint.count() && afterDecimalPoint.at(i) == QLatin1Char('0'))
            ++i;
        afterUse += i;
    }

    const int afterRemove = afterDecimalPoint.count() - afterUse;
    afterDecimalPoint.chop(afterRemove);

    QString result = beforeDecimalPoint;
    if (afterUse > 0)
        result.append(QLatin1Char('.'));
    result += afterDecimalPoint;

    return result;
}

static bool xmlExtractBenchmarkInformation(const QString &code, const QString &tagStart,
                                           QString &description)
{
    if (code.startsWith(tagStart)) {
        int start = code.indexOf(QLatin1String(" metric=\"")) + 9;
        const QString metric = code.mid(start, code.indexOf(QLatin1Char('"'), start) - start);
        start = code.indexOf(QLatin1String(" value=\"")) + 8;
        const double value = code.mid(start, code.indexOf(QLatin1Char('"'), start) - start).toDouble();
        start = code.indexOf(QLatin1String(" iterations=\"")) + 13;
        const int iterations = code.mid(start, code.indexOf(QLatin1Char('"'), start) - start).toInt();
        QString metricsText;
        if (metric == QLatin1String("WalltimeMilliseconds"))         // default
            metricsText = QLatin1String("msecs");
        else if (metric == QLatin1String("CPUTicks"))                // -tickcounter
            metricsText = QLatin1String("CPU ticks");
        else if (metric == QLatin1String("Events"))                  // -eventcounter
            metricsText = QLatin1String("events");
        else if (metric == QLatin1String("InstructionReads"))        // -callgrind
            metricsText = QLatin1String("instruction reads");
        else if (metric == QLatin1String("CPUCycles"))               // -perf
            metricsText = QLatin1String("CPU cycles");
        description = QObject::tr("%1 %2 per iteration (total: %3, iterations: %4)")
                .arg(formatResult(value))
                .arg(metricsText)
                .arg(formatResult(value * (double)iterations))
                .arg(iterations);
        return true;
    }
    return false;
}

void emitTestResultCreated(const TestResult &testResult)
{
    emit m_instance->testResultCreated(testResult);
}

/****************** XML line parser helper end ******************/

void processOutput(QProcess *testRunner)
{
    if (!testRunner)
        return;
    static QString className;
    static QString testCase;
    static QString dataTag;
    static Result::Type result = Result::UNKNOWN;
    static QString description;
    static QString file;
    static int lineNumber = 0;
    static QString duration;
    static bool readingDescription = false;
    static QString qtVersion;
    static QString qtestVersion;
    static QString benchmarkDescription;

    while (testRunner->canReadLine()) {
        // TODO Qt5 uses UTF-8 - while Qt4 uses ISO-8859-1 - could this be a problem?
        const QString line = QString::fromUtf8(testRunner->readLine()).trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1String("<?xml version"))) {
            className = QString();
            continue;
        }
        if (xmlStartsWith(line, QLatin1String("<TestCase name=\""), className))
            continue;
        if (xmlStartsWith(line, QLatin1String("<TestFunction name=\""), testCase)) {
            dataTag = QString();
            description = QString();
            duration = QString();
            file = QString();
            result = Result::UNKNOWN;
            lineNumber = 0;
            readingDescription = false;
            emitTestResultCreated(
                        TestResult(QString(), QString(), QString(), Result::MESSAGE_CURRENT_TEST,
                                   QObject::tr("Entering Test Function %1::%2")
                                   .arg(className).arg(testCase)));
            continue;
        }
        if (xmlStartsWith(line, QLatin1String("<Duration msecs=\""), duration)) {
            continue;
        }
        if (xmlExtractTypeFileLine(line, QLatin1String("<Message"), result, file, lineNumber))
            continue;
        if (xmlCData(line, QLatin1String("<DataTag>"), dataTag))
            continue;
        if (xmlCData(line, QLatin1String("<Description>"), description)) {
            if (!line.endsWith(QLatin1String("</Description>")))
                readingDescription = true;
            continue;
        }
        if (xmlExtractTypeFileLine(line, QLatin1String("<Incident"), result, file, lineNumber)) {
            if (line.endsWith(QLatin1String("/>"))) {
                TestResult testResult(className, testCase, dataTag, result, description);
                if (!file.isEmpty())
                    file = QFileInfo(testRunner->workingDirectory(), file).canonicalFilePath();
                testResult.setFileName(file);
                testResult.setLine(lineNumber);
                emitTestResultCreated(testResult);
            }
            continue;
        }
        if (xmlExtractBenchmarkInformation(line, QLatin1String("<BenchmarkResult"), benchmarkDescription)) {
            TestResult testResult(className, testCase, dataTag, Result::BENCHMARK, benchmarkDescription);
            emitTestResultCreated(testResult);
            continue;
        }
        if (line == QLatin1String("</Message>") || line == QLatin1String("</Incident>")) {
            TestResult testResult(className, testCase, dataTag, result, description);
            if (!file.isEmpty())
                file = QFileInfo(testRunner->workingDirectory(), file).canonicalFilePath();
            testResult.setFileName(file);
            testResult.setLine(lineNumber);
            emitTestResultCreated(testResult);
            description = QString();
        } else if (line == QLatin1String("</TestFunction>") && !duration.isEmpty()) {
            TestResult testResult(className, testCase, QString(), Result::MESSAGE_INTERNAL,
                                  QObject::tr("execution took %1ms").arg(duration));
            emitTestResultCreated(testResult);
            m_currentFuture->setProgressValue(m_currentFuture->progressValue() + 1);
        } else if (line == QLatin1String("</TestCase>") && !duration.isEmpty()) {
            TestResult testResult(className, QString(), QString(), Result::MESSAGE_INTERNAL,
                                  QObject::tr("Test execution took %1ms").arg(duration));
            emitTestResultCreated(testResult);
        } else if (readingDescription) {
            if (line.endsWith(QLatin1String("]]></Description>"))) {
                description.append(QLatin1Char('\n'));
                description.append(line.left(line.indexOf(QLatin1String("]]></Description>"))));
                readingDescription = false;
            } else {
                description.append(QLatin1Char('\n'));
                description.append(line);
            }
        } else if (xmlStartsWith(line, QLatin1String("<QtVersion>"), qtVersion)) {
            emitTestResultCreated(FaultyTestResult(Result::MESSAGE_INTERNAL,
                QObject::tr("Qt Version: %1").arg(qtVersion)));
        } else if (xmlStartsWith(line, QLatin1String("<QTestVersion>"), qtestVersion)) {
            emitTestResultCreated(FaultyTestResult(Result::MESSAGE_INTERNAL,
                QObject::tr("QTest Version: %1").arg(qtestVersion)));
        } else {
//            qDebug() << "Unhandled line:" << line; // TODO remove
        }
    }
}

static QString which(const QString &pathValue, const QString &command)
{
    if (pathValue.isEmpty() || command.isEmpty())
        return QString();

    QStringList pathList;
#ifdef Q_OS_WIN
    pathList = pathValue.split(QLatin1Char(';'));
#else
    pathList = pathValue.split(QLatin1Char(':'));
#endif

    foreach (const QString &path, pathList) {
        const QString filePath = path + QDir::separator() + command;
        QFileInfo commandFileInfo(filePath);
        if (commandFileInfo.exists() && commandFileInfo.isExecutable())
            return filePath;
#ifdef Q_OS_WIN
        commandFileInfo = QFileInfo(filePath + QLatin1String(".exe"));
        if (commandFileInfo.exists())
            return commandFileInfo.absoluteFilePath();
        commandFileInfo = QFileInfo(filePath + QLatin1String(".bat"));
        if (commandFileInfo.exists())
            return commandFileInfo.absoluteFilePath();
        commandFileInfo = QFileInfo(filePath + QLatin1String(".cmd"));
        if (commandFileInfo.exists())
            return commandFileInfo.absoluteFilePath();
#endif
    }
    return QString();
}

void performTestRun(QFutureInterface<void> &future, const QList<TestConfiguration *> selectedTests, const int timeout, const QString metricsOption, TestRunner* testRunner)
{
    int testCaseCount = 0;
    foreach (const TestConfiguration *config, selectedTests)
        testCaseCount += config->testCaseCount();

    m_currentFuture = &future;
    QProcess testProcess;
    testProcess.setReadChannelMode(QProcess::MergedChannels);
    testProcess.setReadChannel(QProcess::StandardOutput);
    QObject::connect(testRunner, &TestRunner::requestStopTestRun, [&] () {
        if (testProcess.state() != QProcess::NotRunning && m_currentFuture)
            m_currentFuture->cancel();
    });

    QObject::connect(&testProcess, &QProcess::readyReadStandardOutput, [&] () {
        processOutput(&testProcess);
    });

    m_currentFuture->setProgressRange(0, testCaseCount);
    m_currentFuture->setProgressValue(0);

    foreach (const TestConfiguration *testConfiguration, selectedTests) {
        if (m_currentFuture->isCanceled())
            break;
        QString command = testConfiguration->targetFile();
        QString workingDirectory = testConfiguration->workingDirectory();
        QStringList argumentList;
        Utils::Environment environment = testConfiguration->environment();

        argumentList << QLatin1String("-xml");
        if (!metricsOption.isEmpty())
            argumentList << metricsOption;
        if (testConfiguration->testCases().count())
            argumentList << testConfiguration->testCases();
        QString runCommand;
        if (!QDir::toNativeSeparators(command).contains(QDir::separator())) {
            if (environment.hasKey(QLatin1String("PATH")))
                runCommand = which(environment.value(QLatin1String("PATH")), command);
        } else if (QFileInfo(command).exists()) {
            runCommand = command;
        }

        if (runCommand.isEmpty()) {
            emitTestResultCreated(FaultyTestResult(Result::MESSAGE_FATAL,
                QObject::tr("*** Could not find command '%1' ***").arg(command)));
            continue;
        }

        testProcess.setWorkingDirectory(workingDirectory);
        testProcess.setProcessEnvironment(environment.toProcessEnvironment());
        QTime executionTimer;

        if (argumentList.count()) {
            testProcess.start(runCommand, argumentList);
        } else {
            testProcess.start(runCommand);
        }

        bool ok = testProcess.waitForStarted();
        executionTimer.start();
        if (ok) {
            while (testProcess.state() == QProcess::Running && executionTimer.elapsed() < timeout) {
                if (m_currentFuture->isCanceled()) {
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
    m_currentFuture->setProgressValue(testCaseCount);

    m_currentFuture = 0;
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
