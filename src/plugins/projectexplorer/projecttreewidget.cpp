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

#include "projecttreewidget.h"

#include "projectexplorer.h"
#include "projectnodes.h"
#include "project.h"
#include "session.h"
#include "projectmodels.h"
#include "projecttree.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/find/itemviewfind.h>

#include <utils/algorithm.h>
#include <utils/navigationtreeview.h>
#include <utils/progressindicator.h>
#include <utils/tooltip/tooltip.h>
#include <utils/utilsicons.h>

#include <QApplication>
#include <QSettings>

#include <QStyledItemDelegate>
#include <QVBoxLayout>
#include <QToolButton>
#include <QPainter>
#include <QAction>
#include <QLineEdit>
#include <QMenu>

#include <memory>

using namespace Core;
using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;
using namespace Utils;

QList<ProjectTreeWidget *> ProjectTreeWidget::m_projectTreeWidgets;

namespace {

class ProjectTreeItemDelegate : public QStyledItemDelegate
{
public:
    ProjectTreeItemDelegate(QTreeView *view) : QStyledItemDelegate(view),
        m_view(view)
    {
        connect(m_view->model(), &QAbstractItemModel::modelReset,
                this, &ProjectTreeItemDelegate::deleteAllIndicators);

        // Actually this only needs to delete the indicators in the effected rows and *after* it,
        // but just be lazy and nuke all the indicators.
        connect(m_view->model(), &QAbstractItemModel::rowsAboutToBeRemoved,
                this, &ProjectTreeItemDelegate::deleteAllIndicators);
        connect(m_view->model(), &QAbstractItemModel::rowsAboutToBeInserted,
                this, &ProjectTreeItemDelegate::deleteAllIndicators);
    }

    ~ProjectTreeItemDelegate() override
    {
        deleteAllIndicators();
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QStyledItemDelegate::paint(painter, option, index);

        if (index.data(Project::isParsingRole).toBool()) {
            QStyleOptionViewItem opt = option;
            initStyleOption(&opt, index);
            ProgressIndicatorPainter *indicator = findOrCreateIndicatorPainter(index);

            QStyle *style = option.widget ? option.widget->style() : QApplication::style();
            const QRect rect = style->subElementRect(QStyle::SE_ItemViewItemDecoration, &opt, opt.widget);

            indicator->paint(*painter, rect);
        } else {
            delete m_indicators.value(index);
            m_indicators.remove(index);
        }
    }

private:
    ProgressIndicatorPainter *findOrCreateIndicatorPainter(const QModelIndex &index) const
    {
        ProgressIndicatorPainter *indicator = m_indicators.value(index);
        if (!indicator)
            indicator = createIndicatorPainter(index);
        return indicator;
    }

    ProgressIndicatorPainter *createIndicatorPainter(const QModelIndex &index) const
    {
        auto indicator = new ProgressIndicatorPainter(ProgressIndicatorSize::Small);
        indicator->setUpdateCallback([index, this]() { m_view->update(index); });
        indicator->startAnimation();
        m_indicators.insert(index, indicator);
        return indicator;
    }

    void deleteAllIndicators()
    {
        qDeleteAll(m_indicators);
        m_indicators.clear();
    }

    mutable QHash<QModelIndex, ProgressIndicatorPainter *> m_indicators;
    QTreeView *m_view;
};

bool debug = false;
}

class ProjectTreeView : public NavigationTreeView
{
public:
    ProjectTreeView()
    {
        setEditTriggers(QAbstractItemView::EditKeyPressed);
        setContextMenuPolicy(Qt::CustomContextMenu);
        setDragEnabled(true);
        setDragDropMode(QAbstractItemView::DragDrop);
        viewport()->setAcceptDrops(true);
        setDropIndicatorShown(true);
        auto context = new IContext(this);
        context->setContext(Context(ProjectExplorer::Constants::C_PROJECT_TREE));
        context->setWidget(this);

        ICore::addContextObject(context);

        connect(this, &ProjectTreeView::expanded,
                this, &ProjectTreeView::invalidateSize);
        connect(this, &ProjectTreeView::collapsed,
                this, &ProjectTreeView::invalidateSize);
    }

