// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "utils/futuresynchronizer.h"
#include <app/app_version.h>

#include <client/bridgedfileaccess.h>
#include <client/cmdbridgeclient.h>

#include <utils/async.h>
#include <utils/devicefileaccess.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/processreaper.h>
#include <utils/qtcprocess.h>
#include <utils/temporarydirectory.h>

#include <QDeadlineTimer>
#include <QElapsedTimer>
#include <QLocalServer>
#include <QLocalSocket>
#include <QObject>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTest>
#include <QThread>
#include <QTimer>

#include <atomic>
#include <memory>
#include <optional>
#include <thread>

#ifdef Q_OS_UNIX
#include <sys/resource.h>
#endif

using namespace Utils;

#define QVERIFY_DEPLOY_RESULT(result) \
    QVERIFY2(result, result ? #result : static_cast<const char *>(result.error().message.toUtf8()))

// Some of what the bridge does is POSIX only: deployAndInit() probes the device
// with echo/which/uname, chmod bits mean little on Windows, creating a symlink
// there needs a privilege, and socket forwarding listens on a unix socket. The
// tests covering those skip on a Windows host; the rest run everywhere.
#define SKIP_ON_WINDOWS(what) \
    do { \
        if (HostOsInfo::isWindowsHost()) \
            QSKIP(what " is POSIX only."); \
    } while (false)

static qint64 peakResidentBytes()
{
#ifdef Q_OS_UNIX
    rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) != 0)
        return 0;
#ifdef Q_OS_MACOS
    return usage.ru_maxrss; // In bytes.
#else
    return qint64(usage.ru_maxrss) * 1024; // In KiB.
#endif
#else
    return 0;
#endif
}

// The application on the device listens on a TCP port; the tool connects to
// the forwarded port on this machine. The test process plays the application.
struct TcpForwardFixture
{
    CmdBridge::FileAccess fileAccess;
    QTcpServer app;
    std::unique_ptr<CmdBridge::TcpPortForward> forward;
};

static bool setupTcpForward(TcpForwardFixture &f, const QString &libExecPath)
{
    if (!f.app.listen(QHostAddress::LocalHost, 0))
        return false;
    const CmdBridge::FileAccess::DeployResult res = f.fileAccess.deployAndInit(
        FilePath::fromUserInput(libExecPath),
        FilePath::fromUserInput("/"),
        Environment::systemEnvironment());
    if (!res)
        return false;
    auto forward = f.fileAccess.forwardTcpPort("127.0.0.1", f.app.serverPort());
    if (!forward)
        return false;
    f.forward = std::move(*forward);
    return true;
}

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
    FilePath bridgePath; // The local bridge binary, see initTestCase().

private:
    // The bridge runs on this very host, so there is nothing to deploy and nothing
    // to probe: we know the platform, and deployAndInit() would only shortcut to
    // init() anyway. Going straight to init() is what lets these tests run on
    // Windows, where the probing (echo, which, uname) has no chance.
    Result<> initFileAccess(CmdBridge::FileAccess &fileAccess)
    {
        return fileAccess.init(bridgePath, Environment::systemEnvironment(), false);
    }

