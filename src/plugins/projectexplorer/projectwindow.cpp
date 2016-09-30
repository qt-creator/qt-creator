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

#include "projectwindow.h"

#include "kit.h"
#include "kitmanager.h"
#include "panelswidget.h"
#include "project.h"
#include "projectexplorer.h"
#include "projectpanelfactory.h"
#include "session.h"
#include "target.h"
#include "targetsettingspanel.h"

#include <coreplugin/coreicons.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>

#include <utils/navigationtreeview.h>
#include <utils/qtcassert.h>
#include <utils/styledbar.h>
#include <utils/treemodel.h>
#include <utils/basetreeview.h>

#include <QApplication>
#include <QComboBox>
#include <QDockWidget>
#include <QHeaderView>
#include <QMenu>
#include <QStyledItemDelegate>
#include <QTimer>
#include <QTreeView>
#include <QVBoxLayout>

using namespace Core;
using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

class MiscSettingsGroupItem;
class RootItem;

// Standard third level for the generic case: i.e. all except for the Build/Run page
class MiscSettingsPanelItem : public TreeItem // TypedTreeItem<TreeItem, MiscSettingsGroupItem>
{
public:
    MiscSettingsPanelItem(ProjectPanelFactory *factory, Project *project)
        : m_factory(factory), m_project(project)
    {}

    ~MiscSettingsPanelItem() { delete m_widget; }

    QVariant data(int column, int role) const override;
    Qt::ItemFlags flags(int column) const override;
    bool setData(int column, const QVariant &, int role) override;

protected:
    ProjectPanelFactory *m_factory = nullptr;
    QPointer<Project> m_project;

    mutable QPointer<QWidget> m_widget = nullptr;
};

QVariant MiscSettingsPanelItem::data(int column, int role) const
{
    Q_UNUSED(column)
    if (role == Qt::DisplayRole) {
        if (m_factory)
            return m_factory->displayName();
    }

    if (role == PanelWidgetRole) {
        if (!m_widget) {
            auto panelsWidget = new PanelsWidget;
            QWidget *widget = m_factory->createWidget(m_project);
            panelsWidget->addPropertiesPanel(m_factory->displayName(),
                                             QIcon(m_factory->icon()),
                                             widget);
            panelsWidget->setFocusProxy(widget);
            m_widget = panelsWidget;
        }

        return QVariant::fromValue<QWidget *>(m_widget.data());
    }

    if (role == ActiveItemRole)  // We are the active one.
        return QVariant::fromValue<TreeItem *>(const_cast<MiscSettingsPanelItem *>(this));

    return QVariant();
}

Qt::ItemFlags MiscSettingsPanelItem::flags(int column) const
{
    if (m_factory && m_project) {
        if (!m_factory->supports(m_project))
            return Qt::ItemIsSelectable;
    }
    return TreeItem::flags(column);
}

bool MiscSettingsPanelItem::setData(int column, const QVariant &, int role)
{
    if (role == ItemActivatedDirectlyRole) {
        // Bubble up
        return parent()->setData(column, QVariant::fromValue(static_cast<TreeItem *>(this)),
                                 ItemActivatedFromBelowRole);
    }

    return false;
}

// The lower part of the second tree level, i.e. the project settings list.
// The upper part is the TargetSettingsPanelItem .
class MiscSettingsGroupItem : public TreeItem // TypedTreeItem<MiscSettingsPanelItem, ProjectItem>
{
public:
    explicit MiscSettingsGroupItem(Project *project)
        : m_project(project)
    {
        QTC_ASSERT(m_project, return);
        foreach (ProjectPanelFactory *factory, ProjectPanelFactory::factories())
            appendChild(new MiscSettingsPanelItem(factory, project));
    }

    Qt::ItemFlags flags(int) const override
    {
        return Qt::ItemIsEnabled;
    }

    QVariant data(int column, int role) const override
    {
        switch (role) {
        case Qt::DisplayRole:
            return ProjectWindow::tr("Project Settings");

        case PanelWidgetRole:
        case ActiveItemRole:
            if (0 <= m_currentPanelIndex && m_currentPanelIndex < childCount())
                return childAt(m_currentPanelIndex)->data(column, role);
        }
        return QVariant();
    }

