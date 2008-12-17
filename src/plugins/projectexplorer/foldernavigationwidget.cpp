/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "foldernavigationwidget.h"
#include "projectexplorer.h"
#include "projectexplorerconstants.h"

#include <coreplugin/icore.h>
#include <coreplugin/filemanager.h>
#include <coreplugin/editormanager/editormanager.h>

#include <utils/pathchooser.h>

#include <QtCore/QDebug>
#include <QtGui/QVBoxLayout>
#include <QtGui/QToolButton>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

namespace {
bool debug = false;
}

namespace ProjectExplorer {
namespace Internal {

class FirstRowFilter : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    FirstRowFilter(QObject *parent = 0) : QSortFilterProxyModel(parent) {}
protected:
    bool filterAcceptsRow (int source_row, const QModelIndex & ) const {
        return source_row != 0;
    }
};

} // namespace Internal
} // namespace ProjectExplorer

/*!
  /class FolderNavigationWidget

  Shows a file system folder
  */
FolderNavigationWidget::FolderNavigationWidget(Core::ICore *core, QWidget *parent)
        : QWidget(parent),
          m_core(core),
          m_explorer(ProjectExplorerPlugin::instance()),
          m_view(new QListView(this)),
          m_dirModel(new QDirModel(this)),
          m_filter(new FirstRowFilter(this)),
          m_title(new QLabel(this)),
          m_autoSync(false)
{
    m_dirModel->setFilter(QDir::Dirs | QDir::Files | QDir::Drives | QDir::Readable | QDir::Writable
                          | QDir::Executable | QDir::Hidden);
    m_dirModel->setSorting(QDir::Name | QDir::DirsFirst);
    m_filter->setSourceModel(m_dirModel);
    m_view->setModel(m_filter);
    m_view->setFrameStyle(QFrame::NoFrame);
    setFocusProxy(m_view);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(m_title);
    layout->addWidget(m_view);
    m_title->setMargin(5);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);

    // connections
    connect(m_view, SIGNAL(activated(const QModelIndex&)),
            this, SLOT(openItem(const QModelIndex&)));

    setAutoSynchronization(true);
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
    if (sync == m_autoSync)
        return;

    m_autoSync = sync;

    Core::FileManager *fileManager = m_core->fileManager();
    if (m_autoSync) {
        connect(fileManager, SIGNAL(currentFileChanged(const QString&)),
                this, SLOT(setCurrentFile(const QString&)));
        setCurrentFile(fileManager->currentFile());
    } else {
        disconnect(fileManager, SIGNAL(currentFileChanged(const QString&)),
                this, SLOT(setCurrentFile(const QString&)));
    }
}

void FolderNavigationWidget::setCurrentFile(const QString &filePath)
{
    if (debug)
        qDebug() << "FolderNavigationWidget::setCurrentFile(" << filePath << ")";

    QString dir = QFileInfo(filePath).path();
    if (dir.isEmpty())
        dir = Core::Utils::PathChooser::homePath();

    QModelIndex dirIndex = m_dirModel->index(dir);
    QModelIndex fileIndex = m_dirModel->index(filePath);

    m_view->setRootIndex(m_filter->mapFromSource(dirIndex));
    if (dirIndex.isValid()) {
        setCurrentTitle(QDir(m_dirModel->filePath(dirIndex)));
        if (fileIndex.isValid()) {
            QItemSelectionModel *selections = m_view->selectionModel();
            QModelIndex mainIndex = m_filter->mapFromSource(fileIndex);
            selections->setCurrentIndex(mainIndex, QItemSelectionModel::SelectCurrent
                                                 | QItemSelectionModel::Clear);
            m_view->scrollTo(mainIndex);
        }
    } else {
        setCurrentTitle(QDir());
    }
}

void FolderNavigationWidget::openItem(const QModelIndex &index)
{
    if (index.isValid()) {
        const QModelIndex srcIndex = m_filter->mapToSource(index);
        if (m_dirModel->isDir(srcIndex)) {
            m_view->setRootIndex(index);
            setCurrentTitle(QDir(m_dirModel->filePath(srcIndex)));
        } else {
            const QString filePath = m_dirModel->filePath(srcIndex);
            m_core->editorManager()->openEditor(filePath);
            m_core->editorManager()->ensureEditorManagerVisible();
        }
    }
}

void FolderNavigationWidget::setCurrentTitle(const QDir &dir)
{
    m_title->setText(dir.dirName());
    m_title->setToolTip(dir.absolutePath());
}

FolderNavigationWidgetFactory::FolderNavigationWidgetFactory(Core::ICore *core)
    : m_core(core)
{
}

FolderNavigationWidgetFactory::~FolderNavigationWidgetFactory()
{
}

QString FolderNavigationWidgetFactory::displayName()
{
    return tr("File System");
}

QKeySequence FolderNavigationWidgetFactory::activationSequence()
{
    return QKeySequence(Qt::ALT + Qt::Key_Y);
}

Core::NavigationView FolderNavigationWidgetFactory::createWidget()
{
    Core::NavigationView n;
    FolderNavigationWidget *ptw = new FolderNavigationWidget(m_core);
    n.widget = ptw;
    QToolButton *toggleSync = new QToolButton;
    toggleSync->setProperty("type", "dockbutton");
    toggleSync->setIcon(QIcon(":/qworkbench/images/linkicon.png"));
    toggleSync->setCheckable(true);
    toggleSync->setChecked(ptw->autoSynchronization());
    toggleSync->setToolTip(tr("Synchronize with Editor"));
    connect(toggleSync, SIGNAL(clicked(bool)), ptw, SLOT(toggleAutoSynchronization()));
    n.doockToolBarWidgets << toggleSync;
    return n;
}

#include "foldernavigationwidget.moc"
