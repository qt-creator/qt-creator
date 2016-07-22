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

#include <QApplication>
#include <QComboBox>
#include <QDockWidget>
#include <QHeaderView>
#include <QMenu>
#include <QStyledItemDelegate>
#include <QTreeView>
#include <QVBoxLayout>

using namespace Core;
using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

// The first tree level, i.e. projects.
class ProjectItem : public TreeItem
{
public:
    explicit ProjectItem(Project *project) : m_project(project)
    {
        QTC_ASSERT(m_project, return);
        foreach (ProjectPanelFactory *factory, ProjectPanelFactory::factories())
            appendChild(factory->createSelectorItem(m_project));
    }

    QVariant data(int column, int role) const override
    {
        switch (role) {
        case Qt::DisplayRole:
            return m_project->displayName();

        case ProjectDisplayNameRole:
            return m_project->displayName();

        case Qt::DecorationRole: {
            QVariant icon;
            forSecondLevelChildren<TreeItem *>([this, &icon](TreeItem *item) {
                QVariant sicon = item->data(0, Qt::DecorationRole);
                if (sicon.isValid())
                    icon = sicon;
            });
            return icon;
        }

        case Qt::FontRole: {
            QFont font;
            font.setBold(m_project == SessionManager::startupProject());
            return font;
        }

        case ActiveWidgetRole:
        case ActiveIndexRole:
            if (0 <= m_currentPanelIndex && m_currentPanelIndex < childCount())
                return childAt(m_currentPanelIndex)->data(column, role);
        }
        return QVariant();
    }

    bool setData(int column, const QVariant &data, int role) override
    {
        Q_UNUSED(column)

        if (role == ItemActivaterRole) {
            // Possible called from child item.
            TreeItem *item = data.value<TreeItem *>();
            m_currentPanelIndex = children().indexOf(item);

            SessionManager::setStartupProject(m_project);

            // Bubble up.
            parent()->setData(0, QVariant::fromValue(static_cast<TreeItem *>(this)), role);
            return true;
        }

        return false;
    }

    Project *project() const { return m_project; }

private:
    int m_currentPanelIndex = 0;

    Project * const m_project;
};

class RootItem : public TypedTreeItem<ProjectItem>
{
public:
    QVariant data(int column, int role) const override
    {
        if (role == ActiveWidgetRole) {
            if (0 <= m_currentProjectIndex && m_currentProjectIndex < childCount())
                return childAt(m_currentProjectIndex)->data(column, role);
        }

        return QVariant();
    }

    bool setData(int column, const QVariant &data, int role) override
    {
        Q_UNUSED(column)

        if (role == ItemActivaterRole) {
            // Possible called from child item.
            if (TreeItem *t = data.value<TreeItem *>())
                m_currentProjectIndex = children().indexOf(t);
            updateAll();
            return true;
        }

        return false;
    }

    int m_currentProjectIndex = -1;
};


//
// SelectorModel
//

class SelectorModel
    : public LeveledTreeModel<RootItem, ProjectItem, TreeItem>
{
    Q_OBJECT

public:
    SelectorModel(QObject *parent)
        : LeveledTreeModel<RootItem, ProjectItem, TreeItem>(parent)
    {
        setRootItem(new RootItem);
        setHeader({ ProjectWindow::tr("Projects") });
    }

signals:
    void needPanelUpdate();
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
        TreeItem *item = model->itemForIndex(index);
        if (item && item->level() == 2)
            s = QSize(s.width(), 3 * s.height());
        return s;
    }

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        auto model = static_cast<const SelectorModel *>(index.model());
        TreeItem *item = model->itemForIndex(index);
        QStyleOptionViewItem opt = option;
        if (item && item->level() == 2) {
            opt.font.setBold(true);
            opt.font.setPointSizeF(opt.font.pointSizeF() * 1.2);
        }
        QStyledItemDelegate::paint(painter, opt, index);
    }
};

//
// SelectorTree
//

class SelectorTree : public NavigationTreeView
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
};


//
// ProjectWindow
//

ProjectWindow::ProjectWindow()
{
    setBackgroundRole(QPalette::Base);

    m_selectorModel = new SelectorModel(this);
    connect(m_selectorModel, &SelectorModel::needPanelUpdate,
            this, &ProjectWindow::updatePanel);

    m_selectorTree = new SelectorTree;
    m_selectorTree->setModel(m_selectorModel);
    m_selectorTree->setItemDelegate(new SelectorDelegate);
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

    auto styledBarLayout = new QHBoxLayout(styledBar);
    styledBarLayout->setContentsMargins(0, 0, 0, 0);
    styledBarLayout->addWidget(m_projectSelection);

    auto selectorView = new QWidget; // Black blob + Project tree + Combobox below.
    selectorView->setObjectName("ProjectSelector"); // Needed for dock widget state saving
    selectorView->setWindowTitle(tr("Project Selector"));
    selectorView->setAutoFillBackground(true);

    auto innerLayout = new QVBoxLayout;
    innerLayout->setContentsMargins(14, 0, 14, 0);
    //innerLayout->addWidget(m_projectSelection);
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
        Utils::FileName aPath = pa->projectFilePath();
        Utils::FileName bPath = pb->projectFilePath();
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
        int index = projectItem->parent()->children().indexOf(projectItem);
        QTC_ASSERT(index != -1, return);
        m_projectSelection->setCurrentIndex(index);
        m_selectorModel->rootItem()->m_currentProjectIndex = index;
        m_selectorTree->update();
        m_selectorTree->setRootIndex(m_selectorModel->indexForItem(m_selectorModel->rootItem()->childAt(index)));
        m_selectorTree->expandAll();
        QModelIndex activeIndex = projectItem->data(0, ActiveIndexRole).value<QModelIndex>();
        m_selectorTree->selectionModel()->setCurrentIndex(activeIndex, QItemSelectionModel::SelectCurrent);
        updatePanel();
    }
}

void ProjectWindow::projectSelected(int index)
{
    auto projectItem = m_selectorModel->rootItem()->childAt(index);
    QTC_ASSERT(projectItem, return);
    SessionManager::setStartupProject(projectItem->project());
}

void ProjectWindow::itemActivated(const QModelIndex &index)
{
    m_selectorModel->setData(index, QVariant(), ItemActivaterRole);
    updatePanel();
}

void ProjectWindow::updatePanel()
{
    if (QWidget *widget = centralWidget()) {
        takeCentralWidget();
        widget->hide(); // Don't delete.
    }

    RootItem *rootItem = m_selectorModel->rootItem();
    if (QWidget *widget = rootItem->data(0, ActiveWidgetRole).value<QWidget *>()) {
        setCentralWidget(widget);
        widget->show();
        if (hasFocus()) // we get assigned focus from setFocusToCurrentMode, pass that on
            widget->setFocus();
    }
}

ProjectItem *ProjectWindow::itemForProject(Project *project) const
{
    return m_selectorModel->findFirstLevelItem([project](ProjectItem *item) {
        return item->project() == project;
    });
}

} // namespace Internal
} // namespace ProjectExplorer

#include "projectwindow.moc"
