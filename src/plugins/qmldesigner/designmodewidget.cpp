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

#include "designmodewidget.h"
#include "switchsplittabwidget.h"

#include <designeractionmanager.h>

#include "qmldesignerplugin.h"
#include "crumblebar.h"
#include "documentwarningwidget.h"
#include "edit3dview.h"

#include <texteditor/textdocument.h>
#include <nodeinstanceview.h>
#include <itemlibrarywidget.h>
#include <theme.h>

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actionmanager_p.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/modemanager.h>
#include <qmldesigner/qmldesignerconstants.h>

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

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/stylehelper.h>

#include <QSettings>
#include <QToolBar>
#include <QLayout>
#include <QBoxLayout>
#include <QDir>
#include <QComboBox>

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

class DesignerSideBarItem : public Core::SideBarItem
{
public:
    explicit DesignerSideBarItem(QWidget *widget, WidgetInfo::ToolBarWidgetFactoryInterface *createToolBarWidgets, const QString &id);
    ~DesignerSideBarItem() override;

    QList<QToolButton *> createToolBarWidgets() override;

private:
    WidgetInfo::ToolBarWidgetFactoryInterface *m_toolBarWidgetFactory;

};

DesignerSideBarItem::DesignerSideBarItem(QWidget *widget, WidgetInfo::ToolBarWidgetFactoryInterface *toolBarWidgetFactory, const QString &id)
    : Core::SideBarItem(widget, id) , m_toolBarWidgetFactory(toolBarWidgetFactory)
{
}

DesignerSideBarItem::~DesignerSideBarItem()
{
    delete m_toolBarWidgetFactory;
}

QList<QToolButton *> DesignerSideBarItem::createToolBarWidgets()
{
    if (m_toolBarWidgetFactory)
        return m_toolBarWidgetFactory->createToolBarWidgets();

    return QList<QToolButton *>();
}

// ---------- DesignModeWidget
DesignModeWidget::DesignModeWidget()
    : m_toolBar(new Core::EditorToolBar(this))
    , m_crumbleBar(new CrumbleBar(this))
{
}

