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

#include <extensionsystem/pluginmanager.h>

#include <coreplugin/actionmanager/command.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/fileiconprovider.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/fileutils.h>
#include <coreplugin/find/findplugin.h>

#include <texteditor/findinfiles.h>

#include <utils/hostosinfo.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/elidinglabel.h>
#include <utils/itemviews.h>
#include <utils/utilsicons.h>

#include <QDebug>
#include <QSize>
#include <QFileSystemModel>
#include <QVBoxLayout>
#include <QToolButton>
#include <QSortFilterProxyModel>
#include <QAction>
#include <QMenu>
#include <QFileDialog>
#include <QContextMenuEvent>
#include <QDir>
#include <QFileInfo>

enum { debug = 0 };

namespace ProjectExplorer {
namespace Internal {

// Hide the '.' entry.
class DotRemovalFilter : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit DotRemovalFilter(QObject *parent = nullptr);
protected:
    virtual bool filterAcceptsRow(int source_row, const QModelIndex &parent) const;
    Qt::DropActions supportedDragActions() const;
};

DotRemovalFilter::DotRemovalFilter(QObject *parent) : QSortFilterProxyModel(parent)
{ }

bool DotRemovalFilter::filterAcceptsRow(int source_row, const QModelIndex &parent) const
{
    const QVariant fileName = sourceModel()->data(parent.child(source_row, 0));
    if (Utils::HostOsInfo::isAnyUnixHost())
        if (sourceModel()->data(parent) == QLatin1String("/") && fileName == QLatin1String(".."))
            return false;
    return fileName != QLatin1String(".");
}

Qt::DropActions DotRemovalFilter::supportedDragActions() const
{
    return sourceModel()->supportedDragActions();
}

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

/*!
  \class FolderNavigationWidget

  Shows a file system folder
  */
FolderNavigationWidget::FolderNavigationWidget(QWidget *parent) : QWidget(parent),
    m_listView(new Utils::ListView(this)),
    m_fileSystemModel(new FolderNavigationModel(this)),
    m_filterHiddenFilesAction(new QAction(tr("Show Hidden Files"), this)),
    m_filterModel(new DotRemovalFilter(this)),
    m_title(new Utils::ElidingLabel(this)),
    m_toggleSync(new QToolButton(this))
{
    m_fileSystemModel->setResolveSymlinks(false);
    m_fileSystemModel->setIconProvider(Core::FileIconProvider::iconProvider());
    QDir::Filters filters = QDir::AllDirs | QDir::Files | QDir::Drives
                            | QDir::Readable| QDir::Writable
                            | QDir::Executable | QDir::Hidden;
    if (Utils::HostOsInfo::isWindowsHost()) // Symlinked directories can cause file watcher warnings on Win32.
        filters |= QDir::NoSymLinks;
    m_fileSystemModel->setFilter(filters);
    m_filterModel->setSourceModel(m_fileSystemModel);
    m_filterHiddenFilesAction->setCheckable(true);
    setHiddenFilesFilter(false);
    m_listView->setIconSize(QSize(16,16));
    m_listView->setModel(m_filterModel);
    m_listView->setFrameStyle(QFrame::NoFrame);
    m_listView->setAttribute(Qt::WA_MacShowFocusRect, false);
    m_listView->setDragEnabled(true);
    m_listView->setDragDropMode(QAbstractItemView::DragOnly);
    setFocusProxy(m_listView);

    auto layout = new QVBoxLayout();
    layout->addWidget(m_title);
    layout->addWidget(m_listView);
    m_title->setMargin(5);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);

    m_toggleSync->setIcon(Utils::Icons::LINK.icon());
    m_toggleSync->setCheckable(true);
    m_toggleSync->setToolTip(tr("Synchronize with Editor"));
    setAutoSynchronization(true);

    // connections
    connect(m_listView, &QAbstractItemView::activated,
            this, &FolderNavigationWidget::slotOpenItem);
    connect(m_filterHiddenFilesAction, &QAction::toggled,
            this, &FolderNavigationWidget::setHiddenFilesFilter);
    connect(m_toggleSync, &QAbstractButton::clicked,
            this, &FolderNavigationWidget::toggleAutoSynchronization);
    connect(m_filterModel, &QAbstractItemModel::layoutChanged,
            this, &FolderNavigationWidget::ensureCurrentIndex);
}

