/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "debuggerplugin.h"

#include "debuggerconstants.h"
#include "debuggermanager.h"
#include "debuggerrunner.h"
#include "gdbengine.h"

#include "ui_gdboptionpage.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/basemode.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/findplaceholder.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/minisplitter.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/navigationwidget.h>
#include <coreplugin/outputpane.h>
#include <coreplugin/rightpane.h>
#include <coreplugin/uniqueidmanager.h>

#include <cplusplus/ExpressionUnderCursor.h>

#include <cppeditor/cppeditorconstants.h>

#include <extensionsystem/pluginmanager.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/session.h>

#include <texteditor/basetexteditor.h>
#include <texteditor/basetextmark.h>
#include <texteditor/itexteditor.h>
#include <texteditor/texteditorconstants.h>

#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QObject>
#include <QtCore/QPoint>
#include <QtCore/QSettings>
#include <QtCore/QtPlugin>

#include <QtGui/QLineEdit>
#include <QtGui/QDockWidget>
#include <QtGui/QMainWindow>
#include <QtGui/QPlainTextEdit>
#include <QtGui/QTextBlock>
#include <QtGui/QTextCursor>


using namespace Core;
using namespace Debugger::Constants;
using namespace Debugger::Internal;
using namespace ProjectExplorer;
using namespace TextEditor;


namespace Debugger {
namespace Constants {

const char * const STARTEXTERNAL        = "Debugger.StartExternal";
const char * const ATTACHEXTERNAL       = "Debugger.AttachExternal";

const char * const RUN_TO_LINE          = "Debugger.RunToLine";
const char * const RUN_TO_FUNCTION      = "Debugger.RunToFunction";
const char * const JUMP_TO_LINE         = "Debugger.JumpToLine";
const char * const TOGGLE_BREAK         = "Debugger.ToggleBreak";
const char * const BREAK_BY_FUNCTION    = "Debugger.BreakByFunction";
const char * const BREAK_AT_MAIN        = "Debugger.BreakAtMain";
const char * const ADD_TO_WATCH         = "Debugger.AddToWatch";

#ifdef Q_OS_MAC
const char * const INTERRUPT_KEY            = "Shift+F5";
const char * const RESET_KEY                = "Ctrl+Shift+F5";
const char * const STEP_KEY                 = "F7";
const char * const STEPOUT_KEY              = "Shift+F7";
const char * const NEXT_KEY                 = "F6";
const char * const STEPI_KEY                = "Shift+F9";
const char * const NEXTI_KEY                = "Shift+F6";
const char * const RUN_TO_LINE_KEY          = "Shift+F8";
const char * const RUN_TO_FUNCTION_KEY      = "Ctrl+F6";
const char * const JUMP_TO_LINE_KEY         = "Alt+D,Alt+L";
const char * const TOGGLE_BREAK_KEY         = "F8";
const char * const BREAK_BY_FUNCTION_KEY    = "Alt+D,Alt+F";
const char * const BREAK_AT_MAIN_KEY        = "Alt+D,Alt+M";
const char * const ADD_TO_WATCH_KEY         = "Alt+D,Alt+W";
#else
const char * const INTERRUPT_KEY            = "Shift+F5";
const char * const RESET_KEY                = "Ctrl+Shift+F5";
const char * const STEP_KEY                 = "F11";
const char * const STEPOUT_KEY              = "Shift+F11";
const char * const NEXT_KEY                 = "F10";
const char * const STEPI_KEY                = "";
const char * const NEXTI_KEY                = "";
const char * const RUN_TO_LINE_KEY          = "";
const char * const RUN_TO_FUNCTION_KEY      = "";
const char * const JUMP_TO_LINE_KEY         = "";
const char * const TOGGLE_BREAK_KEY         = "F9";
const char * const BREAK_BY_FUNCTION_KEY    = "";
const char * const BREAK_AT_MAIN_KEY        = "";
const char * const ADD_TO_WATCH_KEY         = "Ctrl+Alt+Q";
#endif

} // namespace Constants
} // namespace Debugger


static ProjectExplorer::SessionManager *sessionManager()
{
    return ProjectExplorer::ProjectExplorerPlugin::instance()->session();
}

///////////////////////////////////////////////////////////////////////
//
// DebugMode
//
///////////////////////////////////////////////////////////////////////

namespace Debugger {
namespace Internal {

class DebugMode : public Core::BaseMode
{
    Q_OBJECT

public:
    DebugMode(QObject *parent = 0);
    ~DebugMode();

