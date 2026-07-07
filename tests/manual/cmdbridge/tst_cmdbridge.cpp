// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "utils/futuresynchronizer.h"
#include <app/app_version.h>

#include <client/bridgedfileaccess.h>
#include <client/cmdbridgeclient.h>

#include <utils/devicefileaccess.h>
#include <utils/environment.h>
#include <utils/processreaper.h>
#include <utils/qtcprocess.h>
#include <utils/temporarydirectory.h>

#include <QElapsedTimer>
#include <QLocalServer>
#include <QLocalSocket>
#include <QObject>
#include <QSignalSpy>
#include <QTest>

using namespace Utils;

#define QVERIFY_DEPLOY_RESULT(result) \
    QVERIFY2(result, result ? #result : static_cast<const char *>(result.error().message.toUtf8()))

class TestDFA : public UnixDeviceFileAccess
{
    friend class tst_CmdBridge;

public:
    using UnixDeviceFileAccess::UnixDeviceFileAccess;

    Result<RunResult> runInShellImpl(const CommandLine &cmdLine, const QByteArray &inputData = {}) const final
    {
        // Note: Don't convert into Utils::Process. See more comments in this change in gerrit.
        QProcess p;
        p.setProgram(cmdLine.executable().toUrlishString());
        p.setArguments(cmdLine.splitArguments());
        p.setProcessChannelMode(QProcess::SeparateChannels);

        p.start();
        p.waitForStarted();
        if (inputData.size() > 0) {
            p.write(inputData);
            p.closeWriteChannel();
        }
        p.waitForFinished();
        return RunResult{p.exitCode(), p.readAllStandardOutput(), p.readAllStandardError()};
    }

    void findUsingLs(const QString &current, const FileFilter &filter, QStringList *found)
    {
        UnixDeviceFileAccess::findUsingLs(current, filter, found, {});
    }
};

class tst_CmdBridge : public QObject
{
    Q_OBJECT

    QString libExecPath = TEST_LIBEXEC_PATH;

private slots:
    void initTestCase()
    {
        TemporaryDirectory::setMasterTemporaryDirectory(
            QDir::tempPath() + "/" + Core::Constants::IDE_CASED_ID + "-XXXXXX");
    }

    void cleanupTestCase() { ProcessReaper::deleteAll(); }

    void testDeviceEnvironment()
    {
        CmdBridge::FileAccess fileAccess;
        CmdBridge::FileAccess::DeployResult res = fileAccess.deployAndInit(
            FilePath::fromUserInput(libExecPath),
            FilePath::fromUserInput("/"),
            Environment::systemEnvironment());

        if (!res)
            qDebug() << "Failed to deploy and init:" << res.error().message;

        QVERIFY_DEPLOY_RESULT(res);

        Result<Environment> env = fileAccess.deviceEnvironment();
        QVERIFY_RESULT(env);
        QVERIFY(env->hasChanges());
        QVERIFY(!env->toStringList().isEmpty());
        env = fileAccess.deviceEnvironment();
    }

    void testSetPermissions()
    {
        CmdBridge::FileAccess fileAccess;
        CmdBridge::FileAccess::DeployResult res = fileAccess.deployAndInit(
            FilePath::fromUserInput(libExecPath),
            FilePath::fromUserInput("/"),
            Environment::systemEnvironment());

        QVERIFY_DEPLOY_RESULT(res);

        Result<FilePath> tempFile = fileAccess.createTempFile(FilePath::fromUserInput(QDir::tempPath())
                                                  / "test.XXXXXX");
        QVERIFY_RESULT(tempFile);

        QFile::Permissions perms = QFile::ReadOwner | QFile::WriteOwner;

        QVERIFY(fileAccess.setPermissions(*tempFile, perms));
        QCOMPARE(*fileAccess.permissions(*tempFile) & 0xF0FF, perms);

        perms = QFile::ReadOwner | QFile::WriteOwner | QFile::ReadGroup | QFile::WriteOther;

        QVERIFY(fileAccess.setPermissions(*tempFile, perms));
        QCOMPARE(*fileAccess.permissions(*tempFile) & 0xF0FF, perms);

        QVERIFY(fileAccess.removeFile(*tempFile));
    }

    void testTempFile()
    {
        CmdBridge::FileAccess fileAccess;
        CmdBridge::FileAccess::DeployResult res = fileAccess.deployAndInit(
            FilePath::fromUserInput(libExecPath),
            FilePath::fromUserInput("/"),
            Environment::systemEnvironment());

        QVERIFY_DEPLOY_RESULT(res);

        Result<FilePath> tempFile = fileAccess.createTempFile(FilePath::fromUserInput(QDir::tempPath())
                                                  / "test.XXXXXX");

        QVERIFY_RESULT(tempFile);
        QVERIFY_RESULT(fileAccess.exists(*tempFile));
        QVERIFY(*fileAccess.exists(*tempFile));
        QVERIFY_RESULT(fileAccess.removeFile(*tempFile));
        QVERIFY_RESULT(fileAccess.exists(*tempFile));
        QVERIFY(!*fileAccess.exists(*tempFile));

        tempFile = fileAccess.createTempFile(FilePath::fromUserInput(QDir::tempPath())
                                             / "test.XXXXXX");
        QVERIFY_RESULT(tempFile);
        QVERIFY(tempFile->fileName().startsWith("test."));
        QVERIFY(!tempFile->fileName().startsWith("test.XXXXXX"));
        QVERIFY_RESULT(fileAccess.exists(*tempFile));
        QVERIFY(*fileAccess.exists(*tempFile));
        QVERIFY_RESULT(fileAccess.removeFile(*tempFile));
        QVERIFY(!*fileAccess.exists(*tempFile));
    }

    void testTempDir()
    {
        CmdBridge::FileAccess fileAccess;
        CmdBridge::FileAccess::DeployResult res = fileAccess.deployAndInit(
            FilePath::fromUserInput(libExecPath),
            FilePath::fromUserInput("/"),
            Environment::systemEnvironment());

        QVERIFY_DEPLOY_RESULT(res);

        Result<FilePath> tempDir = fileAccess.createTempDir(
            FilePath::fromUserInput(QDir::tempPath()) / "test.XXXXXX");

        QVERIFY_RESULT(tempDir);
        QVERIFY_RESULT(fileAccess.exists(*tempDir));
        QVERIFY(*fileAccess.exists(*tempDir));
        QVERIFY_RESULT(fileAccess.isWritableDirectory(*tempDir));
        QVERIFY(*fileAccess.isWritableDirectory(*tempDir));
        QVERIFY_RESULT(fileAccess.removeRecursively(*tempDir));
        QVERIFY_RESULT(fileAccess.exists(*tempDir));
        QVERIFY(!*fileAccess.exists(*tempDir));
    }

    void testTempFileWithoutPlaceholder()
    {
        CmdBridge::FileAccess fileAccess;
        CmdBridge::FileAccess::DeployResult res = fileAccess.deployAndInit(
            FilePath::fromUserInput(libExecPath),
            FilePath::fromUserInput("/"),
            Environment::systemEnvironment());

        QVERIFY_DEPLOY_RESULT(res);

        const FilePath pattern = FilePath::fromUserInput(QDir::tempPath()) / "test.txt";

        // We have to create a file with the pattern name to test the automatic placeholder
        // adding. go' os.CreateTemp() will fail if no placeholder is present, and the file exists.
        pattern.writeFileContents("Test");

        Result<FilePath> tempFile = fileAccess.createTempFile(pattern);

        QVERIFY_RESULT(tempFile);
        QVERIFY_RESULT(fileAccess.exists(*tempFile));
        QVERIFY(*fileAccess.exists(*tempFile));
        QVERIFY_RESULT(fileAccess.removeFile(*tempFile));
        QVERIFY_RESULT(fileAccess.exists(*tempFile));
        QVERIFY(!*fileAccess.exists(*tempFile));
        QVERIFY(tempFile->fileName().startsWith("test.txt."));
    }

    void testIsWritableDirectory()
    {
        CmdBridge::FileAccess fileAccess;
        CmdBridge::FileAccess::DeployResult res = fileAccess.deployAndInit(
            FilePath::fromUserInput(libExecPath),
            FilePath::fromUserInput("/"),
            Environment::systemEnvironment());

        QVERIFY_DEPLOY_RESULT(res);

        const FilePath testDir = FilePath::fromUserInput(QDir::tempPath() + "/testIsWritableDir");
        QVERIFY_RESULT(fileAccess.isWritableDirectory(testDir));
        QVERIFY(!*fileAccess.isWritableDirectory(testDir));
    }

    void testEnsureWritableDir()
    {
        CmdBridge::FileAccess fileAccess;
        CmdBridge::FileAccess::DeployResult res = fileAccess.deployAndInit(
            FilePath::fromUserInput(libExecPath),
            FilePath::fromUserInput("/"),
            Environment::systemEnvironment());

        QVERIFY_DEPLOY_RESULT(res);

        const FilePath testFile = FilePath::fromUserInput(
            QDir::tempPath() + "/testEnsureWritableDir");
        QVERIFY(!QDir(testFile.path()).exists());

        const auto result = fileAccess.ensureWritableDirectory(testFile);
        QVERIFY_RESULT(result);
        QVERIFY(QDir(testFile.path()).exists());
        QVERIFY_RESULT(testFile.removeRecursively());
    }

    void testFileContents()
    {
        CmdBridge::FileAccess fileAccess;
        CmdBridge::FileAccess::DeployResult res = fileAccess.deployAndInit(
            FilePath::fromUserInput(libExecPath),
            FilePath::fromUserInput("/"),
            Environment::systemEnvironment());

        QVERIFY_DEPLOY_RESULT(res);

        const FilePath testFile = FilePath::fromUserInput(QDir::tempPath() + "/test.txt");

        QByteArray testData = R"(
This is a test file.
It is just big enough to allow us to test a few things.
We want some lines to be longer than others.
We want some lines to be shorter than others.
We want some lines to be empty.

We want to test the beginning of the file.
We want to test the middle of the file.
We want to test the end of the file.

This should be enough to test the fileContents method.
The end.
)";

        auto writeResult = fileAccess.writeFileContents(testFile, testData);
        QTC_ASSERT_RESULT(writeResult, QVERIFY(writeResult));
        QCOMPARE(*writeResult, testData.size());

        auto fileContents = fileAccess.fileContents(testFile, -1, 0);

        QTC_ASSERT_RESULT(fileContents, QVERIFY(fileContents));

        QVERIFY(fileContents->size() > 100);

        auto midContent = fileAccess.fileContents(testFile, 10, 10);
        QTC_ASSERT_RESULT(midContent, QVERIFY(midContent));

        QCOMPARE(*midContent, fileContents->mid(10, 10));

        auto endContent = fileAccess.fileContents(testFile, -1, 10);
        QTC_ASSERT_RESULT(endContent, QVERIFY(endContent));

        QCOMPARE(*endContent, fileContents->mid(10));

        QVERIFY(fileContents->size() > 0);

        const FilePath fileCopy = FilePath::fromUserInput(QDir::tempPath() + "/testcopy.txt");
        QVERIFY_RESULT(fileAccess.copyFile(testFile, fileCopy));
        QVERIFY_RESULT(fileAccess.exists(fileCopy));
        QVERIFY(*fileAccess.exists(fileCopy));
        QCOMPARE(fileAccess.fileContents(fileCopy, -1, 0), fileContents);

        const FilePath mvTarget = FilePath::fromUserInput(QDir::tempPath() + "/testmoved.txt");
        QVERIFY_RESULT(fileAccess.renameFile(fileCopy, mvTarget));
        QVERIFY_RESULT(fileAccess.exists(mvTarget));
        QVERIFY(*fileAccess.exists(mvTarget));
        QVERIFY_RESULT(fileAccess.exists(fileCopy));
        QVERIFY(!*fileAccess.exists(fileCopy));
        QCOMPARE(fileAccess.fileContents(mvTarget, -1, 0), fileContents);

        QVERIFY_RESULT(fileAccess.removeFile(mvTarget));
        QVERIFY_RESULT(fileAccess.removeFile(testFile));
        QVERIFY(!fileAccess.removeFile(testFile));

        QVERIFY_RESULT(fileAccess.ensureExistingFile(testFile));
        QVERIFY_RESULT(fileAccess.removeFile(testFile));

        const FilePath dir = FilePath::fromUserInput(QDir::tempPath() + "/testdir");
        QVERIFY_RESULT(fileAccess.createDirectory(dir));
        QVERIFY_RESULT(fileAccess.exists(dir));
        QVERIFY(*fileAccess.exists(dir));
        QVERIFY_RESULT(fileAccess.isReadableDirectory(dir));
        QVERIFY(*fileAccess.isReadableDirectory(dir));
        QVERIFY_RESULT(fileAccess.removeRecursively(dir));
        QVERIFY_RESULT(fileAccess.exists(dir));
        QVERIFY(!*fileAccess.exists(dir));
    }

    void testIs()
    {
        Result<FilePath> bridgePath = CmdBridge::Client::getCmdBridgePath(HostOsInfo::hostOs(),
                                                              HostOsInfo::hostArchitecture(),
                                                              FilePath::fromUserInput(libExecPath));
        QTC_ASSERT_RESULT(bridgePath, QSKIP("No bridge found"));

        CmdBridge::Client client(*bridgePath, Environment::systemEnvironment());
        client.start();

        bool result = client.is("/tmp", CmdBridge::Client::Is::Dir)->result();
        QVERIFY(result);

        result = client.is("/idonotexist", CmdBridge::Client::Is::Dir)->result();
        QVERIFY(!result);
        result = client.is("/idonotexist", CmdBridge::Client::Is::Exists)->result();
        QVERIFY(!result);
    }

    void testStat()
    {
        Result<FilePath> bridgePath = CmdBridge::Client::getCmdBridgePath(HostOsInfo::hostOs(),
                                                              HostOsInfo::hostArchitecture(),
                                                              FilePath::fromUserInput(libExecPath));
        QTC_ASSERT_RESULT(bridgePath, QSKIP("No bridge found"));

        CmdBridge::Client client(*bridgePath, Environment::systemEnvironment());
        QTC_ASSERT_RESULT(client.start(), return);

        try {
            auto result = client.stat("/tmp")->result();
            qDebug() << result.numHardLinks;

        } catch (const std::exception &e) {
            qDebug() << e.what();
        }
    }

    void testSymLink()
    {
        CmdBridge::FileAccess fileAccess;
        CmdBridge::FileAccess::DeployResult res = fileAccess.deployAndInit(
            FilePath::fromUserInput(libExecPath),
            FilePath::fromUserInput("/"),
            Environment::systemEnvironment());

        QVERIFY_DEPLOY_RESULT(res);

        // FIXME: /tmp is not everywhere a symlink. Create a test link ourselves?
        Result<FilePath> tmptarget = fileAccess.symLinkTarget(FilePath::fromUserInput("/tmp"));
        QVERIFY_RESULT(tmptarget);

        qDebug() << "SymLinkTarget:" << tmptarget->toUserOutput();
    }

    void testFileId()
    {
        CmdBridge::FileAccess fileAccess;
        CmdBridge::FileAccess::DeployResult res = fileAccess.deployAndInit(
            FilePath::fromUserInput(libExecPath),
            FilePath::fromUserInput("/"),
            Environment::systemEnvironment());

        QVERIFY_DEPLOY_RESULT(res);

        Result<QByteArray> fileId = fileAccess.fileId(FilePath::fromUserInput(QDir::rootPath()));
        QVERIFY_RESULT(fileId);

        qDebug() << "File Id:" << *fileId;
    }

    void testFreeSpace()
    {
        CmdBridge::FileAccess fileAccess;
        CmdBridge::FileAccess::DeployResult res = fileAccess.deployAndInit(
            FilePath::fromUserInput(libExecPath),
            FilePath::fromUserInput("/"),
            Environment::systemEnvironment());

        QVERIFY_DEPLOY_RESULT(res);

        Result<quint64> freeSpace = fileAccess.bytesAvailable(FilePath::fromUserInput(QDir::rootPath()));
        QVERIFY_RESULT(freeSpace);

        qDebug() << "Free Space:" << *freeSpace;
    }

    void testFind()
    {
        CmdBridge::FileAccess fileAccess;
        CmdBridge::FileAccess::DeployResult res = fileAccess.deployAndInit(
            FilePath::fromUserInput(libExecPath),
            FilePath::fromUserInput("/"),
            Environment::systemEnvironment());

        QVERIFY_DEPLOY_RESULT(res);
        fileAccess.iterateDirectory(
            FilePath::fromUserInput(QDir::homePath()),
            [](const FilePath &path, const FilePathInfo &) {
                qDebug() << path;
                return IterationPolicy::Continue;
            },
            FileFilter({}, DirFilterFlag::AllEntries, DirIteratorFlag::NoIteratorFlags));

        TestDFA dfa;
        dfa.iterateDirectory(
            FilePath::fromUserInput(QDir::homePath()),
            [](const FilePath &path, const FilePathInfo &) {
                qDebug() << path;
                return IterationPolicy::Continue;
            },
            FileFilter({}, DirFilterFlag::AllEntries, DirIteratorFlag::NoIteratorFlags));

        // We had a bug where symlinks were filtered by the name of their target
        // instead of the name of the link itself. Test that this is not the case.
        const FilePath testDir = TemporaryDirectory::masterDirectoryFilePath()
                                 / "test-find-symlink-names";
        QVERIFY_RESULT(testDir.ensureWritableDir());

        const FilePath original = testDir / "original-file.txt";
        original.writeFileContents("Hello World");

        QVERIFY_RESULT((original).createSymLink(testDir / "link.txt"));

        QStringList results;

        fileAccess.iterateDirectory(
            testDir,
            [&results](const FilePath &path, const FilePathInfo &) {
                results << path.fileName();
                return IterationPolicy::Continue;
            },
            FileFilter{{"original*"}, DirFilterFlag::AllEntries});

        QCOMPARE(results, QStringList{"original-file.txt"});
    }

    void testBridge()
    {
        CmdBridge::FileAccess fileAccess;
        CmdBridge::FileAccess::DeployResult res = fileAccess.deployAndInit(
            FilePath::fromUserInput(libExecPath),
            FilePath::fromUserInput("/"),
            Environment::systemEnvironment());

        QVERIFY_DEPLOY_RESULT(res);

        QElapsedTimer timer;
        timer.start();
        fileAccess.iterateDirectory(
            FilePath::fromUserInput(QDir::rootPath()),
            [](const FilePath &, const FilePathInfo &) {
                //qDebug() << path.toString() << info.lastModified;
                return IterationPolicy::Continue;
            },
            FileFilter({}, DirFilterFlag::AllEntries, DirIteratorFlag::NoIteratorFlags));
        qDebug() << "CmdBridge Find took:" << timer.restart();

        TestDFA dfa;
        dfa.iterateDirectory(
            FilePath::fromUserInput(QDir::rootPath()),
            [](const FilePath &, const FilePathInfo &) {
                //qDebug() << path.toString() << info.lastModified;
                return IterationPolicy::Continue;
            },
            FileFilter({}, DirFilterFlag::AllEntries, DirIteratorFlag::NoIteratorFlags));
        qDebug() << "Fallback Find took:" << timer.elapsed();
    }

    void testInit()
    {
        Result<FilePath> bridgePath = CmdBridge::Client::getCmdBridgePath(HostOsInfo::hostOs(),
                                                              HostOsInfo::hostArchitecture(),
                                                              FilePath::fromUserInput(libExecPath));
        QTC_ASSERT_RESULT(bridgePath, QSKIP("No bridge found"));

        CmdBridge::Client client(*bridgePath, Environment::systemEnvironment());

        auto result = client.start();
        QTC_ASSERT_RESULT(result, QFAIL("Failed to start bridge"));

        auto lsRes = client.execute({"ls", {"-lach"}});

        lsRes->waitForFinished();

        for (int i = 0; i < lsRes->resultCount(); ++i) {
            auto res = lsRes->resultAt(i);
            if (res.index() == 0) {
                auto [stdOut, stdErr] = std::get<0>(res);
                qDebug() << stdOut;
                qDebug() << stdErr;
            } else {
                auto exitCode = std::get<1>(res);
                qDebug() << "Exit code:" << exitCode;
            }
        }
    }

    void testSocketForward()
    {
        {
            CmdBridge::FileAccess fileAccess;
            CmdBridge::FileAccess::DeployResult res = fileAccess.deployAndInit(
                FilePath::fromUserInput(libExecPath),
                FilePath::fromUserInput("/"),
                Environment::systemEnvironment());

            QVERIFY_DEPLOY_RESULT(res);

            // Set up a local QLocalServer that the forwarding will connect to.
            const QString localServerName = QDir::tempPath() + "/tst-cmdbridge-local.sock";
            QFile::remove(localServerName);
            QLocalServer localServer;
            QVERIFY(localServer.listen(localServerName));

            // Ask the bridge to create a remote socket server and forward to our local one.
            auto forwardResult = fileAccess.forwardLocalSocketServer(localServerName);
            QVERIFY_RESULT(forwardResult);
            auto &forward = *forwardResult;
            QVERIFY(forward);

            const QString remotePath = forward->path().path();
            qDebug() << "Remote socket path:" << remotePath;
            QVERIFY(!remotePath.isEmpty());
            QVERIFY(QFile::exists(remotePath));

            // Connect a "remote app" client to the Go socket server.
            QLocalSocket remoteClient;
            remoteClient.connectToServer(remotePath);
            QVERIFY(remoteClient.waitForConnected(5000));

            // The forwarding chain is asynchronous (QFutureWatcher -> onRemoteConnect
            // -> QLocalSocket::connectToServer) so we must spin the event loop with
            // QTRY_* instead of using blocking waitFor* calls.
            // Note: use hasPendingConnections() inside QTRY_VERIFY because
            // nextPendingConnection() consumes the connection—and QTRY_VERIFY
            // evaluates its expression one extra time in the final QVERIFY.
            QTRY_VERIFY_WITH_TIMEOUT(localServer.hasPendingConnections(), 5000);
            QLocalSocket *localConn = localServer.nextPendingConnection();
            QVERIFY(localConn);

            // Test remote -> local direction.
            // Use += to accumulate: QTRY_VERIFY evaluates the expression one extra
            // time in its final QVERIFY, and readAll() consumes the buffer.
            const QByteArray testData1 = "Hello from remote";
            remoteClient.write(testData1);
            remoteClient.flush();
            QByteArray localReceived;
            QTRY_VERIFY_WITH_TIMEOUT(
                (localReceived += localConn->readAll()).size() >= testData1.size(), 5000);
            QCOMPARE(localReceived, testData1);

            // Test local -> remote direction.
            const QByteArray testData2 = "Hello from local";
            localConn->write(testData2);
            localConn->flush();
            QByteArray remoteReceived;
            QTRY_VERIFY_WITH_TIMEOUT(
                (remoteReceived += remoteClient.readAll()).size() >= testData2.size(), 5000);
            QCOMPARE(remoteReceived, testData2);

            // Test that closing the remote client is detected.
            QSignalSpy localDisconnected(localConn, &QLocalSocket::disconnected);
            remoteClient.disconnectFromServer();
            QTRY_VERIFY_WITH_TIMEOUT(!localDisconnected.isEmpty(), 5000);

            // Test reconnection: a second remote client should be accepted.
            QLocalSocket remoteClient2;
            remoteClient2.connectToServer(remotePath);
            QVERIFY(remoteClient2.waitForConnected(5000));

            QTRY_VERIFY_WITH_TIMEOUT(localServer.hasPendingConnections(), 5000);
            QLocalSocket *localConn2 = localServer.nextPendingConnection();
            QVERIFY(localConn2);

            const QByteArray testData3 = "Second connection";
            remoteClient2.write(testData3);
            remoteClient2.flush();
            QByteArray localReceived2;
            QTRY_VERIFY_WITH_TIMEOUT(
                (localReceived2 += localConn2->readAll()).size() >= testData3.size(), 5000);
            QCOMPARE(localReceived2, testData3);

            // Destroy the forward handle; the remote socket server should be torn down.
            forward.reset();

            // Give the Go side a moment to clean up.
            QTRY_VERIFY_WITH_TIMEOUT(!QFile::exists(remotePath), 5000);

            localServer.close();
            QFile::remove(localServerName);
        }
        auto emptyFlush = []() {
            Utils::futureSynchronizer()->flushFinishedFutures();
            return Utils::futureSynchronizer()->isEmpty();
        };
        QTRY_VERIFY_WITH_TIMEOUT(emptyFlush(), 5000);
    }

    void testSocketForwardMultipleConnections()
    {
        {
            CmdBridge::FileAccess fileAccess;
            CmdBridge::FileAccess::DeployResult res = fileAccess.deployAndInit(
                FilePath::fromUserInput(libExecPath),
                FilePath::fromUserInput("/"),
                Environment::systemEnvironment());

            QVERIFY_DEPLOY_RESULT(res);

            const QString localServerName = QDir::tempPath() + "/tst-cmdbridge-multi.sock";
            QFile::remove(localServerName);
            QLocalServer localServer;
            QVERIFY(localServer.listen(localServerName));

            auto forwardResult = fileAccess.forwardLocalSocketServer(localServerName);
            QVERIFY_RESULT(forwardResult);
            auto &forward = *forwardResult;
            QVERIFY(forward);

            const QString remotePath = forward->path().path();
            QVERIFY(!remotePath.isEmpty());
            QVERIFY(QFile::exists(remotePath));

            // Connect two remote clients sequentially so we can pair each one
            // with the local connection it triggers.
            QLocalSocket remote1;
            remote1.connectToServer(remotePath);
            QVERIFY(remote1.waitForConnected(5000));

            QTRY_VERIFY_WITH_TIMEOUT(localServer.hasPendingConnections(), 5000);
            QLocalSocket *local1 = localServer.nextPendingConnection();
            QVERIFY(local1);

            QLocalSocket remote2;
            remote2.connectToServer(remotePath);
            QVERIFY(remote2.waitForConnected(5000));

            QTRY_VERIFY_WITH_TIMEOUT(localServer.hasPendingConnections(), 5000);
            QLocalSocket *local2 = localServer.nextPendingConnection();
            QVERIFY(local2);

            // remote1 -> local1: data must not appear on local2.
            const QByteArray data1 = "Hello from remote 1";
            remote1.write(data1);
            remote1.flush();
            QByteArray received1;
            QTRY_VERIFY_WITH_TIMEOUT(
                (received1 += local1->readAll()).size() >= data1.size(), 5000);
            QCOMPARE(received1, data1);
            // local2 must stay silent.
            QVERIFY(local2->bytesAvailable() == 0);

            // remote2 -> local2.
            const QByteArray data2 = "Hello from remote 2";
            remote2.write(data2);
            remote2.flush();
            QByteArray received2;
            QTRY_VERIFY_WITH_TIMEOUT(
                (received2 += local2->readAll()).size() >= data2.size(), 5000);
            QCOMPARE(received2, data2);

            // local2 -> remote2.
            const QByteArray reply2 = "Reply to remote 2";
            local2->write(reply2);
            local2->flush();
            QByteArray remoteReceived2;
            QTRY_VERIFY_WITH_TIMEOUT(
                (remoteReceived2 += remote2.readAll()).size() >= reply2.size(), 5000);
            QCOMPARE(remoteReceived2, reply2);

            // Close remote1; local1 must disconnect but local2/remote2 must survive.
            QSignalSpy local1Disconnected(local1, &QLocalSocket::disconnected);
            remote1.disconnectFromServer();
            QTRY_VERIFY_WITH_TIMEOUT(!local1Disconnected.isEmpty(), 5000);

            // Connection 2 is still alive: exchange one more round.
            const QByteArray data3 = "Connection 2 still alive";
            remote2.write(data3);
            remote2.flush();
            QByteArray received3;
            QTRY_VERIFY_WITH_TIMEOUT(
                (received3 += local2->readAll()).size() >= data3.size(), 5000);
            QCOMPARE(received3, data3);

            // Tear down.
            forward.reset();
            QTRY_VERIFY_WITH_TIMEOUT(!QFile::exists(remotePath), 5000);

            localServer.close();
            QFile::remove(localServerName);
        }
        auto emptyFlush = []() {
            Utils::futureSynchronizer()->flushFinishedFutures();
            return Utils::futureSynchronizer()->isEmpty();
        };
        QTRY_VERIFY_WITH_TIMEOUT(emptyFlush(), 5000);
    }

    void testSocketForwardAfterLocalServerClose()
    {
        {
            CmdBridge::FileAccess fileAccess;
            CmdBridge::FileAccess::DeployResult res = fileAccess.deployAndInit(
                FilePath::fromUserInput(libExecPath),
                FilePath::fromUserInput("/"),
                Environment::systemEnvironment());

            QVERIFY_DEPLOY_RESULT(res);

            const QString localServerName = QDir::tempPath() + "/tst-cmdbridge-svrclose.sock";
            QFile::remove(localServerName);
            QLocalServer localServer;
            QVERIFY(localServer.listen(localServerName));

            auto forwardResult = fileAccess.forwardLocalSocketServer(localServerName);
            QVERIFY_RESULT(forwardResult);
            auto &forward = *forwardResult;
            QVERIFY(forward);

            const QString remotePath = forward->path().path();

            // Establish a connection before closing the server.
            QLocalSocket remote;
            remote.connectToServer(remotePath);
            QVERIFY(remote.waitForConnected(5000));

            QTRY_VERIFY_WITH_TIMEOUT(localServer.hasPendingConnections(), 5000);
            QLocalSocket *local = localServer.nextPendingConnection();
            QVERIFY(local);

            // Close the server: new clients can no longer connect, but the
            // already-accepted QLocalSocket must remain open and functional.
            localServer.close();

            // remote -> local: the established pipe must still carry data.
            const QByteArray toLocal = "remote to local after server close";
            remote.write(toLocal);
            remote.flush();
            QByteArray localReceived;
            QTRY_VERIFY_WITH_TIMEOUT(
                (localReceived += local->readAll()).size() >= toLocal.size(), 5000);
            QCOMPARE(localReceived, toLocal);

            // local -> remote.
            const QByteArray toRemote = "local to remote after server close";
            local->write(toRemote);
            local->flush();
            QByteArray remoteReceived;
            QTRY_VERIFY_WITH_TIMEOUT(
                (remoteReceived += remote.readAll()).size() >= toRemote.size(), 5000);
            QCOMPARE(remoteReceived, toRemote);

            forward.reset();
            QFile::remove(localServerName);
        }
        auto emptyFlush = []() {
            Utils::futureSynchronizer()->flushFinishedFutures();
            return Utils::futureSynchronizer()->isEmpty();
        };
        QTRY_VERIFY_WITH_TIMEOUT(emptyFlush(), 5000);
    }

    void testSameFile()
    {
        if (HostOsInfo::isLinuxHost())
            QSKIP("Skipping test on Linux, as it will probably be case sensitive.");

        Result<FilePath> bridgePath = CmdBridge::Client::getCmdBridgePath(
            HostOsInfo::hostOs(),
            HostOsInfo::hostArchitecture(),
            FilePath::fromUserInput(libExecPath));
        QTC_ASSERT_RESULT(bridgePath, QSKIP("No bridge found"));

        CmdBridge::Client client(*bridgePath, Environment::systemEnvironment());
        QTC_ASSERT_RESULT(client.start(), return);

        const FilePath file1 = FilePath::fromUserInput(QDir::tempPath());
        const FilePath file2 = FilePath::fromUserInput(QDir::rootPath());

        auto result = client.isSameFile(file1.nativePath(), file2.nativePath());
        QTC_ASSERT_RESULT(result, QFAIL("isSameFile failed"));
        QVERIFY(!result->result());

        const FilePath testDir = TemporaryDirectory::masterDirectoryFilePath()
                                 / "test-samefile-symlinks";
        QVERIFY_RESULT(testDir.ensureWritableDir());

        const FilePath original = testDir / "original-file.txt";
        original.writeFileContents("Hello World");

        const auto link = testDir / "link.txt";
        QVERIFY_RESULT(original.createSymLink(link));

        result = client.isSameFile(original.path(), link.path());
        QVERIFY_RESULT(result);
        QVERIFY(result->result());

        // Check if path is case in-sensitive:
        const auto uppercased = original.withNewFileName("ORIGINAL-FILE.txt");
        const bool isCaseInsensitive = uppercased.exists();
        if (!isCaseInsensitive)
            QVERIFY_RESULT(uppercased.writeFileContents("Other file"));

        result = client.isSameFile(original.path(), uppercased.path());
        QVERIFY_RESULT(result);
        QCOMPARE(result->result(), isCaseInsensitive);
    }
};

QTEST_GUILESS_MAIN(tst_CmdBridge)

#include "tst_cmdbridge.moc"
