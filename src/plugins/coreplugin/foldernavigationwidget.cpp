// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "foldernavigationwidget.h"

#include "actionmanager/actionmanager.h"
#include "actionmanager/command.h"
#include "coreicons.h"
#include "coreplugintr.h"
#include "documentmanager.h"
#include "editormanager/editormanager.h"
#include "editormanager/ieditor.h"
#include "fileutils.h"
#include "icontext.h"
#include "icore.h"
#include "idocument.h"
#include "iwizardfactory.h"

#include <extensionsystem/pluginmanager.h>

#include <utils/algorithm.h>
#include <utils/filecrumblabel.h>
#include <utils/filepath.h>
#include <utils/fsengine/fileiconprovider.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/navigationtreeview.h>
#include <utils/qtcassert.h>
#include <utils/removefiledialog.h>
#include <utils/store.h>
#include <utils/stringutils.h>
#include <utils/styledbar.h>
#include <utils/stylehelper.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QDir>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QScrollBar>
#include <QSize>
#include <QSortFilterProxyModel>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

using namespace Utils;

const int PATH_ROLE = Qt::UserRole;
const int ID_ROLE = Qt::UserRole + 1;
const int SORT_ROLE = Qt::UserRole + 2;

const char PROJECTSDIRECTORYROOT_ID[] = "A.Projects";
const char C_FOLDERNAVIGATIONWIDGET[] = "ProjectExplorer.FolderNavigationWidget";

const char kSettingsBase[] = "FolderNavigationWidget.";
const char kHiddenFilesKey[] = ".HiddenFilesFilter";
const char kSyncKey[] = ".SyncWithEditor";
const char kShowBreadCrumbs[] = ".ShowBreadCrumbs";
const char kSyncRootWithEditor[] = ".SyncRootWithEditor";
const char kShowFoldersOnTop[] = ".ShowFoldersOnTop";

const char ADDNEWFILE[] = "QtCreator.FileSystem.AddNewFile";
const char RENAMEFILE[] = "QtCreator.FileSystem.RenameFile";
const char REMOVEFILE[] = "QtCreator.FileSystem.RemoveFile";

namespace Core {

static FolderNavigationWidgetFactory *m_instance = nullptr;

QVector<FolderNavigationWidgetFactory::RootDirectory>
    FolderNavigationWidgetFactory::m_rootDirectories;

Utils::FilePath FolderNavigationWidgetFactory::m_fallbackSyncFilePath;

FolderNavigationWidgetFactory *FolderNavigationWidgetFactory::instance()
{
    return m_instance;
}

namespace Internal {

// Call delayLayoutOnce to delay reporting the new heightForWidget by the double-click interval.
// Call setScrollBarOnce to set a scroll bar's value once during layouting (where heightForWidget
// is called).
class DelayedFileCrumbLabel : public Utils::FileCrumbLabel
{
public:
    DelayedFileCrumbLabel(QWidget *parent) : Utils::FileCrumbLabel(parent) {}

    int immediateHeightForWidth(int w) const;
    int heightForWidth(int w) const final;
    void delayLayoutOnce();
    void setScrollBarOnce(QScrollBar *bar, int value);

private:
    void setScrollBarOnce() const;

    QPointer<QScrollBar> m_bar;
    int m_barValue = 0;
    bool m_delaying = false;
};

// FolderNavigationModel: Shows path as tooltip.
class FolderNavigationModel : public QFileSystemModel
{
public:
    enum Roles {
        IsFolderRole = Qt::UserRole + 50 // leave some gap for the custom roles in QFileSystemModel
    };

    explicit FolderNavigationModel(QObject *parent = nullptr);
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const final;
    Qt::DropActions supportedDragActions() const final;
    Qt::ItemFlags flags(const QModelIndex &index) const final;
    bool setData(const QModelIndex &index, const QVariant &value, int role) final;
};

// Sorts folders on top if wanted
class FolderSortProxyModel : public QSortFilterProxyModel
{
public:
    FolderSortProxyModel(QObject *parent = nullptr);

protected:
    bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const override;
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
};

bool FolderSortProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    if (static_cast<QFileSystemModel *>(sourceModel())->rootPath().isEmpty()) {
        QModelIndex sourceIndex = sourceModel()->index(source_row, 0, source_parent);
        while (sourceIndex.isValid()) {
            if (sourceIndex.data().toString() == FilePath::specialRootName())
                return false;

            sourceIndex = sourceIndex.parent();
        }
    }

    return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
}

FolderSortProxyModel::FolderSortProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
}

bool FolderSortProxyModel::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const
{
    const QAbstractItemModel *src = sourceModel();
    if (sortRole() == FolderNavigationModel::IsFolderRole) {
        const bool leftIsFolder = src->data(source_left, FolderNavigationModel::IsFolderRole)
                                      .toBool();
        const bool rightIsFolder = src->data(source_right, FolderNavigationModel::IsFolderRole)
                                       .toBool();
        if (leftIsFolder != rightIsFolder)
            return leftIsFolder;
    }
    const QString leftName = src->data(source_left, QFileSystemModel::FileNameRole).toString();
    const QString rightName = src->data(source_right, QFileSystemModel::FileNameRole).toString();
    return Utils::FilePath::fromString(leftName) < Utils::FilePath::fromString(rightName);
}

