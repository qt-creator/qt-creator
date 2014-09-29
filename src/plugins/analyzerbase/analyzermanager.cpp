/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
** Author: Andreas Hartmetz, KDAB (andreas.hartmetz@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "analyzermanager.h"

#include "analyzerplugin.h"
#include "ianalyzertool.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/findplaceholder.h>
#include <coreplugin/icore.h>
#include <coreplugin/imode.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/navigationwidget.h>
#include <coreplugin/outputpane.h>
#include <coreplugin/rightpane.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/editormanager.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/project.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>

#include <utils/fancymainwindow.h>
#include <utils/styledbar.h>
#include <utils/qtcassert.h>
#include <utils/checkablemessagebox.h>
#include <utils/statuslabel.h>

#include <QVariant>
#include <QDebug>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDockWidget>
#include <QAction>
#include <QToolButton>
#include <QComboBox>
#include <QStackedWidget>
#include <QDialog>
#include <QDialogButtonBox>
#include <QPointer>
#include <QPushButton>

using namespace Core;
using namespace Utils;
using namespace Core::Constants;
using namespace Analyzer::Internal;
using namespace Analyzer::Constants;
using namespace ProjectExplorer;

namespace Analyzer {
namespace Internal {

const char LAST_ACTIVE_TOOL[] = "Analyzer.Plugin.LastActiveTool";
const char INITIAL_DOCK_AREA[] = "initial_dock_area";

////////////////////////////////////////////////////////////////////
//
// AnalyzerMode
//
////////////////////////////////////////////////////////////////////

class AnalyzerMode : public IMode
{
    Q_OBJECT

public:
    AnalyzerMode(QObject *parent = 0)
        : IMode(parent)
    {
        setContext(Context(C_ANALYZEMODE, C_NAVIGATION_PANE));
        setDisplayName(tr("Analyze"));
        setIcon(QIcon(QLatin1String(":/images/analyzer_mode.png")));
        setPriority(P_MODE_ANALYZE);
        setId(MODE_ANALYZE);
    }

    ~AnalyzerMode()
    {
        delete m_widget;
        m_widget = 0;
    }
};

} // namespace Internal

////////////////////////////////////////////////////////////////////
//
// AnalyzerManagerPrivate
//
////////////////////////////////////////////////////////////////////

class AnalyzerManagerPrivate : public QObject
{
    Q_OBJECT

public:
    typedef QHash<QString, QVariant> FancyMainWindowSettings;

    AnalyzerManagerPrivate(AnalyzerManager *qq);
    ~AnalyzerManagerPrivate();

    /**
     * After calling this, a proper instance of IMode is initialized
     * It is delayed since an analyzer mode makes no sense without analyzer tools
     *
     * \note Call this before adding a tool to the manager
     */
    void delayedInit();

    void setupActions();
    void createModeMainWindow();
    bool showPromptDialog(const QString &title, const QString &text,
        const QString &stopButtonText, const QString &cancelButtonText) const;

    void activateDock(Qt::DockWidgetArea area, QDockWidget *dockWidget);
    void deactivateDock(QDockWidget *dockWidget);
    void addAction(AnalyzerAction *action);
    void selectSavedTool();
    void selectAction(AnalyzerAction *action);
    void handleToolStarted();
    void handleToolFinished();
    void saveToolSettings(AnalyzerAction *action);
    void loadToolSettings(AnalyzerAction *action);

    // Convenience.
    bool isActionRunnable(AnalyzerAction *action) const;

public slots:
    void startTool();
    void selectToolboxAction(int);
    void selectMenuAction();
    void modeChanged(Core::IMode *mode);
    void resetLayout();
    void updateRunActions();

public:
    AnalyzerManager *q;
    AnalyzerMode *m_mode;
    bool m_isRunning;
    FancyMainWindow *m_mainWindow;
    AnalyzerAction *m_currentAction;
    QList<AnalyzerAction *> m_actions;
    QAction *m_startAction;
    QAction *m_stopAction;
    ActionContainer *m_menu;
    QComboBox *m_toolBox;
    QStackedWidget *m_controlsStackWidget;
    StatusLabel *m_statusLabel;
    typedef QMap<IAnalyzerTool *, FancyMainWindowSettings> MainWindowSettingsMap;
    QHash<IAnalyzerTool *, QList<QDockWidget *> > m_toolWidgets;
    QHash<IAnalyzerTool *, QWidget *> m_controlsWidgetFromTool;
    MainWindowSettingsMap m_defaultSettings;

    // list of dock widgets to prevent memory leak
    typedef QPointer<QDockWidget> DockPtr;
    QList<DockPtr> m_dockWidgets;
};

AnalyzerManagerPrivate::AnalyzerManagerPrivate(AnalyzerManager *qq):
    q(qq),
    m_mode(0),
    m_isRunning(false),
    m_mainWindow(0),
    m_currentAction(0),
    m_startAction(0),
    m_stopAction(0),
    m_menu(0),
    m_toolBox(new QComboBox),
    m_controlsStackWidget(new QStackedWidget),
    m_statusLabel(new StatusLabel)
{
    m_toolBox->setObjectName(QLatin1String("AnalyzerManagerToolBox"));
    connect(m_toolBox, SIGNAL(activated(int)), SLOT(selectToolboxAction(int)));

    setupActions();

    ProjectExplorerPlugin *pe = ProjectExplorerPlugin::instance();
    connect(pe, SIGNAL(updateRunActions()), SLOT(updateRunActions()));
}

AnalyzerManagerPrivate::~AnalyzerManagerPrivate()
{
    // as we have to setParent(0) on dock widget that are not selected,
    // we keep track of all and make sure we don't leak any
    foreach (const DockPtr &ptr, m_dockWidgets) {
        if (ptr)
            delete ptr.data();
    }
}

void AnalyzerManagerPrivate::setupActions()
{
    const Context globalcontext(C_GLOBAL);
    Command *command = 0;

    // Menus
    m_menu = Core::ActionManager::createMenu(M_DEBUG_ANALYZER);
    m_menu->menu()->setTitle(tr("&Analyze"));
    m_menu->menu()->setEnabled(true);

    m_menu->appendGroup(G_ANALYZER_CONTROL);
    m_menu->appendGroup(G_ANALYZER_TOOLS);
    m_menu->appendGroup(G_ANALYZER_REMOTE_TOOLS);
    m_menu->appendGroup(G_ANALYZER_OPTIONS);

    ActionContainer *menubar = Core::ActionManager::actionContainer(MENU_BAR);
    ActionContainer *mtools = Core::ActionManager::actionContainer(M_TOOLS);
    menubar->addMenu(mtools, m_menu);

    m_startAction = new QAction(tr("Start"), m_menu);
    m_startAction->setIcon(QIcon(QLatin1String(ANALYZER_CONTROL_START_ICON)));
    Core::ActionManager::registerAction(m_startAction, "Analyzer.Start", globalcontext);
    connect(m_startAction, SIGNAL(triggered()), this, SLOT(startTool()));

    m_stopAction = new QAction(tr("Stop"), m_menu);
    m_stopAction->setEnabled(false);
    m_stopAction->setIcon(QIcon(QLatin1String(ANALYZER_CONTROL_STOP_ICON)));
    command = Core::ActionManager::registerAction(m_stopAction, "Analyzer.Stop", globalcontext);
    m_menu->addAction(command, G_ANALYZER_CONTROL);

    m_menu->addSeparator(globalcontext, G_ANALYZER_TOOLS);
    m_menu->addSeparator(globalcontext, G_ANALYZER_REMOTE_TOOLS);
    m_menu->addSeparator(globalcontext, G_ANALYZER_OPTIONS);
}

void AnalyzerManagerPrivate::delayedInit()
{
    if (m_mode)
        return;

    m_mode = new AnalyzerMode(q);
    createModeMainWindow();

    connect(ModeManager::instance(), SIGNAL(currentModeChanged(Core::IMode*)),
            this, SLOT(modeChanged(Core::IMode*)));

    // Right-side window with editor, output etc.
    MiniSplitter *mainWindowSplitter = new MiniSplitter;
    mainWindowSplitter->addWidget(m_mainWindow);
    mainWindowSplitter->addWidget(new OutputPanePlaceHolder(m_mode, mainWindowSplitter));
    mainWindowSplitter->setStretchFactor(0, 10);
    mainWindowSplitter->setStretchFactor(1, 0);
    mainWindowSplitter->setOrientation(Qt::Vertical);

    // Navigation + right-side window.
    MiniSplitter *splitter = new MiniSplitter;
    splitter->addWidget(new NavigationWidgetPlaceHolder(m_mode));
    splitter->addWidget(mainWindowSplitter);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    Core::IContext *modeContextObject = new Core::IContext(this);
    modeContextObject->setContext(Core::Context(Core::Constants::C_EDITORMANAGER));
    modeContextObject->setWidget(splitter);
    Core::ICore::addContextObject(modeContextObject);
    m_mode->setWidget(splitter);

    AnalyzerPlugin::instance()->addAutoReleasedObject(m_mode);

    // Populate Windows->Views menu with standard actions.
    Context analyzerContext(C_ANALYZEMODE);
    ActionContainer *viewsMenu = Core::ActionManager::actionContainer(Id(M_WINDOW_VIEWS));
    Command *cmd = Core::ActionManager::registerAction(m_mainWindow->menuSeparator1(),
        "Analyzer.Views.Separator1", analyzerContext);
    cmd->setAttribute(Command::CA_Hide);
    viewsMenu->addAction(cmd, G_DEFAULT_THREE);
    cmd = Core::ActionManager::registerAction(m_mainWindow->autoHideTitleBarsAction(),
        "Analyzer.Views.AutoHideTitleBars", analyzerContext);
    cmd->setAttribute(Command::CA_Hide);
    viewsMenu->addAction(cmd, G_DEFAULT_THREE);
    cmd = Core::ActionManager::registerAction(m_mainWindow->menuSeparator2(),
        "Analyzer.Views.Separator2", analyzerContext);
    cmd->setAttribute(Command::CA_Hide);
    viewsMenu->addAction(cmd, G_DEFAULT_THREE);
    cmd = Core::ActionManager::registerAction(m_mainWindow->resetLayoutAction(),
        "Analyzer.Views.ResetSimple", analyzerContext);
    cmd->setAttribute(Command::CA_Hide);
    viewsMenu->addAction(cmd, G_DEFAULT_THREE);
}

static QToolButton *toolButton(QAction *action)
{
    QToolButton *button = new QToolButton;
    button->setDefaultAction(action);
    return button;
}

void AnalyzerManagerPrivate::createModeMainWindow()
{
    m_mainWindow = new FancyMainWindow();
    m_mainWindow->setObjectName(QLatin1String("AnalyzerManagerMainWindow"));
    m_mainWindow->setDocumentMode(true);
    m_mainWindow->setDockNestingEnabled(true);
    m_mainWindow->setDockActionsVisible(false);
    connect(m_mainWindow, SIGNAL(resetLayout()), SLOT(resetLayout()));

    QBoxLayout *editorHolderLayout = new QVBoxLayout;
    editorHolderLayout->setMargin(0);
    editorHolderLayout->setSpacing(0);

    QWidget *editorAndFindWidget = new QWidget;
    editorAndFindWidget->setLayout(editorHolderLayout);
    editorHolderLayout->addWidget(new EditorManagerPlaceHolder(m_mode));
    editorHolderLayout->addWidget(new FindToolBarPlaceHolder(editorAndFindWidget));

    MiniSplitter *documentAndRightPane = new MiniSplitter;
    documentAndRightPane->addWidget(editorAndFindWidget);
    documentAndRightPane->addWidget(new RightPanePlaceHolder(m_mode));
    documentAndRightPane->setStretchFactor(0, 1);
    documentAndRightPane->setStretchFactor(1, 0);

    StyledBar *analyzeToolBar = new StyledBar;
    analyzeToolBar->setProperty("topBorder", true);
    QHBoxLayout *analyzeToolBarLayout = new QHBoxLayout(analyzeToolBar);
    analyzeToolBarLayout->setMargin(0);
    analyzeToolBarLayout->setSpacing(0);
    analyzeToolBarLayout->addWidget(toolButton(m_startAction));
    analyzeToolBarLayout->addWidget(toolButton(m_stopAction));
    analyzeToolBarLayout->addWidget(new StyledSeparator);
    analyzeToolBarLayout->addWidget(m_toolBox);
    analyzeToolBarLayout->addWidget(m_controlsStackWidget);
    analyzeToolBarLayout->addWidget(m_statusLabel);
    analyzeToolBarLayout->addStretch();

    QDockWidget *dock = new QDockWidget(tr("Analyzer Toolbar"));
    dock->setObjectName(QLatin1String("Analyzer Toolbar"));
    dock->setWidget(analyzeToolBar);
    dock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    dock->setProperty("managed_dockwidget", QLatin1String("true"));
    dock->setAllowedAreas(Qt::BottomDockWidgetArea);
    // hide title bar
    dock->setTitleBarWidget(new QWidget(dock));
    m_mainWindow->addDockWidget(Qt::BottomDockWidgetArea, dock);
    m_mainWindow->setToolBarDockWidget(dock);

    QWidget *centralWidget = new QWidget;
    m_mainWindow->setCentralWidget(centralWidget);

    QVBoxLayout *centralLayout = new QVBoxLayout(centralWidget);
    centralWidget->setLayout(centralLayout);
    centralLayout->setMargin(0);
    centralLayout->setSpacing(0);
    centralLayout->addWidget(documentAndRightPane);
    centralLayout->setStretch(0, 1);
    centralLayout->setStretch(1, 0);
}

void AnalyzerManagerPrivate::activateDock(Qt::DockWidgetArea area, QDockWidget *dockWidget)
{
    dockWidget->setParent(m_mainWindow);
    m_mainWindow->addDockWidget(area, dockWidget);

    Context globalContext(C_GLOBAL);

    QAction *toggleViewAction = dockWidget->toggleViewAction();
    toggleViewAction->setText(dockWidget->windowTitle());
    Command *cmd = ActionManager::registerAction(toggleViewAction,
        Id("Analyzer.").withSuffix(dockWidget->objectName()), globalContext);
    cmd->setAttribute(Command::CA_Hide);

    ActionContainer *viewsMenu = Core::ActionManager::actionContainer(Id(M_WINDOW_VIEWS));
    viewsMenu->addAction(cmd);
}

void AnalyzerManagerPrivate::deactivateDock(QDockWidget *dockWidget)
{
    QAction *toggleViewAction = dockWidget->toggleViewAction();
    ActionManager::unregisterAction(toggleViewAction,
        Id("Analyzer.").withSuffix(dockWidget->objectName()));
    m_mainWindow->removeDockWidget(dockWidget);
    dockWidget->hide();
    // Prevent saveState storing the data of the wrong children.
    dockWidget->setParent(0);
}

bool AnalyzerManagerPrivate::showPromptDialog(const QString &title, const QString &text,
    const QString &stopButtonText, const QString &cancelButtonText) const
{
    CheckableMessageBox messageBox(ICore::mainWindow());
    messageBox.setWindowTitle(title);
    messageBox.setText(text);
    messageBox.setStandardButtons(QDialogButtonBox::Yes|QDialogButtonBox::Cancel);
    if (!stopButtonText.isEmpty())
        messageBox.button(QDialogButtonBox::Yes)->setText(stopButtonText);
    if (!cancelButtonText.isEmpty())
        messageBox.button(QDialogButtonBox::Cancel)->setText(cancelButtonText);
    messageBox.setDefaultButton(QDialogButtonBox::Yes);
    messageBox.setCheckBoxVisible(false);
    messageBox.exec();
    return messageBox.clickedStandardButton() == QDialogButtonBox::Yes;
}

bool AnalyzerManagerPrivate::isActionRunnable(AnalyzerAction *action) const
{
    if (!action || m_isRunning)
        return false;
    if (action->startMode() == StartRemote)
        return true;

    return ProjectExplorerPlugin::canRun(SessionManager::startupProject(), action->tool()->runMode(), 0);
}

void AnalyzerManagerPrivate::startTool()
{
    QTC_ASSERT(m_currentAction, return);
    m_currentAction->tool()->startTool(m_currentAction->startMode());
}

void AnalyzerManagerPrivate::modeChanged(IMode *mode)
{
    if (mode && mode == m_mode) {
        m_mainWindow->setDockActionsVisible(true);
        static bool firstTime = true;
        if (firstTime)
            selectSavedTool();
        firstTime = false;
        updateRunActions();
    } else {
        m_mainWindow->setDockActionsVisible(false);
    }
}

void AnalyzerManagerPrivate::selectSavedTool()
{
    const QSettings *settings = ICore::settings();

    if (settings->contains(QLatin1String(LAST_ACTIVE_TOOL))) {
        const Id lastAction = Id::fromSetting(settings->value(QLatin1String(LAST_ACTIVE_TOOL)));
        foreach (AnalyzerAction *action, m_actions) {
            if (action->id() == lastAction) {
                selectAction(action);
                return;
            }
        }
    }
    // fallback to first available tool
    if (!m_actions.isEmpty())
        selectAction(m_actions.first());
}

void AnalyzerManagerPrivate::selectMenuAction()
{
    AnalyzerManager::showMode();
    AnalyzerAction *action = qobject_cast<AnalyzerAction *>(sender());
    QTC_ASSERT(action, return);
    selectAction(action);
    startTool();
}

void AnalyzerManagerPrivate::selectToolboxAction(int index)
{
    selectAction(m_actions[index]);
}

void AnalyzerManagerPrivate::selectAction(AnalyzerAction *action)
{
    if (m_currentAction == action)
        return;

    const int actionIndex = m_actions.indexOf(action);
    QTC_ASSERT(actionIndex >= 0, return);

    // Clean up old tool.
    if (m_currentAction) {
        saveToolSettings(m_currentAction);
        foreach (QDockWidget *widget, m_toolWidgets.value(m_currentAction->tool()))
            deactivateDock(widget);
    }

    // Now change the tool.
    m_currentAction = action;

    IAnalyzerTool *tool = action->tool();
    if (!m_defaultSettings.contains(tool)) {
        QWidget *widget = tool->createWidgets();
        QTC_CHECK(widget);
        m_defaultSettings.insert(tool, m_mainWindow->saveSettings());
        QTC_CHECK(!m_controlsWidgetFromTool.contains(tool));
        m_controlsWidgetFromTool[tool] = widget;
        m_controlsStackWidget->addWidget(widget);
    }
    foreach (QDockWidget *widget, m_toolWidgets.value(tool))
        activateDock(Qt::DockWidgetArea(widget->property(INITIAL_DOCK_AREA).toInt()), widget);

    loadToolSettings(action);

    QTC_CHECK(m_controlsWidgetFromTool.contains(tool));
    m_controlsStackWidget->setCurrentWidget(m_controlsWidgetFromTool.value(tool));
    m_toolBox->setCurrentIndex(actionIndex);

    updateRunActions();
}

void AnalyzerManagerPrivate::addAction(AnalyzerAction *action)
{
    delayedInit(); // Make sure that there is a valid IMode instance.

    const bool blocked = m_toolBox->blockSignals(true); // Do not make current.

    Id menuGroup = action->menuGroup();
    if (menuGroup.isValid()) {
        Command *command = ActionManager::registerAction(action, action->id(), Context(C_GLOBAL));
        m_menu->addAction(command, menuGroup);
    }

    m_actions.append(action);
    m_toolBox->addItem(action->text());
    m_toolBox->blockSignals(blocked);
    connect(action, SIGNAL(triggered()), SLOT(selectMenuAction()));
    m_toolBox->setEnabled(true);
}

void AnalyzerManagerPrivate::handleToolStarted()
{
    m_isRunning = true; // FIXME: Make less global.
    updateRunActions();
}

void AnalyzerManagerPrivate::handleToolFinished()
{
    m_isRunning = false;
    updateRunActions();
}

void AnalyzerManagerPrivate::loadToolSettings(AnalyzerAction *action)
{
    QTC_ASSERT(m_mainWindow, return);
    QSettings *settings = ICore::settings();
    settings->beginGroup(QLatin1String("AnalyzerViewSettings_") + action->id().toString());
    if (settings->value(QLatin1String("ToolSettingsSaved"), false).toBool())
        m_mainWindow->restoreSettings(settings);
    else
        m_mainWindow->restoreSettings(m_defaultSettings.value(action->tool()));
    settings->endGroup();
}

void AnalyzerManagerPrivate::saveToolSettings(AnalyzerAction *action)
{
    QTC_ASSERT(action, return);
    QTC_ASSERT(m_mainWindow, return);

    QSettings *settings = ICore::settings();
    settings->beginGroup(QLatin1String("AnalyzerViewSettings_") + action->id().toString());
    m_mainWindow->saveSettings(settings);
    settings->setValue(QLatin1String("ToolSettingsSaved"), true);
    settings->endGroup();
    settings->setValue(QLatin1String(LAST_ACTIVE_TOOL), action->id().toString());
}

void AnalyzerManagerPrivate::updateRunActions()
{
    QString disabledReason;
    if (m_isRunning)
        disabledReason = tr("An analysis is still in progress.");
    else if (!m_currentAction)
        disabledReason = tr("No analyzer tool selected.");
    else
        ProjectExplorerPlugin::canRun(SessionManager::startupProject(),
                                      m_currentAction->tool()->runMode(), &disabledReason);

    m_startAction->setEnabled(isActionRunnable(m_currentAction));
    m_startAction->setToolTip(disabledReason);
    m_toolBox->setEnabled(!m_isRunning);
    m_stopAction->setEnabled(m_isRunning);
    foreach (AnalyzerAction *action, m_actions)
        action->setEnabled(isActionRunnable(action));
}

////////////////////////////////////////////////////////////////////
//
// AnalyzerManager
//
////////////////////////////////////////////////////////////////////

static AnalyzerManagerPrivate *d = 0;

AnalyzerManager::AnalyzerManager(QObject *parent)
  : QObject(parent)
{
    QTC_CHECK(d == 0);
    d = new AnalyzerManagerPrivate(this);
}

AnalyzerManager::~AnalyzerManager()
{
    QTC_CHECK(d);
    delete d;
    d = 0;
}

void AnalyzerManager::shutdown()
{
    if (d->m_currentAction)
        d->saveToolSettings(d->m_currentAction);
}

void AnalyzerManager::addAction(AnalyzerAction *action)
{
    d->addAction(action);
}

QDockWidget *AnalyzerManager::createDockWidget(IAnalyzerTool *tool,
                                               QWidget *widget, Qt::DockWidgetArea area)
{
    QTC_ASSERT(!widget->objectName().isEmpty(), return 0);
    QDockWidget *dockWidget = d->m_mainWindow->addDockForWidget(widget);
    dockWidget->setProperty(INITIAL_DOCK_AREA, int(area));
    d->m_dockWidgets.append(AnalyzerManagerPrivate::DockPtr(dockWidget));
    d->m_toolWidgets[tool].push_back(dockWidget);
    return dockWidget;
}

void AnalyzerManager::selectTool(IAnalyzerTool *tool, StartMode mode)
{
    foreach (AnalyzerAction *action, d->m_actions)
        if (action->tool() == tool && action->startMode() == mode)
            d->selectAction(action);
}

void AnalyzerManager::startTool()
{
    d->startTool();
}

FancyMainWindow *AnalyzerManager::mainWindow()
{
    return d->m_mainWindow;
}

void AnalyzerManagerPrivate::resetLayout()
{
    QTC_ASSERT(m_currentAction, return);
    m_mainWindow->restoreSettings(m_defaultSettings.value(m_currentAction->tool()));
}

void AnalyzerManager::showStatusMessage(const QString &message, int timeoutMS)
{
    d->m_statusLabel->showStatusMessage(message, timeoutMS);
}

void AnalyzerManager::showPermanentStatusMessage(const QString &message)
{
    showStatusMessage(message, -1);
}

void AnalyzerManager::showMode()
{
    if (d->m_mode)
        ModeManager::activateMode(d->m_mode->id());
}

void AnalyzerManager::stopTool()
{
    stopAction()->trigger();
}

QAction *AnalyzerManager::stopAction()
{
    return d->m_stopAction;
}

void AnalyzerManager::handleToolStarted()
{
    d->handleToolStarted();
}

void AnalyzerManager::handleToolFinished()
{
    d->handleToolFinished();
}

AnalyzerRunControl *AnalyzerManager::createRunControl(
    const AnalyzerStartParameters &sp, RunConfiguration *runConfiguration)
{
    foreach (AnalyzerAction *action, d->m_actions) {
        IAnalyzerTool *tool = action->tool();
        if (tool->runMode() == sp.runMode && action->startMode() == sp.startMode)
            return tool->createRunControl(sp, runConfiguration);
    }
    QTC_CHECK(false);
    return 0;
}

} // namespace Analyzer

#include "analyzermanager.moc"
