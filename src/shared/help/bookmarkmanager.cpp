// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "bookmarkmanager.h"

#include <localhelpmanager.h>

#include <coreplugin/icore.h>

#include <help/helptr.h>

#include <utils/fancylineedit.h>
#include <utils/layoutbuilder.h>
#include <utils/styledbar.h>
#include <utils/utilsicons.h>

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
#include <QRegularExpression>

#include <QHelpEngine>

using namespace Help::Internal;

static const char kBookmarksKey[] = "Help/Bookmarks";

BookmarkDialog::BookmarkDialog(BookmarkManager *manager, const QString &title,
        const QString &url, QWidget *parent)
    : QDialog(parent)
    , m_url(url)
    , m_title(title)
    , bookmarkManager(manager)
{
    installEventFilter(this);
    resize(450, 0);
    setWindowTitle(::Help::Tr::tr("Add Bookmark"));

    proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setFilterKeyColumn(0);
    proxyModel->setDynamicSortFilter(true);
    proxyModel->setFilterRole(Qt::UserRole + 10);
    proxyModel->setSourceModel(bookmarkManager->treeBookmarkModel());
    proxyModel->setFilterRegularExpression(QRegularExpression(QLatin1String("Folder")));

    m_bookmarkEdit = new QLineEdit(title);
    m_bookmarkFolders = new QComboBox;
    m_bookmarkFolders->addItems(bookmarkManager->bookmarkFolders());

    m_toolButton = new QToolButton;
    m_toolButton->setFixedSize(24, 24);

    m_treeView = new QTreeView;
    m_treeView->setModel(proxyModel);
    m_treeView->expandAll();
    m_treeView->header()->setVisible(false);
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    QSizePolicy treeViewSP(QSizePolicy::Preferred, QSizePolicy::Ignored);
    treeViewSP.setVerticalStretch(1);
    m_treeView->setSizePolicy(treeViewSP);

    m_newFolderButton = new QPushButton(::Help::Tr::tr("New Folder"));
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    using namespace Layouting;
    Column {
        Form {
            ::Help::Tr::tr("Bookmark:"), m_bookmarkEdit, br,
            ::Help::Tr::tr("Add in folder:"), m_bookmarkFolders, br,
        },
        Row { m_toolButton, hr },
        m_treeView,
        Row { m_newFolderButton, m_buttonBox, }
    }.attachTo(this);

    toggleExpanded();

    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &BookmarkDialog::reject);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &BookmarkDialog::addAccepted);
    connect(m_newFolderButton, &QPushButton::clicked, this, &BookmarkDialog::addNewFolder);
    connect(m_toolButton, &QToolButton::clicked, this, &BookmarkDialog::toggleExpanded);
    connect(m_bookmarkEdit, &QLineEdit::textChanged, this, &BookmarkDialog::textChanged);

    connect(bookmarkManager->treeBookmarkModel(),
            &QStandardItemModel::itemChanged,
            this, &BookmarkDialog::itemChanged);

    connect(m_bookmarkFolders, &QComboBox::currentIndexChanged,
            this, &BookmarkDialog::selectBookmarkFolder);

    connect(m_treeView, &TreeView::customContextMenuRequested,
            this, &BookmarkDialog::showContextMenu);

    connect(m_treeView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &BookmarkDialog::currentChanged);
}

BookmarkDialog::~BookmarkDialog()
{
}

void BookmarkDialog::addAccepted()
{
    QItemSelectionModel *model = m_treeView->selectionModel();
    const QModelIndexList &list = model->selection().indexes();

    QModelIndex index;
    if (!list.isEmpty())
        index = proxyModel->mapToSource(list.at(0));

    bookmarkManager->addNewBookmark(index, m_bookmarkEdit->text(), m_url);
    accept();
}

void BookmarkDialog::addNewFolder()
{
    QItemSelectionModel *model = m_treeView->selectionModel();
    const QModelIndexList &list = model->selection().indexes();

    QModelIndex index;
    if (!list.isEmpty())
        index = list.at(0);

    QModelIndex newFolder =
        bookmarkManager->addNewFolder(proxyModel->mapToSource(index));
    if (newFolder.isValid()) {
        m_treeView->expand(index);
        const QModelIndex &index = proxyModel->mapFromSource(newFolder);
        model->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect);

        m_bookmarkFolders->clear();
        m_bookmarkFolders->addItems(bookmarkManager->bookmarkFolders());

        const QString &name = index.data().toString();
        m_bookmarkFolders->setCurrentIndex(m_bookmarkFolders->findText(name));
    }
    m_treeView->setFocus();
}