FolderNavigationModel::FolderNavigationModel(QObject *parent) : QFileSystemModel(parent)
{ }

QVariant FolderNavigationModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::ToolTipRole)
        return QDir::toNativeSeparators(QDir::cleanPath(filePath(index)));
    else if (role == IsFolderRole)
        return isDir(index);
    else
        return QFileSystemModel::data(index, role);
}

Qt::DropActions FolderNavigationModel::supportedDragActions() const
{
    return Qt::MoveAction;
}

Qt::ItemFlags FolderNavigationModel::flags(const QModelIndex &index) const
{
    if (index.isValid() && !fileInfo(index).isRoot())
        return QFileSystemModel::flags(index) | Qt::ItemIsEditable;
    return QFileSystemModel::flags(index);
}

bool FolderNavigationModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    QTC_ASSERT(index.isValid() && parent(index).isValid() && index.column() == 0
                   && role == Qt::EditRole && value.canConvert<QString>(),
               return false);
    const QString afterFileName = value.toString();
    const Utils::FilePath beforeFilePath = Utils::FilePath::fromString(filePath(index));
    const Utils::FilePath parentPath = Utils::FilePath::fromString(filePath(parent(index)));
    const Utils::FilePath afterFilePath = parentPath.pathAppended(afterFileName);
    if (beforeFilePath == afterFilePath)
        return false;
    // need to rename through file system model, which takes care of not changing our selection
    const bool success = QFileSystemModel::setData(index, value, role);
    // for files we can do more than just rename on disk, for directories the user is on his/her own
    if (success && fileInfo(index).isFile()) {
        Core::DocumentManager::renamedFile(beforeFilePath, afterFilePath);
        emit m_instance->fileRenamed(beforeFilePath, afterFilePath);
    }
    return success;
}

static void showOnlyFirstColumn(QTreeView *view)
{
    const int columnCount = view->header()->count();
    for (int i = 1; i < columnCount; ++i)
        view->setColumnHidden(i, true);
}

static bool isChildOf(const QModelIndex &index, const QModelIndex &parent)
{
    if (index == parent)
        return true;
    QModelIndex current = index;
    while (current.isValid()) {
        current = current.parent();
        if (current == parent)
            return true;
    }
    return false;
}

} // namespace Internal

using namespace Internal;

