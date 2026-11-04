// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <utils/commandline.h>
#include <utils/filepath.h>
#include <utils/processenums.h>
#include <utils/processinterface.h>
#include <utils/qtcprocess.h>
#include <utils/result.h>
#include <utils/terminalhooks.h>
#include <utils/terminalinterface.h>

#include <QLocalServer>
#include <QLocalSocket>
#include <QObject>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>

#ifdef Q_OS_UNIX
#include <signal.h>
#endif

using namespace Utils;

// A fake terminal backend: instead of launching the real process stub in an
// external terminal, it connects to TerminalInterface's stub socket and either
// writes a canned stub-protocol line or drops the connection. That lets us
// exercise TerminalInterface's error reporting headlessly, without a terminal
// emulator or the process-stub binary. Guards QTCREATORBUG-12263 (a failing
// terminal must be reported specifically, not as a misleading catch-all).
class FakeStubCreator : public StubCreator
{
public:
    QByteArray reply;                       // e.g. "err:chdir 2\n"
    bool disconnectWithoutInferior = false; // simulate stub dying before the inferior runs

    Result<qint64> startStubProcess(const ProcessSetupData &setup) override
    {
        // The stub server socket is passed to the (real) stub via "-s <socket>".
        const QStringList args = setup.m_commandLine.splitArguments();
        const int idx = args.indexOf("-s");
        if (idx < 0 || idx + 1 >= args.size())
            return ResultError(QString("stub command line has no -s <socket>"));

        auto *socket = new QLocalSocket(this);
        socket->connectToServer(args.at(idx + 1));
        if (!socket->waitForConnected(5000)) {
            return ResultError(
                QString("could not connect to stub socket: %1").arg(socket->errorString()));
        }

        if (disconnectWithoutInferior) {
            // Delay so the server-side onNewStubConnection() runs first; then the
            // disconnect is delivered as a genuine "stub exited" event.
            QTimer::singleShot(100, socket, [socket] { socket->disconnectFromServer(); });
        } else if (!reply.isEmpty()) {
            socket->write(reply);
            socket->flush();
        }

        return qint64(424242); // fake stub pid; the real inferior is never launched
    }
};

class tst_TerminalInterface : public QObject
{
    Q_OBJECT

private slots:
    void reportsSpecificError_data();
    void reportsSpecificError();
    void killReachesTheInferiorsChildren();
    void exitHangsUpTheInferiorsChildren();
    void closingTheTerminalHangsUpTheInferiorsChildren();
    void stopSignalDoesNotFreezeTheInferior();
};

void tst_TerminalInterface::reportsSpecificError_data()
{
    QTest::addColumn<QByteArray>("reply");
    QTest::addColumn<bool>("disconnect");
    QTest::addColumn<QString>("expectedFragment");

    // The reporter's case (QTCREATORBUG-12263): a working-directory failure must
    // be reported as exactly that, never as a misleading generic message.
    QTest::newRow("chdir") << QByteArray("err:chdir 2\n") << false
                           << QString("Cannot change to working directory");
    QTest::newRow("exec") << QByteArray("err:exec 13\n") << false
                          << QString("Cannot execute");
    QTest::newRow("stub-exits-before-inferior")
        << QByteArray() << true
        << QString("stub exited before the inferior was started");
}

