/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "bookmarkmanager.h"

#include "localhelpmanager.h"

#include <utils/filterlineedit.h>
#include <utils/styledbar.h>

#include <QMenu>
#include <QIcon>
#include <QStyle>
#include <QLabel>
#include <QLayout>
#include <QEvent>
#include <QComboBox>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMessageBox>
#include <QHeaderView>
#include <QToolButton>
#include <QPushButton>
#include <QApplication>
#include <QDialogButtonBox>
#include <QSortFilterProxyModel>

#include <QHelpEngine>

using namespace Help::Internal;

BookmarkDialog::BookmarkDialog(BookmarkManager *manager, const QString &title,
        const QString &url, QWidget *parent)
    : QDialog(parent)
    , m_url(url)
    , m_title(title)
    , bookmarkManager(manager)
{
    installEventFilter(this);

    ui.setupUi(this);
    ui.bookmarkEdit->setText(title);
    ui.newFolderButton->setVisible(false);
    ui.buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
    ui.bookmarkFolders->addItems(bookmarkManager->bookmarkFolders());

    proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setFilterKeyColumn(0);
    proxyModel->setDynamicSortFilter(true);
    proxyModel->setFilterRole(Qt::UserRole + 10);
    proxyModel->setSourceModel(bookmarkManager->treeBookmarkModel());
    proxyModel->setFilterRegExp(QRegExp(QLatin1String("Folder"),
        Qt::CaseSensitive, QRegExp::FixedString));
    ui.treeView->setModel(proxyModel);

    ui.treeView->expandAll();
    ui.treeView->setVisible(false);
    ui.treeView->header()->setVisible(false);
    ui.treeView->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui.buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    connect(ui.buttonBox, SIGNAL(accepted()), this, SLOT(addAccepted()));
    connect(ui.newFolderButton, SIGNAL(clicked()), this, SLOT(addNewFolder()));
    connect(ui.toolButton, SIGNAL(clicked()), this, SLOT(toolButtonClicked()));
    connect(ui.bookmarkEdit, SIGNAL(textChanged(QString)), this,
        SLOT(textChanged(QString)));

    connect(bookmarkManager->treeBookmarkModel(),
        SIGNAL(itemChanged(QStandardItem*)),
        this, SLOT(itemChanged(QStandardItem*)));

    connect(ui.bookmarkFolders, SIGNAL(currentIndexChanged(QString)), this,
        SLOT(selectBookmarkFolder(QString)));

    connect(ui.treeView, SIGNAL(customContextMenuRequested(QPoint)), this,
        SLOT(customContextMenuRequested(QPoint)));

    connect(ui.treeView->selectionModel(), SIGNAL(currentChanged(QModelIndex,
        QModelIndex)), this, SLOT(currentChanged(QModelIndex)));
}

BookmarkDialog::~BookmarkDialog()
{
}

void BookmarkDialog::addAccepted()
{
    QItemSelectionModel *model = ui.treeView->selectionModel();
    const QModelIndexList &list = model->selection().indexes();

    QModelIndex index;
    if (!list.isEmpty())
        index = proxyModel->mapToSource(list.at(0));

    bookmarkManager->addNewBookmark(index, ui.bookmarkEdit->text(), m_url);
    accept();
}

void BookmarkDialog::addNewFolder()
{
    QItemSelectionModel *model = ui.treeView->selectionModel();
    const QModelIndexList &list = model->selection().indexes();

    QModelIndex index;
    if (!list.isEmpty())
        index = list.at(0);

    QModelIndex newFolder =
        bookmarkManager->addNewFolder(proxyModel->mapToSource(index));
    if (newFolder.isValid()) {
        ui.treeView->expand(index);
        const QModelIndex &index = proxyModel->mapFromSource(newFolder);
        model->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect);

        ui.bookmarkFolders->clear();
        ui.bookmarkFolders->addItems(bookmarkManager->bookmarkFolders());

        const QString &name = index.data().toString();
        ui.bookmarkFolders->setCurrentIndex(ui.bookmarkFolders->findText(name));
    }
    ui.treeView->setFocus();
}

