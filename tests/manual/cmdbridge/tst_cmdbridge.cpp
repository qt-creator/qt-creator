// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <app/app_version.h>

#include <client/bridgedfileaccess.h>
#include <client/cmdbridgeclient.h>

#include <utils/devicefileaccess.h>
#include <utils/environment.h>
#include <utils/qtcprocess.h>
#include <utils/singleton.h>
#include <utils/temporarydirectory.h>

#include <QObject>
#include <QtConcurrent>
#include <QtTest>

using namespace Utils;

class TestDFA : public UnixDeviceFileAccess
{
    friend class tst_CmdBridge;

public:
    using UnixDeviceFileAccess::UnixDeviceFileAccess;

    RunResult runInShell(const CommandLine &cmdLine, const QByteArray &inputData = {}) const override
    {
        // Note: Don't convert into Utils::Process. See more comments in this change in gerrit.
        QProcess p;
        p.setProgram(cmdLine.executable().toString());
        p.setArguments(cmdLine.splitArguments());
        p.setProcessChannelMode(QProcess::SeparateChannels);

        p.start();
        p.waitForStarted();
        if (inputData.size() > 0) {
            p.write(inputData);
            p.closeWriteChannel();
        }
        p.waitForFinished();
        return {p.exitCode(), p.readAllStandardOutput(), p.readAllStandardError()};
    }

    void findUsingLs(const QString &current, const FileFilter &filter, QStringList *found)
    {
        UnixDeviceFileAccess::findUsingLs(current, filter, found, {});
    }
};

class tst_CmdBridge : public QObject
{
    Q_OBJECT

    QString libExecPath;

private slots:
    void initTestCase()
    {
        TemporaryDirectory::setMasterTemporaryDirectory(
            QDir::tempPath() + "/" + Core::Constants::IDE_CASED_ID + "-XXXXXX");
    }

    void cleanupTestCase() { Singleton::deleteAll(); }

    void testDeviceEnvironment()
    {
        CmdBridge::FileAccess fileAccess;
        auto res = fileAccess.deployAndInit(
            FilePath::fromUserInput(libExecPath), FilePath::fromUserInput("/"));

        QVERIFY(res);

        Environment env = fileAccess.deviceEnvironment();
        QVERIFY(env.hasChanges());
        QVERIFY(!env.toStringList().isEmpty());
        env = fileAccess.deviceEnvironment();
    }

    void testSetPermissions()
    {
        CmdBridge::FileAccess fileAccess;
        auto res = fileAccess.deployAndInit(
            FilePath::fromUserInput(libExecPath), FilePath::fromUserInput("/"));

        QVERIFY(res);

        auto tempFile = fileAccess.createTempFile(FilePath::fromUserInput(QDir::tempPath())
                                                  / "test.XXXXXX");
        QVERIFY(tempFile);

        QFile::Permissions perms = QFile::ReadOwner | QFile::WriteOwner;

        QVERIFY(fileAccess.setPermissions(*tempFile, perms));
        QCOMPARE(fileAccess.permissions(*tempFile) & 0xF0FF, perms);

        perms = QFile::ReadOwner | QFile::WriteOwner | QFile::ReadGroup | QFile::WriteOther;

        QVERIFY(fileAccess.setPermissions(*tempFile, perms));
        QCOMPARE(fileAccess.permissions(*tempFile) & 0xF0FF, perms);

        QVERIFY(fileAccess.removeFile(*tempFile));
    }