void tst_TerminalInterface::reportsSpecificError()
{
    QFETCH(QByteArray, reply);
    QFETCH(bool, disconnect);
    QFETCH(QString, expectedFragment);

    // Inject the fake terminal backend (last-added callback set wins).
    Terminal::Hooks::instance().addCallbackSet(
        "test",
        {[](const Terminal::OpenTerminalParameters &) {},
         [reply, disconnect] {
             auto *stubCreator = new FakeStubCreator;
             stubCreator->reply = reply;
             stubCreator->disconnectWithoutInferior = disconnect;
             auto *iface = new TerminalInterface;
             iface->setStubCreator(stubCreator); // interface takes ownership
             return iface;
         }});

    QTemporaryDir workDir;
    QVERIFY(workDir.isValid());

    Process process;
    process.setTerminalMode(TerminalMode::Run);
    process.setCommand({FilePath::fromString("/bin/echo"), {"hello"}});
    process.setWorkingDirectory(FilePath::fromString(workDir.path()));

    QSignalSpy doneSpy(&process, &Process::done);
    process.start();
    QVERIFY2(doneSpy.wait(15000), "TerminalInterface never reported done()");

    QVERIFY2(process.errorString().contains(expectedFragment),
             qPrintable(QString("error string was \"%1\", expected to contain \"%2\"")
                            .arg(process.errorString(), expectedFragment)));

    Terminal::Hooks::instance().removeCallbackSet("test");
}

#ifdef Q_OS_UNIX
static FilePath processStub()
{
    return FilePath::fromString(QCoreApplication::applicationDirPath())
        .pathAppended(TEST_RELATIVE_LIBEXEC_PATH)
        .pathAppended("qtcreator_process_stub");
}

static bool isGone(int pid)
{
    if (::kill(pid, 0) != 0)
        return true;
    // A zombie still answers kill(0).
    const Result<QByteArray> stat = FilePath::fromString(QString("/proc/%1/stat").arg(pid))
                                        .fileContents();
    return stat && stat->mid(stat->lastIndexOf(')') + 2, 1) == "Z";
}
#endif

void tst_TerminalInterface::killReachesTheInferiorsChildren()
{
#ifndef Q_OS_UNIX
    QSKIP("The process stub kills a process group only on Unix.");
#else
    const FilePath stub = processStub();
    QVERIFY2(stub.isExecutableFile(), qPrintable(stub.toUserOutput()));

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const FilePath pidFile = FilePath::fromString(dir.filePath("child"));

    QLocalServer server;
    const qint64 ownPid = QCoreApplication::applicationPid();
    QVERIFY(server.listen(QString("tst_terminalinterface-%1").arg(ownPid)));

    Process stubProcess;
    stubProcess.setCommand({stub,
                            {"-s", server.fullServerName(), "--wait", "", "--", "/bin/sh", "-c",
                             "sleep 1000 & echo $! > " + pidFile.path() + "; wait"}});
    stubProcess.start();

    QVERIFY(server.waitForNewConnection(10000));
    QLocalSocket *socket = server.nextPendingConnection();
    QByteArray received;
    QTRY_VERIFY((received += socket->readAll()).contains("\npid "));

    int child = 0;
    QTRY_VERIFY((child = pidFile.fileContents().value_or(QByteArray()).trimmed().toInt()) > 0);
    const auto cleanup = qScopeGuard([child] { ::kill(child, SIGKILL); });

    socket->write("k");
    socket->flush();
    QTRY_VERIFY((received += socket->readAll()).contains("crash "));

    QTRY_VERIFY2(isGone(child), "the inferior's child survived the kill");
#endif
}

void tst_TerminalInterface::exitHangsUpTheInferiorsChildren()
{
#ifndef Q_OS_UNIX
    QSKIP("The inferior takes over the terminal only on Unix.");
#else
    const FilePath stub = processStub();
    QVERIFY2(stub.isExecutableFile(), qPrintable(stub.toUserOutput()));

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const FilePath pidFile = FilePath::fromString(dir.filePath("child"));

    QLocalServer server;
    const qint64 ownPid = QCoreApplication::applicationPid();
    QVERIFY(server.listen(QString("tst_terminalinterface-hup-%1").arg(ownPid)));

    // The pty makes the stub the controlling process of a terminal, as in a real run.
    Process stubProcess;
    stubProcess.setPtyData(Pty::Data());
    stubProcess.setCommand({stub,
                            {"-s", server.fullServerName(), "--wait", "", "--", "/bin/sh", "-c",
                             "sleep 1000 & echo $! > " + pidFile.path()}});
    stubProcess.start();

    QVERIFY(server.waitForNewConnection(10000));

    int child = 0;
    QTRY_VERIFY((child = pidFile.fileContents().value_or(QByteArray()).trimmed().toInt()) > 0);
    const auto cleanup = qScopeGuard([child] { ::kill(child, SIGKILL); });

    QTRY_VERIFY(stubProcess.state() == ProcessState::NotRunning);

    QTRY_VERIFY2(isGone(child), "the inferior's child survived the end of the run");
#endif
}

