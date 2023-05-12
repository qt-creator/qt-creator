// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <app/app_version.h>

#include <utils/async.h>
#include <utils/filesearch.h>
#include <utils/launcherinterface.h>
#include <utils/scopedtimer.h>
#include <utils/temporarydirectory.h>

#include <QDirIterator>
#include <QObject>
#include <QTemporaryDir>
#include <QtTest>

#include <unordered_set>

using namespace Tasking;
using namespace Utils;

static const int s_subDirsCount = 8;
static const int s_subFilesCount = 8;
static const int s_treeDepth = 4;

static const QDir::Filters s_filters = QDir::Dirs | QDir::Files | QDir::Hidden
                                     | QDir::NoDotAndDotDot;

static const char s_dirPrefix[] = "dir_";
static const char s_filePrefix[] = "file_";

static int expectedDirsCountHelper(int depth)
{
    if (depth == 1)
        return s_subDirsCount + 1; // +1 -> dir itself
    return expectedDirsCountHelper(depth - 1) * s_subDirsCount + 1; // +1 -> dir itself
}

static int expectedDirsCount(int tasksCount)
{
    return expectedDirsCountHelper(s_treeDepth) * tasksCount + 1; // +1 -> root temp dir
}

static int expectedFilesCountHelper(int depth)
{
    if (depth == 0)
        return s_subFilesCount;
    return expectedFilesCountHelper(depth - 1) * s_subDirsCount + s_subFilesCount;
}

static int expectedFilesCount(int tasksCount)
{
    return expectedFilesCountHelper(s_treeDepth) * tasksCount + 1; // +1 -> fileTemplate.txt
}

static void generate(QPromise<void> &promise, const QString &parentPath,
                     const QString &templateFile, int depth)
{
    const QDir parentDir(parentPath);
    for (int i = 0; i < s_subFilesCount; ++i)
        QFile::copy(templateFile, parentDir.filePath(QString("%1%2").arg(s_filePrefix).arg(i)));

    if (depth == 0)
        return;

    if (promise.isCanceled())
        return;

    const QString templateDir("dirTemplate");
    if (!parentDir.mkdir(templateDir)) {
        promise.future().cancel();
        return;
    }

    const QString templateDirPath = parentDir.filePath(templateDir);
    generate(promise, templateDirPath, templateFile, depth - 1);

    if (promise.isCanceled())
        return;

    for (int i = 0; i < s_subDirsCount - 1; ++i) {
        const QString dirName = QString("%1%2").arg(s_dirPrefix).arg(i);
        const FilePath source = FilePath::fromString(templateDirPath);
        const FilePath destination = FilePath::fromString(parentDir.filePath(dirName));
        if (!source.copyRecursively(destination)) {
            promise.future().cancel();
            return;
        }
    }
}

class tst_SubDirFileIterator : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        TemporaryDirectory::setMasterTemporaryDirectory(
            QDir::tempPath() + "/" + Core::Constants::IDE_CASED_ID + "-XXXXXX");

        const QString libExecPath(qApp->applicationDirPath() + '/'
                                  + QLatin1String(TEST_RELATIVE_LIBEXEC_PATH));
        LauncherInterface::setPathToLauncher(libExecPath);

        qDebug() << "This manual test compares the performance of the SubDirFileIterator with "
                    "a manually written iterator using QDir::entryInfoList().";
        QTC_SCOPED_TIMER("GENERATING TEMPORARY FILES TREE");
        const int tasksCount = QThread::idealThreadCount();
        m_filesCount = expectedFilesCount(tasksCount);
        m_dirsCount = expectedDirsCount(tasksCount);
        m_tempDir.reset(new QTemporaryDir);
        qDebug() << "Generating on" << tasksCount << "cores...";
        qDebug() << "Generating" << m_filesCount << "files...";
        qDebug() << "Generating" << m_dirsCount << "dirs...";
        qDebug() << "Generating inside" << m_tempDir->path() << "dir...";
        const QString templateFile = m_tempDir->filePath("fileTemplate.txt");
        {
            QFile file(templateFile);
            QVERIFY(file.open(QIODevice::ReadWrite));
            file.write("X");
        }

        // Parallelize tree generation
        const QDir parentDir(m_tempDir->path());
        const auto onSetup = [](const QString &parentPath, const QString &templateFile) {
            return [parentPath, templateFile](Async<void> &async) {
                async.setConcurrentCallData(generate, parentPath, templateFile, s_treeDepth);
            };
        };
        QList<TaskItem> tasks {parallel};
        for (int i = 0; i < tasksCount; ++i) {
            const QString dirName = QString("%1%2").arg(s_dirPrefix).arg(i);
            QVERIFY(parentDir.mkdir(dirName));
            tasks.append(AsyncTask<void>(onSetup(parentDir.filePath(dirName), templateFile)));
        }

        TaskTree taskTree(tasks);
        QVERIFY(taskTree.runBlocking());
    }

    void cleanupTestCase()
    {
        QTC_SCOPED_TIMER("CLEANING UP");

        // Parallelize tree removal
        const auto removeTree = [](const QString &parentPath) {
            FilePath::fromString(parentPath).removeRecursively();
        };

        const QDir parentDir(m_tempDir->path());
        const auto onSetup = [removeTree](const QString &parentPath) {
            return [parentPath, removeTree](Async<void> &async) {
                async.setConcurrentCallData(removeTree, parentPath);
            };
        };
        QList<TaskItem> tasks {parallel};
        const int tasksCount = QThread::idealThreadCount();
        for (int i = 0; i < tasksCount; ++i) {
            const QString dirName = QString("%1%2").arg(s_dirPrefix).arg(i);
            tasks.append(AsyncTask<void>(onSetup(parentDir.filePath(dirName))));
        }

        TaskTree taskTree(tasks);
        QVERIFY(taskTree.runBlocking());

        m_tempDir.reset();
        Singleton::deleteAll();
    }

    void testSubDirFileIterator()
    {
        QTC_SCOPED_TIMER("ITERATING with SubDirFileIterator");
        int filesCount = 0;
        {
            const FilePath root(FilePath::fromString(m_tempDir->path()));
            SubDirFileIterator it({root}, {}, {});
            auto i = it.begin();
            const auto e = it.end();
            while (i != e) {
                ++filesCount;
                ++i;
                if (filesCount % 100000 == 0)
                    qDebug() << filesCount << '/' << m_filesCount << "files visited so far...";
            }
        }
        qDebug() << "Visited" << filesCount << "files.";
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
            ++dirsCount; // for root itself
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
        qDebug() << "Visited" << filesCount << "files and" << dirsCount << "directories.";
    }

    void testQDirIterator()
    {
        QTC_SCOPED_TIMER("ITERATING with QDirIterator");
        int filesCount = 0;
        int dirsCount = 0;
        {
            ++dirsCount; // for root itself
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
        qDebug() << "Visited" << filesCount << "files and" << dirsCount << "directories.";
    }

private:
    int m_dirsCount = 0;
    int m_filesCount = 0;
    std::unique_ptr<QTemporaryDir> m_tempDir;
};

QTEST_GUILESS_MAIN(tst_SubDirFileIterator)

#include "tst_subdirfileiterator.moc"
