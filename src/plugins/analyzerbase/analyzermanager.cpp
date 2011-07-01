/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Andreas Hartmetz, KDAB (andreas.hartmetz@kdab.com)
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "analyzermanager.h"

#include "analyzerconstants.h"
#include "analyzerplugin.h"
#include "analyzerruncontrol.h"
#include "analyzerruncontrolfactory.h"
#include "analyzeroptionspage.h"
#include "analyzerstartparameters.h"
#include "analyzerutils.h"
#include "ianalyzertool.h"
#include "startremotedialog.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/findplaceholder.h>
#include <coreplugin/icore.h>
#include <coreplugin/imode.h>
#include <coreplugin/minisplitter.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/navigationwidget.h>
#include <coreplugin/outputpane.h>
#include <coreplugin/rightpane.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/editormanager/editormanager.h>

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/iprojectmanager.h>
#include <projectexplorer/target.h>

#include <extensionsystem/iplugin.h>

#include <utils/fancymainwindow.h>
#include <utils/styledbar.h>
#include <utils/qtcassert.h>
#include <utils/checkablemessagebox.h>
#include <utils/statuslabel.h>
#include <utils/ssh/sshconnection.h>

#include <projectexplorer/applicationrunconfiguration.h>
#include <remotelinux/linuxdeviceconfiguration.h>
#include <remotelinux/remotelinuxrunconfiguration.h>

#include <QtCore/QVariant>
#include <QtCore/QDebug>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QDockWidget>
#include <QtGui/QAction>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QToolButton>
#include <QtGui/QComboBox>
#include <QtGui/QStackedWidget>
#include <QtGui/QDialog>
#include <QtGui/QApplication>
#include <QtGui/QLabel>
#include <QtGui/QCheckBox>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QMainWindow>
#include <QtGui/QPushButton>

using namespace Core;
using namespace Analyzer::Internal;

namespace Analyzer {
namespace Internal {

const char LAST_ACTIVE_TOOL[] = "Analyzer.Plugin.LastActiveTool";
const char INITIAL_DOCK_AREA[] = "initial_dock_area";

AnalyzerStartParameters localStartParameters(ProjectExplorer::RunConfiguration *runConfiguration)
{
    AnalyzerStartParameters sp;
    QTC_ASSERT(runConfiguration, return sp);
    ProjectExplorer::LocalApplicationRunConfiguration *rc =
            qobject_cast<ProjectExplorer::LocalApplicationRunConfiguration *>(runConfiguration);
    QTC_ASSERT(rc, return sp);

    sp.startMode = StartLocal;
    sp.environment = rc->environment();
    sp.workingDirectory = rc->workingDirectory();
    sp.debuggee = rc->executable();
    sp.debuggeeArgs = rc->commandLineArguments();
    sp.displayName = rc->displayName();
    sp.connParams.host = QLatin1String("localhost");
    sp.connParams.port = rc->qmlDebugServerPort();
    return sp;
}

AnalyzerStartParameters remoteLinuxStartParameters(ProjectExplorer::RunConfiguration *runConfiguration)
{
    AnalyzerStartParameters sp;
    RemoteLinux::RemoteLinuxRunConfiguration * const rc
        = qobject_cast<RemoteLinux::RemoteLinuxRunConfiguration *>(runConfiguration);
    QTC_ASSERT(rc, return sp);

    sp.startMode = StartRemote;
    sp.debuggee = rc->remoteExecutableFilePath();
    sp.debuggeeArgs = rc->arguments();
    sp.connParams = rc->deviceConfig()->sshParameters();
    sp.analyzerCmdPrefix = rc->commandPrefix();
    sp.displayName = rc->displayName();
    return sp;
}


// AnalyzerMode ////////////////////////////////////////////////////

class AnalyzerMode : public Core::IMode
{
    Q_OBJECT

public:
    AnalyzerMode(QObject *parent = 0)
        : Core::IMode(parent)
    {
        setContext(Core::Context(
            Core::Constants::C_EDITORMANAGER,
            Constants::C_ANALYZEMODE,
            Core::Constants::C_NAVIGATION_PANE));
        setDisplayName(tr("Analyze"));
        setIcon(QIcon(":/images/analyzer_mode.png"));
        setPriority(Constants::P_MODE_ANALYZE);
        setId(QLatin1String(Constants::MODE_ANALYZE));
        setType(Core::Constants::MODE_EDIT_TYPE);
    }