/*!
    \class FolderNavigationWidget

    Shows a file system tree, with the root directory selectable from a dropdown.

    \internal
*/
FolderNavigationWidget::FolderNavigationWidget(QWidget *parent) : QWidget(parent),
    m_listView(new Utils::NavigationTreeView(this)),
    m_fileSystemModel(new FolderNavigationModel(this)),
    m_sortProxyModel(new FolderSortProxyModel(m_fileSystemModel)),
    m_filterHiddenFilesAction(new QAction(Tr::tr("Show Hidden Files"), this)),
    m_showBreadCrumbsAction(new QAction(Tr::tr("Show Bread Crumbs"), this)),
    m_showFoldersOnTopAction(new QAction(Tr::tr("Show Folders on Top"), this)),
    m_toggleSync(new QToolButton(this)),
    m_toggleRootSync(new QToolButton(this)),
    m_rootSelector(new QComboBox),
    m_crumbContainer(new QWidget(this)),
    m_crumbLabel(new DelayedFileCrumbLabel(this))
{
    auto context = new Core::IContext(this);
    context->setContext(Core::Context(C_FOLDERNAVIGATIONWIDGET));
    context->setWidget(this);
    Core::ICore::addContextObject(context);

    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
    m_sortProxyModel->setSourceModel(m_fileSystemModel);
    m_sortProxyModel->setSortRole(FolderNavigationModel::IsFolderRole);
    m_sortProxyModel->sort(0);

    m_fileSystemModel->setResolveSymlinks(false);
    m_fileSystemModel->setIconProvider(Utils::FileIconProvider::iconProvider());
    QDir::Filters filters = QDir::AllEntries | QDir::NoDotAndDotDot;
    if (Utils::HostOsInfo::isWindowsHost()) // Symlinked directories can cause file watcher warnings on Win32.
        filters |= QDir::NoSymLinks;
    m_fileSystemModel->setFilter(filters);
    m_fileSystemModel->setRootPath(QString());
    m_filterHiddenFilesAction->setCheckable(true);
    setHiddenFilesFilter(false);
    m_showBreadCrumbsAction->setCheckable(true);
    setShowBreadCrumbs(true);
    m_showFoldersOnTopAction->setCheckable(true);
    setShowFoldersOnTop(true);
    m_listView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_listView->setIconSize(QSize(16,16));
    m_listView->setModel(m_sortProxyModel);
    m_listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_listView->setDragEnabled(true);
    m_listView->setDragDropMode(QAbstractItemView::DragOnly);
    m_listView->viewport()->installEventFilter(this);
    showOnlyFirstColumn(m_listView);
    setFocusProxy(m_listView);

    auto selectorWidget = new Utils::StyledBar(this);
    selectorWidget->setLightColored(true);
    auto selectorLayout = new QHBoxLayout(selectorWidget);
    selectorWidget->setLayout(selectorLayout);
    selectorLayout->setSpacing(0);
    selectorLayout->setContentsMargins(0, 0, 0, 0);
    selectorLayout->addWidget(m_rootSelector, 10);

    auto crumbContainerLayout = new QVBoxLayout;
    crumbContainerLayout->setSpacing(0);
    crumbContainerLayout->setContentsMargins(0, 0, 0, 0);
    m_crumbContainer->setLayout(crumbContainerLayout);
    auto crumbLayout = new QVBoxLayout;
    crumbLayout->setSpacing(0);
    crumbLayout->setContentsMargins(4, 4, 4, 4);
    crumbLayout->addWidget(m_crumbLabel);
    crumbContainerLayout->addLayout(crumbLayout);
    crumbContainerLayout->addWidget(Layouting::createHr(this));
    m_crumbLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    auto layout = new QVBoxLayout();
    layout->addWidget(selectorWidget);
    layout->addWidget(m_crumbContainer);
    layout->addWidget(m_listView);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);

    m_toggleSync->setIcon(Utils::Icons::LINK_TOOLBAR.icon());
    m_toggleSync->setCheckable(true);
    m_toggleSync->setToolTip(Tr::tr("Synchronize with Editor"));

    m_toggleRootSync->setIcon(Utils::Icons::LINK.icon());
    m_toggleRootSync->setCheckable(true);
    m_toggleRootSync->setToolTip(Tr::tr("Synchronize Root Directory with Editor"));
    selectorLayout->addWidget(m_toggleRootSync);

    // connections
    connect(Core::EditorManager::instance(), &Core::EditorManager::currentEditorChanged,
            this, &FolderNavigationWidget::handleCurrentEditorChanged);
    connect(m_listView, &QAbstractItemView::activated, this, [this](const QModelIndex &index) {
        openItem(m_sortProxyModel->mapToSource(index));
    });
    // Delay updating crumble path by event loop cylce, because that can scroll, which doesn't
    // work well when done directly in currentChanged (the wrong item can get highlighted).
    // We cannot use Qt::QueuedConnection directly, because the QModelIndex could get invalidated
    // in the meantime, so use a queued invokeMethod instead.
    connect(m_listView->selectionModel(),
            &QItemSelectionModel::currentChanged,
            this,
            [this](const QModelIndex &index) {
                const QModelIndex sourceIndex = m_sortProxyModel->mapToSource(index);
                const auto filePath = Utils::FilePath::fromString(
                    m_fileSystemModel->filePath(sourceIndex));
                QMetaObject::invokeMethod(this, [this, filePath] { setCrumblePath(filePath); },
                                          Qt::QueuedConnection);
            });
    connect(m_crumbLabel, &Utils::FileCrumbLabel::pathClicked,
            this, [this](const Utils::FilePath &path) {
        const QModelIndex rootIndex = m_sortProxyModel->mapToSource(m_listView->rootIndex());
        const QModelIndex fileIndex = m_fileSystemModel->index(path.toString());
        if (!isChildOf(fileIndex, rootIndex))
            selectBestRootForFile(path);
        selectFile(path);
    });
    connect(m_filterHiddenFilesAction,
            &QAction::toggled,
            this,
            &FolderNavigationWidget::setHiddenFilesFilter);
    connect(m_showBreadCrumbsAction,
            &QAction::toggled,
            this,
            &FolderNavigationWidget::setShowBreadCrumbs);
    connect(m_showFoldersOnTopAction,
            &QAction::toggled,
            this,
            &FolderNavigationWidget::setShowFoldersOnTop);
    connect(m_toggleSync,
            &QAbstractButton::clicked,
            this,
            &FolderNavigationWidget::toggleAutoSynchronization);
    connect(m_toggleRootSync, &QAbstractButton::clicked,
            this, [this] { setRootAutoSynchronization(!m_rootAutoSync); });
    connect(m_rootSelector, &QComboBox::currentIndexChanged, this, [this](int index) {
                const auto directory = m_rootSelector->itemData(index).value<Utils::FilePath>();
                m_rootSelector->setToolTip(directory.toUserOutput());
                setRootDirectory(directory);
                const QModelIndex rootIndex = m_sortProxyModel->mapToSource(m_listView->rootIndex());
                const QModelIndex fileIndex = m_sortProxyModel->mapToSource(m_listView->currentIndex());
                if (!isChildOf(fileIndex, rootIndex))
                    selectFile(directory);
            });

    setAutoSynchronization(true);
    setRootAutoSynchronization(true);
}