    void invalidateSize()
    {
        m_cachedSize = -1;
    }

    void setModel(QAbstractItemModel *newModel) override
    {
        // Note: Don't connect to column signals, as we have only one column
        if (model()) {
            QAbstractItemModel *m = model();
            disconnect(m, &QAbstractItemModel::dataChanged,
                       this, &ProjectTreeView::invalidateSize);
            disconnect(m, &QAbstractItemModel::layoutChanged,
                       this, &ProjectTreeView::invalidateSize);
            disconnect(m, &QAbstractItemModel::modelReset,
                       this, &ProjectTreeView::invalidateSize);
            disconnect(m, &QAbstractItemModel::rowsInserted,
                       this, &ProjectTreeView::invalidateSize);
            disconnect(m, &QAbstractItemModel::rowsMoved,
                       this, &ProjectTreeView::invalidateSize);
            disconnect(m, &QAbstractItemModel::rowsRemoved,
                       this, &ProjectTreeView::invalidateSize);
        }
        if (newModel) {
            connect(newModel, &QAbstractItemModel::dataChanged,
                    this, &ProjectTreeView::invalidateSize);
            connect(newModel, &QAbstractItemModel::layoutChanged,
                    this, &ProjectTreeView::invalidateSize);
            connect(newModel, &QAbstractItemModel::modelReset,
                    this, &ProjectTreeView::invalidateSize);
            connect(newModel, &QAbstractItemModel::rowsInserted,
                    this, &ProjectTreeView::invalidateSize);
            connect(newModel, &QAbstractItemModel::rowsMoved,
                    this, &ProjectTreeView::invalidateSize);
            connect(newModel, &QAbstractItemModel::rowsRemoved,
                    this, &ProjectTreeView::invalidateSize);
        }
        NavigationTreeView::setModel(newModel);
    }

    int sizeHintForColumn(int column) const override
    {
        if (m_cachedSize < 0)
            m_cachedSize = NavigationTreeView::sizeHintForColumn(column);

        return m_cachedSize;
    }

private:
    mutable int m_cachedSize = -1;
};

/*!
  /class ProjectTreeWidget

  Shows the projects in form of a tree.
  */