void BookmarkDialog::toolButtonClicked()
{
    bool visible = !ui.treeView->isVisible();
    ui.treeView->setVisible(visible);
    ui.newFolderButton->setVisible(visible);

    if (visible) {
        resize(QSize(width(), 400));
        ui.toolButton->setText(QLatin1String("-"));
    } else {
        resize(width(), minimumHeight());
        ui.toolButton->setText(QLatin1String("+"));
    }
}

void BookmarkDialog::itemChanged(QStandardItem *item)
{
    if (renameItem != item) {
        renameItem = item;
        oldText = item->text();
        return;
    }

    if (item->text() != oldText) {
        ui.bookmarkFolders->clear();
        ui.bookmarkFolders->addItems(bookmarkManager->bookmarkFolders());

        QString name = tr("Bookmarks");
        const QModelIndex& index = ui.treeView->currentIndex();
        if (index.isValid())
            name = index.data().toString();
        ui.bookmarkFolders->setCurrentIndex(ui.bookmarkFolders->findText(name));
    }
}

void BookmarkDialog::textChanged(const QString& string)
{
    ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!string.isEmpty());
}

void BookmarkDialog::selectBookmarkFolder(const QString &folderName)
{
    if (folderName.isEmpty())
        return;

    if (folderName == tr("Bookmarks")) {
        ui.treeView->clearSelection();
        return;
    }

    QStandardItemModel *model = bookmarkManager->treeBookmarkModel();
    QList<QStandardItem*> list = model->findItems(folderName,
        Qt::MatchCaseSensitive | Qt::MatchRecursive, 0);
    if (!list.isEmpty()) {
        const QModelIndex &index = model->indexFromItem(list.at(0));
        QItemSelectionModel *model = ui.treeView->selectionModel();
        if (model) {
            model->setCurrentIndex(proxyModel->mapFromSource(index),
                QItemSelectionModel::ClearAndSelect);
        }
    }
}

void BookmarkDialog::customContextMenuRequested(const QPoint &point)
{
    QModelIndex index = ui.treeView->indexAt(point);
    if (!index.isValid())
        return;

    QMenu menu(QLatin1String(""), this);

    QAction *removeItem = menu.addAction(tr("Delete Folder"));
    QAction *renameItem = menu.addAction(tr("Rename Folder"));

    QAction *picked = menu.exec(ui.treeView->mapToGlobal(point));
    if (!picked)
        return;

    const QModelIndex &proxyIndex = proxyModel->mapToSource(index);
    if (picked == removeItem) {
        bookmarkManager->removeBookmarkItem(ui.treeView, proxyIndex);
        ui.bookmarkFolders->clear();
        ui.bookmarkFolders->addItems(bookmarkManager->bookmarkFolders());

        QString name = tr("Bookmarks");
        index = ui.treeView->currentIndex();
        if (index.isValid())
            name = index.data().toString();
        ui.bookmarkFolders->setCurrentIndex(ui.bookmarkFolders->findText(name));
    }
    else if (picked == renameItem) {
        BookmarkModel *model = bookmarkManager->treeBookmarkModel();
        if (QStandardItem *item = model->itemFromIndex(proxyIndex)) {
            item->setEditable(true);
            ui.treeView->edit(index);
            item->setEditable(false);
        }
    }
}

void BookmarkDialog::currentChanged(const QModelIndex &current)
{
    QString text = tr("Bookmarks");
    if (current.isValid())
        text = current.data().toString();
    ui.bookmarkFolders->setCurrentIndex(ui.bookmarkFolders->findText(text));
}

