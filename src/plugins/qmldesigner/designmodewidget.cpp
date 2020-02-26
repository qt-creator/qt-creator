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

#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QSettings>
#include <QToolBar>
#include <QLayout>
#include <QBoxLayout>
#include <QDir>

#include <advanceddockingsystem/dockareawidget.h>
#include <advanceddockingsystem/docksplitter.h>

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

    QSettings *settings = Core::ICore::settings();

    settings->beginGroup("Bauhaus");
    settings->endGroup();
}

void DesignModeWidget::saveSettings() // savePerspectives
{
    return;

    QSettings *settings = Core::ICore::settings();

    settings->beginGroup("Bauhaus");
    settings->endGroup();
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

bool DesignModeWidget::eventFilter(QObject *obj, QEvent *event) // TODO
{
    if (event->type() == QEvent::Hide) {
        qDebug() << ">>> HIDE";
        m_outputPaneDockWidget->toggleView(false);
        return true;
    } else if (event->type() == QEvent::Show) {
        qDebug() << ">>> SHOW";
        m_outputPaneDockWidget->toggleView(true);
        return true;
    } else {
        // standard event processing
        return QObject::eventFilter(obj, event);
    }
}

void DesignModeWidget::setup()
{
    auto &actionManager = viewManager().designerActionManager();
    actionManager.createDefaultDesignerActions();
    actionManager.createDefaultAddResourceHandler();
    actionManager.polishActions();

    m_dockManager = new ADS::DockManager(this);
    m_dockManager->setConfigFlags(ADS::DockManager::DefaultNonOpaqueConfig);
    m_dockManager->setSettings(Core::ICore::settings(QSettings::UserScope));

    QString sheet = QString::fromUtf8(Utils::FileReader::fetchQrc(":/qmldesigner/dockwidgets.css"));
    m_dockManager->setStyleSheet(Theme::replaceCssColors(sheet));

    // Setup Actions and Menus
    Core::ActionContainer *mwindow = Core::ActionManager::actionContainer(Core::Constants::M_WINDOW);
    // Window > Views
    Core::ActionContainer *mviews = Core::ActionManager::createMenu(Core::Constants::M_WINDOW_VIEWS);
    mviews->menu()->addSeparator();
    // Window > Workspaces
    Core::ActionContainer *mworkspaces = Core::ActionManager::createMenu(QmlDesigner::Constants::M_WINDOW_WORKSPACES);
    mwindow->addMenu(mworkspaces, Core::Constants::G_WINDOW_VIEWS);
    mworkspaces->menu()->setTitle(tr("&Workspaces"));
    mworkspaces->setOnAllDisabledBehavior(Core::ActionContainer::Show); // TODO what does it exactly do?!

    // Connect opening of the 'window' menu with creation of the workspaces menu
    connect(mwindow->menu(), &QMenu::aboutToShow, this, &DesignModeWidget::aboutToShowWorkspaces);

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
            sheet += "QLabel { background-color: #4f4f4f; }";
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

        //outputPanePlaceholder->installEventFilter(this);
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

    // Get a list of all available workspaces
    QStringList workspaces = m_dockManager->workspaces();
    QString workspace = ADS::Constants::FACTORY_DEFAULT_NAME;

    // If there is no factory default workspace create one and write the xml file
    if (!workspaces.contains(ADS::Constants::FACTORY_DEFAULT_NAME)) {
        createFactoryDefaultWorkspace();
        // List of workspaces needs to be updated
        workspaces = m_dockManager->workspaces();
    }

    // Determine workspace to restore at startup
    if (m_dockManager->autoRestorLastWorkspace()) {
        QString lastWorkspace = m_dockManager->lastWorkspace();
        if (!lastWorkspace.isEmpty() && workspaces.contains(lastWorkspace))
            workspace = lastWorkspace;
        else
            qDebug() << "Couldn't restore last workspace!";
    }

    if (workspace.isNull() && workspaces.contains(ADS::Constants::DEFAULT_NAME)) {
        workspace = ADS::Constants::DEFAULT_NAME;
    }

    m_dockManager->openWorkspace(workspace);

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

    for (const auto &workspace : m_dockManager->workspaces())
    {
        QAction *action = ag->addAction(workspace);
        action->setData(workspace);
        action->setCheckable(true);
        if (workspace == m_dockManager->activeWorkspace())
            action->setChecked(true);
    }
    menu->addActions(ag->actions());
}

void DesignModeWidget::createFactoryDefaultWorkspace()
{
    ADS::DockAreaWidget* centerArea = nullptr;
    ADS::DockAreaWidget* leftArea = nullptr;
    ADS::DockAreaWidget* rightArea = nullptr;
    ADS::DockAreaWidget* bottomArea = nullptr;

    // Iterate over all widgets and only get the central once to start with creating the factory
    // default workspace layout.
    for (const WidgetInfo &widgetInfo : viewManager().widgetInfos()) {
        if (widgetInfo.placementHint == widgetInfo.CentralPane) {
            ADS::DockWidget *dockWidget = m_dockManager->findDockWidget(widgetInfo.uniqueId);
            if (centerArea)
                m_dockManager->addDockWidget(ADS::CenterDockWidgetArea, dockWidget, centerArea);
            else
                centerArea = m_dockManager->addDockWidget(ADS::CenterDockWidgetArea, dockWidget);
        }
    }

    // Iterate over all widgets and get the remaining left, right and bottom widgets
    for (const WidgetInfo &widgetInfo : viewManager().widgetInfos()) {
        if (widgetInfo.placementHint == widgetInfo.LeftPane) {
            ADS::DockWidget *dockWidget = m_dockManager->findDockWidget(widgetInfo.uniqueId);
            if (leftArea)
                m_dockManager->addDockWidget(ADS::CenterDockWidgetArea, dockWidget, leftArea);
            else
                leftArea = m_dockManager->addDockWidget(ADS::LeftDockWidgetArea, dockWidget, centerArea);
        }

        if (widgetInfo.placementHint == widgetInfo.RightPane) {
            ADS::DockWidget *dockWidget = m_dockManager->findDockWidget(widgetInfo.uniqueId);
            if (rightArea)
                m_dockManager->addDockWidget(ADS::CenterDockWidgetArea, dockWidget, rightArea);
            else
                rightArea = m_dockManager->addDockWidget(ADS::RightDockWidgetArea, dockWidget, centerArea);
        }

        if (widgetInfo.placementHint == widgetInfo.BottomPane) {
            ADS::DockWidget *dockWidget = m_dockManager->findDockWidget(widgetInfo.uniqueId);
            if (bottomArea)
                m_dockManager->addDockWidget(ADS::CenterDockWidgetArea, dockWidget, bottomArea);
            else
                bottomArea = m_dockManager->addDockWidget(ADS::BottomDockWidgetArea, dockWidget, centerArea);
        }
    }

    // Iterate over all 'special' widgets
    QStringList specialWidgets = {"Projects", "FileSystem", "OpenDocuments"};
    ADS::DockAreaWidget* leftBottomArea = nullptr;
    for (const QString &uniqueId : specialWidgets) {
        ADS::DockWidget *dockWidget = m_dockManager->findDockWidget(uniqueId);
        if (leftBottomArea)
            m_dockManager->addDockWidget(ADS::CenterDockWidgetArea, dockWidget, leftBottomArea);
        else
            leftBottomArea = m_dockManager->addDockWidget(ADS::BottomDockWidgetArea, dockWidget, leftArea);
    }

    // Add the last widget 'OutputPane' as the bottom bottom area
    ADS::DockWidget *dockWidget = m_dockManager->findDockWidget("OutputPane");
    m_dockManager->addDockWidget(ADS::BottomDockWidgetArea, dockWidget, bottomArea);

    // TODO This is just a test
    auto splitter = centerArea->dockContainer()->rootSplitter();
    splitter->setSizes({100, 800, 100});
    // TODO

    m_dockManager->createWorkspace(ADS::Constants::FACTORY_DEFAULT_NAME);

    // Write the xml file
    Utils::FilePath fileName = m_dockManager->workspaceNameToFileName(ADS::Constants::FACTORY_DEFAULT_NAME);
    QString errorString;
    Utils::FileSaver fileSaver(fileName.toString(), QIODevice::Text);
    QByteArray data = m_dockManager->saveState();
    if (!fileSaver.hasError()) {
        fileSaver.write(data);
    }
    if (!fileSaver.finalize()) {
        errorString = fileSaver.errorString();
    }
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
