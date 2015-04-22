/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "projecttreewidget.h"

#include "projectexplorer.h"
#include "projectnodes.h"
#include "project.h"
#include "session.h"
#include "projectmodels.h"
#include "projecttree.h"

#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/find/itemviewfind.h>

#include <utils/navigationtreeview.h>
#include <utils/algorithm.h>

#include <QDebug>
#include <QSettings>

#include <QStyledItemDelegate>
#include <QVBoxLayout>
#include <QToolButton>
#include <QAction>
#include <QMenu>

using namespace Core;
using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

QList<ProjectTreeWidget *> ProjectTreeWidget::m_projectTreeWidgets;

namespace {

class ProjectTreeItemDelegate : public QStyledItemDelegate
{
public:
    ProjectTreeItemDelegate(QObject *parent) : QStyledItemDelegate(parent)
    { }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        QStyleOptionViewItem opt = option;
        if (!index.data(Project::EnabledRole).toBool())
            opt.state &= ~QStyle::State_Enabled;
        QStyledItemDelegate::paint(painter, opt, index);
    }
};

bool debug = false;
}

class ProjectTreeView : public Utils::NavigationTreeView
{
public:
    ProjectTreeView()
    {
        setEditTriggers(QAbstractItemView::EditKeyPressed);
        setContextMenuPolicy(Qt::CustomContextMenu);
        setDragEnabled(true);
        setDragDropMode(QAbstractItemView::DragOnly);
        m_context = new IContext(this);
        m_context->setContext(Context(ProjectExplorer::Constants::C_PROJECT_TREE));
        m_context->setWidget(this);
        ICore::addContextObject(m_context);
    }
    ~ProjectTreeView()
    {
        ICore::removeContextObject(m_context);
        delete m_context;
    }

private:
    IContext *m_context;
};

/*!
  /class ProjectTreeWidget

  Shows the projects in form of a tree.
  */
ProjectTreeWidget::ProjectTreeWidget(QWidget *parent)
        : QWidget(parent),
          m_view(0),
          m_model(0),
          m_filterProjectsAction(0),
          m_autoSync(false),
          m_autoExpand(true)
{
    m_model = new FlatModel(SessionManager::sessionNode(), this);
    Project *pro = SessionManager::startupProject();
    if (pro)
        m_model->setStartupProject(pro->rootProjectNode());

    m_view = new ProjectTreeView;
    m_view->setModel(m_model);
    m_view->setItemDelegate(new ProjectTreeItemDelegate(this));
    setFocusProxy(m_view);
    m_view->installEventFilter(this);
    initView();

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(ItemViewFind::createSearchableWrapper(
                          m_view, ItemViewFind::DarkColored,
                          ItemViewFind::FetchMoreWhileSearching));
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);

    m_filterProjectsAction = new QAction(tr("Simplify Tree"), this);
    m_filterProjectsAction->setCheckable(true);
    m_filterProjectsAction->setChecked(false); // default is the traditional complex tree
    connect(m_filterProjectsAction, SIGNAL(toggled(bool)), this, SLOT(setProjectFilter(bool)));

    m_filterGeneratedFilesAction = new QAction(tr("Hide Generated Files"), this);
    m_filterGeneratedFilesAction->setCheckable(true);
    m_filterGeneratedFilesAction->setChecked(true);
    connect(m_filterGeneratedFilesAction, SIGNAL(toggled(bool)), this, SLOT(setGeneratedFilesFilter(bool)));

    // connections
    connect(m_model, SIGNAL(modelReset()),
            this, SLOT(initView()));
    connect(m_model, &FlatModel::renamed,
            this, &ProjectTreeWidget::renamed);
    connect(m_view, SIGNAL(activated(QModelIndex)),
            this, SLOT(openItem(QModelIndex)));
    connect(m_view->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this, SLOT(handleCurrentItemChange(QModelIndex)));
    connect(m_view, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(showContextMenu(QPoint)));

    QObject *sessionManager = SessionManager::instance();
    connect(sessionManager, SIGNAL(singleProjectAdded(ProjectExplorer::Project*)),
            this, SLOT(handleProjectAdded(ProjectExplorer::Project*)));
    connect(sessionManager, SIGNAL(startupProjectChanged(ProjectExplorer::Project*)),
            this, SLOT(startupProjectChanged(ProjectExplorer::Project*)));

    connect(sessionManager, SIGNAL(aboutToLoadSession(QString)),
            this, SLOT(disableAutoExpand()));
    connect(sessionManager, SIGNAL(sessionLoaded(QString)),
            this, SLOT(loadExpandData()));
    connect(sessionManager, SIGNAL(aboutToSaveSession()),
            this, SLOT(saveExpandData()));

    m_toggleSync = new QToolButton;
    m_toggleSync->setIcon(QIcon(QLatin1String(Core::Constants::ICON_LINK)));
    m_toggleSync->setCheckable(true);
    m_toggleSync->setChecked(autoSynchronization());
    m_toggleSync->setToolTip(tr("Synchronize with Editor"));
    connect(m_toggleSync, SIGNAL(clicked(bool)), this, SLOT(toggleAutoSynchronization()));

    setAutoSynchronization(true);

    m_projectTreeWidgets << this;
    ProjectTree::registerWidget(this);
}