    // IMode
    void activated() {}
    void shutdown() {}
};

DebugMode::DebugMode(QObject *parent)
  : BaseMode(parent)
{
    setName(tr("Debug"));
    setUniqueModeName(Constants::MODE_DEBUG);
    setIcon(QIcon(":/fancyactionbar/images/mode_Debug.png"));
    setPriority(Constants::P_MODE_DEBUG);
}

DebugMode::~DebugMode()
{
    // Make sure the editor manager does not get deleted
    EditorManager::instance()->setParent(0);
}

} // namespace Internal
} // namespace Debugger


///////////////////////////////////////////////////////////////////////
//
// LocationMark
//
///////////////////////////////////////////////////////////////////////

namespace Debugger {
namespace Internal {

class LocationMark : public TextEditor::BaseTextMark
{
    Q_OBJECT

public:
    LocationMark(const QString &fileName, int linenumber)
        : BaseTextMark(fileName, linenumber)
    {}
    ~LocationMark();

    QIcon icon() const;
    void updateLineNumber(int /*lineNumber*/) {}
    void updateBlock(const QTextBlock & /*block*/) {}
    void removedFromEditor() {}
};

LocationMark::~LocationMark()
{
    //qDebug() << "LOCATIONMARK DESTRUCTOR";
}

QIcon LocationMark::icon() const
{
    static const QIcon icon(":/gdbdebugger/images/location.svg");
    return icon;
}

} // namespace Internal
} // namespace Debugger


///////////////////////////////////////////////////////////////////////
//
// GdbOptionPage
//
///////////////////////////////////////////////////////////////////////

namespace Debugger {
namespace Internal {

class GdbOptionPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    GdbOptionPage(DebuggerPlugin *plugin) : m_plugin(plugin) {}

    // IOptionsPage
    QString name() const { return tr("Gdb"); }
    QString category() const { return "Debugger"; }
    QString trCategory() const { return tr("Debugger"); }

    QWidget *createPage(QWidget *parent);
    void apply();
    void finish() {} // automatically calls "apply"

private:
    friend class DebuggerPlugin;
    Ui::GdbOptionPage m_ui;

    DebuggerSettings m_settings;
    DebuggerPlugin *m_plugin;
};

QWidget *GdbOptionPage::createPage(QWidget *parent)
{
    QWidget *w = new QWidget(parent);
    m_settings = *m_plugin->m_manager->settings();
    m_ui.setupUi(w);
    m_ui.gdbLocationChooser->setExpectedKind(Core::Utils::PathChooser::Command);
    m_ui.gdbLocationChooser->setPromptDialogTitle(tr("Choose Gdb Location"));
    m_ui.gdbLocationChooser->setPath(m_settings.m_gdbCmd);
    m_ui.scriptFileChooser->setExpectedKind(Core::Utils::PathChooser::File);
    m_ui.scriptFileChooser->setPromptDialogTitle(tr("Choose Location of Startup Script File"));
    m_ui.scriptFileChooser->setPath(m_settings.m_scriptFile);
    m_ui.environmentEdit->setText(m_settings.m_gdbEnv);

    m_ui.radioButtonAllPluginBreakpoints->
        setChecked(m_settings.m_pluginAllBreakpoints);
    m_ui.radioButtonSelectedPluginBreakpoints->
        setChecked(m_settings.m_pluginSelectedBreakpoints);
    m_ui.radioButtonNoPluginBreakpoints->
        setChecked(m_settings.m_pluginNoBreakpoints);
    m_ui.lineEditSelectedPluginBreakpointsPattern->
        setText(m_settings.m_pluginSelectedBreakpointsPattern);
    m_ui.lineEditSelectedPluginBreakpointsPattern->
        setEnabled(m_settings.m_pluginSelectedBreakpoints);

    m_ui.checkBoxSkipKnownFrames->setChecked(m_settings.m_skipKnownFrames);
    m_ui.checkBoxDebugDumpers->setChecked(m_settings.m_debugDumpers);
    m_ui.checkBoxUseCustomDumpers->setChecked(m_settings.m_useCustomDumpers);
    m_ui.checkBoxUseToolTips->setChecked(m_settings.m_useToolTips);

    connect(m_ui.radioButtonSelectedPluginBreakpoints, SIGNAL(toggled(bool)),
        m_ui.lineEditSelectedPluginBreakpointsPattern, SLOT(setEnabled(bool)));

#ifndef QT_DEBUG
#if 0
    cmd = am->registerAction(m_manager->m_dumpLogAction,
        Constants::DUMP_LOG, globalcontext);
    //cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+D,Ctrl+L")));
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+F11")));
    mdebug->addAction(cmd);
#endif
#endif

    // FIXME
    m_ui.environmentEdit->hide();
    m_ui.labelEnvironment->hide();

    //m_dumpLogAction = new QAction(this);
    //m_dumpLogAction->setText(tr("Dump Log File for Debugging Purposes"));
    //
    connect(m_ui.checkBoxUseCustomDumpers, SIGNAL(clicked(bool)),
        m_plugin->m_manager, SLOT(setUseCustomDumpers(bool)));

    return w;
}

void GdbOptionPage::apply()
{
    m_settings.m_gdbCmd   = m_ui.gdbLocationChooser->path();
    m_settings.m_gdbEnv   = m_ui.environmentEdit->text();
    m_settings.m_scriptFile = m_ui.scriptFileChooser->path();

    m_settings.m_skipKnownFrames = m_ui.checkBoxSkipKnownFrames->isChecked();
    m_settings.m_debugDumpers = m_ui.checkBoxDebugDumpers->isChecked();
    m_settings.m_useCustomDumpers = m_ui.checkBoxUseCustomDumpers->isChecked();
    m_settings.m_useToolTips = m_ui.checkBoxUseToolTips->isChecked();

    m_settings.m_pluginAllBreakpoints =
        m_ui.radioButtonAllPluginBreakpoints->isChecked();
    m_settings.m_pluginSelectedBreakpoints =
        m_ui.radioButtonSelectedPluginBreakpoints->isChecked();
    m_settings.m_pluginNoBreakpoints =
        m_ui.radioButtonNoPluginBreakpoints->isChecked();
    m_settings.m_pluginSelectedBreakpointsPattern =
        m_ui.lineEditSelectedPluginBreakpointsPattern->text();

    *m_plugin->m_manager->settings() = m_settings;
    m_plugin->writeSettings();
}

} // namespace Internal
} // namespace Debugger


