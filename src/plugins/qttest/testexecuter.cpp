/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "testexecuter.h"
#include "testgenerator.h"
#include "qsystem.h"
#include "testsuite.h"
#include "testoutputwindow.h"
#include "resultsview.h"
#include "testconfigurations.h"
#include "ui_recorddlg.h"

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <texteditor/basetexteditor.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qtsupport/qtversionmanager.h>
#include <projectexplorer/target.h>

#include <utils/stylehelper.h>

#include <stdlib.h>
#ifndef Q_OS_WIN
# include <unistd.h>
#endif
#include <sys/types.h>
#include <signal.h>

#include <QDir>
#include <QTimer>
#include <QProcess>
#include <QMessageBox>
#include <QRegExp>

TestExecuter *TestExecuter::m_instance = 0;

TestExecuter *TestExecuter::instance()
{
    if (!m_instance)
        m_instance = new TestExecuter();
    return m_instance;
}

TestExecuter::TestExecuter() :
    m_recordedEventsEdit(0),
    m_lastFinishedTest(QString())
{
#ifdef QTTEST_DEBUGGER_SUPPORT
    m_debugEngine = 0;
#endif
    m_selectedTests.clear();

    m_curTestCode = 0;
    m_testCfg = 0;
    m_testBusy = false;
    m_runRequired = false;
    m_stopTesting = false;
    m_manualStop = false;
    m_killTestRequested = false;
    m_testOutputFile = QString::fromLatin1("$HOME%1.qttest%1last_test_output").arg(QDir::separator());
    m_pendingFailure = false;
    m_inBuildMode = false;
    m_recordingEvents = false;
    m_abortRecording = false;
    m_progressBar = 0;
    m_testResultsStream = 0;
    m_peekedResult.clear();

    m_executer.setReadChannelMode(QProcess::MergedChannels);
    m_executer.setReadChannel(QProcess::StandardOutput);
    QObject::connect(&m_executer, SIGNAL(readyReadStandardOutput()),
        this, SLOT(parseOutput()), Qt::DirectConnection);

    QObject::connect(&m_executer, SIGNAL(finished(int,QProcess::ExitStatus)),
        this, SLOT(onExecuterFinished()), Qt::DirectConnection);

#ifndef QTTEST_PLUGIN_LEAN
    QObject::connect(QTestIDE::instance(), SIGNAL(syntaxError(QString,QString,int)),
        this, SLOT(syntaxError(QString,QString,int)));
    QObject::connect(&qscriptSystemTest, SIGNAL(testResult(QVariantMap)),
        this, SLOT(onTestResult(QVariantMap)));
#endif
}

TestExecuter::~TestExecuter()
{
    m_instance = 0;
}

void TestExecuter::manualStop()
{
    if (m_manualStop)
        return;
    emit testStop();

    m_manualStop = true;
#ifndef QTTEST_PLUGIN_LEAN
    if (m_curTestCode && m_curTestCode->testType() == TestCode::TypeSystemTest)
        QTestIDE::instance()->abortTest();
#endif

    stopTesting();

    if (m_progressBar)
        m_progressBar->reportCanceled();
    if (m_progressLabel)
        m_progressLabel->setText(tr("Stopping..."));
}

void TestExecuter::stopTesting()
{
    m_killTestRequested = false;
    m_stopTesting = true;
    m_executer.terminate();
    if (!m_executer.waitForFinished(5000))
        m_executer.kill();
}

QWidget *TestExecuter::createProgressWidget()
{
    m_progressLabel = new QLabel;
    m_progressLabel->setAlignment(Qt::AlignCenter);
    // ### TODO this setup should be done by style
    QFont f = m_progressLabel->font();
    f.setBold(true);
    f.setPointSizeF(Utils::StyleHelper::sidebarFontSize());
    m_progressLabel->setFont(f);
    m_progressLabel->setPalette(Utils::StyleHelper::sidebarFontPalette(m_progressLabel->palette()));
    return m_progressLabel;
}

void TestExecuter::runTests(bool singleTest, bool forceManual)
{
    if (m_testBusy)
        return;

    m_selectedTests.clear();
    if (singleTest) {
        QString tmp = TestConfigurations::instance().currentTestCase();
        if (!tmp.isEmpty() && !TestConfigurations::instance().currentTestFunction().isEmpty())
            tmp += "::" + TestConfigurations::instance().currentTestFunction();
        if (!tmp.isEmpty())
            m_selectedTests = QStringList(tmp);
    } else {
        m_selectedTests = TestConfigurations::instance().selectedTests();
    }
    if (m_selectedTests.count() == 0) {
        testOutputPane()->append(tr("No test selected"));
        endTest();
        return;
    }
    runSelectedTests(forceManual);
}

