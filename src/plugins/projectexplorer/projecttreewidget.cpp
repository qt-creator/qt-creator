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
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
#include "projecttreewidget.h"
#include "projectexplorer.h"
#include "projectexplorerconstants.h"
#include "projectmodels.h"

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>

#include <QtGui/QHeaderView>
#include <QtGui/QVBoxLayout>
#include <QtGui/QToolButton>
#include <QtGui/QFocusEvent>
#include <QtCore/QDebug>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

namespace {
    bool debug = false;
}

class ProjectTreeView : public QTreeView
{
public:
    ProjectTreeView()
    {
        setEditTriggers(QAbstractItemView::EditKeyPressed);
        setFrameStyle(QFrame::NoFrame);
        setIndentation(indentation() * 9/10);
        {
            QHeaderView *treeHeader = header();
            treeHeader->setVisible(false);
            treeHeader->setResizeMode(QHeaderView::ResizeToContents);
            treeHeader->setStretchLastSection(true);
        }
        setContextMenuPolicy(Qt::CustomContextMenu);
        setUniformRowHeights(true);
        setTextElideMode(Qt::ElideNone);
//        setExpandsOnDoubleClick(false);
    }

protected:
    // This is a workaround to stop Qt from redrawing the project tree every
    // time the user opens or closes a menu when it has focus. Would be nicer to
    // fix it in Qt.
    void focusInEvent(QFocusEvent *event)
    {
        if (event->reason() != Qt::PopupFocusReason)
            QTreeView::focusInEvent(event);
    }

    void focusOutEvent(QFocusEvent *event)
    {
        if (event->reason() != Qt::PopupFocusReason)
            QTreeView::focusOutEvent(event);
    }
};

/*!
  /class ProjectTreeWidget

  Shows the projects in form of a tree.
  */
ProjectTreeWidget::ProjectTreeWidget(Core::ICore *core, QWidget *parent)
        : QWidget(parent),
          m_core(core),
          m_explorer(ProjectExplorerPlugin::instance()),
          m_view(0),
          m_model(0),
          m_filterProjectsAction(0),
          m_autoSync(false)
{
    m_model = new FlatModel(m_explorer->session()->sessionNode(), this);

    m_view = new ProjectTreeView;
    m_view->setModel(m_model);
    setFocusProxy(m_view);
    initView();

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(m_view);
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);

    m_filterProjectsAction = new QAction(tr("Simplify tree"), this);
    m_filterProjectsAction->setCheckable(true);
    m_filterProjectsAction->setChecked(false); // default is the traditional complex tree
    connect(m_filterProjectsAction, SIGNAL(toggled(bool)), this, SLOT(setProjectFilter(bool)));

    m_filterGeneratedFilesAction = new QAction(tr("Hide generated files"), this);
    m_filterGeneratedFilesAction->setCheckable(true);
    m_filterGeneratedFilesAction->setChecked(true);
    connect(m_filterGeneratedFilesAction, SIGNAL(toggled(bool)), this, SLOT(setGeneratedFilesFilter(bool)));

    // connections
    connect(m_model, SIGNAL(modelReset()),
            this, SLOT(initView()));
    connect(m_view, SIGNAL(activated(const QModelIndex&)),
            this, SLOT(openItem(const QModelIndex&)));
    connect(m_view->selectionModel(), SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)),
            this, SLOT(handleCurrentItemChange(const QModelIndex&)));
    connect(m_view, SIGNAL(customContextMenuRequested(const QPoint&)),
            this, SLOT(showContextMenu(const QPoint&)));
    connect(m_explorer->session(), SIGNAL(singleProjectAdded(ProjectExplorer::Project *)),
            this, SLOT(handleProjectAdded(ProjectExplorer::Project *)));
    connect(m_explorer->session(), SIGNAL(startupProjectChanged(ProjectExplorer::Project *)),
            this, SLOT(startupProjectChanged(ProjectExplorer::Project *)));

    setAutoSynchronization(true);
}

void ProjectTreeWidget::toggleAutoSynchronization()
{
    setAutoSynchronization(!m_autoSync);
}

bool ProjectTreeWidget::autoSynchronization() const
{
    return m_autoSync;
}

void ProjectTreeWidget::setAutoSynchronization(bool sync, bool syncNow)
{
    if (sync == m_autoSync)
        return;

    m_autoSync = sync;

    if (debug)
        qDebug() << (m_autoSync ? "Enabling auto synchronization" : "Disabling auto synchronization");
    if (m_autoSync) {
        connect(m_explorer, SIGNAL(currentNodeChanged(ProjectExplorer::Node*, ProjectExplorer::Project*)),
                this, SLOT(setCurrentItem(ProjectExplorer::Node*, ProjectExplorer::Project*)));
        if (syncNow)
            setCurrentItem(m_explorer->currentNode(), m_explorer->currentProject());
    } else {
        disconnect(m_explorer, SIGNAL(currentNodeChanged(ProjectExplorer::Node*, ProjectExplorer::Project*)),
                this, SLOT(setCurrentItem(ProjectExplorer::Node*, ProjectExplorer::Project*)));
    }
}

void ProjectTreeWidget::editCurrentItem()
{
    if (!m_view->selectionModel()->selectedIndexes().isEmpty())
        m_view->edit(m_view->selectionModel()->selectedIndexes().first());
}

