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

#include "foldernavigationwidget.h"
#include "projectexplorer.h"
#include "projectexplorericons.h"

#include <coreplugin/actionmanager/command.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/fileiconprovider.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/fileutils.h>

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/navigationtreeview.h>
#include <utils/utilsicons.h>

#include <QComboBox>
#include <QHeaderView>
#include <QSize>
#include <QTimer>
#include <QFileSystemModel>
#include <QVBoxLayout>
#include <QToolButton>
#include <QAction>
#include <QMenu>
#include <QContextMenuEvent>
#include <QDir>
#include <QFileInfo>

const int PATH_ROLE = Qt::UserRole;
const int ID_ROLE = Qt::UserRole + 1;
const int SORT_ROLE = Qt::UserRole + 2;

const char PROJECTSDIRECTORYROOT_ID[] = "A.Projects";

namespace ProjectExplorer {
namespace Internal {

static FolderNavigationWidgetFactory *m_instance = nullptr;

QVector<FolderNavigationWidgetFactory::RootDirectory>
    FolderNavigationWidgetFactory::m_rootDirectories;

// FolderNavigationModel: Shows path as tooltip.
class FolderNavigationModel : public QFileSystemModel
{
public:
    explicit FolderNavigationModel(QObject *parent = nullptr);
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    Qt::DropActions supportedDragActions() const;
};

FolderNavigationModel::FolderNavigationModel(QObject *parent) : QFileSystemModel(parent)
{ }

QVariant FolderNavigationModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::ToolTipRole)
        return QDir::toNativeSeparators(QDir::cleanPath(filePath(index)));
    else
        return QFileSystemModel::data(index, role);
}

Qt::DropActions FolderNavigationModel::supportedDragActions() const
{
    return Qt::MoveAction;
}

static void showOnlyFirstColumn(QTreeView *view)
{
    const int columnCount = view->header()->count();
    for (int i = 1; i < columnCount; ++i)
        view->setColumnHidden(i, true);
}

/*!
    \class FolderNavigationWidget

    Shows a file system tree, with the root directory selectable from a dropdown.

    \internal
*/
FolderNavigationWidget::FolderNavigationWidget(QWidget *parent) : QWidget(parent),
    m_listView(new Utils::NavigationTreeView(this)),
    m_fileSystemModel(new FolderNavigationModel(this)),
    m_filterHiddenFilesAction(new QAction(tr("Show Hidden Files"), this)),
    m_toggleSync(new QToolButton(this)),
    m_rootSelector(new QComboBox)
{
    m_fileSystemModel->setResolveSymlinks(false);
    m_fileSystemModel->setIconProvider(Core::FileIconProvider::iconProvider());
    QDir::Filters filters = QDir::AllEntries | QDir::NoDotAndDotDot;
    if (Utils::HostOsInfo::isWindowsHost()) // Symlinked directories can cause file watcher warnings on Win32.
        filters |= QDir::NoSymLinks;
    m_fileSystemModel->setFilter(filters);
    m_fileSystemModel->setRootPath(QString());
    m_filterHiddenFilesAction->setCheckable(true);
    setHiddenFilesFilter(false);
    m_listView->setIconSize(QSize(16,16));
    m_listView->setModel(m_fileSystemModel);
    m_listView->setDragEnabled(true);
    m_listView->setDragDropMode(QAbstractItemView::DragOnly);
    showOnlyFirstColumn(m_listView);
    setFocusProxy(m_listView);

    auto layout = new QVBoxLayout();
    layout->addWidget(m_rootSelector);
    layout->addWidget(m_listView);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);

    m_toggleSync->setIcon(Utils::Icons::LINK.icon());
    m_toggleSync->setCheckable(true);
    m_toggleSync->setToolTip(tr("Synchronize with Editor"));
    setAutoSynchronization(true);

    // connections
    connect(m_listView, &QAbstractItemView::activated,
            this, [this](const QModelIndex &index) { openItem(index); });
    connect(m_filterHiddenFilesAction, &QAction::toggled,
            this, &FolderNavigationWidget::setHiddenFilesFilter);
    connect(m_toggleSync, &QAbstractButton::clicked,
            this, &FolderNavigationWidget::toggleAutoSynchronization);
    connect(m_rootSelector,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this,
            [this](int index) {
                const auto directory = m_rootSelector->itemData(index).value<Utils::FileName>();
                m_rootSelector->setToolTip(directory.toString());
                setRootDirectory(directory);
            });
    connect(m_rootSelector,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::activated),
            this,
            [this] {
                if (m_autoSync && Core::EditorManager::currentEditor())
                    selectFile(Core::EditorManager::currentEditor()->document()->filePath());
            });
}

