/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com <http://qt.digia.com/>
**
** This file is part of the Qt Enterprise LicenseChecker Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com <http://qt.digia.com/>
**
****************************************************************************/

#include <clangstaticanalyzer/clangstaticanalyzerconstants.h>
#include <clangstaticanalyzer/clangstaticanalyzerrunner.h>

#include <QtTest>

using namespace ClangStaticAnalyzer::Internal;

class ClangStaticAnalyzerRunnerTest : public QObject
{
    Q_OBJECT

public:
    ClangStaticAnalyzerRunnerTest() {}
    virtual ~ClangStaticAnalyzerRunnerTest() {}

private slots:
    void runWithTestCodeGeneratedOneIssue();
    void runWithNonExistentFileToAnalyze();
};

static bool writeFile(const QString &filePath, const QByteArray &source)
{
    Utils::FileSaver saver(filePath);
    return saver.write(source) && saver.finalize();
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
    ClangStaticAnalyzerRunner runner(QLatin1String("clang"), temporaryDir.path());

    ClangStaticAnalyzerRunnerSignalTester st(&runner);
    QVERIFY(runner.run(testFilePath));
    QVERIFY(st.expectStartedSignal());
    QVERIFY(st.expectFinishWithSuccessSignal());
}

void ClangStaticAnalyzerRunnerTest::runWithNonExistentFileToAnalyze()
{
    QTemporaryDir temporaryDir(QDir::tempPath() + QLatin1String("/qtc-clangstaticanalyzer-XXXXXX"));
    QVERIFY(temporaryDir.isValid());
    ClangStaticAnalyzerRunner runner(QLatin1String("clang"), temporaryDir.path());

    ClangStaticAnalyzerRunnerSignalTester st(&runner);
    QVERIFY(runner.run(QLatin1String("not.existing.file.111")));

    QVERIFY(st.expectStartedSignal());
    QVERIFY(st.expectFinishWithFailureSignal(finishedWithBadExitCode(1)));
}

QTEST_MAIN(ClangStaticAnalyzerRunnerTest)

#include "tst_clangstaticanalyzerrunner.moc"