private slots:
    void initTestCase()
    {
        TemporaryDirectory::setMasterTemporaryDirectory(
            QDir::tempPath() + "/" + Core::Constants::IDE_CASED_ID + "-XXXXXX");

        // Every test here talks to a real bridge, and there is none to talk to in a
        // build configured with -DBUILD_EXECUTABLE_CMDBRIDGE=OFF. Skipping here skips
        // them all.
        const Result<FilePath> path = CmdBridge::Client::getCmdBridgePath(
            HostOsInfo::hostOs(),
            HostOsInfo::hostArchitecture(),
            FilePath::fromUserInput(libExecPath));
        if (!path)
            QSKIP(qPrintable(path.error()));
        bridgePath = *path;
    }

    void cleanupTestCase() { ProcessReaper::deleteAll(); }

    void testDeviceEnvironment()
    {
        SKIP_ON_WINDOWS("Deploying the bridge");

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
        SKIP_ON_WINDOWS("Setting permission bits");

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
        QVERIFY_RESULT(initFileAccess(fileAccess));

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
        QVERIFY_RESULT(initFileAccess(fileAccess));

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
        QVERIFY_RESULT(initFileAccess(fileAccess));

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
        QVERIFY_RESULT(initFileAccess(fileAccess));

        const FilePath testDir = FilePath::fromUserInput(QDir::tempPath() + "/testIsWritableDir");
        QVERIFY_RESULT(fileAccess.isWritableDirectory(testDir));
        QVERIFY(!*fileAccess.isWritableDirectory(testDir));
    }

    void testEnsureWritableDir()
    {
        CmdBridge::FileAccess fileAccess;
        QVERIFY_RESULT(initFileAccess(fileAccess));

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
        QVERIFY_RESULT(initFileAccess(fileAccess));

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
        CmdBridge::Client client(bridgePath, Environment::systemEnvironment());
        client.start();

        // Ask about paths we know the state of, instead of POSIX ones that do not
        // exist on a Windows host.
        const FilePath existing = TemporaryDirectory::masterDirectoryFilePath();
        const FilePath missing = existing / "idonotexist";

        bool result = client.is(existing.nativePath(), CmdBridge::Client::Is::Dir)->result();
        QVERIFY(result);

        result = client.is(missing.nativePath(), CmdBridge::Client::Is::Dir)->result();
        QVERIFY(!result);
        result = client.is(missing.nativePath(), CmdBridge::Client::Is::Exists)->result();
        QVERIFY(!result);
    }

    void testStat()
    {
        CmdBridge::Client client(bridgePath, Environment::systemEnvironment());
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
        SKIP_ON_WINDOWS("Creating symlinks");

        CmdBridge::FileAccess fileAccess;
        CmdBridge::FileAccess::DeployResult res = fileAccess.deployAndInit(
            FilePath::fromUserInput(libExecPath),
            FilePath::fromUserInput("/"),
            Environment::systemEnvironment());

        QVERIFY_DEPLOY_RESULT(res);

        // Create the link instead of relying on /tmp being one, which only holds
        // on macOS.
        const FilePath testDir = TemporaryDirectory::masterDirectoryFilePath()
                                 / "test-symlink-target";
        QVERIFY_RESULT(testDir.ensureWritableDir());

        const FilePath original = testDir / "original-file.txt";
        QVERIFY_RESULT(original.writeFileContents("Hello World"));
        const FilePath link = testDir / "link.txt";
        QVERIFY_RESULT(original.createSymLink(link));

        const Result<FilePath> target = fileAccess.symLinkTarget(link);
        QVERIFY_RESULT(target);
        QCOMPARE(target->path(), original.path());
    }

    void testFileId()
    {
        CmdBridge::FileAccess fileAccess;
        QVERIFY_RESULT(initFileAccess(fileAccess));

        Result<QByteArray> fileId = fileAccess.fileId(FilePath::fromUserInput(QDir::rootPath()));
        QVERIFY_RESULT(fileId);

        qDebug() << "File Id:" << *fileId;
    }

    void testFreeSpace()
    {
        CmdBridge::FileAccess fileAccess;
        QVERIFY_RESULT(initFileAccess(fileAccess));

        Result<quint64> freeSpace = fileAccess.bytesAvailable(FilePath::fromUserInput(QDir::rootPath()));
        QVERIFY_RESULT(freeSpace);

        qDebug() << "Free Space:" << *freeSpace;
    }

    void testFind()
    {
        CmdBridge::FileAccess fileAccess;
        QVERIFY_RESULT(initFileAccess(fileAccess));

        // Walk a tree we control, and require the bridge to report the same entries
        // as the shell fallback does. That fallback shells out to "find" and "stat",
        // so it can only be compared against on a Unix host.
        const FilePath walkDir = TemporaryDirectory::masterDirectoryFilePath() / "test-find-walk";
        QVERIFY_RESULT(walkDir.ensureWritableDir());
        for (const char *const &name : {"one.txt", "two.txt", "three.txt"})
            QVERIFY_RESULT((walkDir / name).writeFileContents("Hello World"));
        QVERIFY_RESULT((walkDir / "subdir").ensureWritableDir());

        const FileFilter walkFilter({},
                                    DirFilterFlag::AllEntries | DirFilterFlag::NoDotAndDotDot,
                                    DirIteratorFlag::NoIteratorFlags);
        const auto collector = [](QStringList *names) {
            return [names](const FilePath &path, const FilePathInfo &) {
                *names << path.fileName();
                return IterationPolicy::Continue;
            };
        };

        QStringList viaBridge;
        fileAccess.iterateDirectory(walkDir, collector(&viaBridge), walkFilter);
        viaBridge.sort();
        QCOMPARE(viaBridge, QStringList({"one.txt", "subdir", "three.txt", "two.txt"}));

        // The rest needs the shell fallback's tools, and creating a symlink needs a
        // privilege on Windows.
        if (HostOsInfo::isWindowsHost())
            return;

        TestDFA dfa;
        QStringList viaFallback;
        dfa.iterateDirectory(walkDir, collector(&viaFallback), walkFilter);
        viaFallback.sort();
        QCOMPARE(viaFallback, viaBridge);

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
        QVERIFY_RESULT(initFileAccess(fileAccess));

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

        if (HostOsInfo::isWindowsHost())
            return; // The fallback runs "ls".

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
        CmdBridge::Client client(bridgePath, Environment::systemEnvironment());

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

    void testTeardown()
    {
        std::optional<ProcessResultData> resultData;
        Result<QFuture<CmdBridge::Client::Stat>> pending = ResultError(QString("not started"));
        {
            CmdBridge::Client client(bridgePath, Environment::systemEnvironment());
            QVERIFY_RESULT(client.start());

            // Direct connection: the client reports this from its own thread, and waiting
            // for that thread is the last thing its destructor does.
            connect(
                &client,
                &CmdBridge::Client::done,
                this,
                [&resultData](const ProcessResultData &data) { resultData = data; },
                Qt::DirectConnection);

            pending = client.stat(QDir::rootPath());
        }

        // Whether the job made it through or not, it must not be left pending, and one
        // that did not must carry the error the handlers report. A promise that is
        // merely destroyed cancels its future too, so finished alone proves nothing.
        QVERIFY_RESULT(pending);
        QVERIFY(pending->isFinished());
        if (pending->isCanceled())
            QVERIFY_THROWS_EXCEPTION(std::exception, pending->waitForFinished());

        // The bridge is asked to exit and does so by itself, it must not be killed.
        QVERIFY(resultData);
        QCOMPARE(resultData->m_exitStatus, ProcessExitStatus::NormalExit);
        QCOMPARE(resultData->m_exitCode, 0);
    }

    void testTeardownOfUnresponsiveBridge()
    {
        // "cat" stands in for a bridge that answers nothing and does not exit by itself.
        // Tearing the client down must wait for neither.
        const FilePath cat = Environment::systemEnvironment().searchInPath("cat");
        if (cat.isEmpty())
            QSKIP("No \"cat\" to stand in for an unresponsive bridge");

        // Owned by the worker: when it hangs, the test fails and leaves this scope
        // while the worker is still running.
        const auto startResult = std::make_shared<Result<>>(ResultOk);
        // Tear down on a worker so that a client that does hang fails the test instead
        // of blocking it forever.
        const QFuture<void> teardown = asyncRun([cat, startResult] {
            CmdBridge::Client client(cat, Environment::systemEnvironment());
            *startResult = client.start();
        });

        // Above threadStopTimeout, so the bound does not race the timeout it bounds.
        QTRY_VERIFY_WITH_TIMEOUT(teardown.isFinished(), 30000);
        QVERIFY_RESULT(*startResult);
    }

    // init() asks the bridge one question before reporting success, so a binary that
    // cannot answer must be reported rather than handed out as working file access.
    void testInitOfSilentBridge_data()
    {
        QTest::addColumn<QString>("standIn");
        // Gone before answering, cleanly and not: either way the exit is what
        // concludes the question left pending.
        QTest::newRow("exits") << QString("true");
        QTest::newRow("fails") << QString("false");
        // Answers nothing and does not exit either: only the bound ends the wait.
        QTest::newRow("hangs") << QString("cat");
    }

    void testInitOfSilentBridge()
    {
        QFETCH(QString, standIn);

        const FilePath standInPath = Environment::systemEnvironment().searchInPath(standIn);
        if (standInPath.isEmpty())
            QSKIP(qPrintable(QString("No \"%1\" to stand in for a silent bridge").arg(standIn)));

        // init() reads its bound off the environment, so shorten it: the "hangs" row
        // has to wait the whole of it, and the default is meant to be generous.
        Environment::modifySystemEnvironment({{"QTC_CMDBRIDGE_ANSWER_TIMEOUT", "2"}});
        const QScopeGuard resetTimeout([] {
            Environment::modifySystemEnvironment(
                {{"QTC_CMDBRIDGE_ANSWER_TIMEOUT", {}, EnvironmentItem::Unset}});
        });

        // Owned by the worker: when init() hangs, the test fails and leaves this scope
        // while the worker is still running.
        const auto initResult = std::make_shared<Result<>>(ResultOk);
        // On a worker so that an init() that does hang fails the test instead of
        // blocking it forever.
        const QFuture<void> init = asyncRun([standInPath, initResult] {
            CmdBridge::FileAccess fileAccess;
            *initResult = fileAccess.init(standInPath, Environment::systemEnvironment(), false);
        });

        // Well above the bound applied above, so this does not race the timeout it bounds.
        QTRY_VERIFY_WITH_TIMEOUT(init.isFinished(), 30000);
        QVERIFY2(!*initResult, "A bridge that never answered was reported as working.");

        // Whatever ended the wait, the report has to name it. A process that merely
        // exited has no errorString(), which left the message saying no more than that
        // there was no answer.
        const QString error = initResult->error();
        QVERIFY2(!error.endsWith(": "), qPrintable(error));
        qDebug() << error;
    }

    // The wait for that first answer must not run an event loop: init() reaches the
    // main thread for an Android device, where pumping events lets the very adb
    // notification that started the connect arrive again, mid-deploy.
    void testInitDoesNotPumpEvents()
    {
        const FilePath cat = Environment::systemEnvironment().searchInPath("cat");
        if (cat.isEmpty())
            QSKIP("No \"cat\" to stand in for an unresponsive bridge");

        Environment::modifySystemEnvironment({{"QTC_CMDBRIDGE_ANSWER_TIMEOUT", "2"}});
        const QScopeGuard resetTimeout([] {
            Environment::modifySystemEnvironment(
                {{"QTC_CMDBRIDGE_ANSWER_TIMEOUT", {}, EnvironmentItem::Unset}});
        });

        // Due well within the bound below, but only ever delivered by an event loop.
        // A timer object rather than singleShot(): the shot must die with this
        // stack frame, or it fires into it from a later test's event processing.
        bool delivered = false;
        QTimer probe;
        probe.setSingleShot(true);
        QObject::connect(&probe, &QTimer::timeout, [&delivered] { delivered = true; });
        probe.start(100);

        {
            CmdBridge::FileAccess fileAccess;
            QVERIFY(!fileAccess.init(cat, Environment::systemEnvironment(), false));

            // init() has returned, so whatever it processed, it processed before now.
            QVERIFY2(!delivered, "The wait for the first answer ran an event loop.");
        }

        // On the main thread the access hands the teardown of a bridge that has to be
        // killed to the synchronizer, and leaving that to overlap the next test is
        // what makes a failure there look like this one's.
        auto emptyFlush = []() {
            Utils::futureSynchronizer()->flushFinishedFutures();
            return Utils::futureSynchronizer()->isEmpty();
        };
        QTRY_VERIFY_WITH_TIMEOUT(emptyFlush(), 10000);
    }

    // Job handlers run on the bridge thread, and a continuation attached without a
    // launch policy runs wherever the promise is concluded - which is that very
    // handler, inside readPacket(). Asking for another job from there deadlocked as
    // long as readPacket() ran the handler while holding the jobs lock for reading,
    // because registering a job asks for the same lock for writing.
    // Note: without that fix this does not merely fail, it leaves the bridge thread
    // stuck, so the run does not shut down cleanly.
    void testNestedRequestFromHandler()
    {
        // The continuation runs on the bridge thread and may outlive this function, so
        // everything it touches is shared with it, the client included.
        auto client
            = std::make_shared<CmdBridge::Client>(bridgePath, Environment::systemEnvironment());
        QVERIFY_RESULT(client->start());

        const QString root = QDir::rootPath();
        auto ranOffMainThread = std::make_shared<std::atomic_bool>(false);
        auto nested = std::make_shared<std::optional<QFuture<CmdBridge::Client::Stat>>>();
        auto nestedReady = std::make_shared<std::atomic_bool>(false);

        Result<QFuture<CmdBridge::Client::Stat>> first = client->stat(root);
        QVERIFY_RESULT(first);

        QFuture<void> chain = first->then(
            [client, root, ranOffMainThread, nested, nestedReady](CmdBridge::Client::Stat) {
                *ranOffMainThread = !QThread::isMainThread();
                if (Result<QFuture<CmdBridge::Client::Stat>> second = client->stat(root))
                    *nested = *second;
                // Stored last, so that observing it means the write above is visible.
                *nestedReady = true;
            });

        QTRY_VERIFY(nestedReady->load());
        QVERIFY(nested->has_value());
        QTRY_VERIFY((*nested)->isFinished());

        if (!*ranOffMainThread) {
            QSKIP("The first job finished before the continuation was attached, so it "
                  "ran inline on this thread and the handler path was not exercised.");
        }
    }

    // What the queue hands over is gone from it: unlike the results of a
    // QFuture, nothing keeps a copy for the life of the connection.
    void testTcpEventsAreHandedOver()
    {
        int wakes = 0;
        CmdBridge::Client::TcpEvents events([&wakes] { ++wakes; });
        QVERIFY(events.take().isEmpty());

        events.push({CmdBridge::Client::TcpEvents::Data, "one"});
        events.push({CmdBridge::Client::TcpEvents::Data, "two"});
        events.push({CmdBridge::Client::TcpEvents::Closed, {}});
        QCOMPARE(wakes, 1); // Only the transition from empty needs a wake-up.

        const auto taken = events.take();
        QCOMPARE(taken.size(), 3);
        QCOMPARE(taken.at(0).bytes, QByteArray("one"));
        QCOMPARE(taken.at(1).bytes, QByteArray("two"));
        QCOMPARE(taken.at(2).kind, CmdBridge::Client::TcpEvents::Closed);
        QVERIFY(events.take().isEmpty());

        events.push({CmdBridge::Client::TcpEvents::Stopped, {}});
        QCOMPARE(wakes, 2);
    }

    // After detach() the wake-up must neither run nor be running, as what it
    // reaches may be deleted right afterwards.
    void testTcpEventsDetachWaitsForTheWakeUp()
    {
        std::atomic<bool> inWake = false;
        std::atomic<bool> wakeFinished = false;
        CmdBridge::Client::TcpEvents events([&] {
            inWake = true;
            QThread::msleep(200);
            wakeFinished = true;
        });

        std::thread pusher([&events] {
            events.push({CmdBridge::Client::TcpEvents::Data, "x"});
        });
        QTRY_VERIFY_WITH_TIMEOUT(inWake.load(), 5000);
        events.detach();
        QVERIFY(wakeFinished.load());
        pusher.join();

        // Later events are kept, but nobody is woken for them.
        events.take();
        int wakes = 0;
        CmdBridge::Client::TcpEvents detached([&wakes] { ++wakes; });
        detached.detach();
        detached.push({CmdBridge::Client::TcpEvents::Data, "y"});
        QCOMPARE(wakes, 0);
        QCOMPARE(detached.take().size(), 1);
    }

    // The handle is destroyed while the device still streams into it; what is in
    // flight must not reach the destroyed handle.
    void testTcpForwardDestroyWhileDataStreams()
    {
        SKIP_ON_WINDOWS("TCP forwarding");

        for (int round = 0; round < 20; ++round) {
            TcpForwardFixture f;
            QVERIFY(setupTcpForward(f, libExecPath));

            QTcpSocket tool;
            tool.connectToHost(QHostAddress::LocalHost, f.forward->localPort());
            QVERIFY(tool.waitForConnected(5000));
            QTRY_VERIFY_WITH_TIMEOUT(f.app.hasPendingConnections(), 5000);
            QTcpSocket *app = f.app.nextPendingConnection();
            QVERIFY(app);

            const QByteArray chunk(16384, 'z');
            for (int i = 0; i < 200; ++i)
                app->write(chunk);
            app->flush();
            QTRY_VERIFY_WITH_TIMEOUT(tool.bytesAvailable() > 0, 5000);

            QSignalSpy toolDisconnected(&tool, &QTcpSocket::disconnected);
            f.forward.reset();
            QTRY_VERIFY_WITH_TIMEOUT(!toolDisconnected.isEmpty(), 5000);
        }
    }

    // A connection that lives as long as a profiling session must not hold on
    // to what the application sent through it.
    void testTcpForwardDoesNotRetainDeliveredData()
    {
        SKIP_ON_WINDOWS("TCP forwarding");

        TcpForwardFixture f;
        QVERIFY(setupTcpForward(f, libExecPath));

        QTcpSocket tool;
        tool.connectToHost(QHostAddress::LocalHost, f.forward->localPort());
        QVERIFY(tool.waitForConnected(5000));
        QTRY_VERIFY_WITH_TIMEOUT(f.app.hasPendingConnections(), 5000);
        QTcpSocket *app = f.app.nextPendingConnection();
        QVERIFY(app);

        constexpr qint64 total = 256LL * 1024 * 1024;
        const QByteArray chunk(65536, 'z');
        const qint64 before = peakResidentBytes();
        qint64 sent = 0;
        qint64 received = 0;
        QElapsedTimer timer;
        timer.start();
        while (received < total && timer.elapsed() < 120000) {
            while (sent < total && app->bytesToWrite() < 4 * 1024 * 1024) {
                app->write(chunk);
                sent += chunk.size();
            }
            QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents, 5);
            received += tool.readAll().size();
        }
        QCOMPARE(received, total);

        // Keeping everything would have raised the peak by as much as was sent.
        const qint64 grown = peakResidentBytes() - before;
        QVERIFY2(grown < total / 2,
                 qPrintable(QString("The peak memory grew by %1 MiB while %2 MiB were delivered")
                                .arg(grown / 1048576)
                                .arg(total / 1048576)));
    }


    void testTcpForward()
    {
        SKIP_ON_WINDOWS("TCP forwarding");

        TcpForwardFixture f;
        QVERIFY(setupTcpForward(f, libExecPath));
        QVERIFY(f.forward->localPort() > 0);

        QTcpSocket tool;
        tool.connectToHost(QHostAddress::LocalHost, f.forward->localPort());
        QVERIFY(tool.waitForConnected(5000));

        // Sent before the bridge has necessarily finished dialing.
        tool.write("early");
        QTRY_VERIFY_WITH_TIMEOUT(f.app.hasPendingConnections(), 5000);
        QTcpSocket *app = f.app.nextPendingConnection();
        QVERIFY(app);
        QByteArray fromTool;
        QTRY_VERIFY_WITH_TIMEOUT((fromTool += app->readAll()).size() >= 5, 5000);
        QCOMPARE(fromTool, QByteArray("early"));

        app->write("answer");
        QByteArray fromApp;
        QTRY_VERIFY_WITH_TIMEOUT((fromApp += tool.readAll()).size() >= 6, 5000);
        QCOMPARE(fromApp, QByteArray("answer"));

        // The tool hangs up: the application sees it.
        QSignalSpy appDisconnected(app, &QTcpSocket::disconnected);
        tool.disconnectFromHost();
        QTRY_VERIFY_WITH_TIMEOUT(!appDisconnected.isEmpty(), 5000);

        // A second connection through the same forward works independently.
        QTcpSocket tool2;
        tool2.connectToHost(QHostAddress::LocalHost, f.forward->localPort());
        QVERIFY(tool2.waitForConnected(5000));
        QTRY_VERIFY_WITH_TIMEOUT(f.app.hasPendingConnections(), 5000);
        QTcpSocket *app2 = f.app.nextPendingConnection();
        QVERIFY(app2);

        // The application hangs up: the tool sees it.
        QSignalSpy toolDisconnected(&tool2, &QTcpSocket::disconnected);
        app2->write("bye");
        app2->disconnectFromHost();
        QTRY_VERIFY_WITH_TIMEOUT(!toolDisconnected.isEmpty(), 5000);
        QCOMPARE(tool2.readAll(), QByteArray("bye"));
    }

    void testTcpForwardRefusedDialClosesTheTool()
    {
        SKIP_ON_WINDOWS("TCP forwarding");

        TcpForwardFixture f;
        QVERIFY(setupTcpForward(f, libExecPath));
        f.app.close(); // Nothing listens on the forwarded port any more.

        QTcpSocket tool;
        QSignalSpy toolDisconnected(&tool, &QTcpSocket::disconnected);
        tool.connectToHost(QHostAddress::LocalHost, f.forward->localPort());
        QVERIFY(tool.waitForConnected(5000));
        QTRY_VERIFY_WITH_TIMEOUT(!toolDisconnected.isEmpty(), 10000);
    }

    void testTcpForwardManyConnectionsLargeData()
    {
        SKIP_ON_WINDOWS("TCP forwarding");

        TcpForwardFixture f;
        QVERIFY(setupTcpForward(f, libExecPath));

        constexpr int connections = 8;
        constexpr int size = 4 * 1024 * 1024;
        QByteArray payload(size, 0);
        for (int i = 0; i < size; ++i)
            payload[i] = char((i * 31) ^ (i >> 8));

        QList<std::shared_ptr<QTcpSocket>> tools;
        for (int i = 0; i < connections; ++i) {
            auto tool = std::make_shared<QTcpSocket>();
            tool->connectToHost(QHostAddress::LocalHost, f.forward->localPort());
            QVERIFY(tool->waitForConnected(5000));
            tools.append(tool);
        }
        QTRY_COMPARE_WITH_TIMEOUT(
            [&] {
                // Accept as they come; nextPendingConnection() consumes them.
                return int(f.app.hasPendingConnections());
            }(),
            1,
            5000);

        // Every tool sends the whole payload; the application echoes it back.
        QList<QTcpSocket *> apps;
        QList<QByteArray> echoed(connections), received(connections);
        QTRY_VERIFY_WITH_TIMEOUT(
            [&] {
                while (f.app.hasPendingConnections())
                    apps.append(f.app.nextPendingConnection());
                return apps.size() == connections;
            }(),
            10000);
        for (QTcpSocket *app : std::as_const(apps)) {
            connect(app, &QTcpSocket::readyRead, app, [app] { app->write(app->readAll()); });
            if (app->bytesAvailable() > 0)
                app->write(app->readAll());
        }
        for (const auto &tool : std::as_const(tools))
            tool->write(payload);

        QTRY_VERIFY_WITH_TIMEOUT(
            [&] {
                for (int i = 0; i < connections; ++i)
                    received[i] += tools[i]->readAll();
                return std::all_of(received.cbegin(), received.cend(), [&](const QByteArray &r) {
                    return r.size() >= size;
                });
            }(),
            60000);
        for (const QByteArray &r : std::as_const(received)) {
            int firstDiff = -1;
            for (int i = 0; i < qMin(r.size(), payload.size()); ++i) {
                if (r[i] != payload[i]) {
                    firstDiff = i;
                    break;
                }
            }
            QVERIFY2(r == payload,
                     qPrintable(QString("size %1 of %2, first difference at %3")
                                    .arg(r.size()).arg(payload.size()).arg(firstDiff)));
        }
    }

    void testTcpForwardDestroyClosesConnections()
    {
        SKIP_ON_WINDOWS("TCP forwarding");

        TcpForwardFixture f;
        QVERIFY(setupTcpForward(f, libExecPath));

        QTcpSocket tool;
        tool.connectToHost(QHostAddress::LocalHost, f.forward->localPort());
        QVERIFY(tool.waitForConnected(5000));
        QTRY_VERIFY_WITH_TIMEOUT(f.app.hasPendingConnections(), 5000);
        QTcpSocket *app = f.app.nextPendingConnection();
        QVERIFY(app);

        QSignalSpy appDisconnected(app, &QTcpSocket::disconnected);
        QSignalSpy toolDisconnected(&tool, &QTcpSocket::disconnected);
        f.forward.reset();
        QTRY_VERIFY_WITH_TIMEOUT(!appDisconnected.isEmpty(), 5000);
        QTRY_VERIFY_WITH_TIMEOUT(!toolDisconnected.isEmpty(), 5000);
    }

    // A short-lived tool writes and hangs up before the bridge has dialed.
    void testTcpForwardKeepsDataSentBeforeTheDial()
    {
        SKIP_ON_WINDOWS("TCP forwarding");

        TcpForwardFixture f;
        QVERIFY(setupTcpForward(f, libExecPath));

        QTcpSocket tool;
        tool.connectToHost(QHostAddress::LocalHost, f.forward->localPort());
        QVERIFY(tool.waitForConnected(5000));
        tool.write("early");
        tool.disconnectFromHost();

        QTRY_VERIFY_WITH_TIMEOUT(f.app.hasPendingConnections(), 5000);
        QTcpSocket *app = f.app.nextPendingConnection();
        QVERIFY(app);
        QByteArray fromTool;
        QTRY_VERIFY_WITH_TIMEOUT((fromTool += app->readAll(),
                                  app->state() == QAbstractSocket::UnconnectedState),
                                 5000);
        fromTool += app->readAll();
        QCOMPARE(fromTool, QByteArray("early"));
    }

    // The tool writes its last bytes and hangs up at once on an established
    // connection. The stop of the connection must not overtake the bytes.
    void testTcpForwardKeepsDataSentBeforeHangingUp()
    {
        SKIP_ON_WINDOWS("TCP forwarding");

        TcpForwardFixture f;
        QVERIFY(setupTcpForward(f, libExecPath));

        // Losing the bytes was a race, so one round would not reliably show it.
        for (int round = 0; round < 10; ++round) {
            QTcpSocket tool;
            tool.connectToHost(QHostAddress::LocalHost, f.forward->localPort());
            QVERIFY(tool.waitForConnected(5000));
            QTRY_VERIFY_WITH_TIMEOUT(f.app.hasPendingConnections(), 5000);
            QTcpSocket *app = f.app.nextPendingConnection();
            QVERIFY(app);

            // Once the answer is through, the connection is established.
            app->write("hi");
            QByteArray fromApp;
            QTRY_VERIFY_WITH_TIMEOUT((fromApp += tool.readAll()).size() >= 2, 5000);

            tool.write("bye");
            tool.disconnectFromHost();

            QByteArray fromTool;
            QTRY_VERIFY_WITH_TIMEOUT((fromTool += app->readAll(),
                                      app->state() == QAbstractSocket::UnconnectedState),
                                     5000);
            fromTool += app->readAll();
            QVERIFY2(fromTool == "bye",
                     qPrintable(QString("round %1: \"%2\"").arg(round)
                                    .arg(QString::fromUtf8(fromTool))));
            delete app;
        }
    }

    // The forward goes away while the bridge is still dialing. Once the dial
    // succeeds, the connection on the device must be closed all the same.
    void testTcpForwardDestroyDuringDialClosesConnection()
    {
        SKIP_ON_WINDOWS("TCP forwarding");

        TcpForwardFixture f;
        QVERIFY(setupTcpForward(f, libExecPath));

        QTcpSocket tool;
        tool.connectToHost(QHostAddress::LocalHost, f.forward->localPort());
        QVERIFY(tool.waitForConnected(5000));

        // Let the forward accept and dial, but stop as soon as the dial reaches the
        // application: waitForNewConnection() does not run the event loop, so the
        // forward does not get to see the dial finish.
        QDeadlineTimer deadline(5000);
        bool dialed = false;
        while (!deadline.hasExpired() && !(dialed = f.app.waitForNewConnection(10)))
            QCoreApplication::processEvents();
        QVERIFY(dialed);
        QTcpSocket *app = f.app.nextPendingConnection();
        QVERIFY(app);

        QSignalSpy appDisconnected(app, &QTcpSocket::disconnected);
        f.forward.reset();
        QTRY_VERIFY_WITH_TIMEOUT(!appDisconnected.isEmpty(), 5000);
    }

    void testTcpForwardRejectsInvalidPorts()
    {
        SKIP_ON_WINDOWS("TCP forwarding");

        TcpForwardFixture f;
        QVERIFY(setupTcpForward(f, libExecPath));

        QVERIFY(!f.fileAccess.forwardTcpPort("127.0.0.1", 0));
        QVERIFY(!f.fileAccess.forwardTcpPort("127.0.0.1", 70000));
        QVERIFY(!f.fileAccess.forwardTcpPort("127.0.0.1", f.app.serverPort(), -1));
        QVERIFY(!f.fileAccess.forwardTcpPort("127.0.0.1", f.app.serverPort(), 70000));
    }

    void testTcpForwardToIPv6Address()
    {
        SKIP_ON_WINDOWS("TCP forwarding");

        TcpForwardFixture f;
        QVERIFY(setupTcpForward(f, libExecPath));
        QTcpServer app;
        if (!app.listen(QHostAddress::LocalHostIPv6, 0))
            QSKIP("No IPv6 loopback.");

        auto forward = f.fileAccess.forwardTcpPort("::1", app.serverPort());
        if (!forward)
            QFAIL(qPrintable(forward.error()));

        QTcpSocket tool;
        tool.connectToHost(QHostAddress::LocalHost, (*forward)->localPort());
        QVERIFY(tool.waitForConnected(5000));
        tool.write("six");
        QTRY_VERIFY_WITH_TIMEOUT(app.hasPendingConnections(), 5000);
        QTcpSocket *appSocket = app.nextPendingConnection();
        QVERIFY(appSocket);
        QByteArray fromTool;
        QTRY_VERIFY_WITH_TIMEOUT((fromTool += appSocket->readAll()).size() >= 3, 5000);
        QCOMPARE(fromTool, QByteArray("six"));
    }


    void testSocketForward()
    {
        SKIP_ON_WINDOWS("Socket forwarding");

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
        SKIP_ON_WINDOWS("Socket forwarding");

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
        SKIP_ON_WINDOWS("Socket forwarding");

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

        CmdBridge::Client client(bridgePath, Environment::systemEnvironment());
        QTC_ASSERT_RESULT(client.start(), return);

        const FilePath file1 = FilePath::fromUserInput(QDir::tempPath());
        const FilePath file2 = FilePath::fromUserInput(QDir::rootPath());

        auto result = client.isSameFile(file1.nativePath(), file2.nativePath());
        QTC_ASSERT_RESULT(result, QFAIL("isSameFile failed"));
        QVERIFY(!result->result());

        const FilePath testDir = TemporaryDirectory::masterDirectoryFilePath() / "test-samefile";
        QVERIFY_RESULT(testDir.ensureWritableDir());

        const FilePath original = testDir / "original-file.txt";
        QVERIFY_RESULT(original.writeFileContents("Hello World"));

        // Check if path is case in-sensitive:
        const auto uppercased = original.withNewFileName("ORIGINAL-FILE.txt");
        const bool isCaseInsensitive = uppercased.exists();
        if (!isCaseInsensitive)
            QVERIFY_RESULT(uppercased.writeFileContents("Other file"));

        result = client.isSameFile(original.path(), uppercased.path());
        QVERIFY_RESULT(result);
        QCOMPARE(result->result(), isCaseInsensitive);
    }

    void testSameFileSymLink()
    {
        SKIP_ON_WINDOWS("Creating a symlink");

        CmdBridge::Client client(bridgePath, Environment::systemEnvironment());
        QTC_ASSERT_RESULT(client.start(), return);

        const FilePath testDir = TemporaryDirectory::masterDirectoryFilePath()
                                 / "test-samefile-symlinks";
        QVERIFY_RESULT(testDir.ensureWritableDir());

        const FilePath original = testDir / "original-file.txt";
        QVERIFY_RESULT(original.writeFileContents("Hello World"));

        const auto link = testDir / "link.txt";
        QVERIFY_RESULT(original.createSymLink(link));

        const Result<QFuture<bool>> result = client.isSameFile(original.path(), link.path());
        QVERIFY_RESULT(result);
        QVERIFY(result->result());
    }
};

QTEST_GUILESS_MAIN(tst_CmdBridge)

#include "tst_cmdbridge.moc"