    bool setData(int column, const QVariant &data, int role) override
    {
        Q_UNUSED(column)

        if (role == ItemActivatedFromBelowRole) {
            TreeItem *item = data.value<TreeItem *>();
            QTC_ASSERT(item, return false);
            m_currentPanelIndex = children().indexOf(item);
            QTC_ASSERT(m_currentPanelIndex != -1, return false);
            parent()->setData(0, QVariant::fromValue(static_cast<TreeItem *>(this)),
                              ItemActivatedFromBelowRole);
            return true;
        }

        if (role == ItemActivatedDirectlyRole) {
            m_currentPanelIndex = 0; // Use the first ('Editor') page.
            parent()->setData(0, QVariant::fromValue(static_cast<TreeItem *>(this)),
                              ItemActivatedFromBelowRole);
            return true;
        }

        return false;
    }

    Project *project() const { return m_project; }

private:
    int m_currentPanelIndex = -1;

    Project * const m_project;
};

// The first tree level, i.e. projects (nowadays in the combobox in the top bar...)
class ProjectItem : public TreeItem // TypedTreeItem<TreeItem, RootItem>
{
public:
    explicit ProjectItem(Project *project) : m_project(project)
    {
        QTC_ASSERT(m_project, return);
        QString display = ProjectWindow::tr("Build & Run");
        appendChild(m_targetsItem = new TargetGroupItem(display, project));
        appendChild(m_miscItem = new MiscSettingsGroupItem(project));
    }

    QVariant data(int column, int role) const override
    {
        switch (role) {
        case Qt::DisplayRole:
            return m_project->displayName();

        case ProjectDisplayNameRole:
            return m_project->displayName();

        case Qt::FontRole: {
            QFont font;
            font.setBold(m_project == SessionManager::startupProject());
            return font;
        }

        case PanelWidgetRole:
        case ActiveItemRole:
            if (m_currentChildIndex == 0)
                return m_targetsItem->data(column, role);
            if (m_currentChildIndex == 1)
                return m_miscItem->data(column, role);
        }
        return QVariant();
    }

    bool setData(int column, const QVariant &data, int role) override
    {
        Q_UNUSED(column)

        if (role == ItemDeactivatedFromBelowRole) {
            parent()->setData(0, QVariant::fromValue(static_cast<TreeItem *>(this)), role);
            return true;
        }

        if (role == ItemActivatedFromBelowRole) {
            TreeItem *item = data.value<TreeItem *>();
            QTC_ASSERT(item, return false);
            int res = children().indexOf(item);
            QTC_ASSERT(res >= 0, return false);
            m_currentChildIndex = res;
            parent()->setData(0, QVariant::fromValue(static_cast<TreeItem *>(this)), role);
            return true;
        }

        if (role == ItemActivatedDirectlyRole) {
            // Someone selected the project using the combobox or similar.
            SessionManager::setStartupProject(m_project);
            m_currentChildIndex = 0; // Use some Target page by defaults
            m_targetsItem->setData(column, data, ItemActivatedFromAboveRole); // And propagate downwards.
            parent()->setData(0, QVariant::fromValue(static_cast<TreeItem *>(this)),
                              ItemActivatedFromBelowRole);
            return true;
        }

        if (role == ItemActivatedFromAboveRole) {
            // Someone selected the project using the combobox or similar.
            // Do not change the previous active subitem,
            SessionManager::setStartupProject(m_project);
            // Downwards.
            if (m_currentChildIndex == 0)
                m_targetsItem->setData(column, data, role);
            else if (m_currentChildIndex == 1)
                m_miscItem->setData(column, data, role);

            return true;
        }

        return false;
    }

    Project *project() const { return m_project; }

private:
    int m_currentChildIndex = 0; // Start with Build & Run.
    Project *m_project;
    TargetGroupItem *m_targetsItem;
    MiscSettingsGroupItem *m_miscItem;
};


class RootItem : public TypedTreeItem<ProjectItem>
{
public:
    QVariant data(int column, int role) const override
    {
        if (role == PanelWidgetRole || role == ActiveItemRole) {
            if (0 <= m_currentProjectIndex && m_currentProjectIndex < childCount())
                return childAt(m_currentProjectIndex)->data(column, role);
        }

        return QVariant();
    }

    bool setData(int column, const QVariant &data, int role) override
    {
        Q_UNUSED(column)

        switch (role) {
        case ItemActivatedFromBelowRole: {
            TreeItem *t = data.value<TreeItem *>();
            QTC_CHECK(t);
            m_currentProjectIndex = children().indexOf(t);
            updateAll();
            updateExternals();
            return true;
        }
        case ItemDeactivatedFromBelowRole: {
            QTC_CHECK(data.isValid());
            updateAll();
            updateExternals();
            return true;
        }
        case ItemActivatedFromAboveRole:
        case ItemActivatedDirectlyRole: {
            QTC_CHECK(!data.isValid());
            updateAll();
            updateExternals();
            return true;
        }
        }

        return false;
    }

