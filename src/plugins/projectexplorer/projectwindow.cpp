// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projectwindow.h"

#include "buildinfo.h"
#include "buildmanager.h"
#include "buildsettingspropertiespage.h"
#include "devicesupport/idevicefactory.h"
#include "kit.h"
#include "kitmanager.h"
#include "kitoptionspage.h"
#include "panelswidget.h"
#include "project.h"
#include "projectexplorer.h"
#include "projectexplorerconstants.h"
#include "projectexplorersettings.h"
#include "projectexplorertr.h"
#include "projectimporter.h"
#include "projectmanager.h"
#include "projectpanelfactory.h"
#include "projectsettingswidget.h"
#include "projectwindow.h"
#include "runsettingspropertiespage.h"
#include "target.h"
#include "targetsetuppage.h"
#include "task.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/coreicons.h>
#include <coreplugin/coreplugintr.h>
#include <coreplugin/find/optionspopup.h>
#include <coreplugin/findplaceholder.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/outputwindow.h>

#include <texteditor/fontsettings.h>
#include <texteditor/texteditorsettings.h>

#include <utils/algorithm.h>
#include <utils/basetreeview.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/navigationtreeview.h>
#include <utils/qtcassert.h>
#include <utils/qtcsettings.h>
#include <utils/qtcwidgets.h>
#include <utils/styledbar.h>
#include <utils/stylehelper.h>
#include <utils/treemodel.h>
#include <utils/utilsicons.h>

#include <QApplication>
#include <QComboBox>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QFileDialog>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QMetaObject>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QStyledItemDelegate>
#include <QTimer>
#include <QToolButton>
#include <QToolTip>
#include <QTreeView>
#include <QVBoxLayout>

#include <cmath>

using namespace Core;
using namespace Utils;

namespace ProjectExplorer::Internal {

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
    m_zoomIn.setText(ActionManager::command(Core::Constants::ZOOM_IN)->action()->text());
    connect(&m_zoomIn, &QAction::triggered, this, [this] { zoomIn(); });
    ActionManager::registerAction(&m_zoomIn,
                                  Core::Constants::ZOOM_IN,
                                  Context(kBuildSystemOutputContext));

    m_zoomOut.setIcon(Utils::Icons::MINUS_TOOLBAR.icon());
    m_zoomOut.setText(ActionManager::command(Core::Constants::ZOOM_OUT)->action()->text());
    connect(&m_zoomOut, &QAction::triggered, this, [this] { zoomOut(); });
    ActionManager::registerAction(&m_zoomOut,
                                  Core::Constants::ZOOM_OUT,
                                  Context(kBuildSystemOutputContext));
}

QWidget *BuildSystemOutputWindow::toolBar()
{
    if (!m_toolBar) {
        m_toolBar = new StyledBar(this);
        auto clearButton
            = Command::toolButtonWithAppendedShortcut(&m_clear, Core::Constants::OUTPUTPANE_CLEAR);

        m_filterOutputLineEdit = new FancyLineEdit;
        m_filterOutputLineEdit->setButtonVisible(FancyLineEdit::Left, true);
        m_filterOutputLineEdit->setButtonIcon(FancyLineEdit::Left, Utils::Icons::MAGNIFIER.icon());
        m_filterOutputLineEdit->setFiltering(true);
        m_filterOutputLineEdit->setHistoryCompleter("ProjectsMode.BuildSystemOutput.Filter");
        m_filterOutputLineEdit->setAttribute(Qt::WA_MacShowFocusRect, false);
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

        auto zoomInButton = Command::toolButtonWithAppendedShortcut(&m_zoomIn,
                                                                    Core::Constants::ZOOM_IN);
        auto zoomOutButton = Command::toolButtonWithAppendedShortcut(&m_zoomOut,
                                                                     Core::Constants::ZOOM_OUT);

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
                           m_invertFilterAction.isChecked(),
                           0 /* before context */,
                           0 /* after context */);
}

class ProjectPanel
{
public:
    ProjectPanel() = default;
    ProjectPanel(const QString &displayName, QWidget *widget)
        : displayName(displayName), widget(widget)
    {}

    QString displayName;
    QWidget *widget = nullptr;
};

using ProjectPanels = QList<ProjectPanel>;

// Overall structure:
// All items are derived from ProjectItemBase
//
// ProjectModel
//    ProjectModel::rootItem()
//       ProjectItem (one!)
//           TargetGroupItem
//               TargetItem
//               ...
//           VanishedTargetsGroupItem
//               VanishedTargetPanelItem
//               ...
//           MiscSettingsGroupItem
//               MiscSettingsPanelItem
//               ...

class ProjectItemBase : public TreeItem
{
public:
    ProjectItemBase() = default;

    ~ProjectItemBase() = default;

    // This item got activated through user interaction and
    // is now responsible for the central widget.
    virtual void itemActivatedDirectly() {}

    // A subitem got activated and gives us the opportunity to adjust.
    virtual void itemActivatedFromBelow(const ProjectItemBase * /*trigger*/) {}

    // A parent item got activated and makes us its active child.
    virtual void itemActivatedFromAbove() {}

    // A subitem got deactivated and gives us the opportunity to adjust.
    virtual void itemDeactivatedFromBelow() {}

    // A subitem got updated, re-expansion is necessary.
    virtual void itemUpdatedFromBelow() {}

    // The index of the currently selected item in the tree view.
    virtual ProjectItemBase *activeItem() { return nullptr; }

    // The kit id in case the item is associated with a kit.
    virtual Id kitId() const { return {}; }

    // This item's widget to be shown as central widget.
    virtual ProjectPanels panelWidgets() const { return {}; }

    // To augment a context menu
    virtual void addToMenu(QMenu * /*menu*/) const {}

    ProjectItemBase *parent() const { return static_cast<ProjectItemBase *>(TreeItem::parent()); }
    ProjectItemBase *childAt(int pos) const { return static_cast<ProjectItemBase *>(TreeItem::childAt(pos)); }
};