void FolderNavigationWidget::toggleAutoSynchronization()
{
    setAutoSynchronization(!m_autoSync);
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
    m_rootSelector->setItemData(index, qVariantFromValue(directory.path), PATH_ROLE);
    m_rootSelector->setItemData(index, directory.id, ID_ROLE);
    m_rootSelector->setItemData(index, directory.sortValue, SORT_ROLE);
    m_rootSelector->setItemData(index, directory.path.toUserOutput(), Qt::ToolTipRole);
    m_rootSelector->setItemIcon(index, directory.icon);
    if (m_rootSelector->currentIndex() == previousIndex)
        m_rootSelector->setCurrentIndex(index);
    if (previousIndex < m_rootSelector->count())
        m_rootSelector->removeItem(previousIndex);
    if (m_autoSync) // we might find a better root for current selection now
        setCurrentEditor(Core::EditorManager::currentEditor());
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
        setCurrentEditor(Core::EditorManager::currentEditor());
}

bool FolderNavigationWidget::autoSynchronization() const
{
    return m_autoSync;
}

void FolderNavigationWidget::setAutoSynchronization(bool sync)
{
    m_toggleSync->setChecked(sync);
    if (sync == m_autoSync)
        return;

    m_autoSync = sync;

    if (m_autoSync) {
        connect(Core::EditorManager::instance(), &Core::EditorManager::currentEditorChanged,
                this, &FolderNavigationWidget::setCurrentEditor);
        setCurrentEditor(Core::EditorManager::currentEditor());
    } else {
        disconnect(Core::EditorManager::instance(), &Core::EditorManager::currentEditorChanged,
                this, &FolderNavigationWidget::setCurrentEditor);
    }
}

void FolderNavigationWidget::setCurrentEditor(Core::IEditor *editor)
{
    if (!editor || editor->document()->filePath().isEmpty() || editor->document()->isTemporary())
        return;
    const Utils::FileName filePath = editor->document()->filePath();
    // switch to most fitting root
    const int bestRootIndex = bestRootForFile(filePath);
    m_rootSelector->setCurrentIndex(bestRootIndex);
    // select
    selectFile(filePath);
}

void FolderNavigationWidget::selectFile(const Utils::FileName &filePath)
{
    const QModelIndex fileIndex = m_fileSystemModel->index(filePath.toString());
    if (fileIndex.isValid()) {
        // TODO This only scrolls to the right position if all directory contents are loaded.
        // Unfortunately listening to directoryLoaded was still not enough (there might also
        // be some delayed sorting involved?).
        // Use magic timer for scrolling.
        m_listView->setCurrentIndex(fileIndex);
        QTimer::singleShot(200, this, [this, filePath] {
            const QModelIndex fileIndex = m_fileSystemModel->index(filePath.toString());
            m_listView->scrollTo(fileIndex);
        });
    }
}

void FolderNavigationWidget::setRootDirectory(const Utils::FileName &directory)
{
    const QModelIndex index = m_fileSystemModel->setRootPath(directory.toString());
    m_listView->setRootIndex(index);
}