ProjectTreeWidget::~ProjectTreeWidget()
{
    m_projectTreeWidgets.removeOne(this);
    ProjectTree::unregisterWidget(this);
}

// returns how many nodes need to be expanded to make node visible
int ProjectTreeWidget::expandedCount(Node *node)
{
    if (m_projectTreeWidgets.isEmpty())
        return 0;
    FlatModel *model = m_projectTreeWidgets.first()->m_model;
    QModelIndex index = model->indexForNode(node);
    if (!index.isValid())
        return 0;

    int count = 0;
    foreach (ProjectTreeWidget *tree, m_projectTreeWidgets) {
        QModelIndex idx = index;
        while (idx.isValid() && idx != tree->m_view->rootIndex()) {
            if (!tree->m_view->isExpanded(idx))
                ++count;
            idx = model->parent(idx);
        }
    }
    return count;
}

void ProjectTreeWidget::rowsInserted(const QModelIndex &parent, int start, int end)
{
    Node *node = m_model->nodeForIndex(parent);
    const QString path = node->path().toString();
    const QString displayName = node->displayName();

    auto it = m_toExpand.find(ExpandData(path, displayName));
    if (it != m_toExpand.end()) {
        m_view->expand(parent);
        m_toExpand.erase(it);
    }
    int i = start;
    while (i <= end) {
        QModelIndex idx = m_model->index(i, 0, parent);
        Node *n = m_model->nodeForIndex(idx);
        if (n && n->path() == m_delayedRename) {
            m_view->setCurrentIndex(idx);
            m_delayedRename.clear();
            break;
        }
        ++i;
    }
}

Node *ProjectTreeWidget::nodeForFile(const Utils::FileName &fileName)
{
    return mostExpandedNode(SessionManager::nodesForFile(fileName));
}

Node *ProjectTreeWidget::mostExpandedNode(const QList<Node *> &nodes)
{
    Node *bestNode = 0;
    int bestNodeExpandCount = INT_MAX;

    foreach (Node *node, nodes) {
        if (!bestNode) {
            bestNode = node;
            bestNodeExpandCount = ProjectTreeWidget::expandedCount(node);
        } else if (node->nodeType() < bestNode->nodeType()) {
            bestNode = node;
            bestNodeExpandCount = ProjectTreeWidget::expandedCount(node);
        } else if (node->nodeType() == bestNode->nodeType()) {
            int nodeExpandCount = ProjectTreeWidget::expandedCount(node);
            if (nodeExpandCount < bestNodeExpandCount) {
                bestNode = node;
                bestNodeExpandCount = ProjectTreeWidget::expandedCount(node);
            }
        }
    }
    return bestNode;
}

void ProjectTreeWidget::disableAutoExpand()
{
    m_autoExpand = false;
}