    void testTempFile()
    {
        CmdBridge::FileAccess fileAccess;
        auto res = fileAccess.deployAndInit(
            FilePath::fromUserInput(libExecPath), FilePath::fromUserInput("/"));

        QVERIFY(res);

        auto tempFile = fileAccess.createTempFile(FilePath::fromUserInput(QDir::tempPath()));

        QVERIFY(tempFile);
        QVERIFY(fileAccess.exists(*tempFile));
        QVERIFY(fileAccess.removeFile(*tempFile));
        QVERIFY(!fileAccess.exists(*tempFile));

        tempFile = fileAccess.createTempFile(FilePath::fromUserInput(QDir::tempPath())
                                             / "test.XXXXXX");
        QVERIFY(tempFile);
        QVERIFY(tempFile->fileName().startsWith("test."));
        QVERIFY(!tempFile->fileName().startsWith("test.XXXXXX"));
        QVERIFY(fileAccess.exists(*tempFile));
        QVERIFY(fileAccess.removeFile(*tempFile));
        QVERIFY(!fileAccess.exists(*tempFile));
    }

    void testTempFileWithoutPlaceholder()
    {
        CmdBridge::FileAccess fileAccess;
        auto res = fileAccess.deployAndInit(
            FilePath::fromUserInput(libExecPath), FilePath::fromUserInput("/"));

        QVERIFY(res);

        const FilePath pattern = FilePath::fromUserInput(QDir::tempPath()) / "test.txt";

        // We have to create a file with the pattern name to test the automatic placeholder
        // adding. go' os.CreateTemp() will fail if no placeholder is present, and the file exists.
        pattern.writeFileContents("Test");

        auto tempFile = fileAccess.createTempFile(pattern);

        QVERIFY(tempFile);
        QVERIFY(fileAccess.exists(*tempFile));
        QVERIFY(fileAccess.removeFile(*tempFile));
        QVERIFY(!fileAccess.exists(*tempFile));
        QVERIFY(tempFile->fileName().startsWith("test.txt."));
    }

    void testFileContents()
    {
        CmdBridge::FileAccess fileAccess;
        auto res = fileAccess.deployAndInit(
            FilePath::fromUserInput(libExecPath), FilePath::fromUserInput("/"));

        QVERIFY(res);

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
        QTC_ASSERT_EXPECTED(writeResult, QVERIFY(writeResult));
        QCOMPARE(*writeResult, testData.size());

        auto fileContents = fileAccess.fileContents(testFile, -1, 0);

        QTC_ASSERT_EXPECTED(fileContents, QVERIFY(fileContents));

        QVERIFY(fileContents->size() > 100);

        auto midContent = fileAccess.fileContents(testFile, 10, 10);
        QTC_ASSERT_EXPECTED(midContent, QVERIFY(midContent));

        QCOMPARE(*midContent, fileContents->mid(10, 10));

        auto endContent = fileAccess.fileContents(testFile, -1, 10);
        QTC_ASSERT_EXPECTED(endContent, QVERIFY(endContent));

        QCOMPARE(*endContent, fileContents->mid(10));

        QVERIFY(fileContents->size() > 0);

        const FilePath fileCopy = FilePath::fromUserInput(QDir::tempPath() + "/testcopy.txt");
        QVERIFY(fileAccess.copyFile(testFile, fileCopy));
        QVERIFY(fileAccess.exists(fileCopy));
        QCOMPARE(fileAccess.fileContents(fileCopy, -1, 0), fileContents);

        const FilePath mvTarget = FilePath::fromUserInput(QDir::tempPath() + "/testmoved.txt");
        QVERIFY(fileAccess.renameFile(fileCopy, mvTarget));
        QVERIFY(fileAccess.exists(mvTarget));
        QVERIFY(!fileAccess.exists(fileCopy));
        QCOMPARE(fileAccess.fileContents(mvTarget, -1, 0), fileContents);

        QVERIFY(fileAccess.removeFile(mvTarget));
        QVERIFY(fileAccess.removeFile(testFile));
        QVERIFY(!fileAccess.removeFile(testFile));

        QVERIFY(fileAccess.ensureExistingFile(testFile));
        QVERIFY(fileAccess.removeFile(testFile));

        const FilePath dir = FilePath::fromUserInput(QDir::tempPath() + "/testdir");
        QVERIFY(fileAccess.createDirectory(dir));
        QVERIFY(fileAccess.exists(dir));
        QVERIFY(fileAccess.isReadableDirectory(dir));
        QVERIFY(fileAccess.removeRecursively(dir, nullptr));
        QVERIFY(!fileAccess.exists(dir));
    }