void TestExecuter::runSelectedTests(bool forceManual)
{
    if (m_testBusy)
        return;
    m_testBusy = true;
    emit testStarted();

    m_manualStop = false;

    testOutputPane()->clear();

    int maxProgress = 0;
    m_lastFinishedTest.clear();
    QString lastTc;
    bool lastIsSystemTest = false;
    // pretend we executed these tests - so the progress bar keeps counting nicely.
    foreach (const QString &item, m_selectedTests) {
        QString tmp = item.left(item.indexOf("::"));
        if (lastTc != tmp) {
            lastTc = tmp;
            ++maxProgress;

            TestCode *tmp = m_testCollection.findCodeByTestCaseName(lastTc);
            if (tmp && tmp->testType() == TestCode::TypeSystemTest)
                lastIsSystemTest = true;
        } else if (lastIsSystemTest) {
            ++maxProgress;
        }
    }

    m_testCfg = TestConfigurations::instance().activeConfiguration();
    if (!m_testCfg) {
        testOutputPane()->append(tr("No test configuration defined. This is unusual."));
        endTest();
        return;
    }

    m_pendingInsertions.clear();
    TestResultsWindow::instance()->clearContents();
    testOutputPane()->append(tr("*********** Start testing  *************"));

    TestResultsWindow::instance()->popup();

    m_curTestCode = 0;
    QString changeNo;
    QString lastConfigName;
    QString lastPlatform;
    QString lastBranch;
    m_testCfg = 0;
    m_testFailedUnexpectedly = false;

    bool uploadResults = false;

    if (m_progressBar == 0)
        m_progressBar = new QFutureInterface<void>;
    QFutureWatcher<void> fw;
    fw.setFuture(m_progressBar->future());
    Core::FutureProgress *progress =
        Core::ICore::instance()->progressManager()->addTask(m_progressBar->future(),
        tr("Testing"), "QtTest::TestExecuter::Testing", Core::ProgressManager::KeepOnFinish);
    progress->setWidget(createProgressWidget());
    connect(progress, SIGNAL(clicked()), TestResultsWindow::instance(), SLOT(popup()));
    connect(&fw, SIGNAL(canceled()), this, SLOT(manualStop()));

    m_progress = 0;
    m_progressBar->setProgressRange(0, maxProgress);
    m_progressBar->reportStarted();
    m_progressBar->setProgressValueAndText(0, tr("Just started"));

    m_progressFailCount = 0;
    m_progressPassCount = 0;

    while (!m_manualStop && getNextTest()) {
        m_progressBar->setProgressValue(m_progress++);
        if (!testStopped())
            m_progressLabel->setText(tr("%1 failed\n%2 passed")
                .arg(m_progressFailCount).arg(m_progressPassCount));

        // Is this a valid test?
        if (!m_curTestCode) {
            addTestResult("CFAIL", QString(), "Path '" + m_curTestCode->actualFileName()
                + "' does not contain a valid testcase");
            continue;
        }

        m_testCfg = TestConfigurations::instance().findConfig(m_curTestCode->actualBasePath());
        if (!m_testCfg) {
            testOutputPane()->append(tr("Test configuration for '%1' not found. Skipping test.").arg(m_curTestCode->actualFileName()));
            continue;
        }

        if (m_testCfg->configName() != lastConfigName) {
            if (uploadResults && !lastConfigName.isEmpty())
                m_testResultUploader.uploadResultsToDatabase(saveResults(changeNo, lastPlatform, lastBranch),
                    m_testCfg);

            uploadResults = m_testCfg->uploadResults();
            if (!uploadResults)
                testOutputPane()->append(tr("Test results will not be uploaded into the results database."));
            QSystem::unsetEnvKey(m_testCfg->buildEnvironment(), "QTEST_COLORED");
            QSystem::addEnvPath(m_testCfg->buildEnvironment(), "PATH",
                m_testCfg->buildPath() + QDir::separator() + "bin");

            if (uploadResults) {
                QRegExp validBranchRegEx(QLatin1String("^(.+)-(.+)"));
                QRegExp validBranchSpecializationRegEx(QLatin1String(".*"));
                if (m_testCfg->uploadBranch().isEmpty() || !validBranchRegEx.exactMatch(m_testCfg->uploadBranch())) {
                    testOutputPane()->append(tr("-- ATTENTION: Uploading of test results failed. No branch specified or branch "
                        "name \"%1\" is not in form: \n\t<Product>-<Version>\n. Check \"Branch\" value in Test Settings.")
                        .arg(m_testCfg->uploadBranch()));
                    uploadResults = false;
                } else {
                    if (!m_testCfg->uploadBranchSpecialization().isEmpty() && !validBranchSpecializationRegEx.exactMatch(m_testCfg->uploadBranchSpecialization())) {
                        testOutputPane()->append(tr("-- ATTENTION: Uploading of test results failed. "
                            "Optional Branch specialization value \"%1\" is not in form: \n\t<Specialization>\n. "
                            "Check \"Branch\" \"Specialization\" value in Test Settings.")
                            .arg(m_testCfg->uploadBranchSpecialization()));
                        uploadResults = false;
                    } else {
                        lastBranch = m_testCfg->uploadBranch();
                        if (!m_testCfg->uploadBranchSpecialization().isEmpty())
                            lastBranch += QLatin1String("-") + m_testCfg->uploadBranchSpecialization();
                        testOutputPane()->append(tr("Tested branch: %1").arg(lastBranch));
                    }
                }

                lastPlatform = m_testCfg->QMAKESPEC();
                if (lastPlatform.isEmpty()) {
                    testOutputPane()->append(tr("-- ATTENTION: Uploading of test results failed. "
                        "No QMAKESPEC specified. Set QMAKESPEC in project's build environment "
                        "or set a custom \"QMakespec\" value in Test Settings."));
                    uploadResults = false;
                } else {
                    QRegExp validQmakespecSpecializationRegEx(QLatin1String(".*"));
                    QString lastPlatformSpecialization = m_testCfg->QMAKESPECSpecialization();
                    if (!lastPlatformSpecialization.isEmpty() && !validQmakespecSpecializationRegEx.exactMatch(lastPlatformSpecialization)) {
                        testOutputPane()->append(tr("-- ATTENTION: Uploading of test results failed. "
                            "QMAKESPEC \"Specialization\" \"%1\" not in form: \n\t <Specialization>\n. "
                            "Set the QMAKESPEC specialization value in Test Settings or clear its current value.")
                            .arg(lastPlatformSpecialization));
                        uploadResults = false;
                    } else {
                        if (!lastPlatformSpecialization.isEmpty())
                            lastPlatform += QLatin1String("_") + lastPlatformSpecialization;
                        testOutputPane()->append(tr("Tested platform: %1").arg(lastPlatform));
                    }
                }

                changeNo = m_testCfg->uploadChange();
                if (changeNo.isEmpty()) {
                    testOutputPane()->append(tr("-- ATTENTION: Uploading of test results failed. "
                        "No changeNumber specified. Set a custom \"Change\" value in Test Settings."));
                    uploadResults = false;
                } else {
                    testOutputPane()->append(tr("Testing change: %1").arg(changeNo));
                }
                testOutputPane()->append(tr("Tester: %1").arg(QSystem::userName()));
                testOutputPane()->append(tr("Host machine: %1").arg(sanitizedForFilename(QSystem::hostName())));
            }
        }

        lastConfigName = m_testCfg->configName();

        testOutputPane()->append(QString());
        bool ok = buildTestCase();
        if (ok && !m_manualStop)
            ok = runTestCase(forceManual);
        if (ok && !m_manualStop)
            ok = postProcess();
    }
    m_progressBar->setProgressValue(m_progress);

    if (testStopped()) {
        testOutputPane()->append(tr("-- Testing halted by user"));
    } else {
        m_progressLabel->setText(tr("%1 failed\n%2 passed")
            .arg(m_progressFailCount).arg(m_progressPassCount));

        if (uploadResults) {
            // save the xml results and upload files to version control and xml results to database
            m_testResultUploader.uploadResultsToDatabase(saveResults(changeNo, lastPlatform, lastBranch), m_testCfg);
        }
    }

    testOutputPane()->append(QString());
    testOutputPane()->append(tr("******** All testing finished **********"));

    TestResultsWindow::instance()->popup();

    if (m_progressFailCount > 0)
        m_progressBar->reportCanceled();
    m_progressBar->reportFinished();

    applyPendingInsertions();

    delete m_progressBar;
    m_progressBar = 0;
    endTest();
}