DesignModeWidget::~DesignModeWidget()
{
    for (QPointer<QWidget> widget : m_viewWidgets) {
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
        if (factory->id() == "Projects") {
            navigationView = factory->createWidget();
            hideToolButtons(navigationView.dockToolBarWidgets);
        }
    }

    if (navigationView.widget) {
        QByteArray sheet = Utils::FileReader::fetchQrc(":/qmldesigner/stylesheet.css");
        sheet += Utils::FileReader::fetchQrc(":/qmldesigner/scrollbar.css");
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
    empty->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
    toolBar->addWidget(empty);
}

void DesignModeWidget::setup()
{
    auto &actionManager = viewManager().designerActionManager();
    actionManager.createDefaultDesignerActions();
    actionManager.createDefaultAddResourceHandler();
    actionManager.polishActions();

    auto settings = Core::ICore::settings(QSettings::UserScope);

    ADS::DockManager::setConfigFlags(ADS::DockManager::DefaultNonOpaqueConfig);
    ADS::DockManager::setConfigFlag(ADS::DockManager::FocusHighlighting, true);
    m_dockManager = new ADS::DockManager(this);
    m_dockManager->setSettings(settings);
    m_dockManager->setWorkspacePresetsPath(Core::ICore::resourcePath() + QLatin1String("/qmldesigner/workspacePresets/"));

    QString sheet = QString::fromUtf8(Utils::FileReader::fetchQrc(":/qmldesigner/dockwidgets.css"));
    m_dockManager->setStyleSheet(Theme::replaceCssColors(sheet));

    // Setup icons
    QColor buttonColor(Theme::getColor(Theme::QmlDesigner_TabLight)); // TODO Use correct color roles
    QColor tabColor(Theme::getColor(Theme::QmlDesigner_TabDark));

    const QString closeUnicode = Theme::getIconUnicode(Theme::Icon::adsClose);
    const QString menuUnicode = Theme::getIconUnicode(Theme::Icon::adsDropDown);
    const QString undockUnicode = Theme::getIconUnicode(Theme::Icon::adsDetach);

    const QString fontName = "qtds_propertyIconFont.ttf";
    const QIcon tabsCloseIcon = Utils::StyleHelper::getIconFromIconFont(fontName, closeUnicode, 28, 28, tabColor);
    const QIcon menuIcon = Utils::StyleHelper::getIconFromIconFont(fontName, menuUnicode, 28, 28, buttonColor);
    const QIcon undockIcon = Utils::StyleHelper::getIconFromIconFont(fontName, undockUnicode, 28, 28, buttonColor);
    const QIcon closeIcon = Utils::StyleHelper::getIconFromIconFont(fontName, closeUnicode, 28, 28, buttonColor);

    m_dockManager->iconProvider().registerCustomIcon(ADS::TabCloseIcon, tabsCloseIcon);
    m_dockManager->iconProvider().registerCustomIcon(ADS::DockAreaMenuIcon, menuIcon);
    m_dockManager->iconProvider().registerCustomIcon(ADS::DockAreaUndockIcon, undockIcon);
    m_dockManager->iconProvider().registerCustomIcon(ADS::DockAreaCloseIcon, closeIcon);
    m_dockManager->iconProvider().registerCustomIcon(ADS::FloatingWidgetCloseIcon, closeIcon);

    // Setup Actions and Menus
    Core::ActionContainer *mview = Core::ActionManager::actionContainer(Core::Constants::M_VIEW);
    // Window > Views
    Core::ActionContainer *mviews = Core::ActionManager::createMenu(Core::Constants::M_VIEW_VIEWS);
    mviews->menu()->addSeparator();
    // Window > Workspaces
    Core::ActionContainer *mworkspaces = Core::ActionManager::createMenu(QmlDesigner::Constants::M_WINDOW_WORKSPACES);
    mview->addMenu(mworkspaces, Core::Constants::G_VIEW_VIEWS);
    mworkspaces->menu()->setTitle(tr("&Workspaces"));
    mworkspaces->setOnAllDisabledBehavior(Core::ActionContainer::Show);
    // Connect opening of the 'workspaces' menu with creation of the workspaces menu
    connect(mworkspaces->menu(), &QMenu::aboutToShow, this, &DesignModeWidget::aboutToShowWorkspaces);
    // Disable workspace menu when context is different to C_DESIGN_MODE
    connect(Core::ICore::instance(), &Core::ICore::contextChanged,
            this, [mworkspaces](const Core::Context &context){
                if (context.contains(Core::Constants::C_DESIGN_MODE))
                    mworkspaces->menu()->setEnabled(true);
                else
                    mworkspaces->menu()->setEnabled(false);
                });

    // Create a DockWidget for each QWidget and add them to the DockManager
    const Core::Context designContext(Core::Constants::C_DESIGN_MODE);
    static const Core::Id actionToggle("QmlDesigner.Toggle");

    // First get all navigation views
    QList<Core::INavigationWidgetFactory *> factories = Core::INavigationWidgetFactory::allNavigationFactories();

    for (Core::INavigationWidgetFactory *factory : factories) {
        Core::NavigationView navigationView;
        navigationView.widget = nullptr;
        QString uniqueId;
        QString title;

        if (factory->id() == "Projects") {
            navigationView = factory->createWidget();
            hideToolButtons(navigationView.dockToolBarWidgets);
            navigationView.widget->setWindowTitle(tr(factory->id().name()));
            uniqueId = "Projects";
            title = "Projects";
        }
        if (factory->id() == "File System") {
            navigationView = factory->createWidget();
            hideToolButtons(navigationView.dockToolBarWidgets);
            navigationView.widget->setWindowTitle(tr(factory->id().name()));
            uniqueId = "FileSystem";
            title = "File System";
        }
        if (factory->id() == "Open Documents") {
            navigationView = factory->createWidget();
            hideToolButtons(navigationView.dockToolBarWidgets);
            navigationView.widget->setWindowTitle(tr(factory->id().name()));
            uniqueId = "OpenDocuments";
            title = "Open Documents";
        }

        if (navigationView.widget) {
            // Apply stylesheet to QWidget
            QByteArray sheet = Utils::FileReader::fetchQrc(":/qmldesigner/stylesheet.css");
            sheet += Utils::FileReader::fetchQrc(":/qmldesigner/scrollbar.css");
            sheet += "QLabel { background-color: creatorTheme.DSsectionHeadBackground; }";
            navigationView.widget->setStyleSheet(Theme::replaceCssColors(QString::fromUtf8(sheet)));

            // Create DockWidget
            ADS::DockWidget *dockWidget = new ADS::DockWidget(uniqueId);
            dockWidget->setWidget(navigationView.widget);
            dockWidget->setWindowTitle(title);
            m_dockManager->addDockWidget(ADS::NoDockWidgetArea, dockWidget);

            // Create menu action
            auto command = Core::ActionManager::registerAction(dockWidget->toggleViewAction(),
                                                               actionToggle.withSuffix(uniqueId + "Widget"),
                                                               designContext);
            command->setAttribute(Core::Command::CA_Hide);
            mviews->addAction(command);
        }
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

        // Create menu action
        auto command = Core::ActionManager::registerAction(dockWidget->toggleViewAction(),
                                                           actionToggle.withSuffix(widgetInfo.uniqueId + "Widget"),
                                                           designContext);
        command->setAttribute(Core::Command::CA_Hide);
        mviews->addAction(command);
    }

    // Finally the output pane
    {
        auto outputPanePlaceholder = new Core::OutputPanePlaceHolder(Core::Constants::MODE_DESIGN);
        m_outputPaneDockWidget = new ADS::DockWidget("OutputPane");
        m_outputPaneDockWidget->setWidget(outputPanePlaceholder);
        m_outputPaneDockWidget->setWindowTitle("Output Pane");
        m_dockManager->addDockWidget(ADS::NoDockWidgetArea, m_outputPaneDockWidget);
        // Create menu action
        auto command = Core::ActionManager::registerAction(m_outputPaneDockWidget->toggleViewAction(),
                                                           actionToggle.withSuffix("OutputPaneWidget"),
                                                           designContext);
        command->setAttribute(Core::Command::CA_Hide);
        mviews->addAction(command);

        connect(outputPanePlaceholder, &Core::OutputPanePlaceHolder::visibilityChangeRequested,
                m_outputPaneDockWidget, &ADS::DockWidget::toggleView);
    }

    // Create toolbars
    auto toolBar = new QToolBar();

    toolBar->addAction(viewManager().componentViewAction());
    toolBar->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    DesignerActionToolBar *designerToolBar = QmlDesignerPlugin::instance()->viewManager().designerActionManager().createToolBar(m_toolBar);

    designerToolBar->layout()->addWidget(toolBar);

    m_toolBar->addCenterToolBar(designerToolBar);
    m_toolBar->setMinimumWidth(320);
    m_toolBar->setToolbarCreationFlags(Core::EditorToolBar::FlagsStandalone);
    m_toolBar->setNavigationVisible(true);

    connect(m_toolBar, &Core::EditorToolBar::goForwardClicked, this, &DesignModeWidget::toolBarOnGoForwardClicked);
    connect(m_toolBar, &Core::EditorToolBar::goBackClicked, this, &DesignModeWidget::toolBarOnGoBackClicked);

    QToolBar* toolBarWrapper = new QToolBar();
    toolBarWrapper->addWidget(m_toolBar);
    toolBarWrapper->addWidget(createCrumbleBarFrame());
    toolBarWrapper->setMovable(false);
    addToolBar(Qt::TopToolBarArea, toolBarWrapper);

    if (currentDesignDocument())
        setupNavigatorHistory(currentDesignDocument()->textEditor());

    m_dockManager->initialize();

    connect(Core::ModeManager::instance(), &Core::ModeManager::currentModeChanged,
            this, [this](Core::Id mode, Core::Id oldMode) {
        if (mode == Core::Constants::MODE_DESIGN) {
            m_dockManager->reloadActiveWorkspace();
            m_dockManager->setModeChangeState(false);
        }

        if (oldMode == Core::Constants::MODE_DESIGN
            && mode != Core::Constants::MODE_DESIGN) {
            m_dockManager->save();
            m_dockManager->setModeChangeState(true);
            for (auto floatingWidget : m_dockManager->floatingWidgets())
                floatingWidget->hide();
        }
    });

    addSpacerToToolBar(toolBar);

    auto workspaceComboBox = new QComboBox();
    workspaceComboBox->setMinimumWidth(120);
    workspaceComboBox->setToolTip(tr("Switch the active workspace."));
    auto sortedWorkspaces = m_dockManager->workspaces();
    Utils::sort(sortedWorkspaces);
    workspaceComboBox->addItems(sortedWorkspaces);
    workspaceComboBox->setCurrentText(m_dockManager->activeWorkspace());
    toolBar->addWidget(workspaceComboBox);

    connect(m_dockManager, &ADS::DockManager::workspaceListChanged,
            workspaceComboBox, [this, workspaceComboBox]() {
                workspaceComboBox->clear();
                auto sortedWorkspaces = m_dockManager->workspaces();
                Utils::sort(sortedWorkspaces);
                workspaceComboBox->addItems(sortedWorkspaces);
                workspaceComboBox->setCurrentText(m_dockManager->activeWorkspace());
    });
    connect(m_dockManager, &ADS::DockManager::workspaceLoaded, workspaceComboBox, &QComboBox::setCurrentText);
    connect(workspaceComboBox, QOverload<int>::of(&QComboBox::activated),
            m_dockManager, [this, workspaceComboBox] (int index) {
            Q_UNUSED(index)
            m_dockManager->openWorkspace(workspaceComboBox->currentText());
    });

    const QIcon gaIcon = Utils::StyleHelper::getIconFromIconFont(fontName,
                                                                 Theme::getIconUnicode(Theme::Icon::annotationBubble), 36, 36);
    toolBar->addAction(gaIcon, tr("Edit global annotation for current file."), [&](){
        ModelNode node = currentDesignDocument()->rewriterView()->rootModelNode();

        if (node.isValid()) {
            m_globalAnnotationEditor.setModelNode(node);
            m_globalAnnotationEditor.showWidget();
        }
    });



    viewManager().enableWidgets();
    readSettings();
    show();
}

void DesignModeWidget::aboutToShowWorkspaces()
{
    Core::ActionContainer *aci = Core::ActionManager::actionContainer(QmlDesigner::Constants::M_WINDOW_WORKSPACES);
    QMenu *menu = aci->menu();
    menu->clear();

    auto *ag = new QActionGroup(menu);

    connect(ag, &QActionGroup::triggered, this, [this](QAction *action) {
        QString workspace = action->data().toString();
        m_dockManager->openWorkspace(workspace);
    });

    QAction *action = menu->addAction("Manage...");
    connect(action, &QAction::triggered,
            m_dockManager, &ADS::DockManager::showWorkspaceMananger);

    menu->addSeparator();

    // Sort the list of workspaces
    auto sortedWorkspaces = m_dockManager->workspaces();
    Utils::sort(sortedWorkspaces);

    for (const auto &workspace : sortedWorkspaces)
    {
        QAction *action = ag->addAction(workspace);
        action->setData(workspace);
        action->setCheckable(true);
        if (workspace == m_dockManager->activeWorkspace())
            action->setChecked(true);
    }
    menu->addActions(ag->actions());
}

void DesignModeWidget::toolBarOnGoBackClicked()
{
    if (m_navigatorHistoryCounter > 0) {
        --m_navigatorHistoryCounter;
        m_keepNavigatorHistory = true;
        Core::EditorManager::openEditor(m_navigatorHistory.at(m_navigatorHistoryCounter),
                                        Core::Id(), Core::EditorManager::DoNotMakeVisible);
        m_keepNavigatorHistory = false;
    }
}

void DesignModeWidget::toolBarOnGoForwardClicked()
{
    if (m_navigatorHistoryCounter < (m_navigatorHistory.size() - 1)) {
        ++m_navigatorHistoryCounter;
        m_keepNavigatorHistory = true;
        Core::EditorManager::openEditor(m_navigatorHistory.at(m_navigatorHistoryCounter),
                                        Core::Id(), Core::EditorManager::DoNotMakeVisible);
        m_keepNavigatorHistory = false;
    }
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

    const bool canGoBack = m_navigatorHistoryCounter > 0;
    const bool canGoForward = m_navigatorHistoryCounter < (m_navigatorHistory.size() - 1);
    m_toolBar->setCanGoBack(canGoBack);
    m_toolBar->setCanGoForward(canGoForward);
    m_toolBar->setCurrentEditor(editor);
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

void DesignModeWidget::showInternalTextEditor()
{
    auto dockWidget = m_dockManager->findDockWidget("TextEditor");
    if (dockWidget)
        dockWidget->toggleView(true);
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
    }

    m_initStatus = Initialized;
}

} // namespace Internal
} // namespace Designer