void FolderNavigationWidget::toggleAutoSynchronization()
{
    setAutoSynchronization(!m_autoSync);
}

void FolderNavigationWidget::setShowBreadCrumbs(bool show)
{
    m_showBreadCrumbsAction->setChecked(show);
    m_crumbContainer->setVisible(show);
}

void FolderNavigationWidget::setShowFoldersOnTop(bool onTop)
{
    m_showFoldersOnTopAction->setChecked(onTop);
    m_sortProxyModel->setSortRole(onTop ? int(FolderNavigationModel::IsFolderRole)
                                        : int(QFileSystemModel::FileNameRole));
}

static bool itemLessThan(QComboBox *combo,
                         int index,
                         const FolderNavigationWidgetFactory::RootDirectory &directory)
{
    return combo->itemData(index, SORT_ROLE).toInt() < directory.sortValue
           || (combo->itemData(index, SORT_ROLE).toInt() == directory.sortValue
               && combo->itemData(index, Qt::DisplayRole).toString() < directory.displayName);
}

void FolderNavigationWidget::insertRootDirectory(
    const FolderNavigationWidgetFactory::RootDirectory &directory)
{
    // Find existing. Do not remove yet, to not mess up the current selection.
    int previousIndex = 0;
    while (previousIndex < m_rootSelector->count()
           && m_rootSelector->itemData(previousIndex, ID_ROLE).toString() != directory.id)
        ++previousIndex;
    // Insert sorted.
    int index = 0;
    while (index < m_rootSelector->count() && itemLessThan(m_rootSelector, index, directory))
        ++index;
    m_rootSelector->insertItem(index, directory.displayName);
    if (index <= previousIndex) // item was inserted, update previousIndex
        ++previousIndex;
    m_rootSelector->setItemData(index, QVariant::fromValue(directory.path), PATH_ROLE);
    m_rootSelector->setItemData(index, directory.id, ID_ROLE);
    m_rootSelector->setItemData(index, directory.sortValue, SORT_ROLE);
    m_rootSelector->setItemData(index, directory.path.toUserOutput(), Qt::ToolTipRole);
    m_rootSelector->setItemIcon(index, directory.icon);
    if (m_rootSelector->currentIndex() == previousIndex)
        m_rootSelector->setCurrentIndex(index);
    if (previousIndex < m_rootSelector->count())
        m_rootSelector->removeItem(previousIndex);
    if (Core::EditorManager::currentEditor()) {
        if (m_autoSync) // we might find a better root for current selection now
            handleCurrentEditorChanged(Core::EditorManager::currentEditor());
    } else if (m_rootAutoSync) {
        // assume the new root is better (e.g. because a project was opened)
        m_rootSelector->setCurrentIndex(index);
    }
}

void FolderNavigationWidget::removeRootDirectory(const QString &id)
{
    for (int i = 0; i < m_rootSelector->count(); ++i) {
        if (m_rootSelector->itemData(i, ID_ROLE).toString() == id) {
            m_rootSelector->removeItem(i);
            break;
        }
    }
    if (m_autoSync) // we might need to find a new root for current selection
        handleCurrentEditorChanged(Core::EditorManager::currentEditor());
}

void FolderNavigationWidget::addNewItem()
{
    const QModelIndex current = m_sortProxyModel->mapToSource(m_listView->currentIndex());
    if (!current.isValid())
        return;
    const auto filePath = Utils::FilePath::fromString(m_fileSystemModel->filePath(current));
    const Utils::FilePath path = filePath.isDir() ? filePath : filePath.parentDir();
    Core::ICore::showNewItemDialog(Tr::tr("New File", "Title of dialog"),
                                   Utils::filtered(Core::IWizardFactory::allWizardFactories(),
                                                   Utils::equal(&Core::IWizardFactory::kind,
                                                                Core::IWizardFactory::FileWizard)),
                                   path);
}

void FolderNavigationWidget::editCurrentItem()
{
    const QModelIndex current = m_listView->currentIndex();
    if (m_listView->model()->flags(current) & Qt::ItemIsEditable)
        m_listView->edit(current);
}

void FolderNavigationWidget::removeCurrentItem()
{
    const QModelIndex current = m_sortProxyModel->mapToSource(m_listView->currentIndex());
    if (!current.isValid() || m_fileSystemModel->isDir(current))
        return;
    const FilePath filePath = FilePath::fromString(m_fileSystemModel->filePath(current));
    RemoveFileDialog dialog(filePath, Core::ICore::dialogParent());
    dialog.setDeleteFileVisible(false);
    if (dialog.exec() == QDialog::Accepted) {
        emit m_instance->aboutToRemoveFile(filePath);
        Core::FileChangeBlocker changeGuard(filePath);
        Core::FileUtils::removeFiles({filePath}, true /*delete from disk*/);
    }
}

