// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QDebug>
#include <QDir>
#include <QIODevice>
#include <QProcess>
#include <QRandomGenerator>
#include <QSignalSpy>
#include <QTest>

#include <utils/commandline.h>
#include <utils/devicefileaccess.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/link.h>

//TESTED_COMPONENT=src/libs/utils

QT_BEGIN_NAMESPACE
namespace QTest {
template<>
char *toString(const Utils::FilePath &filePath)
{
    return qstrdup(filePath.toUrlishString().toLocal8Bit().constData());
}
} // namespace QTest
QT_END_NAMESPACE

using namespace Utils;

class TestDFA : public UnixDeviceFileAccess
{
public:
    using UnixDeviceFileAccess::UnixDeviceFileAccess;

    Result<RunResult> runInShellImpl(const CommandLine &cmdLine,
                                     const QByteArray &inputData = {}) const override
    {
        if (m_refused.contains(cmdLine.executable().path()))
            return ResultError("refused");
        // Note: Don't convert into Utils::Process. See more comments in this change in gerrit.
        QProcess p;
        p.setProgram(cmdLine.executable().toFSPathString());
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

    Result<FilePath> createTempDir(const FilePath &filePath) override
    {
        return UnixDeviceFileAccess::createTempDir(filePath);
    }

    Result<QString> homeDirectory(const QString &user) const override
    {
        return UnixDeviceFileAccess::homeDirectory(user);
    }

    QStringList m_refused;
};

class tst_unixdevicefileaccess : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        if (HostOsInfo::isWindowsHost())
            QSKIP("This test is only for Unix hosts");

        m_fileSizeTestFile.writeFileContents(QByteArray(1024, 'a'));