// Second level

//
// TargetGroupItem
//

class TargetItem;

class TargetGroupItem final : public ProjectItemBase
{
public:
    explicit TargetGroupItem(Project *project);
    ~TargetGroupItem() final;

    Qt::ItemFlags flags(int) const final { return Qt::NoItemFlags; }

    ProjectPanels panelWidgets() const final;
    ProjectItemBase *activeItem() final;
    void itemActivatedFromBelow(const ProjectItemBase *) final;
    void itemUpdatedFromBelow() final;

    TargetItem *currentTargetItem() const;
    TreeItem *buildSettingsItem() const;
    TreeItem *runSettingsItem() const;
    TargetItem *targetItem(Target *target) const;

    void scheduleRebuildContents();
    void rebuildContents();

private:
    const QPointer<Project> m_project;
    bool m_rebuildScheduled = false;

    mutable QPointer<PanelsWidget> m_targetSetupPage;
    QObject m_guard;
};

class VanishedTargetPanelItem final : public ProjectItemBase
{
public:
    VanishedTargetPanelItem(const Store &store, Project *project)
        : m_store(store)
        , m_project(project)
    {}

    QVariant data(int column, int role) const final;
    Qt::ItemFlags flags(int column) const final;

    void addToMenu(QMenu *menu) const;
    void itemActivatedDirectly();

protected:
    Store m_store;
    const QPointer<Project> m_project;
};

static QString deviceTypeDisplayName(const Store &store)
{
    Id deviceTypeId = Id::fromSetting(store.value(Target::deviceTypeKey()));
    if (!deviceTypeId.isValid())
        deviceTypeId = Constants::DESKTOP_DEVICE_TYPE;

    QString typeDisplayName = Tr::tr("Unknown device type");
    if (deviceTypeId.isValid()) {
        if (IDeviceFactory *factory = IDeviceFactory::find(deviceTypeId))
            typeDisplayName = factory->displayName();
    }
    return typeDisplayName;
}

static QString msgOptionsForRestoringSettings()
{
    return "<html>"
           + Tr::tr("The project was configured for kits that no longer exist. Select one of the "
                    "following options in the context menu to restore the project's settings:")
           + "<ul><li>"
           + Tr::tr("Create a new kit with the same name for the same device type, with the "
                    "original build, deploy, and run steps. Other kit settings are not restored.")
           + "</li><li>" + Tr::tr("Copy the build, deploy, and run steps to another kit.")
           + "</li></ul></html>";
}

QVariant VanishedTargetPanelItem::data(int column, int role) const
{
    Q_UNUSED(column)
    switch (role) {
    case Qt::DisplayRole:
        //: vanished target display role: vanished target name (device type name)
        return Tr::tr("%1 (%2)").arg(m_store.value(Target::displayNameKey()).toString(),
                                     deviceTypeDisplayName(m_store));
    case Qt::ToolTipRole:
        return msgOptionsForRestoringSettings();
    }

    return {};
}

void VanishedTargetPanelItem::addToMenu(QMenu *menu) const
{
    const int index = indexInParent();
    menu->addAction(Tr::tr("Create a New Kit"),
                    m_project.data(),
                    [index, store = m_store, project = m_project] {
        Target *t = project->createKitAndTargetFromStore(store);
        if (t) {
            project->setActiveTarget(t, SetActive::Cascade);
            project->removeVanishedTarget(index);
        }
    });
    QMenu *copyMenu = menu->addMenu(Tr::tr("Copy Steps to Another Kit"));
    const QList<Kit *> kits = KitManager::kits();
    for (Kit *kit : kits) {
        QAction *copyAction = copyMenu->addAction(kit->displayName());
        QObject::connect(copyAction,
                         &QAction::triggered,
                         [index, store = m_store, project = m_project, kit] {
            if (project->copySteps(store, kit))
                project->removeVanishedTarget(index);
        });
    }
    menu->addSeparator();
    menu->addAction(Tr::tr("Remove Vanished Target \"%1\"")
                        .arg(m_store.value(Target::displayNameKey()).toString()),
                    m_project.data(),
                    [index, project = m_project] { project->removeVanishedTarget(index); });
    menu->addAction(Tr::tr("Remove All Vanished Targets"),
                    m_project.data(),
                    [project = m_project] { project->removeAllVanishedTargets(); });
}

void VanishedTargetPanelItem::itemActivatedDirectly()
{
    QMenu menu;
    addToMenu(&menu);
    menu.exec(QCursor::pos());
}

Qt::ItemFlags VanishedTargetPanelItem::flags(int column) const
{
    Q_UNUSED(column)
    return Qt::ItemIsEnabled;
}

// The middle part of the second tree level, i.e. the list of vanished configured kits/targets.
class VanishedTargetsGroupItem : public ProjectItemBase
{
public:
    explicit VanishedTargetsGroupItem(Project *project)
        : m_project(project)
    {
        QTC_ASSERT(m_project, return);
        rebuild();
    }

    void rebuild()
    {
        removeChildren();
        for (const Store &store : m_project->vanishedTargets())
            appendChild(new VanishedTargetPanelItem(store, m_project));
    }

    Qt::ItemFlags flags(int) const final { return Qt::NoItemFlags; }

    QVariant data(int column, int role) const final
    {
        Q_UNUSED(column)
        switch (role) {
        case Qt::ToolTipRole:
            return msgOptionsForRestoringSettings();
        }
        return {};
    }

private:
    const QPointer<Project> m_project;
};

// Standard third level for the generic case: i.e. all except for the Build/Run page

class MiscSettingsPanelItem final : public ProjectItemBase
{
public:
    MiscSettingsPanelItem(ProjectPanelFactory *factory, Project *project)
        : m_factory(factory), m_project(project)
    {}

    ~MiscSettingsPanelItem() final { delete m_widget; }

    QVariant data(int column, int role) const final;
    Qt::ItemFlags flags(int column) const final;

    ProjectPanels panelWidgets() const final;