bool BookmarkDialog::eventFilter(QObject *object, QEvent *e)
{
    if (object == this && e->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent*>(e);

        QModelIndex index = ui.treeView->currentIndex();
        switch (ke->key()) {
            case Qt::Key_F2: {
                const QModelIndex &source = proxyModel->mapToSource(index);
                QStandardItem *item =
                    bookmarkManager->treeBookmarkModel()->itemFromIndex(source);
                if (item) {
                    item->setEditable(true);
                    ui.treeView->edit(index);
                    item->setEditable(false);
                }
            }   break;

            case Qt::Key_Delete: {
                bookmarkManager->removeBookmarkItem(ui.treeView,
                    proxyModel->mapToSource(index));
                ui.bookmarkFolders->clear();
                ui.bookmarkFolders->addItems(bookmarkManager->bookmarkFolders());

                QString name = tr("Bookmarks");
                index = ui.treeView->currentIndex();
                if (index.isValid())
                    name = index.data().toString();
                ui.bookmarkFolders->setCurrentIndex(ui.bookmarkFolders->findText(name));
            }   break;

            default:
                break;
        }
    }
    return QObject::eventFilter(object, e);
}


// #pragma mark -- BookmarkWidget


BookmarkWidget::BookmarkWidget(BookmarkManager *manager, QWidget *parent,
        bool showButtons)
    : QWidget(parent)
    , addButton(0)
    , removeButton(0)
    , bookmarkManager(manager)
{
    setup(showButtons);
    installEventFilter(this);
}

BookmarkWidget::~BookmarkWidget()
{
}

void BookmarkWidget::removeClicked()
{
    const QModelIndex& index = treeView->currentIndex();
    if (searchField->text().isEmpty()) {
        bookmarkManager->removeBookmarkItem(treeView,
            filterBookmarkModel->mapToSource(index));
    }
}

void BookmarkWidget::filterChanged()
{
    bool searchBookmarks = searchField->text().isEmpty();
    if (!searchBookmarks) {
        regExp.setPattern(searchField->text());
        filterBookmarkModel->setSourceModel(bookmarkManager->listBookmarkModel());
    } else {
        regExp.setPattern(QLatin1String(""));
        filterBookmarkModel->setSourceModel(bookmarkManager->treeBookmarkModel());
    }

    if (addButton)
        addButton->setEnabled(searchBookmarks);

    if (removeButton)
        removeButton->setEnabled(searchBookmarks);

    filterBookmarkModel->setFilterRegExp(regExp);

    const QModelIndex &index = treeView->indexAt(QPoint(1, 1));
    if (index.isValid())
        treeView->setCurrentIndex(index);

    if (searchBookmarks)
        expandItems();
}

void BookmarkWidget::expand(const QModelIndex& index)
{
    const QModelIndex& source = filterBookmarkModel->mapToSource(index);
    QStandardItem *item =
        bookmarkManager->treeBookmarkModel()->itemFromIndex(source);
    if (item)
        item->setData(treeView->isExpanded(index), Qt::UserRole + 11);
}

void BookmarkWidget::activated(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    QString data = index.data(Qt::UserRole + 10).toString();
    if (data != QLatin1String("Folder"))
        emit linkActivated(data);
}

void BookmarkWidget::customContextMenuRequested(const QPoint &point)
{
    QModelIndex index = treeView->indexAt(point);
    if (!index.isValid())
        return;

    QAction *showItem = 0;
    QAction *removeItem = 0;
    QAction *renameItem = 0;
    QAction *showItemNewTab = 0;

    QMenu menu(QLatin1String(""), this);
    QString data = index.data(Qt::UserRole + 10).toString();
    if (data == QLatin1String("Folder")) {
        removeItem = menu.addAction(tr("Delete Folder"));
        renameItem = menu.addAction(tr("Rename Folder"));
    } else {
        showItem = menu.addAction(tr("Show Bookmark"));
        showItemNewTab = menu.addAction(tr("Show Bookmark as New Page"));
        if (searchField->text().isEmpty()) {
            menu.addSeparator();
            removeItem = menu.addAction(tr("Delete Bookmark"));
            renameItem = menu.addAction(tr("Rename Bookmark"));
        }
    }

    QAction *pickedAction = menu.exec(treeView->mapToGlobal(point));
    if (!pickedAction)
        return;

    if (pickedAction == showItem)
        emit linkActivated(data);
    else if (pickedAction == showItemNewTab)
        emit createPage(QUrl(data), false);
    else if (pickedAction == removeItem) {
        bookmarkManager->removeBookmarkItem(treeView,
            filterBookmarkModel->mapToSource(index));
    }
    else if (pickedAction == renameItem) {
        const QModelIndex &source = filterBookmarkModel->mapToSource(index);
        QStandardItem *item =
            bookmarkManager->treeBookmarkModel()->itemFromIndex(source);
        if (item) {
            item->setEditable(true);
            treeView->edit(index);
            item->setEditable(false);
        }
    }
}