void TestExecuter::endTest()
{
    m_testBusy = false;
    if (m_progressLabel && m_manualStop)
        m_progressLabel->setText(tr("Cancelled"));

    emit testFinished();
}

bool TestExecuter::buildTestCase()
{
    m_syntaxError.clear();

    if (!m_testCfg) {
        testOutputPane()->append(tr("No test configuration defined: building test case aborted"));
        return false;
    }

    QString realTc = m_curTestCode->targetFileName(m_testCfg->buildPath());

    // Make sure build directory exists
    QFileInfo tgtInf(realTc);
    QString tgtPath = QDir::convertSeparators(tgtInf.absolutePath());
    QDir().mkpath(tgtPath);

    testOutputPane()->append(tr("********** Build %1 **********").arg(tgtInf.baseName()));

    QString proFile = m_curTestCode->projectFileName();
    if (proFile.isEmpty()) {
        addTestResult("CFAIL", QString(), "No .pro file found.");
        return false;
    }
    m_inBuildMode = true;
    bool isSystemTest = (m_curTestCode->testType() == TestCode::TypeSystemTest);
    bool ok = exec(m_testCfg->qmakeCommand(isSystemTest), QStringList() << proFile, tgtPath);

    if (ok && m_curTestCode->testType() != TestCode::TypeSystemTest) {
        if (m_testCfg->makeCommand().isEmpty()) {
            const QString path = QSystem::envKey(m_testCfg->buildEnvironment(), "PATH");
            testOutputPane()->append(tr("-- No 'make' or 'nmake' instance found in PATH (%1).").arg(path));
            return false;
        }

        ok = exec(m_testCfg->makeCommand(), QStringList(), tgtPath);
    }
    m_inBuildMode = false;

    return ok;
}

bool TestExecuter::postProcess()
{
    if (!m_testCfg) {
        testOutputPane()->append(tr("No test configuration defined: post processing results aborted"));
        return false;
    }

    QString cmd = m_testCfg->postProcessScript();
    if (cmd.isEmpty())
        return true;

    QSystem::setEnvKey(m_testCfg->buildEnvironment(), "TEST_OUTPUT_FILE", m_testOutputFile);
    QSystem::processEnvValue(m_testCfg->buildEnvironment(), cmd);

    if (exec(cmd,QStringList()))
        return true;

    addTestResult("FAIL", QString(),
                  QString::fromLatin1("Could not run postprocess script on Test Case '%1'").arg(m_curTestCode->actualFileName()));
    return false;
}

bool xmlLineStartsWith(QString &line, const QString &expression, QString &variable)
{
    if (line.startsWith(expression)) {
        variable = line.mid(expression.length());
        variable = variable.left(variable.indexOf(QLatin1Char('"')));
        variable = variable.left(variable.indexOf("</"));
        return !variable.isEmpty();
    }
    return false;
}

bool xmlLineCData(QString &line, const QString &expression, QString &variable)
{
    if (line.startsWith(expression)) {
        int pos = line.indexOf("<![CDATA[");
        if (pos > 0) {
            variable = line.mid(pos+9);
            variable = variable.left(variable.indexOf("]]>"));
            return !variable.isEmpty();
        }
    }
    return false;
}

void xmlLineVariable(QString &line, const QString &expression, QString &variable)
{
    int pos = line.indexOf(expression);
    if (pos > 0) {
        variable = line.mid(pos+expression.length()+1);
        variable = variable.left(variable.indexOf(QLatin1Char('"')));
    }
}

bool TestExecuter::canReadLine()
{
    if (m_testResultsStream) {
        if (!m_peekedResult.isEmpty())
            return true;
        return !m_testResultsStream->atEnd();
    } else {
        return m_executer.canReadLine();
    }
}

QString TestExecuter::readLine()
{
    if (m_testResultsStream) {
        if (!m_peekedResult.isEmpty()) {
            QString tmp = m_peekedResult;
            m_peekedResult.clear();
            return tmp;
        }
        return m_testResultsStream->readLine();
    } else {
        return m_executer.readLine();
    }
}

QStringList TestExecuter::peek()
{
    if (m_testResultsStream) {
        QStringList tmp;
        if (!m_peekedResult.isEmpty()) {
            tmp.append(m_peekedResult);
            m_peekedResult.clear();
        }
        while (!m_testResultsStream->atEnd()) {
            QString line = m_testResultsStream->readLine();
            if (line.startsWith(QLatin1Char(' '))) {
                tmp.append(line);
            } else {
                m_peekedResult = line;
                return tmp;
            }
        }
        m_peekedResult.clear();
        return tmp;
    } else {
        QByteArray data = m_executer.peek(1024);
        QStringList tmp;
        QStringList lines = QString(data).split(QLatin1Char('\n'));
        for (int i = 0; i < lines.size(); ++i) {
            if (lines[i].startsWith(QLatin1Char(' ')))
                tmp.append(lines[i]);
            else
                return tmp;
        }
        return tmp;
    }
}

