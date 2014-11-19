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
#include "testresultspane.h"
#include "testrunner.h"

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
static QProcess *m_runner = 0;
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
    if (m_runner)
        delete m_runner;
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
                                   ResultType &result, QString &file, int &line)
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

/****************** XML line parser helper end ******************/

void processOutput()
{
    if (!m_runner)
        return;
    static QString className;
    static QString testCase;
    static QString dataTag;
    static ResultType result = ResultType::UNKNOWN;
    static QString description;
    static QString file;
    static int lineNumber = 0;
    static QString duration;
    static bool readingDescription = false;
    static QString qtVersion;
    static QString qtestVersion;

    while (m_runner->canReadLine()) {
        // TODO Qt5 uses UTF-8 - while Qt4 uses ISO-8859-1 - could this be a problem?
        const QString line = QString::fromUtf8(m_runner->readLine()).trimmed();
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
            result = ResultType::UNKNOWN;
            lineNumber = 0;
            readingDescription = false;
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
                    file = QFileInfo(m_runner->workingDirectory(), file).canonicalFilePath();
                testResult.setFileName(file);
                testResult.setLine(lineNumber);
                TestResultsPane::instance()->addTestResult(testResult);
            }
            continue;
        }
        if (line == QLatin1String("</Message>") || line == QLatin1String("</Incident>")) {
            TestResult testResult(className, testCase, dataTag, result, description);
            if (!file.isEmpty())
                file = QFileInfo(m_runner->workingDirectory(), file).canonicalFilePath();
            testResult.setFileName(file);
            testResult.setLine(lineNumber);
            TestResultsPane::instance()->addTestResult(testResult);
            description = QString();
        } else if (line == QLatin1String("</TestFunction>") && !duration.isEmpty()) {
            TestResult testResult(className, testCase, QString(), ResultType::MESSAGE_INTERNAL,
                                  QObject::tr("execution took %1ms").arg(duration));
            TestResultsPane::instance()->addTestResult(testResult);
            m_currentFuture->setProgressValue(m_currentFuture->progressValue() + 1);
        } else if (line == QLatin1String("</TestCase>") && !duration.isEmpty()) {
            TestResult testResult(className, QString(), QString(), ResultType::MESSAGE_INTERNAL,
                                  QObject::tr("Test execution took %1ms").arg(duration));
            TestResultsPane::instance()->addTestResult(testResult);
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
            TestResultsPane::instance()->addTestResult(
                        TestResult(QString(), QString(), QString(), ResultType::MESSAGE_INTERNAL,
                                   QObject::tr("Qt Version: %1").arg(qtVersion)));
        } else if (xmlStartsWith(line, QLatin1String("<QTestVersion>"), qtestVersion)) {
            TestResultsPane::instance()->addTestResult(
                        TestResult(QString(), QString(), QString(), ResultType::MESSAGE_INTERNAL,
                                   QObject::tr("QTest Version: %1").arg(qtestVersion)));
        } else {
//            qDebug() << "Unhandled line:" << line; // TODO remove
        }
    }
}

static QString which(const QString &path, const QString &cmd)
{
    if (path.isEmpty() || cmd.isEmpty())
        return QString();

    QStringList paths;
#ifdef Q_OS_WIN
    paths = path.split(QLatin1Char(';'));
#else
    paths = path.split(QLatin1Char(':'));
#endif

    foreach (const QString &p, paths) {
        const QString fName = p + QDir::separator() + cmd;
        QFileInfo fi(fName);
        if (fi.exists() && fi.isExecutable())
            return fName;
#ifdef Q_OS_WIN
        fi = QFileInfo(fName + QLatin1String(".exe"));
        if (fi.exists())
            return fi.absoluteFilePath();
        fi = QFileInfo(fName + QLatin1String(".bat"));
        if (fi.exists())
            return fi.absoluteFilePath();
        fi = QFileInfo(fName + QLatin1String(".cmd"));
        if (fi.exists())
            return fi.absoluteFilePath();
#endif
    }
    return QString();
}

bool performExec(const QString &cmd, const QStringList &args, const QString &workingDir,
                 const Utils::Environment &env, int timeout = 60000)
{
    QString runCmd;
    if (!QDir::toNativeSeparators(cmd).contains(QDir::separator())) {
        if (env.hasKey(QLatin1String("PATH")))
            runCmd = which(env.value(QLatin1String("PATH")), cmd);
    } else if (QFileInfo(cmd).exists()) {
        runCmd = cmd;
    }

    if (runCmd.isEmpty()) {
        TestResultsPane::instance()->addTestResult(
                    TestResult(QString(), QString(), QString(), ResultType::MESSAGE_FATAL,
                               QObject::tr("*** Could not find command '%1' ***").arg(cmd)));
        return false;
    }

    m_runner->setWorkingDirectory(workingDir);
    m_runner->setProcessEnvironment(env.toProcessEnvironment());
    QTime executionTimer;

    if (args.count()) {
        m_runner->start(runCmd, args);
    } else {
        m_runner->start(runCmd);
    }

    bool ok = m_runner->waitForStarted();
    executionTimer.start();
    if (ok) {
        while (m_runner->state() == QProcess::Running && executionTimer.elapsed() < timeout) {
            if (m_currentFuture->isCanceled()) {
                m_runner->kill();
                m_runner->waitForFinished();
                TestResultsPane::instance()->addTestResult(
                            TestResult(QString(), QString(), QString(), ResultType::MESSAGE_FATAL,
                                       QObject::tr("*** Test Run canceled by user ***")));
            }
            qApp->processEvents();
        }
    }
    if (ok && executionTimer.elapsed() < timeout) {
        return m_runner->exitCode() == 0;
    } else {
        if (m_runner->state() != QProcess::NotRunning) {
            m_runner->kill();
            m_runner->waitForFinished();
            TestResultsPane::instance()->addTestResult(
                        TestResult(QString(), QString(), QString(), ResultType::MESSAGE_FATAL,
                                   QObject::tr("*** Test Case canceled due to timeout ***\n"
                                               "Maybe raise the timeout?")));
        }
        return false;
    }
}