    void testIs()
    {
        auto bridgePath = CmdBridge::Client::getCmdBridgePath(HostOsInfo::hostOs(),
                                                              HostOsInfo::hostArchitecture(),
                                                              FilePath::fromUserInput(libExecPath));
        QTC_ASSERT_EXPECTED(bridgePath, QSKIP("No bridge found"));

        CmdBridge::Client client(*bridgePath);
        client.start();

        auto result = client.is("/tmp", CmdBridge::Client::Is::Dir)->result();
        QVERIFY(result);

        result = client.is("/idonotexist", CmdBridge::Client::Is::Dir)->result();
        QVERIFY(!result);
        result = client.is("/idonotexist", CmdBridge::Client::Is::Exists)->result();
        QVERIFY(!result);
    }

    void testStat()
    {
        auto bridgePath = CmdBridge::Client::getCmdBridgePath(HostOsInfo::hostOs(),
                                                              HostOsInfo::hostArchitecture(),
                                                              FilePath::fromUserInput(libExecPath));
        QTC_ASSERT_EXPECTED(bridgePath, QSKIP("No bridge found"));

        CmdBridge::Client client(*bridgePath);
        QTC_ASSERT_EXPECTED(client.start(), return);

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
        auto res = fileAccess.deployAndInit(
            FilePath::fromUserInput(libExecPath), FilePath::fromUserInput("/"));

        QVERIFY(res);

        FilePath tmptarget = fileAccess.symLinkTarget(FilePath::fromUserInput("/tmp"));

        qDebug() << "SymLinkTarget:" << tmptarget.toUserOutput();
    }

    void testFileId()
    {
        CmdBridge::FileAccess fileAccess;
        auto res = fileAccess.deployAndInit(
            FilePath::fromUserInput(libExecPath), FilePath::fromUserInput("/"));

        QVERIFY(res);

        QByteArray fileId = fileAccess.fileId(FilePath::fromUserInput(QDir::rootPath()));

        qDebug() << "File Id:" << fileId;
    }

    void testFreeSpace()
    {
        CmdBridge::FileAccess fileAccess;
        auto res = fileAccess.deployAndInit(
            FilePath::fromUserInput(libExecPath), FilePath::fromUserInput("/"));

        QVERIFY(res);

        quint64 freeSpace = fileAccess.bytesAvailable(FilePath::fromUserInput(QDir::rootPath()));

        qDebug() << "Free Space:" << freeSpace;
    }

    void testFind()
    {
        CmdBridge::FileAccess fileAccess;
        auto res = fileAccess.deployAndInit(
            FilePath::fromUserInput(libExecPath), FilePath::fromUserInput("/"));

        QVERIFY(res);
        fileAccess.iterateDirectory(
            FilePath::fromUserInput(QDir::homePath()),
            [](const FilePath &path, const FilePathInfo &) {
                qDebug() << path;
                //qDebug() << path.toString() << info.lastModified;
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
    }

    void testBridge()
    {
        CmdBridge::FileAccess fileAccess;
        auto res = fileAccess.deployAndInit(
            FilePath::fromUserInput(libExecPath), FilePath::fromUserInput("/"));

        QVERIFY(res);

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
        qDebug() << "DeviceShell Find took:" << timer.elapsed();
    }

    void testInit()
    {
        auto bridgePath = CmdBridge::Client::getCmdBridgePath(HostOsInfo::hostOs(),
                                                              HostOsInfo::hostArchitecture(),
                                                              FilePath::fromUserInput(libExecPath));
        QTC_ASSERT_EXPECTED(bridgePath, QSKIP("No bridge found"));

        CmdBridge::Client client(*bridgePath);

        auto result = client.start();
        QTC_ASSERT_EXPECTED(result, QFAIL("Failed to start bridge"));

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