void TestExecuter::parseOutput()
{
    if (!m_testCfg) {
        testOutputPane()->append(tr("No test configuration defined: parsing output aborted"));
        return;
    }

    // if we get more output _after_ we have started the kill timer, we cancel the operation
    // and leave it to the user to terminate a hanging test.
    m_killTestRequested = false;

    m_executeTimer.start();
    while (canReadLine()) {
        QString line = readLine();
        line = line.trimmed();
          if (m_inBuildMode) {
            // Parse build output
            if (line.contains(" error: ")) {
                QString file = line.section(QLatin1Char(':'), 0, 0);

                int lineNum = line.section(QLatin1Char(':'), 1, 1).toInt();

                // The error string will continue across multiple lines so need to peek
                QString errorStr = line.section(QLatin1Char(':'), 3);
                errorStr.remove(QLatin1Char('\n'));
                QStringList lines = peek();
                for (int i = 0; i < lines.size(); ++i)
                    errorStr.append(QLatin1Char(' ') + lines[i].trimmed());

                addTestResult("CFAIL", QString(), errorStr, file, lineNum);
            } else {
                if (!m_testSettings.showVerbose()) {
                    QStringList tmp = line.split(QLatin1Char(' '));
                    if (tmp.count() > 0) {
                        QString cmd = tmp[0];
                        if (cmd.contains("moc")) {
                            line = "moc " + tmp[tmp.count()-3].split(QDir::separator()).last();
                        } else if (cmd.contains("g++") || cmd.contains("gcc")) {
                            if (tmp[1] == "-c") {
                                line = cmd + " -c "
                                    + tmp[tmp.count()-1].split(QDir::separator()).last();
                            } else if (tmp.contains("-o")) {
                                int pos = tmp.indexOf("-o");
                                if (pos > 0 && (pos < tmp.count()-2)) {
                                    line = cmd + QLatin1Char(' ') + tmp[pos+1].split(QDir::separator()).last();
                                }
                            }
                        }
                    }
                }
            }
        } else {
            if (m_xmlMode) {
                if (line.startsWith(QLatin1String("<?xml version"))) {
                    m_xmlTestfunction.clear();
                    m_xmlTestcase.clear();
                    m_xmlDatatag.clear();
                    m_xmlFile.clear();
                    m_xmlLine = "-1";
                    m_xmlResult.clear();
                    m_xmlDescription.clear();
                    m_xmlQtVersion.clear();
                    m_xmlQtestVersion.clear();
                    m_xmlLog.clear();
                    continue;
                }

                if (!line.isEmpty()) {
                    m_xmlLog.append(line);
                    if (line == "</Message>" || line == "</Incident>") {
                        if (!m_xmlResult.isEmpty()) {
                            line = m_xmlResult.toUpper();

                            if (line == "FAIL")
                                line += QLatin1Char('!');

                            while (line.length() < 7)
                                line += QLatin1Char(' ');

                            QString testDescriptor = m_xmlTestcase + "::"
                                + m_xmlTestfunction + QLatin1Char('(')+m_xmlDatatag+QLatin1Char(')');
                            line += ": " + testDescriptor + m_xmlDescription;

                            if (!m_xmlFile.isEmpty())
                                line += "\nLoc: [" + m_xmlFile + QLatin1Char('(') + m_xmlLine + ")]";

                            if (m_curTestCode->testType() != TestCode::TypeSystemTest) {
                                if (m_xmlResult.contains("fail") || m_xmlResult.contains("pass")) {
                                    addTestResult(m_xmlResult.toUpper(), QString(),
                                        testDescriptor + m_xmlDescription,
                                        m_xmlFile, m_xmlLine.toInt(), m_xmlDatatag);
                                }
                            }
                        }
                        m_xmlResult.clear();
                        m_xmlDescription.clear();

                    } else if (xmlLineCData(line, "<Description>", m_xmlDescription)) {
                        if (!m_xmlDescription.isEmpty())
                            m_xmlDescription = QLatin1Char('\n') + m_xmlDescription;
                        continue;

                    } else if (xmlLineCData(line, "<DataTag>", m_xmlDatatag)) {
                        continue;

                    } else if (xmlLineStartsWith(line, "<Message type=\"", m_xmlResult)) {
                        xmlLineVariable(line, "file=", m_xmlFile);

                        if (m_xmlFile.startsWith(m_testCfg->buildPath()))
                            m_xmlFile = m_xmlFile.replace(m_testCfg->buildPath(), m_testCfg->srcPath());

                        xmlLineVariable(line, "line=", m_xmlLine);
                        continue;

                    } else if (xmlLineStartsWith(line, "<Incident type=\"", m_xmlResult)) {
                        xmlLineVariable(line, "file=", m_xmlFile);

                        if (m_xmlFile.startsWith(m_testCfg->buildPath()))
                            m_xmlFile = m_xmlFile.replace(m_testCfg->buildPath(),m_testCfg->srcPath());

                        xmlLineVariable(line,"line=",m_xmlLine);

                        if (line.endsWith(QLatin1String("/>"))) {
                            line = m_xmlResult.toUpper();
                            while (line.length() < 7)
                                line += QLatin1Char(' ');
                            QString testDescriptor = m_xmlTestcase + "::"
                                + m_xmlTestfunction + QLatin1Char('(')+m_xmlDatatag+QLatin1Char(')');
                            line += ": " + testDescriptor + m_xmlDescription;

                            if (!m_xmlFile.isEmpty())
                                line += "\nLoc: [" + m_xmlFile + QLatin1Char('(') + m_xmlLine + ")]";

                            if (m_curTestCode->testType() != TestCode::TypeSystemTest) {
                                if (m_xmlResult.contains("fail") || m_xmlResult.contains("pass")) {
                                    addTestResult(m_xmlResult.toUpper(), QString(), testDescriptor
                                        + m_xmlDescription, m_xmlFile, m_xmlLine.toInt(), m_xmlDatatag);
                                }
                            }
                        }

                    } else if (line == "</TestFunction>") {
                        m_xmlTestfunction.clear();
                        m_xmlDatatag.clear();
                        continue;

                    } else if (line == "</TestCase>") {
                        line = "********* Finished testing of " + m_xmlTestcase + " *********";
                        m_xmlTestcase.clear();
                        // In case the testcase hangs at the end, we need a mechanism
                        // to terminate the process. So set a timer that will kill the
                        // executer if we haven't heard anything for 1.5 seconds.
                        m_killTestRequested = true;
                        QTimer::singleShot(1500, this, SLOT(onKillTestRequested()));

                    } else if (line.startsWith(QLatin1String("</Environment>"))) {
                        line = "Config: Using QTest library " + m_xmlQtestVersion
                            + ", Qt " + m_xmlQtVersion;

                    } else if (xmlLineStartsWith(line, "<TestCase name=\"", m_xmlTestcase)) {
                        line = "********* Start testing of " + m_xmlTestcase + " *********";

                    } else if (line.startsWith(QLatin1String("<Environment>"))) {
                        continue;

                    } else if (xmlLineStartsWith(line, "<TestFunction name=\"", m_xmlTestfunction)
                        || xmlLineStartsWith(line,"<QtVersion>",m_xmlQtVersion)
                        || xmlLineStartsWith(line,"<QTestVersion>",m_xmlQtestVersion)) {
                        continue;

                    } else if (line.startsWith(QLatin1String("<anonymous>()@"))) {
                        m_xmlFile = line.mid(line.indexOf(QLatin1Char('@'))+1);
                        m_xmlLine = m_xmlFile.mid(m_xmlFile.indexOf(QLatin1Char(':'))+1);
                        m_xmlFile = m_xmlFile.left(m_xmlFile.indexOf(QLatin1Char(':')));
                        if (!m_syntaxError.isEmpty()) {
                            addTestResult("CFAIL", QString(), m_syntaxError, m_xmlFile, m_xmlLine.toInt());
                            m_syntaxError.clear();
                        }
                    }
                }
            }

            // Filter out noise
            static QStringList filter = QStringList()
                << "unhandled event watchForKeyRelease"
                << "QIODevice::putChar"
                << "QFile::seek: IODevice is not open"
                << "QSqlQuery::exec"
                << "QSqlQuery::prepare"
                << "QLayout: Attempting"
                << "QTimeLine::start: already running"
                << "QAbstractSocket::waitForBytesWritten) is not allowed in UnconnectedState"
                << "QmemoryFile: No size not specified";

            int i = 0;
            while (i < filter.count()) {
                if (line.contains(filter[i])) {
                    line.clear();
                    break;
                }
                ++i;
            }
        }
        if (!line.isEmpty())
            testOutputPane()->append(line);
    }
}

