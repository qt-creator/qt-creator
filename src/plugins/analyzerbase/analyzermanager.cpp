/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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
#include "analyzersettings.h"

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
using namespace Core::Constants;
using namespace Analyzer::Internal;
using namespace Analyzer::Constants;
using namespace ProjectExplorer;

namespace Analyzer {
namespace Internal {

const char LAST_ACTIVE_TOOL[] = "Analyzer.Plugin.LastActiveTool";
const char LAST_ACTIVE_MODE[] = "Analyzer.Plugin.LastActiveMode";
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
        setContext(Context(C_EDITORMANAGER, C_ANALYZEMODE, C_NAVIGATION_PANE));
        setDisplayName(tr("Analyze"));
        setIcon(QIcon(QLatin1String(":/images/analyzer_mode.png")));
        setPriority(P_MODE_ANALYZE);
        setId(MODE_ANALYZE);
    }

    ~AnalyzerMode()
    {
        // Make sure the editor manager does not get deleted.
        delete m_widget;
        m_widget = 0;
        EditorManager::instance()->setParent(0);
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
    void addTool(IAnalyzerTool *tool, StartMode mode);
    void selectSavedTool();
    void selectTool(QAction *action);
    void handleToolStarted();
    void handleToolFinished();
    void saveToolSettings(QAction *action);
    void loadToolSettings(IAnalyzerTool *tool);
    QAction *actionFromToolAndMode(IAnalyzerTool *tool, StartMode mode);

    // Convenience.
    bool isActionRunnable(QAction *action) const;

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
    Utils::FancyMainWindow *m_mainWindow;
    QAction *m_currentAction;
    QHash<QAction *, IAnalyzerTool *> m_toolFromAction;
    QHash<QAction *, StartMode> m_modeFromAction;
    QList<IAnalyzerTool *> m_tools;
    QList<QAction *> m_actions;
    QAction *m_startAction;
    QAction *m_stopAction;
    ActionContainer *m_menu;
    QComboBox *m_toolBox;
    QStackedWidget *m_controlsStackWidget;
    Utils::StatusLabel *m_statusLabel;
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
    m_statusLabel(new Utils::StatusLabel)
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
    command = Core::ActionManager::registerAction(m_startAction, "Analyzer.Start", globalcontext);
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
    m_mode->setWidget(splitter);

    AnalyzerPlugin::instance()->addAutoReleasedObject(m_mode);

    // Populate Windows->Views menu with standard actions.
    Context analyzerContext(C_ANALYZEMODE);
    ActionContainer *viewsMenu = Core::ActionManager::actionContainer(Id(M_WINDOW_VIEWS));
    Command *cmd = Core::ActionManager::registerAction(m_mainWindow->menuSeparator1(),
        Id("Analyzer.Views.Separator1"), analyzerContext);
    cmd->setAttribute(Command::CA_Hide);
    viewsMenu->addAction(cmd, G_DEFAULT_THREE);
    cmd = Core::ActionManager::registerAction(m_mainWindow->toggleLockedAction(),
        Id("Analyzer.Views.ToggleLocked"), analyzerContext);
    cmd->setAttribute(Command::CA_Hide);
    viewsMenu->addAction(cmd, G_DEFAULT_THREE);
    cmd = Core::ActionManager::registerAction(m_mainWindow->menuSeparator2(),
        Id("Analyzer.Views.Separator2"), analyzerContext);
    cmd->setAttribute(Command::CA_Hide);
    viewsMenu->addAction(cmd, G_DEFAULT_THREE);
    cmd = Core::ActionManager::registerAction(m_mainWindow->resetLayoutAction(),
        Id("Analyzer.Views.ResetSimple"), analyzerContext);
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
    m_mainWindow = new Utils::FancyMainWindow();
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

    Utils::StyledBar *analyzeToolBar = new Utils::StyledBar;
    analyzeToolBar->setProperty("topBorder", true);
    QHBoxLayout *analyzeToolBarLayout = new QHBoxLayout(analyzeToolBar);
    analyzeToolBarLayout->setMargin(0);
    analyzeToolBarLayout->setSpacing(0);
    analyzeToolBarLayout->addWidget(toolButton(m_startAction));
    analyzeToolBarLayout->addWidget(toolButton(m_stopAction));
    analyzeToolBarLayout->addWidget(new Utils::StyledSeparator);
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
    Utils::CheckableMessageBox messageBox(ICore::mainWindow());
    messageBox.setWindowTitle(title);
    messageBox.setText(text);
    messageBox.setStandardButtons(QDialogButtonBox::Yes|QDialogButtonBox::Cancel);
    if (!stopButtonText.isEmpty())
        messageBox.button(QDialogButtonBox::Yes)->setText(stopButtonText);
    if (!cancelButtonText.isEmpty())
        messageBox.button(QDialogButtonBox::Cancel)->setText(cancelButtonText);
    messageBox.setDefaultButton(QDialogButtonBox::Yes);
    messageBox.setCheckBoxVisible(false);
    messageBox.exec();;
    return messageBox.clickedStandardButton() == QDialogButtonBox::Yes;
}

bool AnalyzerManagerPrivate::isActionRunnable(QAction *action) const
{
    if (!action || m_isRunning)
        return false;
    if (m_modeFromAction.value(action) == StartRemote)
        return true;

    IAnalyzerTool *tool = m_toolFromAction.value(action);
    ProjectExplorerPlugin *pe = ProjectExplorerPlugin::instance();
    return pe->canRun(pe->startupProject(), tool->runMode());
}

void AnalyzerManagerPrivate::startTool()
{
    IAnalyzerTool *tool = m_toolFromAction.value(m_currentAction);
    StartMode mode = m_modeFromAction.value(m_currentAction);
    QTC_ASSERT(tool, return);
    tool->startTool(mode);
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

QAction *AnalyzerManagerPrivate::actionFromToolAndMode(IAnalyzerTool *tool, StartMode mode)
{
    foreach (QAction *action, m_actions)
        if (m_toolFromAction.value(action) == tool && m_modeFromAction[action] == mode)
            return action;
    QTC_CHECK(false);
    return 0;
}

void AnalyzerManagerPrivate::selectSavedTool()
{
    const QSettings *settings = ICore::settings();

    if (settings->contains(QLatin1String(LAST_ACTIVE_TOOL))) {
        const Id lastTool = Id::fromSetting(settings->value(QLatin1String(LAST_ACTIVE_TOOL)));
        const StartMode lastMode = StartMode(settings->value(QLatin1String(LAST_ACTIVE_MODE)).toInt());
        foreach (IAnalyzerTool *tool, m_tools) {
            if (tool->id() == lastTool) {
                selectTool(actionFromToolAndMode(tool, lastMode));
                return;
            }
        }
    }
    // fallback to first available tool
    if (!m_actions.isEmpty())
        selectTool(m_actions.first());
}

void AnalyzerManagerPrivate::selectMenuAction()
{
    AnalyzerManager::showMode();
    QAction *action = qobject_cast<QAction *>(sender());
    QTC_ASSERT(action, return);
    selectTool(action);
    startTool();
}

void AnalyzerManagerPrivate::selectToolboxAction(int index)
{
    selectTool(m_actions[index]);
}

void AnalyzerManagerPrivate::selectTool(QAction *action)
{
    if (m_currentAction == action)
        return;

    const int actionIndex = m_actions.indexOf(action);
    QTC_ASSERT(actionIndex >= 0, return);

    // Clean up old tool.
    if (IAnalyzerTool *oldTool = m_toolFromAction.value(m_currentAction)) {
        saveToolSettings(m_currentAction);
        foreach (QDockWidget *widget, m_toolWidgets.value(oldTool))
            deactivateDock(widget);
    }

    // Now change the tool.
    m_currentAction = action;

    IAnalyzerTool *tool = m_toolFromAction.value(action);
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

    loadToolSettings(tool);

    QTC_CHECK(m_controlsWidgetFromTool.contains(tool));
    m_controlsStackWidget->setCurrentWidget(m_controlsWidgetFromTool.value(tool));
    m_toolBox->setCurrentIndex(actionIndex);

    updateRunActions();
}

void AnalyzerManagerPrivate::addTool(IAnalyzerTool *tool, StartMode mode)
{
    delayedInit(); // Make sure that there is a valid IMode instance.

    const bool blocked = m_toolBox->blockSignals(true); // Do not make current.

    QString actionName = tool->displayName();
    Id menuGroup = Constants::G_ANALYZER_TOOLS;
    if (mode == StartRemote) {
        actionName += IAnalyzerTool::tr(" (External)");
        menuGroup = Constants::G_ANALYZER_REMOTE_TOOLS;
    }
    Id actionId = tool->id().withSuffix(mode);
    QAction *action = new QAction(actionName, this);
    Command *command = Core::ActionManager::registerAction(action, actionId, Context(C_GLOBAL));
    m_menu->addAction(command, menuGroup);
    command->action()->setData(int(StartLocal));
    // Assuming this happens before project loading.
    if (mode == StartLocal)
        command->action()->setEnabled(false);
    m_actions.append(action);
    m_toolFromAction[action] = tool;
    m_modeFromAction[action] = mode;
    m_toolBox->addItem(actionName);
    m_toolBox->blockSignals(blocked);
    connect(action, SIGNAL(triggered()), SLOT(selectMenuAction()));

    if (!m_tools.contains(tool))
        m_tools.append(tool);
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

void AnalyzerManagerPrivate::loadToolSettings(IAnalyzerTool *tool)
{
    QTC_ASSERT(m_mainWindow, return);
    QSettings *settings = ICore::settings();
    settings->beginGroup(QLatin1String("AnalyzerViewSettings_") + tool->id().toString());
    if (settings->value(QLatin1String("ToolSettingsSaved"), false).toBool())
        m_mainWindow->restoreSettings(settings);
    else
        m_mainWindow->restoreSettings(m_defaultSettings.value(tool));
    settings->endGroup();
}

void AnalyzerManagerPrivate::saveToolSettings(QAction *action)
{
    IAnalyzerTool *tool = m_toolFromAction.value(action);
    StartMode mode = m_modeFromAction.value(action);
    if (!tool)
        return; // no active tool, do nothing
    QTC_ASSERT(m_mainWindow, return);

    QSettings *settings = ICore::settings();
    settings->beginGroup(QLatin1String("AnalyzerViewSettings_") + tool->id().toString());
    m_mainWindow->saveSettings(settings);
    settings->setValue(QLatin1String("ToolSettingsSaved"), true);
    settings->endGroup();
    settings->setValue(QLatin1String(LAST_ACTIVE_TOOL), tool->id().toString());
    settings->setValue(QLatin1String(LAST_ACTIVE_MODE), int(mode));
}

void AnalyzerManagerPrivate::updateRunActions()
{
    ProjectExplorerPlugin *pe = ProjectExplorerPlugin::instance();
    Project *project = pe->startupProject();

    bool startEnabled = isActionRunnable(m_currentAction);
    IAnalyzerTool *tool = m_toolFromAction.value(m_currentAction);

    QString disabledReason;
    if (m_isRunning)
        disabledReason = tr("An analysis is still in progress.");
    else if (!tool)
        disabledReason = tr("No analyzer tool selected.");
    else
        disabledReason = pe->cannotRunReason(project, tool->runMode());

    m_startAction->setEnabled(startEnabled);
    m_startAction->setToolTip(disabledReason);
    m_toolBox->setEnabled(!m_isRunning);
    m_stopAction->setEnabled(m_isRunning);
    foreach (QAction *action, m_actions)
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

void AnalyzerManager::extensionsInitialized()
{
    foreach (IAnalyzerTool *tool, d->m_tools)
        tool->extensionsInitialized();
}

void AnalyzerManager::shutdown()
{
    d->saveToolSettings(d->m_currentAction);
}

void AnalyzerManager::addTool(IAnalyzerTool *tool, StartMode mode)
{
    d->addTool(tool, mode);
}

QDockWidget *AnalyzerManager::createDockWidget(IAnalyzerTool *tool, const QString &title,
                                               QWidget *widget, Qt::DockWidgetArea area)
{
    QTC_ASSERT(!widget->objectName().isEmpty(), return 0);
    QDockWidget *dockWidget = d->m_mainWindow->addDockForWidget(widget);
    dockWidget->setProperty(INITIAL_DOCK_AREA, int(area));
    d->m_dockWidgets.append(AnalyzerManagerPrivate::DockPtr(dockWidget));
    dockWidget->setWindowTitle(title);
    d->m_toolWidgets[tool].push_back(dockWidget);
    return dockWidget;
}

QList<IAnalyzerTool *> AnalyzerManager::tools()
{
    return d->m_tools;
}

void AnalyzerManager::selectTool(IAnalyzerTool *tool, StartMode mode)
{
    d->selectTool(d->actionFromToolAndMode(tool, mode));
}

void AnalyzerManager::startTool()
{
    d->startTool();
}

Utils::FancyMainWindow *AnalyzerManager::mainWindow()
{
    return d->m_mainWindow;
}

void AnalyzerManagerPrivate::resetLayout()
{
    IAnalyzerTool *tool = m_toolFromAction.value(m_currentAction);
    m_mainWindow->restoreSettings(m_defaultSettings.value(tool));
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
    foreach (IAnalyzerTool *tool, d->m_tools)
        if (tool->runMode() == sp.runMode)
            return tool->createRunControl(sp, runConfiguration);
    QTC_CHECK(false);
    return 0;
}

} // namespace Analyzer

#include "analyzermanager.moc"
