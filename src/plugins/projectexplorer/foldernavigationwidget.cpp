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

#include "foldernavigationwidget.h"
#include "projectexplorer.h"
#include "projectexplorerconstants.h"

#include <extensionsystem/pluginmanager.h>

#include <coreplugin/actionmanager/command.h>
#include <coreplugin/icore.h>
#include <coreplugin/fileiconprovider.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/fileutils.h>

#include <find/findplugin.h>
#include <texteditor/findinfiles.h>

#include <utils/environment.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/elidinglabel.h>

#include <QDebug>
#include <QSize>
#include <QFileSystemModel>
#include <QVBoxLayout>
#include <QToolButton>
#include <QPushButton>
#include <QLabel>
#include <QListView>
#include <QSortFilterProxyModel>
#include <QAction>
#include <QMenu>
#include <QFileDialog>
#include <QContextMenuEvent>

enum { debug = 0 };

namespace ProjectExplorer {
namespace Internal {

// Hide the '.' entry.
class DotRemovalFilter : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit DotRemovalFilter(QObject *parent = 0);
protected:
    virtual bool filterAcceptsRow(int source_row, const QModelIndex &parent) const;
private:
#if defined(Q_OS_UNIX)
    const QVariant m_root;
    const QVariant m_dotdot;
#endif
    const QVariant m_dot;
};

DotRemovalFilter::DotRemovalFilter(QObject *parent) :
    QSortFilterProxyModel(parent),
#if defined(Q_OS_UNIX)
    m_root(QString(QLatin1Char('/'))),
    m_dotdot(QLatin1String("..")),
#endif
    m_dot(QString(QLatin1Char('.')))
{
}

bool DotRemovalFilter::filterAcceptsRow(int source_row, const QModelIndex &parent) const
{
    const QVariant fileName = sourceModel()->data(parent.child(source_row, 0));
#if defined(Q_OS_UNIX)
    if (sourceModel()->data(parent) == m_root && fileName == m_dotdot)
        return false;
#endif
    return fileName != m_dot;
}

// FolderNavigationModel: Shows path as tooltip.
class FolderNavigationModel : public QFileSystemModel
{
public:
    explicit FolderNavigationModel(QObject *parent = 0);
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
};

FolderNavigationModel::FolderNavigationModel(QObject *parent) :
    QFileSystemModel(parent)
{
}

QVariant FolderNavigationModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::ToolTipRole)
        return QDir::toNativeSeparators(QDir::cleanPath(filePath(index)));
    else
        return QFileSystemModel::data(index, role);
}

/*!
  \class FolderNavigationWidget

  Shows a file system folder
  */