    ProjectPanelFactory *factory() const { return m_factory; }

    ProjectItemBase *activeItem() final;
    void itemActivatedDirectly() final;

protected:
    ProjectPanelFactory *m_factory = nullptr;
    const QPointer<Project> m_project;
    mutable QPointer<QWidget> m_widget = nullptr;
};

QVariant MiscSettingsPanelItem::data(int column, int role) const
{
    Q_UNUSED(column)
    if (role == Qt::DisplayRole) {
        if (m_factory)
            return m_factory->displayName();
    }
    return {};
}

ProjectPanels MiscSettingsPanelItem::panelWidgets() const
{
    if (!m_widget) {
        ProjectSettingsWidget *inner = m_factory->createWidget(m_project);
        m_widget = new PanelsWidget(m_factory->displayName(), inner);
        m_widget->setFocusProxy(inner);
    }
    ProjectPanel panel;
    panel.displayName = m_factory->displayName();
    panel.widget = m_widget;
    return QList{panel};
}

ProjectItemBase *MiscSettingsPanelItem::activeItem()
{
    return this; // We are the active one.
}

Qt::ItemFlags MiscSettingsPanelItem::flags(int column) const
{
    if (m_factory && m_project) {
        if (!m_factory->supports(m_project))
            return Qt::ItemIsSelectable;
    }
    return TreeItem::flags(column);
}

void MiscSettingsPanelItem::itemActivatedDirectly()
{
    // Bubble up
    return parent()->itemActivatedFromBelow(this);
}

// The lower part of the second tree level, i.e. the project settings list.
// The upper part is the TargetSettingsPanelItem .
class MiscSettingsGroupItem : public ProjectItemBase
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

    Qt::ItemFlags flags(int) const final
    {
        return Qt::NoItemFlags;
    }

    ProjectPanels panelWidgets() const final
    {
        if (0 <= m_currentPanelIndex && m_currentPanelIndex < childCount())
            return childAt(m_currentPanelIndex)->panelWidgets();
        return {};
    }

    ProjectItemBase *activeItem() final
    {
        if (0 <= m_currentPanelIndex && m_currentPanelIndex < childCount())
            return childAt(m_currentPanelIndex)->activeItem();
        return nullptr;
    }

    void itemActivatedFromBelow(const ProjectItemBase *trigger) final
    {
        m_currentPanelIndex = indexOf(trigger);
        QTC_ASSERT(m_currentPanelIndex != -1, return);
        parent()->itemActivatedFromBelow(this);
    }

    Project *project() const { return m_project; }

private:
    int m_currentPanelIndex = -1;

    const QPointer<Project> m_project;
};

// The first tree level, i.e. projects.
class ProjectItem : public ProjectItemBase
{
public:
    ProjectItem() = default;

    ProjectItem(Project *project, const std::function<void()> &changeListener)
        : m_project(project), m_changeListener(changeListener)
    {
        QTC_ASSERT(m_project, return);
        appendChild(m_targetsItem = new TargetGroupItem(m_project));
        appendChild(m_vanishedTargetsItem = new VanishedTargetsGroupItem(m_project));
        appendChild(m_miscItem = new MiscSettingsGroupItem(m_project));
        QObject::connect(
            m_project,
            &Project::vanishedTargetsChanged,
            &m_guard,
            [this] { m_vanishedTargetsItem->rebuild(); },
            Qt::QueuedConnection /* this is triggered by a child item, so queue */);
    }

    QVariant data(int column, int role) const final
    {
        Q_UNUSED(column);
        switch (role) {
        case Qt::DisplayRole:
            return m_project->displayName();

        case Qt::FontRole: {
            QFont font;
            font.setBold(m_project == ProjectManager::startupProject());
            return font;
        }
        }

        return {};
    }

    ProjectPanels panelWidgets() const final
    {
        if (ProjectItemBase *child = childAt(m_currentChildIndex))
            return child->panelWidgets();
        return {};
    }

    ProjectItemBase *activeItem() final
    {
        if (ProjectItemBase *child = childAt(m_currentChildIndex))
            return child->activeItem();
        return nullptr;
    }

    void itemUpdatedFromBelow() final
    {
        announceChange();
    }

    void itemDeactivatedFromBelow() final
    {
        announceChange();
    }

    void itemActivatedFromBelow(const ProjectItemBase *item) final
    {
        QTC_ASSERT(item, return);
        int res = indexOf(item);
        QTC_ASSERT(res >= 0, return);
        m_currentChildIndex = res;
        announceChange();
    }

    void itemActivatedFromAbove() final
    {
        // Someone selected the project using the combobox or similar.
        ProjectManager::setStartupProject(m_project);
        m_currentChildIndex = 0; // Use some Target page by defaults
        m_targetsItem->itemActivatedFromAbove(); // And propagate downwards.
        announceChange();
    }

    void announceChange()
    {
        m_changeListener();
    }

    Project *project() const { return m_project; }

    TreeItem *itemForProjectPanel(Utils::Id panelId)
    {
        return m_miscItem->findChildAtLevel(1, [panelId](const TreeItem *item){
            return static_cast<const MiscSettingsPanelItem *>(item)->factory()->id() == panelId;
        });
    }

    TreeItem *activeBuildSettingsItem() const { return m_targetsItem->buildSettingsItem(); }
    TreeItem *activeRunSettingsItem() const { return m_targetsItem->runSettingsItem(); }

    TargetGroupItem *targetsItem() const { return m_targetsItem; }

private:
    QObject m_guard;
    int m_currentChildIndex = 0; // Start with Build & Run.
    Project *m_project = nullptr;
    TargetGroupItem *m_targetsItem = nullptr;
    VanishedTargetsGroupItem *m_vanishedTargetsItem = nullptr;
    MiscSettingsGroupItem *m_miscItem = nullptr;
    const std::function<void ()> m_changeListener;
};

class TargetSetupPageWrapper : public QWidget
{
public:
    explicit TargetSetupPageWrapper(Project *project);