void ProjectTreeWidget::loadExpandData()
{
    m_autoExpand = true;
    QList<QVariant> data = SessionManager::value(QLatin1String("ProjectTree.ExpandData")).value<QList<QVariant>>();
    QSet<ExpandData> set = Utils::transform<QSet>(data, [](const QVariant &v) {
        QStringList list = v.toStringList();
        if (list.size() != 2)
            return ExpandData();
        return ExpandData(list.at(0), list.at(1));
    });

    set.remove(ExpandData());

    recursiveLoadExpandData(m_view->rootIndex(), set);

    // store remaning nodes to expand
    m_toExpand = set;
}

void ProjectTreeWidget::recursiveLoadExpandData(const QModelIndex &index, QSet<ExpandData> &data)
{
    Node *node = m_model->nodeForIndex(index);
    const QString path = node->path().toString();
    const QString displayName = node->displayName();
    auto it = data.find(ExpandData(path, displayName));
    if (it != data.end()) {
        m_view->expand(index);
        data.erase(it);
        int count = m_model->rowCount(index);
        for (int i = 0; i < count; ++i)
            recursiveLoadExpandData(index.child(i, 0), data);
    }
}

void ProjectTreeWidget::saveExpandData()
{
    QList<QVariant> data;
    recursiveSaveExpandData(m_view->rootIndex(), &data);
    // TODO if there are multiple ProjectTreeWidgets, the last one saves the data
    SessionManager::setValue(QLatin1String("ProjectTree.ExpandData"), data);
}

void ProjectTreeWidget::recursiveSaveExpandData(const QModelIndex &index, QList<QVariant> *data)
{
    Q_ASSERT(data);
    if (m_view->isExpanded(index) || index == m_view->rootIndex()) {
        // Note: We store the path+displayname of the node, which isn't unique for e.g. .pri files
        // but works for most nodes
        Node *node = m_model->nodeForIndex(index);
        const QStringList &list = ExpandData(node->path().toString(), node->displayName()).toStringList();
        data->append(QVariant::fromValue(list));
        int count = m_model->rowCount(index);
        for (int i = 0; i < count; ++i)
            recursiveSaveExpandData(index.child(i, 0), data);
    }
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

void ProjectTreeWidget::setAutoSynchronization(bool sync)
{
    m_toggleSync->setChecked(sync);
    if (sync == m_autoSync)
        return;

    m_autoSync = sync;

    if (debug)
        qDebug() << (m_autoSync ? "Enabling auto synchronization" : "Disabling auto synchronization");

    if (m_autoSync) {
        // sync from document manager
        Utils::FileName fileName;
        if (IDocument *doc = EditorManager::currentDocument())
            fileName = doc->filePath();
        if (!currentNode() || currentNode()->path() != fileName)
            setCurrentItem(ProjectTreeWidget::nodeForFile(fileName));
    }
}

void ProjectTreeWidget::collapseAll()
{
    m_view->collapseAll();
}

void ProjectTreeWidget::editCurrentItem()
{
    m_delayedRename.clear();
    if (m_view->selectionModel()->currentIndex().isValid())
        m_view->edit(m_view->selectionModel()->currentIndex());
}


void ProjectTreeWidget::renamed(const Utils::FileName &oldPath, const Utils::FileName &newPath)
{
    Q_UNUSED(oldPath);
    if (!currentNode() || currentNode()->path() != newPath) {
        // try to find the node
        Node *node = nodeForFile(newPath);
        if (node)
            m_view->setCurrentIndex(m_model->indexForNode(node));
        else
            m_delayedRename = newPath;
    }
}

void ProjectTreeWidget::setCurrentItem(Node *node)
{
    const QModelIndex mainIndex = m_model->indexForNode(node);

    if (mainIndex.isValid()) {
        if (mainIndex != m_view->selectionModel()->currentIndex()) {
            m_view->setCurrentIndex(mainIndex);
            m_view->scrollTo(mainIndex);
        }
    } else {
        m_view->clearSelection();
    }

}

void ProjectTreeWidget::handleCurrentItemChange(const QModelIndex &current)
{
    Q_UNUSED(current);
    ProjectTree::nodeChanged(this);
}

Node *ProjectTreeWidget::currentNode()
{
    return m_model->nodeForIndex(m_view->currentIndex());
}

void ProjectTreeWidget::sync(Node *node)
{
    if (m_autoSync)
        setCurrentItem(node);
}

void ProjectTreeWidget::showContextMenu(const QPoint &pos)
{
    QModelIndex index = m_view->indexAt(pos);
    Node *node = m_model->nodeForIndex(index);
    ProjectTree::showContextMenu(this, m_view->mapToGlobal(pos), node);
}

void ProjectTreeWidget::handleProjectAdded(Project *project)
{
    Node *node = project->rootProjectNode();
    QModelIndex idx = m_model->indexForNode(node);
    if (m_autoExpand) // disabled while session restoring
        m_view->setExpanded(idx, true);
    m_view->setCurrentIndex(idx);

    connect(m_model, &FlatModel::rowsInserted,
            this, &ProjectTreeWidget::rowsInserted);
}

void ProjectTreeWidget::startupProjectChanged(Project *project)
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

    setCurrentItem(ProjectTree::currentNode());
}

