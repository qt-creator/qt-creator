// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <app/app_version.h>

#include <utils/algorithm.h>
#include <utils/deviceshell.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/launcherinterface.h>
#include <utils/process.h>
#include <utils/temporarydirectory.h>

#include <QObject>
#include <QtConcurrent>
#include <QtTest>

using namespace Utils;

class TestShell : public DeviceShell
{
public:
    TestShell(CommandLine cmdLine, bool failScript = false)
        : DeviceShell(failScript)
        , m_cmdLine(std::move(cmdLine))
    {
        start();
    }

private:
    void setupShellProcess(Process *shellProcess) override
    {
        shellProcess->setCommand(m_cmdLine);
    }

    CommandLine m_cmdLine;
};

bool testDocker(const FilePath &executable)
{
    Process p;
    p.setCommand({executable, {"info", "--format", "{{.OSType}}"}});
    p.runBlocking();
    const QString platform = p.cleanedStdOut().trimmed();
    return p.result() == ProcessResult::FinishedWithSuccess && platform == "linux";
}

class tst_DeviceShell : public QObject
{
    Q_OBJECT
private:
    QByteArray m_asciiTestData{256, Qt::Uninitialized};

    QList<CommandLine> m_availableShells;
    bool m_dockerSetupCheckOk{false};

private:
    QString testString(int length)
    {
        QRandomGenerator generator;
        QString result;
        for (int i = 0; i < length; ++i)
            result.append(QChar{generator.bounded('a', 'z')});

        return result;
    }

private slots:
    void initTestCase()
    {
        TemporaryDirectory::setMasterTemporaryDirectory(
            QDir::tempPath() + "/" + Core::Constants::IDE_CASED_ID + "-XXXXXX");

        const QString libExecPath(qApp->applicationDirPath() + '/'
                                  + QLatin1String(TEST_RELATIVE_LIBEXEC_PATH));
        LauncherInterface::setPathToLauncher(libExecPath);

        std::iota(m_asciiTestData.begin(), m_asciiTestData.end(), 0);

        const FilePath dockerExecutable = Environment::systemEnvironment()
                                              .searchInPath("docker", {"/usr/local/bin"});

        if (dockerExecutable.exists()) {
            m_availableShells.append({dockerExecutable, {"run", "-i", "--rm", "alpine"}});
            if (testDocker(dockerExecutable)) {
                m_dockerSetupCheckOk = true;
            } else {
                // On linux, docker needs some post-install steps: https://docs.docker.com/engine/install/linux-postinstall/
                // Also check if you can start a simple container from the command line: "docker run -it alpine".
                qWarning() << "Checking docker failed, tests will be skipped.";
            }
        }

        // On older versions of macOS, base64 does not understand "-d" ( only "-D" ), so we skip the tests here.
        const QVersionNumber osVersionNumber = QVersionNumber::fromString(
            QSysInfo::productVersion());

        const bool isOldMacOS = Utils::HostOsInfo::isMacHost()
                                && osVersionNumber.majorVersion() <= 10
                                && osVersionNumber.minorVersion() <= 14;

        if (isOldMacOS)
            qWarning() << "Skipping local tests, as macOS version is <= 10.14";

        if (!Utils::HostOsInfo::isWindowsHost() && !isOldMacOS) {
            // Windows by default has bash.exe, which does not work unless a working wsl is installed.
            // Therefore we only test shells on linux / mac hosts.
            const auto shells = {"dash", "bash", "sh", "zsh"};

            for (const auto &shell : shells) {
                const FilePath executable = Environment::systemEnvironment()
                                                .searchInPath(shell, {"/usr/local/bin"});
                if (executable.exists())
                    m_availableShells.append({executable, {}});
            }
        }

        if (m_availableShells.isEmpty()) {
            QSKIP("Skipping deviceshell tests, as no compatible shell could be found");
        }
    }

    void cleanupTestCase() { Singleton::deleteAll(); }

