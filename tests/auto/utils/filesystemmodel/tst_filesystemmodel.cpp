// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <utils/filepath.h>
#include <utils/filesystemmodel.h>

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

using namespace Utils;

class tst_filesystemmodel : public QObject
{
    Q_OBJECT

private slots:
    void fetchingChanged();
    void fetchingChangedEmptyDir();
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

// The model resolves file icons through the platform theme, which requires a
// full QApplication; QTEST_GUILESS_MAIN crashes in the icon worker.
QTEST_MAIN(tst_filesystemmodel)

#include "tst_filesystemmodel.moc"
