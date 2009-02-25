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

#include "projecttreewidget.h"

#include "projectexplorer.h"
#include "projectexplorerconstants.h"
#include "projectmodels.h"

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QSettings>

#include <QtGui/QHeaderView>
#include <QtGui/QVBoxLayout>
#include <QtGui/QToolButton>
#include <QtGui/QFocusEvent>
#include <QtGui/QPalette>

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
            treeHeader->setResizeMode(QHeaderView::Stretch);
            treeHeader->setStretchLastSection(true);
        }
        setContextMenuPolicy(Qt::CustomContextMenu);
        setUniformRowHeights(true);
        setTextElideMode(Qt::ElideNone);
//        setExpandsOnDoubleClick(false);
        setAttribute(Qt::WA_MacShowFocusRect, false);
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

#ifdef Q_OS_MAC
    void keyPressEvent(QKeyEvent *event)
    {
        if ((event->key() == Qt::Key_Return
                || event->key() == Qt::Key_Enter)
                && event->modifiers() == 0
                && currentIndex().isValid()) {
            emit activated(currentIndex());
            return;
        }
        QTreeView::keyPressEvent(event);
    }
#endif
};

/*!
  /class ProjectTreeWidget

  Shows the projects in form of a tree.
  */
ProjectTreeWidget::ProjectTreeWidget(QWidget *parent)
        : QWidget(parent),
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

    m_toggleSync = new QToolButton;
    m_toggleSync->setProperty("type", "dockbutton");
    m_toggleSync->setIcon(QIcon(":/core/images/linkicon.png"));
    m_toggleSync->setCheckable(true);
    m_toggleSync->setChecked(autoSynchronization());
    m_toggleSync->setToolTip(tr("Synchronize with Editor"));
    connect(m_toggleSync, SIGNAL(clicked(bool)), this, SLOT(toggleAutoSynchronization()));

    //setAutoSynchronization(true);
}

QToolButton *ProjectTreeWidget::toggleSync()
{
    return m_toggleSync;
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
    m_toggleSync->setChecked(sync);
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
    // node might be 0. that's okay
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
    for (int i = 0; i < m_model->rowCount(sessionIndex); ++i)
        m_view->expand(m_model->index(i, 0, sessionIndex));

    setCurrentItem(m_explorer->currentNode(), m_explorer->currentProject());
}

void ProjectTreeWidget::openItem(const QModelIndex &mainIndex)
{
    Node *node = m_model->nodeForIndex(mainIndex);
    if (node->nodeType() == FileNodeType) {
        Core::EditorManager *editorManager = Core::EditorManager::instance();
        editorManager->openEditor(node->path());
        editorManager->ensureEditorManagerVisible();
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

bool ProjectTreeWidget::generatedFilesFilter()
{
    return m_model->generatedFilesFilterEnabled();
}

bool ProjectTreeWidget::projectFilter()
{
    return m_model->projectFilterEnabled();
}


ProjectTreeWidgetFactory::ProjectTreeWidgetFactory()
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
    ProjectTreeWidget *ptw = new ProjectTreeWidget;
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

    n.doockToolBarWidgets << filter << ptw->toggleSync();
    return n;
}

void ProjectTreeWidgetFactory::saveSettings(int position, QWidget *widget)
{
    ProjectTreeWidget *ptw = qobject_cast<ProjectTreeWidget *>(widget);
    Q_ASSERT(ptw);
    QSettings *settings = Core::ICore::instance()->settings();
    settings->setValue("ProjectTreeWidget."+QString::number(position)+".ProjectFilter", ptw->projectFilter());
    settings->setValue("ProjectTreeWidget."+QString::number(position)+".GeneratedFilter", ptw->generatedFilesFilter());
    settings->setValue("ProjectTreeWidget."+QString::number(position)+".SyncWithEditor", ptw->autoSynchronization());
}

void ProjectTreeWidgetFactory::restoreSettings(int position, QWidget *widget)
{
    ProjectTreeWidget *ptw = qobject_cast<ProjectTreeWidget *>(widget);
    Q_ASSERT(ptw);
    QSettings *settings = Core::ICore::instance()->settings();
    ptw->setProjectFilter(settings->value("ProjectTreeWidget."+QString::number(position)+".ProjectFilter", false).toBool());
    ptw->setGeneratedFilesFilter(settings->value("ProjectTreeWidget."+QString::number(position)+".GeneratedFilter", true).toBool());
    ptw->setAutoSynchronization(settings->value("ProjectTreeWidget."+QString::number(position)+".SyncWithEditor", true).toBool());
}