ProjectTreeWidget::ProjectTreeWidget(QWidget *parent) : QWidget(parent)
{
    // We keep one instance per tree as this also manages the
    // simple/non-simple etc state which is per tree.
    m_model = new FlatModel(this);
    m_view = new ProjectTreeView;
    m_view->setModel(m_model);
    m_view->setItemDelegate(new ProjectTreeItemDelegate(m_view));
    setFocusProxy(m_view);
    m_view->installEventFilter(this);

    auto layout = new QVBoxLayout();
    layout->addWidget(ItemViewFind::createSearchableWrapper(
                          m_view, ItemViewFind::DarkColored,
                          ItemViewFind::FetchMoreWhileSearching));
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);

    m_filterProjectsAction = new QAction(tr("Simplify Tree"), this);
    m_filterProjectsAction->setCheckable(true);
    m_filterProjectsAction->setChecked(false); // default is the traditional complex tree
    connect(m_filterProjectsAction, &QAction::toggled, this, &ProjectTreeWidget::setProjectFilter);

    m_filterGeneratedFilesAction = new QAction(tr("Hide Generated Files"), this);
    m_filterGeneratedFilesAction->setCheckable(true);
    m_filterGeneratedFilesAction->setChecked(true);
    connect(m_filterGeneratedFilesAction, &QAction::toggled,
            this, &ProjectTreeWidget::setGeneratedFilesFilter);

    m_filterDisabledFilesAction = new QAction(tr("Hide Disabled Files"), this);
    m_filterDisabledFilesAction->setCheckable(true);
    m_filterDisabledFilesAction->setChecked(false);
    connect(m_filterDisabledFilesAction, &QAction::toggled,
            this, &ProjectTreeWidget::setDisabledFilesFilter);

    const char focusActionId[] = "ProjectExplorer.FocusDocumentInProjectTree";
    if (!ActionManager::command(focusActionId)) {
        auto focusDocumentInProjectTree = new QAction(tr("Focus Document in Project Tree"), this);
        Command *cmd = ActionManager::registerAction(focusDocumentInProjectTree, focusActionId);
        cmd->setDefaultKeySequence(
            QKeySequence(useMacShortcuts ? tr("Meta+Shift+L") : tr("Alt+Shift+L")));
        connect(focusDocumentInProjectTree, &QAction::triggered, this, [this]() {
            syncFromDocumentManager();
        });
    }

    m_trimEmptyDirectoriesAction = new QAction(tr("Hide Empty Directories"), this);
    m_trimEmptyDirectoriesAction->setCheckable(true);
    m_trimEmptyDirectoriesAction->setChecked(true);
    connect(m_trimEmptyDirectoriesAction, &QAction::toggled,
            this, &ProjectTreeWidget::setTrimEmptyDirectories);

    // connections
    connect(m_model, &FlatModel::renamed,
            this, &ProjectTreeWidget::renamed);
    connect(m_model, &FlatModel::requestExpansion,
            m_view, &QTreeView::expand);
    connect(m_view, &QAbstractItemView::activated,
            this, &ProjectTreeWidget::openItem);
    connect(m_view->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &ProjectTreeWidget::handleCurrentItemChange);
    connect(m_view, &QWidget::customContextMenuRequested,
            this, &ProjectTreeWidget::showContextMenu);
    connect(m_view, &QTreeView::expanded,
            m_model, &FlatModel::onExpanded);
    connect(m_view, &QTreeView::collapsed,
            m_model, &FlatModel::onCollapsed);

    m_toggleSync = new QToolButton(this);
    m_toggleSync->setIcon(Icons::LINK_TOOLBAR.icon());
    m_toggleSync->setCheckable(true);
    m_toggleSync->setChecked(autoSynchronization());
    m_toggleSync->setToolTip(tr("Synchronize with Editor"));
    connect(m_toggleSync, &QAbstractButton::clicked,
            this, &ProjectTreeWidget::toggleAutoSynchronization);

    setCurrentItem(ProjectTree::currentNode());
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
    if (m_delayedRename.isEmpty())
        return;
    Node *node = m_model->nodeForIndex(parent);
    QTC_ASSERT(node, return);
    for (int i = start; i <= end && !m_delayedRename.isEmpty(); ++i) {
        QModelIndex idx = m_model->index(i, 0, parent);
        Node *n = m_model->nodeForIndex(idx);
        if (!n)
            continue;
        const int renameIdx = m_delayedRename.indexOf(n->filePath());
        if (renameIdx != -1) {
            m_view->setCurrentIndex(idx);
            m_delayedRename.removeAt(renameIdx);
        }
    }
}

Node *ProjectTreeWidget::nodeForFile(const FilePath &fileName)
{
    if (fileName.isEmpty())
        return nullptr;
    Node *bestNode = nullptr;
    int bestNodeExpandCount = INT_MAX;

    // FIXME: Check that the values used make sense in the context.
    auto priority = [](Node *node) {
        if (node->asFileNode())
            return 1;
        if (node->isFolderNodeType())
            return 2;
        if (node->isVirtualFolderType())
            return 3;
        if (node->isProjectNodeType())
            return 4;
        QTC_CHECK(false);
        return 1;
    };

    // FIXME: Looks like this could be done with less cycles.
    for (Project *project : SessionManager::projects()) {
        if (ProjectNode *projectNode = project->rootProjectNode()) {
            projectNode->forEachGenericNode([&](Node *node) {
                if (node->filePath() == fileName) {
                    if (!bestNode || priority(node) < priority(bestNode)) {
                        bestNode = node;
                        bestNodeExpandCount = ProjectTreeWidget::expandedCount(node);
                    } else if (priority(node) == priority(bestNode)) {
                        int nodeExpandCount = ProjectTreeWidget::expandedCount(node);
                        if (nodeExpandCount < bestNodeExpandCount) {
                            bestNode = node;
                            bestNodeExpandCount = ProjectTreeWidget::expandedCount(node);
                        }
                    }
                }
            });
        }
    }

    return bestNode;
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

    if (m_autoSync)
        syncFromDocumentManager();
}