void BookmarkDialog::toggleExpanded()
{
    const char expand[] = "+";
    const char collapse[] = "-";
    const bool doCollapse = m_toolButton->text() != expand;
    m_toolButton->setText(doCollapse ? expand : collapse);
    m_treeView->setVisible(!doCollapse);
    m_newFolderButton->setVisible(!doCollapse);
    for (int i = 0; i <= 1; ++i) // Hack: resize twice to avoid "jumping" of m_toolButton
        resize(width(), (doCollapse ? 1 : 400));
}

void BookmarkDialog::itemChanged(QStandardItem *item)
{
    if (renameItem != item) {
        renameItem = item;
        oldText = item->text();
        return;
    }

    if (item->text() != oldText) {
        m_bookmarkFolders->clear();
        m_bookmarkFolders->addItems(bookmarkManager->bookmarkFolders());

        QString name = ::Help::Tr::tr("Bookmarks");
        const QModelIndex& index = m_treeView->currentIndex();
        if (index.isValid())
            name = index.data().toString();
        m_bookmarkFolders->setCurrentIndex(m_bookmarkFolders->findText(name));
    }
}

void BookmarkDialog::textChanged(const QString& string)
{
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!string.isEmpty());
}

void BookmarkDialog::selectBookmarkFolder(int index)
{
    const QString folderName = m_bookmarkFolders->itemText(index);
    if (folderName == ::Help::Tr::tr("Bookmarks")) {
        m_treeView->clearSelection();
        return;
    }

    QStandardItemModel *model = bookmarkManager->treeBookmarkModel();
    QList<QStandardItem*> list = model->findItems(folderName,
        Qt::MatchCaseSensitive | Qt::MatchRecursive, 0);
    if (!list.isEmpty()) {
        const QModelIndex &index = model->indexFromItem(list.at(0));
        QItemSelectionModel *model = m_treeView->selectionModel();
        if (model) {
            model->setCurrentIndex(proxyModel->mapFromSource(index),
                QItemSelectionModel::ClearAndSelect);
        }
    }
}

void BookmarkDialog::showContextMenu(const QPoint &point)
{
    QModelIndex index = m_treeView->indexAt(point);
    if (!index.isValid())
        return;

    QMenu menu(this);

    QAction *removeItem = menu.addAction(::Help::Tr::tr("Delete Folder"));
    QAction *renameItem = menu.addAction(::Help::Tr::tr("Rename Folder"));

    QAction *picked = menu.exec(m_treeView->mapToGlobal(point));
    if (!picked)
        return;

    const QModelIndex &proxyIndex = proxyModel->mapToSource(index);
    if (picked == removeItem) {
        bookmarkManager->removeBookmarkItem(m_treeView, proxyIndex);
        m_bookmarkFolders->clear();
        m_bookmarkFolders->addItems(bookmarkManager->bookmarkFolders());

        QString name = ::Help::Tr::tr("Bookmarks");
        index = m_treeView->currentIndex();
        if (index.isValid())
            name = index.data().toString();
        m_bookmarkFolders->setCurrentIndex(m_bookmarkFolders->findText(name));
    } else if (picked == renameItem) {
        BookmarkModel *model = bookmarkManager->treeBookmarkModel();
        if (QStandardItem *item = model->itemFromIndex(proxyIndex)) {
            item->setEditable(true);
            m_treeView->edit(index);
            item->setEditable(false);
        }
    }
}

void BookmarkDialog::currentChanged(const QModelIndex &current)
{
    QString text = ::Help::Tr::tr("Bookmarks");
    if (current.isValid())
        text = current.data().toString();
    m_bookmarkFolders->setCurrentIndex(m_bookmarkFolders->findText(text));
}