void ProjectTreeWidget::setCurrentItem(Node *node, Project *project)
{
    if (debug)
        qDebug() << "ProjectTreeWidget::setCurrentItem(" << (project ? project->name() : "0")
                 << ", " <<  (node ? node->path() : "0") << ")";
    if (!project) {
        m_view->selectionModel()->reset();
        return;
    }

    const QModelIndex mainIndex = m_model->indexForNode(node);

    if (!mainIndex.isValid()) {
        if (debug)
            qDebug() << "no main index, clear selection";
        m_view->selectionModel()->clearSelection();
    } else if (mainIndex != m_view->selectionModel()->currentIndex()) {
        QItemSelectionModel *selections = m_view->selectionModel();
        if (debug)
            qDebug() << "ProjectTreeWidget - changing selection";

        selections->setCurrentIndex(mainIndex, QItemSelectionModel::SelectCurrent
                                             | QItemSelectionModel::Clear);
        m_view->scrollTo(mainIndex);
    }
}

void ProjectTreeWidget::handleCurrentItemChange(const QModelIndex &current)
{
    Node *node = m_model->nodeForIndex(current);
    Q_ASSERT(node);

    bool autoSync = autoSynchronization();
    setAutoSynchronization(false);
    m_explorer->setCurrentNode(node);
    setAutoSynchronization(autoSync, false);
}

void ProjectTreeWidget::showContextMenu(const QPoint &pos)
{
    QModelIndex index = m_view->indexAt(pos);
    Node *node = m_model->nodeForIndex(index);
    m_explorer->showContextMenu(m_view->mapToGlobal(pos), node);
}

void ProjectTreeWidget::handleProjectAdded(ProjectExplorer::Project *project)
{
    Node *node = project->rootProjectNode();
    QModelIndex idx = m_model->indexForNode(node);
    m_view->setExpanded(idx, true);
}

void ProjectTreeWidget::startupProjectChanged(ProjectExplorer::Project *project)
{
    if (project) {
        ProjectNode *node = project->rootProjectNode();
        m_model->setStartupProject(node);
    } else {
        m_model->setStartupProject(0);
    }
}

void ProjectTreeWidget::initView()
{
    QModelIndex sessionIndex = m_model->index(0, 0);

    // hide root folder
    m_view->setRootIndex(sessionIndex);

    while (m_model->canFetchMore(sessionIndex))
        m_model->fetchMore(sessionIndex);

    // expand top level projects
    for (int i = 0; i < m_model->rowCount(sessionIndex); ++i) {
        m_view->expand(m_model->index(i, 0, sessionIndex));
    }

    setCurrentItem(m_explorer->currentNode(), m_explorer->currentProject());
}

void ProjectTreeWidget::openItem(const QModelIndex &mainIndex)
{
    Node *node = m_model->nodeForIndex(mainIndex);
    if (node->nodeType() == FileNodeType) {
        m_core->editorManager()->openEditor(node->path());
        m_core->editorManager()->ensureEditorManagerVisible();
    }
}

void ProjectTreeWidget::setProjectFilter(bool filter)
{
    m_model->setProjectFilterEnabled(filter);
    m_filterProjectsAction->setChecked(filter);
}

void ProjectTreeWidget::setGeneratedFilesFilter(bool filter)
{
    m_model->setGeneratedFilesFilterEnabled(filter);
    m_filterGeneratedFilesAction->setChecked(filter);
}

ProjectTreeWidgetFactory::ProjectTreeWidgetFactory(Core::ICore *core)
    : m_core(core)
{
}

ProjectTreeWidgetFactory::~ProjectTreeWidgetFactory()
{
}

QString ProjectTreeWidgetFactory::displayName()
{
    return tr("Projects");
}

QKeySequence ProjectTreeWidgetFactory::activationSequence()
{
    return QKeySequence(Qt::ALT + Qt::Key_X);
}

Core::NavigationView ProjectTreeWidgetFactory::createWidget()
{
    Core::NavigationView n;
    ProjectTreeWidget *ptw = new ProjectTreeWidget(m_core);
    n.widget = ptw;

    QToolButton *filter = new QToolButton;
    filter->setProperty("type", "dockbutton");
    filter->setIcon(QIcon(":/projectexplorer/images/filtericon.png"));
    filter->setToolTip(tr("Filter tree"));
    filter->setPopupMode(QToolButton::InstantPopup);
    QMenu *filterMenu = new QMenu(filter);
    filterMenu->addAction(ptw->m_filterProjectsAction);
    filterMenu->addAction(ptw->m_filterGeneratedFilesAction);
    filter->setMenu(filterMenu);

    QToolButton *toggleSync = new QToolButton;
    toggleSync->setProperty("type", "dockbutton");
    toggleSync->setIcon(QIcon(":/qworkbench/images/linkicon.png"));
    toggleSync->setCheckable(true);
    toggleSync->setChecked(ptw->autoSynchronization());
    toggleSync->setToolTip(tr("Synchronize with Editor"));
    connect(toggleSync, SIGNAL(clicked(bool)), ptw, SLOT(toggleAutoSynchronization()));

    n.doockToolBarWidgets << filter << toggleSync;
    return n;
}

