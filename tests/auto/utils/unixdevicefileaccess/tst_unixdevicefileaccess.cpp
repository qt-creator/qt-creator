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
    }

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
    TestDFA m_dfa;
    DeviceFileAccess *m_dfaPtr = &m_dfa;

    QTemporaryDir m_tempDir;
    FilePath m_fileSizeTestFile = FilePath::fromString(m_tempDir.filePath("size-test"));
};

QTEST_GUILESS_MAIN(tst_unixdevicefileaccess)

#include "tst_unixdevicefileaccess.moc"