void BookmarkWidget::setup(bool showButtons)
{
    regExp.setPatternSyntax(QRegExp::FixedString);
    regExp.setCaseSensitivity(Qt::CaseInsensitive);

    QLayout *vlayout = new QVBoxLayout(this);
    vlayout->setMargin(0);
    vlayout->setSpacing(0);

    searchField = new Utils::FilterLineEdit(this);
    setFocusProxy(searchField);

    Utils::StyledBar *toolbar = new Utils::StyledBar(this);
    toolbar->setSingleRow(false);
    QLayout *tbLayout = new QHBoxLayout();
    tbLayout->setMargin(4);
    tbLayout->addWidget(searchField);
    toolbar->setLayout(tbLayout);

    vlayout->addWidget(toolbar);

    searchField->installEventFilter(this);
    connect(searchField, SIGNAL(textChanged(QString)), this,
        SLOT(filterChanged()));

    treeView = new TreeView(this);
    treeView->setFrameStyle(QFrame::NoFrame);
    vlayout->addWidget(treeView);

#ifdef Q_OS_MAC
#   define SYSTEM "mac"
#else
#   define SYSTEM "win"
#endif

    if (showButtons) {
        QLayout *hlayout = new QHBoxLayout();
        vlayout->addItem(hlayout);

        hlayout->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding));

        addButton = new QToolButton(this);
        addButton->setText(tr("Add"));
        addButton->setIcon(QIcon(QLatin1String(":/trolltech/assistant/images/"
            SYSTEM "/addtab.png")));
        addButton->setAutoRaise(true);
        addButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        hlayout->addWidget(addButton);
        connect(addButton, SIGNAL(clicked()), this, SIGNAL(addBookmark()));

        removeButton = new QToolButton(this);
        removeButton->setText(tr("Remove"));
        removeButton->setIcon(QIcon(QLatin1String(":/trolltech/assistant/images/"
            SYSTEM "/closetab.png")));
        removeButton->setAutoRaise(true);
        removeButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        hlayout->addWidget(removeButton);
        connect(removeButton, SIGNAL(clicked()), this, SLOT(removeClicked()));
    }

    filterBookmarkModel = new QSortFilterProxyModel(this);
    treeView->setModel(filterBookmarkModel);

    treeView->setDragEnabled(true);
    treeView->setAcceptDrops(true);
    treeView->setAutoExpandDelay(1000);
    treeView->setDropIndicatorShown(true);
    treeView->header()->setVisible(false);
    treeView->viewport()->installEventFilter(this);
    treeView->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(treeView, SIGNAL(expanded(QModelIndex)), this,
        SLOT(expand(QModelIndex)));
    connect(treeView, SIGNAL(collapsed(QModelIndex)), this,
        SLOT(expand(QModelIndex)));
    connect(treeView, SIGNAL(activated(QModelIndex)), this,
        SLOT(activated(QModelIndex)));
    connect(treeView, SIGNAL(customContextMenuRequested(QPoint)),
        this, SLOT(customContextMenuRequested(QPoint)));

    filterBookmarkModel->setFilterKeyColumn(0);
    filterBookmarkModel->setDynamicSortFilter(true);
    filterBookmarkModel->setSourceModel(bookmarkManager->treeBookmarkModel());

    expandItems();
}