void TestExecuter::onKillTestRequested()
{
    if (m_killTestRequested)
        stopTesting();
}

bool TestExecuter::exec(const QString &cmd, const QStringList &arguments, const QString &workDir, int timeout)
{
    if (!m_testCfg) {
        testOutputPane()->append(tr("No test configuration defined: exec aborted"));
        return false;
    }

    // Kill the process if it's still running
    if (m_executer.state() != QProcess::NotRunning) {
        m_executer.kill();
        m_executer.waitForFinished();
    }
    Q_ASSERT(m_executer.state() == QProcess::NotRunning);

    if ((!workDir.isEmpty()) && (m_executer.workingDirectory() != workDir)) {
        if (m_inBuildMode)
            testOutputPane()->append("cd " + workDir);
        m_executer.setWorkingDirectory(workDir);
    }
    m_executer.setEnvironment(*m_testCfg->buildEnvironment());

    if (m_inBuildMode)
        testOutputPane()->append(cmd + QLatin1Char(' ') + arguments.join(QString(QLatin1Char(' '))));

    m_executerFinished = false;
    if (arguments.count() > 0) {
        QString realCmd;
        if (!QDir::toNativeSeparators(cmd).contains(QDir::separator()))
            realCmd = QSystem::which(m_testCfg->PATH(), cmd);
        else if (QFile::exists(cmd))
            realCmd = cmd;

        if (realCmd.isEmpty()) {
            const QString path = QSystem::envKey(m_testCfg->buildEnvironment(), "PATH");
            testOutputPane()->append(tr("-- No '%1' instance found in PATH (%2) + ).").arg(cmd, path));
            return false;
        }
        m_executer.start(realCmd, arguments);
    } else {
        m_executer.start(cmd);
    }

    QTime time;
    time.start();
    bool ok = m_executer.waitForStarted(30000);
    m_executeTimer.start();
    if (ok) {
        while (!m_executerFinished && (m_executeTimer.elapsed() < timeout))
            qApp->processEvents();
    }

    if (ok && m_executeTimer.elapsed() < timeout) {
        return m_executer.exitCode() == 0;
    } else {
        testOutputPane()->append(QString(tr("-- %1 failed: %2, time elapsed: %3"))
            .arg(cmd, m_executer.errorString()).arg(time.elapsed()));
        return false;
    }
}

void TestExecuter::onExecuterFinished()
{
    m_executerFinished = true;
}