    ~AnalyzerMode()
    {
        // Make sure the editor manager does not get deleted.
        if (m_widget) {
            delete m_widget;
            m_widget = 0;
        }
        Core::EditorManager::instance()->setParent(0);
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
     * After calling this, a proper instance of Core::IMode is initialized
     * It is delayed since an analyzer mode makes no sense without analyzer tools
     *
     * \note Call this before adding a tool to the manager
     */
    void delayedInit();

    void setupActions();
    void createModeMainWindow();
    bool showPromptDialog(const QString &title, const QString &text,
        const QString &stopButtonText, const QString &cancelButtonText) const;

    void addDock(Qt::DockWidgetArea area, QDockWidget *dockWidget);
    void addTool(IAnalyzerTool *tool, const StartModes &modes);
    void selectSavedTool();
    void selectTool(IAnalyzerTool *tool, StartMode mode);
    void stopTool(IAnalyzerTool *tool, StartMode mode);
    void handleToolFinished(IAnalyzerTool *tool, StartMode mode);
    void saveToolSettings(IAnalyzerTool *tool, StartMode mode);
    void loadToolSettings(IAnalyzerTool *tool);
    QAction *actionFromToolAndMode(IAnalyzerTool *tool, StartMode mode);

    ProjectExplorer::RunControl *createRunControl
        (ProjectExplorer::RunConfiguration *runConfiguration, const QString &mode);

    // Convenience.
    void startLocalTool(IAnalyzerTool *tool, StartMode mode);
    void startRemoteTool(IAnalyzerTool *tool, StartMode mode);

public slots:
    void startTool();
    void stopTool() { stopTool(m_currentTool, m_currentMode); }

    void selectAction();
    void selectAction(QAction *);
    void selectAction(int);
    void modeChanged(Core::IMode *mode);
    void resetLayout();
    void updateRunActions();

public:
    AnalyzerManager *q;
    AnalyzerMode *m_mode;
    AnalyzerRunControlFactory *m_runControlFactory;
    ProjectExplorer::RunControl *m_currentRunControl;
    Utils::FancyMainWindow *m_mainWindow;
    IAnalyzerTool *m_currentTool;
    StartMode m_currentMode;
    QHash<QAction *, IAnalyzerTool *> m_toolFromAction;
    QHash<QAction *, StartMode> m_modeFromAction;
    QList<IAnalyzerTool *> m_tools;
    QList<QAction *> m_actions;
    QAction *m_startAction;
    QAction *m_stopAction;
    ActionContainer *m_menu;
    QComboBox *m_toolBox;
    QStackedWidget *m_controlsWidget;
    Utils::StatusLabel *m_statusLabel;
    typedef QMap<IAnalyzerTool *, FancyMainWindowSettings> MainWindowSettingsMap;
    QHash<IAnalyzerTool *, QList<QDockWidget *> > m_toolWidgets;
    MainWindowSettingsMap m_defaultSettings;

    // list of dock widgets to prevent memory leak
    typedef QWeakPointer<QDockWidget> DockPtr;
    QList<DockPtr> m_dockWidgets;

    bool m_restartOnStop;
};

AnalyzerManagerPrivate::AnalyzerManagerPrivate(AnalyzerManager *qq):
    q(qq),
    m_mode(0),
    m_runControlFactory(0),
    m_currentRunControl(0),
    m_mainWindow(0),
    m_currentTool(0),
    m_currentMode(),
    m_startAction(0),
    m_stopAction(0),
    m_menu(0),
    m_toolBox(new QComboBox),
    m_controlsWidget(new QStackedWidget),
    m_statusLabel(new Utils::StatusLabel),
    m_restartOnStop(false)
{
    m_toolBox->setObjectName(QLatin1String("AnalyzerManagerToolBox"));
    connect(m_toolBox, SIGNAL(currentIndexChanged(int)), SLOT(selectAction(int)));

    m_runControlFactory = new AnalyzerRunControlFactory();
    AnalyzerPlugin::instance()->addAutoReleasedObject(m_runControlFactory);

    setupActions();

    connect(ModeManager::instance(), SIGNAL(currentModeChanged(Core::IMode*)),
            this, SLOT(modeChanged(Core::IMode*)));
    ProjectExplorer::ProjectExplorerPlugin *pe =
        ProjectExplorer::ProjectExplorerPlugin::instance();
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
    Core::ICore *core = Core::ICore::instance();
    Core::ActionManager *am = core->actionManager();
    const Core::Context globalcontext(Core::Constants::C_GLOBAL);
    Core::Command *command = 0;

    // Menus
    m_menu = am->createMenu(Constants::M_DEBUG_ANALYZER);
    m_menu->menu()->setTitle(tr("&Analyze"));
    m_menu->menu()->setEnabled(true);

    m_menu->appendGroup(Constants::G_ANALYZER_TOOLS);
    m_menu->appendGroup(Constants::G_ANALYZER_REMOTE_TOOLS);

    Core::ActionContainer *menubar =
        am->actionContainer(Core::Constants::MENU_BAR);
    Core::ActionContainer *mtools =
        am->actionContainer(Core::Constants::M_TOOLS);
    menubar->addMenu(mtools, m_menu);

    m_startAction = new QAction(tr("Start"), m_menu);
    m_startAction->setIcon(QIcon(Constants::ANALYZER_CONTROL_START_ICON));
    command = am->registerAction(m_startAction, Constants::START, globalcontext);
    connect(m_startAction, SIGNAL(triggered()), this, SLOT(startTool()));

    m_stopAction = new QAction(tr("Stop"), m_menu);
    m_stopAction->setEnabled(false);
    m_stopAction->setIcon(QIcon(Constants::ANALYZER_CONTROL_STOP_ICON));
    command = am->registerAction(m_stopAction, Constants::STOP, globalcontext);
    connect(m_stopAction, SIGNAL(triggered()), this, SLOT(stopTool()));

    QAction *separatorAction = new QAction(m_menu);
    separatorAction->setSeparator(true);
    command = am->registerAction(separatorAction,
        "Menu.Action.Analyzer.Tools.Separator", globalcontext);
    m_menu->addAction(command, Constants::G_ANALYZER_REMOTE_TOOLS);
}

void AnalyzerManagerPrivate::delayedInit()
{
    if (m_mode)
        return;

    m_mode = new AnalyzerMode(q);
    createModeMainWindow();

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
    m_mainWindow->setDockActionsVisible(ModeManager::instance()->currentMode()->id() ==
                                        Constants::MODE_ANALYZE);
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
    analyzeToolBarLayout->addWidget(m_controlsWidget);
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

void AnalyzerManagerPrivate::addDock(Qt::DockWidgetArea area, QDockWidget *dockWidget)
{
    dockWidget->setParent(m_mainWindow);
    m_mainWindow->addDockWidget(area, dockWidget);

    Context globalContext(Core::Constants::C_GLOBAL);

    ActionManager *am = ICore::instance()->actionManager();
    QAction *toggleViewAction = dockWidget->toggleViewAction();
    toggleViewAction->setText(dockWidget->windowTitle());
    Command *cmd = am->registerAction(toggleViewAction,
        QString("Analyzer." + dockWidget->objectName()), globalContext);
    cmd->setAttribute(Command::CA_Hide);

    ActionContainer *viewsMenu =
        am->actionContainer(Core::Id(Core::Constants::M_WINDOW_VIEWS));
    viewsMenu->addAction(cmd);
}

bool buildTypeAccepted(IAnalyzerTool::ToolMode toolMode,
                       ProjectExplorer::BuildConfiguration::BuildType buildType)
{
    if (toolMode == IAnalyzerTool::AnyMode)
        return true;
    if (buildType == ProjectExplorer::BuildConfiguration::Unknown)
        return true;
    if (buildType == ProjectExplorer::BuildConfiguration::Debug
            && toolMode == IAnalyzerTool::DebugMode)
        return true;
    if (buildType == ProjectExplorer::BuildConfiguration::Release
            && toolMode == IAnalyzerTool::ReleaseMode)
        return true;
    return false;
}

bool AnalyzerManagerPrivate::showPromptDialog(const QString &title, const QString &text,
    const QString &stopButtonText, const QString &cancelButtonText) const
{
    Utils::CheckableMessageBox messageBox(Core::ICore::instance()->mainWindow());
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

ProjectExplorer::RunControl *AnalyzerManagerPrivate::createRunControl
    (ProjectExplorer::RunConfiguration *runConfiguration, const QString &mode)
{
    Q_UNUSED(mode);

    // FIXME: Move to tool implementations.
    AnalyzerStartParameters sp;
    if (qobject_cast<ProjectExplorer::LocalApplicationRunConfiguration *>(runConfiguration)) {
        sp = localStartParameters(runConfiguration);
    } else if (qobject_cast<RemoteLinux::RemoteLinuxRunConfiguration *>(runConfiguration)) {
        sp = remoteLinuxStartParameters(runConfiguration);
    } else {
        // Might be S60DeviceRunfiguration, or something else ...
        //sp.startMode = StartRemote;
        sp.startMode = m_currentMode;
    }

    IAnalyzerTool *tool = m_currentTool;
    AnalyzerRunControl *rc = new AnalyzerRunControl(tool, sp, runConfiguration);
    m_currentRunControl = rc;
    return rc;
}

void AnalyzerManagerPrivate::startRemoteTool(IAnalyzerTool *tool, StartMode mode)
{
    Q_UNUSED(tool);
    StartRemoteDialog dlg;
    if (dlg.exec() != QDialog::Accepted)
        return;

    AnalyzerStartParameters sp;
    sp.toolId = tool->id();
    sp.startMode = mode;
    sp.connParams = dlg.sshParams();
    sp.debuggee = dlg.executable();
    sp.debuggeeArgs = dlg.arguments();
    sp.displayName = dlg.executable();
    sp.workingDirectory = dlg.workingDirectory();

    AnalyzerRunControl *rc = new AnalyzerRunControl(tool, sp, 0);
    m_currentRunControl = rc;

    ProjectExplorer::ProjectExplorerPlugin::instance()
        ->startRunControl(rc, Constants::MODE_ANALYZE);
}

void AnalyzerManagerPrivate::startLocalTool(IAnalyzerTool *tool, StartMode mode)
{
    int index = m_tools.indexOf(tool);
    QTC_ASSERT(index >= 0, return);
    QTC_ASSERT(index < m_tools.size(), return);
    QTC_ASSERT(tool == m_currentTool, return);
    QTC_ASSERT(mode == StartLocal, /**/);

    // Make sure mode is shown.
    q->showMode();

    ProjectExplorer::ProjectExplorerPlugin *pe = ProjectExplorer::ProjectExplorerPlugin::instance();

    // ### not sure if we're supposed to check if the RunConFiguration isEnabled
    ProjectExplorer::Project *pro = pe->startupProject();
    const ProjectExplorer::RunConfiguration *runConfig = 0;
    ProjectExplorer::BuildConfiguration::BuildType buildType = ProjectExplorer::BuildConfiguration::Unknown;
    if (pro) {
        if (const ProjectExplorer::Target *target = pro->activeTarget()) {
            runConfig = target->activeRunConfiguration();
            // Build configuration is 0 for QML projects.
            if (const ProjectExplorer::BuildConfiguration *buildConfig = target->activeBuildConfiguration())
                buildType = buildConfig->buildType();
        }
    }
    if (!runConfig || !runConfig->isEnabled())
        return;

    // Check if there already is an analyzer run.
    if (m_currentRunControl) {
        // ask if user wants to restart the analyzer
        const QString msg = tr("<html><head/><body><center><i>%1</i> is still running. "
            "You have to quit the Analyzer before being able to run another instance."
            "<center/><center>Force it to quit?</center></body></html>")
                .arg(m_currentTool->displayName());
        bool stopRequested = showPromptDialog(tr("Analyzer Still Running"), msg,
                                     tr("Stop Active Run"), tr("Keep Running"));
        if (!stopRequested)
            return; // no restart, keep it running, do nothing

        // user selected to stop the active run. stop it, activate restart on stop
        m_restartOnStop = true;
        stopTool(m_currentTool, m_currentMode);
        return;
    }

    IAnalyzerTool::ToolMode toolMode = tool->toolMode();

    // Check the project for whether the build config is in the correct mode
    // if not, notify the user and urge him to use the correct mode.
    if (!buildTypeAccepted(toolMode, buildType)) {
        const QString toolName = tool->displayName();
        const QString currentMode = buildType == ProjectExplorer::BuildConfiguration::Debug ? tr("Debug") : tr("Release");

        QSettings *settings = Core::ICore::instance()->settings();
        const QString configKey = QLatin1String(Constants::MODE_ANALYZE) + QLatin1Char('/') + QLatin1String("AnalyzeCorrectMode");
        int ret;
        if (settings->contains(configKey)) {
            ret = settings->value(configKey, QDialog::Accepted).toInt();
        } else {
            QString toolModeString;
            switch (toolMode) {
                case IAnalyzerTool::DebugMode:
                    toolModeString = tr("Debug");
                case IAnalyzerTool::ReleaseMode:
                    toolModeString = tr("Release");
                case IAnalyzerTool::AnyMode:
                    break;
            }
            const QString title = tr("Run %1 in %2 Mode?").arg(toolName).arg(currentMode);
            const QString message = tr("<html><head/><body><p>You are trying to run the tool '%1' on an application in %2 mode. "
                                       "The tool is designed to be used in %3 mode.</p><p>"
                                       "Do you want to continue and run it in %2 mode?</p></body></html>").
                                       arg(toolName).arg(currentMode).arg(toolModeString);
            const QString checkBoxText = tr("&Do not ask again");
            bool checkBoxSetting = false;
            const QDialogButtonBox::StandardButton button =
                Utils::CheckableMessageBox::question(Core::ICore::instance()->mainWindow(), title, message, checkBoxText,
                                                     &checkBoxSetting, QDialogButtonBox::Yes|QDialogButtonBox::Cancel,
                                                     QDialogButtonBox::Cancel);
            ret = button == QDialogButtonBox::Yes ? QDialog::Accepted : QDialog::Rejected;

            if (checkBoxSetting && ret == QDialog::Accepted)
                settings->setValue(configKey, ret);
        }
        if (ret == QDialog::Rejected)
            return;
    }

    pe->runProject(pro, Constants::MODE_ANALYZE);

    updateRunActions();
}

void AnalyzerManagerPrivate::startTool()
{
    m_currentTool->startTool(m_currentMode);
}

void AnalyzerManagerPrivate::stopTool(IAnalyzerTool *tool, StartMode mode)
{
    QTC_ASSERT(tool == m_currentTool, /**/);
    QTC_ASSERT(mode == m_currentMode, /**/);
    if (m_currentRunControl)
        return;

    // be sure to call handleToolFinished only once, and only when the engine is really finished
    if (m_currentRunControl->stop() == ProjectExplorer::RunControl::StoppedSynchronously)
        handleToolFinished(tool, mode);
    // else: wait for the finished() signal to trigger handleToolFinished()
}

void AnalyzerManagerPrivate::modeChanged(IMode *mode)
{
    if (!m_mainWindow)
        return;
    const bool makeVisible = mode->id() == Constants::MODE_ANALYZE;
    if (!makeVisible)
        return;
    m_mainWindow->setDockActionsVisible(makeVisible);
}

QAction *AnalyzerManagerPrivate::actionFromToolAndMode(IAnalyzerTool *tool, StartMode mode)
{
    foreach (QAction *action, m_actions)
        if (m_toolFromAction[action] == tool && m_modeFromAction[action] == mode)
            return action;
    QTC_ASSERT(false, /**/);
    return 0;
}

void AnalyzerManagerPrivate::selectSavedTool()
{
    const QSettings *settings = Core::ICore::instance()->settings();
    const QByteArray lastActiveAction =
        settings->value(QLatin1String(LAST_ACTIVE_TOOL), QString()).toByteArray();
    foreach (QAction *action, m_actions) {
        IAnalyzerTool *tool = m_toolFromAction[action];
        StartMode mode = m_modeFromAction[action];
        if (tool->actionId(mode) == lastActiveAction) {
            selectTool(tool, mode);
            break;
        }
    }
}

void AnalyzerManagerPrivate::selectAction()
{
    selectAction(qobject_cast<QAction *>(sender()));
}

void AnalyzerManagerPrivate::selectAction(int index)
{
    selectAction(m_actions[index]);
}

void AnalyzerManagerPrivate::selectAction(QAction *action)
{
    selectTool(m_toolFromAction[action], m_modeFromAction[action]);
}

void AnalyzerManagerPrivate::selectTool(IAnalyzerTool *tool, StartMode mode)
{
    if (m_currentTool == tool && m_currentMode == mode)
        return;

    QAction *action = actionFromToolAndMode(tool, mode);
    const int actionIndex = m_actions.indexOf(action);
    QTC_ASSERT(actionIndex >= 0, return);

    // Guard against recursion by m_toolBox->setCurrentIndex.
    static bool inSelectTool = false;
    if (inSelectTool)
        return;
    inSelectTool = true;

    saveToolSettings(m_currentTool, m_currentMode);


    // Clean up old tool.
    if (m_currentTool) {
        ActionManager *am = ICore::instance()->actionManager();

        foreach (QDockWidget *widget, m_toolWidgets.value(m_currentTool)) {
            QAction *toggleViewAction = widget->toggleViewAction();
            am->unregisterAction(toggleViewAction,
                QString("Analyzer." + widget->objectName()));
            m_mainWindow->removeDockWidget(widget);
            ///NOTE: QMainWindow (and FancyMainWindow) just look at
            /// @c findChildren<QDockWidget*>()
            ///if we don't do this, all kind of havoc might happen, including:
            ///- improper saveState/restoreState
            ///- improper list of qdockwidgets in popup menu
            ///- ...
            widget->setParent(0);
        }
        m_currentTool->toolDeselected();
    }

    // Now change the tool.
    m_currentTool = tool;
    m_currentMode = mode;

    const bool firstTime = !m_defaultSettings.contains(tool);
    if (firstTime) {
        tool->initializeDockWidgets();
        m_defaultSettings.insert(tool, m_mainWindow->saveSettings());
    } else {
        foreach (QDockWidget *widget, m_toolWidgets.value(tool))
            addDock(Qt::DockWidgetArea(widget->property(INITIAL_DOCK_AREA).toInt()), widget);
    }

    loadToolSettings(tool);

    m_toolBox->setCurrentIndex(actionIndex);
    m_controlsWidget->setCurrentIndex(actionIndex);

    updateRunActions();
    inSelectTool = false;
}

void AnalyzerManagerPrivate::addTool(IAnalyzerTool *tool, const StartModes &modes)
{
    delayedInit(); // Make sure that there is a valid IMode instance.

    const bool blocked = m_toolBox->blockSignals(true); // Do not make current.
    m_controlsWidget->addWidget(tool->createControlWidget());
    ActionManager *am = Core::ICore::instance()->actionManager();
    foreach (StartMode mode, modes) {
        QString actionName = tool->actionName(mode);
        QString menuGroup = tool->menuGroup(mode);
        QString actionId = tool->actionId(mode);
        QAction *action = new QAction(actionName, 0);
        Core::Command *command = am->registerAction(action, actionId,
            Core::Context(Core::Constants::C_GLOBAL));
        m_menu->addAction(command, menuGroup);
        m_actions.append(action);
        m_toolFromAction[action] = tool;
        m_modeFromAction[action] = mode;
        m_toolBox->addItem(actionName);
        m_toolBox->blockSignals(blocked);
        connect(action, SIGNAL(triggered()), SLOT(selectAction()));
    }
    m_tools.append(tool);
    m_toolBox->setEnabled(true);
}

void AnalyzerManagerPrivate::handleToolFinished(IAnalyzerTool *tool, StartMode mode)
{
    QTC_ASSERT(tool == m_currentTool, /**/);
    QTC_ASSERT(mode == m_currentMode, /**/);
    m_currentRunControl = 0;
    updateRunActions();

    if (m_restartOnStop) {
        m_currentTool->startTool(m_currentMode);
        m_restartOnStop = false;
    }
}

void AnalyzerManagerPrivate::loadToolSettings(IAnalyzerTool *tool)
{
    QTC_ASSERT(m_mainWindow, return);
    QSettings *settings = Core::ICore::instance()->settings();
    settings->beginGroup(QLatin1String("AnalyzerViewSettings_") + tool->id());
    if (settings->value("ToolSettingsSaved", false).toBool())
        m_mainWindow->restoreSettings(settings);
    settings->endGroup();
}

void AnalyzerManagerPrivate::saveToolSettings(IAnalyzerTool *tool, StartMode mode)
{
    if (!tool)
        return; // no active tool, do nothing
    QTC_ASSERT(m_mainWindow, return);

    QSettings *settings = Core::ICore::instance()->settings();
    settings->beginGroup(QLatin1String("AnalyzerViewSettings_") + tool->id());
    m_mainWindow->saveSettings(settings);
    settings->setValue("ToolSettingsSaved", true);
    settings->endGroup();
    settings->setValue(QLatin1String(LAST_ACTIVE_TOOL), tool->actionId(mode));
}

void AnalyzerManagerPrivate::updateRunActions()
{
    ProjectExplorer::ProjectExplorerPlugin *pe =
        ProjectExplorer::ProjectExplorerPlugin::instance();
    ProjectExplorer::Project *project = pe->startupProject();

    bool startEnabled = !m_currentRunControl
        && pe->canRun(project, Constants::MODE_ANALYZE)
        && m_currentTool;

    QString disabledReason;
    if (m_currentRunControl)
        disabledReason = tr("An analysis is still in progress.");
    else if (!m_currentTool)
        disabledReason = tr("No analyzer tool selected.");
    else
        disabledReason = pe->cannotRunReason(project, Constants::MODE_ANALYZE);

    m_startAction->setEnabled(startEnabled);
    m_startAction->setToolTip(disabledReason);
    m_toolBox->setEnabled(!m_currentRunControl);
    m_stopAction->setEnabled(m_currentRunControl);
}

////////////////////////////////////////////////////////////////////
//
// AnalyzerManager
//
////////////////////////////////////////////////////////////////////

static AnalyzerManager *m_instance = 0;

AnalyzerManager::AnalyzerManager(QObject *parent)
  : QObject(parent),
    d(new AnalyzerManagerPrivate(this))
{
    m_instance = this;
}

AnalyzerManager::~AnalyzerManager()
{
    delete d;
}

void AnalyzerManager::extensionsInitialized()
{
    if (d->m_tools.isEmpty())
        return;

    foreach (IAnalyzerTool *tool, d->m_tools)
        tool->extensionsInitialized();
    d->selectSavedTool();
}

void AnalyzerManager::shutdown()
{
    d->saveToolSettings(d->m_currentTool, d->m_currentMode);
}

void AnalyzerManager::addTool(IAnalyzerTool *tool, const StartModes &modes)
{
    m_instance->d->addTool(tool, modes);
}

QDockWidget *AnalyzerManager::createDockWidget(IAnalyzerTool *tool, const QString &title,
                                               QWidget *widget, Qt::DockWidgetArea area)
{
    QTC_ASSERT(!widget->objectName().isEmpty(), return 0);
    AnalyzerManagerPrivate *d = m_instance->d;
    QDockWidget *dockWidget = d->m_mainWindow->addDockForWidget(widget);
    dockWidget->setProperty(INITIAL_DOCK_AREA, int(area));
    d->m_dockWidgets.append(AnalyzerManagerPrivate::DockPtr(dockWidget));
    dockWidget->setWindowTitle(title);
    d->m_toolWidgets[tool].push_back(dockWidget);
    d->addDock(area, dockWidget);
    return dockWidget;
}

void AnalyzerManager::selectTool(IAnalyzerTool *tool, StartMode mode)
{
    m_instance->d->selectTool(tool, mode);
}

void AnalyzerManager::startTool(IAnalyzerTool *tool, StartMode mode)
{
    QTC_ASSERT(tool == m_instance->d->m_currentTool, return);
    tool->startTool(mode);
}

Utils::FancyMainWindow *AnalyzerManager::mainWindow()
{
    return m_instance->d->m_mainWindow;
}

void AnalyzerManagerPrivate::resetLayout()
{
    m_mainWindow->restoreSettings(m_defaultSettings.value(m_currentTool));
}

void AnalyzerManager::showStatusMessage(const QString &message, int timeoutMS)
{
    m_instance->d->m_statusLabel->showStatusMessage(message, timeoutMS);
}

void AnalyzerManager::showPermanentStatusMessage(const QString &message)
{
    showStatusMessage(message, -1);
}

QString AnalyzerManager::msgToolStarted(const QString &name)
{
    return tr("Tool \"%1\" started...").arg(name);
}

QString AnalyzerManager::msgToolFinished(const QString &name, int issuesFound)
{
    return issuesFound ?
        tr("Tool \"%1\" finished, %n issues were found.", 0, issuesFound).arg(name) :
        tr("Tool \"%1\" finished, no issues were found.").arg(name);
}

void AnalyzerManager::showMode()
{
    if (m_instance->d->m_mode)
        ModeManager::instance()->activateMode(m_instance->d->m_mode->id());
}

void AnalyzerManager::stopTool(IAnalyzerTool *tool, StartMode mode)
{
    m_instance->d->stopTool(tool, mode);
}

void AnalyzerManager::startLocalTool(IAnalyzerTool *tool, StartMode mode)
{
    m_instance->d->startLocalTool(tool, mode);
}

void AnalyzerManager::startRemoteTool(IAnalyzerTool *tool, StartMode mode)
{
    m_instance->d->startRemoteTool(tool, mode);
}

ProjectExplorer::RunControl *AnalyzerManager::createRunControl
        (ProjectExplorer::RunConfiguration *runConfiguration, const QString &mode)
{
    return m_instance->d->createRunControl(runConfiguration, mode);
}

void AnalyzerManager::handleToolFinished(IAnalyzerTool *tool, StartMode mode)
{
    m_instance->d->handleToolFinished(tool, mode);
}

IAnalyzerTool *AnalyzerManager::toolById(const QByteArray &id)
{
    foreach (IAnalyzerTool *tool, m_instance->d->m_tools)
        if (tool->id() == id)
            return tool;
    QTC_ASSERT(false, qDebug() << "NO ANAYLYZER TOOL FOUND FOR ID" << id);
    return 0;
}

} // namespace Analyzer

#include "analyzermanager.moc"