bool BookmarkDialog::eventFilter(QObject *object, QEvent *e)
{
    if (object == this && e->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent*>(e);

        QModelIndex index = m_treeView->currentIndex();
        switch (ke->key()) {
            case Qt::Key_F2: {
                const QModelIndex &source = proxyModel->mapToSource(index);
                QStandardItem *item =
                    bookmarkManager->treeBookmarkModel()->itemFromIndex(source);
                if (item) {
                    item->setEditable(true);
                    m_treeView->edit(index);
                    item->setEditable(false);
                }
            }   break;

            case Qt::Key_Backspace:
            case Qt::Key_Delete: {
                bookmarkManager->removeBookmarkItem(m_treeView,
                    proxyModel->mapToSource(index));
                m_bookmarkFolders->clear();
                m_bookmarkFolders->addItems(bookmarkManager->bookmarkFolders());

                QString name = ::Help::Tr::tr("Bookmarks");
                index = m_treeView->currentIndex();
                if (index.isValid())
                    name = index.data().toString();
                m_bookmarkFolders->setCurrentIndex(m_bookmarkFolders->findText(name));
            }   break;

            default:
                break;
        }
    }
    return QObject::eventFilter(object, e);
}


// #pragma mark -- BookmarkWidget


BookmarkWidget::BookmarkWidget(BookmarkManager *manager, QWidget *parent)
    : QWidget(parent)
    , bookmarkManager(manager)
    , m_isOpenInNewPageActionVisible(true)
{
    setup();
    installEventFilter(this);
}

BookmarkWidget::~BookmarkWidget()
{
}

void BookmarkWidget::setOpenInNewPageActionVisible(bool visible)
{
    m_isOpenInNewPageActionVisible = visible;
}