void BookmarkWidget::expandItems()
{
    QStandardItemModel *model = bookmarkManager->treeBookmarkModel();
    QList<QStandardItem*>list = model->findItems(QLatin1String("*"),
        Qt::MatchWildcard | Qt::MatchRecursive, 0);
    foreach (const QStandardItem* item, list) {
        const QModelIndex& index = model->indexFromItem(item);
        treeView->setExpanded(filterBookmarkModel->mapFromSource(index),
            item->data(Qt::UserRole + 11).toBool());
    }
}

bool BookmarkWidget::eventFilter(QObject *object, QEvent *e)
{
    if ((object == this) || (object == treeView->viewport())) {
        QModelIndex index = treeView->currentIndex();
        if (e->type() == QEvent::KeyPress) {
            QKeyEvent *ke = static_cast<QKeyEvent*>(e);
            if (index.isValid() && searchField->text().isEmpty()) {
                const QModelIndex &src = filterBookmarkModel->mapToSource(index);
                if (ke->key() == Qt::Key_F2) {
                    QStandardItem *item =
                        bookmarkManager->treeBookmarkModel()->itemFromIndex(src);
                    if (item) {
                        item->setEditable(true);
                        treeView->edit(index);
                        item->setEditable(false);
                    }
                } else if (ke->key() == Qt::Key_Delete) {
                    bookmarkManager->removeBookmarkItem(treeView, src);
                }
            }

            switch (ke->key()) {
                default: break;
                case Qt::Key_Up: {
                case Qt::Key_Down:
                    treeView->subclassKeyPressEvent(ke);
                }   break;

                case Qt::Key_Enter: {
                case Qt::Key_Return:
                    index = treeView->selectionModel()->currentIndex();
                    if (index.isValid()) {
                        QString data = index.data(Qt::UserRole + 10).toString();
                        if (!data.isEmpty() && data != QLatin1String("Folder"))
                            emit linkActivated(data);
                    }
                }   break;
            }
        } else if (e->type() == QEvent::MouseButtonRelease) {
            if (index.isValid()) {
                QMouseEvent *me = static_cast<QMouseEvent*>(e);
                bool controlPressed = me->modifiers() & Qt::ControlModifier;
                if (((me->button() == Qt::LeftButton) && controlPressed)
                    || (me->button() == Qt::MidButton)) {
                        QString data = index.data(Qt::UserRole + 10).toString();
                        if (!data.isEmpty() && data != QLatin1String("Folder"))
                            emit createPage(QUrl(data), false);
                }
            }
        }
    } else if (object == searchField && e->type() == QEvent::FocusIn) {
        if (static_cast<QFocusEvent *>(e)->reason() != Qt::MouseFocusReason) {
            searchField->selectAll();
            searchField->setFocus();

            QModelIndex index = treeView->indexAt(QPoint(1, 1));
            if (index.isValid())
                treeView->setCurrentIndex(index);
        }
    }
    return QWidget::eventFilter(object, e);
}


// #pragma mark -- BookmarkModel


BookmarkModel::BookmarkModel(int rows, int columns, QObject * parent)
    : QStandardItemModel(rows, columns, parent)
{
}

BookmarkModel::~BookmarkModel()
{
}

Qt::DropActions BookmarkModel::supportedDropActions() const
{
    return Qt::MoveAction;
}

Qt::ItemFlags BookmarkModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags defaultFlags = QStandardItemModel::flags(index);
    if ((!index.isValid()) // can only happen for the invisible root item
        || index.data(Qt::UserRole + 10).toString() == QLatin1String("Folder"))
        return (Qt::ItemIsDropEnabled | defaultFlags) &~ Qt::ItemIsDragEnabled;

    return (Qt::ItemIsDragEnabled | defaultFlags) &~ Qt::ItemIsDropEnabled;
}


