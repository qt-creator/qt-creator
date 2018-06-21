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

#include "ui_bookmarkdialog.h"

#include <utils/navigationtreeview.h>

#include <QUrl>
#include <QObject>
#include <QString>
#include <QByteArray>
#include <QDataStream>

#include <QIcon>
#include <QDialog>
#include <QWidget>
#include <QTreeView>
#include <QStandardItemModel>

QT_BEGIN_NAMESPACE
class QEvent;
class QLineEdit;
class QTreeView;
class QToolButton;
class QStandardItem;
class QAbstractItemModel;
class QSortFilterProxyModel;
QT_END_NAMESPACE

namespace Utils { class FancyLineEdit; }

class BookmarkManager;

class BookmarkDialog : public QDialog
{
    Q_OBJECT

public:
    BookmarkDialog(BookmarkManager *manager, const QString &title,
        const QString &url, QWidget *parent = 0);
    ~BookmarkDialog();

private:
    void addAccepted();
    void addNewFolder();
    void toolButtonClicked();
    void itemChanged(QStandardItem *item);
    void textChanged(const QString& string);
    void selectBookmarkFolder(const QString &folderName);
    void customContextMenuRequested(const QPoint &point);
    void currentChanged(const QModelIndex& current);
    bool eventFilter(QObject *object, QEvent *e);

    QString m_url;
    QString m_title;

    QString oldText;
    QStandardItem *renameItem;

    Ui::BookmarkDialog ui;
    BookmarkManager *bookmarkManager;
    QSortFilterProxyModel *proxyModel;
};

class TreeView : public Utils::NavigationTreeView
{
    Q_OBJECT

public:
    TreeView(QWidget* parent = 0) : Utils::NavigationTreeView(parent) {}
    void subclassKeyPressEvent(QKeyEvent* event)
    {
        Utils::NavigationTreeView::keyPressEvent(event);
    }
};

class BookmarkWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BookmarkWidget(BookmarkManager *manager, QWidget *parent = 0);
    ~BookmarkWidget();

    void setOpenInNewPageActionVisible(bool visible);

signals:
    void addBookmark();
    void linkActivated(const QUrl &url);
    void createPage(const QUrl &url, bool fromSearch);

private:
    void filterChanged();
    void expand(const QModelIndex& index);
    void activated(const QModelIndex &index);
    void customContextMenuRequested(const QPoint &point);
    void setup();
    void expandItems();
    bool eventFilter(QObject *object, QEvent *event);

    QRegExp regExp;
    TreeView *treeView;
    Utils::FancyLineEdit *searchField;
    BookmarkManager *bookmarkManager;
    QSortFilterProxyModel* filterBookmarkModel;
    bool m_isOpenInNewPageActionVisible;
};

class BookmarkModel : public QStandardItemModel
{
    Q_OBJECT

public:
    BookmarkModel(int rows, int columns, QObject *parent = 0);
    ~BookmarkModel();

    Qt::DropActions supportedDropActions() const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
};

class BookmarkManager : public QObject
{
    Q_OBJECT

public:
    BookmarkManager();
    ~BookmarkManager();

    BookmarkModel *treeBookmarkModel() const;
    BookmarkModel *listBookmarkModel() const;

    void saveBookmarks();
    QStringList bookmarkFolders() const;
    QModelIndex addNewFolder(const QModelIndex &index);
    void removeBookmarkItem(QTreeView *treeView, const QModelIndex& index);
    void showBookmarkDialog(QWidget *parent, const QString &name, const QString &url);
    void addNewBookmark(const QModelIndex &index, const QString &name, const QString &url);
    void setupBookmarkModels();

private:
    void itemChanged(QStandardItem *item);
    QString uniqueFolderName() const;
    void removeBookmarkFolderItems(QStandardItem *item);
    void readBookmarksRecursive(const QStandardItem *item, QDataStream &stream,
        const qint32 depth) const;

    const QIcon m_folderIcon;
    const QIcon m_bookmarkIcon;

    QString oldText;

    BookmarkModel *treeModel;
    BookmarkModel *listModel;
    QStandardItem *renameItem;
    bool m_isModelSetup = false;
};