    ~TargetSetupPageWrapper()
    {
        QTC_CHECK(m_targetSetupPage);
    }

protected:
    void keyReleaseEvent(QKeyEvent *event) final
    {
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
            event->accept();
    }

    void keyPressEvent(QKeyEvent *event) final
    {
        if ((m_targetSetupPage && m_targetSetupPage->importLineEditHasFocus())
                || (m_configureButton && !m_configureButton->isEnabled())) {
            return;
        }
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
            event->accept();
            if (m_targetSetupPage)
                done();
        }
    }

private:
    void done()
    {
        QTC_ASSERT(m_targetSetupPage, return);
        m_targetSetupPage->disconnect();
        m_targetSetupPage->setupProject(m_project);
        m_targetSetupPage->deleteLater();
        m_targetSetupPage = nullptr;
        Core::ModeManager::activateMode(Core::Constants::MODE_EDIT);
    }

    void completeChanged()
    {
        m_configureButton->setEnabled(m_targetSetupPage && m_targetSetupPage->isComplete());
    }

    QPointer<Project> m_project;
    QPointer<TargetSetupPage> m_targetSetupPage;
    QPushButton *m_configureButton = nullptr;
    QVBoxLayout *m_setupPageContainer = nullptr;
};

TargetSetupPageWrapper::TargetSetupPageWrapper(Project *project)
    : m_project(project)
{
    auto box = new QDialogButtonBox(this);

    m_configureButton = new QPushButton(this);
    m_configureButton->setText(Tr::tr("&Configure Project"));
    box->addButton(m_configureButton, QDialogButtonBox::AcceptRole);

    auto hbox = new QHBoxLayout;
    hbox->addStretch();
    hbox->addWidget(box);

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    m_setupPageContainer = new QVBoxLayout;
    layout->addLayout(m_setupPageContainer);
    layout->addLayout(hbox);

    m_targetSetupPage = new TargetSetupPage(this);
    m_targetSetupPage->setProjectPath(m_project->projectFilePath());
    m_targetSetupPage->setTasksGenerator([this](const Kit *k) {
        QTC_ASSERT(m_project.get(), return Tasks());
        return m_project->projectIssues(k);
    });
    m_targetSetupPage->setProjectImporter(m_project->projectImporter());
    m_targetSetupPage->initializePage();
    m_targetSetupPage->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    m_setupPageContainer->addWidget(m_targetSetupPage);

    completeChanged();

    connect(m_configureButton, &QAbstractButton::clicked,
            this, &TargetSetupPageWrapper::done);

    connect(m_targetSetupPage, &QWizardPage::completeChanged,
            this, &TargetSetupPageWrapper::completeChanged);
}

//
// Third level: The per-kit entries
//
class TargetItem final : public ProjectItemBase
{
public:
    TargetItem(Project *project, Id kitId, const Tasks &issues)
        : m_project(project), m_kitId(kitId), m_kitIssues(issues)
    {
        m_kitWarningForProject = containsType(m_kitIssues, Task::TaskType::Warning);
        m_kitErrorsForProject = containsType(m_kitIssues, Task::TaskType::Error);
    }

    ~TargetItem()
    {
        delete m_buildSettingsWidget;
        delete m_runSettingsWidget;
    }

    Target *target() const
    {
        return m_project->target(m_kitId);
    }

    Id kitId() const
    {
        return m_kitId;
    }

    Qt::ItemFlags flags(int column) const final
    {
        Q_UNUSED(column)
        return m_kitErrorsForProject ? Qt::ItemFlags({})
                                     : Qt::ItemFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    }

    QVariant data(int column, int role) const final
    {
        switch (role) {
        case Qt::DisplayRole: {
            if (Kit *kit = KitManager::kit(m_kitId))
                return kit->displayName();
            break;
        }

        case Qt::DecorationRole: {
            const Kit *k = KitManager::kit(m_kitId);
            if (!k)
                break;
            if (m_kitErrorsForProject)
                return kitIconWithOverlay(*k, IconOverlay::Error);
            if (!isEnabled())
                return kitIconWithOverlay(*k, IconOverlay::Add);
            if (m_kitWarningForProject)
                return kitIconWithOverlay(*k, IconOverlay::Warning);
            return k->icon();
        }

        case Qt::ForegroundRole: {
            if (!isEnabled())
                return Utils::creatorColor(Theme::TextColorDisabled);
            break;
        }

        case Qt::FontRole: {
            QFont font = parent()->data(column, role).value<QFont>();
            if (TargetItem *targetItem = static_cast<TargetGroupItem *>(parent())->currentTargetItem()) {
                Target *t = targetItem->target();
                if (t && t->id() == m_kitId && m_project == ProjectManager::startupProject())
                    font.setBold(true);
            }
            return font;
        }

        case Qt::ToolTipRole: {
            Kit *k = KitManager::kit(m_kitId);
            if (!k)
                break;
            const QString extraText = [this] {
                if (m_kitErrorsForProject)
                    return QString("<h3>" + Tr::tr("Kit is unsuited for project") + "</h3>");
                if (!isEnabled())
                    return QString("<h3>" + Tr::tr("Click to activate") + "</h3>");
                return QString();
            }();
            return k->toHtml(m_kitIssues, extraText);
        }
        default:
            break;
        }
        return {};
    }

    ProjectPanels panelWidgets() const final
    {
        if (!m_buildSettingsWidget)
            m_buildSettingsWidget = new PanelsWidget(Tr::tr("Build Settings"), createBuildSettingsWidget(target()));
        if (!m_runSettingsWidget)
            m_runSettingsWidget = new PanelsWidget(Tr::tr("Run Settings"), createRunSettingsWidget(target()));

        return {
            ProjectPanel(Tr::tr("Build Settings"), m_buildSettingsWidget),
            ProjectPanel(Tr::tr("Run Settings"), m_runSettingsWidget)
        };
    }

    void addToMenu(QMenu *menu) const final
    {
        addToContextMenu(menu, flags(/*column = */ 0) & Qt::ItemIsSelectable);
    }