FolderNavigationWidget::FolderNavigationWidget(QWidget *parent)
    : QWidget(parent),
      m_listView(new QListView(this)),
      m_fileSystemModel(new FolderNavigationModel(this)),
      m_filterHiddenFilesAction(new QAction(tr("Show Hidden Files"), this)),
      m_filterModel(new DotRemovalFilter(this)),
      m_title(new Utils::ElidingLabel(this)),
      m_autoSync(false),
      m_toggleSync(new QToolButton(this))
{
    m_fileSystemModel->setResolveSymlinks(false);
    m_fileSystemModel->setIconProvider(Core::FileIconProvider::instance());
    QDir::Filters filters = QDir::AllDirs | QDir::Files | QDir::Drives
                            | QDir::Readable| QDir::Writable
                            | QDir::Executable | QDir::Hidden;
#ifdef Q_OS_WIN // Symlinked directories can cause file watcher warnings on Win32.
    filters |= QDir::NoSymLinks;
#endif
    m_fileSystemModel->setFilter(filters);
    m_filterModel->setSourceModel(m_fileSystemModel);
    m_filterHiddenFilesAction->setCheckable(true);
    setHiddenFilesFilter(false);
    m_listView->setIconSize(QSize(16,16));
    m_listView->setModel(m_filterModel);
    m_listView->setFrameStyle(QFrame::NoFrame);
    m_listView->setAttribute(Qt::WA_MacShowFocusRect, false);
    setFocusProxy(m_listView);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(m_title);
    layout->addWidget(m_listView);
    m_title->setMargin(5);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);

    m_toggleSync->setIcon(QIcon(QLatin1String(Core::Constants::ICON_LINK)));
    m_toggleSync->setCheckable(true);
    m_toggleSync->setToolTip(tr("Synchronize with Editor"));
    setAutoSynchronization(true);

    // connections
    connect(m_listView, SIGNAL(activated(QModelIndex)),
            this, SLOT(slotOpenItem(QModelIndex)));
    connect(m_filterHiddenFilesAction, SIGNAL(toggled(bool)), this, SLOT(setHiddenFilesFilter(bool)));
    connect(m_toggleSync, SIGNAL(clicked(bool)), this, SLOT(toggleAutoSynchronization()));
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
        connect(Core::DocumentManager::instance(), SIGNAL(currentFileChanged(QString)),
                this, SLOT(setCurrentFile(QString)));
        setCurrentFile(Core::DocumentManager::currentFile());
    } else {
        disconnect(Core::DocumentManager::instance(), SIGNAL(currentFileChanged(QString)),
                this, SLOT(setCurrentFile(QString)));
    }
}

void FolderNavigationWidget::setCurrentFile(const QString &filePath)
{
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
    m_listView->setRootIndex(m_filterModel->mapFromSource(index));
    const QDir current(QDir::cleanPath(newDirectory));
    setCurrentTitle(current.dirName(),
                    QDir::toNativeSeparators(current.absolutePath()));
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

void FolderNavigationWidget::openItem(const QModelIndex &srcIndex)
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
    if (m_fileSystemModel->isDir(srcIndex)) { // Change to directory
        const QFileInfo fi = m_fileSystemModel->fileInfo(srcIndex);
        if (fi.isReadable() && fi.isExecutable())
            setCurrentDirectory(m_fileSystemModel->filePath(srcIndex));
        return;
    }
    // Open file.
    Core::EditorManager::openEditor(m_fileSystemModel->filePath(srcIndex), Core::Id(), Core::EditorManager::ModeSwitch);
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
    // Explorer & teminal
    QAction *actionExplorer = menu.addAction(Core::FileUtils::msgGraphicalShellAction());
    actionExplorer->setEnabled(hasCurrentItem);
    QAction *actionTerminal = menu.addAction(Core::FileUtils::msgTerminalAction());
    actionTerminal->setEnabled(hasCurrentItem);

    QAction *actionFind = menu.addAction(msgFindOnFileSystem());
    actionFind->setEnabled(hasCurrentItem);
    // open with...
    if (!m_fileSystemModel->isDir(current)) {
        QMenu *openWith = menu.addMenu(tr("Open with"));
        Core::DocumentManager::populateOpenWithMenu(openWith,
                                                m_fileSystemModel->filePath(current));
    }

    // Open file dialog to choose a path starting from current
    QAction *actionChooseFolder = menu.addAction(tr("Choose Folder..."));

    QAction *action = menu.exec(ev->globalPos());
    if (!action)
        return;

    ev->accept();
    if (action == actionOpen) { // Handle open file.
        openItem(current);
        return;
    }
    if (action == actionChooseFolder) { // Open file dialog
        const QString newPath = QFileDialog::getExistingDirectory(this, tr("Choose Folder"), currentDirectory());
        if (!newPath.isEmpty())
            setCurrentDirectory(newPath);
        return;
    }
    if (action == actionTerminal) {
        Core::FileUtils::openTerminal(m_fileSystemModel->filePath(current));
        return;
    }
    if (action == actionExplorer) {
        Core::FileUtils::showInGraphicalShell(this, m_fileSystemModel->filePath(current));
        return;
    }
    if (action == actionFind) {
        QFileInfo info = m_fileSystemModel->fileInfo(current);
        if (m_fileSystemModel->isDir(current))
            findOnFileSystem(info.absoluteFilePath());
        else
            findOnFileSystem(info.absolutePath());
        return;
    }
    Core::DocumentManager::executeOpenWithMenuAction(action);
}

