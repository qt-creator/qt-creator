// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/itemviews.h>
#include <utils/fileutils.h>
#include <coreplugin/inavigationwidgetfactory.h>

#include <QAbstractItemModel>
#include <QMultiMap>
#include <QList>
#include <QListView>
#include <QPixmap>
#include <QStyledItemDelegate>

namespace Core { class IContext; }

namespace TextEditor::Internal {

class Bookmark;
class BookmarksPlugin;
class BookmarkContext;

class BookmarkManager final : public QAbstractItemModel
{
    Q_OBJECT

public:
    BookmarkManager();
    ~BookmarkManager() final;

    void updateBookmark(Bookmark *bookmark);
    void updateBookmarkFileName(Bookmark *bookmark, const Utils::FilePath &oldFilePath);
    void deleteBookmark(Bookmark *bookmark); // Does not remove the mark
    void removeAllBookmarks();
    Bookmark *bookmarkForIndex(const QModelIndex &index) const;

    enum State { NoBookMarks, HasBookMarks, HasBookmarksInDocument };
    State state() const;

    // Model stuff
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const final;
    QModelIndex parent(const QModelIndex &child) const final;
    int rowCount(const QModelIndex &parent = QModelIndex()) const final;
    int columnCount(const QModelIndex &parent = QModelIndex()) const final;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const final;
    Qt::ItemFlags flags(const QModelIndex &index) const final;

    Qt::DropActions supportedDragActions() const final;
    QStringList mimeTypes() const final;
    QMimeData *mimeData(const QModelIndexList &indexes) const final;
    Qt::DropActions supportedDropActions() const final;
    bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const final;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) final;

    // this QItemSelectionModel is shared by all views
    QItemSelectionModel *selectionModel() const;

    bool hasBookmarkInPosition(const Utils::FilePath &fileName, int lineNumber);

    enum Roles {
        Filename = Qt::UserRole,
        LineNumber = Qt::UserRole + 1,
        Directory = Qt::UserRole + 2,
        LineText = Qt::UserRole + 3,
        Note = Qt::UserRole + 4
    };

    void toggleBookmark(const Utils::FilePath &fileName, int lineNumber);
    void nextInDocument();
    void prevInDocument();
    void next();
    void prev();
    void move(Bookmark* mark, int newRow);
    void moveUp();
    void moveDown();
    void edit();
    void editByFileAndLine(const Utils::FilePath &fileName, int lineNumber);
    bool gotoBookmark(const Bookmark *bookmark) const;

signals:
    void updateActions(bool enableToggle, int state);
    void currentIndexChanged(const QModelIndex &);

private:
    void updateActionStatus();
    void loadBookmarks();
    bool isAtCurrentBookmark() const;

    void documentPrevNext(bool next);

    Bookmark *findBookmark(const Utils::FilePath &filePath, int lineNumber);
    void insertBookmark(int index, Bookmark *bookmark, bool userset = true);
    void addBookmark(Bookmark *bookmark, bool userset = true);
    void addBookmark(const QString &s);
    static QString bookmarkToString(const Bookmark *b);
    void saveBookmarks();

    QMap<Utils::FilePath, QVector<Bookmark *>> m_bookmarksMap;

    QList<Bookmark *> m_bookmarksList;
    QItemSelectionModel *m_selectionModel;
};

class BookmarkViewFactory : public Core::INavigationWidgetFactory
{
public:
    BookmarkViewFactory(BookmarkManager *bm);

private:
    Core::NavigationView createWidget() override;

    BookmarkManager *m_manager;
};

} // Bookmarks::Internal