    void testArguments_data()
    {
        QTest::addColumn<CommandLine>("cmdLine");
        QTest::addColumn<QString>("testData");

        for (const auto &cmdLine : std::as_const(m_availableShells)) {
            QTest::newRow((cmdLine.executable().baseName() + " : simple").toUtf8())
                << cmdLine << "Hallo Welt!";
            QTest::newRow((cmdLine.executable().baseName() + " : japanese").toUtf8())
                << cmdLine
                << QString::fromUtf8(u8"\xe8\xac\x9d\xe3\x81\x8d\xe3\x82\x81\xe9\x80\x80\x31\x30"
                                     u8"\xe8\x89\xaf\xe3\x81\x9a\xe3"
                                     u8"\x82\xa4\xe3\x81\xb5\xe3\x81\x8b\xe7\x89\x88\xe8\x84\xb3"
                                     u8"\xe3\x83\xa9\xe3\x83\xaf\xe6"
                                     u8"\xad\xa2\xe9\x80\x9a\xe3\x83\xa8\xe3\x83\xb2\xe3\x82\xad");
            QTest::newRow((cmdLine.executable().baseName() + " : german").toUtf8())
                << cmdLine
                << QString::fromUtf8(u8"\x48\x61\x6c\x6c\xc3\xb6\x2c\x20\x77\x69\x65\x20\x67\xc3"
                                     u8"\xa4\x68\x74\x20\x65\x73\x20"
                                     u8"\x64\xc3\xbc\x72");

            QTest::newRow((cmdLine.executable().baseName() + " : long").toUtf8())
                << cmdLine << testString(4096 * 16);
        }
    }

    void testArguments()
    {
        QFETCH(CommandLine, cmdLine);
        QFETCH(QString, testData);

        if (cmdLine.executable().toString().contains("docker") && !m_dockerSetupCheckOk) {
            QSKIP("Docker was found, but does not seem to be set up correctly, skipping.");
        }

        TestShell shell(cmdLine);
        QCOMPARE(shell.state(), DeviceShell::State::Succeeded);

        QRandomGenerator generator;

        const RunResult result = shell.runInShell({"echo", {testData}});
        QCOMPARE(result.exitCode, 0);
        const QString expected = testData + "\n";
        const QString resultAsUtf8 = QString::fromUtf8(result.stdOut);
        QCOMPARE(resultAsUtf8.size(), expected.size());
        QCOMPARE(resultAsUtf8, expected);
    }

    void testStdin_data()
    {
        QTest::addColumn<CommandLine>("cmdLine");
        QTest::addColumn<QString>("testData");

        for (const auto &cmdLine : std::as_const(m_availableShells)) {
            QTest::newRow((cmdLine.executable().baseName() + " : simple").toUtf8())
                << cmdLine << "Hallo Welt!";
            QTest::newRow((cmdLine.executable().baseName() + " : japanese").toUtf8())
                << cmdLine
                << QString::fromUtf8(u8"\xe8\xac\x9d\xe3\x81\x8d\xe3\x82\x81\xe9\x80\x80\x31\x30"
                                     u8"\xe8\x89\xaf\xe3\x81\x9a\xe3"
                                     u8"\x82\xa4\xe3\x81\xb5\xe3\x81\x8b\xe7\x89\x88\xe8\x84\xb3"
                                     u8"\xe3\x83\xa9\xe3\x83\xaf\xe6"
                                     u8"\xad\xa2\xe9\x80\x9a\xe3\x83\xa8\xe3\x83\xb2\xe3\x82\xad");
            QTest::newRow((cmdLine.executable().baseName() + " : german").toUtf8())
                << cmdLine
                << QString::fromUtf8(u8"\x48\x61\x6c\x6c\xc3\xb6\x2c\x20\x77\x69\x65\x20\x67\xc3"
                                     u8"\xa4\x68\x74\x20\x65\x73\x20"
                                     u8"\x64\xc3\xbc\x72");

            QTest::newRow((cmdLine.executable().baseName() + " : long").toUtf8())
                << cmdLine << testString(4096 * 16);
        }
    }

    void testStdin()
    {
        QFETCH(CommandLine, cmdLine);
        QFETCH(QString, testData);

        if (cmdLine.executable().toString().contains("docker") && !m_dockerSetupCheckOk) {
            QSKIP("Docker was found, but does not seem to be set up correctly, skipping.");
        }

        TestShell shell(cmdLine);
        QCOMPARE(shell.state(), DeviceShell::State::Succeeded);

        QRandomGenerator generator;

        const RunResult result = shell.runInShell({"cat", {}}, testData.toUtf8());
        QCOMPARE(result.exitCode, 0);
        const QString resultAsUtf8 = QString::fromUtf8(result.stdOut);
        QCOMPARE(resultAsUtf8.size(), testData.size());
        QCOMPARE(resultAsUtf8, testData);
    }

    void testAscii_data()
    {
        QTest::addColumn<CommandLine>("cmdLine");
        for (const auto &cmdLine : std::as_const(m_availableShells)) {
            QTest::newRow(cmdLine.executable().baseName().toUtf8()) << cmdLine;
        }
    }

