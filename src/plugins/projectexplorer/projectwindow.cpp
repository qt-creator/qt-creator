// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projectwindow.h"

#include "buildinfo.h"
#include "kit.h"
#include "kitmanager.h"
#include "kitoptionspage.h"
#include "panelswidget.h"
#include "project.h"
#include "projectexplorerconstants.h"
#include "projectexplorertr.h"
#include "projectimporter.h"
#include "projectmanager.h"
#include "projectpanelfactory.h"
#include "projectsettingswidget.h"
#include "target.h"
#include "targetsettingspanel.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/commandbutton.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/coreicons.h>
#include <coreplugin/coreplugintr.h>
#include <coreplugin/find/optionspopup.h>
#include <coreplugin/findplaceholder.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/outputwindow.h>

#include <utils/algorithm.h>
#include <utils/basetreeview.h>
#include <utils/hostosinfo.h>
#include <utils/navigationtreeview.h>
#include <utils/qtcassert.h>
#include <utils/qtcsettings.h>
#include <utils/styledbar.h>
#include <utils/treemodel.h>
#include <utils/utilsicons.h>

#include <texteditor/fontsettings.h>
#include <texteditor/texteditorsettings.h>

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

const char kBuildSystemOutputContext[] = "ProjectsMode.BuildSystemOutput";
const char kRegExpActionId[] = "OutputFilter.RegularExpressions.BuildSystemOutput";
const char kCaseSensitiveActionId[] = "OutputFilter.CaseSensitive.BuildSystemOutput";
const char kInvertActionId[] = "OutputFilter.Invert.BuildSystemOutput";

class BuildSystemOutputWindow : public OutputWindow
{
public:
    BuildSystemOutputWindow();

    QWidget *toolBar();

private:
    void updateFilter();

    QPointer<QWidget> m_toolBar;
    QPointer<FancyLineEdit> m_filterOutputLineEdit;
    QAction m_clear;
    QAction m_filterActionRegexp;
    QAction m_filterActionCaseSensitive;
    QAction m_invertFilterAction;
    QAction m_zoomIn;
    QAction m_zoomOut;
};

BuildSystemOutputWindow::BuildSystemOutputWindow()
    : OutputWindow(Context(kBuildSystemOutputContext), "ProjectsMode.BuildSystemOutput.Zoom")
{
    setReadOnly(true);

    Command *clearCommand = ActionManager::command(Core::Constants::OUTPUTPANE_CLEAR);
    m_clear.setIcon(Utils::Icons::CLEAN_TOOLBAR.icon());
    m_clear.setText(clearCommand->action()->text());
    ActionManager::registerAction(&m_clear,
                                  Core::Constants::OUTPUTPANE_CLEAR,
                                  Context(kBuildSystemOutputContext));
    connect(&m_clear, &QAction::triggered, this, &OutputWindow::clear);

    m_filterActionRegexp.setCheckable(true);
    m_filterActionRegexp.setText(Tr::tr("Use Regular Expressions"));
    connect(&m_filterActionRegexp, &QAction::toggled, this, &BuildSystemOutputWindow::updateFilter);
    Core::ActionManager::registerAction(&m_filterActionRegexp,
                                        kRegExpActionId,
                                        Context(Constants::C_PROJECTEXPLORER));

    m_filterActionCaseSensitive.setCheckable(true);
    m_filterActionCaseSensitive.setText(Tr::tr("Case Sensitive"));
    connect(&m_filterActionCaseSensitive,
            &QAction::toggled,
            this,
            &BuildSystemOutputWindow::updateFilter);
    Core::ActionManager::registerAction(&m_filterActionCaseSensitive,
                                        kCaseSensitiveActionId,
                                        Context(Constants::C_PROJECTEXPLORER));

    m_invertFilterAction.setCheckable(true);
    m_invertFilterAction.setText(Tr::tr("Show Non-matching Lines"));
    connect(&m_invertFilterAction, &QAction::toggled, this, &BuildSystemOutputWindow::updateFilter);
    Core::ActionManager::registerAction(&m_invertFilterAction,
                                        kInvertActionId,
                                        Context(Constants::C_PROJECTEXPLORER));

    connect(TextEditor::TextEditorSettings::instance(),
            &TextEditor::TextEditorSettings::fontSettingsChanged,
            this,
            [this] { setBaseFont(TextEditor::TextEditorSettings::fontSettings().font()); });
    setBaseFont(TextEditor::TextEditorSettings::fontSettings().font());

    m_zoomIn.setIcon(Utils::Icons::PLUS_TOOLBAR.icon());
    connect(&m_zoomIn, &QAction::triggered, this, [this] { zoomIn(); });
    ActionManager::registerAction(&m_zoomIn,
                                  Core::Constants::ZOOM_IN,
                                  Context(kBuildSystemOutputContext));

    m_zoomOut.setIcon(Utils::Icons::MINUS_TOOLBAR.icon());
    connect(&m_zoomOut, &QAction::triggered, this, [this] { zoomOut(); });
    ActionManager::registerAction(&m_zoomOut,
                                  Core::Constants::ZOOM_OUT,
                                  Context(kBuildSystemOutputContext));
}

