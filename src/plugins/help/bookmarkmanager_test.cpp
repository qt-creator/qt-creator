// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifdef WITH_TESTS

#include "bookmarkmanager_test.h"

#include "bookmarkmanager.h"

#include <QStandardItem>
#include <QTest>
#include <QTreeView>

namespace Help::Internal {

// In-process port of the bookmark handling of the Squish system test
// suite_HELP/tst_HELP06: creating nested bookmark folders, adding a bookmark
// and removing folders/bookmarks, keeping the tree and list models in sync.
class BookmarkManagerTest final : public QObject
{
    Q_OBJECT

private slots:
    void testBookmarkTree();
};

void BookmarkManagerTest::testBookmarkTree()
{
    BookmarkManager manager;
    BookmarkModel *tree = manager.treeBookmarkModel();
    BookmarkModel *list = manager.listBookmarkModel();

    // Create nested folders Sample > Folder 1 > Folder 2.
    QStandardItem *sample = tree->itemFromIndex(manager.addNewFolder({}));
    sample->setText("Sample");
    QStandardItem *folder1 = tree->itemFromIndex(manager.addNewFolder(sample->index()));
    folder1->setText("Folder 1");
    QStandardItem *folder2 = tree->itemFromIndex(manager.addNewFolder(folder1->index()));
    folder2->setText("Folder 2");

    // Add a bookmark inside Folder 2. It goes into both models.
    manager.addNewBookmark(folder2->index(), "Configuring Qt Creator", "qthelp://doc/index.html");

    QCOMPARE(tree->rowCount(), 1);        // Sample at the root
    QCOMPARE(sample->rowCount(), 1);      // Folder 1
    QCOMPARE(folder1->rowCount(), 1);     // Folder 2
    QCOMPARE(folder2->rowCount(), 1);     // the bookmark
    QCOMPARE(list->rowCount(), 1);
    QVERIFY(manager.bookmarkFolders().contains("Sample"));

    // A bare view is only used as the dialog parent; none of the removals below
    // touch a non-empty folder, so no confirmation dialog is shown.
    QTreeView view;

    // Removing the bookmark drops it from both the tree and the list model.
    manager.removeBookmarkItem(&view, folder2->child(0)->index());
    QCOMPARE(folder2->rowCount(), 0);
    QCOMPARE(list->rowCount(), 0);

    // Removing the now-empty folders unwinds the tree.
    manager.removeBookmarkItem(&view, folder2->index());
    QCOMPARE(folder1->rowCount(), 0);
    manager.removeBookmarkItem(&view, folder1->index());
    QCOMPARE(sample->rowCount(), 0);
    manager.removeBookmarkItem(&view, sample->index());
    QCOMPARE(tree->rowCount(), 0);
}

QObject *createBookmarkManagerTest()
{
    return new BookmarkManagerTest;
}

} // namespace Help::Internal

#include "bookmarkmanager_test.moc"

#endif // WITH_TESTS
