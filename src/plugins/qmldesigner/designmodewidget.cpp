// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "designmodewidget.h"

#include <designeractionmanager.h>

#include "qmldesignerplugin.h"
#include "crumblebar.h"

#include <texteditor/textdocument.h>
#include <nodeinstanceview.h>
#include <itemlibrarywidget.h>
#include <theme.h>
#include <toolbar.h>

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actionmanager_p.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/modemanager.h>
#include <qmldesigner/qmldesignerconstants.h>
#include <qmldesignerbase/qmldesignerbaseplugin.h>

#include <coreplugin/outputpane.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/designmode.h>
#include <coreplugin/icore.h>
#include <coreplugin/minisplitter.h>
#include <coreplugin/sidebar.h>
#include <coreplugin/editortoolbar.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/idocument.h>
#include <coreplugin/inavigationwidgetfactory.h>

#include <qmlprojectmanager/qmlproject.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/stylehelper.h>
#include <utils/transientscroll.h>

#include <QActionGroup>
#include <QApplication>
#include <QBoxLayout>
#include <QComboBox>
#include <QDir>
#include <QSettings>
#include <QToolBar>

#include <advanceddockingsystem/dockareawidget.h>
#include <advanceddockingsystem/docksplitter.h>
#include <advanceddockingsystem/iconprovider.h>

using Core::MiniSplitter;
using Core::IEditor;
using Core::EditorManager;

using namespace QmlDesigner;

enum {
    debug = false
};

static void hideToolButtons(QList<QToolButton*> &buttons)
{
    for (QToolButton *button : buttons)
        button->hide();
}

namespace QmlDesigner {
namespace Internal {

class ItemLibrarySideBarItem : public Core::SideBarItem
{
public:
    explicit ItemLibrarySideBarItem(QWidget *widget, const QString &id);
    ~ItemLibrarySideBarItem() override;

