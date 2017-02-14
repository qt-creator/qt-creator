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

#include <clangstaticanalyzer/clangstaticanalyzerconstants.h>
#include <clangstaticanalyzer/clangstaticanalyzerrunner.h>

#include <utils/hostosinfo.h>
#include <utils/temporarydirectory.h>

#include <QtTest>

using namespace ClangStaticAnalyzer::Internal;

static QString clangExecutablePath()
{
    const QString clangFileName = Utils::HostOsInfo::withExecutableSuffix(QStringLiteral("clang"));
    const Utils::Environment environment = Utils::Environment::systemEnvironment();

    return environment.searchInPath(clangFileName).toString();
}

static bool writeFile(const QString &filePath, const QByteArray &source)
{
    Utils::FileSaver saver(filePath);
    return saver.write(source) && saver.finalize();
}

static bool waitUntilSignalCounterIsGreatorThanZero(int &signalCounter, int timeOutInMs = 5000)
{
    QElapsedTimer timer;
    timer.start();

    while (timer.elapsed() <= timeOutInMs) {
        QCoreApplication::processEvents();

        if (signalCounter != 0) {
            if (signalCounter == 1)
                return true;

            qDebug() << "signalCounter:" << signalCounter;
            return false;
        }

        QThread::msleep(50);
    }

    qDebug() << "signalCounter:" << signalCounter;
    return false;
}

class ClangStaticAnalyzerRunnerTest : public QObject
{
    Q_OBJECT

public:
    ClangStaticAnalyzerRunnerTest();
    virtual ~ClangStaticAnalyzerRunnerTest() {}

private slots:
    void runWithTestCodeGeneratedOneIssue();
    void runWithNonExistentFileToAnalyze();

private:
    QString m_clangExecutable;
};

ClangStaticAnalyzerRunnerTest::ClangStaticAnalyzerRunnerTest()
    : m_clangExecutable(clangExecutablePath())
{
}

class ClangStaticAnalyzerRunnerSignalTester
{
public:
    ClangStaticAnalyzerRunnerSignalTester(ClangStaticAnalyzerRunner *runner);

    bool expectStartedSignal();
    bool expectFinishWithSuccessSignal();
    bool expectFinishWithFailureSignal(const QString &expectedErrorMessage = QString());

private:
    int m_startedSignalEmitted;
    int m_finishedWithSuccessEmitted;
    int m_finishedWithFailureEmitted;
    QString m_finishedWithFailureErrorMessage;
};

ClangStaticAnalyzerRunnerSignalTester::ClangStaticAnalyzerRunnerSignalTester(
        ClangStaticAnalyzerRunner *runner)
    : m_startedSignalEmitted(0)
    , m_finishedWithSuccessEmitted(0)
    , m_finishedWithFailureEmitted(0)
{
    QObject::connect(runner, &ClangStaticAnalyzerRunner::started, [this] {
        ++m_startedSignalEmitted;
    });

    QObject::connect(runner, &ClangStaticAnalyzerRunner::finishedWithSuccess, [this] {
        ++m_finishedWithSuccessEmitted;
    });

    QObject::connect(runner,
                     &ClangStaticAnalyzerRunner::finishedWithFailure,
                     [this] (const QString &errorMessage, const QString &) {
        ++m_finishedWithFailureEmitted;
        m_finishedWithFailureErrorMessage = errorMessage;
    });
}

bool ClangStaticAnalyzerRunnerSignalTester::expectStartedSignal()
{
    return waitUntilSignalCounterIsGreatorThanZero(m_startedSignalEmitted);
}

bool ClangStaticAnalyzerRunnerSignalTester::expectFinishWithSuccessSignal()
{
    return waitUntilSignalCounterIsGreatorThanZero(m_finishedWithSuccessEmitted);
}

bool ClangStaticAnalyzerRunnerSignalTester::expectFinishWithFailureSignal(
        const QString &expectedErrorMessage)
{
    if (waitUntilSignalCounterIsGreatorThanZero(m_finishedWithFailureEmitted)) {
        if (m_finishedWithFailureErrorMessage == expectedErrorMessage) {
            return true;
        } else {
            qDebug() << "Actual error message:" << m_finishedWithFailureErrorMessage;
            qDebug() << "Expected error message:" << expectedErrorMessage;
            return false;
        }
    }

    return false;
}

void ClangStaticAnalyzerRunnerTest::runWithTestCodeGeneratedOneIssue()
{
    if (m_clangExecutable.isEmpty())
        QSKIP("Clang executable in PATH required.");

    const QString testFilePath = QDir::tempPath() + QLatin1String("/testcode.cpp");
    const QByteArray source =
            "void f(int *p) {}\n"
            "void f2(int *p) {\n"
            "    delete p;\n"
            "    f(p); // warn: use after free\n"
            "}\n";
    QVERIFY(writeFile(testFilePath, source));

    Utils::TemporaryDirectory temporaryDir("runWithTestCodeGeneratedOneIssue");
    ClangStaticAnalyzerRunner runner(m_clangExecutable, temporaryDir.path(),
                                     Utils::Environment::systemEnvironment());

    ClangStaticAnalyzerRunnerSignalTester st(&runner);
    QVERIFY(runner.run(testFilePath));

    QVERIFY(st.expectStartedSignal());
    QVERIFY(st.expectFinishWithSuccessSignal());
}

void ClangStaticAnalyzerRunnerTest::runWithNonExistentFileToAnalyze()
{
    if (m_clangExecutable.isEmpty())
        QSKIP("Clang executable in PATH required.");

    Utils::TemporaryDirectory temporaryDir("runWithNonExistentFileToAnalyze");
    ClangStaticAnalyzerRunner runner(m_clangExecutable, temporaryDir.path(),
                                     Utils::Environment::systemEnvironment());

    ClangStaticAnalyzerRunnerSignalTester st(&runner);
    QVERIFY(runner.run(QLatin1String("not.existing.file.111")));

    QVERIFY(st.expectStartedSignal());
    QVERIFY(st.expectFinishWithFailureSignal(finishedWithBadExitCode(1)));
}

int main(int argc, char *argv[])
{
    Utils::TemporaryDirectory::setMasterTemporaryDirectory(
                QDir::tempPath() + "/qtc-clangstaticanalyzer-test-XXXXXX");

    QCoreApplication app(argc, argv);
    ClangStaticAnalyzerRunnerTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_clangstaticanalyzerrunner.moc"