    void testAscii()
    {
        QFETCH(CommandLine, cmdLine);

        if (cmdLine.executable().toString().contains("docker") && !m_dockerSetupCheckOk) {
            QSKIP("Docker was found, but does not seem to be set up correctly, skipping.");
        }

        TestShell shell(cmdLine);
        QCOMPARE(shell.state(), DeviceShell::State::Succeeded);

        const RunResult result = shell.runInShell({"cat", {}}, m_asciiTestData);
        QCOMPARE(result.stdOut, m_asciiTestData);
    }

    void testStdErr_data()
    {
        QTest::addColumn<CommandLine>("cmdLine");
        for (const auto &cmdLine : m_availableShells) {
            QTest::newRow(cmdLine.executable().baseName().toUtf8()) << cmdLine;
        }
    }

    void testStdErr()
    {
        QFETCH(CommandLine, cmdLine);

        if (cmdLine.executable().toString().contains("docker") && !m_dockerSetupCheckOk) {
            QSKIP("Docker was found, but does not seem to be set up correctly, skipping.");
        }

        TestShell shell(cmdLine);
        QCOMPARE(shell.state(), DeviceShell::State::Succeeded);

        const RunResult result = shell.runInShell({"cat", {}}, m_asciiTestData);
        QCOMPARE(result.stdOut, m_asciiTestData);
        QVERIFY(result.stdErr.isEmpty());

        const RunResult result2 = shell.runInShell(
            {"cat", {"/tmp/i-do-not-exist.none"}});
        QVERIFY(!result2.stdErr.isEmpty());
        QVERIFY(result2.exitCode != 0);
    }

    void testNoCommand_data()
    {
        QTest::addColumn<CommandLine>("cmdLine");
        for (const auto &cmdLine : m_availableShells) {
            QTest::newRow(cmdLine.executable().baseName().toUtf8()) << cmdLine;
        }
    }

    void testNoCommand()
    {
        QFETCH(CommandLine, cmdLine);

        if (cmdLine.executable().toString().contains("docker") && !m_dockerSetupCheckOk) {
            QSKIP("Docker was found, but does not seem to be set up correctly, skipping.");
        }

        TestShell shell(cmdLine);
        QCOMPARE(shell.state(), DeviceShell::State::Succeeded);

        const RunResult result = shell.runInShell({}, {});

        QVERIFY(result.exitCode == 255);
    }

    void testMultiThreadedFind_data()
    {
        QTest::addColumn<CommandLine>("cmdLine");
        for (const auto &cmdLine : m_availableShells) {
            QTest::newRow(cmdLine.executable().baseName().toUtf8()) << cmdLine;
        }
    }

    void testMultiThreadedFind()
    {
        QFETCH(CommandLine, cmdLine);

        if (cmdLine.executable().toString().contains("docker") && !m_dockerSetupCheckOk) {
            QSKIP("Docker was found, but does not seem to be set up correctly, skipping.");
        }

        TestShell shell(cmdLine);
        QCOMPARE(shell.state(), DeviceShell::State::Succeeded);

        int maxDepth = 4;
        int numMs = 0;

        while (true) {
            QElapsedTimer t;
            t.start();
            const RunResult result = shell.runInShell({"find", {"/usr", "-maxdepth",
                                                                QString::number(maxDepth)}});
            numMs = t.elapsed();
            qDebug() << "adjusted maxDepth" << maxDepth << "took" << numMs << "ms";
            if (numMs < 100 || maxDepth == 1) {
                break;
            }
            maxDepth--;
        }

        const auto find = [&shell, maxDepth](int i) {
            QElapsedTimer t;
            t.start();
            const RunResult result = shell.runInShell({"find", {"/usr", "-maxdepth",
                                                                QString::number(maxDepth)}});
            qDebug() << i << "took" << t.elapsed() << "ms";
            return result.stdOut;
        };
        const QList<int> runs{1,2,3,4,5,6,7,8,9};
        const QList<QByteArray> results = QtConcurrent::blockingMapped(runs, find);
        QVERIFY(!Utils::anyOf(results, [&results](const QByteArray r) { return r != results[0]; }));
    }

    void testNoScript_data()
    {
        QTest::addColumn<CommandLine>("cmdLine");
        for (const auto &cmdLine : m_availableShells) {
            QTest::newRow(cmdLine.executable().baseName().toUtf8()) << cmdLine;
        }
    }

    void testNoScript()
    {
        QFETCH(CommandLine, cmdLine);

        TestShell shell(cmdLine, true);
        QCOMPARE(shell.state(), DeviceShell::State::Failed);

        const RunResult result = shell.runInShell({"echo", {"Hello"}});
        QCOMPARE(result.exitCode, 0);
    }
};

QTEST_GUILESS_MAIN(tst_DeviceShell)

#include "tst_deviceshell.moc"
