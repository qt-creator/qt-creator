// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <utils/filepath.h>
#include <utils/filesystemmodel.h>

#include <QRegularExpression>
#include <QSignalSpy>
#include <QSortFilterProxyModel>
#include <QTemporaryDir>
#include <QTest>

using namespace Utils;

class tst_filesystemmodel : public QObject
{
    Q_OBJECT

private slots:
    void fetchingChanged();
    void fetchingChangedEmptyDir();
    void removeRowsKeepsProxyMappingConsistent();
};

// The model reports the start and end of the asynchronous directory listing
// via fetchingChanged(path, true/false), with directoryLoaded and the rows
// available by the time the end is reported.
void tst_filesystemmodel::fetchingChanged()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const FilePath root = FilePath::fromString(tempDir.path());
    QVERIFY(root.pathAppended("a.txt").writeFileContents("a"));
    QVERIFY(root.pathAppended("b.txt").writeFileContents("b"));

    FileSystemModel model;
    QSignalSpy fetchingSpy(&model, &FileSystemModel::fetchingChanged);
    QSignalSpy loadedSpy(&model, &FileSystemModel::directoryLoaded);

    model.setRootPath(root);
    const QModelIndex rootIndex = model.index(0, 0);
    QVERIFY(rootIndex.isValid());
    QVERIFY(model.canFetchMore(rootIndex));

    model.fetchMore(rootIndex);

    // The fetch start is reported synchronously ...
    QCOMPARE(fetchingSpy.size(), 1);
    QCOMPARE(fetchingSpy.at(0).at(0).value<FilePath>(), root);
    QCOMPARE(fetchingSpy.at(0).at(1).toBool(), true);

    // ... a second fetchMore while fetching must not restart the fetch ...
    QVERIFY(!model.canFetchMore(rootIndex));
    model.fetchMore(rootIndex);
    QCOMPARE(fetchingSpy.size(), 1);

    // ... and the end arrives once the worker thread delivered the listing.
    QTRY_COMPARE(fetchingSpy.size(), 2);
    QCOMPARE(fetchingSpy.at(1).at(0).value<FilePath>(), root);
    QCOMPARE(fetchingSpy.at(1).at(1).toBool(), false);

    QCOMPARE(loadedSpy.size(), 1);
    QCOMPARE(model.rowCount(rootIndex), 2);
}

// Empty directories complete the fetching cycle, too. The spinner in
// FileDialog relies on the false notification to ever arrive.
void tst_filesystemmodel::fetchingChangedEmptyDir()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const FilePath root = FilePath::fromString(tempDir.path());

    FileSystemModel model;
    QSignalSpy fetchingSpy(&model, &FileSystemModel::fetchingChanged);

    model.setRootPath(root);
    const QModelIndex rootIndex = model.index(0, 0);
    model.fetchMore(rootIndex);

    QTRY_COMPARE(fetchingSpy.size(), 2);
    QCOMPARE(fetchingSpy.at(1).at(1).toBool(), false);
    QCOMPARE(model.rowCount(rootIndex), 0);
}

// Removing rows must not leave stale rows on the remaining siblings while the
// removal is reported. Receivers of rowsRemoved, like a proxy model mapping an
// expanded subdirectory's items, call parent() on the grandchildren. A wrong
// row corrupts the proxy's mapping, which crashes on a later removal.
void tst_filesystemmodel::removeRowsKeepsProxyMappingConsistent()
{
    QTest::failOnWarning(QRegularExpression("QSortFilterProxyModel"));

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const FilePath root = FilePath::fromString(tempDir.path());
    // Only directories, so that every row after a removed one has children.
    const int dirCount = 6;
    for (int i = 0; i < dirCount; ++i) {
        const FilePath dir = root.pathAppended(QString("d%1").arg(i));
        QVERIFY(dir.createDir());
        QVERIFY(dir.pathAppended("inner.txt").writeFileContents("x"));
    }

    FileSystemModel model;
    QSortFilterProxyModel proxy;
    proxy.setSourceModel(&model);
    QSignalSpy loadedSpy(&model, &FileSystemModel::directoryLoaded);

    model.setRootPath(root);
    const QModelIndex rootIndex = model.index(0, 0);
    model.fetchMore(rootIndex);
    QTRY_COMPARE(loadedSpy.size(), 1);
    QCOMPARE(model.rowCount(rootIndex), dirCount);

    FilePaths dirs;
    for (int row = 0; row < dirCount; ++row) {
        const QModelIndex dirIndex = model.index(row, 0, rootIndex);
        dirs.append(model.filePath(dirIndex));
        model.fetchMore(dirIndex);
    }
    QTRY_COMPARE(loadedSpy.size(), 1 + dirCount);

    const auto mapInnerFiles = [&] {
        for (const FilePath &dir : std::as_const(dirs))
            proxy.mapFromSource(model.index(dir.pathAppended("inner.txt")));
    };
    mapInnerFiles();
    connect(&model, &QAbstractItemModel::rowsRemoved, &proxy, mapInnerFiles);

    // Each removal re-keys the proxy's mappings of the following siblings, so
    // it takes more than one to run into a corrupted mapping.
    for (int removed = 1; removed <= 3; ++removed) {
        QVERIFY(model.filePath(model.index(0, 0, rootIndex)).removeRecursively());
        QTRY_COMPARE(model.rowCount(rootIndex), dirCount - removed);
    }

    for (int row = 0; row < model.rowCount(rootIndex); ++row) {
        const QModelIndex sourceDir = model.index(row, 0, rootIndex);
        const QModelIndex sourceInner = model.index(model.filePath(sourceDir) / "inner.txt");
        QCOMPARE(model.parent(sourceInner), sourceDir);
        const QModelIndex proxyInner = proxy.mapFromSource(sourceInner);
        QVERIFY(proxyInner.isValid());
        QCOMPARE(proxy.mapToSource(proxyInner.parent()), sourceDir);
    }
}

// The model resolves file icons through the platform theme, which requires a
// full QApplication; QTEST_GUILESS_MAIN crashes in the icon worker.
QTEST_MAIN(tst_filesystemmodel)

#include "tst_filesystemmodel.moc"
