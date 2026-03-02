// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <app/app_version.h>

#include <client/bridgedfileaccess.h>
#include <client/cmdbridgeclient.h>

#include <utils/devicefileaccess.h>
#include <utils/environment.h>
#include <utils/processreaper.h>
#include <utils/qtcprocess.h>
#include <utils/temporarydirectory.h>

#include <QElapsedTimer>
#include <QObject>
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
        QVERIFY(fileAccess.exists(*tempFile).value());
        QVERIFY_RESULT(fileAccess.removeFile(*tempFile));
        QVERIFY_RESULT(fileAccess.exists(*tempFile));
        QVERIFY(!fileAccess.exists(*tempFile).value());

        tempFile = fileAccess.createTempFile(FilePath::fromUserInput(QDir::tempPath())
                                             / "test.XXXXXX");
        QVERIFY_RESULT(tempFile);
        QVERIFY(tempFile->fileName().startsWith("test."));
        QVERIFY(!tempFile->fileName().startsWith("test.XXXXXX"));
        QVERIFY_RESULT(fileAccess.exists(*tempFile));
        QVERIFY(fileAccess.exists(*tempFile).value());
        QVERIFY_RESULT(fileAccess.removeFile(*tempFile));
        QVERIFY(!fileAccess.exists(*tempFile).value());
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
        QVERIFY(fileAccess.exists(*tempDir).value());
        QVERIFY_RESULT(fileAccess.isWritableDirectory(*tempDir));
        QVERIFY(fileAccess.isWritableDirectory(*tempDir).value());
        QVERIFY_RESULT(fileAccess.removeRecursively(*tempDir));
        QVERIFY_RESULT(fileAccess.exists(*tempDir));
        QVERIFY(!fileAccess.exists(*tempDir).value());
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
        QVERIFY(fileAccess.exists(*tempFile).value());
        QVERIFY_RESULT(fileAccess.removeFile(*tempFile));
        QVERIFY_RESULT(fileAccess.exists(*tempFile));
        QVERIFY(!fileAccess.exists(*tempFile).value());
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
        QVERIFY(!fileAccess.isWritableDirectory(testDir).value());
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
        QVERIFY(fileAccess.exists(fileCopy).value());
        QCOMPARE(fileAccess.fileContents(fileCopy, -1, 0), fileContents);

        const FilePath mvTarget = FilePath::fromUserInput(QDir::tempPath() + "/testmoved.txt");
        QVERIFY_RESULT(fileAccess.renameFile(fileCopy, mvTarget));
        QVERIFY_RESULT(fileAccess.exists(mvTarget));
        QVERIFY(fileAccess.exists(mvTarget).value());
        QVERIFY_RESULT(fileAccess.exists(fileCopy));
        QVERIFY(!fileAccess.exists(fileCopy).value());
        QCOMPARE(fileAccess.fileContents(mvTarget, -1, 0), fileContents);

        QVERIFY_RESULT(fileAccess.removeFile(mvTarget));
        QVERIFY_RESULT(fileAccess.removeFile(testFile));
        QVERIFY(!fileAccess.removeFile(testFile));

        QVERIFY_RESULT(fileAccess.ensureExistingFile(testFile));
        QVERIFY_RESULT(fileAccess.removeFile(testFile));

        const FilePath dir = FilePath::fromUserInput(QDir::tempPath() + "/testdir");
        QVERIFY_RESULT(fileAccess.createDirectory(dir));
        QVERIFY_RESULT(fileAccess.exists(dir));
        QVERIFY(fileAccess.exists(dir).value());
        QVERIFY_RESULT(fileAccess.isReadableDirectory(dir));
        QVERIFY(fileAccess.isReadableDirectory(dir).value());
        QVERIFY_RESULT(fileAccess.removeRecursively(dir));
        QVERIFY_RESULT(fileAccess.exists(dir));
        QVERIFY(!fileAccess.exists(dir).value());
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
            FileFilter({}, QDir::AllEntries, QDirIterator::NoIteratorFlags));

        TestDFA dfa;
        dfa.iterateDirectory(
            FilePath::fromUserInput(QDir::homePath()),
            [](const FilePath &path, const FilePathInfo &) {
                qDebug() << path;
                return IterationPolicy::Continue;
            },
            FileFilter({}, QDir::AllEntries, QDirIterator::NoIteratorFlags));

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
            FileFilter{{"original*"}, QDir::AllEntries});

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
            FileFilter({}, QDir::AllEntries, QDirIterator::NoIteratorFlags));
        qDebug() << "CmdBridge Find took:" << timer.restart();

        TestDFA dfa;
        dfa.iterateDirectory(
            FilePath::fromUserInput(QDir::rootPath()),
            [](const FilePath &, const FilePathInfo &) {
                //qDebug() << path.toString() << info.lastModified;
                return IterationPolicy::Continue;
            },
            FileFilter({}, QDir::AllEntries, QDirIterator::NoIteratorFlags));
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
};

QTEST_GUILESS_MAIN(tst_CmdBridge)

#include "tst_cmdbridge.moc"