///////////////////////////////////////////////////////////////////////
//
// DebuggerPlugin
//
///////////////////////////////////////////////////////////////////////

DebuggerPlugin::DebuggerPlugin()
{
    m_generalOptionPage = 0;
    m_locationMark = 0;
    m_manager = 0;
    m_debugMode = 0;
}

DebuggerPlugin::~DebuggerPlugin()
{}

static QSettings *settings()
{
    return ICore::instance()->settings();
}

void DebuggerPlugin::shutdown()
{
    if (m_debugMode)
        m_debugMode->shutdown(); // saves state including manager information
    QTC_ASSERT(m_manager, /**/);
    if (m_manager)
        m_manager->shutdown();

    writeSettings();

    //qDebug() << "DebuggerPlugin::~DebuggerPlugin";
    removeObject(m_debugMode);
    removeObject(m_generalOptionPage);

    // FIXME: when using the line below, BreakWindow etc gets deleted twice.
    // so better leak for now...
    delete m_debugMode;
    m_debugMode = 0;

    delete m_generalOptionPage;
    m_generalOptionPage = 0;

    delete m_locationMark;
    m_locationMark = 0;

    delete m_manager;
    m_manager = 0;
}

bool DebuggerPlugin::initialize(const QStringList &arguments, QString *error_message)
{
    Q_UNUSED(arguments);
    Q_UNUSED(error_message);

    m_manager = new DebuggerManager;

    ICore *core = ICore::instance();
    QTC_ASSERT(core, return false);

    Core::ActionManager *am = core->actionManager();
    QTC_ASSERT(am, return false);

    Core::UniqueIDManager *uidm = core->uniqueIDManager();
    QTC_ASSERT(uidm, return false);

    QList<int> globalcontext;
    globalcontext << Core::Constants::C_GLOBAL_ID;

    QList<int> cppcontext;
    cppcontext << uidm->uniqueIdentifier(ProjectExplorer::Constants::LANG_CXX);

    QList<int> debuggercontext;
    debuggercontext << uidm->uniqueIdentifier(C_GDBDEBUGGER);

    QList<int> cppeditorcontext;
    cppeditorcontext << uidm->uniqueIdentifier(CppEditor::Constants::C_CPPEDITOR);

    QList<int> texteditorcontext;
    texteditorcontext << uidm->uniqueIdentifier(TextEditor::Constants::C_TEXTEDITOR);

    m_gdbRunningContext = uidm->uniqueIdentifier(Constants::GDBRUNNING);

    m_breakpointMarginAction = new QAction(this);
    m_breakpointMarginAction->setText("Toggle Breakpoint");
    //m_breakpointMarginAction->setIcon(QIcon(":/gdbdebugger/images/breakpoint.svg"));
    connect(m_breakpointMarginAction, SIGNAL(triggered()),
        this, SLOT(breakpointMarginActionTriggered()));

    //Core::ActionContainer *mcppcontext =
    //    am->actionContainer(CppEditor::Constants::M_CONTEXT);

    Core::ActionContainer *mdebug =
        am->actionContainer(ProjectExplorer::Constants::M_DEBUG);

    Core::Command *cmd = 0;
    cmd = am->registerAction(m_manager->m_startExternalAction,
        Constants::STARTEXTERNAL, globalcontext);
    mdebug->addAction(cmd, Core::Constants::G_DEFAULT_ONE);

#ifndef Q_OS_WIN
    cmd = am->registerAction(m_manager->m_attachExternalAction,
        Constants::ATTACHEXTERNAL, globalcontext);
    mdebug->addAction(cmd, Core::Constants::G_DEFAULT_ONE);
#endif

    cmd = am->registerAction(m_manager->m_continueAction,
        ProjectExplorer::Constants::DEBUG, QList<int>() << m_gdbRunningContext);

    cmd = am->registerAction(m_manager->m_stopAction,
        Constants::INTERRUPT, globalcontext);
    cmd->setAttribute(Core::Command::CA_UpdateText);
    cmd->setAttribute(Core::Command::CA_UpdateIcon);
    cmd->setDefaultKeySequence(QKeySequence(Constants::INTERRUPT_KEY));
    cmd->setDefaultText(tr("Stop Debugger/Interrupt Debugger"));
    mdebug->addAction(cmd, Core::Constants::G_DEFAULT_ONE);

    cmd = am->registerAction(m_manager->m_resetAction,
        Constants::RESET, globalcontext);
    cmd->setAttribute(Core::Command::CA_UpdateText);
    cmd->setDefaultKeySequence(QKeySequence(Constants::RESET_KEY));
    cmd->setDefaultText(tr("Reset Debugger"));
    //disabled mdebug->addAction(cmd, Core::Constants::G_DEFAULT_ONE);

    QAction *sep = new QAction(this);
    sep->setSeparator(true);
    cmd = am->registerAction(sep, QLatin1String("Debugger.Sep1"), globalcontext);
    mdebug->addAction(cmd);

    cmd = am->registerAction(m_manager->m_nextAction,
        Constants::NEXT, debuggercontext);
    cmd->setDefaultKeySequence(QKeySequence(Constants::NEXT_KEY));
    mdebug->addAction(cmd);

    cmd = am->registerAction(m_manager->m_stepAction,
        Constants::STEP, debuggercontext);
    cmd->setDefaultKeySequence(QKeySequence(Constants::STEP_KEY));
    mdebug->addAction(cmd);

    cmd = am->registerAction(m_manager->m_stepOutAction,
        Constants::STEPOUT, debuggercontext);
    cmd->setDefaultKeySequence(QKeySequence(Constants::STEPOUT_KEY));
    mdebug->addAction(cmd);

    cmd = am->registerAction(m_manager->m_nextIAction,
        Constants::NEXTI, debuggercontext);
    cmd->setDefaultKeySequence(QKeySequence(Constants::NEXTI_KEY));
    mdebug->addAction(cmd);

    cmd = am->registerAction(m_manager->m_stepIAction,
        Constants::STEPI, debuggercontext);
    cmd->setDefaultKeySequence(QKeySequence(Constants::STEPI_KEY));
    mdebug->addAction(cmd);

    cmd = am->registerAction(m_manager->m_runToLineAction,
        Constants::RUN_TO_LINE, debuggercontext);
    cmd->setDefaultKeySequence(QKeySequence(Constants::RUN_TO_LINE_KEY));
    mdebug->addAction(cmd);

    cmd = am->registerAction(m_manager->m_runToFunctionAction,
        Constants::RUN_TO_FUNCTION, debuggercontext);
    cmd->setDefaultKeySequence(QKeySequence(Constants::RUN_TO_FUNCTION_KEY));
    mdebug->addAction(cmd);

    cmd = am->registerAction(m_manager->m_jumpToLineAction,
        Constants::JUMP_TO_LINE, debuggercontext);
    mdebug->addAction(cmd);

    sep = new QAction(this);
    sep->setSeparator(true);
    cmd = am->registerAction(sep, QLatin1String("Debugger.Sep3"), globalcontext);
    mdebug->addAction(cmd);

    cmd = am->registerAction(m_manager->m_breakAction,
        Constants::TOGGLE_BREAK, cppeditorcontext);
    cmd->setDefaultKeySequence(QKeySequence(Constants::TOGGLE_BREAK_KEY));
    mdebug->addAction(cmd);
    //mcppcontext->addAction(cmd);

    cmd = am->registerAction(m_manager->m_breakByFunctionAction,
        Constants::BREAK_BY_FUNCTION, globalcontext);
    mdebug->addAction(cmd);

    cmd = am->registerAction(m_manager->m_breakAtMainAction,
        Constants::BREAK_AT_MAIN, globalcontext);
    mdebug->addAction(cmd);

    sep = new QAction(this);
    sep->setSeparator(true);
    cmd = am->registerAction(sep, QLatin1String("Debugger.Sep2"), globalcontext);
    mdebug->addAction(cmd);

    sep = new QAction(this);
    sep->setSeparator(true);
    cmd = am->registerAction(sep, QLatin1String("Debugger.Sep4"), globalcontext);
    mdebug->addAction(cmd);

    cmd = am->registerAction(m_manager->m_watchAction,
        Constants::ADD_TO_WATCH, cppeditorcontext);
    //cmd->setDefaultKeySequence(QKeySequence(tr("ALT+D,ALT+W")));
    mdebug->addAction(cmd);

    // Views menu
    cmd = am->registerAction(sep, QLatin1String("Debugger.Sep5"), globalcontext);
    mdebug->addAction(cmd);
    ActionContainer *viewsMenu = am->createMenu(Constants::M_DEBUG_VIEWS);
    QMenu *m = viewsMenu->menu();
    m->setEnabled(true);
    m->setTitle(tr("&Views"));
    mdebug->addMenu(viewsMenu, Core::Constants::G_DEFAULT_THREE);

    m_toggleLockedAction = new QAction(tr("Locked"), this);
    m_toggleLockedAction->setCheckable(true);
    m_toggleLockedAction->setChecked(true);
    connect(m_toggleLockedAction, SIGNAL(toggled(bool)),
        m_manager, SLOT(setLocked(bool)));
    foreach (QDockWidget *dockWidget, m_manager->dockWidgets()) {
        cmd = am->registerAction(dockWidget->toggleViewAction(),
            "Debugger." + dockWidget->objectName(), debuggercontext);
        viewsMenu->addAction(cmd);
        //m->addAction(dockWidget->toggleViewAction());
    }
    m->addSeparator();
    m->addAction(m_toggleLockedAction);
    m->addSeparator();

    QAction *resetToSimpleAction = viewsMenu->menu()->addAction(tr("Reset to default layout"));
    connect(resetToSimpleAction, SIGNAL(triggered()),
        m_manager, SLOT(setSimpleDockWidgetArrangement()));

   // FIXME:
    m_generalOptionPage = new GdbOptionPage(this);
    addObject(m_generalOptionPage);

    m_locationMark = 0;


    //
    // Debug mode setup
    //
    m_debugMode = new DebugMode(this);
    //addAutoReleasedObject(m_debugMode);

    addAutoReleasedObject(new DebuggerRunner(m_manager));

    QList<int> context;
    context.append(uidm->uniqueIdentifier(Core::Constants::C_EDITORMANAGER));
    context.append(uidm->uniqueIdentifier(Debugger::Constants::C_GDBDEBUGGER));
    context.append(uidm->uniqueIdentifier(Core::Constants::C_NAVIGATION_PANE));
    m_debugMode->setContext(context);

    QBoxLayout *editorHolderLayout = new QVBoxLayout;
    editorHolderLayout->setMargin(0);
    editorHolderLayout->setSpacing(0);
    editorHolderLayout->addWidget(new EditorManagerPlaceHolder(m_debugMode));
    editorHolderLayout->addWidget(new FindToolBarPlaceHolder(m_debugMode));

    QWidget *editorAndFindWidget = new QWidget;
    editorAndFindWidget->setLayout(editorHolderLayout);

    MiniSplitter *rightPaneSplitter = new MiniSplitter;
    rightPaneSplitter->addWidget(editorAndFindWidget);
    rightPaneSplitter->addWidget(new RightPanePlaceHolder(m_debugMode));
    rightPaneSplitter->setStretchFactor(0, 1);
    rightPaneSplitter->setStretchFactor(1, 0);

    QWidget *centralWidget = new QWidget;

    m_manager->mainWindow()->setCentralWidget(centralWidget);

    MiniSplitter *splitter = new MiniSplitter;
    splitter->addWidget(m_manager->mainWindow());
    splitter->addWidget(new OutputPanePlaceHolder(m_debugMode));
    splitter->setStretchFactor(0, 10);
    splitter->setStretchFactor(1, 0);
    splitter->setOrientation(Qt::Vertical);

    MiniSplitter *splitter2 = new MiniSplitter;
    splitter2->addWidget(new NavigationWidgetPlaceHolder(m_debugMode));
    splitter2->addWidget(splitter);
    splitter2->setStretchFactor(0, 0);
    splitter2->setStretchFactor(1, 1);

    m_debugMode->setWidget(splitter2);

    QToolBar *debugToolBar = new QToolBar;
    debugToolBar->setProperty("topBorder", true);
    debugToolBar->addAction(am->command(ProjectExplorer::Constants::DEBUG)->action());
    debugToolBar->addAction(am->command(Constants::INTERRUPT)->action());
    debugToolBar->addAction(am->command(Constants::NEXT)->action());
    debugToolBar->addAction(am->command(Constants::STEP)->action());
    debugToolBar->addAction(am->command(Constants::STEPOUT)->action());
    debugToolBar->addSeparator();
    debugToolBar->addAction(am->command(Constants::STEPI)->action());
    debugToolBar->addAction(am->command(Constants::NEXTI)->action());
    debugToolBar->addSeparator();
    debugToolBar->addWidget(new QLabel(tr("Threads:")));

    QComboBox *threadBox = new QComboBox;
    threadBox->setModel(m_manager->threadsModel());
    connect(threadBox, SIGNAL(activated(int)),
        m_manager->threadsWindow(), SIGNAL(threadSelected(int)));
    debugToolBar->addWidget(threadBox);
    debugToolBar->addWidget(m_manager->statusLabel());

    QWidget *stretch = new QWidget;
    stretch->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    debugToolBar->addWidget(stretch);

    QBoxLayout *toolBarAddingLayout = new QVBoxLayout(centralWidget);
    toolBarAddingLayout->setMargin(0);
    toolBarAddingLayout->setSpacing(0);
    toolBarAddingLayout->addWidget(rightPaneSplitter);
    toolBarAddingLayout->addWidget(debugToolBar);

    m_manager->createDockWidgets();
    m_manager->setSimpleDockWidgetArrangement();
    readSettings();

    connect(ModeManager::instance(), SIGNAL(currentModeChanged(Core::IMode*)),
            this, SLOT(focusCurrentEditor(Core::IMode*)));
    m_debugMode->widget()->setFocusProxy(EditorManager::instance());
    addObject(m_debugMode);

    //
    //  Connections
    //

    // ProjectExplorer
    connect(sessionManager(), SIGNAL(sessionLoaded()),
       m_manager, SLOT(sessionLoaded()));
    connect(sessionManager(), SIGNAL(aboutToSaveSession()),
       m_manager, SLOT(aboutToSaveSession()));

    // EditorManager
    QObject *editorManager = core->editorManager();
    connect(editorManager, SIGNAL(editorAboutToClose(Core::IEditor*)),
        this, SLOT(editorAboutToClose(Core::IEditor*)));
    connect(editorManager, SIGNAL(editorOpened(Core::IEditor*)),
        this, SLOT(editorOpened(Core::IEditor*)));

    // Application interaction
    connect(m_manager, SIGNAL(currentTextEditorRequested(QString*,int*,QObject**)),
        this, SLOT(queryCurrentTextEditor(QString*,int*,QObject**)));

    connect(m_manager, SIGNAL(setSessionValueRequested(QString,QVariant)),
        this, SLOT(setSessionValue(QString,QVariant)));
    connect(m_manager, SIGNAL(sessionValueRequested(QString,QVariant*)),
        this, SLOT(querySessionValue(QString,QVariant*)));
    connect(m_manager, SIGNAL(setConfigValueRequested(QString,QVariant)),
        this, SLOT(setConfigValue(QString,QVariant)));
    connect(m_manager, SIGNAL(configValueRequested(QString,QVariant*)),
        this, SLOT(queryConfigValue(QString,QVariant*)));

    connect(m_manager, SIGNAL(resetLocationRequested()),
        this, SLOT(resetLocation()));
    connect(m_manager, SIGNAL(gotoLocationRequested(QString,int,bool)),
        this, SLOT(gotoLocation(QString,int,bool)));
    connect(m_manager, SIGNAL(statusChanged(int)),
        this, SLOT(changeStatus(int)));
    connect(m_manager, SIGNAL(previousModeRequested()),
        this, SLOT(activatePreviousMode()));
    connect(m_manager, SIGNAL(debugModeRequested()),
        this, SLOT(activateDebugMode()));

    return true;
}