bool TestExecuter::runTestCase(bool forceManual)
{
    if (!m_testCfg) {
        testOutputPane()->append(tr("No test configuration defined: running test case aborted"));
        return false;
    }

    QString cmd;
    QStringList args;
    QStringList env = *m_testCfg->runEnvironment();
    m_xmlMode = true;
    uint timeout = 30000;

    if (m_curTestCode->testType() == TestCode::TypeSystemTest) {
        QString m_testPlatform;
        if (m_testCfg->uploadPlatform().toLower().contains(QLatin1String("windows")))
            m_testPlatform = QLatin1String("win");
        else if (m_testCfg->uploadPlatform().toLower().contains(QLatin1String("mac")))
            m_testPlatform = QLatin1String("mac");
        else if (m_testCfg->uploadPlatform().toLower().contains(QLatin1String("linux")))
            m_testPlatform = QLatin1String("linux");
        else
            m_testPlatform = QLatin1String("symbian");

        QSystem::setEnvKey(&env, QLatin1String("TESTPLATFORM"), m_testPlatform);
        args << QLatin1String("-env") << (QLatin1String("TESTPLATFORM=") + m_testPlatform);

        if (forceManual)
            args << QLatin1String("-force-manual");

        QString deviceName;
        QString deviceType;
        Utils::SshConnectionParameters sshParam(Utils::SshConnectionParameters::DefaultProxy);
        m_testCfg->isRemoteTarget(deviceName, deviceType, sshParam);

        QVariantMap connectionParam;
        connectionParam[QLatin1String("host")] = sshParam.host;
        connectionParam[QLatin1String("username")] = sshParam.userName;
        connectionParam[QLatin1String("sshPort")] = sshParam.port;
        connectionParam[QLatin1String("sshTimeout")] = sshParam.timeout;

        if (sshParam.authenticationType == Utils::SshConnectionParameters::AuthenticationByPassword) {
            connectionParam[QLatin1String("password")] = sshParam.password;
        } else {
            connectionParam[QLatin1String("privateKeyFile")] = sshParam.privateKeyFile;
        }

#ifndef QTTEST_PLUGIN_LEAN
        m_qscriptSystemTest.setConnectionParameters(deviceType, sshParam);
#else
        args << QLatin1String("-authost") << sshParam.host;
        args << QLatin1String("-username") << sshParam.userName;
        if (sshParam.authenticationType == Utils::SshConnectionParameters::AuthenticationByPassword) {
            args << QLatin1String("-pwd") << sshParam.password;
        } else {
            args << QLatin1String("-private-key") << sshParam.privateKeyFile;
        }
#endif

        timeout = 600000; // 10 minutes to run a manual test should be sufficient?
        cmd = QLatin1String("qtuitestrunner");
    } else {
        QFileInfo tmp(m_curTestCode->actualFileName());
        QString testRelPath = QDir::convertSeparators(tmp.absolutePath());
        testRelPath = testRelPath.remove(m_testCfg->srcPath()); // remove filename and srcPath part
        QString exePath;
        QString exeFile = m_curTestCode->execFileName();
        if (exeFile.isEmpty()) {
            testOutputPane()->append(tr("Unknown executable name for Test Case '%1'").arg(m_curTestCode->testCase()));
            addTestResult(QLatin1String("FAIL"), QString(),
                QString("Unknown executable name for Test Case '" + m_curTestCode->testCase() + QLatin1Char('\'')));
            return false;
        }

        QString exePath1 = m_testCfg->buildPath();
        exePath1 += testRelPath;

#ifdef Q_OS_MAC
        exePath1 += QLatin1Char('/') + tmp.baseName() + QLatin1String(".app/Contents/MacOS");
#endif
        exePath = QSystem::which(exePath1, exeFile);

        QString exePath2, exePath3, exePath4;
        if (exePath.isEmpty()) {
            exePath3 = m_testCfg->buildPath();
            exePath3 += QDir::separator() + QLatin1String("bin");
            exePath = QSystem::which(exePath3, exeFile);

            if (exePath.isEmpty()) {
                exePath4 = m_testCfg->buildPath();
                exePath4 += QString::fromLatin1("%1build%1tests%1bin").arg(QDir::separator());
                exePath = QSystem::which(exePath4, exeFile);

                if (exePath.isEmpty()) {
                    exePath2 = QString::fromLatin1("%1%2debug").arg(exePath1).arg(QDir::separator());
                    exePath = QSystem::which(exePath2, exeFile);
                }
            }
        }

        if (exePath.isEmpty()) {
            testOutputPane()->append(tr("Test Case '%1' not found").arg(exeFile));
            addTestResult(QLatin1String("FAIL"), QString(),
                QLatin1String("Test Case '") + exeFile + QLatin1String("' not found in:\n - '")  + exePath1
                + QLatin1String("' or \n - '")  + exePath2 + QLatin1String("' or \n - '")  + exePath3
                + QLatin1String("' or \n - '")  + exePath4 + QLatin1String("'."));
            return false;
        }

        cmd = exePath;
    }

    args << QLatin1String("-xml");

    if (m_testSettings.learnMode() == 1)
        args << QLatin1String("-learn");
    else if (m_testSettings.learnMode() == 2)
        args << QLatin1String("-learn-all");


    // Grab a list of all the functions we want to execute
    bool hasTests = false;
    foreach (const QString &item, m_selectedTests) {
        if (item.startsWith(m_curTestCode->testCase() + QLatin1String("::"))) {
            QString func = item.mid(item.indexOf(QLatin1String("::"))+2);
            if ((func != QLatin1String("init")) && (func != QLatin1String("initTestCase")) && (func != QLatin1String("cleanup"))
                && (func != QLatin1String("cleanupTestCase")) && !func.endsWith(QLatin1String("_data"))) {
                args << func;
                hasTests = true;
            }
        }
    }

    // If the user only selected init/cleanup functions skip to the next test
    if (!hasTests)
        return false;

    // If we have a postprocessing step, pipe output to a file
    if (!m_testCfg->postProcessScript().isEmpty())
        cmd.append(QLatin1String(" | tee ") + m_testOutputFile);

#ifndef Q_OS_WIN
    QString libPath = QSystem::envKey(m_testCfg->buildEnvironment(), QLatin1String("QTDIR"));
    if (!libPath.isEmpty()) {
        libPath = libPath + QDir::separator() + QLatin1String("lib");
# ifdef Q_OS_MAC
        QSystem::addEnvPath(m_testCfg->buildEnvironment(), QLatin1String("DYLD_LIBRARY_PATH"), libPath);
# else
        QSystem::addEnvPath(m_testCfg->buildEnvironment(), QLatin1String("LD_LIBRARY_PATH"), libPath);
# endif
    }
#endif

    bool cancel = false;
    if (m_curTestCode->testType() == TestCode::TypeSystemTest) {
        if (m_curTestCode->hasUnsavedChanges()) {
            QMessageBox msgBox;
            msgBox.setText(tr("Unsaved Changes"));
            msgBox.setInformativeText(tr("File '%1' has unsaved changes.\nThe file must be saved before proceeding.")
                .arg(m_curTestCode->baseFileName()));
            if (QMessageBox::warning(0, tr("Unsaved Changes"),
                tr("File \"%1\" has unsaved changes.\n"
                "The file must be saved before proceeding.").arg(m_curTestCode->baseFileName()),
                QMessageBox::Save | QMessageBox::Cancel, QMessageBox::Save) == QMessageBox::Save) {
                m_curTestCode->save();
            } else {
                cancel = true;
            }
        }
        if (!cancel) {
            QString testOutput;
            testOutputPane()->append(tr("********* Start testing of %1 *********")
                .arg(m_curTestCode->baseFileName()));

            args.prepend(m_curTestCode->actualFileName());
            testOutputPane()->append(QString::fromLatin1("%1 %2").arg(cmd).arg(args.join(QString(QLatin1Char(' ')))));
            if (exec(cmd, args, m_testCfg->buildPath(), timeout))
                return true;
            testOutputPane()->append(tr("********* Finished testing of %1 *********")
                .arg(m_curTestCode->baseFileName()));

#ifndef QTTEST_PLUGIN_LEAN
            m_qscriptSystemTest.runTest(m_curTestCode->actualFileName(), args, env, &testOutput);
            if (qscript_system_test.isAborted()) {
                testOutputPane()->append(tr("********* Aborted testing of %1 *********")
                    .arg(m_curTestCode->baseFileName()));
                if (!m_manualStop)
                    manualStop();
            } else {
                testOutputPane()->append(tr("********* Finished testing of %1 *********")
                    .arg(m_curTestCode->baseFileName()));
            }
#endif

            m_xmlLog = testOutput.split(QLatin1Char('\n'));
            m_xmlLog.removeAll(QString("<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>"));
        } else {
            addTestResult("FAIL", m_curTestCode->testCase(),
                QString::fromLatin1("Execution cancelled for Test Case '%1'").arg(m_curTestCode->testCase()));
            return false;
        }
    } else {
        // Start the cmd line process
        if (exec(cmd, args, m_testCfg->buildPath(), timeout))
            return true;
    }

    // special case. If test cases fail, the test application (Unit test) will return with exit code 1,
    // but this doesn't mean the execution failed, so we still return positive (true).
    if (m_executer.exitCode() >= 0)
        return true;

    addTestResult(QLatin1String("FAIL"), m_curTestCode->testCase(),
        QString::fromLatin1("Execution failed for Test Case '%1' with exit code '%2': %3")
        .arg(m_curTestCode->testCase() + QLatin1Char('\''))
        .arg(m_executer.exitCode())
        .arg(m_executer.errorString()));
    return false;
}

