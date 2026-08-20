// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// Protocol checks for the debugger bridge (bridge.py). The bridge runs inside
// gdb, so the checks live in bridgedriver.py, which installs a fake gdb module
// and drives the server over a pipe; this test runs them one per row and
// reports what the driver said. No gdb and no inferior are involved.

#include <QProcess>
#include <QStandardPaths>
#include <QTest>

// The driver reports a platform it cannot run on with this exit code; the
// output cannot say it, because stderr is merged into it and whatever the
// bridge warns about would come first.

static const int skipExitCode = 3;

class tst_Bridge : public QObject
{
    Q_OBJECT

private slots:
    void protocol_data();
    void protocol();
};

// Returns true if the given path is a working Python interpreter. On Windows,
// QStandardPaths::findExecutable() may return the Microsoft Store "App
// execution alias" stub, which only prints a message pointing at the Store.
static bool isWorkingPython(const QString &python)
{
    QProcess probe;
    probe.setProcessChannelMode(QProcess::MergedChannels);
    probe.start(python, {"--version"});
    if (!probe.waitForStarted() || !probe.waitForFinished(10000))
        return false;
    if (probe.exitStatus() != QProcess::NormalExit || probe.exitCode() != 0)
        return false;
    const QString version = QString::fromLocal8Bit(probe.readAll()).trimmed();
    return version.startsWith("Python ") && version.length() > 7
           && version.at(7).isDigit();
}

static QString findWorkingPython()
{
    QStringList candidates;
    const QString pythonRoot = qEnvironmentVariable("PYTHON3_PATH");
    if (!pythonRoot.isEmpty()) {
        candidates << QStandardPaths::findExecutable("python3", {pythonRoot})
                   << QStandardPaths::findExecutable("python", {pythonRoot});
    }
    candidates << QStandardPaths::findExecutable("python3")
               << QStandardPaths::findExecutable("python");

    for (const QString &candidate : std::as_const(candidates)) {
        if (!candidate.isEmpty() && isWorkingPython(candidate))
            return candidate;
    }
    return {};
}

void tst_Bridge::protocol_data()
{
    QTest::addColumn<QString>("check");

    // One row per behavior the bridge has to keep. Each of these went wrong
    // once; bridgedriver.py describes what each one pins down.
    QTest::newRow("framing") << "framing";
    QTest::newRow("initialize reports dumpers") << "initialize-reports-dumpers";
    QTest::newRow("extra dumpers are loaded") << "extra-dumpers-are-loaded";
    QTest::newRow("launch passes cwd and environment")
        << "launch-passes-cwd-and-environment";
    QTest::newRow("stdout cannot corrupt the protocol")
        << "stdout-cannot-corrupt-the-protocol";
    QTest::newRow("server owns the stop events") << "server-owns-the-stop-events";
    QTest::newRow("attach failure is reported") << "attach-failure-is-reported";
    QTest::newRow("temporary breakpoint stop") << "temporary-breakpoint-stop";
    QTest::newRow("shutdown quits gdb") << "shutdown-quits-gdb";
    QTest::newRow("breakpoint source with spaces") << "breakpoint-source-with-spaces";
    QTest::newRow("catchpoints are created") << "catchpoints-are-created";
    QTest::newRow("a catchpoint keeps its settings")
        << "a-catchpoint-keeps-its-settings";
    QTest::newRow("a catchpoint reports its catch type")
        << "a-catchpoint-reports-its-catch-type";
    QTest::newRow("a breakpoint that cannot be reported is not left behind")
        << "a-breakpoint-that-cannot-be-reported-is-not-left-behind";
    QTest::newRow("target configuration reaches gdb")
        << "target-configuration-reaches-gdb";
    QTest::newRow("windows paths survive the payload")
        << "windows-paths-survive-the-payload";
    QTest::newRow("watchpoint is not asked for locations")
        << "watchpoint-is-not-asked-for-locations";
    QTest::newRow("interrupt does not end the session")
        << "interrupt-does-not-end-the-session";
    QTest::newRow("failed breakpoint request carries the modelid")
        << "failed-breakpoint-request-carries-the-modelid";
    QTest::newRow("moving a breakpoint recreates it")
        << "moving-a-breakpoint-recreates-it";
    QTest::newRow("inferior output event") << "inferior-output-event";
    QTest::newRow("a resuming console command reports the stop")
        << "a-resuming-console-command-reports-the-stop";
    QTest::newRow("a failing console command answers with an error")
        << "a-failing-console-command-answers-with-an-error";
    QTest::newRow("one module is not reported twice")
        << "one-module-is-not-reported-twice";
    QTest::newRow("a newline cannot smuggle a gdb command")
        << "a-newline-cannot-smuggle-a-gdb-command";
    QTest::newRow("an unusable program fails the launch")
        << "an-unusable-program-fails-the-launch";
    QTest::newRow("a rejected argument fails the launch")
        << "a-rejected-argument-fails-the-launch";
    QTest::newRow("a loaded library is reported") << "a-loaded-library-is-reported";
    QTest::newRow("startup commands reach gdb") << "startup-commands-reach-gdb";
    QTest::newRow("data requests use the dumpers") << "data-requests-use-the-dumpers";
}

void tst_Bridge::protocol()
{
    QFETCH(QString, check);

    const QString python = findWorkingPython();
    if (python.isEmpty())
        QSKIP("No working Python interpreter found in PYTHON3_PATH or PATH.");

    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("DUMPERDIR", QString::fromLocal8Bit(DUMPERDIR));
    process.setProcessEnvironment(env);

    process.start(python, {QString::fromLocal8Bit(BRIDGE_DRIVER), check});
    QVERIFY2(process.waitForStarted(), qPrintable(process.errorString()));
    QVERIFY2(process.waitForFinished(60000), "The bridge driver timed out.");

    const QByteArray output = process.readAll();
    QVERIFY2(process.exitStatus() == QProcess::NormalExit, output.constData());
    if (process.exitCode() == skipExitCode)
        QSKIP(output.constData());
    QVERIFY2(process.exitCode() == 0, output.constData());
}

QTEST_GUILESS_MAIN(tst_Bridge)

#include "tst_bridge.moc"