// #pragma mark -- BookmarkManager


BookmarkManager::BookmarkManager()
    : m_folderIcon(QApplication::style()->standardIcon(QStyle::SP_DirClosedIcon))
    , m_bookmarkIcon(QLatin1String(":/help/images/bookmark.png"))
    , treeModel(new BookmarkModel(0, 1, this))
    , listModel(new BookmarkModel(0, 1, this))
{
    connect(treeModel, SIGNAL(itemChanged(QStandardItem*)), this,
        SLOT(itemChanged(QStandardItem*)));
}

BookmarkManager::~BookmarkManager()
{
    treeModel->clear();
    listModel->clear();
}

BookmarkModel* BookmarkManager::treeBookmarkModel() const
{
    return treeModel;
}

BookmarkModel* BookmarkManager::listBookmarkModel() const
{
    return listModel;
}

void BookmarkManager::saveBookmarks()
{
    QByteArray bookmarks;
    QDataStream stream(&bookmarks, QIODevice::WriteOnly);

    readBookmarksRecursive(treeModel->invisibleRootItem(), stream, 0);
    (&LocalHelpManager::helpEngine())->setCustomValue(QLatin1String("Bookmarks"),
        bookmarks);
}

QStringList BookmarkManager::bookmarkFolders() const
{
    QStringList folders(tr("Bookmarks"));

    QList<QStandardItem*>list = treeModel->findItems(QLatin1String("*"),
        Qt::MatchWildcard | Qt::MatchRecursive, 0);

    QString data;
    foreach (const QStandardItem *item, list) {
        data = item->data(Qt::UserRole + 10).toString();
        if (data == QLatin1String("Folder"))
            folders << item->data(Qt::DisplayRole).toString();
    }
    return folders;
}

QModelIndex BookmarkManager::addNewFolder(const QModelIndex& index)
{
    QStandardItem *item = new QStandardItem(uniqueFolderName());
    item->setEditable(false);
    item->setIcon(m_folderIcon);
    item->setData(false, Qt::UserRole + 11);
    item->setData(QLatin1String("Folder"), Qt::UserRole + 10);
    item->setIcon(QApplication::style()->standardIcon(QStyle::SP_DirClosedIcon));

    if (index.isValid())
        treeModel->itemFromIndex(index)->appendRow(item);
    else
        treeModel->appendRow(item);
    return treeModel->indexFromItem(item);
}

void BookmarkManager::removeBookmarkItem(QTreeView *treeView,
    const QModelIndex& index)
{
    QStandardItem *item = treeModel->itemFromIndex(index);
    if (item) {
        QString data = index.data(Qt::UserRole + 10).toString();
        if (data == QLatin1String("Folder") && item->rowCount() > 0) {
            int value = QMessageBox::question(treeView, tr("Remove"),
                tr("Deleting a folder also removes its content.<br>"
                "Do you want to continue?"),
                QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);

            if (value == QMessageBox::Cancel)
                return;
        }

        if (data != QLatin1String("Folder")) {
            QList<QStandardItem*>itemList = listModel->findItems(item->text());
            foreach (const QStandardItem *i, itemList) {
                if (i->data(Qt::UserRole + 10) == data) {
                    listModel->removeRow(i->row());
                    break;
                }
            }
        } else {
            removeBookmarkFolderItems(item);
        }
        treeModel->removeRow(item->row(), index.parent());
    }
}

void BookmarkManager::showBookmarkDialog(QWidget* parent, const QString &name,
    const QString &url)
{
    BookmarkDialog dialog(this, name, url, parent);
    dialog.exec();
}