void DebuggerPlugin::extensionsInitialized()
{
}

/*! Activates the previous mode when the current mode is the debug mode. */
void DebuggerPlugin::activatePreviousMode()
{
    Core::ModeManager *const modeManager = ICore::instance()->modeManager();

    if (modeManager->currentMode() == modeManager->mode(Constants::MODE_DEBUG)
            && !m_previousMode.isEmpty()) {
        modeManager->activateMode(m_previousMode);
        m_previousMode.clear();
    }
}

void DebuggerPlugin::activateDebugMode()
{
    ModeManager *modeManager = ModeManager::instance();
    m_previousMode = QLatin1String(modeManager->currentMode()->uniqueModeName());
    modeManager->activateMode(QLatin1String(MODE_DEBUG));
}

void DebuggerPlugin::queryCurrentTextEditor(QString *fileName, int *lineNumber, QObject **object)
{
    EditorManager *editorManager = EditorManager::instance();
    if (!editorManager)
        return;
    Core::IEditor *editor = editorManager->currentEditor();
    ITextEditor *textEditor = qobject_cast<ITextEditor*>(editor);
    if (!textEditor)
        return;
    if (fileName)
        *fileName = textEditor->file()->fileName();
    if (lineNumber)
        *lineNumber = textEditor->currentLine();
    if (object)
        *object = textEditor->widget();
}

