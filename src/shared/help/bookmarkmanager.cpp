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

#include "bookmarkmanager.h"
#include "centralwidget.h"

#include <QtGui/QMenu>
#include <QtGui/QIcon>
#include <QtGui/QStyle>
#include <QtGui/QLabel>
#include <QtGui/QLayout>
#include <QtCore/QEvent>
#include <QtGui/QComboBox>
#include <QtGui/QKeyEvent>
#include <QtGui/QLineEdit>
#include <QtGui/QMessageBox>
#include <QtGui/QHeaderView>
#include <QtGui/QToolButton>
#include <QtGui/QPushButton>
#include <QtGui/QApplication>
#include <QtHelp/QHelpEngineCore>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QSortFilterProxyModel>

QT_BEGIN_NAMESPACE

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
    connect(ui.bookmarkEdit, SIGNAL(textChanged(const QString&)), this,
        SLOT(textChanged(const QString&)));

    connect(bookmarkManager->treeBookmarkModel(), SIGNAL(itemChanged(QStandardItem*)),
        this, SLOT(itemChanged(QStandardItem*)));

    connect(ui.bookmarkFolders, SIGNAL(currentIndexChanged(const QString&)), this,
        SLOT(selectBookmarkFolder(const QString&)));

    connect(ui.treeView, SIGNAL(customContextMenuRequested(const QPoint&)), this,
        SLOT(customContextMenuRequested(const QPoint&)));

    connect(ui.treeView->selectionModel(),
        SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)),
        this, SLOT(currentChanged(const QModelIndex&, const QModelIndex&)));
}

BookmarkDialog::~BookmarkDialog()
{
}

void BookmarkDialog::addAccepted()
{
    const QItemSelection selection = ui.treeView->selectionModel()->selection();
    const QModelIndexList list = selection.indexes();

    QModelIndex index;
    if (!list.isEmpty())
        index = proxyModel->mapToSource(list.at(0));

    bookmarkManager->addNewBookmark(index, ui.bookmarkEdit->text(), m_url);
    accept();
}