bool TestExecuter::testFailedUnexpectedly()
{
    return m_testFailedUnexpectedly;
}

void TestExecuter::addTestResult(const QString &result, const QString &test, const QString &reason,
    const QString &file, int line, const QString &dataTag)
{
    if (result.contains(QLatin1String("FAIL"))) {
        if (!result.contains(QLatin1String("XFAIL")))
            m_testFailedUnexpectedly = true;
        ++m_progressFailCount;
    } else if (result.contains(QLatin1String("PASS"))) {
        if (reason.contains(QLatin1String("::initTestCase()")) || reason.contains(QLatin1String("::cleanupTestCase()")))
            return;
        ++m_progressPassCount;
    }

    QString fn = file;
    if (file.isEmpty())
        fn = m_curTestCode->actualFileName();

    // In some cases, the testcase filename might be a symbolic link from the
    // build directory to the depot. In this case, follow the link.
    {
        QFileInfo info(fn);
        if (info.exists())
            fn = info.canonicalFilePath();
    }
    TestResultsWindow::instance()->addResult(result, test, reason, dataTag, fn, line);
}

bool TestExecuter::getNextTest()
{
    QString curTestName;
    if (m_curTestCode)
        curTestName = m_curTestCode->testCase();

    if (m_selectedTests.count() > 0) {
        if (curTestName.isEmpty()) {
            curTestName = m_selectedTests[0].left(m_selectedTests[0].indexOf(QLatin1String("::")));
            if (!curTestName.isEmpty()) {
                m_curTestCode = m_testCollection.findCodeByTestCaseName(curTestName);
                return (m_curTestCode != 0);
            }
        }

        bool tcFound = false;
        QString tcName = curTestName + QLatin1String("::");
        foreach (const QString &item, m_selectedTests) {
            if (!tcFound) {
                if (item.startsWith(tcName))
                    tcFound = true;
            } else {
                if (!item.startsWith(tcName)) {
                    curTestName = item.left(item.indexOf(QLatin1String("::")));
                    if (!curTestName.isEmpty()) {
                        m_curTestCode = m_testCollection.findCodeByTestCaseName(curTestName);
                        return m_curTestCode != 0;
                    }
                }
            }
        }
    }
    return false;
}

bool TestExecuter::testBusy() const
{
    return m_testBusy;
}

QString TestExecuter::saveResults(const QString &changeNo, const QString &platform, const QString &branch)
{
    QString fname = QDir::homePath() + QDir::separator() + QLatin1String(".qttest")
        + QDir::separator() + QLatin1String("pending_test_results");
    QDir().mkpath(fname);

    fname += QDir::separator();
    fname += QLatin1String("creator_upload_USER_");
    fname += sanitizedForFilename(QSystem::userName());
    fname += QLatin1String("_HOST_");
    fname += sanitizedForFilename(QSystem::hostName());
    fname += QLatin1String("_ON_");
    fname += sanitizedForFilename(QDateTime::currentDateTime().toString());
    fname += QLatin1String(".xml");

    QFile xmlFile(fname);
    xmlFile.remove();

    if (m_xmlLog.count() > 0) {
        if (xmlFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
            QTextStream xml_stream(&xmlFile);
            xml_stream << testrHeader(changeNo, platform, branch);
            xml_stream << '\n';
            xml_stream << QString("\n<!-- file 1 of 1: filename -->\n");
            xml_stream << m_xmlLog.join(QString(QLatin1Char('\n')));
            xml_stream << '\n';
            xml_stream << testrFooter();
            xmlFile.close();
            return fname;
        }
    }
    return QString();
}

QString TestExecuter::sanitizedForFilename(const QString &in) const
{
    QString out = in;
    static const QRegExp replaceRx(QLatin1String("[^a-zA-Z0-9\\-_\\.]"));
    out.replace(replaceRx, QString(QLatin1Char('_')));
    return out;
}

QByteArray TestExecuter::testrHeader(const QString &changeNo,
    const QString &platform, const QString &branch) const
{
    QByteArray ret;
    ret += "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n";
    ret += "<Testrun>\n";
    ret += "<Environment>\n";
    ret += "  <HostName>fake_host</HostName>\n";
    ret += "  <MakeSpec>" + platform.toLatin1() + "</MakeSpec>\n";
    ret += "  <ChangeNumber>" + changeNo.toLatin1() + "</ChangeNumber>\n";
    ret += "  <Branch>" + branch.toLatin1() + "</Branch>\n";
    ret += "</Environment>\n";

    QDateTime dt = QDateTime::currentDateTime();
    ret += "<Timestamp date=\"" + dt.date().toString("yyyy-MM-dd").toLatin1()
        + "\" time=\"" + dt.time().toString("hh:mm:ss").toLatin1() + "\"/>\n";

    return ret;
}