void FolderNavigationWidget::syncWithFilePath(const Utils::FilePath &filePath)
{
    if (filePath.isEmpty())
        return;
    if (m_rootAutoSync)
        selectBestRootForFile(filePath);
    selectFile(filePath);
}

bool FolderNavigationWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_listView->viewport()) {
        if (event->type() == QEvent::MouseButtonPress) {
            // select the current root when clicking outside any other item
            auto me = static_cast<QMouseEvent *>(event);
            const QModelIndex index = m_listView->indexAt(me->pos());
            if (!index.isValid())
                m_listView->setCurrentIndex(m_listView->rootIndex());
        }
    }
    return false;
}

bool FolderNavigationWidget::autoSynchronization() const
{
    return m_autoSync;
}

void FolderNavigationWidget::setAutoSynchronization(bool sync)
{
    m_toggleSync->setChecked(sync);
    m_toggleRootSync->setEnabled(sync);
    m_toggleRootSync->setChecked(sync ? m_rootAutoSync : false);
    if (sync == m_autoSync)
        return;
    m_autoSync = sync;
    if (m_autoSync)
        handleCurrentEditorChanged(Core::EditorManager::currentEditor());
}

void FolderNavigationWidget::setRootAutoSynchronization(bool sync)
{
    m_toggleRootSync->setChecked(sync);
    if (sync == m_rootAutoSync)
        return;
    m_rootAutoSync = sync;
    if (m_rootAutoSync)
        handleCurrentEditorChanged(Core::EditorManager::currentEditor());
}

void FolderNavigationWidget::handleCurrentEditorChanged(Core::IEditor *editor)
{
    if (!m_autoSync || !editor || editor->document()->filePath().isEmpty()
            || editor->document()->isTemporary())
        return;
    syncWithFilePath(editor->document()->filePath());
}

void FolderNavigationWidget::selectBestRootForFile(const Utils::FilePath &filePath)
{
    const int bestRootIndex = bestRootForFile(filePath);
    m_rootSelector->setCurrentIndex(bestRootIndex);
}

void FolderNavigationWidget::selectFile(const Utils::FilePath &filePath)
{
    const QModelIndex fileIndex = m_sortProxyModel->mapFromSource(
        m_fileSystemModel->index(filePath.toString()));
    if (fileIndex.isValid() || filePath.isEmpty() /* Computer root */) {
        // TODO This only scrolls to the right position if all directory contents are loaded.
        // Unfortunately listening to directoryLoaded was still not enough (there might also
        // be some delayed sorting involved?).
        // Use magic timer for scrolling.
        m_listView->setCurrentIndex(fileIndex);
        QTimer::singleShot(200, this, [this, filePath] {
            const QModelIndex fileIndex = m_sortProxyModel->mapFromSource(
                m_fileSystemModel->index(filePath.toString()));
            if (fileIndex == m_listView->rootIndex()) {
                m_listView->horizontalScrollBar()->setValue(0);
                m_listView->verticalScrollBar()->setValue(0);
            } else {
                m_listView->scrollTo(fileIndex);
            }
            setCrumblePath(filePath);
        });
    }
}

void FolderNavigationWidget::setRootDirectory(const Utils::FilePath &directory)
{
    const QModelIndex index = m_sortProxyModel->mapFromSource(
        m_fileSystemModel->setRootPath(directory.toString()));
    m_listView->setRootIndex(index);
}

int FolderNavigationWidget::bestRootForFile(const Utils::FilePath &filePath)
{
    int index = 0; // Computer is default
    int commonLength = 0;
    for (int i = 1; i < m_rootSelector->count(); ++i) {
        const auto root = m_rootSelector->itemData(i).value<Utils::FilePath>();
        if ((filePath == root || filePath.isChildOf(root))
                && root.toString().size() > commonLength) {
            index = i;
            commonLength = root.toString().size();
        }
    }
    return index;
}

void FolderNavigationWidget::openItem(const QModelIndex &index)
{
    QTC_ASSERT(index.isValid(), return);
    // signal "activate" is also sent when double-clicking folders
    // but we don't want to do anything in that case
    if (m_fileSystemModel->isDir(index))
        return;
    const QString path = m_fileSystemModel->filePath(index);
    Core::EditorManager::openEditor(FilePath::fromString(path),
                                    {},
                                    Core::EditorManager::AllowExternalEditor);
}

void FolderNavigationWidget::createNewFolder(const QModelIndex &parent)
{
    static const QString baseName = Tr::tr("New Folder");
    // find non-existing name
    const QDir dir(m_fileSystemModel->filePath(parent));
    const QSet<Utils::FilePath> existingItems
        = Utils::transform<QSet>(dir.entryList({baseName + '*'}, QDir::AllEntries),
                                 [](const QString &entry) {
                                     return Utils::FilePath::fromString(entry);
                                 });
    const Utils::FilePath name = Utils::makeUniquelyNumbered(Utils::FilePath::fromString(baseName),
                                                   existingItems);
    // create directory and edit
    const QModelIndex index = m_sortProxyModel->mapFromSource(
        m_fileSystemModel->mkdir(parent, name.toString()));
    if (!index.isValid())
        return;
    m_listView->setCurrentIndex(index);
    m_listView->edit(index);
}