    QList<QToolButton *> createToolBarWidgets() override;
};

ItemLibrarySideBarItem::ItemLibrarySideBarItem(QWidget *widget, const QString &id) : Core::SideBarItem(widget, id) {}

ItemLibrarySideBarItem::~ItemLibrarySideBarItem() = default;

QList<QToolButton *> ItemLibrarySideBarItem::createToolBarWidgets()
{
    return qobject_cast<ItemLibraryWidget*>(widget())->createToolBarWidgets();
}

// ---------- DesignModeWidget
DesignModeWidget::DesignModeWidget()
    : m_toolBar(new Core::EditorToolBar(this))
    , m_crumbleBar(new CrumbleBar(this))
{
    setAcceptDrops(true);
    if (Utils::StyleHelper::isQDSTheme() || Core::ICore::isQtDesignStudio())
        qApp->setStyle(QmlDesignerBasePlugin::style());
}

DesignModeWidget::~DesignModeWidget()
{
    for (QPointer<QWidget> widget : std::as_const(m_viewWidgets)) {
        if (widget)
            widget.clear();
    }

    delete m_dockManager;
}

QWidget *DesignModeWidget::createProjectExplorerWidget(QWidget *parent)
{
    const QList<Core::INavigationWidgetFactory *> factories =
            Core::INavigationWidgetFactory::allNavigationFactories();

    Core::NavigationView navigationView;
    navigationView.widget = nullptr;

    for (Core::INavigationWidgetFactory *factory : factories) {
        if (factory->id() == "Project") {
            navigationView = factory->createWidget();
            hideToolButtons(navigationView.dockToolBarWidgets);
        }
    }

    if (navigationView.widget) {
        QByteArray sheet = Utils::FileReader::fetchQrc(":/qmldesigner/stylesheet.css");
        sheet += "QLabel { background-color: #4f4f4f; }";
        navigationView.widget->setStyleSheet(Theme::replaceCssColors(QString::fromUtf8(sheet)));
        navigationView.widget->setParent(parent);
    }

    return navigationView.widget;
}

void DesignModeWidget::readSettings() // readPerspectives
{
    return;
}

void DesignModeWidget::saveSettings() // savePerspectives
{
    return;
}

void DesignModeWidget::enableWidgets()
{
    if (debug)
        qDebug() << Q_FUNC_INFO;

    viewManager().enableWidgets();
    m_isDisabled = false;
}

void DesignModeWidget::disableWidgets()
{
    if (debug)
        qDebug() << Q_FUNC_INFO;

    viewManager().disableWidgets();
    m_isDisabled = true;
}

static void addSpacerToToolBar(QToolBar *toolBar)
{
    QWidget* empty = new QWidget();
    empty->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolBar->addWidget(empty);
}

void DesignModeWidget::setup()
{
    auto settings = Core::ICore::settings(QSettings::UserScope);

    ADS::DockManager::setConfigFlags(ADS::DockManager::DefaultNonOpaqueConfig);
    ADS::DockManager::setConfigFlag(ADS::DockManager::FocusHighlighting, true);
    ADS::DockManager::setConfigFlag(ADS::DockManager::DockAreaHasCloseButton, false);
    ADS::DockManager::setConfigFlag(ADS::DockManager::DockAreaHasUndockButton, false);
    ADS::DockManager::setConfigFlag(ADS::DockManager::DockAreaHasTabsMenuButton, false);
    ADS::DockManager::setConfigFlag(ADS::DockManager::OpaqueSplitterResize, true);
    ADS::DockManager::setConfigFlag(ADS::DockManager::AllTabsHaveCloseButton, false);
    ADS::DockManager::setConfigFlag(ADS::DockManager::RetainTabSizeWhenCloseButtonHidden, true);

    //ADS::DockManager::setAutoHideConfigFlags(ADS::DockManager::DefaultAutoHideConfig);

    m_dockManager = new ADS::DockManager(this);
    m_dockManager->setSettings(settings);
    m_dockManager->setWorkspacePresetsPath(
        Core::ICore::resourcePath("qmldesigner/workspacePresets/").toString());

    QString sheet = QString::fromUtf8(Utils::FileReader::fetchQrc(":/qmldesigner/dockwidgets.css"));
    m_dockManager->setStyleSheet(Theme::replaceCssColors(sheet));

    // Setup icons
    const QString closeUnicode = Theme::getIconUnicode(Theme::Icon::close_small);
    const QString maximizeUnicode = Theme::getIconUnicode(Theme::Icon::maxBar_small);
    const QString normalUnicode = Theme::getIconUnicode(Theme::Icon::normalBar_small);

    const QString fontName = "qtds_propertyIconFont.ttf";
    const QSize size = QSize(28, 28);

    auto tabCloseIconNormal = Utils::StyleHelper::IconFontHelper(
        closeUnicode, Theme::getColor(Theme::DSdockWidgetTitleBar), size, QIcon::Normal, QIcon::Off);
    auto tabCloseIconActive = Utils::StyleHelper::IconFontHelper(
        closeUnicode, Theme::getColor(Theme::DSdockWidgetTitleBar), size, QIcon::Active, QIcon::Off);
    auto tabCloseIconFocus = Utils::StyleHelper::IconFontHelper(
        closeUnicode, Theme::getColor(Theme::DSdockWidgetTitleBar), size, QIcon::Selected, QIcon::Off);

    const QIcon tabsCloseIcon = Utils::StyleHelper::getIconFromIconFont(fontName,
                                                                        {tabCloseIconNormal,
                                                                         tabCloseIconActive,
                                                                         tabCloseIconFocus});

    ADS::DockManager::iconProvider().registerCustomIcon(ADS::TabCloseIcon, tabsCloseIcon);

    auto floatingWidgetCloseIconNormal = Utils::StyleHelper::IconFontHelper(
        closeUnicode, Theme::getColor(Theme::DStitleBarText), QSize(17, 17), QIcon::Normal, QIcon::Off);
    const QIcon floatingWidgetCloseIcon = Utils::StyleHelper::getIconFromIconFont(
        fontName, {floatingWidgetCloseIconNormal});

    ADS::DockManager::iconProvider().registerCustomIcon(ADS::FloatingWidgetCloseIcon,
                                                        floatingWidgetCloseIcon);

    auto floatingWidgetMaxIconNormal = Utils::StyleHelper::IconFontHelper(maximizeUnicode,
                                                                          Theme::getColor(
                                                                              Theme::DStitleBarText),
                                                                          QSize(17, 17),
                                                                          QIcon::Normal,
                                                                          QIcon::Off);
    const QIcon floatingWidgetMaxIcon = Utils::StyleHelper::getIconFromIconFont(
        fontName, {floatingWidgetMaxIconNormal});

    ADS::DockManager::iconProvider().registerCustomIcon(ADS::FloatingWidgetMaximizeIcon,
                                                        floatingWidgetMaxIcon);

    auto floatingWidgetNormalIconNormal = Utils::StyleHelper::IconFontHelper(
        normalUnicode, Theme::getColor(Theme::DStitleBarText), QSize(17, 17), QIcon::Normal, QIcon::Off);
    const QIcon floatingWidgetNormalIcon = Utils::StyleHelper::getIconFromIconFont(
        fontName, {floatingWidgetNormalIconNormal});

    ADS::DockManager::iconProvider().registerCustomIcon(ADS::FloatingWidgetNormalIcon,
                                                        floatingWidgetNormalIcon);

    // Setup Actions and Menus
    Core::ActionContainer *mview = Core::ActionManager::actionContainer(Core::Constants::M_VIEW);
    // View > Views
    Core::ActionContainer *mviews = Core::ActionManager::createMenu(Core::Constants::M_VIEW_VIEWS);
    mviews->menu()->addSeparator();
    // View > Workspaces
    Core::ActionContainer *mworkspaces = Core::ActionManager::createMenu(QmlDesigner::Constants::M_VIEW_WORKSPACES);
    mview->addMenu(mworkspaces, Core::Constants::G_VIEW_VIEWS);
    mworkspaces->menu()->setTitle(tr("&Workspaces"));
    mworkspaces->setOnAllDisabledBehavior(Core::ActionContainer::Show);
    // Connect opening of the 'workspaces' menu with creation of the workspaces menu
    connect(mworkspaces->menu(), &QMenu::aboutToShow, this, &DesignModeWidget::aboutToShowWorkspaces);

    // Initially disable menus
    mworkspaces->menu()->setEnabled(false);

    // Enable/disable menus when mode is different to MODE_DESIGN
    connect(Core::ModeManager::instance(),
            &Core::ModeManager::currentModeChanged,
            this,
            [mworkspaces](Utils::Id mode, Utils::Id) {
                if (mode == Core::Constants::MODE_DESIGN) {
                    mworkspaces->menu()->setEnabled(true);
                } else {
                    mworkspaces->menu()->setEnabled(false);
                }
            });

    // Create a DockWidget for each QWidget and add them to the DockManager
    const Core::Context designContext(Core::Constants::C_DESIGN_MODE);
    static const Utils::Id actionToggle("QmlDesigner.Toggle");

    // First get all navigation views
    QList<Core::INavigationWidgetFactory *> factories = Core::INavigationWidgetFactory::allNavigationFactories();
    QList<Core::Command *> viewCommands;
    const QList<Utils::Id> navigationViewIds = {"Projects", "File System", "Open Documents"};

    for (Core::INavigationWidgetFactory *factory : factories) {
        Core::NavigationView navigationView = {nullptr, {}};

        if (!navigationViewIds.contains(factory->id()))
            continue;

        navigationView = factory->createWidget();

        if (!navigationView.widget)
            continue;

        hideToolButtons(navigationView.dockToolBarWidgets);
        navigationView.widget->setWindowTitle(tr(factory->id().name()));

        QString idString = factory->id().toSetting().toString();
        const QString title = idString;
        const QString uniqueId = idString.remove(" "); // title without whitespaces

        // Apply stylesheet to QWidget
        QByteArray sheet = Utils::FileReader::fetchQrc(":/qmldesigner/stylesheet.css");
        sheet += "QLabel { background-color: creatorTheme.DSsectionHeadBackground; }";
        navigationView.widget->setStyleSheet(Theme::replaceCssColors(QString::fromUtf8(sheet)));

        // Create DockWidget
        ADS::DockWidget *dockWidget = new ADS::DockWidget(uniqueId);
        dockWidget->setWidget(navigationView.widget);
        dockWidget->setWindowTitle(title);
        m_dockManager->addDockWidget(ADS::NoDockWidgetArea, dockWidget);

        // Set unique id as object name
        navigationView.widget->setObjectName(uniqueId);

        // Create menu action
        auto command = Core::ActionManager::registerAction(dockWidget->toggleViewAction(),
                                                           actionToggle.withSuffix(uniqueId + "Widget"),
                                                           designContext);
        command->setAttribute(Core::Command::CA_Hide);
        viewCommands.append(command);
    }

    // Afterwards get all the other widgets
    for (const WidgetInfo &widgetInfo : viewManager().widgetInfos()) {
        // Create DockWidget
        ADS::DockWidget *dockWidget = new ADS::DockWidget(widgetInfo.uniqueId);
        dockWidget->setWidget(widgetInfo.widget);
        dockWidget->setWindowTitle(widgetInfo.tabName);
        m_dockManager->addDockWidget(ADS::NoDockWidgetArea, dockWidget);

        // Add to view widgets
        m_viewWidgets.append(widgetInfo.widget);

        // Set unique id as object name
        widgetInfo.widget->setObjectName(widgetInfo.uniqueId);

        // Create menu action
        auto command = Core::ActionManager::registerAction(dockWidget->toggleViewAction(),
                                                           actionToggle.withSuffix(
                                                               widgetInfo.uniqueId + "Widget"),
                                                           designContext);
        command->setAttribute(Core::Command::CA_Hide);
        viewCommands.append(command);
    }

    // Finally the output pane
    {
        const QString uniqueId = "OutputPane";
        auto outputPanePlaceholder = new Core::OutputPanePlaceHolder(Core::Constants::MODE_DESIGN);
        m_outputPaneDockWidget = new ADS::DockWidget(uniqueId);
        m_outputPaneDockWidget->setWidget(outputPanePlaceholder);
        m_outputPaneDockWidget->setWindowTitle(tr("Output"));
        m_dockManager->addDockWidget(ADS::NoDockWidgetArea, m_outputPaneDockWidget);

        // Set unique id as object name
        outputPanePlaceholder->setObjectName(uniqueId);

        // Create menu action
        auto command = Core::ActionManager::registerAction(m_outputPaneDockWidget->toggleViewAction(),
                                                           actionToggle.withSuffix("OutputPaneWidget"),
                                                           designContext);
        command->setAttribute(Core::Command::CA_Hide);
        viewCommands.append(command);

        connect(outputPanePlaceholder,
                &Core::OutputPanePlaceHolder::visibilityChangeRequested,
                m_outputPaneDockWidget,
                &ADS::DockWidget::toggleView);
    }

    std::sort(viewCommands.begin(), viewCommands.end(), [](Core::Command *first, Core::Command *second) {
        return first->description() < second->description();
    });

    for (Core::Command *command : viewCommands)
        mviews->addAction(command);

    // Create toolbars
    if (!ToolBar::isVisible()) {
        auto toolBar = new QToolBar();

        toolBar->addAction(viewManager().componentViewAction());
        toolBar->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        DesignerActionToolBar *designerToolBar = QmlDesignerPlugin::instance()
                                                     ->viewManager()
                                                     .designerActionManager()
                                                     .createToolBar(m_toolBar);

        designerToolBar->layout()->addWidget(toolBar);

        m_toolBar->addCenterToolBar(designerToolBar);
        m_toolBar->setMinimumWidth(320);
        m_toolBar->setToolbarCreationFlags(Core::EditorToolBar::FlagsStandalone);
        m_toolBar->setNavigationVisible(true);

        connect(m_toolBar,
                &Core::EditorToolBar::goForwardClicked,
                this,
                &DesignModeWidget::toolBarOnGoForwardClicked);
        connect(m_toolBar,
                &Core::EditorToolBar::goBackClicked,
                this,
                &DesignModeWidget::toolBarOnGoBackClicked);

        QToolBar* toolBarWrapper = new QToolBar();
        toolBarWrapper->addWidget(m_toolBar);
        toolBarWrapper->addWidget(createCrumbleBarFrame());
        toolBarWrapper->setMovable(false);
        addToolBar(Qt::TopToolBarArea, toolBarWrapper);

        addSpacerToToolBar(toolBar);

        const QIcon gaIcon = Utils::StyleHelper::getIconFromIconFont(
                    fontName, Theme::getIconUnicode(Theme::Icon::annotationBubble),
                    36, 36, Theme::getColor(Theme::IconsBaseColor));
        toolBar->addAction(gaIcon, tr("Edit global annotation for current file."), [&](){
            ModelNode node = currentDesignDocument()->rewriterView()->rootModelNode();

            if (node.isValid()) {
                m_globalAnnotationEditor.setModelNode(node);
                m_globalAnnotationEditor.showWidget();
            }
        });
    }

    if (currentDesignDocument())
        setupNavigatorHistory(currentDesignDocument()->textEditor());

    m_dockManager->initialize();
    if (style()->styleHint(QStyle::SH_ScrollBar_Transient, nullptr, this))
        Utils::GlobalTransientSupport::support(m_dockManager);

    // Hide all floating widgets if the initial mode isn't design mode
    if (Core::ModeManager::instance()->currentModeId() != Core::Constants::MODE_DESIGN) {
        for (auto &floatingWidget : m_dockManager->floatingWidgets())
            floatingWidget->hide();
    }

    connect(Core::ModeManager::instance(),
            &Core::ModeManager::currentModeChanged,
            this,
            [this](Utils::Id mode, Utils::Id previousMode) {
                if (mode == Core::Constants::MODE_DESIGN) {
                    m_dockManager->reloadActiveWorkspace();
                    m_dockManager->setModeChangeState(false);
                }

                if (previousMode == Core::Constants::MODE_DESIGN
                    && mode != Core::Constants::MODE_DESIGN) {
                    m_dockManager->save();
                    m_dockManager->setModeChangeState(true);
                    for (auto &floatingWidget : m_dockManager->floatingWidgets())
                        floatingWidget->hide();
                }
            });

    viewManager().enableWidgets();
    readSettings();
    show();
}

void DesignModeWidget::aboutToShowWorkspaces()
{
    Core::ActionContainer *aci = Core::ActionManager::actionContainer(
        QmlDesigner::Constants::M_VIEW_WORKSPACES);
    QMenu *menu = aci->menu();
    menu->clear();

    auto *ag = new QActionGroup(menu);

    connect(ag, &QActionGroup::triggered, this, [this](QAction *action) {
        m_dockManager->openWorkspace(action->data().toString());
    });

    QAction *action = menu->addAction(tr("Manage..."));
    connect(action, &QAction::triggered, m_dockManager, &ADS::DockManager::showWorkspaceMananger);

    QAction *resetWorkspace = menu->addAction(tr("Reset Active"));
    connect(resetWorkspace, &QAction::triggered, this, [this]() {
        if (m_dockManager->resetWorkspacePreset(m_dockManager->activeWorkspace()->fileName()))
            m_dockManager->reloadActiveWorkspace();
    });

    menu->addSeparator();

    auto workspaces = m_dockManager->workspaces();
    for (const auto &workspace : std::as_const(workspaces)) {
        QAction *action = ag->addAction(workspace.name());
        action->setData(workspace.fileName());
        action->setCheckable(true);
        if (workspace == *m_dockManager->activeWorkspace())
            action->setChecked(true);
    }
    menu->addActions(ag->actions());
}

void DesignModeWidget::toolBarOnGoBackClicked()
{
    if (m_navigatorHistoryCounter > 0) {
        --m_navigatorHistoryCounter;
        m_keepNavigatorHistory = true;
        Core::EditorManager::openEditor(Utils::FilePath::fromString(
                                            m_navigatorHistory.at(m_navigatorHistoryCounter)),
                                        Utils::Id(),
                                        Core::EditorManager::DoNotMakeVisible);
        m_keepNavigatorHistory = false;
    }
}

void DesignModeWidget::toolBarOnGoForwardClicked()
{
    if (m_navigatorHistoryCounter < (m_navigatorHistory.size() - 1)) {
        ++m_navigatorHistoryCounter;
        m_keepNavigatorHistory = true;
        Core::EditorManager::openEditor(Utils::FilePath::fromString(
                                            m_navigatorHistory.at(m_navigatorHistoryCounter)),
                                        Utils::Id(),
                                        Core::EditorManager::DoNotMakeVisible);
        m_keepNavigatorHistory = false;
    }
}

bool DesignModeWidget::canGoForward()
{
    return m_canGoForward;
}

bool DesignModeWidget::canGoBack()
{
    return m_canGoBack;
}

ADS::DockManager *DesignModeWidget::dockManager() const
{
    return m_dockManager;
}

GlobalAnnotationEditor &DesignModeWidget::globalAnnotationEditor()
{
    return m_globalAnnotationEditor;
}

void DesignModeWidget::dragEnterEvent(QDragEnterEvent *event)
{
    event->accept();
    event->setDropAction(Qt::IgnoreAction);
}

DesignDocument *DesignModeWidget::currentDesignDocument() const
{
    return QmlDesignerPlugin::instance()->documentManager().currentDesignDocument();
}

ViewManager &DesignModeWidget::viewManager()
{
    return QmlDesignerPlugin::instance()->viewManager();
}

void DesignModeWidget::setupNavigatorHistory(Core::IEditor *editor)
{
    if (!m_keepNavigatorHistory)
        addNavigatorHistoryEntry(editor->document()->filePath());

    m_canGoBack = m_navigatorHistoryCounter > 0;
    m_canGoForward = m_navigatorHistoryCounter < (m_navigatorHistory.size() - 1);
    m_toolBar->setCanGoBack(m_canGoBack);
    m_toolBar->setCanGoForward(m_canGoForward);
    if (!ToolBar::isVisible())
        m_toolBar->setCurrentEditor(editor);

    emit navigationHistoryChanged();
}

void DesignModeWidget::addNavigatorHistoryEntry(const Utils::FilePath &fileName)
{
    if (m_navigatorHistoryCounter > 0)
        m_navigatorHistory.insert(m_navigatorHistoryCounter + 1, fileName.toString());
    else
        m_navigatorHistory.append(fileName.toString());

    ++m_navigatorHistoryCounter;
}

QWidget *DesignModeWidget::createCrumbleBarFrame()
{
    auto frame = new Utils::StyledBar(this);
    frame->setSingleRow(false);
    auto layout = new QHBoxLayout(frame);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_crumbleBar->crumblePath());

    return frame;
}

CrumbleBar *DesignModeWidget::crumbleBar() const
{
    return m_crumbleBar;
}

void DesignModeWidget::showDockWidget(const QString &objectName, bool focus)
{
    auto dockWidget = m_dockManager->findDockWidget(objectName);
    if (dockWidget) {
        dockWidget->toggleView(true);

        if (focus)
            dockWidget->setFocus();
    }
}

void DesignModeWidget::contextHelp(const Core::IContext::HelpCallback &callback) const
{
    if (currentDesignDocument())
        currentDesignDocument()->contextHelp(callback);
    else
        callback({});
}

void DesignModeWidget::initialize()
{
    if (m_initStatus == NotInitialized) {
        m_initStatus = Initializing;
        setup();
        emit initialized();
    }
    m_initStatus = Initialized;
}

} // namespace Internal
} // namespace Designer
