/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Author: Andreas Hartmetz, KDAB (andreas.hartmetz@kdab.com)
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

#include "analyzermanager.h"

#include "analyzericons.h"
#include "analyzerstartparameters.h"
#include "analyzerruncontrol.h"
#include "../debuggerplugin.h"

#include "../debuggermainwindow.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreicons.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/findplaceholder.h>
#include <coreplugin/icore.h>
#include <coreplugin/imode.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/navigationwidget.h>
#include <coreplugin/outputpane.h>
#include <coreplugin/rightpane.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>

#include <utils/algorithm.h>
#include <utils/fancymainwindow.h>
#include <utils/styledbar.h>
#include <utils/qtcassert.h>
#include <utils/checkablemessagebox.h>
#include <utils/statuslabel.h>

#include <QAction>
#include <QComboBox>
#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QHBoxLayout>
#include <QMenu>
#include <QPointer>
#include <QPushButton>
#include <QStackedWidget>
#include <QSettings>
#include <QToolButton>
#include <QVBoxLayout>
#include <QVariant>

using namespace Core;
using namespace Utils;
using namespace Core::Constants;
using namespace Analyzer::Constants;
using namespace ProjectExplorer;
using namespace Debugger;
using namespace Debugger::Internal;

namespace Analyzer {

bool ActionDescription::isRunnable(QString *reason) const
{
    if (m_customToolStarter) // Something special. Pretend we can always run it.
        return true;

    return ProjectExplorerPlugin::canRun(SessionManager::startupProject(), m_runMode, reason);
}

static bool buildTypeAccepted(QFlags<ToolMode> toolMode, BuildConfiguration::BuildType buildType)
{
    if (buildType == BuildConfiguration::Unknown)
        return true;
    if (buildType == BuildConfiguration::Debug && (toolMode & DebugMode))
        return true;
    if (buildType == BuildConfiguration::Release && (toolMode & ReleaseMode))
        return true;
    if (buildType == BuildConfiguration::Profile && (toolMode & ProfileMode))
        return true;
    return false;
}

void ActionDescription::startTool() const
{
    // Make sure mode is shown.
    AnalyzerManager::showMode();

    TaskHub::clearTasks(Constants::ANALYZERTASK_ID);
    AnalyzerManager::showPermanentStatusMessage(QString());

    if (m_toolPreparer && !m_toolPreparer())
        return;

    // ### not sure if we're supposed to check if the RunConFiguration isEnabled
    Project *pro = SessionManager::startupProject();
    RunConfiguration *rc = 0;
    BuildConfiguration::BuildType buildType = BuildConfiguration::Unknown;
    if (pro) {
        if (const Target *target = pro->activeTarget()) {
            // Build configuration is 0 for QML projects.
            if (const BuildConfiguration *buildConfig = target->activeBuildConfiguration())
                buildType = buildConfig->buildType();
            rc = target->activeRunConfiguration();
        }
    }

    // Custom start.
    if (m_customToolStarter) {
        if (rc) {
            m_customToolStarter(rc);
        } else {
            QMessageBox *errorDialog = new QMessageBox(ICore::mainWindow());
            errorDialog->setIcon(QMessageBox::Warning);
            errorDialog->setWindowTitle(m_text);
            errorDialog->setText(tr("Cannot start %1 without a project. Please open the project "
                                    "and try again.").arg(m_text));
            errorDialog->setStandardButtons(QMessageBox::Ok);
            errorDialog->setDefaultButton(QMessageBox::Ok);
            errorDialog->show();
        }
        return;
    }

    // Check the project for whether the build config is in the correct mode
    // if not, notify the user and urge him to use the correct mode.
    if (!buildTypeAccepted(m_toolMode, buildType)) {
        QString currentMode;
        switch (buildType) {
            case BuildConfiguration::Debug:
                currentMode = AnalyzerManager::tr("Debug");
                break;
            case BuildConfiguration::Profile:
                currentMode = AnalyzerManager::tr("Profile");
                break;
            case BuildConfiguration::Release:
                currentMode = AnalyzerManager::tr("Release");
                break;
            default:
                QTC_CHECK(false);
        }

        QString toolModeString;
        switch (m_toolMode) {
            case DebugMode:
                toolModeString = AnalyzerManager::tr("in Debug mode");
                break;
            case ProfileMode:
                toolModeString = AnalyzerManager::tr("in Profile mode");
                break;
            case ReleaseMode:
                toolModeString = AnalyzerManager::tr("in Release mode");
                break;
            case SymbolsMode:
                toolModeString = AnalyzerManager::tr("with debug symbols (Debug or Profile mode)");
                break;
            case OptimizedMode:
                toolModeString = AnalyzerManager::tr("on optimized code (Profile or Release mode)");
                break;
            default:
                QTC_CHECK(false);
        }
        const QString toolName = m_text; // The action text is always the name of the tool
        const QString title = AnalyzerManager::tr("Run %1 in %2 Mode?").arg(toolName).arg(currentMode);
        const QString message = AnalyzerManager::tr("<html><head/><body><p>You are trying "
            "to run the tool \"%1\" on an application in %2 mode. "
            "The tool is designed to be used %3.</p><p>"
            "Run-time characteristics differ significantly between "
            "optimized and non-optimized binaries. Analytical "
            "findings for one mode may or may not be relevant for "
            "the other.</p><p>"
            "Running tools that need debug symbols on binaries that "
            "don't provide any may lead to missing function names "
            "or otherwise insufficient output.</p><p>"
            "Do you want to continue and run the tool in %2 mode?</p></body></html>")
                .arg(toolName).arg(currentMode).arg(toolModeString);
        if (Utils::CheckableMessageBox::doNotAskAgainQuestion(ICore::mainWindow(),
                title, message, ICore::settings(), QLatin1String("AnalyzerCorrectModeWarning"))
                    != QDialogButtonBox::Yes)
            return;
    }

    ProjectExplorerPlugin::runStartupProject(m_runMode);
}

namespace Internal {

const char LAST_ACTIVE_ACTION[] = "Analyzer.Plugin.LastActiveTool";

////////////////////////////////////////////////////////////////////
//
// AnalyzerMode
//
////////////////////////////////////////////////////////////////////

class AnalyzerMode : public IMode
{
public:
    AnalyzerMode(QObject *parent = 0)
        : IMode(parent)
    {
        setContext(Context(C_ANALYZEMODE, C_NAVIGATION_PANE));
        setDisplayName(AnalyzerManager::tr("Analyze"));
        setIcon(Utils::Icon::modeIcon(Icons::MODE_ANALYZE_CLASSIC,
                                      Icons::MODE_ANALYZE_FLAT, Icons::MODE_ANALYZE_FLAT_ACTIVE));
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
    Q_DECLARE_TR_FUNCTIONS(Analyzer::AnalyzerManager)

public:
    AnalyzerManagerPrivate(AnalyzerManager *qq);

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

    void registerAction(Core::Id actionId, const ActionDescription &desc);
    void selectSavedTool();
    void selectAction(Id actionId);
    void handleToolStarted();
    void handleToolFinished();
    void modeChanged(IMode *mode);
    void resetLayout();
    void updateRunActions();

public:
    AnalyzerManager *q;
    Internal::AnalyzerMode *m_mode = 0;
    bool m_isRunning = false;
    Core::Id m_currentActionId;
    QHash<Id, QAction *> m_actions;
    QHash<Id, ActionDescription> m_descriptions;
    QAction *m_startAction = 0;
    QAction *m_stopAction = 0;
    ActionContainer *m_menu = 0;
    MainWindowBase *m_mainWindow;
};

AnalyzerManagerPrivate::AnalyzerManagerPrivate(AnalyzerManager *qq):
    q(qq), m_mainWindow(new MainWindowBase)
{
    QComboBox *toolBox = m_mainWindow->toolBox();
    toolBox->setObjectName(QLatin1String("AnalyzerManagerToolBox"));
    toolBox->insertSeparator(0);
    connect(toolBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, [this, toolBox](int item) {
        selectAction(Id::fromSetting(toolBox->itemData(item)));
    });

    setupActions();

    ProjectExplorerPlugin *pe = ProjectExplorerPlugin::instance();
    connect(pe, &ProjectExplorerPlugin::updateRunActions, this, &AnalyzerManagerPrivate::updateRunActions);
}

void AnalyzerManagerPrivate::setupActions()
{
    Command *command = 0;

    // Menus
    m_menu = ActionManager::createMenu(M_DEBUG_ANALYZER);
    m_menu->menu()->setTitle(tr("&Analyze"));
    m_menu->menu()->setEnabled(true);

    m_menu->appendGroup(G_ANALYZER_CONTROL);
    m_menu->appendGroup(G_ANALYZER_TOOLS);
    m_menu->appendGroup(G_ANALYZER_REMOTE_TOOLS);
    m_menu->appendGroup(G_ANALYZER_OPTIONS);

    ActionContainer *menubar = ActionManager::actionContainer(MENU_BAR);
    ActionContainer *mtools = ActionManager::actionContainer(M_TOOLS);
    menubar->addMenu(mtools, m_menu);

    m_startAction = new QAction(tr("Start"), m_menu);
    m_startAction->setIcon(Analyzer::Icons::ANALYZER_CONTROL_START.icon());
    ActionManager::registerAction(m_startAction, "Analyzer.Start");
    connect(m_startAction, &QAction::triggered, this, [this] {
        QTC_ASSERT(m_descriptions.contains(m_currentActionId), return);
        m_descriptions.value(m_currentActionId).startTool();
    });

    m_stopAction = new QAction(tr("Stop"), m_menu);
    m_stopAction->setEnabled(false);
    m_stopAction->setIcon(ProjectExplorer::Icons::STOP_SMALL.icon());
    command = ActionManager::registerAction(m_stopAction, "Analyzer.Stop");
    m_menu->addAction(command, G_ANALYZER_CONTROL);

    m_menu->addSeparator(G_ANALYZER_TOOLS);
    m_menu->addSeparator(G_ANALYZER_REMOTE_TOOLS);
    m_menu->addSeparator(G_ANALYZER_OPTIONS);
}

void AnalyzerManagerPrivate::delayedInit()
{
    if (m_mode)
        return;

    m_mode = new Internal::AnalyzerMode(q);
    createModeMainWindow();

    connect(ModeManager::instance(), &ModeManager::currentModeChanged,
            this, &AnalyzerManagerPrivate::modeChanged);

    // Right-side window with editor, output etc.
    auto mainWindowSplitter = new MiniSplitter;
    mainWindowSplitter->addWidget(m_mainWindow);
    mainWindowSplitter->addWidget(new OutputPanePlaceHolder(m_mode, mainWindowSplitter));
    mainWindowSplitter->setStretchFactor(0, 10);
    mainWindowSplitter->setStretchFactor(1, 0);
    mainWindowSplitter->setOrientation(Qt::Vertical);

    // Navigation + right-side window.
    auto splitter = new MiniSplitter;
    splitter->addWidget(new NavigationWidgetPlaceHolder(m_mode));
    splitter->addWidget(mainWindowSplitter);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);