void BookmarkManager::addNewBookmark(const QModelIndex& index,
    const QString &name, const QString &url)
{
    QStandardItem *item = new QStandardItem(name);
    item->setEditable(false);
    item->setIcon(m_bookmarkIcon);
    item->setData(false, Qt::UserRole + 11);
    item->setData(url, Qt::UserRole + 10);

    if (index.isValid())
        treeModel->itemFromIndex(index)->appendRow(item);
    else
        treeModel->appendRow(item);
    listModel->appendRow(item->clone());
}

void BookmarkManager::itemChanged(QStandardItem *item)
{
    if (renameItem != item) {
        renameItem = item;
        oldText = item->text();
        return;
    }

    if (item->text() != oldText) {
        if (item->data(Qt::UserRole + 10).toString() != QLatin1String("Folder")) {
            QList<QStandardItem*>itemList = listModel->findItems(oldText);
            if (itemList.count() > 0)
                itemList.at(0)->setText(item->text());
        }
    }
}

void BookmarkManager::setupBookmarkModels()
{
    treeModel->clear();
    listModel->clear();

    qint32 depth;
    bool expanded;
    QString name, type;
    QList<int> lastDepths;
    QList<QStandardItem*> parents;

    QByteArray ba = LocalHelpManager::helpEngine()
        .customValue(QLatin1String("Bookmarks")).toByteArray();
    QDataStream stream(ba);
    while (!stream.atEnd()) {
        stream >> depth >> name >> type >> expanded;

        QStandardItem *item = new QStandardItem(name);
        item->setEditable(false);
        item->setData(type, Qt::UserRole + 10);
        item->setData(expanded, Qt::UserRole + 11);
        if (depth == 0) {
            parents.clear(); lastDepths.clear();
            treeModel->appendRow(item);
            parents << item; lastDepths << depth;
        } else {
            if (depth <= lastDepths.last()) {
                while (depth <= lastDepths.last() && parents.count() > 0) {
                    parents.pop_back(); lastDepths.pop_back();
                }
            }
            parents.last()->appendRow(item);
            if (type == QLatin1String("Folder"))
                parents << item; lastDepths << depth;
        }

        if (type != QLatin1String("Folder")) {
            item->setIcon(m_bookmarkIcon);
            listModel->appendRow(item->clone());
        } else {
            item->setIcon(m_folderIcon);
        }
    }
}

QString BookmarkManager::uniqueFolderName() const
{
    QString folderName = tr("New Folder");
    QList<QStandardItem*> list = treeModel->findItems(folderName,
        Qt::MatchContains | Qt::MatchRecursive, 0);
    if (!list.isEmpty()) {
        QStringList names;
        foreach (const QStandardItem *item, list)
            names << item->text();

        QString folderNameBase = tr("New Folder") + QLatin1String(" %1");
        for (int i = 1; i <= names.count(); ++i) {
            folderName = folderNameBase.arg(i);
            if (!names.contains(folderName))
                break;
        }
    }
    return folderName;
}

void BookmarkManager::removeBookmarkFolderItems(QStandardItem *item)
{
    for (int j = 0; j < item->rowCount(); ++j) {
        QStandardItem *child = item->child(j);
        if (child->rowCount() > 0)
            removeBookmarkFolderItems(child);

        QString data = child->data(Qt::UserRole + 10).toString();
        QList<QStandardItem*>itemList = listModel->findItems(child->text());
        foreach (const QStandardItem *i, itemList) {
            if (i->data(Qt::UserRole + 10) == data) {
                listModel->removeRow(i->row());
                break;
            }
        }
    }
}

void BookmarkManager::readBookmarksRecursive(const QStandardItem *item,
    QDataStream &stream, const qint32 depth) const
{
    for (int j = 0; j < item->rowCount(); ++j) {
        const QStandardItem *child = item->child(j);
        stream << depth;
        stream << child->data(Qt::DisplayRole).toString();
        stream << child->data(Qt::UserRole + 10).toString();
        stream << child->data(Qt::UserRole + 11).toBool();

        if (child->rowCount() > 0)
            readBookmarksRecursive(child, stream, (depth +1));
    }
}