void DebuggerPlugin::editorOpened(Core::IEditor *editor)
{
    if (ITextEditor *textEditor = qobject_cast<ITextEditor *>(editor)) {
        connect(textEditor, SIGNAL(markRequested(TextEditor::ITextEditor*,int)),
            this, SLOT(requestMark(TextEditor::ITextEditor*,int)));
        connect(editor, SIGNAL(tooltipRequested(TextEditor::ITextEditor*,QPoint,int)),
            this, SLOT(showToolTip(TextEditor::ITextEditor*,QPoint,int)));
        connect(textEditor, SIGNAL(markContextMenuRequested(TextEditor::ITextEditor*,int,QMenu*)),
            this, SLOT(requestContextMenu(TextEditor::ITextEditor*,int,QMenu*)));
    }
}

void DebuggerPlugin::editorAboutToClose(Core::IEditor *editor)
{
    if (ITextEditor *textEditor = qobject_cast<ITextEditor *>(editor)) {
        disconnect(textEditor, SIGNAL(markRequested(TextEditor::ITextEditor*,int)),
            this, SLOT(requestMark(TextEditor::ITextEditor*,int)));
        disconnect(editor, SIGNAL(tooltipRequested(TextEditor::ITextEditor*,QPoint,int)),
            this, SLOT(showToolTip(TextEditor::ITextEditor*,QPoint,int)));
        disconnect(textEditor, SIGNAL(markContextMenuRequested(TextEditor::ITextEditor*,int,QMenu*)),
            this, SLOT(requestContextMenu(TextEditor::ITextEditor*,int,QMenu*)));
    }
}