void FolderNavigationWidget::setCrumblePath(const Utils::FilePath &filePath)
{
    const QModelIndex index = m_fileSystemModel->index(filePath.toString());
    const int width = m_crumbLabel->width();
    const int previousHeight = m_crumbLabel->immediateHeightForWidth(width);
    m_crumbLabel->setPath(filePath);
    const int currentHeight = m_crumbLabel->immediateHeightForWidth(width);
    const int diff = currentHeight - previousHeight;
    if (diff != 0 && m_crumbLabel->isVisible()) {
        // try to fix scroll position, otherwise delay layouting
        QScrollBar *bar = m_listView->verticalScrollBar();
        const int newBarValue = bar ? bar->value() + diff : 0;
        const QRect currentItemRect = m_listView->visualRect(index);
        const int currentItemVStart = currentItemRect.y();
        const int currentItemVEnd = currentItemVStart + currentItemRect.height();
        const bool currentItemStillVisibleAsBefore = (diff < 0 || currentItemVStart > diff
                                                      || currentItemVEnd <= 0);
        if (bar && bar->minimum() <= newBarValue && bar->maximum() >= newBarValue
                && currentItemStillVisibleAsBefore) {
            // we need to set the scroll bar when the layout request from the crumble path is
            // handled, otherwise it will flicker
            m_crumbLabel->setScrollBarOnce(bar, newBarValue);
        } else {
            m_crumbLabel->delayLayoutOnce();
        }
    }
}

void FolderNavigationWidget::contextMenuEvent(QContextMenuEvent *ev)
{
    QMenu menu;
    // Open current item
    const QModelIndex current = m_sortProxyModel->mapToSource(m_listView->currentIndex());
    const bool hasCurrentItem = current.isValid();
    QAction *actionOpenFile = nullptr;
    QAction *newFolder = nullptr;
    QAction *removeFolder = nullptr;
    const bool isDir = m_fileSystemModel->isDir(current);
    const Utils::FilePath filePath = hasCurrentItem ? Utils::FilePath::fromString(
                                                          m_fileSystemModel->filePath(current))
                                                    : Utils::FilePath();
    if (hasCurrentItem) {
        if (!isDir)
            actionOpenFile = menu.addAction(Tr::tr("Open \"%1\"").arg(filePath.toUserOutput()));
        emit m_instance->aboutToShowContextMenu(&menu, filePath, isDir);
    }

    // we need dummy DocumentModel::Entry with absolute file path in it
    // to get EditorManager::addNativeDirAndOpenWithActions() working
    Core::DocumentModel::Entry fakeEntry;
    Core::IDocument document;
    document.setFilePath(filePath);
    fakeEntry.document = &document;
    Core::EditorManager::addNativeDirAndOpenWithActions(&menu, &fakeEntry);

    if (hasCurrentItem) {
        menu.addAction(Core::ActionManager::command(ADDNEWFILE)->action());
        if (!isDir)
            menu.addAction(Core::ActionManager::command(REMOVEFILE)->action());
        if (m_fileSystemModel->flags(current) & Qt::ItemIsEditable)
            menu.addAction(Core::ActionManager::command(RENAMEFILE)->action());
        newFolder = menu.addAction(Tr::tr("New Folder"));
        if (isDir)
            removeFolder = menu.addAction(Tr::tr("Remove Folder"));
    }

    menu.addSeparator();
    QAction *const collapseAllAction = menu.addAction(Tr::tr("Collapse All"));

    QAction *action = menu.exec(ev->globalPos());
    if (!action)
        return;

    ev->accept();
    if (action == actionOpenFile) {
        openItem(current);
    } else if (action == newFolder) {
        if (isDir)
            createNewFolder(current);
        else
            createNewFolder(current.parent());
    } else if (action == removeFolder) {
        RemoveFileDialog dialog(filePath, Core::ICore::dialogParent());
        dialog.setDeleteFileVisible(false);
        if (dialog.exec() == QDialog::Accepted) {
            QString errorMessage;
            filePath.removeRecursively(&errorMessage);
            if (!errorMessage.isEmpty())
                QMessageBox::critical(ICore::dialogParent(), Tr::tr("Error"), errorMessage);
        }
    } else if (action == collapseAllAction) {
        m_listView->collapseAll();
    }
}

bool FolderNavigationWidget::rootAutoSynchronization() const
{
    return m_rootAutoSync;
}

void FolderNavigationWidget::setHiddenFilesFilter(bool filter)
{
    QDir::Filters filters = m_fileSystemModel->filter();
    if (filter)
        filters |= QDir::Hidden;
    else
        filters &= ~(QDir::Hidden);
    m_fileSystemModel->setFilter(filters);
    m_filterHiddenFilesAction->setChecked(filter);
}