    auto modeContextObject = new IContext(this);
    modeContextObject->setContext(Context(C_EDITORMANAGER));
    modeContextObject->setWidget(splitter);
    ICore::addContextObject(modeContextObject);
    m_mode->setWidget(splitter);

    Debugger::Internal::DebuggerPlugin::instance()->addAutoReleasedObject(m_mode);

    // Populate Windows->Views menu with standard actions.
    Context analyzerContext(C_ANALYZEMODE);
    ActionContainer *viewsMenu = ActionManager::actionContainer(M_WINDOW_VIEWS);
    Command *cmd = ActionManager::registerAction(m_mainWindow->menuSeparator1(),
        "Analyzer.Views.Separator1", analyzerContext);
    cmd->setAttribute(Command::CA_Hide);
    viewsMenu->addAction(cmd, G_DEFAULT_THREE);
    cmd = ActionManager::registerAction(m_mainWindow->autoHideTitleBarsAction(),
        "Analyzer.Views.AutoHideTitleBars", analyzerContext);
    cmd->setAttribute(Command::CA_Hide);
    viewsMenu->addAction(cmd, G_DEFAULT_THREE);
    cmd = ActionManager::registerAction(m_mainWindow->menuSeparator2(),
        "Analyzer.Views.Separator2", analyzerContext);
    cmd->setAttribute(Command::CA_Hide);
    viewsMenu->addAction(cmd, G_DEFAULT_THREE);
    cmd = ActionManager::registerAction(m_mainWindow->resetLayoutAction(),
        "Analyzer.Views.ResetSimple", analyzerContext);
    cmd->setAttribute(Command::CA_Hide);
    viewsMenu->addAction(cmd, G_DEFAULT_THREE);
}

static QToolButton *toolButton(QAction *action)
{
    auto button = new QToolButton;
    button->setDefaultAction(action);
    return button;
}

void AnalyzerManagerPrivate::createModeMainWindow()
{
    m_mainWindow->setObjectName(QLatin1String("AnalyzerManagerMainWindow"));
    m_mainWindow->setLastSettingsName(QLatin1String(Internal::LAST_ACTIVE_ACTION));

    auto editorHolderLayout = new QVBoxLayout;
    editorHolderLayout->setMargin(0);
    editorHolderLayout->setSpacing(0);

    auto editorAndFindWidget = new QWidget;
    editorAndFindWidget->setLayout(editorHolderLayout);
    editorHolderLayout->addWidget(new EditorManagerPlaceHolder(m_mode));
    editorHolderLayout->addWidget(new FindToolBarPlaceHolder(editorAndFindWidget));

    auto documentAndRightPane = new MiniSplitter;
    documentAndRightPane->addWidget(editorAndFindWidget);
    documentAndRightPane->addWidget(new RightPanePlaceHolder(m_mode));
    documentAndRightPane->setStretchFactor(0, 1);
    documentAndRightPane->setStretchFactor(1, 0);

    auto analyzeToolBar = new StyledBar;
    analyzeToolBar->setProperty("topBorder", true);

    auto analyzeToolBarLayout = new QHBoxLayout(analyzeToolBar);
    analyzeToolBarLayout->setMargin(0);
    analyzeToolBarLayout->setSpacing(0);
    analyzeToolBarLayout->addWidget(toolButton(m_startAction));
    analyzeToolBarLayout->addWidget(toolButton(m_stopAction));
    analyzeToolBarLayout->addWidget(new StyledSeparator);
    analyzeToolBarLayout->addWidget(m_mainWindow->toolBox());
    analyzeToolBarLayout->addWidget(m_mainWindow->controlsStack());
    analyzeToolBarLayout->addWidget(m_mainWindow->statusLabel());
    analyzeToolBarLayout->addStretch();

    auto dock = new QDockWidget(tr("Analyzer Toolbar"));
    dock->setObjectName(QLatin1String("Analyzer Toolbar"));
    dock->setWidget(analyzeToolBar);
    dock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    dock->setProperty("managed_dockwidget", QLatin1String("true"));
    dock->setAllowedAreas(Qt::BottomDockWidgetArea);
    // hide title bar
    dock->setTitleBarWidget(new QWidget(dock));
    m_mainWindow->addDockWidget(Qt::BottomDockWidgetArea, dock);
    m_mainWindow->setToolBarDockWidget(dock);

    auto centralWidget = new QWidget;
    m_mainWindow->setCentralWidget(centralWidget);

    auto centralLayout = new QVBoxLayout(centralWidget);
    centralWidget->setLayout(centralLayout);
    centralLayout->setMargin(0);
    centralLayout->setSpacing(0);
    centralLayout->addWidget(documentAndRightPane);
    centralLayout->setStretch(0, 1);
    centralLayout->setStretch(1, 0);
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

void AnalyzerManagerPrivate::modeChanged(IMode *mode)
{
    if (mode && mode == m_mode) {
        m_mainWindow->setDockActionsVisible(true);
        static bool firstTime = !m_currentActionId.isValid();
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

    if (settings->contains(QLatin1String(Internal::LAST_ACTIVE_ACTION))) {
        const Id lastAction = Id::fromSetting(settings->value(QLatin1String(Internal::LAST_ACTIVE_ACTION)));
        selectAction(lastAction);
    }
    // fallback to first available tool
    if (!m_descriptions.isEmpty())
        selectAction(m_descriptions.begin().key());
}

void AnalyzerManagerPrivate::selectAction(Id actionId)
{
    QTC_ASSERT(m_descriptions.contains(actionId), return);
    if (m_currentActionId == actionId)
        return;

    // Now change the tool.
    m_currentActionId = actionId;
    ActionDescription desc = m_descriptions.value(actionId);

    m_mainWindow->restorePerspective(desc.perspectiveId());

    const int toolboxIndex = m_mainWindow->toolBox()->findText(desc.text());
    QTC_ASSERT(toolboxIndex >= 0, return);
    m_mainWindow->toolBox()->setCurrentIndex(toolboxIndex);

    updateRunActions();
}

void AnalyzerManagerPrivate::registerAction(Id actionId, const ActionDescription &desc)
{
    delayedInit(); // Make sure that there is a valid IMode instance.

    auto action = new QAction(this);
    action->setText(desc.text());
    action->setToolTip(desc.toolTip());
    m_actions.insert(actionId, action);
    m_descriptions.insert(actionId, desc);

    int index = -1;
    if (desc.menuGroup() == Constants::G_ANALYZER_REMOTE_TOOLS) {
        index = m_mainWindow->toolBox()->count();
    } else if (desc.menuGroup() == Constants::G_ANALYZER_TOOLS) {
        for (int i = m_mainWindow->toolBox()->count(); --i >= 0; )
            if (m_mainWindow->toolBox()->itemText(i).isEmpty())
                index = i;
    }

    if (index >= 0)
        m_mainWindow->toolBox()->insertItem(index, desc.text(), actionId.toSetting());

    Id menuGroup = desc.menuGroup();
    if (menuGroup.isValid()) {
        Command *command = ActionManager::registerAction(action, actionId);
        m_menu->addAction(command, menuGroup);
    }

    connect(action, &QAction::triggered, this, [this, desc, actionId] {
        selectAction(actionId);
        desc.startTool();
    });
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

void AnalyzerManagerPrivate::updateRunActions()
{
    QString disabledReason;
    bool enabled = false;
    if (m_isRunning)
        disabledReason = tr("An analysis is still in progress.");
    else if (m_currentActionId.isValid())
        enabled = m_descriptions.value(m_currentActionId).isRunnable(&disabledReason);
    else
        disabledReason = tr("No analyzer tool selected.");

    m_startAction->setEnabled(enabled);
    m_startAction->setToolTip(disabledReason);
    m_mainWindow->toolBox()->setEnabled(!m_isRunning);
    m_stopAction->setEnabled(m_isRunning);

    for (auto it = m_actions.begin(), end = m_actions.end(); it != end; ++it)
        it.value()->setEnabled(!m_isRunning && m_descriptions.value(it.key()).isRunnable());
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
    d->m_mainWindow->saveCurrentPerspective();
}

void AnalyzerManager::registerAction(Id actionId, const ActionDescription &desc)
{
    d->registerAction(actionId, desc);
}

void AnalyzerManager::registerDockWidget(Id dockId, QWidget *widget)
{
    QTC_ASSERT(!widget->objectName().isEmpty(), return);
    QDockWidget *dockWidget = d->m_mainWindow->registerDockWidget(dockId, widget);

    QAction *toggleViewAction = dockWidget->toggleViewAction();
    toggleViewAction->setText(dockWidget->windowTitle());

    Command *cmd = ActionManager::registerAction(toggleViewAction,
        Id("Analyzer.").withSuffix(dockWidget->objectName()));
    cmd->setAttribute(Command::CA_Hide);

    ActionContainer *viewsMenu = ActionManager::actionContainer(Id(M_WINDOW_VIEWS));
    viewsMenu->addAction(cmd);
}

void AnalyzerManager::registerToolbar(Id toolbarId, QWidget *widget)
{
    d->m_mainWindow->registerToolbar(toolbarId, widget);
}

Perspective::Split::Split(Id dockId, Id existing, Perspective::SplitType splitType, bool visibleByDefault, Qt::DockWidgetArea area)
    : dockId(dockId), existing(existing), splitType(splitType), visibleByDefault(visibleByDefault), area(area)
{}

Perspective::Perspective(std::initializer_list<Perspective::Split> splits)
    : m_splits(splits)
{
    for (const Split &split : splits)
        m_docks.append(split.dockId);
}

void Perspective::addSplit(const Split &split)
{
    m_docks.append(split.dockId);
    m_splits.append(split);
}

void AnalyzerManager::registerPerspective(Id perspectiveId, const Perspective &perspective)
{
    d->m_mainWindow->registerPerspective(perspectiveId, perspective);
}

void AnalyzerManager::selectAction(Id actionId, bool alsoRunIt)
{
    d->selectAction(actionId);
    if (alsoRunIt)
        d->m_descriptions.value(actionId).startTool();
}

void AnalyzerManager::enableMainWindow(bool on)
{
    d->m_mainWindow->setEnabled(on);
}

void AnalyzerManager::showStatusMessage(const QString &message, int timeoutMS)
{
    d->m_mainWindow->showStatusMessage(message, timeoutMS);
}

void AnalyzerManager::showPermanentStatusMessage(const QString &message)
{
    d->m_mainWindow->showStatusMessage(message, -1);
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

AnalyzerRunControl *AnalyzerManager::createRunControl(RunConfiguration *runConfiguration, Id runMode)
{
    foreach (const ActionDescription &action, d->m_descriptions) {
        if (action.runMode() == runMode)
            return action.runControlCreator()(runConfiguration, runMode);
    }
    return 0;
}

bool operator==(const AnalyzerConnection &c1, const AnalyzerConnection &c2)
{
    return c1.connParams == c2.connParams
        && c1.analyzerHost == c2.analyzerHost
        && c1.analyzerSocket == c2.analyzerSocket
        && c1.analyzerPort == c2.analyzerPort;
}

} // namespace Analyzer