    void itemActivatedDirectly() final
    {
        if (!isEnabled()) {
            m_project->addTargetForKit(KitManager::kit(m_kitId));
        } else {
            // Go to Run page, when on Run previously etc.
            m_project->setActiveTarget(target(), SetActive::Cascade);
            parent()->itemActivatedFromBelow(this);
        }
    }

    void itemActivatedFromAbove() final
    {
        // Usually programmatic activation, e.g. after opening the Project mode.
        m_project->setActiveTarget(target(), SetActive::Cascade);
    }

    void addToContextMenu(QMenu *menu, bool isSelectable) const
    {
        Kit *kit = KitManager::kit(m_kitId);
        QTC_ASSERT(kit, return);
        const QString projectName = m_project->displayName();

        QAction *enableAction = menu->addAction(Tr::tr("Enable Kit for Project \"%1\"").arg(projectName));
        enableAction->setEnabled(isSelectable && m_kitId.isValid() && !isEnabled());
        QObject::connect(enableAction, &QAction::triggered, [this, kit] {
            m_project->addTargetForKit(kit);
        });

        QAction * const enableForAllAction
                = menu->addAction(Tr::tr("Enable Kit for All Projects"));
        enableForAllAction->setEnabled(isSelectable);
        QObject::connect(enableForAllAction, &QAction::triggered, [kit] {
            for (Project * const p : ProjectManager::projects()) {
                if (!p->target(kit))
                    p->addTargetForKit(kit);
            }
        });

        QAction *disableAction = menu->addAction(Tr::tr("Disable Kit for Project \"%1\"").arg(projectName));
        disableAction->setEnabled(isSelectable && m_kitId.isValid() && isEnabled());
        QObject::connect(disableAction, &QAction::triggered, m_project, [this] {
            Target *t = target();
            QTC_ASSERT(t, return);
            QString kitName = t->displayName();
            if (BuildManager::isBuilding(t)) {
                QMessageBox box;
                QPushButton *closeAnyway = box.addButton(Tr::tr("Cancel Build and Disable Kit in This Project"), QMessageBox::AcceptRole);
                QPushButton *cancelClose = box.addButton(Tr::tr("Do Not Remove"), QMessageBox::RejectRole);
                box.setDefaultButton(cancelClose);
                box.setWindowTitle(Tr::tr("Disable Kit \"%1\" in This Project?").arg(kitName));
                box.setText(Tr::tr("The kit <b>%1</b> is currently being built.").arg(kitName));
                box.setInformativeText(Tr::tr("Do you want to cancel the build process and remove the kit anyway?"));
                box.exec();
                if (box.clickedButton() != closeAnyway)
                    return;
                BuildManager::cancel();
            }

            QCoreApplication::processEvents();

            m_project->removeTarget(t);
        });

        QAction *disableForAllAction = menu->addAction(Tr::tr("Disable Kit for All Projects"));
        disableForAllAction->setEnabled(isSelectable);
        QObject::connect(disableForAllAction, &QAction::triggered, [kit] {
            for (Project * const p : ProjectManager::projects()) {
                Target * const t = p->target(kit);
                if (!t)
                    continue;
                if (BuildManager::isBuilding(t))
                    BuildManager::cancel();
                p->removeTarget(t);
            }
        });

        QMenu *copyMenu = menu->addMenu(Tr::tr("Copy Steps From Another Kit..."));
        if (m_kitId.isValid() && m_project->target(m_kitId)) {
            const QList<Kit *> kits = KitManager::kits();
            for (Kit *kit : kits) {
                QAction *copyAction = copyMenu->addAction(kit->displayName());
                if (kit->id() == m_kitId || !m_project->target(kit->id())) {
                    copyAction->setEnabled(false);
                } else {
                    QObject::connect(copyAction, &QAction::triggered, [this, kit] {
                        Target *thisTarget = m_project->target(m_kitId);
                        Target *sourceTarget = m_project->target(kit->id());
                        Project::copySteps(sourceTarget, thisTarget);
                    });
                }
            }
        } else {
            copyMenu->setEnabled(false);
        }
    }

private:
    enum class IconOverlay {
        Add,
        Warning,
        Error
    };

    static QIcon kitIconWithOverlay(const Kit &kit, IconOverlay overlayType)
    {
        QIcon overlayIcon;
        switch (overlayType) {
        case IconOverlay::Add: {
            static const QIcon add = Utils::Icons::OVERLAY_ADD.icon();
            overlayIcon = add;
            break;
        }
        case IconOverlay::Warning: {
            static const QIcon warning = Utils::Icons::OVERLAY_WARNING.icon();
            overlayIcon = warning;
            break;
        }
        case IconOverlay::Error: {
            static const QIcon err = Utils::Icons::OVERLAY_ERROR.icon();
            overlayIcon = err;
            break;
        }
        }
        const QSize iconSize(16, 16);
        const QRect iconRect(QPoint(), iconSize);
        QPixmap result(iconSize * qApp->devicePixelRatio());
        result.fill(Qt::transparent);
        result.setDevicePixelRatio(qApp->devicePixelRatio());
        QPainter p(&result);
        kit.icon().paint(&p, iconRect, Qt::AlignCenter,
                         overlayType == IconOverlay::Add ? QIcon::Disabled : QIcon::Normal);
        overlayIcon.paint(&p, iconRect);
        return result;
    }

    bool isEnabled() const { return target() != nullptr; }

public:
    QPointer<Project> m_project; // Not owned.

    Id m_kitId;
    bool m_kitErrorsForProject = false;
    bool m_kitWarningForProject = false;
    Tasks m_kitIssues;

    mutable QPointer<QWidget> m_buildSettingsWidget;
    mutable QPointer<QWidget> m_runSettingsWidget;
};

//
// Also third level:
//