void tst_TerminalInterface::closingTheTerminalHangsUpTheInferiorsChildren()
{
#ifndef Q_OS_UNIX
    QSKIP("The inferior takes over the terminal only on Unix.");
#else
    const FilePath stub = processStub();
    QVERIFY2(stub.isExecutableFile(), qPrintable(stub.toUserOutput()));

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const FilePath pidFile = FilePath::fromString(dir.filePath("child"));

    QLocalServer server;
    const qint64 ownPid = QCoreApplication::applicationPid();
    QVERIFY(server.listen(QString("tst_terminalinterface-close-%1").arg(ownPid)));

    Process stubProcess;
    stubProcess.setPtyData(Pty::Data());
    stubProcess.setCommand({stub,
                            {"-s", server.fullServerName(), "--wait", "Exit code %1", "--",
                             "/bin/sh", "-c", "sleep 1000 & echo $! > " + pidFile.path()}});
    stubProcess.start();

    QVERIFY(server.waitForNewConnection(10000));
    QLocalSocket *socket = server.nextPendingConnection();
    QByteArray received;
    QTRY_VERIFY((received += socket->readAll()).contains("exit "));

    const int spidStart = received.indexOf("spid ");
    QVERIFY(spidStart >= 0);
    const QByteArray spid = received.mid(spidStart + 5);
    const int stubPid = spid.left(spid.indexOf('\n')).toInt();
    QVERIFY(stubPid > 0);

    const int child = pidFile.fileContents().value_or(QByteArray()).trimmed().toInt();
    QVERIFY(child > 0);
    const auto cleanup = qScopeGuard([child] { ::kill(child, SIGKILL); });
    QVERIFY2(!isGone(child), "the inferior's child died before the hangup");

    // What the terminal sends its controlling process when its window is closed.
    ::kill(stubPid, SIGHUP);
    QTRY_VERIFY(stubProcess.state() == ProcessState::NotRunning);

    QTRY_VERIFY2(isGone(child), "the inferior's child survived closing the terminal");
#endif
}

void tst_TerminalInterface::stopSignalDoesNotFreezeTheInferior()
{
#ifndef Q_OS_UNIX
    QSKIP("Stop signals exist only on Unix.");
#else
    const FilePath stub = processStub();
    QVERIFY2(stub.isExecutableFile(), qPrintable(stub.toUserOutput()));

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const FilePath marker = FilePath::fromString(dir.filePath("resumed"));

    QLocalServer server;
    const qint64 ownPid = QCoreApplication::applicationPid();
    QVERIFY(server.listen(QString("tst_terminalinterface-tstp-%1").arg(ownPid)));

    Process stubProcess;
    stubProcess.setPtyData(Pty::Data());
    stubProcess.setCommand({stub,
                            {"-s", server.fullServerName(), "--wait", "", "--", "/bin/sh", "-c",
                             "kill -TSTP $$; echo resumed > \"$0\"",
                             marker.path()}});
    stubProcess.start();

    QVERIFY(server.waitForNewConnection(10000));

    QTRY_VERIFY2(marker.exists(), "a stop signal froze the inferior");
    QTRY_VERIFY(stubProcess.state() == ProcessState::NotRunning);
#endif
}

QTEST_GUILESS_MAIN(tst_TerminalInterface)

#include "tst_terminalinterface.moc"