int FolderNavigationWidget::bestRootForFile(const Utils::FileName &filePath)
{
    int index = 0; // Computer is default
    int commonLength = 0;
    for (int i = 1; i < m_rootSelector->count(); ++i) {
        const auto root = m_rootSelector->itemData(i).value<Utils::FileName>();
        if (filePath.isChildOf(root) && root.length() > commonLength) {
            index = i;
            commonLength = root.length();
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
    Core::EditorManager::openEditor(path);
}

QStringList FolderNavigationWidget::projectsInDirectory(const QModelIndex &index) const
{
    QTC_ASSERT(index.isValid() && m_fileSystemModel->isDir(index), return {});
    const QFileInfo fi = m_fileSystemModel->fileInfo(index);
    if (!fi.isReadable() || !fi.isExecutable())
        return {};
    const QString path = m_fileSystemModel->filePath(index);
    // Try to find project files in directory and open those.
    return FolderNavigationWidget::projectFilesInDirectory(path);
}

void FolderNavigationWidget::openProjectsInDirectory(const QModelIndex &index)
{
    const QStringList projectFiles = projectsInDirectory(index);
    if (!projectFiles.isEmpty())
        Core::ICore::instance()->openFiles(projectFiles);
}

void FolderNavigationWidget::contextMenuEvent(QContextMenuEvent *ev)
{
    QMenu menu;
    // Open current item
    const QModelIndex current = m_listView->currentIndex();
    const bool hasCurrentItem = current.isValid();
    QAction *actionOpenFile = nullptr;
    QAction *actionOpenProjects = nullptr;
    QAction *actionOpenAsProject = nullptr;
    const Utils::FileName filePath = hasCurrentItem ? Utils::FileName::fromString(
                                                          m_fileSystemModel->filePath(current))
                                                    : Utils::FileName();
    if (hasCurrentItem) {
        const QString fileName = m_fileSystemModel->fileName(current);
        if (m_fileSystemModel->isDir(current)) {
            actionOpenProjects = menu.addAction(tr("Open Project in \"%1\"").arg(fileName));
            if (projectsInDirectory(current).isEmpty())
                actionOpenProjects->setEnabled(false);
        } else {
            actionOpenFile = menu.addAction(tr("Open \"%1\"").arg(fileName));
            if (ProjectExplorerPlugin::isProjectFile(Utils::FileName::fromString(fileName)))
                actionOpenAsProject = menu.addAction(tr("Open Project \"%1\"").arg(fileName));
        }
    }

    // we need dummy DocumentModel::Entry with absolute file path in it
    // to get EditorManager::addNativeDirAndOpenWithActions() working
    Core::DocumentModel::Entry fakeEntry;
    Core::IDocument document;
    document.setFilePath(filePath);
    fakeEntry.document = &document;
    Core::EditorManager::addNativeDirAndOpenWithActions(&menu, &fakeEntry);

    QAction *action = menu.exec(ev->globalPos());
    if (!action)
        return;

    ev->accept();
    if (action == actionOpenFile)
        openItem(current);
    else if (action == actionOpenAsProject)
        ProjectExplorerPlugin::openProject(filePath.toString());
    else if (action == actionOpenProjects)
        openProjectsInDirectory(current);
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

QStringList FolderNavigationWidget::projectFilesInDirectory(const QString &path)
{
    QDir dir(path);
    QStringList projectFiles;
    foreach (const QFileInfo &i, dir.entryInfoList(ProjectExplorerPlugin::projectFileGlobs(), QDir::Files))
        projectFiles.append(i.absoluteFilePath());
    return projectFiles;
}

// --------------------FolderNavigationWidgetFactory
FolderNavigationWidgetFactory::FolderNavigationWidgetFactory()
{
    m_instance = this;
    setDisplayName(tr("File System"));
    setPriority(400);
    setId("File System");
    setActivationSequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+Y") : tr("Alt+Y")));
    insertRootDirectory({QLatin1String("A.Computer"),
                         0 /*sortValue*/,
                         FolderNavigationWidget::tr("Computer"),
                         Utils::FileName(),
                         Icons::DESKTOP_DEVICE_SMALL.icon()});
    insertRootDirectory({QLatin1String("A.Home"),
                         10 /*sortValue*/,
                         FolderNavigationWidget::tr("Home"),
                         Utils::FileName::fromString(QDir::homePath()),
                         Utils::Icons::HOME.icon()});
    updateProjectsDirectoryRoot();
    connect(Core::DocumentManager::instance(),
            &Core::DocumentManager::projectsDirectoryChanged,
            this,
            &FolderNavigationWidgetFactory::updateProjectsDirectoryRoot);
}

Core::NavigationView FolderNavigationWidgetFactory::createWidget()
{
    auto fnw = new FolderNavigationWidget;
    for (const RootDirectory &root : m_rootDirectories)
        fnw->insertRootDirectory(root);
    connect(this,
            &FolderNavigationWidgetFactory::rootDirectoryAdded,
            fnw,
            &FolderNavigationWidget::insertRootDirectory);
    connect(this,
            &FolderNavigationWidgetFactory::rootDirectoryRemoved,
            fnw,
            &FolderNavigationWidget::removeRootDirectory);

    Core::NavigationView n;
    n.widget = fnw;
    auto filter = new QToolButton;
    filter->setIcon(Utils::Icons::FILTER.icon());
    filter->setToolTip(tr("Filter Files"));
    filter->setPopupMode(QToolButton::InstantPopup);
    filter->setProperty("noArrow", true);
    auto filterMenu = new QMenu(filter);
    filterMenu->addAction(fnw->m_filterHiddenFilesAction);
    filter->setMenu(filterMenu);
    n.dockToolBarWidgets << filter << fnw->m_toggleSync;
    return n;
}

void FolderNavigationWidgetFactory::saveSettings(QSettings *settings, int position, QWidget *widget)
{
    auto fnw = qobject_cast<FolderNavigationWidget *>(widget);
    QTC_ASSERT(fnw, return);
    const QString baseKey = QLatin1String("FolderNavigationWidget.") + QString::number(position);
    settings->setValue(baseKey + QLatin1String(".HiddenFilesFilter"), fnw->hiddenFilesFilter());
    settings->setValue(baseKey + QLatin1String(".SyncWithEditor"), fnw->autoSynchronization());
}

void FolderNavigationWidgetFactory::restoreSettings(QSettings *settings, int position, QWidget *widget)
{
    auto fnw = qobject_cast<FolderNavigationWidget *>(widget);
    QTC_ASSERT(fnw, return);
    const QString baseKey = QLatin1String("FolderNavigationWidget.") + QString::number(position);
    fnw->setHiddenFilesFilter(settings->value(baseKey + QLatin1String(".HiddenFilesFilter"), false).toBool());
    fnw->setAutoSynchronization(settings->value(baseKey +  QLatin1String(".SyncWithEditor"), true).toBool());
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

int FolderNavigationWidgetFactory::rootIndex(const QString &id)
{
    return Utils::indexOf(m_rootDirectories,
                          [id](const RootDirectory &entry) { return entry.id == id; });
}

void FolderNavigationWidgetFactory::updateProjectsDirectoryRoot()
{
    insertRootDirectory({QLatin1String(PROJECTSDIRECTORYROOT_ID),
                         20 /*sortValue*/,
                         FolderNavigationWidget::tr("Projects"),
                         Core::DocumentManager::projectsDirectory(),
                         Utils::Icons::PROJECT.icon()});
}

} // namespace Internal
} // namespace ProjectExplorer