QString FolderNavigationWidget::msgFindOnFileSystem()
{
    return tr("Find in this directory...");
}

void FolderNavigationWidget::findOnFileSystem(const QString &pathIn)
{
    const QFileInfo fileInfo(pathIn);
    const QString folder = fileInfo.isDir() ? fileInfo.absoluteFilePath() : fileInfo.absolutePath();

    TextEditor::FindInFiles *fif = ExtensionSystem::PluginManager::getObject<TextEditor::FindInFiles>();
    if (!fif)
        return;
    Find::FindPlugin *plugin = Find::FindPlugin::instance();
    if (!plugin)
        return;
    fif->setDirectory(folder);
    Find::FindPlugin::instance()->openFindDialog(fif);
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

// --------------------FolderNavigationWidgetFactory
FolderNavigationWidgetFactory::FolderNavigationWidgetFactory()
{
}

FolderNavigationWidgetFactory::~FolderNavigationWidgetFactory()
{
}

QString FolderNavigationWidgetFactory::displayName() const
{
    return tr("File System");
}

int FolderNavigationWidgetFactory::priority() const
{
    return 400;
}

Core::Id FolderNavigationWidgetFactory::id() const
{
    return Core::Id("File System");
}

QKeySequence FolderNavigationWidgetFactory::activationSequence() const
{
    return QKeySequence(Core::UseMacShortcuts ? tr("Meta+Y") : tr("Alt+Y"));
}

Core::NavigationView FolderNavigationWidgetFactory::createWidget()
{
    Core::NavigationView n;
    FolderNavigationWidget *fnw = new FolderNavigationWidget;
    n.widget = fnw;
    QToolButton *filter = new QToolButton;
    filter->setIcon(QIcon(QLatin1String(Core::Constants::ICON_FILTER)));
    filter->setToolTip(tr("Filter Files"));
    filter->setPopupMode(QToolButton::InstantPopup);
    filter->setProperty("noArrow", true);
    QMenu *filterMenu = new QMenu(filter);
    filterMenu->addAction(fnw->m_filterHiddenFilesAction);
    filter->setMenu(filterMenu);
    n.dockToolBarWidgets << filter << fnw->m_toggleSync;
    return n;
}

void FolderNavigationWidgetFactory::saveSettings(int position, QWidget *widget)
{
    FolderNavigationWidget *fnw = qobject_cast<FolderNavigationWidget *>(widget);
    QTC_ASSERT(fnw, return);
    QSettings *settings = Core::ICore::settings();
    const QString baseKey = QLatin1String("FolderNavigationWidget.") + QString::number(position);
    settings->setValue(baseKey + QLatin1String(".HiddenFilesFilter"), fnw->hiddenFilesFilter());
    settings->setValue(baseKey + QLatin1String(".SyncWithEditor"), fnw->autoSynchronization());
}

void FolderNavigationWidgetFactory::restoreSettings(int position, QWidget *widget)
{
    FolderNavigationWidget *fnw = qobject_cast<FolderNavigationWidget *>(widget);
    QTC_ASSERT(fnw, return);
    QSettings *settings = Core::ICore::settings();
    const QString baseKey = QLatin1String("FolderNavigationWidget.") + QString::number(position);
    fnw->setHiddenFilesFilter(settings->value(baseKey + QLatin1String(".HiddenFilesFilter"), false).toBool());
    fnw->setAutoSynchronization(settings->value(baseKey +  QLatin1String(".SyncWithEditor"), true).toBool());
}
} // namespace Internal
} // namespace ProjectExplorer

#include "foldernavigationwidget.moc"