void FolderNavigationWidget::toggleAutoSynchronization()
{
    setAutoSynchronization(!m_autoSync);
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
                this, &FolderNavigationWidget::setCurrentFile);
        setCurrentFile(Core::EditorManager::currentEditor());
    } else {
        disconnect(Core::EditorManager::instance(), &Core::EditorManager::currentEditorChanged,
                this, &FolderNavigationWidget::setCurrentFile);
    }
}

void FolderNavigationWidget::setCurrentFile(Core::IEditor *editor)
{
    if (!editor)
        return;

    const QString filePath = editor->document()->filePath().toString();
    // Try to find directory of current file
    bool pathOpened = false;
    if (!filePath.isEmpty())  {
        const QFileInfo fi(filePath);
        if (fi.exists())
            pathOpened = setCurrentDirectory(fi.absolutePath());
    }
    if (!pathOpened)  // Default to home.
        setCurrentDirectory(Utils::PathChooser::homePath());

    // Select the current file.
    if (pathOpened) {
        const QModelIndex fileIndex = m_fileSystemModel->index(filePath);
        if (fileIndex.isValid()) {
            QItemSelectionModel *selections = m_listView->selectionModel();
            const QModelIndex mainIndex = m_filterModel->mapFromSource(fileIndex);
            selections->setCurrentIndex(mainIndex, QItemSelectionModel::SelectCurrent
                                                 | QItemSelectionModel::Clear);
            m_listView->scrollTo(mainIndex);
        }
    }
}

bool FolderNavigationWidget::setCurrentDirectory(const QString &directory)
{
    const QString newDirectory = directory.isEmpty() ? QDir::rootPath() : directory;
    if (debug)
        qDebug() << "setcurdir" << directory << newDirectory;
    // Set the root path on the model instead of changing the top index
    // of the view to cause the model to clean out its file watchers.
    const QModelIndex index = m_fileSystemModel->setRootPath(newDirectory);
    if (!index.isValid()) {
        setCurrentTitle(QString(), QString());
        return false;
    }
    QModelIndex oldRootIndex = m_listView->rootIndex();
    QModelIndex newRootIndex = m_filterModel->mapFromSource(index);
    m_listView->setRootIndex(newRootIndex);
    const QDir current(QDir::cleanPath(newDirectory));
    setCurrentTitle(current.dirName(),
                    QDir::toNativeSeparators(current.absolutePath()));
    if (oldRootIndex.parent() == newRootIndex) { // cdUp, so select the old directory
        m_listView->setCurrentIndex(oldRootIndex);
        m_listView->scrollTo(oldRootIndex, QAbstractItemView::EnsureVisible);
    }

    return !directory.isEmpty();
}

QString FolderNavigationWidget::currentDirectory() const
{
    return m_fileSystemModel->rootPath();
}

void FolderNavigationWidget::slotOpenItem(const QModelIndex &viewIndex)
{
    if (viewIndex.isValid())
        openItem(m_filterModel->mapToSource(viewIndex));
}

void FolderNavigationWidget::openItem(const QModelIndex &srcIndex, bool openDirectoryAsProject)
{
    const QString fileName = m_fileSystemModel->fileName(srcIndex);
    if (fileName == QLatin1String("."))
        return;
    if (fileName == QLatin1String("..")) {
        // cd up: Special behaviour: The fileInfo of ".." is that of the parent directory.
        const QString parentPath = m_fileSystemModel->fileInfo(srcIndex).absoluteFilePath();
        setCurrentDirectory(parentPath);
        return;
    }
    const QString path = m_fileSystemModel->filePath(srcIndex);
    if (m_fileSystemModel->isDir(srcIndex)) {
        const QFileInfo fi = m_fileSystemModel->fileInfo(srcIndex);
        if (!fi.isReadable() || !fi.isExecutable())
            return;
        // Try to find project files in directory and open those.
        if (openDirectoryAsProject) {
            const QStringList projectFiles = FolderNavigationWidget::projectFilesInDirectory(path);
            if (!projectFiles.isEmpty())
                Core::ICore::instance()->openFiles(projectFiles);
            return;
        }
        // Change to directory
        setCurrentDirectory(path);
        return;
    }
    // Open file.
    Core::ICore::instance()->openFiles(QStringList(path));
}

void FolderNavigationWidget::setCurrentTitle(QString dirName, const QString &fullPath)
{
    if (dirName.isEmpty())
        dirName = fullPath;
    m_title->setText(dirName);
    m_title->setToolTip(fullPath);
}

QModelIndex FolderNavigationWidget::currentItem() const
{
    const QModelIndex current = m_listView->currentIndex();
    if (current.isValid())
        return m_filterModel->mapToSource(current);
    return QModelIndex();
}