void BookmarkDialog::addNewFolder()
{
    const QItemSelection selection = ui.treeView->selectionModel()->selection();
    const QModelIndexList list = selection.indexes();

    QModelIndex index;
    if (!list.isEmpty())
        index = list.at(0);

    QModelIndex newFolder =
        bookmarkManager->addNewFolder(proxyModel->mapToSource(index));
    if (newFolder.isValid()) {
        ui.treeView->expand(index);
        const QModelIndex &index = proxyModel->mapFromSource(newFolder);
        ui.treeView->selectionModel()->setCurrentIndex(index,
            QItemSelectionModel::ClearAndSelect);

        ui.bookmarkFolders->clear();
        ui.bookmarkFolders->addItems(bookmarkManager->bookmarkFolders());

        const QString name = index.data().toString();
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
        QModelIndex index = model->indexFromItem(list.at(0));
        ui.treeView->selectionModel()->setCurrentIndex(
            proxyModel->mapFromSource(index), QItemSelectionModel::ClearAndSelect);
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

    QAction *picked_action = menu.exec(ui.treeView->mapToGlobal(point));
    if (!picked_action)
        return;

    if (picked_action == removeItem) {
        bookmarkManager->removeBookmarkItem(ui.treeView,
            proxyModel->mapToSource(index));
        ui.bookmarkFolders->clear();
        ui.bookmarkFolders->addItems(bookmarkManager->bookmarkFolders());

        QString name = tr("Bookmarks");
        index = ui.treeView->currentIndex();
        if (index.isValid())
            name = index.data().toString();
        ui.bookmarkFolders->setCurrentIndex(ui.bookmarkFolders->findText(name));
    }
    else if (picked_action == renameItem) {
        QStandardItem *item = bookmarkManager->treeBookmarkModel()->
            itemFromIndex(proxyModel->mapToSource(index));
        if (item) {
            item->setEditable(true);
            ui.treeView->edit(index);
            item->setEditable(false);
        }
    }
}

void BookmarkDialog::currentChanged(const QModelIndex& current,
                                    const QModelIndex& previous)
{
    Q_UNUSED(previous)

    if (!current.isValid()) {
        ui.bookmarkFolders->setCurrentIndex(
            ui.bookmarkFolders->findText(tr("Bookmarks")));
        return;
    }

    ui.bookmarkFolders->setCurrentIndex(
        ui.bookmarkFolders->findText(current.data().toString()));
}

bool BookmarkDialog::eventFilter(QObject *object, QEvent *e)
{
    if (object == this && e->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent*>(e);

        QModelIndex index = ui.treeView->currentIndex();
        switch (ke->key()) {
            case Qt::Key_F2: {
                const QModelIndex& source = proxyModel->mapToSource(index);
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

    QModelIndex index = treeView->indexAt(QPoint(1, 1));
    if (index.isValid())
        treeView->setCurrentIndex(index);

    if (searchBookmarks)
        expandItems();
}

void BookmarkWidget::expand(const QModelIndex& index)
{
    const QModelIndex& source = filterBookmarkModel->mapToSource(index);
    QStandardItem *item = bookmarkManager->treeBookmarkModel()->itemFromIndex(source);
    if (item)
        item->setData(treeView->isExpanded(index), Qt::UserRole + 11);
}

void BookmarkWidget::activated(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    QString data = index.data(Qt::UserRole + 10).toString();
    if (data != QLatin1String("Folder"))
        emit requestShowLink(data);
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
        showItemNewTab = menu.addAction(tr("Show Bookmark in New Tab"));
        if (searchField->text().isEmpty()) {
            menu.addSeparator();
            removeItem = menu.addAction(tr("Delete Bookmark"));
            renameItem = menu.addAction(tr("Rename Bookmark"));
        }
    }

    QAction *picked_action = menu.exec(treeView->mapToGlobal(point));
    if (!picked_action)
        return;

    if (picked_action == showItem) {
        emit requestShowLink(data);
    }
    else if (picked_action == showItemNewTab) {
        CentralWidget::instance()->setSourceInNewTab(data);
    }
    else if (picked_action == removeItem) {
        bookmarkManager->removeBookmarkItem(treeView,
            filterBookmarkModel->mapToSource(index));
    }
    else if (picked_action == renameItem) {
        const QModelIndex& source = filterBookmarkModel->mapToSource(index);
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
    vlayout->setMargin(4);

    QLabel *label = new QLabel(tr("Filter:"), this);
    vlayout->addWidget(label);

    searchField = new QLineEdit(this);
    vlayout->addWidget(searchField);
    connect(searchField, SIGNAL(textChanged(const QString &)), this,
        SLOT(filterChanged()));

    treeView = new TreeView(this);
    vlayout->addWidget(treeView);

    QString system = QLatin1String("win");
#ifdef Q_OS_MAC
    system = QLatin1String("mac");
#endif

    if (showButtons) {
        QLayout *hlayout = new QHBoxLayout();
        vlayout->addItem(hlayout);

        hlayout->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding));

        addButton = new QToolButton(this);
        addButton->setText(tr("Add"));
        addButton->setIcon(QIcon(QString::fromUtf8(
            ":/trolltech/assistant/images/%1/addtab.png").arg(system)));
        addButton->setAutoRaise(true);
        addButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        hlayout->addWidget(addButton);
        connect(addButton, SIGNAL(clicked()), this, SIGNAL(addBookmark()));

        removeButton = new QToolButton(this);
        removeButton->setText(tr("Remove"));
        removeButton->setIcon(QIcon(QString::fromUtf8(
            ":/trolltech/assistant/images/%1/closetab.png").arg(system)));
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

    connect(treeView, SIGNAL(expanded(const QModelIndex&)), this,
        SLOT(expand(const QModelIndex&)));

    connect(treeView, SIGNAL(collapsed(const QModelIndex&)), this,
        SLOT(expand(const QModelIndex&)));

    connect(treeView, SIGNAL(activated(const QModelIndex&)), this,
        SLOT(activated(const QModelIndex&)));

    connect(treeView, SIGNAL(customContextMenuRequested(const QPoint&)),
        this, SLOT(customContextMenuRequested(const QPoint&)));

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

void BookmarkWidget::focusInEvent(QFocusEvent *e)
{
    if (e->reason() != Qt::MouseFocusReason) {
        searchField->selectAll();
        searchField->setFocus();

        QModelIndex index = treeView->indexAt(QPoint(1, 1));
        if (index.isValid())
            treeView->setCurrentIndex(index);

    }
}

bool BookmarkWidget::eventFilter(QObject *object, QEvent *e)
{
    if (object == this && e->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent*>(e);
        QModelIndex index = treeView->currentIndex();
        switch (ke->key()) {
            if (index.isValid() && searchField->text().isEmpty()) {
                case Qt::Key_F2: {
                    const QModelIndex& source = filterBookmarkModel->mapToSource(index);
                    QStandardItem *item =
                        bookmarkManager->treeBookmarkModel()->itemFromIndex(source);
                    if (item) {
                        item->setEditable(true);
                        treeView->edit(index);
                        item->setEditable(false);
                    }
                }   break;

                case Qt::Key_Delete: {
                    bookmarkManager->removeBookmarkItem(treeView,
                        filterBookmarkModel->mapToSource(index));
                }   break;
            }

            case Qt::Key_Up:
            case Qt::Key_Down:
                treeView->subclassKeyPressEvent(ke);
                break;

            case Qt::Key_Enter: {
            case Qt::Key_Return:
                index = treeView->selectionModel()->currentIndex();
                if (index.isValid()) {
                    QString data = index.data(Qt::UserRole + 10).toString();
                    if (!data.isEmpty() && data != QLatin1String("Folder"))
                        emit requestShowLink(data);
                }
            }   break;

            case Qt::Key_Escape:
                emit escapePressed();
                break;

            default:
                break;
        }
    }
    else if (object == treeView->viewport() && e->type() == QEvent::MouseButtonRelease) {
        const QModelIndex& index = treeView->currentIndex();
        QMouseEvent *me = static_cast<QMouseEvent*>(e);
        if (index.isValid() && (me->button() == Qt::MidButton)) {
            QString data = index.data(Qt::UserRole + 10).toString();
            if (!data.isEmpty() && data != QLatin1String("Folder"))
                CentralWidget::instance()->setSourceInNewTab(data);
        }
    }
    return QWidget::eventFilter(object, e);
}




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
    if (index.data(Qt::UserRole + 10).toString() == QLatin1String("Folder"))
        return (Qt::ItemIsDropEnabled | defaultFlags) &~ Qt::ItemIsDragEnabled;

    return (Qt::ItemIsDragEnabled | defaultFlags) &~ Qt::ItemIsDropEnabled;
}




BookmarkManager::BookmarkManager(QHelpEngineCore* _helpEngine)
    : treeModel(new BookmarkModel(0, 1, this))
    , listModel(new BookmarkModel(0, 1, this))
    , helpEngine(_helpEngine)
{
    folderIcon = QApplication::style()->standardIcon(QStyle::SP_DirClosedIcon);
    treeModel->setHeaderData(0, Qt::Horizontal, QObject::tr("Bookmark"));
    listModel->setHeaderData(0, Qt::Horizontal, QObject::tr("Bookmark"));

    connect(treeModel, SIGNAL(itemChanged(QStandardItem*)), this,
        SLOT(itemChanged(QStandardItem*)));
}

BookmarkManager::~BookmarkManager()
{
    treeModel->clear();
    listModel->clear();
}

BookmarkModel* BookmarkManager::treeBookmarkModel()
{
    return treeModel;
}

BookmarkModel* BookmarkManager::listBookmarkModel()
{
    return listModel;
}

void BookmarkManager::saveBookmarks()
{
    qint32 depth = 0;
    QByteArray bookmarks;
    QDataStream stream(&bookmarks, QIODevice::WriteOnly);
    QStandardItem *root = treeModel->invisibleRootItem();

    for (int i = 0; i < root->rowCount(); ++i) {
        const QStandardItem *item = root->child(i);
        stream << depth; // root
        stream << item->data(Qt::DisplayRole).toString();
        stream << item->data(Qt::UserRole + 10).toString();
        stream << item->data(Qt::UserRole + 11).toBool();

        if (item->rowCount() > 0) {
            readBookmarksRecursive(item, stream, (depth +1));
        }
    }
    helpEngine->setCustomValue(QLatin1String("Bookmarks"), bookmarks);
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
    item->setData(false, Qt::UserRole + 11);
    item->setData(QLatin1String("Folder"), Qt::UserRole + 10);
    item->setIcon(QApplication::style()->standardIcon(QStyle::SP_DirClosedIcon));

    if (index.isValid()) {
        treeModel->itemFromIndex(index)->appendRow(item);
    } else {
        treeModel->appendRow(item);
    }
    return treeModel->indexFromItem(item);
}

void BookmarkManager::removeBookmarkItem(QTreeView *treeView, const QModelIndex& index)
{
    QStandardItem *item = treeModel->itemFromIndex(index);
    if (item) {
        QString data = index.data(Qt::UserRole + 10).toString();
        if (data == QLatin1String("Folder") && item->rowCount() > 0) {
            int value = QMessageBox::question(treeView, tr("Remove"),
                        tr("You are going to delete a Folder, this will also<br>"
                        "remove it's content. Are you sure to continue?"),
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
    item->setData(false, Qt::UserRole + 11);
    item->setData(url, Qt::UserRole + 10);

    if (index.isValid()) {
        treeModel->itemFromIndex(index)->appendRow(item);
        listModel->appendRow(item->clone());
    } else {
        treeModel->appendRow(item);
        listModel->appendRow(item->clone());
    }
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

    QByteArray ba = helpEngine->customValue(QLatin1String("Bookmarks")).toByteArray();
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
            if (type == QLatin1String("Folder")) {
                parents << item; lastDepths << depth;
            }
        }

        if (type == QLatin1String("Folder"))
            item->setIcon(folderIcon);
        else
            listModel->appendRow(item->clone());
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

        for (int i = 1; i <= names.count(); ++i) {
            folderName = (tr("New Folder") + QLatin1String(" %1")).arg(i);
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
                                             QDataStream &stream,
                                             const qint32 depth) const
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

QT_END_NAMESPACE