void ProjectTreeWidget::openItem(const QModelIndex &mainIndex)
{
    Node *node = m_model->nodeForIndex(mainIndex);
    if (node->nodeType() != FileNodeType)
        return;
    IEditor *editor = EditorManager::openEditor(node->path().toString());
    if (editor && node->line() >= 0)
        editor->gotoLine(node->line());
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
    setDisplayName(tr("Projects"));
    setPriority(100);
    setId("Projects");
    setActivationSequence(QKeySequence(UseMacShortcuts ? tr("Meta+X") : tr("Alt+X")));
}

NavigationView ProjectTreeWidgetFactory::createWidget()
{
    NavigationView n;
    ProjectTreeWidget *ptw = new ProjectTreeWidget;
    n.widget = ptw;

    QToolButton *filter = new QToolButton;
    filter->setIcon(QIcon(QLatin1String(Core::Constants::ICON_FILTER)));
    filter->setToolTip(tr("Filter Tree"));
    filter->setPopupMode(QToolButton::InstantPopup);
    filter->setProperty("noArrow", true);
    QMenu *filterMenu = new QMenu(filter);
    filterMenu->addAction(ptw->m_filterProjectsAction);
    filterMenu->addAction(ptw->m_filterGeneratedFilesAction);
    filter->setMenu(filterMenu);

    n.dockToolBarWidgets << filter << ptw->toggleSync();
    return n;
}

void ProjectTreeWidgetFactory::saveSettings(int position, QWidget *widget)
{
    ProjectTreeWidget *ptw = qobject_cast<ProjectTreeWidget *>(widget);
    Q_ASSERT(ptw);
    QSettings *settings = ICore::settings();
    const QString baseKey = QLatin1String("ProjectTreeWidget.") + QString::number(position);
    settings->setValue(baseKey + QLatin1String(".ProjectFilter"), ptw->projectFilter());
    settings->setValue(baseKey + QLatin1String(".GeneratedFilter"), ptw->generatedFilesFilter());
    settings->setValue(baseKey + QLatin1String(".SyncWithEditor"), ptw->autoSynchronization());
}

void ProjectTreeWidgetFactory::restoreSettings(int position, QWidget *widget)
{
    ProjectTreeWidget *ptw = qobject_cast<ProjectTreeWidget *>(widget);
    Q_ASSERT(ptw);
    QSettings *settings = ICore::settings();
    const QString baseKey = QLatin1String("ProjectTreeWidget.") + QString::number(position);
    ptw->setProjectFilter(settings->value(baseKey + QLatin1String(".ProjectFilter"), false).toBool());
    ptw->setGeneratedFilesFilter(settings->value(baseKey + QLatin1String(".GeneratedFilter"), true).toBool());
    ptw->setAutoSynchronization(settings->value(baseKey +  QLatin1String(".SyncWithEditor")).toBool());
}