bool FolderNavigationWidget::hiddenFilesFilter() const
{
    return m_filterHiddenFilesAction->isChecked();
}

bool FolderNavigationWidget::isShowingBreadCrumbs() const
{
    return m_showBreadCrumbsAction->isChecked();
}

bool FolderNavigationWidget::isShowingFoldersOnTop() const
{
    return m_showFoldersOnTopAction->isChecked();
}

// --------------------FolderNavigationWidgetFactory
FolderNavigationWidgetFactory::FolderNavigationWidgetFactory()
{
    m_instance = this;
    setDisplayName(Tr::tr("File System"));
    setPriority(400);
    setId("File System");
    setActivationSequence(QKeySequence(Core::useMacShortcuts ? Tr::tr("Meta+Y,Meta+F")
                                                             : Tr::tr("Alt+Y,Alt+F")));
    insertRootDirectory({QLatin1String("A.Computer"),
                         0 /*sortValue*/,
                         Tr::tr("Computer"),
                         Utils::FilePath(),
                         Core::Icons::DESKTOP_DEVICE_SMALL.icon()});
    insertRootDirectory({QLatin1String("A.Home"),
                         10 /*sortValue*/,
                         Tr::tr("Home"),
                         Utils::FilePath::fromString(QDir::homePath()),
                         Utils::Icons::HOME.icon()});
    updateProjectsDirectoryRoot();
    connect(Core::DocumentManager::instance(),
            &Core::DocumentManager::projectsDirectoryChanged,
            this,
            &FolderNavigationWidgetFactory::updateProjectsDirectoryRoot);
    registerActions();
}

Core::NavigationView FolderNavigationWidgetFactory::createWidget()
{
    auto fnw = new FolderNavigationWidget;
    for (const RootDirectory &root : std::as_const(m_rootDirectories))
        fnw->insertRootDirectory(root);
    connect(this,
            &FolderNavigationWidgetFactory::rootDirectoryAdded,
            fnw,
            &FolderNavigationWidget::insertRootDirectory);
    connect(this,
            &FolderNavigationWidgetFactory::rootDirectoryRemoved,
            fnw,
            &FolderNavigationWidget::removeRootDirectory);
    if (!Core::EditorManager::currentDocument() && !m_fallbackSyncFilePath.isEmpty())
        fnw->syncWithFilePath(m_fallbackSyncFilePath);

    Core::NavigationView n;
    n.widget = fnw;
    auto filter = new QToolButton;
    filter->setIcon(Utils::Icons::FILTER.icon());
    filter->setToolTip(Tr::tr("Options"));
    filter->setPopupMode(QToolButton::InstantPopup);
    filter->setProperty(StyleHelper::C_NO_ARROW, true);
    auto filterMenu = new QMenu(filter);
    filterMenu->addAction(fnw->m_filterHiddenFilesAction);
    filterMenu->addAction(fnw->m_showBreadCrumbsAction);
    filterMenu->addAction(fnw->m_showFoldersOnTopAction);
    filter->setMenu(filterMenu);
    n.dockToolBarWidgets << filter << fnw->m_toggleSync;
    return n;
}

const bool kHiddenFilesDefault = false;
const bool kAutoSyncDefault = true;
const bool kShowBreadCrumbsDefault = true;
const bool kRootAutoSyncDefault = true;
const bool kShowFoldersOnTopDefault = true;

void FolderNavigationWidgetFactory::saveSettings(QtcSettings *settings,
                                                 int position,
                                                 QWidget *widget)
{
    auto fnw = qobject_cast<FolderNavigationWidget *>(widget);
    QTC_ASSERT(fnw, return);
    const Key base = numberedKey(kSettingsBase, position);
    settings->setValueWithDefault(base + kHiddenFilesKey,
                                  fnw->hiddenFilesFilter(),
                                  kHiddenFilesDefault);
    settings->setValueWithDefault(base + kSyncKey, fnw->autoSynchronization(), kAutoSyncDefault);
    settings->setValueWithDefault(base + kShowBreadCrumbs,
                                  fnw->isShowingBreadCrumbs(),
                                  kShowBreadCrumbsDefault);
    settings->setValueWithDefault(base + kSyncRootWithEditor,
                                  fnw->rootAutoSynchronization(),
                                  kRootAutoSyncDefault);
    settings->setValueWithDefault(base + kShowFoldersOnTop,
                                  fnw->isShowingFoldersOnTop(),
                                  kShowFoldersOnTopDefault);
}

void FolderNavigationWidgetFactory::restoreSettings(QtcSettings *settings, int position, QWidget *widget)
{
    auto fnw = qobject_cast<FolderNavigationWidget *>(widget);
    QTC_ASSERT(fnw, return);
    const Key base = numberedKey(kSettingsBase, position);
    fnw->setHiddenFilesFilter(settings->value(base + kHiddenFilesKey, kHiddenFilesDefault).toBool());
    fnw->setAutoSynchronization(settings->value(base + kSyncKey, kAutoSyncDefault).toBool());
    fnw->setShowBreadCrumbs(
        settings->value(base + kShowBreadCrumbs, kShowBreadCrumbsDefault).toBool());
    fnw->setRootAutoSynchronization(
        settings->value(base + kSyncRootWithEditor, kRootAutoSyncDefault).toBool());
    fnw->setShowFoldersOnTop(
        settings->value(base + kShowFoldersOnTop, kShowFoldersOnTopDefault).toBool());
}

