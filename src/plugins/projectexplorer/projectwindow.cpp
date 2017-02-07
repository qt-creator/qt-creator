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

#include "buildinfo.h"
#include "kit.h"
#include "kitmanager.h"
#include "kitoptionspage.h"
#include "panelswidget.h"
#include "project.h"
#include "projectexplorer.h"
#include "projectimporter.h"
#include "projectpanelfactory.h"
#include "session.h"
#include "target.h"
#include "targetsettingspanel.h"

#include <coreplugin/coreicons.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/algorithm.h>
#include <utils/basetreeview.h>
#include <utils/navigationtreeview.h>
#include <utils/qtcassert.h>
#include <utils/styledbar.h>
#include <utils/treemodel.h>

#include <QApplication>
#include <QComboBox>
#include <QDockWidget>
#include <QFileDialog>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QStyledItemDelegate>
#include <QTimer>
#include <QTreeView>
#include <QVBoxLayout>

using namespace Core;
using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

class MiscSettingsGroupItem;

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
            QWidget *widget = m_factory->createWidget(m_project);
            m_widget = new PanelsWidget(m_factory->displayName(),
                                        QIcon(m_factory->icon()),
                                        widget);
            m_widget->setFocusProxy(widget);
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
        return Qt::NoItemFlags;
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
            m_currentPanelIndex = indexOf(item);
            QTC_ASSERT(m_currentPanelIndex != -1, return false);
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

// The first tree level, i.e. projects.
class ProjectItem : public TreeItem
{
public:
    ProjectItem() {}

    ProjectItem(Project *project, const std::function<void()> &changeListener)
        : m_project(project), m_changeListener(changeListener)
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

    bool setData(int column, const QVariant &dat, int role) override
    {
        Q_UNUSED(column)

        if (role == ItemUpdatedFromBelowRole) {
            announceChange();
            return true;
        }

        if (role == ItemDeactivatedFromBelowRole) {
            announceChange();
            return true;
        }

        if (role == ItemActivatedFromBelowRole) {
            const TreeItem *item = dat.value<TreeItem *>();
            QTC_ASSERT(item, return false);
            int res = indexOf(item);
            QTC_ASSERT(res >= 0, return false);
            m_currentChildIndex = res;
            announceChange();
            return true;
        }

        if (role == ItemActivatedDirectlyRole) {
            // Someone selected the project using the combobox or similar.
            SessionManager::setStartupProject(m_project);
            m_currentChildIndex = 0; // Use some Target page by defaults
            m_targetsItem->setData(column, dat, ItemActivatedFromAboveRole); // And propagate downwards.
            announceChange();
            return true;
        }

        return false;
    }

    void announceChange()
    {
        m_changeListener();
    }

    Project *project() const { return m_project; }

    QModelIndex activeIndex() const
    {
        TreeItem *activeItem = data(0, ActiveItemRole).value<TreeItem *>();
        return activeItem ? activeItem->index() : QModelIndex();
    }

private:
    int m_currentChildIndex = 0; // Start with Build & Run.
    Project *m_project = nullptr;
    TargetGroupItem *m_targetsItem = nullptr;
    MiscSettingsGroupItem *m_miscItem = nullptr;
    const std::function<void ()> m_changeListener;
};


class SelectorDelegate : public QStyledItemDelegate
{
public:
    SelectorDelegate() {}

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const final;

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const final;
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
    void drawBranches(QPainter *, const QRect &, const QModelIndex &) const final
    {
        return;
    }
};

class ComboBoxItem : public TreeItem
{
public:
    ComboBoxItem(ProjectItem *item) : m_projectItem(item) {}

    QVariant data(int column, int role) const final
    {
        return m_projectItem ? m_projectItem->data(column, role) : QVariant();
    }

    ProjectItem *m_projectItem;
};

using ProjectsModel = TreeModel<TypedTreeItem<ProjectItem>, ProjectItem>;
using ComboBoxModel = TreeModel<TypedTreeItem<ComboBoxItem>, ComboBoxItem>;

//
// ProjectWindowPrivate
//

