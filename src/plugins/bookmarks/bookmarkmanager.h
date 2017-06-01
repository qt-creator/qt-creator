/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <utils/itemviews.h>
#include <coreplugin/inavigationwidgetfactory.h>

#include <QAbstractItemModel>
#include <QMultiMap>
#include <QList>
#include <QListView>
#include <QPixmap>
#include <QStyledItemDelegate>

namespace Core { class IContext; }

namespace Bookmarks {
namespace Internal {

class Bookmark;
class BookmarksPlugin;
class BookmarkContext;

class BookmarkManager : public QAbstractItemModel
{
    Q_OBJECT

public:
    BookmarkManager();
    ~BookmarkManager();

    void updateBookmark(Bookmark *bookmark);
    void updateBookmarkFileName(Bookmark *bookmark, const QString &oldFileName);
    void deleteBookmark(Bookmark *bookmark); // Does not remove the mark
    void removeAllBookmarks();
    Bookmark *bookmarkForIndex(const QModelIndex &index) const;

    enum State { NoBookMarks, HasBookMarks, HasBookmarksInDocument };
    State state() const;

    // Model stuff
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

    Qt::DropActions supportedDragActions() const;
    QStringList mimeTypes() const;
    QMimeData *mimeData(const QModelIndexList &indexes) const;

    // this QItemSelectionModel is shared by all views
    QItemSelectionModel *selectionModel() const;

    bool hasBookmarkInPosition(const QString &fileName, int lineNumber);

    enum Roles {
        Filename = Qt::UserRole,
        LineNumber = Qt::UserRole + 1,
        Directory = Qt::UserRole + 2,
        LineText = Qt::UserRole + 3,
        Note = Qt::UserRole + 4
    };

    void toggleBookmark(const QString &fileName, int lineNumber);
    void nextInDocument();
    void prevInDocument();
    void next();
    void prev();
    void moveUp();
    void moveDown();
    void edit();
    void editByFileAndLine(const QString &fileName, int lineNumber);
    bool gotoBookmark(Bookmark *bookmark);

signals:
    void updateActions(bool enableToggle, int state);
    void currentIndexChanged(const QModelIndex &);

private:
    void updateActionStatus();
    void loadBookmarks();

    void documentPrevNext(bool next);

    Bookmark *findBookmark(const QString &filePath, int lineNumber);
    void addBookmark(Bookmark *bookmark, bool userset = true);
    void addBookmark(const QString &s);
    void addBookmarkToMap(Bookmark *bookmark);
    bool removeBookmarkFromMap(Bookmark *bookmark, const QString &fileName = QString());
    static QString bookmarkToString(const Bookmark *b);
    void saveBookmarks();

    typedef QMultiMap<QString, Bookmark *> FileNameBookmarksMap;
    typedef QMap<QString, FileNameBookmarksMap *> DirectoryFileBookmarksMap;

    DirectoryFileBookmarksMap m_bookmarksMap;

    QList<Bookmark *> m_bookmarksList;
    QItemSelectionModel *m_selectionModel;
};

class BookmarkView : public Utils::ListView
{
    Q_OBJECT

public:
    explicit BookmarkView(BookmarkManager *manager);
    ~BookmarkView();

public slots:
    void gotoBookmark(const QModelIndex &index);

protected slots:
    void removeFromContextMenu();
    void removeAll();

protected:
    void contextMenuEvent(QContextMenuEvent *event);
    void removeBookmark(const QModelIndex &index);
    void keyPressEvent(QKeyEvent *event);

private:
    Core::IContext *m_bookmarkContext;
    QModelIndex m_contextMenuIndex;
    BookmarkManager *m_manager;
};

class BookmarkViewFactory : public Core::INavigationWidgetFactory
{
    Q_OBJECT

public:
    BookmarkViewFactory(BookmarkManager *bm);

private:
    Core::NavigationView createWidget() override;

    BookmarkManager *m_manager;
};

class BookmarkDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    BookmarkDelegate(QObject *parent = 0);

private:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void generateGradientPixmap(int width, int height, const QColor &color, bool selected) const;

    mutable QPixmap m_normalPixmap;
    mutable QPixmap m_selectedPixmap;
};

} // namespace Internal
} // namespace Bookmarks