void performTestRun(QFutureInterface<void> &future, const QList<TestConfiguration *> selectedTests)
{
    int testCaseCount = 0;
    foreach (const TestConfiguration *config, selectedTests)
        testCaseCount += config->testCaseCount();

    m_currentFuture = &future;
    m_runner = new QProcess;
    m_runner->setReadChannelMode(QProcess::MergedChannels);
    m_runner->setReadChannel(QProcess::StandardOutput);

    QObject::connect(m_runner, &QProcess::readyReadStandardOutput, &processOutput);

    future.setProgressRange(0, testCaseCount);
    future.setProgressValue(0);

    foreach (const TestConfiguration *tc, selectedTests) {
        if (future.isCanceled())
            break;
        QString cmd = tc->targetFile();
        QString workDir = tc->workingDirectory();
        QStringList args;
        Utils::Environment env = tc->environment();

        args << QLatin1String("-xml");
        if (tc->testCases().count())
            args << tc->testCases();

        performExec(cmd, args, workDir, env);
    }
    future.setProgressValue(testCaseCount);

    delete m_runner;
    m_runner = 0;
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
            TestResultsPane::instance()->addTestResult(
                        TestResult(QString(), QString(), QString(), ResultType::MESSAGE_WARN,
                                   tr("*** Project is null for '%1' - removing from Test Run ***\n"
                                      "This might be the case for a faulty environment or similar."
                                      ).arg(config->displayName())));
        }
    foreach (TestConfiguration *config, toBeRemoved) {
        m_selectedTests.removeOne(config);
        delete config;
    }

    if (m_selectedTests.empty()) {
        TestResultsPane::instance()->addTestResult(
                    TestResult(QString(), QString(), QString(), ResultType::MESSAGE_WARN,
                               tr("*** No tests selected - canceling Test Run ***")));
        return;
    }

    ProjectExplorer::Project *project = m_selectedTests.at(0)->project();
    if (!project) {
        TestResultsPane::instance()->addTestResult(
                    TestResult(QString(), QString(), QString(), ResultType::MESSAGE_WARN,
                               tr("*** Project is null - canceling Test Run ***\n"
                                  "Actually only Desktop kits are supported - make sure the "
                                  "current active kit is a Desktop kit.")));
        return;
    }

    ProjectExplorer::ProjectExplorerPlugin *pep = ProjectExplorer::ProjectExplorerPlugin::instance();
    ProjectExplorer::Internal::ProjectExplorerSettings pes = pep->projectExplorerSettings();
    if (pes.buildBeforeDeploy) {
        if (!project->hasActiveBuildSettings()) {
            TestResultsPane::instance()->addTestResult(
                        TestResult(QString(), QString(), QString(), ResultType::MESSAGE_FATAL,
                                   tr("*** Project is not configured - canceling Test Run ***")));
            return;
        }
        buildProject(project);
        while (m_building) {
            qApp->processEvents();
        }

        if (!m_buildSucceeded) {
            TestResultsPane::instance()->addTestResult(
                        TestResult(QString(), QString(), QString(), ResultType::MESSAGE_FATAL,
                                   tr("*** Build failed - canceling Test Run ***")));
            return;
        }
    }

    m_executingTests = true;
    emit testRunStarted();
    QFuture<void> future = QtConcurrent::run(&performTestRun , m_selectedTests);
    Core::FutureProgress *progress = Core::ProgressManager::addTask(future, tr("Running Tests"),
                                                                    Autotest::Constants::TASK_INDEX);
    connect(progress, &Core::FutureProgress::finished,
            TestRunner::instance(), &TestRunner::onFinished);
}

void TestRunner::buildProject(ProjectExplorer::Project *project)
{
    m_building = true;
    m_buildSucceeded = false;
    ProjectExplorer::BuildManager *mgr = static_cast<ProjectExplorer::BuildManager *>(
                ProjectExplorer::BuildManager::instance());
    ProjectExplorer::ProjectExplorerPlugin *pep = ProjectExplorer::ProjectExplorerPlugin::instance();
    pep->buildProject(project);
    connect(mgr, &ProjectExplorer::BuildManager::buildQueueFinished,
            this, &TestRunner::buildFinished);
}

void TestRunner::buildFinished(bool success)
{
    ProjectExplorer::BuildManager *mgr = static_cast<ProjectExplorer::BuildManager *>(
                ProjectExplorer::BuildManager::instance());
    disconnect(mgr, &ProjectExplorer::BuildManager::buildQueueFinished,
               this, &TestRunner::buildFinished);
    m_building = false;
    m_buildSucceeded = success;
}

void TestRunner::onFinished()
{
    m_executingTests = false;
    emit testRunFinished();
}

void TestRunner::stopTestRun()
{
    if (m_runner && m_runner->state() != QProcess::NotRunning && m_currentFuture)
        m_currentFuture->cancel();
}

} // namespace Internal
} // namespace Autotest
