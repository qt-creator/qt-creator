// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

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
class QComboBox;
class QDialogButtonBox;
class QEvent;
class QLineEdit;
class QPushButton;
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
    void toggleExpanded();
    void itemChanged(QStandardItem *item);
    void textChanged(const QString& string);
    void selectBookmarkFolder(int index);
    void showContextMenu(const QPoint &point);
    void currentChanged(const QModelIndex& current);
    bool eventFilter(QObject *object, QEvent *e) override;

    QString m_url;
    QString m_title;

    QString oldText;
    QStandardItem *renameItem;

    BookmarkManager *bookmarkManager;
    QSortFilterProxyModel *proxyModel;

    QLineEdit *m_bookmarkEdit;
    QDialogButtonBox *m_buttonBox;
    QComboBox *m_bookmarkFolders;
    QPushButton *m_newFolderButton;
    QTreeView *m_treeView;
    QToolButton *m_toolButton;
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
    void showContextMenu(const QPoint &point);
    void setup();
    void expandItems();
    bool eventFilter(QObject *object, QEvent *event) override;

    QRegularExpression regExp;
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

    Qt::DropActions supportedDropActions() const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
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