        DeviceFileHooks hooks;
        hooks.isSameDevice = [](const FilePath &lhs, const FilePath &rhs) {
            return lhs.scheme() == rhs.scheme() && lhs.host() == rhs.host();
        };
        hooks.fileAccess = [this](const FilePath &) -> Result<DeviceFileAccessPtr> {
            return m_deviceDfa;
        };
        hooks.osType = [](const FilePath &) { return OsTypeLinux; };
        DeviceFileHooks::setupDeviceFileHooks(hooks);
    }

    void init() { m_deviceDfa = std::make_shared<TestDFA>(); }

    void fileSize()
    {
        const auto size = m_dfaPtr->fileSize(m_fileSizeTestFile);
        QCOMPARE(size, 1024);
    }

    void findUsingLs()
    {
        QStringList result;
        m_dfa.findUsingLs(m_tempDir.path(),
                          FileFilter{{}, DirFilterFlag::NoFilter, DirIteratorFlag::Subdirectories},
                          &result);

        QCOMPARE(result, QStringList({".", "..", "size-test"}));

        QDir tDir(m_tempDir.path());
        tDir.mkdir("lsfindsubdir");

        result.clear();
        m_dfa.findUsingLs(m_tempDir.path(),
                          FileFilter{{}, DirFilterFlag::NoFilter, DirIteratorFlag::Subdirectories},
                          &result);
        QCOMPARE(result,
                 QStringList(
                     {".", "..", "lsfindsubdir/.", "lsfindsubdir/..", "lsfindsubdir", "size-test"}));
    }

    void iterateDirectoryDanglingSymlink()
    {
        const FilePath dir = FilePath::fromString(m_tempDir.filePath("dangling"));
        QVERIFY(dir.pathAppended("dir").createDir());
        QVERIFY(dir.pathAppended("file").writeFileContents("file"));
        QVERIFY_RESULT(dir.pathAppended("nowhere").createSymLink(dir.pathAppended("link")));

        QStringList found;
        QVERIFY_RESULT(m_dfaPtr->iterateDirectory(
            dir,
            [&found](const FilePath &entry, const FilePathInfo &) {
                found.append(entry.fileName());
                return IterationPolicy::Continue;
            },
            FileFilter({}, DirFilterFlag::AllEntries | DirFilterFlag::System)));
        found.sort();
        QCOMPARE(found, QStringList({"dir", "file", "link"}));
    }

    void findUsingLsFiltered_data()
    {
        QTest::addColumn<DirFilterFlags>("filters");
        QTest::addColumn<QStringList>("expected");

        const DirFilterFlags noDots = DirFilterFlag::NoDotAndDotDot;
        QTest::newRow("all") << (DirFilterFlag::Dirs | DirFilterFlag::Files
                                 | DirFilterFlag::Hidden | noDots)
                             << QStringList({".hidden", "dir", "file", "link"});
        QTest::newRow("dots") << (DirFilterFlag::Dirs | DirFilterFlag::Files)
                              << QStringList({".", "..", "dir", "file", "link"});
        QTest::newRow("files") << DirFilterFlags(DirFilterFlag::Files) << QStringList("file");
        QTest::newRow("hidden files") << (DirFilterFlag::Files | DirFilterFlag::Hidden)
                                      << QStringList({".hidden", "file"});
        QTest::newRow("dirs") << (DirFilterFlag::Dirs | noDots)
                              << QStringList({"dir", "link"});
        QTest::newRow("no type") << (DirFilterFlag::Hidden | noDots) << QStringList();
    }

    void findUsingLsFiltered()
    {
        QFETCH(DirFilterFlags, filters);
        QFETCH(QStringList, expected);

        const FilePath dir = FilePath::fromString(m_tempDir.filePath("lsfilter"));
        QVERIFY(dir.pathAppended("dir").ensureWritableDir());
        QVERIFY(dir.pathAppended("file").writeFileContents("file"));
        QVERIFY(dir.pathAppended(".hidden").writeFileContents("hidden"));
        if (!dir.pathAppended("link").isSymLink())
            QVERIFY_RESULT(dir.pathAppended("dir").createSymLink(dir.pathAppended("link")));

        QStringList result;
        m_dfa.findUsingLs(dir.path(), FileFilter({}, filters), &result);
        result.sort();
        QCOMPARE(result, expected);
    }

    void copyRecursivelyDanglingSymlink()
    {
        const FilePath src = FilePath::fromString(m_tempDir.filePath("dangling-src"));
        QVERIFY(src.pathAppended("dir").createDir());
        QVERIFY(src.pathAppended("dir/leaf").writeFileContents("leaf"));
        QVERIFY(src.pathAppended("file").writeFileContents("file"));
        QVERIFY_RESULT(src.pathAppended("nowhere").createSymLink(src.pathAppended("dangling")));
        const FilePath target = FilePath::fromString(m_tempDir.filePath("dangling-target"));

        QStringList copied;
        const Result<FileUtils::CopyResult> res = FileUtils::copyRecursively(
            onDevice(src),
            onDevice(target),
            [&copied](const FilePath &from, const FilePath &) -> Result<FileUtils::CopyResult> {
                copied.append(from.fileName());
                return FileUtils::CopyResult::Done;
            });
        QVERIFY_RESULT(res);
        copied.sort();
        QCOMPARE(copied, QStringList({"dangling", "file", "leaf"}));
    }

    void copyRecursivelyWithoutFind()
    {
        const FilePath src = FilePath::fromString(m_tempDir.filePath("copy-src"));
        QVERIFY(src.pathAppended("sub").createDir());
        QVERIFY(src.pathAppended("sub/leaf.txt").writeFileContents("leaf"));
        const FilePath target = FilePath::fromString(m_tempDir.filePath("copy-target"));

        m_deviceDfa->m_refused = {"find"};
        const Result<FileUtils::CopyResult> res
            = FileUtils::copyRecursively(onDevice(src), onDevice(target), &copyFile);
        QVERIFY_RESULT(res);
        QCOMPARE(*res, FileUtils::CopyResult::Done);
        QCOMPARE(QDir(target.path()).entryList(QDir::AllEntries | QDir::NoDotAndDotDot),
                 QStringList("sub"));
        QCOMPARE(target.pathAppended("sub/leaf.txt").fileContents().value_or(QByteArray()),
                 "leaf");
    }

    void copyRecursivelyListingFails()
    {
        const FilePath src = FilePath::fromString(m_tempDir.filePath("unlistable-src"));
        QVERIFY(src.pathAppended("sub").createDir());
        const FilePath target = FilePath::fromString(m_tempDir.filePath("unlistable-target"));

        m_deviceDfa->m_refused = {"find", "ls"};
        const Result<FileUtils::CopyResult> res
            = FileUtils::copyRecursively(onDevice(src), onDevice(target), &copyFile);
        QVERIFY(!res);
        QVERIFY(res.error().contains("unlistable-src"));
    }

    void createTempDir()
    {
        FilePath tmpDirTemplate = FilePath::fromUserInput(QDir::tempPath()) / "qtc-XXXXXX";
        const Result<FilePath> tmpDirResult = m_dfa.createTempDir(tmpDirTemplate);
        QVERIFY_RESULT(tmpDirResult);
        const FilePath tmpDir = *tmpDirResult;
        QVERIFY(tmpDir.isWritableDir());
        QVERIFY(tmpDir.removeRecursively());
    }

    void homeDirectory()
    {
        // "~" (empty user) resolves to the current user's home directory.
        const Result<QString> home = m_dfa.homeDirectory({});
        QVERIFY_RESULT(home);
        QVERIFY(home->startsWith('/'));

        const QString user = qEnvironmentVariable("USER", qEnvironmentVariable("LOGNAME"));
        if (!user.isEmpty()) {
            const Result<QString> userHome = m_dfa.homeDirectory(user);
            QVERIFY_RESULT(userHome);
            QVERIFY(userHome->startsWith('/'));
        }

        // Unknown users and names with shell metacharacters are rejected.
        QVERIFY(!m_dfa.homeDirectory("no_such_user_qtc_test"));
        QVERIFY(!m_dfa.homeDirectory("bad;name"));
    }

private:
    static FilePath onDevice(const FilePath &path)
    {
        return FilePath::fromParts(u"dfa", u"test", path.path());
    }

    static Result<FileUtils::CopyResult> copyFile(const FilePath &from, const FilePath &to)
    {
        if (const Result<> copied = from.copyFile(to); !copied)
            return ResultError(copied.error());
        return FileUtils::CopyResult::Done;
    }

    TestDFA m_dfa;
    std::shared_ptr<TestDFA> m_deviceDfa = std::make_shared<TestDFA>();
    DeviceFileAccess *m_dfaPtr = &m_dfa;

    QTemporaryDir m_tempDir;
    FilePath m_fileSizeTestFile = FilePath::fromString(m_tempDir.filePath("size-test"));
};

QTEST_GUILESS_MAIN(tst_unixdevicefileaccess)

#include "tst_unixdevicefileaccess.moc"