class ProjectWindowPrivate : public QObject
{
public:
    ProjectWindowPrivate(ProjectWindow *parent)
        : q(parent)
    {
        m_projectsModel.setHeader({ ProjectWindow::tr("Projects") });

        m_selectorTree = new SelectorTree;
        m_selectorTree->setModel(&m_projectsModel);
        m_selectorTree->setItemDelegate(new SelectorDelegate);
        m_selectorTree->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(m_selectorTree, &QAbstractItemView::activated,
                this, &ProjectWindowPrivate::itemActivated);
        connect(m_selectorTree, &QWidget::customContextMenuRequested,
                this, &ProjectWindowPrivate::openContextMenu);

        m_projectSelection = new QComboBox;
        m_projectSelection->setModel(&m_comboBoxModel);
        connect(m_projectSelection, static_cast<void(QComboBox::*)(int)>(&QComboBox::activated),
                this, &ProjectWindowPrivate::projectSelected, Qt::QueuedConnection);

        SessionManager *sessionManager = SessionManager::instance();
        connect(sessionManager, &SessionManager::projectAdded,
                this, &ProjectWindowPrivate::registerProject);
        connect(sessionManager, &SessionManager::aboutToRemoveProject,
                this, &ProjectWindowPrivate::deregisterProject);
        connect(sessionManager, &SessionManager::startupProjectChanged,
                this, &ProjectWindowPrivate::startupProjectChanged);

        m_importBuild = new QPushButton(ProjectWindow::tr("Import Existing Build..."));
        connect(m_importBuild, &QPushButton::clicked,
                this, &ProjectWindowPrivate::handleImportBuild);
        connect(sessionManager, &SessionManager::startupProjectChanged, this, [this](Project *project) {
            m_importBuild->setEnabled(project && project->projectImporter());
        });

        m_manageKits = new QPushButton(ProjectWindow::tr("Manage Kits..."));
        connect(m_manageKits, &QPushButton::clicked,
                this, &ProjectWindowPrivate::handleManageKits);

        auto styledBar = new StyledBar; // The black blob on top of the side bar
        styledBar->setObjectName("ProjectModeStyledBar");

        auto selectorView = new QWidget; // Black blob + Combobox + Project tree below.
        selectorView->setObjectName("ProjectSelector"); // Needed for dock widget state saving
        selectorView->setWindowTitle(ProjectWindow::tr("Project Selector"));
        selectorView->setAutoFillBackground(true);
        selectorView->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(selectorView, &QWidget::customContextMenuRequested,
                this, &ProjectWindowPrivate::openContextMenu);

        auto activeLabel = new QLabel(ProjectWindow::tr("Active Project"));
        QFont font = activeLabel->font();
        font.setBold(true);
        font.setPointSizeF(font.pointSizeF() * 1.2);
        activeLabel->setFont(font);

        auto innerLayout = new QVBoxLayout;
        innerLayout->setSpacing(10);
        innerLayout->setContentsMargins(14, innerLayout->spacing(), 14, 0);
        innerLayout->addWidget(m_manageKits);
        innerLayout->addWidget(m_importBuild);
        innerLayout->addSpacerItem(new QSpacerItem(10, 30, QSizePolicy::Maximum, QSizePolicy::Maximum));
        innerLayout->addWidget(activeLabel);
        innerLayout->addWidget(m_projectSelection);
        innerLayout->addWidget(m_selectorTree);

        auto selectorLayout = new QVBoxLayout(selectorView);
        selectorLayout->setContentsMargins(0, 0, 0, 0);
        selectorLayout->addWidget(styledBar);
        selectorLayout->addLayout(innerLayout);

        auto selectorDock = q->addDockForWidget(selectorView, true);
        q->addDockWidget(Qt::LeftDockWidgetArea, selectorDock);
    }

    void updatePanel()
    {
        ProjectItem *projectItem = m_projectsModel.rootItem()->childAt(0);
        setPanel(projectItem->data(0, PanelWidgetRole).value<QWidget *>());

        QModelIndex activeIndex = projectItem->activeIndex();
        m_selectorTree->expandAll();
        m_selectorTree->selectionModel()->clear();
        m_selectorTree->selectionModel()->select(activeIndex, QItemSelectionModel::Select);
    }

    void registerProject(Project *project)
    {
        QTC_ASSERT(itemForProject(project) == nullptr, return);
        auto projectItem = new ProjectItem(project, [this] { updatePanel(); });
        m_comboBoxModel.rootItem()->appendChild(new ComboBoxItem(projectItem));
    }

    void deregisterProject(Project *project)
    {
        ComboBoxItem *item = itemForProject(project);
        QTC_ASSERT(item, return);
        if (item->m_projectItem->parent())
            m_projectsModel.takeItem(item->m_projectItem);
        delete item->m_projectItem;
        item->m_projectItem = nullptr;
        m_comboBoxModel.destroyItem(item);
    }

    void projectSelected(int index)
    {
        Project *project = m_comboBoxModel.rootItem()->childAt(index)->m_projectItem->project();
        SessionManager::setStartupProject(project);
    }

    ComboBoxItem *itemForProject(Project *project) const
    {
        return m_comboBoxModel.findItemAtLevel<1>([project](ComboBoxItem *item) {
            return item->m_projectItem->project() == project;
        });
    }

    void startupProjectChanged(Project *project)
    {
        if (ProjectItem *current = m_projectsModel.rootItem()->childAt(0))
            m_projectsModel.takeItem(current); // Keep item as such alive.
        if (!project) // Shutting down.
            return;
        ComboBoxItem *comboboxItem = itemForProject(project);
        QTC_ASSERT(comboboxItem, return);
        m_projectsModel.rootItem()->appendChild(comboboxItem->m_projectItem);
        m_projectSelection->setCurrentIndex(comboboxItem->indexInParent());
        m_selectorTree->expandAll();
        m_selectorTree->setRootIndex(m_projectsModel.index(0, 0, QModelIndex()));
        updatePanel();
    }