void ProjectTreeWidget::expandNodeRecursively(const QModelIndex &index)
{
    const int rc = index.model()->rowCount(index);
    for (int i = 0; i < rc; ++i)
        expandNodeRecursively(index.model()->index(i, index.column(), index));
    if (rc > 0)
        m_view->expand(index);
}

void ProjectTreeWidget::expandCurrentNodeRecursively()
{
    expandNodeRecursively(m_view->currentIndex());
}

void ProjectTreeWidget::collapseAll()
{
    m_view->collapseAll();
}

void ProjectTreeWidget::expandAll()
{
    m_view->expandAll();
}

void ProjectTreeWidget::editCurrentItem()
{
    m_delayedRename.clear();
    const QModelIndex currentIndex = m_view->selectionModel()->currentIndex();
    if (!currentIndex.isValid())
        return;

    m_view->edit(currentIndex);
    // Select complete file basename for renaming
    const Node *node = m_model->nodeForIndex(currentIndex);
    if (!node)
        return;
    auto *editor = qobject_cast<QLineEdit*>(m_view->indexWidget(currentIndex));
    if (!editor)
        return;

    const QString text = editor->text();
    const int dotIndex = text.lastIndexOf(QLatin1Char('.'));
    if (dotIndex > 0)
        editor->setSelection(0, dotIndex);
}

void ProjectTreeWidget::renamed(const FilePath &oldPath, const FilePath &newPath)
{
    update();
    Q_UNUSED(oldPath)
    if (!currentNode() || currentNode()->filePath() != newPath) {
        // try to find the node
        Node *node = nodeForFile(newPath);
        if (node)
            m_view->setCurrentIndex(m_model->indexForNode(node));
        else
            m_delayedRename << newPath;
    }
}

void ProjectTreeWidget::syncFromDocumentManager()
{
    // sync from document manager
    FilePath fileName;
    if (IDocument *doc = EditorManager::currentDocument())
        fileName = doc->filePath();
    if (!currentNode() || currentNode()->filePath() != fileName)
        setCurrentItem(ProjectTreeWidget::nodeForFile(fileName));
}

void ProjectTreeWidget::setCurrentItem(Node *node)
{
    const QModelIndex mainIndex = m_model->indexForNode(node);

    if (mainIndex.isValid()) {
        if (mainIndex != m_view->selectionModel()->currentIndex()) {
            // Expand everything between the index and the root index!
            QModelIndex parent = mainIndex.parent();
            while (parent.isValid()) {
                m_view->setExpanded(parent, true);
                parent = parent.parent();
            }
            m_view->setCurrentIndex(mainIndex);
            m_view->scrollTo(mainIndex);
        }
    } else {
        m_view->clearSelection();
    }
}