// Format the text for the "open" action of the context menu according
// to the selectect entry
static inline QString actionOpenText(const QFileSystemModel *model,
                                     const QModelIndex &index)
{
    if (!index.isValid())
        return FolderNavigationWidget::tr("Open");
    const QString fileName = model->fileName(index);
    if (fileName == QLatin1String(".."))
        return FolderNavigationWidget::tr("Open Parent Folder");
    return FolderNavigationWidget::tr("Open \"%1\"").arg(fileName);
}

void FolderNavigationWidget::contextMenuEvent(QContextMenuEvent *ev)
{
    QMenu menu;
    // Open current item
    const QModelIndex current = currentItem();
    const bool hasCurrentItem = current.isValid();
    QAction *actionOpen = menu.addAction(actionOpenText(m_fileSystemModel, current));
    actionOpen->setEnabled(hasCurrentItem);

    // we need dummy DocumentModel::Entry with absolute file path in it
    // to get EditorManager::addNativeDirAndOpenWithActions() working
    Core::DocumentModel::Entry fakeEntry;
    Core::IDocument document;
    document.setFilePath(Utils::FileName::fromString(m_fileSystemModel->filePath(current)));
    fakeEntry.document = &document;
    Core::EditorManager::addNativeDirAndOpenWithActions(&menu, &fakeEntry);

    const bool isDirectory = hasCurrentItem && m_fileSystemModel->isDir(current);
    QAction *actionOpenDirectoryAsProject = 0;
    if (isDirectory && m_fileSystemModel->fileName(current) != QLatin1String("..")) {
        actionOpenDirectoryAsProject =
            menu.addAction(tr("Open Project in \"%1\"")
                           .arg(m_fileSystemModel->fileName(current)));
    }

    // Open file dialog to choose a path starting from current
    QAction *actionChooseFolder = menu.addAction(tr("Choose Folder..."));

    QAction *action = menu.exec(ev->globalPos());
    if (!action)
        return;

    ev->accept();
    if (action == actionOpen) { // Handle open file.
        openItem(current);
    } else if (action == actionOpenDirectoryAsProject) {
        openItem(current, true);
    } else if (action == actionChooseFolder) { // Open file dialog
        const QString newPath = QFileDialog::getExistingDirectory(this, tr("Choose Folder"), currentDirectory());
        if (!newPath.isEmpty())
            setCurrentDirectory(newPath);
    }
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

void FolderNavigationWidget::ensureCurrentIndex()
{
    QModelIndex index = m_listView->currentIndex();
    if (!index.isValid()
            || index.parent() != m_listView->rootIndex()) {
        index = m_listView->rootIndex().child(0, 0);
        m_listView->setCurrentIndex(index);
    }
    m_listView->scrollTo(index);
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
    setDisplayName(tr("File System"));
    setPriority(400);
    setId("File System");
    setActivationSequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+Y") : tr("Alt+Y")));
}

Core::NavigationView FolderNavigationWidgetFactory::createWidget()
{
    Core::NavigationView n;
    auto fnw = new FolderNavigationWidget;
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

void FolderNavigationWidgetFactory::saveSettings(int position, QWidget *widget)
{
    auto fnw = qobject_cast<FolderNavigationWidget *>(widget);
    QTC_ASSERT(fnw, return);
    QSettings *settings = Core::ICore::settings();
    const QString baseKey = QLatin1String("FolderNavigationWidget.") + QString::number(position);
    settings->setValue(baseKey + QLatin1String(".HiddenFilesFilter"), fnw->hiddenFilesFilter());
    settings->setValue(baseKey + QLatin1String(".SyncWithEditor"), fnw->autoSynchronization());
}

void FolderNavigationWidgetFactory::restoreSettings(int position, QWidget *widget)
{
    auto fnw = qobject_cast<FolderNavigationWidget *>(widget);
    QTC_ASSERT(fnw, return);
    QSettings *settings = Core::ICore::settings();
    const QString baseKey = QLatin1String("FolderNavigationWidget.") + QString::number(position);
    fnw->setHiddenFilesFilter(settings->value(baseKey + QLatin1String(".HiddenFilesFilter"), false).toBool());
    fnw->setAutoSynchronization(settings->value(baseKey +  QLatin1String(".SyncWithEditor"), true).toBool());
}
} // namespace Internal
} // namespace ProjectExplorer

#include "foldernavigationwidget.moc"