QWidget *BuildSystemOutputWindow::toolBar()
{
    if (!m_toolBar) {
        m_toolBar = new StyledBar(this);
        auto clearButton = new CommandButton(Core::Constants::OUTPUTPANE_CLEAR);
        clearButton->setDefaultAction(&m_clear);
        clearButton->setToolTipBase(m_clear.text());

        m_filterOutputLineEdit = new FancyLineEdit;
        m_filterOutputLineEdit->setButtonVisible(FancyLineEdit::Left, true);
        m_filterOutputLineEdit->setButtonIcon(FancyLineEdit::Left, Utils::Icons::MAGNIFIER.icon());
        m_filterOutputLineEdit->setFiltering(true);
        m_filterOutputLineEdit->setHistoryCompleter("ProjectsMode.BuildSystemOutput.Filter");
        connect(m_filterOutputLineEdit,
                &FancyLineEdit::textChanged,
                this,
                &BuildSystemOutputWindow::updateFilter);
        connect(m_filterOutputLineEdit,
                &FancyLineEdit::returnPressed,
                this,
                &BuildSystemOutputWindow::updateFilter);
        connect(m_filterOutputLineEdit, &FancyLineEdit::leftButtonClicked, this, [this] {
            auto popup = new Core::OptionsPopup(m_filterOutputLineEdit,
                                                {kRegExpActionId,
                                                 kCaseSensitiveActionId,
                                                 kInvertActionId});
            popup->show();
        });

        auto zoomInButton = new CommandButton(Core::Constants::ZOOM_IN);
        zoomInButton->setDefaultAction(&m_zoomIn);
        auto zoomOutButton = new CommandButton(Core::Constants::ZOOM_OUT);
        zoomOutButton->setDefaultAction(&m_zoomOut);

        auto layout = new QHBoxLayout;
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        m_toolBar->setLayout(layout);
        layout->addWidget(clearButton);
        layout->addWidget(m_filterOutputLineEdit);
        layout->addWidget(zoomInButton);
        layout->addWidget(zoomOutButton);
        layout->addStretch();
    }
    return m_toolBar;
}

void BuildSystemOutputWindow::updateFilter()
{
    if (!m_filterOutputLineEdit)
        return;
    updateFilterProperties(m_filterOutputLineEdit->text(),
                           m_filterActionCaseSensitive.isChecked() ? Qt::CaseSensitive
                                                                    : Qt::CaseInsensitive,
                           m_filterActionRegexp.isChecked(),
                           m_invertFilterAction.isChecked());
}

// Standard third level for the generic case: i.e. all except for the Build/Run page
class MiscSettingsPanelItem : public TreeItem // TypedTreeItem<TreeItem, MiscSettingsGroupItem>
{
public:
    MiscSettingsPanelItem(ProjectPanelFactory *factory, Project *project)
        : m_factory(factory), m_project(project)
    {}

    ~MiscSettingsPanelItem() override { delete m_widget; }

    QVariant data(int column, int role) const override;
    Qt::ItemFlags flags(int column) const override;
    bool setData(int column, const QVariant &, int role) override;

