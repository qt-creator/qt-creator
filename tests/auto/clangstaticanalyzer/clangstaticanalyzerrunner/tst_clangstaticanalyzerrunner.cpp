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

#include <QtTest>

using namespace ClangStaticAnalyzer::Internal;

namespace {

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

} // anonymous namespace

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
    QSignalSpy m_spyStarted;
    QSignalSpy m_spyFinishedWithFailure;
    QSignalSpy m_spyFinishedWithSuccess;
};

ClangStaticAnalyzerRunnerSignalTester::ClangStaticAnalyzerRunnerSignalTester(
        ClangStaticAnalyzerRunner *runner)
    : m_spyStarted(runner, SIGNAL(started()))
    , m_spyFinishedWithFailure(runner, SIGNAL(finishedWithFailure(QString,QString)))
    , m_spyFinishedWithSuccess(runner, SIGNAL(finishedWithSuccess(QString)))
{
}

bool ClangStaticAnalyzerRunnerSignalTester::expectStartedSignal()
{
    if (m_spyStarted.wait()) {
        if (m_spyStarted.size() != 1) {
            qDebug() << "started() emitted more than once.";
            return false;
        }
    } else {
        qDebug() << "started() not emitted.";
        return false;
    }

    return true;
}

bool ClangStaticAnalyzerRunnerSignalTester::expectFinishWithSuccessSignal()
{
    if (m_spyFinishedWithSuccess.wait()) {
        if (m_spyFinishedWithSuccess.size() != 1) {
            qDebug() << "finishedWithSuccess() emitted more than once.";
            return false;
        }
    } else {
        qDebug() << "finishedWithSuccess() not emitted.";
        return false;
    }

    if (m_spyFinishedWithFailure.size() != 0) {
        qDebug() << "finishedWithFailure() emitted.";
        return false;
    }

    return true;
}

bool ClangStaticAnalyzerRunnerSignalTester::expectFinishWithFailureSignal(
        const QString &expectedErrorMessage)
{
    if (m_spyFinishedWithFailure.wait()) {
        if (m_spyFinishedWithFailure.size() == 1) {
            const QList<QVariant> args = m_spyFinishedWithFailure.at(0);
            const QString errorMessage = args.at(0).toString();
            if (errorMessage == expectedErrorMessage) {
                if (m_spyFinishedWithSuccess.size() != 0) {
                    qDebug() << "Got expected error, but finishedWithSuccess() was also emitted.";
                    return false;
                }
                return true;
            } else {
                qDebug() << "Actual error message:" << errorMessage;
                qDebug() << "Expected error message:" << expectedErrorMessage;
                return false;
            }
        } else {
            qDebug() << "Finished with more than one failure (expected only one).";
            return false;
        }
    } else {
        qDebug() << "Not finished with failure, but expected.";
        return false;
    }
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

    QTemporaryDir temporaryDir(QDir::tempPath() + QLatin1String("/qtc-clangstaticanalyzer-XXXXXX"));
    QVERIFY(temporaryDir.isValid());
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

    QTemporaryDir temporaryDir(QDir::tempPath() + QLatin1String("/qtc-clangstaticanalyzer-XXXXXX"));
    QVERIFY(temporaryDir.isValid());
    ClangStaticAnalyzerRunner runner(m_clangExecutable, temporaryDir.path(),
                                     Utils::Environment::systemEnvironment());

    ClangStaticAnalyzerRunnerSignalTester st(&runner);
    QVERIFY(runner.run(QLatin1String("not.existing.file.111")));

    QVERIFY(st.expectStartedSignal());
    QVERIFY(st.expectFinishWithFailureSignal(finishedWithBadExitCode(1)));
}

QTEST_MAIN(ClangStaticAnalyzerRunnerTest)

#include "tst_clangstaticanalyzerrunner.moc"