QByteArray TestExecuter::testrFooter() const
{
    return QByteArray("</Testrun>\n");
}

#ifdef QTTEST_DEBUGGER_SUPPORT
void TestExecuter::setDebugEngine(Debugger::Internal::QtUiTestEngine* engine)
{
    if (m_debugEngine) {
        QObject::disconnect(this, SIGNAL(testFinished()),
            m_debugEngine, SLOT(handleTestRunnerFinished()));
        QObject::disconnect(engine, SIGNAL(stopTesting()), this, SLOT(manualStop()));
    }
    m_debugEngine = engine;
    if (!m_debugEngine)
        return;

    QObject::connect(this, SIGNAL(testFinished()),
        engine, SLOT(handleTestRunnerFinished()), Qt::DirectConnection);
    QObject::connect(engine, SIGNAL(stopTesting()),
        this, SLOT(manualStop()), Qt::DirectConnection);
}

Debugger::Internal::QtUiTestEngine* TestExecuter::debugEngine() const
{
    return m_debugEngine;
}
#endif

void TestExecuter::syntaxError(const QString &msg, const QString &file, int line)
{
    TestCode *tmp = m_testCollection.findCode(file, QString(), QString());
    if (tmp) {
        tmp->openTestInEditor(QString());
        tmp->gotoLine(line);
    }
    QFileInfo fi(file);
    const QString message = tr("Syntax error in %1, near line %2.\n%3").arg(fi.fileName()).arg(line).arg(msg);
    QMessageBox::critical(0, tr("System Test Failure"), message);
}

void TestExecuter::onTestResult(const QVariantMap &result)
{
    bool ok = false;
    int line = result["LINE"].toInt(&ok);
    if (!ok)
        line = -1;

    QString test = result["TEST"].toString();
    QString reason = result["REASON"].toString();
    QString file = result["FILE"].toString();

    if (line == -1) {
        QString funcName = test.mid(test.indexOf("::")+2);
        TestFunctionInfo *funcInfo = m_curTestCode->findFunction(funcName);
        if (funcInfo) {
            file = m_curTestCode->actualFileName();
            line = funcInfo->testStartLine()+1;
        }
    }

    addTestResult(result["RESULT"].toString(), test, reason, file, line, result["DATATAG"].toString());

    if (!testStopped())
        m_progressLabel->setText(tr("%1 failed\n%2 passed")
            .arg(m_progressFailCount).arg(m_progressPassCount));

    if (test != m_lastFinishedTest)
        m_progressBar->setProgressValue(m_progress++);

    m_lastFinishedTest = test;
}

void TestExecuter::eventRecordingStarted(const QString &file, int line, const QString &manualSteps)
{
    TestCode *tmp = m_testCollection.findCode(file, QString(), QString());
    if (tmp) {
        tmp->openTestInEditor(QString());
        tmp->gotoLine(line);
    }
    QPointer<QDialog> recordWindow = new QDialog;
    Ui::RecordDialog ui;
    ui.setupUi(recordWindow);

    if (!manualSteps.isEmpty()) {
        ui.steps_view->setPlainText(manualSteps);
    } else {
        ui.steps_view->hide();
        ui.steps_label->hide();
    }

    ui.label->setText("<h2>Events are now being recorded</h2>Select 'Stop' when finished, "
        "or 'Abort' to abandon recording.");
    m_recordedEventsEdit = ui.codeEdit;

    connect(ui.abort_button, SIGNAL(clicked()), recordWindow, SLOT(close()));
    connect(ui.abort_button, SIGNAL(clicked()), this, SLOT(abortRecording()));
    m_abortRecording = false;

    m_recordingEvents = true;
    recordWindow->exec();
    delete recordWindow;
    m_recordingEvents = false;
    if (m_abortRecording)
        return;

    m_pendingInsertions[qMakePair(file, line)] = m_recordedEventsEdit->toPlainText();
}

void TestExecuter::applyPendingInsertions()
{
    QList< QPair<QString, int> > keys = m_pendingInsertions.keys();
    qSort(keys.begin(), keys.end());

    for (int i = keys.size()-1; i >= 0; --i) {
        QPair<QString, int> location = keys[i];
        QString recordedCode = m_pendingInsertions[location];

        TestCode *tmp = m_testCollection.findCode(location.first, QString(), QString());
        if (!tmp)
            continue;

        tmp->openTestInEditor(QString());
        tmp->gotoLine(location.second);

        TextEditor::BaseTextEditor *editable = qobject_cast<TextEditor::BaseTextEditor *>(tmp->editor());
        if (editable) {
            QString insertCode = "//BEGIN Recorded Events";
            int column = editable->currentColumn();
            QStringList recordedLines = recordedCode.split(QLatin1Char('\n'));
            QString indent(QString(' ').repeated(column-1));
            indent.prepend(QLatin1Char('\n'));
            foreach (const QString &line, recordedLines) {
                insertCode.append(indent);
                insertCode.append(line);
            }
            insertCode.append("//END Recorded Events");

            int lineLength = editable->position(TextEditor::ITextEditor::EndOfLine)
                - editable->position();
            editable->replace(lineLength, insertCode);
        }
    }
}

void TestExecuter::recordedCode(const QString &code)
{
    if (m_recordedEventsEdit) {
        m_recordedEventsEdit->setPlainText(code);
        m_recordedEventsEdit->moveCursor(QTextCursor::End);
        m_recordedEventsEdit->ensureCursorVisible();
    }
}

bool TestExecuter::mustStopEventRecording() const
{
    return !m_recordingEvents;
}

void TestExecuter::abortRecording()
{
    m_abortRecording = true;
}

bool TestExecuter::eventRecordingAborted() const
{
    return m_abortRecording;
}

void TestExecuter::setSelectedTests(const QStringList &tests)
{
    m_selectedTests = tests;
}

QString TestExecuter::currentRunningTest() const
{
    QString ret;
    if (m_curTestCode)
        ret = m_curTestCode->actualFileName();
    return ret;
}