    void setCurrentProject(int index)
    {
        m_currentProjectIndex = index;
        m_tree->setRootIndex(childAt(index)->index());
        updateExternals();
    }

    void updateExternals() const
    {
        QTimer::singleShot(0, [this] {
            // Needs to be async to be run after selection changes
            // triggered by the normal QTreeView machinery.
            TreeItem *item = data(0, ActiveItemRole).value<TreeItem *>();
            QTC_ASSERT(item, return);
            QWidget *widget = item->data(0, PanelWidgetRole).value<QWidget *>();
            m_updater(widget);
            QModelIndex idx = item->index();
            m_tree->selectionModel()->clear();
            m_tree->selectionModel()->select(idx, QItemSelectionModel::Select);
        });
    }

public:
    int m_currentProjectIndex = -1;
    std::function<void(QWidget *)> m_updater;
    QTreeView *m_tree = 0;
};


//
// SelectorModel
//

class SelectorModel : public TreeModel<RootItem, ProjectItem, TreeItem>
{
public:
    SelectorModel(QObject *parent)
        : TreeModel<RootItem, ProjectItem, TreeItem>(parent)
    {}
};


//
// SelectorDelegate
//

class SelectorDelegate : public QStyledItemDelegate
{
public:
    SelectorDelegate() {}

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override
    {
        QSize s = QStyledItemDelegate::sizeHint(option, index);
        auto model = static_cast<const SelectorModel *>(index.model());
        if (TreeItem *item = model->itemForIndex(index)) {
            switch (item->level()) {
                case 2: s = QSize(s.width(), 3 * s.height()); break;
            }
        }
        return s;
    }

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        auto model = static_cast<const SelectorModel *>(index.model());
        QStyleOptionViewItem opt = option;
        if (TreeItem *item = model->itemForIndex(index)) {
            switch (item->level()) {
                case 2:
                    opt.font.setBold(true);
                    opt.font.setPointSizeF(opt.font.pointSizeF() * 1.2);
                    break;
            }
        }
        QStyledItemDelegate::paint(painter, opt, index);
    }
};

//
// SelectorTree
//

class SelectorTree : public BaseTreeView
{
public:
    SelectorTree()
    {
        setWindowTitle("Project Kit Selector");

        header()->hide();
        setExpandsOnDoubleClick(false);
        setHeaderHidden(true);
        setItemsExpandable(false); // No user interaction.
        setRootIsDecorated(false);
        setUniformRowHeights(false); // sic!
        setSelectionMode(QAbstractItemView::SingleSelection);
        setSelectionBehavior(QAbstractItemView::SelectRows);
        setEditTriggers(QAbstractItemView::NoEditTriggers);
        setActivationMode(SingleClickActivation);
        setObjectName("ProjectNavigation");
        setContextMenuPolicy(Qt::CustomContextMenu);
    }

    // remove branch indicators
    void drawBranches(QPainter *, const QRect &, const QModelIndex &) const override
    {
        return;
    }
};


//
// ProjectWindow
//

ProjectWindow::ProjectWindow()
{
    setBackgroundRole(QPalette::Base);

    m_selectorModel = new SelectorModel(this);
    m_selectorModel->setHeader({ tr("Projects") });
    m_selectorModel->rootItem()->m_updater = [this](QWidget *panel) {
        if (QWidget *widget = centralWidget()) {
            takeCentralWidget();
            widget->hide(); // Don't delete.
        }
        if (panel) {
            setCentralWidget(panel);
            panel->show();
            if (hasFocus()) // we get assigned focus from setFocusToCurrentMode, pass that on
                panel->setFocus();
        }
    };

    m_selectorTree = new SelectorTree;
    m_selectorTree->setModel(m_selectorModel);
    m_selectorTree->setItemDelegate(new SelectorDelegate);
    m_selectorModel->rootItem()->m_tree = m_selectorTree;
    connect(m_selectorTree, &QAbstractItemView::activated,
            this, &ProjectWindow::itemActivated);

    m_projectSelection = new QComboBox;
    m_projectSelection->setModel(m_selectorModel);
//    m_projectSelection->setProperty("hideicon", true);
//    m_projectSelection->setProperty("notelideasterisk", true);
//    m_projectSelection->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    connect(m_projectSelection, static_cast<void(QComboBox::*)(int)>(&QComboBox::activated),
            this, &ProjectWindow::projectSelected, Qt::QueuedConnection);

    auto styledBar = new StyledBar; // The black blob on top of the side bar
    styledBar->setObjectName("ProjectModeStyledBar");