void DebuggerPlugin::requestContextMenu(TextEditor::ITextEditor *editor,
    int lineNumber, QMenu *menu)
{
    m_breakpointMarginActionLineNumber = lineNumber;
    m_breakpointMarginActionFileName = editor->file()->fileName();
    menu->addAction(m_breakpointMarginAction);
}

void DebuggerPlugin::breakpointMarginActionTriggered()
{
    m_manager->toggleBreakpoint(
        m_breakpointMarginActionFileName,
        m_breakpointMarginActionLineNumber
    );
}

void DebuggerPlugin::requestMark(TextEditor::ITextEditor *editor, int lineNumber)
{
    m_manager->toggleBreakpoint(editor->file()->fileName(), lineNumber);
}

void DebuggerPlugin::showToolTip(TextEditor::ITextEditor *editor,
    const QPoint &point, int pos)
{
    if (!m_manager->settings()->m_useToolTips)
        return;

    QPlainTextEdit *plaintext = qobject_cast<QPlainTextEdit*>(editor->widget());
    if (!plaintext)
        return;

    QString expr = plaintext->textCursor().selectedText();
    if (expr.isEmpty()) {
        QTextCursor tc(plaintext->document());
        tc.setPosition(pos);

        const QChar ch = editor->characterAt(pos);
        if (ch.isLetterOrNumber() || ch == QLatin1Char('_'))
            tc.movePosition(QTextCursor::EndOfWord);

        // Fetch the expression's code.
        CPlusPlus::ExpressionUnderCursor expressionUnderCursor;
        expr = expressionUnderCursor(tc);
    }
    //qDebug() << " TOOLTIP  EXPR " << expr;
    m_manager->setToolTipExpression(point, expr);
}

