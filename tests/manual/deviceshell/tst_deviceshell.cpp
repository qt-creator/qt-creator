// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <app/app_version.h>

#include <utils/deviceshell.h>
#include <utils/environment.h>
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
    TestShell() { start(); }

    static CommandLine cmdLine() {
        static CommandLine cmd;

        if (cmd.isEmpty()) {
            const FilePath dockerExecutable = Environment::systemEnvironment()
                                                  .searchInPath("docker", {"/usr/local/bin"});
            const FilePath dashExecutable = Environment::systemEnvironment()
                                                .searchInPath("dash", {"/usr/local/bin"});
            const FilePath bashExecutable = Environment::systemEnvironment()
                                                .searchInPath("bash", {"/usr/local/bin"});
            const FilePath shExecutable = Environment::systemEnvironment()
                                                .searchInPath("sh", {"/usr/local/bin"});

            if (dockerExecutable.exists()) {
                cmd = {dockerExecutable, {"run", "-i", "--rm","alpine"}};
            } else if (dashExecutable.exists()) {
                cmd = {dashExecutable, {}};
            } else if (bashExecutable.exists()) {
                cmd = {bashExecutable, {}};
            } else if (shExecutable.exists()) {
                cmd = {shExecutable, {}};
            }

            if (cmd.isEmpty()) {
                return cmd;
            }

            qDebug() << "Using shell cmd:" << cmd;
        }

        return cmd;
    }

private:
    void setupShellProcess(Process *shellProcess) override
    {
        shellProcess->setCommand(cmdLine());
    }
};

class tst_DeviceShell : public QObject
{
    Q_OBJECT

    QList<QByteArray> testArrays(const int numArrays)
    {
        QRandomGenerator generator;
        QList<QByteArray> result;

        for (int i = 0; i < numArrays; i++) {
            QByteArray data;
            auto numLines = generator.bounded(1, 100);
            for (int l = 0; l < numLines; l++) {
                auto numChars = generator.bounded(10, 40);
                for (int c = 0; c < numChars; c++) {
                    data += static_cast<char>(generator.bounded('a', 'z'));
                }
                data += '\n';
            }
            result.append(data);
        }
        return result;
    }

    void test(int numCalls, int maxNumThreads)
    {
        TestShell shell;
        QCOMPARE(shell.state(), DeviceShell::State::Succeeded);

        QThreadPool::globalInstance()->setMaxThreadCount(maxNumThreads);

        const QList<QByteArray> testArray = testArrays(numCalls);

        QElapsedTimer t;
        t.start();

        const auto cat = [&shell](const QByteArray &data) {
            return shell.runInShell({"cat", {}}, data).stdOut;
        };
        const QList<QByteArray> results = QtConcurrent::blockingMapped(testArray, cat);
        QCOMPARE(results, testArray);

        qDebug() << "maxThreads:" << maxNumThreads << ", took:" << t.elapsed() / 1000.0
                 << "seconds";
    }

    void testSleep(QList<int> testData, int maxNumThreads)
    {
        TestShell shell;
        QCOMPARE(shell.state(), DeviceShell::State::Succeeded);

        QThreadPool::globalInstance()->setMaxThreadCount(maxNumThreads);

        QElapsedTimer t;
        t.start();

        const auto sleep = [&shell](int time) {
            shell.runInShell({"sleep", {QString("%1").arg(time)}});
            return 0;
        };
        const QList<int> results = QtConcurrent::blockingMapped(testData, sleep);
        QCOMPARE(results, QList<int>(testData.size(), 0));

        qDebug() << "maxThreads:" << maxNumThreads << ", took:" << t.elapsed() / 1000.0
                 << "seconds";
    }

private slots:
    void initTestCase()
    {
        TemporaryDirectory::setMasterTemporaryDirectory(
            QDir::tempPath() + "/" + Core::Constants::IDE_CASED_ID + "-XXXXXX");

        const QString libExecPath(qApp->applicationDirPath() + '/'
                                  + QLatin1String(TEST_RELATIVE_LIBEXEC_PATH));
        LauncherInterface::setPathToLauncher(libExecPath);

        if (TestShell::cmdLine().isEmpty()) {
            QSKIP("Skipping deviceshell tests, as no compatible shell could be found");
        }
    }

    void cleanupTestCase() { Singleton::deleteAll(); }

    void testEncoding_data()
    {
        QTest::addColumn<QString>("utf8string");

        QTest::newRow("japanese") << QString::fromUtf8(
            u8"\xe8\xac\x9d\xe3\x81\x8d\xe3\x82\x81\xe9\x80\x80\x31\x30\xe8\x89\xaf\xe3\x81\x9a\xe3"
            u8"\x82\xa4\xe3\x81\xb5\xe3\x81\x8b\xe7\x89\x88\xe8\x84\xb3\xe3\x83\xa9\xe3\x83\xaf\xe6"
            u8"\xad\xa2\xe9\x80\x9a\xe3\x83\xa8\xe3\x83\xb2\xe3\x82\xad\n");
        QTest::newRow("german") << QString::fromUtf8(
            u8"\x48\x61\x6c\x6c\xc3\xb6\x2c\x20\x77\x69\x65\x20\x67\xc3\xa4\x68\x74\x20\x65\x73\x20"
            u8"\x64\xc3\xbc\x72\n");
    }

    void testEncoding()
    {
        QFETCH(QString, utf8string);

        TestShell shell;
        QCOMPARE(shell.state(), DeviceShell::State::Succeeded);

        const RunResult r = shell.runInShell({"cat", {}}, utf8string.toUtf8());
        const QString output = QString::fromUtf8(r.stdOut);
        QCOMPARE(output, utf8string);
    }

    void testThreading_data()
    {
        QTest::addColumn<int>("numThreads");
        QTest::addColumn<int>("numIterations");

        QTest::newRow("multi-threaded") << 10 << 1000;
        QTest::newRow("single-threaded") << 1 << 1000;
    }

    void testThreading()
    {
        QFETCH(int, numThreads);
        QFETCH(int, numIterations);

        test(numIterations, numThreads);
    }

    void testSleepMulti()
    {
        QList<int> testData{4, 7, 10, 3, 1, 10, 3, 3, 5, 4};
        int full = std::accumulate(testData.begin(), testData.end(), 0);
        qDebug() << "Testing sleep, full time is:" << full << "seconds";
        QElapsedTimer t;
        t.start();
        testSleep(testData, 10);
        const int multiThreadRunTime = t.restart();
        testSleep(testData, 1);
        const int singleThreadRunTime = t.elapsed();
        QVERIFY(multiThreadRunTime < singleThreadRunTime);
    }
};

QTEST_GUILESS_MAIN(tst_DeviceShell)

#include "tst_deviceshell.moc"