    ProjectPanelFactory *factory() const { return m_factory; }

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
            ProjectSettingsWidget *widget = m_factory->createWidget(m_project);
            m_widget = new PanelsWidget(m_factory->displayName(), widget);
            m_widget->setFocusProxy(widget);
        }

        return QVariant::fromValue<QWidget *>(m_widget.data());
    }

    if (role == ActiveItemRole)  // We are the active one.
        return QVariant::fromValue<TreeItem *>(const_cast<MiscSettingsPanelItem *>(this));
    return {};
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
        const QList<ProjectPanelFactory *> factories = ProjectPanelFactory::factories();
        for (ProjectPanelFactory *factory : factories)
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
            return Tr::tr("Project Settings");

        case PanelWidgetRole:
        case ActiveItemRole:
            if (0 <= m_currentPanelIndex && m_currentPanelIndex < childCount())
                return childAt(m_currentPanelIndex)->data(column, role);
        }
        return {};
    }

    bool setData(int column, const QVariant &data, int role) override
    {
        Q_UNUSED(column)

        if (role == ItemActivatedFromBelowRole) {
            auto *item = data.value<TreeItem *>();
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
    ProjectItem() = default;

    ProjectItem(Project *project, const std::function<void()> &changeListener)
        : m_project(project), m_changeListener(changeListener)
    {
        QTC_ASSERT(m_project, return);
        QString display = Tr::tr("Build & Run");
        appendChild(m_targetsItem = new TargetGroupItem(display, project));
        appendChild(m_miscItem = new MiscSettingsGroupItem(project));
    }

    QVariant data(int column, int role) const override
    {
        switch (role) {
        case Qt::DisplayRole:
        case ProjectDisplayNameRole:
            return m_project->displayName();

        case Qt::FontRole: {
            QFont font;
            font.setBold(m_project == ProjectManager::startupProject());
            return font;
        }

        case PanelWidgetRole:
        case ActiveItemRole:
            if (m_currentChildIndex == 0)
                return m_targetsItem->data(column, role);
            if (m_currentChildIndex == 1)
                return m_miscItem->data(column, role);
        }
        return {};
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
            ProjectManager::setStartupProject(m_project);
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
        auto *activeItem = data(0, ActiveItemRole).value<TreeItem *>();
        return activeItem ? activeItem->index() : QModelIndex();
    }

    TreeItem *itemForProjectPanel(Utils::Id panelId)
    {
        return m_miscItem->findChildAtLevel(1, [panelId](const TreeItem *item){
            return static_cast<const MiscSettingsPanelItem *>(item)->factory()->id() == panelId;
        });
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
    SelectorDelegate() = default;

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

private:
    // remove branch indicators
    void drawBranches(QPainter *, const QRect &, const QModelIndex &) const final
    {
        return;
    }

    bool userWantsContextMenu(const QMouseEvent *e) const
    {
        // On Windows, we get additional mouse events for the item view when right-clicking,
        // causing unwanted kit activation (QTCREATORBUG-24156). Let's suppress these.
        return HostOsInfo::isWindowsHost() && e->button() == Qt::RightButton;
    }

    void mousePressEvent(QMouseEvent *e) final
    {
        if (!userWantsContextMenu(e))
            BaseTreeView::mousePressEvent(e);
    }

    void mouseReleaseEvent(QMouseEvent *e) final
    {
        if (!userWantsContextMenu(e))
            BaseTreeView::mouseReleaseEvent(e);
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
        m_projectsModel.setHeader({Tr::tr("Projects")});

        m_selectorTree = new SelectorTree;
        m_selectorTree->setModel(&m_projectsModel);
        m_selectorTree->setItemDelegate(&m_selectorDelegate);
        m_selectorTree->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(m_selectorTree, &QAbstractItemView::activated,
                this, &ProjectWindowPrivate::itemActivated);
        connect(m_selectorTree, &QWidget::customContextMenuRequested,
                this, &ProjectWindowPrivate::openContextMenu);

        m_projectSelection = new QComboBox;
        m_projectSelection->setModel(&m_comboBoxModel);
        connect(m_projectSelection, &QComboBox::activated,
                this, &ProjectWindowPrivate::projectSelected, Qt::QueuedConnection);

        const auto switchProjectAction = new QAction(this);
        ActionManager::registerAction(switchProjectAction, Core::Constants::GOTOPREVINHISTORY,
                                      Context(Constants::C_PROJECTEXPLORER));
        connect(switchProjectAction, &QAction::triggered, this, [this] {
            if (m_projectSelection->count() > 1)
                m_projectSelection->showPopup();
        });

        ProjectManager *sessionManager = ProjectManager::instance();
        connect(sessionManager, &ProjectManager::projectAdded,
                this, &ProjectWindowPrivate::registerProject);
        connect(sessionManager, &ProjectManager::aboutToRemoveProject,
                this, &ProjectWindowPrivate::deregisterProject);
        connect(sessionManager, &ProjectManager::startupProjectChanged,
                this, &ProjectWindowPrivate::startupProjectChanged);

        m_importBuild = new QPushButton(Tr::tr("Import Existing Build..."));
        connect(m_importBuild, &QPushButton::clicked,
                this, &ProjectWindowPrivate::handleImportBuild);
        connect(sessionManager, &ProjectManager::startupProjectChanged, this, [this](Project *project) {
            m_importBuild->setEnabled(project && project->projectImporter());
        });

        m_manageKits = new QPushButton(Tr::tr("Manage Kits..."));
        connect(m_manageKits, &QPushButton::clicked,
                this, &ProjectWindowPrivate::handleManageKits);

        auto styledBar = new StyledBar; // The black blob on top of the side bar
        styledBar->setObjectName("ProjectModeStyledBar");

        auto selectorView = new QWidget; // Black blob + Combobox + Project tree below.
        selectorView->setObjectName("ProjectSelector"); // Needed for dock widget state saving
        selectorView->setWindowTitle(Tr::tr("Project Selector"));
        selectorView->setAutoFillBackground(true);

        auto activeLabel = new QLabel(Tr::tr("Active Project"));
        QFont font = activeLabel->font();
        font.setBold(true);
        font.setPointSizeF(font.pointSizeF() * 1.2);
        activeLabel->setFont(font);

        auto innerLayout = new QVBoxLayout;
        innerLayout->setSpacing(10);
        innerLayout->setContentsMargins(PanelsWidget::PanelVMargin, innerLayout->spacing(),
                                        PanelsWidget::PanelVMargin, 0);

        QStringList list = Core::ICore::settings()->value("HideOptionCategories").toStringList();
        if (!list.contains("Kits")) {
            innerLayout->addWidget(m_manageKits);
            innerLayout->addSpacerItem(new QSpacerItem(10, 30, QSizePolicy::Maximum, QSizePolicy::Maximum));
        }

        innerLayout->addWidget(activeLabel);
        innerLayout->addWidget(m_projectSelection);
        innerLayout->addWidget(m_importBuild);
        innerLayout->addWidget(m_selectorTree);

        auto selectorLayout = new QVBoxLayout(selectorView);
        selectorLayout->setContentsMargins(0, 0, 0, 0);
        selectorLayout->addWidget(styledBar);
        selectorLayout->addLayout(innerLayout);

        auto selectorDock = q->addDockForWidget(selectorView, true);
        q->addDockWidget(Qt::LeftDockWidgetArea, selectorDock);

        m_buildSystemOutput = new BuildSystemOutputWindow;
        auto output = new QWidget;
        // ProjectWindow sets background role to Base which is wrong for the output window,
        // especially the find tool bar (resulting in wrong label color)
        output->setBackgroundRole(QPalette::Window);
        output->setObjectName("BuildSystemOutput");
        output->setWindowTitle(Tr::tr("Build System Output"));
        auto outputLayout = new QVBoxLayout;
        output->setLayout(outputLayout);
        outputLayout->setContentsMargins(0, 0, 0, 0);
        outputLayout->setSpacing(0);
        outputLayout->addWidget(m_buildSystemOutput->toolBar());
        outputLayout->addWidget(m_buildSystemOutput);
        outputLayout->addWidget(new FindToolBarPlaceHolder(m_buildSystemOutput));
        m_outputDock = q->addDockForWidget(output, true);
        q->addDockWidget(Qt::RightDockWidgetArea, m_outputDock);

        m_toggleRightSidebarAction.setCheckable(true);
        m_toggleRightSidebarAction.setChecked(true);
        const auto toolTipText = [](bool checked) {
            return checked ? ::Core::Tr::tr(Core::Constants::TR_HIDE_RIGHT_SIDEBAR)
                           : ::Core::Tr::tr(Core::Constants::TR_SHOW_RIGHT_SIDEBAR);
        };
        m_toggleRightSidebarAction.setText(toolTipText(false)); // always "Show Right Sidebar"
        m_toggleRightSidebarAction.setToolTip(toolTipText(m_toggleRightSidebarAction.isChecked()));
        ActionManager::registerAction(&m_toggleRightSidebarAction,
                                      Core::Constants::TOGGLE_RIGHT_SIDEBAR,
                                      Context(Constants::C_PROJECTEXPLORER));
        connect(&m_toggleRightSidebarAction,
                &QAction::toggled,
                this,
                [this, toolTipText](bool checked) {
                    m_toggleRightSidebarAction.setToolTip(toolTipText(checked));
                    m_outputDock->setVisible(checked);
                });
    }

    void updatePanel()
    {
        ProjectItem *projectItem = m_projectsModel.rootItem()->childAt(0);
        if (!projectItem)
            return;
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
        ProjectManager::setStartupProject(project);
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

    void activateProjectPanel(Utils::Id panelId)
    {
        if (ProjectItem *projectItem = m_projectsModel.rootItem()->childAt(0)) {
            if (TreeItem *item = projectItem->itemForProjectPanel(panelId))
                itemActivated(item->index());
        }
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

        QAction *importBuild = menu.addAction(Tr::tr("Import Existing Build..."));
        importBuild->setEnabled(project && project->projectImporter());
        QAction *manageKits = menu.addAction(Tr::tr("Manage Kits..."));

        QAction *act = menu.exec(m_selectorTree->mapToGlobal(pos));

        if (act == importBuild)
            handleImportBuild();
        else if (act == manageKits)
            handleManageKits();
    }

    void handleManageKits()
    {
        const QModelIndexList selected = m_selectorTree->selectionModel()->selectedIndexes();
        if (!selected.isEmpty()) {
            TreeItem *treeItem = m_projectsModel.itemForIndex(selected.front());
            while (treeItem) {
                const Id kitId = Id::fromSetting(treeItem->data(0, KitIdRole));
                if (kitId.isValid()) {
                    setSelectectKitId(kitId);
                    break;
                }
                treeItem = treeItem->parent();
            }
        }
        ICore::showOptionsDialog(Constants::KITS_SETTINGS_PAGE_ID);
    }

    void handleImportBuild()
    {
        ProjectItem *projectItem = m_projectsModel.rootItem()->childAt(0);
        Project *project = projectItem ? projectItem->project() : nullptr;
        ProjectImporter *projectImporter = project ? project->projectImporter() : nullptr;
        QTC_ASSERT(projectImporter, return);

        FilePath importDir =
                FileUtils::getExistingDirectory(nullptr, Tr::tr("Import Directory"),
                                                project->projectDirectory());

        Target *lastTarget = nullptr;
        BuildConfiguration *lastBc = nullptr;
        for (const BuildInfo &info : projectImporter->import(importDir, false)) {
            Target *target = project->target(info.kitId);
            if (!target)
                target = project->addTargetForKit(KitManager::kit(info.kitId));
            if (target) {
                projectImporter->makePersistent(target->kit());
                BuildConfiguration *bc = info.factory->create(target, info);
                QTC_ASSERT(bc, continue);
                target->addBuildConfiguration(bc);

                lastTarget = target;
                lastBc = bc;
            }
        }
        if (lastTarget && lastBc) {
            lastTarget->setActiveBuildConfiguration(lastBc, SetActive::Cascade);
            project->setActiveTarget(lastTarget, SetActive::Cascade);
        }
    }

    void setPanel(QWidget *panel)
    {
        q->savePersistentSettings();
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
        q->loadPersistentSettings();
    }

    ProjectWindow *q;
    ProjectsModel m_projectsModel;
    ComboBoxModel m_comboBoxModel;
    SelectorDelegate m_selectorDelegate;
    QComboBox *m_projectSelection;
    SelectorTree *m_selectorTree;
    QPushButton *m_importBuild;
    QPushButton *m_manageKits;
    QAction m_toggleRightSidebarAction;
    QDockWidget *m_outputDock;
    BuildSystemOutputWindow *m_buildSystemOutput;
};

//
// ProjectWindow
//

ProjectWindow::ProjectWindow()
    : d(std::make_unique<ProjectWindowPrivate>(this))
{
    setBackgroundRole(QPalette::Base);

    // Request custom context menu but do not provide any to avoid
    // the creation of the dock window selection menu.
    setContextMenuPolicy(Qt::CustomContextMenu);
}

void ProjectWindow::activateProjectPanel(Utils::Id panelId)
{
    d->activateProjectPanel(panelId);
}

OutputWindow *ProjectWindow::buildSystemOutput() const
{
    return d->m_buildSystemOutput;
}

void ProjectWindow::hideEvent(QHideEvent *event)
{
    savePersistentSettings();
    FancyMainWindow::hideEvent(event);
}

void ProjectWindow::showEvent(QShowEvent *event)
{
    FancyMainWindow::showEvent(event);
    loadPersistentSettings();
}

ProjectWindow::~ProjectWindow() = default;

const char PROJECT_WINDOW_KEY[] = "ProjectExplorer.ProjectWindow";

void ProjectWindow::savePersistentSettings() const
{
    if (!centralWidget())
        return;
    QtcSettings * const settings = ICore::settings();
    settings->beginGroup(PROJECT_WINDOW_KEY);
    saveSettings(settings);
    settings->endGroup();
}

void ProjectWindow::loadPersistentSettings()
{
    if (!centralWidget())
        return;
    QtcSettings * const settings = ICore::settings();
    settings->beginGroup(PROJECT_WINDOW_KEY);
    restoreSettings(settings);
    settings->endGroup();
    d->m_toggleRightSidebarAction.setChecked(d->m_outputDock->isVisible());
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