void DebuggerPlugin::setSessionValue(const QString &name, const QVariant &value)
{
    //qDebug() << "SET SESSION VALUE" << name << value;
    QTC_ASSERT(sessionManager(), return);
    sessionManager()->setValue(name, value);
}

void DebuggerPlugin::querySessionValue(const QString &name, QVariant *value)
{
    QTC_ASSERT(sessionManager(), return);
    *value = sessionManager()->value(name);
    //qDebug() << "GET SESSION VALUE: " << name << value;
}


void DebuggerPlugin::setConfigValue(const QString &name, const QVariant &value)
{
    QTC_ASSERT(m_debugMode, return);
    settings()->setValue(name, value);
}

void DebuggerPlugin::queryConfigValue(const QString &name, QVariant *value)
{
    QTC_ASSERT(m_debugMode, return);
    *value = settings()->value(name);
}

void DebuggerPlugin::resetLocation()
{
    //qDebug() << "RESET_LOCATION: current:"  << currentTextEditor();
    //qDebug() << "RESET_LOCATION: locations:"  << m_locationMark;
    //qDebug() << "RESET_LOCATION: stored:"  << m_locationMark->editor();
    delete m_locationMark;
    m_locationMark = 0;
}

void DebuggerPlugin::gotoLocation(const QString &fileName, int lineNumber,
    bool setMarker)
{
    TextEditor::BaseTextEditor::openEditorAt(fileName, lineNumber);
    if (setMarker) {
        resetLocation();
        m_locationMark = new LocationMark(fileName, lineNumber);
    }
}