void FolderNavigationWidgetFactory::insertRootDirectory(const RootDirectory &directory)
{
    const int index = rootIndex(directory.id);
    if (index < 0)
        m_rootDirectories.append(directory);
    else
        m_rootDirectories[index] = directory;
    emit m_instance->rootDirectoryAdded(directory);
}

void FolderNavigationWidgetFactory::removeRootDirectory(const QString &id)
{
    const int index = rootIndex(id);
    QTC_ASSERT(index >= 0, return );
    m_rootDirectories.removeAt(index);
    emit m_instance->rootDirectoryRemoved(id);
}

void FolderNavigationWidgetFactory::setFallbackSyncFilePath(const FilePath &filePath)
{
    m_fallbackSyncFilePath = filePath;
}

int FolderNavigationWidgetFactory::rootIndex(const QString &id)
{
    return Utils::indexOf(m_rootDirectories,
                          [id](const RootDirectory &entry) { return entry.id == id; });
}

void FolderNavigationWidgetFactory::updateProjectsDirectoryRoot()
{
    insertRootDirectory({QLatin1String(PROJECTSDIRECTORYROOT_ID),
                         20 /*sortValue*/,
                         Tr::tr("Projects"),
                         Core::DocumentManager::projectsDirectory(),
                         Utils::Icons::PROJECT.icon()});
}

static FolderNavigationWidget *currentFolderNavigationWidget()
{
    return qobject_cast<FolderNavigationWidget *>(Core::ICore::currentContextWidget());
}

void FolderNavigationWidgetFactory::addRootPath(Utils::Id id, const QString &displayName, const QIcon &icon, const Utils::FilePath &path)
{
    if (path.isDir())
        insertRootDirectory({id.toString(), 0, displayName, path, icon});
}

void FolderNavigationWidgetFactory::removeRootPath(Utils::Id id)
{
    removeRootDirectory(id.toString());
}

void FolderNavigationWidgetFactory::registerActions()
{
    Core::Context context(C_FOLDERNAVIGATIONWIDGET);

    auto add = new QAction(Tr::tr("Add New..."), this);
    Core::ActionManager::registerAction(add, ADDNEWFILE, context);
    connect(add, &QAction::triggered, Core::ICore::instance(), [] {
        if (auto navWidget = currentFolderNavigationWidget())
            navWidget->addNewItem();
    });

    auto rename = new QAction(Tr::tr("Rename..."), this);
    Core::ActionManager::registerAction(rename, RENAMEFILE, context);
    connect(rename, &QAction::triggered, Core::ICore::instance(), [] {
        if (auto navWidget = currentFolderNavigationWidget())
            navWidget->editCurrentItem();
    });

    auto remove = new QAction(Tr::tr("Remove..."), this);
    Core::ActionManager::registerAction(remove, REMOVEFILE, context);
    connect(remove, &QAction::triggered, Core::ICore::instance(), [] {
        if (auto navWidget = currentFolderNavigationWidget())
            navWidget->removeCurrentItem();
    });
}

int DelayedFileCrumbLabel::immediateHeightForWidth(int w) const
{
    return Utils::FileCrumbLabel::heightForWidth(w);
}

int DelayedFileCrumbLabel::heightForWidth(int w) const
{
    static const int doubleDefaultInterval = 800;
    static QHash<int, int> oldHeight;
    setScrollBarOnce();
    int newHeight = Utils::FileCrumbLabel::heightForWidth(w);
    if (!m_delaying || !oldHeight.contains(w)) {
        oldHeight.insert(w, newHeight);
    } else if (oldHeight.value(w) != newHeight){
        auto that = const_cast<DelayedFileCrumbLabel *>(this);
        QTimer::singleShot(std::max(2 * QApplication::doubleClickInterval(), doubleDefaultInterval),
                           that,
                           [that, w, newHeight] {
                               oldHeight.insert(w, newHeight);
                               that->m_delaying = false;
                               that->updateGeometry();
                           });
    }
    return oldHeight.value(w);
}

void DelayedFileCrumbLabel::delayLayoutOnce()
{
    m_delaying = true;
}

void DelayedFileCrumbLabel::setScrollBarOnce(QScrollBar *bar, int value)
{
    m_bar = bar;
    m_barValue = value;
}

void DelayedFileCrumbLabel::setScrollBarOnce() const
{
    if (!m_bar)
        return;
    auto that = const_cast<DelayedFileCrumbLabel *>(this);
    that->m_bar->setValue(m_barValue);
    that->m_bar.clear();
}

} // namespace ProjectExplorer