//    auto styledBarLayout = new QHBoxLayout(styledBar);
//    styledBarLayout->setContentsMargins(0, 0, 0, 0);
//    styledBarLayout->addWidget(m_projectSelection);

    auto selectorView = new QWidget; // Black blob + Project tree + Combobox below.
    selectorView->setObjectName("ProjectSelector"); // Needed for dock widget state saving
    selectorView->setWindowTitle(tr("Project Selector"));
    selectorView->setAutoFillBackground(true);

    auto innerLayout = new QVBoxLayout;
    innerLayout->setSpacing(10);
    innerLayout->setContentsMargins(14, innerLayout->spacing(), 14, 0);
    innerLayout->addWidget(m_projectSelection);
    innerLayout->addWidget(m_selectorTree);

    auto selectorLayout = new QVBoxLayout(selectorView);
    selectorLayout->setContentsMargins(0, 0, 0, 0);
    selectorLayout->addWidget(styledBar);
    selectorLayout->addLayout(innerLayout);

    m_selectorDock = addDockForWidget(selectorView, true);
    addDockWidget(Qt::LeftDockWidgetArea, m_selectorDock);

    SessionManager *sessionManager = SessionManager::instance();
    connect(sessionManager, &SessionManager::projectAdded,
            this, &ProjectWindow::registerProject);
    connect(sessionManager, &SessionManager::aboutToRemoveProject,
            this, &ProjectWindow::deregisterProject);
    connect(sessionManager, &SessionManager::startupProjectChanged,
            this, &ProjectWindow::startupProjectChanged);
    connect(m_selectorTree, &QWidget::customContextMenuRequested,
            this, &ProjectWindow::openContextMenu);
}

void ProjectWindow::openContextMenu(const QPoint &pos)
{
    auto menu = new QMenu;
    menu->setAttribute(Qt::WA_DeleteOnClose);

    QModelIndex index = m_selectorTree->indexAt(pos);
    m_selectorModel->setData(index, QVariant::fromValue(menu), ContextMenuItemAdderRole);

    if (menu->actions().isEmpty())
        delete menu;
    else
        menu->popup(m_selectorTree->mapToGlobal(pos));
}

void ProjectWindow::contextMenuEvent(QContextMenuEvent *event)
{
    Q_UNUSED(event)
    // Do nothing to avoid creation of the dock window selection menu.
}

void ProjectWindow::registerProject(Project *project)
{
    QTC_ASSERT(itemForProject(project) == nullptr, return);

    auto newTab = new ProjectItem(project);

    m_selectorModel->rootItem()->appendChild(newTab);

    // FIXME: Add a TreeModel::insert(item, comparator)
    m_selectorModel->rootItem()->sortChildren([this](const ProjectItem *a, const ProjectItem *b) {
        Project *pa = a->project();
        Project *pb = b->project();
        QString aName = pa->displayName();
        QString bName = pb->displayName();
        if (aName != bName)
            return aName < bName;
        FileName aPath = pa->projectFilePath();
        FileName bPath = pb->projectFilePath();
        if (aPath != bPath)
            return aPath < bPath;
        return pa < pb;
    });

    m_selectorTree->expandAll();
}

void ProjectWindow::deregisterProject(Project *project)
{
    delete m_selectorModel->takeItem(itemForProject(project));
}

void ProjectWindow::startupProjectChanged(Project *project)
{
    if (ProjectItem *projectItem = itemForProject(project)) {
        int index = projectItem->indexInParent();
        QTC_ASSERT(index != -1, return);
        m_projectSelection->setCurrentIndex(index);
        m_selectorModel->rootItem()->setCurrentProject(index);
    }
}

void ProjectWindow::projectSelected(int index)
{
    Project *project = m_selectorModel->rootItem()->childAt(index)->project();
    SessionManager::setStartupProject(project);
}

void ProjectWindow::itemActivated(const QModelIndex &index)
{
    m_selectorModel->setData(index, QVariant(), ItemActivatedDirectlyRole);
}

ProjectItem *ProjectWindow::itemForProject(Project *project) const
{
    return m_selectorModel->findItemAtLevel<1>([project](ProjectItem *item) {
        return item->project() == project;
    });
}

} // namespace Internal
} // namespace ProjectExplorer