void DebuggerPlugin::changeStatus(int status)
{
    bool startIsContinue = (status == DebuggerInferiorStopped);
    ICore *core = ICore::instance();
    if (startIsContinue) {
        core->addAdditionalContext(m_gdbRunningContext);
        core->updateContext();
    } else {
        core->removeAdditionalContext(m_gdbRunningContext);
        core->updateContext();
    }
}

void DebuggerPlugin::writeSettings() const
{
    QTC_ASSERT(m_manager, return);
    QTC_ASSERT(m_manager->mainWindow(), return);

    QSettings *s = settings();
    DebuggerSettings *m = m_manager->settings();
    s->beginGroup(QLatin1String("DebugMode"));
    s->setValue("State", m_manager->mainWindow()->saveState());
    s->setValue("Locked", m_toggleLockedAction->isChecked());
    s->setValue("Location", m->m_gdbCmd);
    s->setValue("Environment", m->m_gdbEnv);
    s->setValue("ScriptFile", m->m_scriptFile);
    s->setValue("AutoRun", m->m_autoRun);
    s->setValue("AutoQuit", m->m_autoQuit);

    s->setValue("UseToolTips", m->m_useToolTips);
    s->setValue("UseCustomDumpers", m->m_useCustomDumpers);
    s->setValue("SkipKnowFrames", m->m_skipKnownFrames);
    s->setValue("DebugDumpers", m->m_debugDumpers);

    s->setValue("AllPluginBreakpoints", m->m_pluginAllBreakpoints);
    s->setValue("SelectedPluginBreakpoints", m->m_pluginSelectedBreakpoints);
    s->setValue("NoPluginBreakpoints", m->m_pluginNoBreakpoints);
    s->setValue("SelectedPluginBreakpointsPattern", m->m_pluginSelectedBreakpointsPattern);

    s->endGroup();
}

void DebuggerPlugin::readSettings()
{
    QSettings *s = settings();
    DebuggerSettings *m = &m_manager->m_settings; 

    QString defaultCommand("gdb");
#if defined(Q_OS_WIN32)
    defaultCommand.append(".exe");
#endif
    //QString defaultScript = ICore::instance()->resourcePath() +
    //    QLatin1String("/gdb/qt4macros");
    QString defaultScript;

    s->beginGroup(QLatin1String("DebugMode"));

    QByteArray ba = s->value("State", QByteArray()).toByteArray();
    m_toggleLockedAction->setChecked(s->value("Locked", true).toBool());
    m->m_gdbCmd     = s->value("Location", defaultCommand).toString();
    m->m_scriptFile = s->value("ScriptFile", defaultScript).toString();
    m->m_gdbEnv     = s->value("Environment", "").toString();
    m->m_autoRun    = s->value("AutoRun", true).toBool();
    m->m_autoQuit   = s->value("AutoQuit", true).toBool();

    m->m_skipKnownFrames  = s->value("SkipKnownFrames", false).toBool();
    m->m_debugDumpers     = s->value("DebugDumpers", false).toBool();
    m->m_useCustomDumpers = s->value("UseCustomDumpers", true).toBool();
    m->m_useToolTips      = s->value("UseToolTips", false).toBool();

    m->m_pluginAllBreakpoints =
        s->value("AllPluginBreakpoints", true).toBool();
    m->m_pluginSelectedBreakpoints =
        s->value("SelectedPluginBreakpoints", false).toBool();
    m->m_pluginNoBreakpoints =
        s->value("NoPluginBreakpoints", false).toBool();
    m->m_pluginSelectedBreakpointsPattern =
        s->value("SelectedPluginBreakpointsPattern").toString();

    s->endGroup();

    m_manager->mainWindow()->restoreState(ba);
}

void DebuggerPlugin::focusCurrentEditor(IMode *mode)
{
    if (mode != m_debugMode)
        return;

    EditorManager *editorManager = EditorManager::instance();

    if (editorManager->currentEditor())
        editorManager->currentEditor()->widget()->setFocus();
}

#include "debuggerplugin.moc"

Q_EXPORT_PLUGIN(DebuggerPlugin)