TargetGroupItem::TargetGroupItem(Project *project)
    : m_project(project)
{
    QObject::connect(project, &Project::addedTarget, &m_guard, [this] { update(); });

    QObject::connect(project, &Project::removedTarget, &m_guard, [this] {
        QTC_ASSERT(parent(), return);
        parent()->itemDeactivatedFromBelow();
    });

    QObject::connect(project, &Project::activeTargetChanged, &m_guard, [this] {
        parent()->itemActivatedFromBelow(this);
    });

    // force a signal since the index has changed
    QObject::connect(KitManager::instance(), &KitManager::kitAdded, &m_guard, [this](Kit *kit) {
        appendChild(new TargetItem(m_project, kit->id(), m_project->projectIssues(kit)));
        scheduleRebuildContents();
    });

    QObject::connect(KitManager::instance(), &KitManager::kitRemoved, &m_guard, [this] {
         scheduleRebuildContents();
    });
    QObject::connect(KitManager::instance(), &KitManager::kitUpdated, &m_guard, [this] {
         scheduleRebuildContents();
    });
    QObject::connect(KitManager::instance(), &KitManager::kitsLoaded, &m_guard, [this] {
         scheduleRebuildContents();
    });

    QObject::connect(ProjectExplorerPlugin::instance(),
                     &ProjectExplorerPlugin::settingsChanged, &m_guard, [this] {
        scheduleRebuildContents();
    });

    rebuildContents();
}

TargetGroupItem::~TargetGroupItem()
{
    delete m_targetSetupPage;
}

ProjectItemBase *TargetGroupItem::activeItem()
{
    if (TargetItem *item = currentTargetItem())
        return item->activeItem();
    return this;
}

ProjectPanels TargetGroupItem::panelWidgets() const
{
    if (TargetItem *item = currentTargetItem())
        return item->panelWidgets();

    if (!m_targetSetupPage) {
        auto inner = new TargetSetupPageWrapper(m_project);
        m_targetSetupPage = new PanelsWidget(Tr::tr("Configure Project"), inner, false);
        m_targetSetupPage->setFocusProxy(inner);
    }

    ProjectPanel panel;
    panel.displayName = Tr::tr("Configure Project");
    panel.widget = m_targetSetupPage;

    return {panel};
}

void TargetGroupItem::itemActivatedFromBelow(const ProjectItemBase *)
{
    parent()->itemActivatedFromBelow(this);
}

void TargetGroupItem::itemUpdatedFromBelow()
{
    // Bubble up to trigger setting the active project.
    QTC_ASSERT(parent(), return);
    parent()->itemUpdatedFromBelow();
}

TargetItem *TargetGroupItem::currentTargetItem() const
{
    return targetItem(m_project->activeTarget());
}

TreeItem *TargetGroupItem::buildSettingsItem() const
{
    if (TargetItem * const targetItem = currentTargetItem()) {
        if (targetItem->childCount() == 2)
            return targetItem->childAt(0);
    }
    return nullptr;
}

TreeItem *TargetGroupItem::runSettingsItem() const
{
    if (TargetItem * const targetItem = currentTargetItem()) {
        if (targetItem->hasChildren())
            return targetItem->childAt(targetItem->childCount() - 1);
    }
    return nullptr;
}

TargetItem *TargetGroupItem::targetItem(Target *target) const
{
    if (target) {
        const Id needle = target->id(); // Unconfigured project have no active target.
        for (int i = 0, n = childCount(); i != n; ++i) {
            ProjectItemBase *child = childAt(i);
            if (child->kitId() == needle)
                return static_cast<TargetItem *>(child);
        }
    }
    return nullptr;
}

void TargetGroupItem::scheduleRebuildContents()
{
    if (m_rebuildScheduled)
        return;
    m_rebuildScheduled = true;
    QMetaObject::invokeMethod(&m_guard, [this] { rebuildContents(); }, Qt::QueuedConnection);
}

void TargetGroupItem::rebuildContents()
{
    m_rebuildScheduled = false;
    QGuiApplication::setOverrideCursor(Qt::WaitCursor);
    const auto sortedKits = KitManager::sortedKits();
    bool isAnyKitNotEnabled = std::any_of(sortedKits.begin(), sortedKits.end(), [this](Kit *kit) {
        return kit && m_project->target(kit->id()) != nullptr;
    });
    removeChildren();

    for (Kit *kit : sortedKits) {
        if (!isAnyKitNotEnabled || projectExplorerSettings().showAllKits || m_project->target(kit->id()) != nullptr)
            appendChild(new TargetItem(m_project, kit->id(), m_project->projectIssues(kit)));
    }

    if (parent())
        parent()->itemUpdatedFromBelow();

    QGuiApplication::restoreOverrideCursor();
}

//
// SelectorTree
//

class SelectorDelegate : public QStyledItemDelegate
{
public:
    SelectorDelegate() = default;

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const final
    {
        QSize s = QStyledItemDelegate::sizeHint(option, index);
        return QSize(s.width(), s.height() * 1.2);
    }
};

