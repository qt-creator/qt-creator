// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <app/app_version.h>

#include <utils/async.h>
#include <utils/filesearch.h>
#include <utils/scopedtimer.h>
#include <utils/singleton.h>
#include <utils/temporarydirectory.h>

#include <QDirIterator>
#include <QObject>
#include <QTemporaryDir>
#include <QtTest>

#include <unordered_set>

using namespace Tasking;
using namespace Utils;

static const int s_topLevelSubDirsCount = 128;
static const int s_subDirsCount = 4;
static const int s_subFilesCount = 4;
static const int s_treeDepth = 5;

static const QDir::Filters s_filters = QDir::Dirs | QDir::Files | QDir::Hidden
                                     | QDir::NoDotAndDotDot;

static const char s_dirPrefix[] = "dir_";
static const char s_filePrefix[] = "file_";

static QString dirName(int suffix)
{
    return QString("%1%2").arg(s_dirPrefix).arg(suffix);
}

static QString fileName(int suffix)
{
    return QString("%1%2.txt").arg(s_filePrefix).arg(suffix);
}

static int expectedDirsCountHelper(int depth)
{
    if (depth == 1)
        return s_subDirsCount + 1; // +1 -> dir itself
    return expectedDirsCountHelper(depth - 1) * s_subDirsCount + 1; // +1 -> dir itself
}

static int expectedDirsCount()
{
    return expectedDirsCountHelper(s_treeDepth) * s_topLevelSubDirsCount;
}

static int expectedFilesCountHelper(int depth)
{
    if (depth == 0)
        return s_subFilesCount;
    return expectedFilesCountHelper(depth - 1) * s_subDirsCount + s_subFilesCount;
}

static int expectedFilesCount()
{
    return expectedFilesCountHelper(s_treeDepth) * s_topLevelSubDirsCount + 1; // +1 -> fileTemplate.txt
}

static bool generateOriginal(const QString &parentPath, const QString &templateFile, int depth)
{
    const QDir parentDir(parentPath);
    for (int i = 0; i < s_subFilesCount; ++i)
        QFile::copy(templateFile, parentDir.filePath(fileName(i)));

    if (depth == 0)
        return true;

    const QString originalDirName = dirName(0);
    if (!parentDir.mkdir(originalDirName))
        return false;

    const QString originalDirPath = parentDir.filePath(originalDirName);
    if (!generateOriginal(originalDirPath, templateFile, depth - 1))
        return false;

    for (int i = 1; i < s_subDirsCount; ++i) {
        const FilePath source = FilePath::fromString(originalDirPath);
        const FilePath destination = FilePath::fromString(parentDir.filePath(dirName(i)));
        if (!source.copyRecursively(destination))
            return false;
    }
    return true;
}

static void generateCopy(QPromise<void> &promise, const QString &sourcePath,
                         const QString &destPath)
{
    const FilePath source = FilePath::fromString(sourcePath);
    const FilePath destination = FilePath::fromString(destPath);
    if (!source.copyRecursively(destination))
        promise.future().cancel();
}

static void removeTree(const QString &parentPath)
{
    FilePath::fromString(parentPath).removeRecursively();
};