void ProjectTreeWidget::handleCurrentItemChange(const QModelIndex &current)
{
    Q_UNUSED(current)
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

void ProjectTreeWidget::showMessage(Node *node, const QString &message)
{
    QModelIndex idx = m_model->indexForNode(node);
    m_view->setCurrentIndex(idx);
    m_view->scrollTo(idx);

    QPoint pos = m_view->mapToGlobal(m_view->visualRect(idx).bottomLeft());
    pos -= ToolTip::offsetFromPosition();
    ToolTip::show(pos, message);
}

void ProjectTreeWidget::showContextMenu(const QPoint &pos)
{
    QModelIndex index = m_view->indexAt(pos);
    Node *node = m_model->nodeForIndex(index);
    ProjectTree::showContextMenu(this, m_view->mapToGlobal(pos), node);
}

void ProjectTreeWidget::openItem(const QModelIndex &mainIndex)
{
    Node *node = m_model->nodeForIndex(mainIndex);
    if (!node || !node->asFileNode())
        return;
    IEditor *editor = EditorManager::openEditor(node->filePath().toString());
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

void ProjectTreeWidget::setDisabledFilesFilter(bool filter)
{
    m_model->setDisabledFilesFilterEnabled(filter);
    m_filterDisabledFilesAction->setChecked(filter);
}

void ProjectTreeWidget::setTrimEmptyDirectories(bool filter)
{
    m_model->setTrimEmptyDirectories(filter);
    m_trimEmptyDirectoriesAction->setChecked(filter);
}

bool ProjectTreeWidget::generatedFilesFilter()
{
    return m_model->generatedFilesFilterEnabled();
}

bool ProjectTreeWidget::disabledFilesFilter()
{
    return m_model->disabledFilesFilterEnabled();
}

bool ProjectTreeWidget::trimEmptyDirectoriesFilter()
{
    return m_model->trimEmptyDirectoriesEnabled();
}

bool ProjectTreeWidget::projectFilter()
{
    return m_model->projectFilterEnabled();
}


ProjectTreeWidgetFactory::ProjectTreeWidgetFactory()
{
    setDisplayName(tr("Projects"));
    setPriority(100);
    setId(ProjectExplorer::Constants::PROJECTTREE_ID);
    setActivationSequence(QKeySequence(useMacShortcuts ? tr("Meta+X") : tr("Alt+X")));
}

NavigationView ProjectTreeWidgetFactory::createWidget()
{
    NavigationView n;
    auto ptw = new ProjectTreeWidget;
    n.widget = ptw;

    auto filter = new QToolButton(ptw);
    filter->setIcon(Icons::FILTER.icon());
    filter->setToolTip(tr("Filter Tree"));
    filter->setPopupMode(QToolButton::InstantPopup);
    filter->setProperty("noArrow", true);
    auto filterMenu = new QMenu(filter);
    filterMenu->addAction(ptw->m_filterProjectsAction);
    filterMenu->addAction(ptw->m_filterGeneratedFilesAction);
    filterMenu->addAction(ptw->m_filterDisabledFilesAction);
    filterMenu->addAction(ptw->m_trimEmptyDirectoriesAction);
    filter->setMenu(filterMenu);

    n.dockToolBarWidgets << filter << ptw->toggleSync();
    return n;
}

void ProjectTreeWidgetFactory::saveSettings(QSettings *settings, int position, QWidget *widget)
{
    auto ptw = qobject_cast<ProjectTreeWidget *>(widget);
    Q_ASSERT(ptw);
    const QString baseKey = QLatin1String("ProjectTreeWidget.") + QString::number(position);
    settings->setValue(baseKey + QLatin1String(".ProjectFilter"), ptw->projectFilter());
    settings->setValue(baseKey + QLatin1String(".GeneratedFilter"), ptw->generatedFilesFilter());
    settings->setValue(baseKey + ".DisabledFilesFilter", ptw->disabledFilesFilter());
    settings->setValue(baseKey + QLatin1String(".TrimEmptyDirsFilter"), ptw->trimEmptyDirectoriesFilter());
    settings->setValue(baseKey + QLatin1String(".SyncWithEditor"), ptw->autoSynchronization());
}

void ProjectTreeWidgetFactory::restoreSettings(QSettings *settings, int position, QWidget *widget)
{
    auto ptw = qobject_cast<ProjectTreeWidget *>(widget);
    Q_ASSERT(ptw);
    const QString baseKey = QLatin1String("ProjectTreeWidget.") + QString::number(position);
    ptw->setProjectFilter(settings->value(baseKey + QLatin1String(".ProjectFilter"), false).toBool());
    ptw->setGeneratedFilesFilter(settings->value(baseKey + QLatin1String(".GeneratedFilter"), true).toBool());
    ptw->setDisabledFilesFilter(settings->value(baseKey + ".DisabledFilesFilter", false).toBool());
    ptw->setTrimEmptyDirectories(settings->value(baseKey + QLatin1String(".TrimEmptyDirsFilter"), true).toBool());
    ptw->setAutoSynchronization(settings->value(baseKey +  QLatin1String(".SyncWithEditor"), true).toBool());
}
