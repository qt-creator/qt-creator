// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTemporaryDir>
#include <QtTest>

#include <utils/algorithm.h>
#include <utils/filepath.h>

#include "../headerdependencies.h"

using namespace CMakeProjectManager::Internal;
using namespace Utils;

class FakeFileSystem
{
public:
    void setTime(const FilePath &path, qint64 time) { m_times.insert(path, time); }

    void remove(const FilePath &path)
    {
        m_times.insert(path, HeaderDependencyStore::MissingFile);
    }

    HeaderDependencyStore::StatFunction statFunction() const
    {
        return [this](const FilePath &path) {
            return m_times.value(path, HeaderDependencyStore::MissingFile);
        };
    }

private:
    QHash<FilePath, qint64> m_times;
};

static QByteArray errorOf(const Result<> &result)
{
    return result ? QByteArray("no error") : result.error().toUtf8();
}

class HeaderDependenciesTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() { QVERIFY(m_temp.isValid()); }

    void unknownSourceIsStale()
    {
        FakeFileSystem files;
        files.setTime(source("main.cpp"), 100);

        HeaderDependencyStore store(files.statFunction());

        QCOMPARE(store.staleScans({scan("main.cpp")}), SourceScans{scan("main.cpp")});
    }

    void unchangedSourceIsNotStale()
    {
        FakeFileSystem files;
        files.setTime(source("main.cpp"), 100);
        files.setTime(source("main.h"), 100);

        HeaderDependencyStore store(files.statFunction());
        store.insert(scan("main.cpp"), {source("main.h")}, 200);
        store.forgetTimes();

        QVERIFY(store.staleScans({scan("main.cpp")}).isEmpty());
    }

    void touchedHeaderInvalidatesOnlyItsIncluders()
    {
        FakeFileSystem files;
        for (const char *name : {"a.cpp", "b.cpp", "shared.h", "a.h", "b.h"})
            files.setTime(source(name), 100);

        HeaderDependencyStore store(files.statFunction());
        store.insert(scan("a.cpp"), {source("shared.h"), source("a.h")}, 200);
        store.insert(scan("b.cpp"), {source("shared.h"), source("b.h")}, 200);

        const SourceScans all{scan("a.cpp"), scan("b.cpp")};

        store.forgetTimes();
        QVERIFY(store.staleScans(all).isEmpty());

        files.setTime(source("a.h"), 300);
        store.forgetTimes();
        QCOMPARE(store.staleScans(all), SourceScans{scan("a.cpp")});

        files.setTime(source("shared.h"), 300);
        store.forgetTimes();
        QCOMPARE(store.staleScans(all), all);
    }

    void touchedSourceIsStale()
    {
        FakeFileSystem files;
        files.setTime(source("main.cpp"), 100);
        files.setTime(source("main.h"), 100);

        HeaderDependencyStore store(files.statFunction());
        store.insert(scan("main.cpp"), {source("main.h")}, 200);

        files.setTime(source("main.cpp"), 300);
        store.forgetTimes();

        QCOMPARE(store.staleScans({scan("main.cpp")}).size(), 1);
    }

    void changedFlagsInvalidate()
    {
        FakeFileSystem files;
        files.setTime(source("main.cpp"), 100);
        files.setTime(source("main.h"), 100);

        HeaderDependencyStore store(files.statFunction());
        store.insert(scan("main.cpp", 0xaaaa), {source("main.h")}, 200);

        store.forgetTimes();
        QVERIFY(store.staleScans({scan("main.cpp", 0xaaaa)}).isEmpty());

        store.forgetTimes();
        QCOMPARE(store.staleScans({scan("main.cpp", 0xbbbb)}).size(), 1);
    }

    void missingFileInvalidates()
    {
        FakeFileSystem files;
        files.setTime(source("main.cpp"), 100);
        files.setTime(source("main.h"), 100);

        HeaderDependencyStore store(files.statFunction());
        store.insert(scan("main.cpp"), {source("main.h")}, 200);

        files.remove(source("main.h"));
        store.forgetTimes();

        QCOMPARE(store.staleScans({scan("main.cpp")}).size(), 1);
    }

    void insertReportsWhatChanged()
    {
        FakeFileSystem files;
        HeaderDependencyStore store(files.statFunction());

        QVERIFY(store.insert(scan("main.cpp"), {source("main.h")}, 100));
        QVERIFY(!store.insert(scan("main.cpp"), {source("main.h")}, 200));
        QVERIFY(!store.insert(scan("main.cpp"), {source("main.h"), source("main.h")}, 300));
        QVERIFY(store.insert(scan("main.cpp"), {source("main.h"), source("extra.h")}, 400));
        QVERIFY(store.insert(scan("main.cpp", 0xaaaa), {source("main.h"), source("extra.h")}, 500));
    }

    void insertReportsNoChangeForAFileThatStaysMissing()
    {
        FakeFileSystem files;
        files.setTime(source("main.cpp"), 100);

        HeaderDependencyStore store(files.statFunction());
        const FilePaths dependencies{source("main.h"), source("ui_mainwindow.h")};

        QVERIFY(store.insert(scan("main.cpp"), dependencies, 200));

        store.forgetTimes();
        QCOMPARE(store.staleScans({scan("main.cpp")}).size(), 1);
        QVERIFY(!store.insert(scan("main.cpp"), dependencies, 300));
    }

    void retainSourcesDropsWhatLeftTheProject()
    {
        FakeFileSystem files;
        HeaderDependencyStore store(files.statFunction());
        store.insert(scan("a.cpp"), {source("a.h")}, 100);
        store.insert(scan("b.cpp"), {source("b.h")}, 100);

        QVERIFY(store.retainSources({source("a.cpp"), source("gone.cpp")}));
        QVERIFY(!store.retainSources({source("a.cpp")}));

        QCOMPARE(store.sources(), FilePaths{source("a.cpp")});
        QVERIFY(store.dependencies(source("b.cpp")).isEmpty());

        HeaderDependencyStore never(files.statFunction());
        never.insert(scan("a.cpp"), {source("a.h")}, 100);

        const FilePath storeFile = tempPath("retain/header-deps");
        const FilePath neverFile = tempPath("retain/never-deps");
        QVERIFY(store.save(storeFile));
        QVERIFY(never.save(neverFile));
        QVERIFY2(storeFile.fileSize() == neverFile.fileSize(),
                 "The paths of the dropped source stayed in the written store.");

        HeaderDependencyStore loaded(files.statFunction());
        QVERIFY(loaded.load(storeFile));
        QCOMPARE(loaded.sources(), FilePaths{source("a.cpp")});
        QCOMPARE(loaded.dependencies(source("a.cpp")), FilePaths{source("a.h")});
    }

    void eachPathIsQueriedOnce()
    {
        FakeFileSystem files;
        files.setTime(source("shared.h"), 100);

        HeaderDependencyStore store(files.statFunction());
        SourceScans scans;
        for (int i = 0; i < 50; ++i) {
            const QString name = QString("unit%1.cpp").arg(i);
            files.setTime(source(name), 100);
            store.insert(scan(name), {source("shared.h")}, 200);
            scans.append(scan(name));
        }

        store.forgetTimes();
        QVERIFY(store.staleScans(scans).isEmpty());

        QCOMPARE(store.statCount(), 51);
    }

    void survivesSaveAndLoad()
    {
        FakeFileSystem files;
        files.setTime(source("a.cpp"), 100);
        files.setTime(source("shared.h"), 100);
        files.setTime(source("a.h"), 100);

        HeaderDependencyStore store(files.statFunction());
        store.insert(scan("a.cpp"), {source("shared.h"), source("a.h")}, 200);

        const FilePath storeFile = tempPath("subdir/header-deps");
        const Result<> saved = store.save(storeFile);
        QVERIFY2(saved.has_value(), errorOf(saved).constData());

        HeaderDependencyStore loaded(files.statFunction());
        const Result<> read = loaded.load(storeFile);
        QVERIFY2(read.has_value(), errorOf(read).constData());

        QCOMPARE(loaded.sources(), FilePaths{source("a.cpp")});
        QCOMPARE(Utils::toSet(loaded.dependencies(source("a.cpp"))),
                 Utils::toSet(FilePaths{source("shared.h"), source("a.h")}));
        QVERIFY(loaded.staleScans({scan("a.cpp")}).isEmpty());

        files.setTime(source("shared.h"), 300);
        loaded.forgetTimes();
        QCOMPARE(loaded.staleScans({scan("a.cpp")}).size(), 1);
    }

    void rejectsForeignStore()
    {
        const FilePath storeFile = tempPath("not-a-store");
        QVERIFY(storeFile.writeFileContents("certainly not a header dependency store"));

        HeaderDependencyStore store;
        QVERIFY(!store.load(storeFile));
        QVERIFY(store.isEmpty());
    }

    void rejectsTruncatedStore()
    {
        FakeFileSystem files;
        files.setTime(source("a.cpp"), 100);
        files.setTime(source("a.h"), 100);

        HeaderDependencyStore store(files.statFunction());
        store.insert(scan("a.cpp"), {source("a.h")}, 200);

        const FilePath storeFile = tempPath("truncated");
        QVERIFY(store.save(storeFile));

        const Result<QByteArray> contents = storeFile.fileContents();
        QVERIFY(contents.has_value());
        QVERIFY(storeFile.writeFileContents(contents->left(contents->size() - 4)));

        HeaderDependencyStore loaded(files.statFunction());
        QVERIFY(!loaded.load(storeFile));
        QVERIFY(loaded.isEmpty());
    }

    void projectHeadersDropSystemHeaders()
    {
        const FilePath sourceDir = tempPath("project");
        const FilePath buildDir = tempPath("project-build");

        const FilePaths dependencies{sourceDir / "main.cpp",
                                     sourceDir / "gui" / "mainwindow.h",
                                     buildDir / "ui_mainwindow.h",
                                     FilePath::fromString("/usr/include/stdio.h"),
                                     FilePath::fromString("/opt/qt/include/QtCore/qobject.h")};

        QCOMPARE(projectHeaders(dependencies, sourceDir, buildDir),
                 FilePaths({sourceDir / "main.cpp",
                            sourceDir / "gui" / "mainwindow.h",
                            buildDir / "ui_mainwindow.h"}));
    }

private:
    FilePath tempPath(const QString &name) const
    {
        return FilePath::fromString(m_temp.path()) / name;
    }

    FilePath source(const QString &name) const { return tempPath("project") / name; }

    SourceScan scan(const QString &name, quint64 flagsHash = 0) const
    {
        return {source(name), flagsHash};
    }

    QTemporaryDir m_temp;
};

QTEST_GUILESS_MAIN(HeaderDependenciesTest)
#include "tst_headerdependencies.moc"