class tst_SubDirFileContainer : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        TemporaryDirectory::setMasterTemporaryDirectory(
            QDir::tempPath() + "/" + Core::Constants::IDE_CASED_ID + "-XXXXXX");

        qDebug() << "This manual test compares the performance of the SubDirFileContainer with a "
                    "manually written iterator using QDir::entryInfoList() and with QDirIterator.";
        QTC_SCOPED_TIMER("GENERATING TEMPORARY FILES TREE");
        m_filesCount = expectedFilesCount();
        m_dirsCount = expectedDirsCount();
        m_tempDir.reset(new QTemporaryDir);
        qDebug() << "Generating on" << qMax(QThread::idealThreadCount() - 1, 1) << "cores...";
        qDebug() << "Generating" << m_filesCount << "files...";
        qDebug() << "Generating" << m_dirsCount << "dirs...";
        qDebug() << "Generating inside" << m_tempDir->path() << "dir...";
        const QString templateFile = m_tempDir->filePath("fileTemplate.txt");
        {
            QFile file(templateFile);
            QVERIFY(file.open(QIODevice::ReadWrite));
            file.write("X");
        }

        const QDir parentDir(m_tempDir->path());
        const QString sourceDirName = dirName(0);
        QVERIFY(parentDir.mkdir(sourceDirName));
        QVERIFY(generateOriginal(parentDir.filePath(sourceDirName), templateFile, s_treeDepth));

        const LoopRepeat iterator(s_topLevelSubDirsCount - 1);

        const auto onCopySetup = [iterator, parentDir, sourceDirName](Async<void> &async) {
            const QString destDirName = dirName(iterator.iteration() + 1);
            QVERIFY(parentDir.mkdir(destDirName));
            async.setConcurrentCallData(generateCopy, parentDir.filePath(sourceDirName),
                                        parentDir.filePath(destDirName));
        };

        const Group recipe = For (iterator) >> Do {
            parallelIdealThreadCountLimit, // Parallelize tree generation
            AsyncTask<void>(onCopySetup)
        };
        QCOMPARE(TaskTree::runBlocking(recipe), DoneWith::Success);
    }

    void cleanupTestCase()
    {
        QTC_SCOPED_TIMER("CLEANING UP");

        const LoopRepeat iterator(s_topLevelSubDirsCount - 1);

        const QDir parentDir(m_tempDir->path());
        const auto onSetup = [iterator, parentDir](Async<void> &async) {
            async.setConcurrentCallData(removeTree,
                                        parentDir.filePath(dirName(iterator.iteration() + 1)));
        };

        const Group recipe = For (iterator) >> Do {
            parallelIdealThreadCountLimit, // Parallelize tree removal
            AsyncTask<void>(onSetup)
        };

        QCOMPARE(TaskTree::runBlocking(recipe), DoneWith::Success);

        m_tempDir.reset();
        Singleton::deleteAll();
    }

    void testSubDirFileContainer()
    {
        QTC_SCOPED_TIMER("ITERATING with FileContainer");
        int filesCount = 0;
        {
            const FilePath root(FilePath::fromString(m_tempDir->path()));
            FileContainer container = SubDirFileContainer({root}, {}, {});
            auto it = container.begin();
            const auto end = container.end();
            while (it != end) {
                ++filesCount;
                ++it;
                if (filesCount % 100000 == 0)
                    qDebug() << filesCount << '/' << m_filesCount << "files visited so far...";
            }
        }
        QCOMPARE(filesCount, m_filesCount);
    }

    void testManualIterator()
    {
        QTC_SCOPED_TIMER("ITERATING with manual iterator");
        int filesCount = 0;
        int dirsCount = 0;
        {
            const QDir root(m_tempDir->path());
            std::unordered_set<QString> visitedFiles;
            std::unordered_set<QString> visitedDirs;
            visitedDirs.emplace(root.absolutePath());
            QFileInfoList workingList = root.entryInfoList(s_filters);
            while (!workingList.isEmpty()) {
                const QFileInfo fi = workingList.takeLast();
                const QString absoluteFilePath = fi.absoluteFilePath();
                if (fi.isDir()) {
                    if (!visitedDirs.emplace(absoluteFilePath).second)
                        continue;
                    if (fi.isSymbolicLink() && !visitedDirs.emplace(fi.symLinkTarget()).second)
                        continue;
                    ++dirsCount;
                    workingList.append(QDir(absoluteFilePath).entryInfoList(s_filters));
                } else {
                    if (!visitedFiles.emplace(absoluteFilePath).second)
                        continue;
                    if (fi.isSymbolicLink() && !visitedFiles.emplace(fi.symLinkTarget()).second)
                        continue;
                    ++filesCount;
                    if (filesCount % 100000 == 0)
                        qDebug() << filesCount << '/' << m_filesCount << "files visited so far...";
                }
            }
        }
        QCOMPARE(filesCount, m_filesCount);
        QCOMPARE(dirsCount, m_dirsCount);
    }

    void testQDirIterator()
    {
        QTC_SCOPED_TIMER("ITERATING with QDirIterator");
        int filesCount = 0;
        int dirsCount = 0;
        {
            QDirIterator it(m_tempDir->path(), s_filters, QDirIterator::Subdirectories
                                                              | QDirIterator::FollowSymlinks);
            while (it.hasNext()) {
                const QFileInfo fi = it.nextFileInfo();
                if (fi.isDir()) {
                    ++dirsCount;
                } else {
                    ++filesCount;
                    if (filesCount % 100000 == 0)
                        qDebug() << filesCount << '/' << m_filesCount << "files visited so far...";
                }
            }
        }
        QCOMPARE(filesCount, m_filesCount);
        QCOMPARE(dirsCount, m_dirsCount);
    }

private:
    int m_dirsCount = 0;
    int m_filesCount = 0;
    std::unique_ptr<QTemporaryDir> m_tempDir;
};

QTEST_GUILESS_MAIN(tst_SubDirFileContainer)

#include "tst_subdirfilecontainer.moc"
