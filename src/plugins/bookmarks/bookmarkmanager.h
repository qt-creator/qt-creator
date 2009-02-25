/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef BOOKMARKMANAGER_H
#define BOOKMARKMANAGER_H

#include <coreplugin/icontext.h>
#include <coreplugin/inavigationwidgetfactory.h>

#include <QtCore/QAbstractItemModel>
#include <QtCore/QList>
#include <QtGui/QListView>
#include <QtGui/QPixmap>
#include <QtGui/QStyledItemDelegate>

namespace ProjectExplorer {
class SessionManager;
}

namespace Core {
class IEditor;
}

namespace TextEditor {
class ITextEditor;
}

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
    void removeBookmark(Bookmark *bookmark); // Does not remove the mark
    Bookmark *bookmarkForIndex(QModelIndex index);

    enum State { NoBookMarks, HasBookMarks, HasBookmarksInDocument };
    State state() const;

    // Model stuff
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    void gotoBookmark(const QModelIndex &);

    // this QItemSelectionModel is shared by all views
    QItemSelectionModel *selectionModel() const;

    enum Roles {
        Filename = Qt::UserRole,
        LineNumber = Qt::UserRole + 1,
        Directory = Qt::UserRole + 2,
        LineText = Qt::UserRole + 3
    };

public slots:
    void toggleBookmark();
    void toggleBookmark(const QString &fileName, int lineNumber);
    void nextInDocument();
    void prevInDocument();
    void next();
    void prev();
    void moveUp();
    void moveDown();

signals:
    void updateActions(int state);
    void currentIndexChanged(const QModelIndex &);

private slots:
    void updateActionStatus();
    void gotoBookmark(Bookmark *bookmark);
    void loadBookmarks();

private:
    TextEditor::ITextEditor *currentTextEditor() const;
    ProjectExplorer::SessionManager* sessionManager() const;

    void documentPrevNext(bool next);

    Bookmark* findBookmark(const QString &path, const QString &fileName, int lineNumber);
    void addBookmark(Bookmark *bookmark, bool userset = true);
    void addBookmark(const QString &s);
    static QString bookmarkToString(const Bookmark *b);
    void saveBookmarks();

    typedef QMultiMap<QString, Bookmark *> FileNameBookmarksMap;
    typedef QMap<QString, FileNameBookmarksMap *> DirectoryFileBookmarksMap;

    DirectoryFileBookmarksMap m_bookmarksMap;

    QIcon m_bookmarkIcon;

    QList<Bookmark *> m_bookmarksList;
    QItemSelectionModel *m_selectionModel;
};

class BookmarkView : public QListView
{
    Q_OBJECT
public:
    BookmarkView(QWidget *parent = 0);
    ~BookmarkView();
    void setModel(QAbstractItemModel *model);
public slots:
    void gotoBookmark(const QModelIndex &index);
protected slots:
    void removeFromContextMenu();
    void removeAll();
protected:
    void contextMenuEvent(QContextMenuEvent *event);
    void removeBookmark(const QModelIndex &index);
private:
    BookmarkContext *m_bookmarkContext;
    QModelIndex m_contextMenuIndex;
};

class BookmarkContext : public Core::IContext
{
public:
    BookmarkContext(BookmarkView *widget);
    virtual QList<int> context() const;
    virtual QWidget *widget();
private:
    BookmarkView *m_bookmarkView;
    QList<int> m_context;
};

class BookmarkViewFactory : public Core::INavigationWidgetFactory
{
public:
    BookmarkViewFactory(BookmarkManager *bm);
    virtual QString displayName();
    virtual QKeySequence activationSequence();
    virtual Core::NavigationView createWidget();
private:
    BookmarkManager *m_manager;
};

class BookmarkDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    BookmarkDelegate(QObject * parent = 0);
    ~BookmarkDelegate();
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;

private:
    void generateGradientPixmap(int width, int height, QColor color, bool selected) const;
    mutable QPixmap *m_normalPixmap;
    mutable QPixmap *m_selectedPixmap;
};

} // namespace Internal
} // namespace Bookmarks

#endif // BOOKMARKMANAGER_H
