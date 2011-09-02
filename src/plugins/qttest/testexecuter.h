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

#ifndef TESTEXECUTER_H
#define TESTEXECUTER_H

#include "testsettings.h"
#include "testcode.h"
#include "testresultuploader.h"

#ifndef QTTEST_PLUGIN_LEAN
# include "qscriptsystemtest.h"
#endif

#ifdef QTTEST_DEBUGGER_SUPPORT
# include <debugger/qtuitest/qtuitestengine.h>
#endif

#include <QtCore/QFutureWatcher>
#include <QProcess>
#include <QVariantMap>

#define NO_X_SERVER_AVAILABLE "No X-server available for testing"
#define COMPILE_ERROR "Compile error"
#define COMPILE_SUCCESS "Compile successful"
#define COMPILE_NOT_AVAIL "Binary not available for testing"
#define COMPILE_UP_TO_DATE "Not recompiled, binary is up-to-date"
#define SELF_TEST "self-test"

QT_BEGIN_NAMESPACE
class QTextEdit;
class QLabel;
QT_END_NAMESPACE

class TestExecuter : public QObject
{
    Q_OBJECT
private:
    static TestExecuter *m_instance;

public:
    TestExecuter();
    virtual ~TestExecuter();

    static TestExecuter *instance();

    bool testBusy() const;
    bool testFailedUnexpectedly();
    bool testStopped() const { return m_manualStop; }

#ifdef QTTEST_DEBUGGER_SUPPORT
    void setDebugEngine(Debugger::Internal::QtUiTestEngine *);
    Debugger::Internal::QtUiTestEngine *debugEngine() const;
#endif

    void eventRecordingStarted(const QString &file, int line, const QString &steps);
    void recordedCode(const QString &code);
    bool mustStopEventRecording() const;
    bool eventRecordingAborted() const;
    void setSelectedTests(const QStringList &);
    QString currentRunningTest() const;

public slots:
    void runTests(bool singleTest, bool forceManual);
    void runSelectedTests(bool forceManual);
    void stopTesting();
    void manualStop();
    void syntaxError(const QString &, const QString &, int);
    void onTestResult(const QVariantMap &);

signals:
    void testStarted();
    void testStop();
    void testFinished();

private slots:
    bool buildTestCase();
    bool runTestCase(bool forceManual);
    bool postProcess();
    void parseOutput();
    void onKillTestRequested();
    void onExecuterFinished();
    void abortRecording();

private:
    // FIXME. These functions are 'borrowed' from storetestresults which isn't the smartest thing to do
    QString sanitizedForFilename(const QString &in) const;
    QString saveResults(const QString &changeNo, const QString &platform, const QString &branch);
    QByteArray testrHeader(const QString &changeNo, const QString &platform, const QString &branch) const;
    QByteArray testrFooter() const;

    void endTest();
    bool getNextTest();
    QWidget *createProgressWidget();

    bool exec(const QString &cmd, const QStringList &arguments, const QString &workDir = "", int timeout = 30000);

    void addTestResult(const QString &result, const QString &test, const QString &reason,
        const QString &file = QString(), int line = -1, const QString &dataTag = QString());

    void applyPendingInsertions();

    TestResultUploader m_testResultUploader;
    TestCollection m_testCollection;
    TestSettings m_testSettings;
    TestConfig *m_testCfg;
    QProcess m_executer;
    bool m_executerFinished;
    QTime m_executeTimer;

    bool m_testFailedUnexpectedly;
    QStringList m_selectedTests;
    QStringList m_failureLines;
    TestCode *m_curTestCode;
    QString m_testOutputFile;
    QString m_syntaxError;

#ifndef QTTEST_PLUGIN_LEAN
    QScriptSystemTest m_qscriptSystemTest;
#endif

    bool m_testBusy;
    bool m_stopTesting;
    bool m_manualStop;
    bool m_runRequired;
    bool m_pendingFailure;
    bool m_killTestRequested;
    bool m_inBuildMode;
    QStringList m_xmlLog;
    bool m_xmlMode;
    QString m_xmlTestcase;
    QString m_xmlTestfunction;
    QString m_xmlDatatag;
    QString m_xmlFile;
    QString m_xmlLine;
    QString m_xmlResult;
    QString m_xmlDescription;
    QString m_xmlQtVersion;
    QString m_xmlQtestVersion;

    // functionality to parse (system test) xml results recorded in a file
    QTextStream *m_testResultsStream;
    bool canReadLine();
    QString readLine();
    QStringList peek();
    QString m_peekedResult;

    // Progress reporting to the progress manager
    QFutureInterface<void> *m_progressBar;
    QPointer<QLabel> m_progressLabel;
    int m_progress;
    int m_progressFailCount;
    int m_progressPassCount;

#ifdef QTTEST_DEBUGGER_SUPPORT
    Debugger::Internal::QtUiTestEngine *m_debugEngine;
#endif
    QPointer<QTextEdit> m_recordedEventsEdit;
    bool m_recordingEvents;
    bool m_abortRecording;
    QMap<QPair<QString, int>, QString> m_pendingInsertions;
    QString m_lastFinishedTest;
};

#endif