    void itemActivated(const QModelIndex &index)
    {
        if (TreeItem *item = m_projectsModel.itemForIndex(index))
            item->setData(0, QVariant(), ItemActivatedDirectlyRole);
    }

    void openContextMenu(const QPoint &pos)
    {
        QMenu menu;

        ProjectItem *projectItem = m_projectsModel.rootItem()->childAt(0);
        Project *project = projectItem ? projectItem->project() : nullptr;

        QModelIndex index = m_selectorTree->indexAt(pos);
        TreeItem *item = m_projectsModel.itemForIndex(index);
        if (item)
            item->setData(0, QVariant::fromValue(&menu), ContextMenuItemAdderRole);

        if (!menu.actions().isEmpty())
            menu.addSeparator();

        QAction *importBuild = menu.addAction(ProjectWindow::tr("Import Existing Build..."));
        importBuild->setEnabled(project && project->projectImporter());
        QAction *manageKits = menu.addAction(ProjectWindow::tr("Manage Kits..."));

        QAction *act = menu.exec(m_selectorTree->mapToGlobal(pos));

        if (act == importBuild)
            handleImportBuild();
        else if (act == manageKits)
            handleManageKits();
    }

    void handleManageKits()
    {
        if (ProjectItem *projectItem = m_projectsModel.rootItem()->childAt(0)) {
            if (KitOptionsPage *page = ExtensionSystem::PluginManager::getObject<KitOptionsPage>())
                page->showKit(KitManager::kit(Id::fromSetting(projectItem->data(0, KitIdRole))));
        }
        ICore::showOptionsDialog(Constants::KITS_SETTINGS_PAGE_ID, ICore::mainWindow());
    }

    void handleImportBuild()
    {
        ProjectItem *projectItem = m_projectsModel.rootItem()->childAt(0);
        Project *project = projectItem ? projectItem->project() : 0;
        ProjectImporter *projectImporter = project ? project->projectImporter() : 0;
        QTC_ASSERT(projectImporter, return);

        QString dir = project->projectDirectory().toString();
        QString importDir = QFileDialog::getExistingDirectory(ICore::mainWindow(),
                                                              ProjectWindow::tr("Import directory"),
                                                              dir);
        FileName path = FileName::fromString(importDir);

        const QList<BuildInfo *> toImport = projectImporter->import(path, false);
        for (BuildInfo *info : toImport) {
            Target *target = project->target(info->kitId);
            if (!target) {
                target = project->createTarget(KitManager::kit(info->kitId));
                if (target)
                    project->addTarget(target);
            }
            if (target) {
                projectImporter->makePersistent(target->kit());
                BuildConfiguration *bc = info->factory()->create(target, info);
                QTC_ASSERT(bc, continue);
                target->addBuildConfiguration(bc);
            }
        }
        qDeleteAll(toImport);
    }

    void setPanel(QWidget *panel)
    {
        if (QWidget *widget = q->centralWidget()) {
            q->takeCentralWidget();
            widget->hide(); // Don't delete.
        }
        if (panel) {
            q->setCentralWidget(panel);
            panel->show();
            if (q->hasFocus()) // we get assigned focus from setFocusToCurrentMode, pass that on
                panel->setFocus();
        }
    }

    ProjectWindow *q;
    ProjectsModel m_projectsModel;
    ComboBoxModel m_comboBoxModel;
    QComboBox *m_projectSelection;
    SelectorTree *m_selectorTree;
    QPushButton *m_importBuild;
    QPushButton *m_manageKits;
};

//
// ProjectWindow
//

ProjectWindow::ProjectWindow()
    : d(new ProjectWindowPrivate(this))
{
    setBackgroundRole(QPalette::Base);

    // Request custom context menu but do not provide any to avoid
    // the creation of the dock window selection menu.
    setContextMenuPolicy(Qt::CustomContextMenu);
}

ProjectWindow::~ProjectWindow()
{
    delete d;
}

QSize SelectorDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize s = QStyledItemDelegate::sizeHint(option, index);
    auto model = static_cast<const ProjectsModel *>(index.model());
    if (TreeItem *item = model->itemForIndex(index)) {
        switch (item->level()) {
        case 2:
            s = QSize(s.width(), 3 * s.height());
            break;
        case 3:
        case 4:
            s = QSize(s.width(), s.height() * 1.2);
            break;
        }
    }
    return s;
}

void SelectorDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    auto model = static_cast<const ProjectsModel *>(index.model());
    QStyleOptionViewItem opt = option;
    if (TreeItem *item = model->itemForIndex(index)) {
        switch (item->level()) {
        case 2: {
            QColor col = creatorTheme()->color(Theme::TextColorNormal);
            opt.palette.setColor(QPalette::Text, col);
            opt.font.setBold(true);
            opt.font.setPointSizeF(opt.font.pointSizeF() * 1.2);
            break;
            }
        }
    }
    QStyledItemDelegate::paint(painter, opt, index);
}

} // namespace Internal
} // namespace ProjectExplorer