class SelectorTree : public TreeView
{
public:
    SelectorTree()
    {
        setWindowTitle("Project Kit Selector");
        setSizeAdjustPolicy(QAbstractItemView::SizeAdjustPolicy::AdjustToContents);
        setFrameStyle(QFrame::NoFrame);
        setItemDelegate(&m_selectorDelegate);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
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

    QSize minimumSizeHint() const final
    {
        return {10, 10};
    }

    void updateSize()
    {
        resizeColumnToContents(0);
        updateGeometry();
    }

private:
    // remove branch indicators
    void drawBranches(QPainter *, const QRect &, const QModelIndex &) const final
    {
        return;
    }

    bool userWantsContextMenu(const QMouseEvent *e) const final
    {
        // On Windows, we get additional mouse events for the item view when right-clicking,
        // causing unwanted kit activation (QTCREATORBUG-24156). Let's suppress these.
        return HostOsInfo::isWindowsHost() && e->button() == Qt::RightButton;
    }

    SelectorDelegate m_selectorDelegate;
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

class ProjectWindowTabWidget : public QTabWidget
{
public:
    ProjectWindowTabWidget(QWidget *parent = nullptr)
        : QTabWidget(parent)
    {
        setTabBar(new QtcTabBar); // Must be the first called setter!
        setDocumentMode(true);
        setTabBarAutoHide(true);
    }
};

class CentralWidget : public QWidget
{
public:
    explicit CentralWidget(QWidget *parent)
        : QWidget(parent)
    {
        m_tabWidget = new ProjectWindowTabWidget(this);

        auto layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        layout->addWidget(new StyledBar(this));
        layout->addWidget(m_tabWidget);
    }

    void setPanels(const ProjectPanels &panels, bool setFocus)
    {
        const int oldIndex = m_tabWidget->currentIndex();

        while (m_tabWidget->count()) {
            const int pos = m_tabWidget->count() - 1;
            m_tabWidget->removeTab(pos);
        }

        for (const ProjectPanel &panel : panels) {
            QTC_ASSERT(panel.widget, continue);
            m_tabWidget->addTab(panel.widget, panel.displayName);
        }

        m_tabWidget->setCurrentIndex(oldIndex);

        if (QWidget *widget = m_tabWidget->currentWidget()) {
            if (setFocus)
                widget->setFocus();
        }
    }

private:
    QTabWidget *m_tabWidget = nullptr;
};

class ShowAllKitsButton final : public QLabel
{
public:
    ShowAllKitsButton(ProjectsModel *projectsModel)
        : m_projectsModel(projectsModel)
    {}

    void mouseReleaseEvent(QMouseEvent *) final
    {
        const bool newShowAllKits = !projectExplorerSettings().showAllKits;
        mutableProjectExplorerSettings().showAllKits = newShowAllKits;
        QtcSettings *settings = Core::ICore::settings();
        settings->setValue(ProjectExplorer::Constants::SHOW_ALL_KITS_SETTINGS_KEY, newShowAllKits);
        updateText();
        m_projectsModel->rootItem()->forFirstLevelChildren([](ProjectItem *item) {
            item->targetsItem()->scheduleRebuildContents();
        });
    }

    void updateText()
    {
        setText(projectExplorerSettings().showAllKits ? Tr::tr("Hide Inactive Kits") : Tr::tr("Show All Kits"));
    }

    ProjectsModel *m_projectsModel;
};

class ProjectWindowPrivate : public QObject
{
public:
    ProjectWindowPrivate(ProjectWindow *parent)
        : q(parent), m_centralWidget(new CentralWidget(q))
    {
        q->setCentralWidget(m_centralWidget);

        m_projectsModel.setHeader({Tr::tr("Projects")});

        m_showAllKitsButton = new ShowAllKitsButton(&m_projectsModel);

        m_targetsView = new SelectorTree;
        m_targetsView->setModel(&m_projectsModel);
        m_targetsView->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(m_targetsView, &QAbstractItemView::activated,
                this, &ProjectWindowPrivate::itemActivated);
        connect(m_targetsView, &QWidget::customContextMenuRequested,
                this, &ProjectWindowPrivate::openContextMenu);

        m_vanishedTargetsView = new SelectorTree;
        m_vanishedTargetsView->setModel(&m_projectsModel);
        m_vanishedTargetsView->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(m_vanishedTargetsView, &QAbstractItemView::activated,
                this, &ProjectWindowPrivate::itemActivated);
        connect(m_vanishedTargetsView, &QWidget::customContextMenuRequested,
                this, &ProjectWindowPrivate::openContextMenu);

        m_projectSettingsView = new SelectorTree;
        m_projectSettingsView->setModel(&m_projectsModel);
        m_projectSettingsView->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(m_projectSettingsView, &QAbstractItemView::activated,
                this, &ProjectWindowPrivate::itemActivated);
        connect(m_projectSettingsView, &QWidget::customContextMenuRequested,
                this, &ProjectWindowPrivate::openContextMenu);

        const QFont labelFont = StyleHelper::uiFont(StyleHelper::UiElementH4);

        auto targetsLabel = new QLabel(Tr::tr("Build & Run"));
        targetsLabel->setFont(labelFont);

        m_vanishedTargetsLabel = new QLabel(Tr::tr("Vanished Targets"));
        m_vanishedTargetsLabel->setFont(labelFont);

        auto projectSettingsLabel = new QLabel(Tr::tr("Project Settings"));
        projectSettingsLabel->setFont(labelFont);

        const int space = 18;
        auto scrolledWidget = new QWidget;
        auto scrolledLayout = new QVBoxLayout(scrolledWidget);
        scrolledLayout->setSizeConstraint(QLayout::SetFixedSize);
        scrolledLayout->setSpacing(0);
        scrolledLayout->addWidget(targetsLabel);
        scrolledLayout->addSpacing(space);
        scrolledLayout->addWidget(m_targetsView);
        scrolledLayout->addSpacing(6);
        scrolledLayout->addWidget(m_showAllKitsButton);
        scrolledLayout->addSpacing(space);
        scrolledLayout->addWidget(m_vanishedTargetsLabel);
        scrolledLayout->addSpacing(space);
        scrolledLayout->addWidget(m_vanishedTargetsView);
        scrolledLayout->addSpacing(space);
        scrolledLayout->addWidget(projectSettingsLabel);
        scrolledLayout->addSpacing(space);
        scrolledLayout->addWidget(m_projectSettingsView);

        m_scrollArea = new QScrollArea;
        m_scrollArea->setFrameStyle(QFrame::NoFrame);
        m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        m_scrollArea->setWidgetResizable(true);
        m_scrollArea->setWidget(scrolledWidget);

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

        auto styledBar = new StyledBar; // The black blob on top of the side bar
        styledBar->setObjectName("ProjectModeStyledBar");

        auto selectorView = new QWidget; // Black blob + Combobox + Project tree below.
        selectorView->setObjectName("ProjectSelector"); // Needed for dock widget state saving
        selectorView->setWindowTitle(Tr::tr("Project Selector"));
        selectorView->setAutoFillBackground(true);

        auto activeLabel = new QLabel(Tr::tr("Active Project"));
        activeLabel->setFont(StyleHelper::uiFont(StyleHelper::UiElementH4));

        auto innerLayout = new QVBoxLayout;
        innerLayout->setSpacing(10);
        innerLayout->setContentsMargins(PanelsWidget::PanelVMargin, innerLayout->spacing(),
                                        PanelsWidget::PanelVMargin, 0);
#ifdef QT_NO_DEBUG
        const QStringList list = Core::ICore::settings()->value("HideOptionCategories").toStringList();
#else
        const QStringList list;
#endif
        if (!list.contains("Kits")) {
            auto manageKits = new QPushButton(Tr::tr("Manage Kits..."));
            connect(manageKits, &QPushButton::clicked,
                    this, &ProjectWindowPrivate::handleManageKits);

            innerLayout->addWidget(manageKits);
            innerLayout->addSpacerItem(new QSpacerItem(10, 30, QSizePolicy::Maximum, QSizePolicy::Maximum));
        }
        innerLayout->addWidget(activeLabel);
        innerLayout->addWidget(m_projectSelection);
        innerLayout->addWidget(m_importBuild);
        innerLayout->addWidget(m_scrollArea);

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

        setPanels(projectItem->panelWidgets());

        m_targetsView->selectionModel()->clear();
        QModelIndex sel;
        if (ProjectItemBase *active = projectItem->activeItem())
            sel = active->index();
        m_targetsView->selectionModel()->select(sel, QItemSelectionModel::Select);

        m_targetsView->updateSize();
        m_vanishedTargetsView->updateSize();
        m_projectSettingsView->updateSize();

        m_showAllKitsButton->updateText();
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

    ProjectItemBase *projectItemForIndex(const QModelIndex &index)
    {
        return static_cast<ProjectItemBase *>(m_projectsModel.itemForIndex(index));
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

        const QModelIndex root = m_projectsModel.index(0, 0, QModelIndex());
        m_targetsView->setRootIndex(m_projectsModel.index(0, 0, root));
        m_vanishedTargetsView->setRootIndex(m_projectsModel.index(1, 0, root));
        m_projectSettingsView->setRootIndex(m_projectsModel.index(2, 0, root));

        const QModelIndex vanishedTargetsRoot = m_projectsModel.index(1, 0, root);
        const bool hasVanishedTargets = m_projectsModel.hasChildren(vanishedTargetsRoot);
        m_vanishedTargetsLabel->setVisible(hasVanishedTargets);
        m_vanishedTargetsView->setVisible(hasVanishedTargets);

        updatePanel();
    }

    void itemActivated(const QModelIndex &index)
    {
        if (ProjectItemBase *item = projectItemForIndex(index))
            item->itemActivatedDirectly();
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

        QModelIndex index = m_targetsView->indexAt(pos);
        if (ProjectItemBase *item = projectItemForIndex(index))
            item->addToMenu(&menu);

        if (!menu.actions().isEmpty())
            menu.addSeparator();

        QAction *importBuild = menu.addAction(Tr::tr("Import Existing Build..."));
        importBuild->setEnabled(project && project->projectImporter());
        QAction *manageKits = menu.addAction(Tr::tr("Manage Kits..."));

        QAction *act = menu.exec(m_targetsView->mapToGlobal(pos));

        if (act == importBuild)
            handleImportBuild();
        else if (act == manageKits)
            handleManageKits();
    }

    void handleManageKits()
    {
        const QModelIndexList selected = m_targetsView->selectionModel()->selectedIndexes();
        if (!selected.isEmpty()) {
            ProjectItemBase *treeItem = projectItemForIndex(selected.front());
            while (treeItem) {
                if (const Id kitId = treeItem->kitId(); kitId.isValid()) {
                    Core::setPreselectedOptionsPageItem(Constants::KITS_SETTINGS_PAGE_ID, kitId);
                    break;
                }
                treeItem = treeItem->parent();
            }
        }
        ICore::showOptionsDialog(Constants::KITS_SETTINGS_PAGE_ID);
    }

    void handleImportBuild()
    {
        ProjectItem *projectItem = static_cast<ProjectItem *>(m_projectsModel.rootItem()->childAt(0));
        Project *project = projectItem ? projectItem->project() : nullptr;
        ProjectImporter *projectImporter = project ? project->projectImporter() : nullptr;
        QTC_ASSERT(projectImporter, return);

        FilePath importDir =
                FileUtils::getExistingDirectory(Tr::tr("Import Directory"),
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

    void setPanels(const ProjectPanels &panels)
    {
         q->savePersistentSettings();
         m_centralWidget->setPanels(panels, q->hasFocus());
         q->loadPersistentSettings();
    }

    ProjectWindow *q;
    ProjectsModel m_projectsModel;
    ComboBoxModel m_comboBoxModel;
    QComboBox *m_projectSelection;
    QLabel *m_vanishedTargetsLabel;
    SelectorTree *m_targetsView;
    SelectorTree *m_vanishedTargetsView;
    SelectorTree *m_projectSettingsView;
    ShowAllKitsButton *m_showAllKitsButton;
    QScrollArea *m_scrollArea;
    QPushButton *m_importBuild;
    QAction m_toggleRightSidebarAction;
    QDockWidget *m_outputDock;
    BuildSystemOutputWindow *m_buildSystemOutput;
    CentralWidget *m_centralWidget;
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

void ProjectWindow::activateBuildSettings()
{
    if (ProjectItem *projectItem = d->m_projectsModel.rootItem()->childAt(0)) {
        if (TreeItem *item = projectItem->activeBuildSettingsItem())
            d->itemActivated(item->index());
    }
}

void ProjectWindow::activateRunSettings()
{
    if (ProjectItem *projectItem = d->m_projectsModel.rootItem()->childAt(0)) {
        if (TreeItem *item = projectItem->activeRunSettingsItem())
            d->itemActivated(item->index());
    }
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

} // namespace ProjectExplorer::Internal