void BookmarkWidget::filterChanged()
{
    bool searchBookmarks = searchField->text().isEmpty();
    if (!searchBookmarks) {
        regExp.setPattern(QRegularExpression::escape(searchField->text()));
        filterBookmarkModel->setSourceModel(bookmarkManager->listBookmarkModel());
    } else {
        regExp.setPattern(QString());
        filterBookmarkModel->setSourceModel(bookmarkManager->treeBookmarkModel());
    }

    filterBookmarkModel->setFilterRegularExpression(regExp);

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

void BookmarkWidget::showContextMenu(const QPoint &point)
{
    QModelIndex index = treeView->indexAt(point);
    if (!index.isValid())
        return;

    QAction *showItem = 0;
    QAction *removeItem = 0;
    QAction *renameItem = 0;
    QAction *showItemNewTab = 0;

    QMenu menu(this);
    QString data = index.data(Qt::UserRole + 10).toString();
    if (data == QLatin1String("Folder")) {
        removeItem = menu.addAction(::Help::Tr::tr("Delete Folder"));
        renameItem = menu.addAction(::Help::Tr::tr("Rename Folder"));
    } else {
        showItem = menu.addAction(::Help::Tr::tr("Show Bookmark"));
        if (m_isOpenInNewPageActionVisible)
            showItemNewTab = menu.addAction(::Help::Tr::tr("Show Bookmark as New Page"));
        if (searchField->text().isEmpty()) {
            menu.addSeparator();
            removeItem = menu.addAction(::Help::Tr::tr("Delete Bookmark"));
            renameItem = menu.addAction(::Help::Tr::tr("Rename Bookmark"));
        }
    }

    QAction *pickedAction = menu.exec(treeView->mapToGlobal(point));
    if (!pickedAction)
        return;

    if (pickedAction == showItem) {
        emit linkActivated(data);
    } else if (pickedAction == showItemNewTab) {
        emit createPage(QUrl(data), false);
    } else if (pickedAction == removeItem) {
        bookmarkManager->removeBookmarkItem(treeView,
            filterBookmarkModel->mapToSource(index));
    } else if (pickedAction == renameItem) {
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

void BookmarkWidget::setup()
{
    regExp.setPatternOptions(QRegularExpression::CaseInsensitiveOption);

    QLayout *vlayout = new QVBoxLayout(this);
    vlayout->setContentsMargins(0, 0, 0, 0);
    vlayout->setSpacing(0);

    searchField = new Utils::FancyLineEdit(this);
    searchField->setFiltering(true);
    setFocusProxy(searchField);

    Utils::StyledBar *toolbar = new Utils::StyledBar(this);
    toolbar->setSingleRow(false);
    QLayout *tbLayout = new QHBoxLayout();
    tbLayout->setContentsMargins(4, 4, 4, 4);
    tbLayout->addWidget(searchField);
    toolbar->setLayout(tbLayout);

    vlayout->addWidget(toolbar);

    searchField->installEventFilter(this);
    connect(searchField, &Utils::FancyLineEdit::textChanged,
            this, &BookmarkWidget::filterChanged);

    treeView = new TreeView(this);
    vlayout->addWidget(treeView);

    filterBookmarkModel = new QSortFilterProxyModel(this);
    treeView->setModel(filterBookmarkModel);

    treeView->setDragEnabled(true);
    treeView->setAcceptDrops(true);
    treeView->setAutoExpandDelay(1000);
    treeView->setDropIndicatorShown(true);
    treeView->viewport()->installEventFilter(this);
    treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    // work around crash on Windows with drag & drop
    // in combination with proxy model and ResizeToContents section resize mode
    treeView->header()->setSectionResizeMode(QHeaderView::Stretch);


    connect(treeView, &TreeView::expanded, this, &BookmarkWidget::expand);
    connect(treeView, &TreeView::collapsed, this, &BookmarkWidget::expand);
    connect(treeView, &TreeView::activated, this, &BookmarkWidget::activated);
    connect(treeView, &TreeView::customContextMenuRequested,
            this, &BookmarkWidget::showContextMenu);

    filterBookmarkModel->setFilterKeyColumn(0);
    filterBookmarkModel->setDynamicSortFilter(true);
    filterBookmarkModel->setSourceModel(bookmarkManager->treeBookmarkModel());

    expandItems();
}

void BookmarkWidget::expandItems()
{
    QStandardItemModel *model = bookmarkManager->treeBookmarkModel();
    const QList<QStandardItem *> list = model->findItems(QLatin1String("*"),
                                                         Qt::MatchWildcard | Qt::MatchRecursive,
                                                         0);
    for (const QStandardItem *item : list) {
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
                } else if (ke->key() == Qt::Key_Delete || ke->key() == Qt::Key_Backspace) {
                    bookmarkManager->removeBookmarkItem(treeView, src);
                }
            }

            switch (ke->key()) {
                default: break;
                case Qt::Key_Up:
                case Qt::Key_Down: {
                    treeView->subclassKeyPressEvent(ke);
                }   break;

                case Qt::Key_Enter:
                case Qt::Key_Return: {
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
                    || (me->button() == Qt::MiddleButton)) {
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
    , m_bookmarkIcon(Utils::Icons::BOOKMARK.icon())
    , treeModel(new BookmarkModel(0, 1, this))
    , listModel(new BookmarkModel(0, 1, this))
{
    connect(treeModel, &BookmarkModel::itemChanged,
            this, &BookmarkManager::itemChanged);
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
    if (!m_isModelSetup)
        return;
    QByteArray bookmarks;
    QDataStream stream(&bookmarks, QIODevice::WriteOnly);

    readBookmarksRecursive(treeModel->invisibleRootItem(), stream, 0);
    Core::ICore::settings()->setValue(kBookmarksKey, bookmarks);
}

QStringList BookmarkManager::bookmarkFolders() const
{
    QStringList folders(::Help::Tr::tr("Bookmarks"));

    const QList<QStandardItem *> list = treeModel->findItems(QLatin1String("*"),
                                                             Qt::MatchWildcard | Qt::MatchRecursive,
                                                             0);

    QString data;
    for (const QStandardItem *item : list) {
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
            int value = QMessageBox::question(treeView, ::Help::Tr::tr("Remove"),
                ::Help::Tr::tr("Deleting a folder also removes its content.<br>"
                               "Do you want to continue?"),
                QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);

            if (value == QMessageBox::Cancel)
                return;
        }

        if (data != QLatin1String("Folder")) {
            const QList<QStandardItem *> itemList = listModel->findItems(item->text());
            for (const QStandardItem *i : itemList) {
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
            if (!itemList.isEmpty())
                itemList.at(0)->setText(item->text());
        }
    }
}

void BookmarkManager::setupBookmarkModels()
{
    m_isModelSetup = true;
    treeModel->clear();
    listModel->clear();

    qint32 depth;
    bool expanded;
    QString name, type;
    QList<int> lastDepths;
    QList<QStandardItem*> parents;

    QByteArray ba;
    Utils::QtcSettings *settings = Core::ICore::settings();
    ba = settings->value(kBookmarksKey).toByteArray();
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
    QString folderName = ::Help::Tr::tr("New Folder");
    const QList<QStandardItem *> list = treeModel->findItems(folderName,
                                                             Qt::MatchContains | Qt::MatchRecursive,
                                                             0);
    if (!list.isEmpty()) {
        QStringList names;
        for (const QStandardItem *item : list)
            names << item->text();

        QString folderNameBase = ::Help::Tr::tr("New Folder") + QLatin1String(" %1");
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
        const QList<QStandardItem*> itemList = listModel->findItems(child->text());
        for (const QStandardItem *i : itemList) {
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
