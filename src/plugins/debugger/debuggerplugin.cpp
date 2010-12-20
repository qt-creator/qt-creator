/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "debuggerplugin.h"

#include "debuggeractions.h"
#include "debuggerconstants.h"
#include "debuggercore.h"
#include "debuggerdialogs.h"
#include "debuggerengine.h"
#include "debuggermainwindow.h"
#include "debuggerrunner.h"
#include "debuggerstringutils.h"
#include "debuggertooltip.h"

#include "breakpoint.h"
#include "breakhandler.h"
#include "breakwindow.h"
#include "consolewindow.h"
#include "disassembleragent.h"
#include "logwindow.h"
#include "moduleswindow.h"
#include "registerwindow.h"
#include "snapshotwindow.h"
#include "stackhandler.h"
#include "stackwindow.h"
#include "sourcefileswindow.h"
#include "threadswindow.h"
#include "watchhandler.h"
#include "watchwindow.h"
#include "watchutils.h"

#include "snapshothandler.h"
#include "threadshandler.h"
#include "gdb/gdboptionspage.h"

#include "ui_commonoptionspage.h"
#include "ui_dumperoptionpage.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/imode.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/findplaceholder.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/imode.h>
#include <coreplugin/icorelistener.h>
#include <coreplugin/manhattanstyle.h>
#include <coreplugin/minisplitter.h>
#include <coreplugin/modemanager.h>

#include <cppeditor/cppeditorconstants.h>
#include <cpptools/cppmodelmanagerinterface.h>

#include <extensionsystem/pluginmanager.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchaintype.h>

#include <qt4projectmanager/qt4projectmanagerconstants.h>

#include <texteditor/basetexteditor.h>
#include <texteditor/fontsettings.h>
#include <texteditor/texteditorsettings.h>

#include <utils/qtcassert.h>
#include <utils/savedaction.h>
#include <utils/styledbar.h>

#include <qml/scriptconsole.h>

#include <QtCore/QTimer>
#include <QtCore/QtPlugin>
#include <QtGui/QComboBox>
#include <QtGui/QDockWidget>
#include <QtGui/QFileDialog>
#include <QtGui/QMenu>
#include <QtGui/QMessageBox>
#include <QtGui/QPushButton>
#include <QtGui/QTextBlock>
#include <QtGui/QTextCursor>
#include <QtGui/QToolButton>
#include <QtGui/QTreeWidget>

#include <climits>

#define DEBUG_STATE 1
#ifdef DEBUG_STATE
//#   define STATE_DEBUG(s)
//    do { QString msg; QTextStream ts(&msg); ts << s;
//      showMessage(msg, LogDebug); } while (0)
#   define STATE_DEBUG(s) do { qDebug() << s; } while(0)
#else
#   define STATE_DEBUG(s)
#endif

// Note: the Debugger process itself and any helper processes like
// gdbserver, the trk client etc are referred to as 'Engine',
// whereas the debugged process is referred to as 'Inferior'.
//
// Transitions marked by '---' are done in the individual engines.
// Transitions marked by '+-+' are done in the base DebuggerEngine.
// Transitions marked by '*' are done asynchronously.
// The GdbEngine->setupEngine() function is described in more detail below.
//
// The engines are responsible for local roll-back to the last
// acknowledged state before calling notify*Failed. I.e. before calling
// notifyEngineSetupFailed() any process started during setupEngine()
// so far must be terminated.
//
//
//
//                        DebuggerNotReady
//                               +
//                      EngineSetupRequested
//                               +
//                  (calls *Engine->setupEngine())
//                            |      |
//                            |      |
//                       {notify-  {notify-
//                        Engine-   Engine-
//                        SetupOk}  SetupFailed}
//                            +      +
//                            +      `+-+-+> EngineSetupFailed
//                            +                   +
//                            +    [calls RunControl->startFailed]
//                            +                   +
//                            +             DebuggerFinished
//                            v
//                      EngineSetupOk
//                            +
//             [calls RunControl->StartSuccessful]
//                            +
//                  InferiorSetupRequested
//                            +
//             (calls *Engine->setupInferior())
//                         |       |
//                         |       |
//                    {notify-   {notify-
//                     Inferior- Inferior-
//                     SetupOk}  SetupFailed}
//                         +       +
//                         +       ` +-+-> InferiorSetupFailed +-+-+-+-+-+->.
//                         +                                                +
//                         +                                                +
//                  EngineRunRequested                                      +
//                         +                                                +
//                 (calls *Engine->runEngine())                             +
//               /       |            |        \                            +
//             /         |            |          \                          +
//            | (core)   | (attach)   |           |                         +
//            |          |            |           |                         +
//      {notify-    {notifyER&- {notifyER&-  {notify-                       +
//      Inferior-     Inferior-   Inferior-  EngineRun-                     +
//     Unrunnable}     StopOk}     RunOk}     Failed}                       +
//           +           +            +           +                         +
//   InferiorUnrunnable  +     InferiorRunOk      +                         +
//                       +                        +                         +
//                InferiorStopOk            EngineRunFailed                 +
//                                                +                         v
//                                                 `-+-+-+-+-+-+-+-+-+-+-+>-+
//                                                                          +
//                                                                          +
//                       #Interrupt@InferiorRunOk#                          +
//                                  +                                       +
//                          InferiorStopRequested                           +
//  #SpontaneousStop                +                                       +
//   @InferiorRunOk#         (calls *Engine->                               +
//          +               interruptInferior())                            +
//      {notify-               |          |                                 +
//     Spontaneous-       {notify-    {notify-                              +
//      Inferior-          Inferior-   Inferior-                            +
//       StopOk}           StopOk}    StopFailed}                           +
//           +              +             +                                 +
//            +            +              +                                 +
//            InferiorStopOk              +                                 +
//                  +                     +                                 +
//                  +                    +                                  +
//                  +                   +                                   +
//        #Stop@InferiorUnrunnable#    +                                    +
//          #Creator Close Event#     +                                     +
//                       +           +                                      +
//                InferiorShutdownRequested                                 +
//                            +                                             +
//           (calls *Engine->shutdownInferior())                            +
//                         |        |                                       +
//                    {notify-   {notify-                                   +
//                     Inferior- Inferior-                                  +
//                  ShutdownOk}  ShutdownFailed}                            +
//                         +        +                                       +
//                         +        +                                       +
//  #Inferior exited#      +        +                                       +
//         |               +        +                                       +
//   {notifyInferior-      +        +                                       +
//      Exited}            +        +                                       +
//           +             +        +                                       +
//            +            +        +                                       +
//             +           +        +                                       +
//            InferiorShutdownOk InferiorShutdownFailed                     +
//                      *          *                                        +
//                  EngineShutdownRequested                                 +
//                            +                                             +
//           (calls *Engine->shutdownEngine())  <+-+-+-+-+-+-+-+-+-+-+-+-+-+'
//                         |        |
//                         |        |
//                    {notify-   {notify-
//                     Engine-    Engine-
//                  ShutdownOk}  ShutdownFailed}
//                         +       +
//            EngineShutdownOk  EngineShutdownFailed
//                         *       *
//                     DebuggerFinished
//


/* Here is a matching graph as a GraphViz graph. View it using
 * \code
grep "^sg1:" debuggerplugin.cpp | cut -c5- | dot -osg1.ps -Tps && gv sg1.ps

sg1: digraph DebuggerStates {
sg1:   DebuggerNotReady -> EngineSetupRequested
sg1:   EngineSetupRequested -> EngineSetupOk [ label="notifyEngineSetupOk", style="dashed" ];
sg1:   EngineSetupRequested -> EngineSetupFailed [ label= "notifyEngineSetupFailed", style="dashed"];
sg1:   EngineSetupFailed -> DebuggerFinished [ label= "RunControl::StartFailed" ];
sg1:   EngineSetupOk -> InferiorSetupRequested [ label= "RunControl::StartSuccessful" ];
sg1:   InferiorSetupRequested -> InferiorSetupFailed [ label="notifyInferiorFailed", style="dashed" ];
sg1:   InferiorSetupRequested -> EngineRunRequested [ label="notifyInferiorSetupOk", style="dashed" ];
sg1:   InferiorSetupFailed -> EngineShutdownRequested
sg1:   EngineRunRequested -> InferiorUnrunnable [ label="notifyInferiorUnrunnable", style="dashed" ];
sg1:   EngineRunRequested -> InferiorStopOk [ label="notifyEngineRunAndInferiorStopOk", style="dashed" ];
sg1:   EngineRunRequested -> InferiorRunOk [ label="notifyEngineRunAndInferiorRunOk", style="dashed" ];
sg1:   EngineRunRequested -> EngineRunFailed [ label="notifyEngineRunFailed", style="dashed" ];
sg1:   EngineRunFailed -> EngineShutdownRequested
sg1:   InferiorRunOk -> InferiorStopOk [ label="SpontaneousStop\nnotifyInferiorSpontaneousStop", style="dashed" ];
sg1:   InferiorRunOk -> InferiorStopRequested [ label="User stop\nEngine::interruptInferior", style="dashed"];
sg1:   InferiorStopRequested -> InferiorStopOk [ label="notifyInferiorStopOk", style="dashed" ];
sg1:   InferiorStopRequested -> InferiorShutdownRequested  [ label="notifyInferiorStopFailed", style="dashed" ];
sg1:   InferiorStopOk -> InferiorRunRequested [ label="User\nEngine::continueInferior" ];
sg1:   InferiorRunRequested -> InferiorRunOk [ label="notifyInferiorRunOk", style="dashed"];
sg1:   InferiorRunRequested -> InferiorRunFailed [ label="notifyInferiorRunFailed", style="dashed"];
sg1:   InferiorRunFailed -> InferiorStopOk
sg1:   InferiorShutdownRequested -> InferiorShutdownOk [ label= "Engine::shutdownInferior\nnotifyInferiorShutdownOk", style="dashed" ];
sg1:   InferiorShutdownRequested -> InferiorShutdownFailed [ label="Engine::shutdownInferior\nnotifyInferiorShutdownFailed", style="dashed" ];
sg1:   InferiorExited -> InferiorShutdownOk [ label="notifyInferiorExited", style="dashed"];
sg1:   InferiorShutdownOk -> EngineShutdownRequested
sg1:   InferiorShutdownFailed -> EngineShutdownRequested
sg1:   EngineShutdownRequested -> EngineShutdownOk [ label="Engine::shutdownEngine\nnotifyEngineShutdownOk", style="dashed" ];
sg1:   EngineShutdownRequested -> EngineShutdownFailed  [ label="Engine::shutdownEngine\nnotifyEngineShutdownFailed", style="dashed" ];
sg1:   EngineShutdownOk -> DebuggerFinished  [ style = "dotted" ];
sg1:   EngineShutdownFailed  -> DebuggerFinished [ style = "dotted" ];
sg1: }
* \endcode */
// Additional signalling:    {notifyInferiorIll}   {notifyEngineIll}


// GdbEngine specific startup. All happens in EngineSetupRequested state
//
// Transitions marked by '---' are done in the individual adapters.
// Transitions marked by '+-+' are done in the GdbEngine.
//
//                  GdbEngine::setupEngine()
//                          +
//            (calls *Adapter->startAdapter())
//                          |      |
//                          |      `---> handleAdapterStartFailed()
//                          |                   +
//                          |             {notifyEngineSetupFailed}
//                          |
//                 handleAdapterStarted()
//                          +
//                 {notifyEngineSetupOk}
//
//
//
//                GdbEngine::setupInferior()
//                          +
//            (calls *Adapter->prepareInferior())
//                          |      |
//                          |      `---> handlePrepareInferiorFailed()
//                          |                   +
//                          |             {notifyInferiorSetupFailed}
//                          |
//                handleInferiorPrepared()
//                          +
//                {notifyInferiorSetupOk}





using namespace Core;
using namespace Debugger::Constants;
using namespace ProjectExplorer;
using namespace TextEditor;

namespace CC = Core::Constants;
namespace PE = ProjectExplorer::Constants;


namespace Debugger {
namespace Constants {

const char * const M_DEBUG_START_DEBUGGING = "QtCreator.Menu.Debug.StartDebugging";

const char * const STARTEXTERNAL        = "Debugger.StartExternal";
const char * const ATTACHEXTERNAL       = "Debugger.AttachExternal";
const char * const ATTACHCORE           = "Debugger.AttachCore";
const char * const ATTACHTCF            = "Debugger.AttachTcf";
const char * const ATTACHREMOTE         = "Debugger.AttachRemote";
const char * const ATTACHREMOTECDB      = "Debugger.AttachRemoteCDB";
const char * const STARTREMOTELLDB      = "Debugger.StartRemoteLLDB";
const char * const DETACH               = "Debugger.Detach";

const char * const RUN_TO_LINE1         = "Debugger.RunToLine1";
const char * const RUN_TO_LINE2         = "Debugger.RunToLine2";
const char * const RUN_TO_FUNCTION      = "Debugger.RunToFunction";
const char * const JUMP_TO_LINE1        = "Debugger.JumpToLine1";
const char * const JUMP_TO_LINE2        = "Debugger.JumpToLine2";
const char * const RETURN_FROM_FUNCTION = "Debugger.ReturnFromFunction";
const char * const SNAPSHOT             = "Debugger.Snapshot";
const char * const TOGGLE_BREAK         = "Debugger.ToggleBreak";
const char * const BREAK_BY_FUNCTION    = "Debugger.BreakByFunction";
const char * const BREAK_AT_MAIN        = "Debugger.BreakAtMain";
const char * const ADD_TO_WATCH1        = "Debugger.AddToWatch1";
const char * const ADD_TO_WATCH2        = "Debugger.AddToWatch2";
const char * const OPERATE_BY_INSTRUCTION  = "Debugger.OperateByInstruction";
const char * const FRAME_UP             = "Debugger.FrameUp";
const char * const FRAME_DOWN           = "Debugger.FrameDown";

#ifdef Q_WS_MAC
const char * const STOP_KEY                 = "Shift+Ctrl+Y";
const char * const RESET_KEY                = "Ctrl+Shift+F5";
const char * const STEP_KEY                 = "Ctrl+Shift+I";
const char * const STEPOUT_KEY              = "Ctrl+Shift+T";
const char * const NEXT_KEY                 = "Ctrl+Shift+O";
const char * const REVERSE_KEY              = "";
const char * const RUN_TO_LINE_KEY          = "Shift+F8";
const char * const RUN_TO_FUNCTION_KEY      = "Ctrl+F6";
const char * const JUMP_TO_LINE_KEY         = "Ctrl+D,Ctrl+L";
const char * const TOGGLE_BREAK_KEY         = "F8";
const char * const BREAK_BY_FUNCTION_KEY    = "Ctrl+D,Ctrl+F";
const char * const BREAK_AT_MAIN_KEY        = "Ctrl+D,Ctrl+M";
const char * const ADD_TO_WATCH_KEY         = "Ctrl+D,Ctrl+W";
const char * const SNAPSHOT_KEY             = "Ctrl+D,Ctrl+S";
#else
const char * const STOP_KEY                 = "Shift+F5";
const char * const RESET_KEY                = "Ctrl+Shift+F5";
const char * const STEP_KEY                 = "F11";
const char * const STEPOUT_KEY              = "Shift+F11";
const char * const NEXT_KEY                 = "F10";
const char * const REVERSE_KEY              = "F12";
const char * const RUN_TO_LINE_KEY          = "";
const char * const RUN_TO_FUNCTION_KEY      = "";
const char * const JUMP_TO_LINE_KEY         = "";
const char * const TOGGLE_BREAK_KEY         = "F9";
const char * const BREAK_BY_FUNCTION_KEY    = "";
const char * const BREAK_AT_MAIN_KEY        = "";
const char * const ADD_TO_WATCH_KEY         = "Ctrl+Alt+Q";
const char * const SNAPSHOT_KEY             = "Ctrl+D,Ctrl+S";
#endif

} // namespace Constants


namespace Cdb {
void addCdb2OptionPages(QList<Core::IOptionsPage*> *);
} // namespace Cdb


namespace Internal {

// FIXME: Outdated?
// The createCdbEngine function takes a list of options pages it can add to.
// This allows for having a "enabled" toggle on the page independently
// of the engine. That's good for not enabling the related ActiveX control
// unnecessarily.

void addGdbOptionPages(QList<IOptionsPage*> *opts);
void addScriptOptionPages(QList<IOptionsPage*> *opts);
void addTcfOptionPages(QList<IOptionsPage*> *opts);
#ifdef CDB_ENABLED
void addCdbOptionPages(QList<IOptionsPage*> *opts);
#endif

#ifdef WITH_LLDB
void addLldbOptionPages(QList<IOptionsPage*> *opts);
#endif

static SessionManager *sessionManager()
{
    return ProjectExplorerPlugin::instance()->session();
}

static QToolButton *toolButton(QAction *action)
{
    QToolButton *button = new QToolButton;
    button->setDefaultAction(action);
    return button;
}

// Retrieve file name and line and optionally address
// from the data set on the text editor context menu action.
static bool positionFromActionData(const QObject *sender,
    QString *fileName, int *lineNumber, quint64 *address)
{
    const QAction *action = qobject_cast<const QAction *>(sender);
    QTC_ASSERT(action, return false);
    const QVariantList data = action->data().toList();
    QTC_ASSERT(data.size() == 3, return false);
    *fileName = data.front().toString();
    *lineNumber = data.at(1).toInt();
    *address = data.at(2).toULongLong();
    return true;
}

struct AttachRemoteParameters
{
    AttachRemoteParameters() : attachPid(0), winCrashEvent(0) {}
    void clear();

    quint64 attachPid;
    QString attachTarget;  // core file name or  server:port
    // Event handle for attaching to crashed Windows processes.
    quint64 winCrashEvent;
};

void AttachRemoteParameters::clear()
{
    attachPid = winCrashEvent = 0;
    attachTarget.clear();
}


///////////////////////////////////////////////////////////////////////
//
// DummyEngine
//
///////////////////////////////////////////////////////////////////////

class DummyEngine : public DebuggerEngine
{
    Q_OBJECT

public:
    DummyEngine() : DebuggerEngine(DebuggerStartParameters()) {}
    ~DummyEngine() {}

    void setupEngine() {}
    void setupInferior() {}
    void runEngine() {}
    void shutdownEngine() {}
    void shutdownInferior() {}
    void executeDebuggerCommand(const QString &) {}
    unsigned debuggerCapabilities() const { return 0; }
    bool acceptsBreakpoint(BreakpointId) const { return false; }
};

static DebuggerEngine *dummyEngine()
{
    static DummyEngine dummy;
    return &dummy;
}


///////////////////////////////////////////////////////////////////////
//
// DebugMode
//
///////////////////////////////////////////////////////////////////////

class DebugMode : public IMode
{
public:
    DebugMode() : m_widget(0) { setObjectName(QLatin1String("DebugMode")); }

    ~DebugMode()
    {
        // Make sure the editor manager does not get deleted.
        //EditorManager::instance()->setParent(0);
        delete m_widget;
    }

    // IMode
    QString displayName() const { return DebuggerPlugin::tr("Debug"); }
    QIcon icon() const { return QIcon(__(":/fancyactionbar/images/mode_Debug.png")); }
    int priority() const { return P_MODE_DEBUG; }
    QWidget *widget();
    QString id() const { return MODE_DEBUG; }
    QString type() const { return CC::MODE_EDIT_TYPE; }
    Context context() const
        { return Context(CC::C_EDITORMANAGER, C_DEBUGMODE, CC::C_NAVIGATION_PANE); }
    QString contextHelpId() const { return QString(); }
private:
    QWidget *m_widget;
};


///////////////////////////////////////////////////////////////////////
//
// CommonOptionsPage
//
///////////////////////////////////////////////////////////////////////

class CommonOptionsPage : public Core::IOptionsPage
{
public:
    CommonOptionsPage() {}

    // IOptionsPage
    QString id() const
        { return _(DEBUGGER_COMMON_SETTINGS_ID); }
    QString displayName() const
        { return QCoreApplication::translate("Debugger", DEBUGGER_COMMON_SETTINGS_NAME); }
    QString category() const
        { return _(DEBUGGER_SETTINGS_CATEGORY);  }
    QString displayCategory() const
        { return QCoreApplication::translate("Debugger", DEBUGGER_SETTINGS_TR_CATEGORY); }
    QIcon categoryIcon() const
        { return QIcon(QLatin1String(DEBUGGER_COMMON_SETTINGS_CATEGORY_ICON)); }

    QWidget *createPage(QWidget *parent);
    void apply() { m_group.apply(ICore::instance()->settings()); }
    void finish() { m_group.finish(); }
    virtual bool matches(const QString &s) const;

private:
    Ui::CommonOptionsPage m_ui;
    Utils::SavedActionSet m_group;
    QString m_searchKeywords;
};

QWidget *CommonOptionsPage::createPage(QWidget *parent)
{
    QWidget *w = new QWidget(parent);
    m_ui.setupUi(w);
    m_group.clear();

    m_group.insert(debuggerCore()->action(ListSourceFiles),
        m_ui.checkBoxListSourceFiles);
    m_group.insert(debuggerCore()->action(UseAlternatingRowColors),
        m_ui.checkBoxUseAlternatingRowColors);
    m_group.insert(debuggerCore()->action(UseToolTipsInMainEditor),
        m_ui.checkBoxUseToolTipsInMainEditor);
    m_group.insert(debuggerCore()->action(CloseBuffersOnExit),
        m_ui.checkBoxCloseBuffersOnExit);
    m_group.insert(debuggerCore()->action(SwitchModeOnExit),
        m_ui.checkBoxSwitchModeOnExit);
    m_group.insert(debuggerCore()->action(AutoDerefPointers), 0);
    m_group.insert(debuggerCore()->action(UseToolTipsInLocalsView), 0);
    m_group.insert(debuggerCore()->action(UseToolTipsInBreakpointsView), 0);
    m_group.insert(debuggerCore()->action(UseAddressInBreakpointsView), 0);
    m_group.insert(debuggerCore()->action(UseAddressInStackView), 0);
    m_group.insert(debuggerCore()->action(MaximalStackDepth),
        m_ui.spinBoxMaximalStackDepth);
    m_group.insert(debuggerCore()->action(ShowStdNamespace), 0);
    m_group.insert(debuggerCore()->action(ShowQtNamespace), 0);
    m_group.insert(debuggerCore()->action(SortStructMembers), 0);
    m_group.insert(debuggerCore()->action(LogTimeStamps), 0);
    m_group.insert(debuggerCore()->action(VerboseLog), 0);
    m_group.insert(debuggerCore()->action(BreakOnThrow), 0);
    m_group.insert(debuggerCore()->action(BreakOnCatch), 0);
#ifdef Q_OS_WIN
    Utils::SavedAction *registerAction = debuggerCore()->action(RegisterForPostMortem);
    m_group.insert(registerAction,
        m_ui.checkBoxRegisterForPostMortem);
    connect(registerAction, SIGNAL(toggled(bool)),
            m_ui.checkBoxRegisterForPostMortem, SLOT(setChecked(bool)));
#endif

    if (m_searchKeywords.isEmpty()) {
        QLatin1Char sep(' ');
        QTextStream(&m_searchKeywords)
                << sep << m_ui.checkBoxUseAlternatingRowColors->text()
                << sep << m_ui.checkBoxUseToolTipsInMainEditor->text()
                << sep << m_ui.checkBoxListSourceFiles->text()
#ifdef Q_OS_WIN
                << sep << m_ui.checkBoxRegisterForPostMortem->text()
#endif
                << sep << m_ui.checkBoxCloseBuffersOnExit->text()
                << sep << m_ui.checkBoxSwitchModeOnExit->text()
                << sep << m_ui.labelMaximalStackDepth->text()
                   ;
        m_searchKeywords.remove(QLatin1Char('&'));
    }
#ifndef Q_OS_WIN
    m_ui.checkBoxRegisterForPostMortem->setVisible(false);
#endif
    return w;
}

bool CommonOptionsPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}

///////////////////////////////////////////////////////////////////////
//
// DebuggingHelperOptionPage
//
///////////////////////////////////////////////////////////////////////

static bool oxygenStyle()
{
    const ManhattanStyle *ms = qobject_cast<const ManhattanStyle *>(qApp->style());
    return ms && !qstrcmp("OxygenStyle", ms->baseStyle()->metaObject()->className());
}

class DebuggingHelperOptionPage : public Core::IOptionsPage
{
    Q_OBJECT // Needs tr-context.
public:
    DebuggingHelperOptionPage() {}

    // IOptionsPage
    QString id() const { return _("Z.DebuggingHelper"); }
    QString displayName() const { return tr("Debugging Helper"); }
    QString category() const { return _(DEBUGGER_SETTINGS_CATEGORY); }
    QString displayCategory() const
    { return QCoreApplication::translate("Debugger", DEBUGGER_SETTINGS_TR_CATEGORY); }
    QIcon categoryIcon() const
    { return QIcon(QLatin1String(DEBUGGER_COMMON_SETTINGS_CATEGORY_ICON)); }

    QWidget *createPage(QWidget *parent);
    void apply() { m_group.apply(ICore::instance()->settings()); }
    void finish() { m_group.finish(); }
    virtual bool matches(const QString &s) const;

private:
    Ui::DebuggingHelperOptionPage m_ui;
    Utils::SavedActionSet m_group;
    QString m_searchKeywords;
};

QWidget *DebuggingHelperOptionPage::createPage(QWidget *parent)
{
    QWidget *w = new QWidget(parent);
    m_ui.setupUi(w);

    m_ui.dumperLocationChooser->setExpectedKind(Utils::PathChooser::Command);
    m_ui.dumperLocationChooser->setPromptDialogTitle(tr("Choose DebuggingHelper Location"));
    m_ui.dumperLocationChooser->setInitialBrowsePathBackup(
        ICore::instance()->resourcePath() + "../../lib");

    m_group.clear();
    m_group.insert(debuggerCore()->action(UseDebuggingHelpers),
        m_ui.debuggingHelperGroupBox);
    m_group.insert(debuggerCore()->action(UseCustomDebuggingHelperLocation),
        m_ui.customLocationGroupBox);
    // Suppress Oxygen style's giving flat group boxes bold titles.
    if (oxygenStyle())
        m_ui.customLocationGroupBox->setStyleSheet(_("QGroupBox::title { font: ; }"));

    m_group.insert(debuggerCore()->action(CustomDebuggingHelperLocation),
        m_ui.dumperLocationChooser);

    m_group.insert(debuggerCore()->action(UseCodeModel),
        m_ui.checkBoxUseCodeModel);
    m_group.insert(debuggerCore()->action(ShowThreadNames),
        m_ui.checkBoxShowThreadNames);


#ifndef QT_DEBUG
#if 0
    cmd = am->registerAction(m_dumpLogAction,
        DUMP_LOG, globalcontext);
    //cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+D,Ctrl+L")));
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+F11")));
    mdebug->addAction(cmd);
#endif
#endif

    if (m_searchKeywords.isEmpty()) {
        QTextStream(&m_searchKeywords)
                << ' ' << m_ui.debuggingHelperGroupBox->title()
                << ' ' << m_ui.customLocationGroupBox->title()
                << ' ' << m_ui.dumperLocationLabel->text()
                << ' ' << m_ui.checkBoxUseCodeModel->text()
                << ' ' << m_ui.checkBoxShowThreadNames->text();
        m_searchKeywords.remove(QLatin1Char('&'));
    }
    return w;
}

bool DebuggingHelperOptionPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}


///////////////////////////////////////////////////////////////////////
//
// Argument parsing
//
///////////////////////////////////////////////////////////////////////

static QString msgParameterMissing(const QString &a)
{
    return DebuggerPlugin::tr("Option '%1' is missing the parameter.").arg(a);
}

static QString msgInvalidNumericParameter(const QString &a, const QString &number)
{
    return DebuggerPlugin::tr("The parameter '%1' of option '%2' is not a number.").arg(number, a);
}

static bool parseArgument(QStringList::const_iterator &it,
    const QStringList::const_iterator &cend,
    AttachRemoteParameters *attachRemoteParameters,
    unsigned *enabledEngines, QString *errorMessage)
{
    const QString &option = *it;
    // '-debug <pid>'
    // '-debug <corefile>'
    // '-debug <server:port>'
    if (*it == _("-debug")) {
        ++it;
        if (it == cend) {
            *errorMessage = msgParameterMissing(*it);
            return false;
        }
        bool ok;
        attachRemoteParameters->attachPid = it->toULongLong(&ok);
        if (!ok)
            attachRemoteParameters->attachTarget = *it;
        return true;
    }
    // -wincrashevent <event-handle>. A handle used for
    // a handshake when attaching to a crashed Windows process.
    if (*it == _("-wincrashevent")) {
        ++it;
        if (it == cend) {
            *errorMessage = msgParameterMissing(*it);
            return false;
        }
        bool ok;
        attachRemoteParameters->winCrashEvent = it->toULongLong(&ok);
        if (!ok) {
            *errorMessage = msgInvalidNumericParameter(option, *it);
            return false;
        }
        return true;
    }
    // Engine disabling.
    if (option == _("-disable-cdb")) {
        *enabledEngines &= ~CdbEngineType;
        return true;
    }
    if (option == _("-disable-gdb")) {
        *enabledEngines &= ~GdbEngineType;
        return true;
    }
    if (option == _("-disable-qmldb")) {
        *enabledEngines &= ~QmlEngineType;
        return true;
    }
    if (option == _("-disable-sdb")) {
        *enabledEngines &= ~ScriptEngineType;
        return true;
    }
    if (option == _("-disable-tcf")) {
        *enabledEngines &= ~TcfEngineType;
        return true;
    }
    if (option == _("-disable-lldb")) {
        *enabledEngines &= ~LldbEngineType;
        return true;
    }

    *errorMessage = DebuggerPlugin::tr("Invalid debugger option: %1").arg(option);
    return false;
}

static bool parseArguments(const QStringList &args,
   AttachRemoteParameters *attachRemoteParameters,
   unsigned *enabledEngines, QString *errorMessage)
{
    attachRemoteParameters->clear();
    const QStringList::const_iterator cend = args.constEnd();
    for (QStringList::const_iterator it = args.constBegin(); it != cend; ++it)
        if (!parseArgument(it, cend, attachRemoteParameters, enabledEngines, errorMessage))
            return false;
    if (Constants::Internal::debug)
        qDebug().nospace() << args << "engines=0x"
            << QString::number(*enabledEngines, 16)
            << " pid" << attachRemoteParameters->attachPid
            << " target" << attachRemoteParameters->attachTarget << '\n';
    return true;
}


///////////////////////////////////////////////////////////////////////
//
// Misc
//
///////////////////////////////////////////////////////////////////////

static bool isDebuggable(IEditor *editor)
{
    // Only blacklist Qml. Whitelisting would fail on C++ code in files
    // with strange names, more harm would be done this way.
    //   IFile *file = editor->file();
    //   return !(file && file->mimeType() == "application/x-qml");
    // Nowadays, even Qml is debuggable.
    return editor;
}


///////////////////////////////////////////////////////////////////////
//
// Debugger Actions
//
///////////////////////////////////////////////////////////////////////

struct DebuggerActions
{
    QAction *continueAction;
    QAction *exitAction; // on the application output button if "Stop" is possible
    QAction *interruptAction; // on the fat debug button if "Pause" is possible
    QAction *undisturbableAction; // on the fat debug button if nothing can be done
    QAction *resetAction; // FIXME: Should not be needed in a stable release
    QAction *stepAction;
    QAction *stepOutAction;
    QAction *runToLineAction; // Debug menu
    QAction *runToFunctionAction;
    QAction *jumpToLineAction; // in the Debug menu
    QAction *returnFromFunctionAction;
    QAction *nextAction;
    //QAction *snapshotAction;
    QAction *watchAction1; // in the Debug menu
    QAction *watchAction2; // in the text editor context menu
    QAction *breakAction;
    QAction *sepAction;
    QAction *reverseDirectionAction;
    QAction *frameUpAction;
    QAction *frameDownAction;
};

static DebuggerPluginPrivate *theDebuggerCore = 0;


///////////////////////////////////////////////////////////////////////
//
// DebuggerPluginPrivate
//
///////////////////////////////////////////////////////////////////////

class DebuggerPluginPrivate : public DebuggerCore
{
    Q_OBJECT

public:
    explicit DebuggerPluginPrivate(DebuggerPlugin *plugin);
    ~DebuggerPluginPrivate();

    bool initialize(const QStringList &arguments, QString *errorMessage);
    void extensionsInitialized();
    void aboutToShutdown();

    void connectEngine(DebuggerEngine *engine);
    void disconnectEngine() { connectEngine(0); }
    DebuggerEngine *currentEngine() const { return m_currentEngine; }

public slots:
    void selectThread(int index)
    {
        currentEngine()->selectThread(index);
    }

    void breakpointSetMarginActionTriggered()
    {
        QString fileName;
        int lineNumber;
        quint64 address;
        if (positionFromActionData(sender(), &fileName, &lineNumber, &address)) {
            if (address)
                toggleBreakpointByAddress(address);
            else
                toggleBreakpointByFileAndLine(fileName, lineNumber);
        }
    }

    void breakpointRemoveMarginActionTriggered()
    {
        const QAction *act = qobject_cast<QAction *>(sender());
        QTC_ASSERT(act, return);
        m_breakHandler->removeBreakpoint(act->data().toInt());
     }

    void breakpointEnableMarginActionTriggered()
    {
        const QAction *act = qobject_cast<QAction *>(sender());
        QTC_ASSERT(act, return);
        breakHandler()->setEnabled(act->data().toInt(), true);
    }

    void breakpointDisableMarginActionTriggered()
    {
        const QAction *act = qobject_cast<QAction *>(sender());
        QTC_ASSERT(act, return);
        breakHandler()->setEnabled(act->data().toInt(), false);
    }

    void updateWatchersHeader(int section, int, int newSize)
    {
        m_watchersWindow->header()->resizeSection(section, newSize);
    }

    void sourceFilesDockToggled(bool on)
    {
        if (on && m_currentEngine->state() == InferiorStopOk)
            m_currentEngine->reloadSourceFiles();
    }

    void modulesDockToggled(bool on)
    {
        if (on && m_currentEngine->state() == InferiorStopOk)
            m_currentEngine->reloadModules();
    }

    void registerDockToggled(bool on)
    {
        if (on && m_currentEngine->state() == InferiorStopOk)
            m_currentEngine->reloadRegisters();
    }

    void synchronizeBreakpoints()
    {
        showMessage("ATTEMPT SYNC", LogDebug);
        for (int i = 0, n = m_snapshotHandler->size(); i != n; ++i) {
            if (DebuggerEngine *engine = m_snapshotHandler->at(i))
                engine->attemptBreakpointSynchronization();
        }
    }

    void synchronizeWatchers()
    {
        for (int i = 0, n = m_snapshotHandler->size(); i != n; ++i) {
            if (DebuggerEngine *engine = m_snapshotHandler->at(i))
                engine->watchHandler()->updateWatchers();
        }
    }

    void editorOpened(Core::IEditor *editor);
    void editorAboutToClose(Core::IEditor *editor);
    void setBusyCursor(bool busy);
    void requestMark(TextEditor::ITextEditor *editor, int lineNumber);
    void showToolTip(TextEditor::ITextEditor *editor, const QPoint &pnt, int pos);
    void requestContextMenu(TextEditor::ITextEditor *editor,
        int lineNumber, QMenu *menu);

    void activatePreviousMode();
    void activateDebugMode();
    void toggleBreakpoint();
    void toggleBreakpointByFileAndLine(const QString &fileName, int lineNumber);
    void toggleBreakpointByAddress(quint64 address);
    void onModeChanged(Core::IMode *mode);
    void showSettingsDialog();

    void debugProject();
    void startExternalApplication();
    void startRemoteCdbSession();
    void startRemoteApplication();
    void startRemoteEngine();
    void attachExternalApplication();
    void attachExternalApplication
        (qint64 pid, const QString &binary, const QString &crashParameter);
    bool attachCmdLine();
    void attachCore();
    void attachCore(const QString &core, const QString &exeFileName);
    void attachRemote(const QString &spec);
    void attachRemoteTcf();

    void enableReverseDebuggingTriggered(const QVariant &value);
    void languagesChanged();
    void showStatusMessage(const QString &msg, int timeout = -1);
    void openMemoryEditor();

    const CPlusPlus::Snapshot &cppCodeModelSnapshot() const;

    void showQtDumperLibraryWarning(const QString &details);
    DebuggerMainWindow *mainWindow() const { return m_mainWindow; }
    bool isDockVisible(const QString &objectName) const
        { return mainWindow()->isDockVisible(objectName); }

    bool hasSnapshots() const { return m_snapshotHandler->size(); }
    void createNewDock(QWidget *widget);

    void runControlStarted(DebuggerEngine *engine);
    void runControlFinished(DebuggerEngine *engine);
    DebuggerLanguages activeLanguages() const;
    QString gdbBinaryForToolChain(int toolChain) const;
    void remoteCommand(const QStringList &options, const QStringList &);

    bool isReverseDebugging() const;

    BreakHandler *breakHandler() const { return m_breakHandler; }
    SnapshotHandler *snapshotHandler() const { return m_snapshotHandler; }

    void setConfigValue(const QString &name, const QVariant &value);
    QVariant configValue(const QString &name) const;

    DebuggerRunControl *createDebugger(const DebuggerStartParameters &sp,
        RunConfiguration *rc = 0);
    void startDebugger(RunControl *runControl);
    void displayDebugger(DebuggerEngine *engine, bool updateEngine = true);

    void dumpLog();
    void cleanupViews();
    void setInitialState();

    void fontSettingsChanged(const TextEditor::FontSettings &settings);
    DebuggerState state() const { return m_state; }

    void updateState(DebuggerEngine *engine);
    void updateWatchersWindow();
    void onCurrentProjectChanged(ProjectExplorer::Project *project);

    void clearStatusMessage();

    void sessionLoaded();
    void aboutToUnloadSession();
    void aboutToSaveSession();

    void executeDebuggerCommand();
    void scriptExpressionEntered(const QString &expression);
    void coreShutdown();

public slots:
    void updateDebugActions();

    void handleExecDetach()
    {
        currentEngine()->resetLocation();
        currentEngine()->detachDebugger();
    }

    void handleExecContinue()
    {
        currentEngine()->resetLocation();
        currentEngine()->continueInferior();
    }

    void handleExecInterrupt()
    {
        currentEngine()->resetLocation();
        currentEngine()->requestInterruptInferior();
    }

    void handleExecReset()
    {
        currentEngine()->resetLocation();
        currentEngine()->notifyEngineIll(); // FIXME: Check.
    }

    void handleExecStep()
    {
        currentEngine()->resetLocation();
        if (boolSetting(OperateByInstruction))
            currentEngine()->executeStepI();
        else
            currentEngine()->executeStep();
    }

    void handleExecNext()
    {
        currentEngine()->resetLocation();
        if (boolSetting(OperateByInstruction))
            currentEngine()->executeNextI();
        else
            currentEngine()->executeNext();
    }

    void handleExecStepOut()
    {
        currentEngine()->resetLocation();
        currentEngine()->executeStepOut();
    }

    void handleExecReturn()
    {
        currentEngine()->resetLocation();
        currentEngine()->executeReturn();
    }

    void handleExecJumpToLine()
    {
        //removeTooltip();
        currentEngine()->resetLocation();
        QString fileName;
        int lineNumber;
        if (currentTextEditorPosition(&fileName, &lineNumber))
            currentEngine()->executeJumpToLine(fileName, lineNumber);
    }

    void handleExecRunToLine()
    {
        //removeTooltip();
        currentEngine()->resetLocation();
        QString fileName;
        int lineNumber;
        if (currentTextEditorPosition(&fileName, &lineNumber))
            currentEngine()->executeRunToLine(fileName, lineNumber);
    }

    void handleExecRunToFunction()
    {
        currentEngine()->resetLocation();
        ITextEditor *textEditor = currentTextEditor();
        QTC_ASSERT(textEditor, return);
        QPlainTextEdit *ed = qobject_cast<QPlainTextEdit*>(textEditor->widget());
        if (!ed)
            return;
        QTextCursor cursor = ed->textCursor();
        QString functionName = cursor.selectedText();
        if (functionName.isEmpty()) {
            const QTextBlock block = cursor.block();
            const QString line = block.text();
            foreach (const QString &str, line.trimmed().split('(')) {
                QString a;
                for (int i = str.size(); --i >= 0; ) {
                    if (!str.at(i).isLetterOrNumber())
                        break;
                    a = str.at(i) + a;
                }
                if (!a.isEmpty()) {
                    functionName = a;
                    break;
                }
            }
        }

        if (!functionName.isEmpty())
            currentEngine()->executeRunToFunction(functionName);
    }

    void slotEditBreakpoint()
    {
        const QAction *act = qobject_cast<QAction *>(sender());
        QTC_ASSERT(act, return);
        const BreakpointId id = act->data().toInt();
        QTC_ASSERT(id > 0, return);
        BreakWindow::editBreakpoint(id, mainWindow());
    }

    void slotRunToLine()
    {
        // Run to line, file name and line number set as list.
        QString fileName;
        int lineNumber;
        quint64 address;
        if (positionFromActionData(sender(), &fileName, &lineNumber, &address))
            currentEngine()->executeRunToLine(fileName, lineNumber);
    }

    void slotJumpToLine()
    {
        QString fileName;
        int lineNumber;
        quint64 address;
        if (positionFromActionData(sender(), &fileName, &lineNumber, &address))
            currentEngine()->executeJumpToLine(fileName, lineNumber);
    }

    void handleAddToWatchWindow()
    {
        // Requires a selection, but that's the only case we want anyway.
        EditorManager *editorManager = EditorManager::instance();
        if (!editorManager)
            return;
        IEditor *editor = editorManager->currentEditor();
        if (!editor)
            return;
        ITextEditor *textEditor = qobject_cast<ITextEditor*>(editor);
        if (!textEditor)
            return;
        QTextCursor tc;
        QPlainTextEdit *ptEdit = qobject_cast<QPlainTextEdit*>(editor->widget());
        if (ptEdit)
            tc = ptEdit->textCursor();
        QString exp;
        if (tc.hasSelection()) {
            exp = tc.selectedText();
        } else {
            int line, column;
            exp = cppExpressionAt(textEditor, tc.position(), &line, &column);
        }
        if (exp.isEmpty())
            return;
        currentEngine()->watchHandler()->watchExpression(exp);
    }

    void handleExecExit()
    {
        currentEngine()->exitDebugger();
    }

    void handleFrameDown()
    {
        currentEngine()->frameDown();
    }

    void handleFrameUp()
    {
        currentEngine()->frameUp();
    }

    void handleOperateByInstructionTriggered(bool operateByInstructionTriggered)
    {
        // Go to source only if we have the file.
        if (currentEngine()->stackHandler()->currentIndex() >= 0) {
            const StackFrame frame = currentEngine()->stackHandler()->currentFrame();
            if (operateByInstructionTriggered || frame.isUsable()) {
                currentEngine()->gotoLocation(Location(frame, true));
            }
        }
    }

    bool isActiveDebugLanguage(int lang) const
    {
        return m_mainWindow->activeDebugLanguages() & lang;
    }

    QVariant sessionValue(const QString &name);
    void setSessionValue(const QString &name, const QVariant &value);
    QIcon locationMarkIcon() const { return m_locationMarkIcon; }

    void openTextEditor(const QString &titlePattern0, const QString &contents);
    void clearCppCodeModelSnapshot();
    void showMessage(const QString &msg, int channel, int timeout = -1);

    Utils::SavedAction *action(int code) const;
    bool boolSetting(int code) const;
    QString stringSetting(int code) const;

    void showModuleSymbols(const QString &moduleName, const Symbols &symbols);

public:
    DebuggerState m_state;
    DebuggerMainWindow *m_mainWindow;
    DebuggerRunControlFactory *m_debuggerRunControlFactory;

    QString m_previousMode;
    Context m_continuableContext;
    Context m_interruptibleContext;
    Context m_undisturbableContext;
    Context m_finishedContext;
    Context m_anyContext;
    AttachRemoteParameters m_attachRemoteParameters;

    QAction *m_debugAction;
    QAction *m_startExternalAction;
    QAction *m_startRemoteAction;
    QAction *m_startRemoteCdbAction;
    QAction *m_startRemoteLldbAction;
    QAction *m_attachExternalAction;
    QAction *m_attachCoreAction;
    QAction *m_attachTcfAction;
    QAction *m_detachAction;
    QToolButton *m_reverseToolButton;

    QIcon m_startIcon;
    QIcon m_exitIcon;
    QIcon m_continueIcon;
    QIcon m_interruptIcon;
    QIcon m_locationMarkIcon;

    QLabel *m_statusLabel;
    QComboBox *m_threadBox;

    DebuggerActions m_actions;

    BreakWindow *m_breakWindow;
    BreakHandler *m_breakHandler;
    //ConsoleWindow *m_consoleWindow;
    QTreeView *m_returnWindow;
    QTreeView *m_localsWindow;
    QTreeView *m_watchersWindow;
    QAbstractItemView *m_registerWindow;
    QAbstractItemView *m_modulesWindow;
    QAbstractItemView *m_snapshotWindow;
    SourceFilesWindow *m_sourceFilesWindow;
    QAbstractItemView *m_stackWindow;
    QAbstractItemView *m_threadsWindow;
    LogWindow *m_logWindow;
    ScriptConsole *m_scriptConsoleWindow;

    bool m_busy;
    QTimer m_statusTimer;
    QString m_lastPermanentStatusMessage;

    mutable CPlusPlus::Snapshot m_codeModelSnapshot;
    DebuggerPlugin *m_plugin;

    SnapshotHandler *m_snapshotHandler;
    bool m_shuttingDown;
    DebuggerEngine *m_currentEngine;
    DebuggerSettings *m_debuggerSettings;
    QSettings *m_coreSettings;
    bool m_gdbBinariesChanged;
    uint m_cmdLineEnabledEngines;
};

DebuggerPluginPrivate::DebuggerPluginPrivate(DebuggerPlugin *plugin)
{
    QTC_ASSERT(!theDebuggerCore, /**/);
    theDebuggerCore = this;

    m_plugin = plugin;

    m_startRemoteCdbAction = 0;
    m_shuttingDown = false;
    m_statusLabel = 0;
    m_threadBox = 0;

    m_breakWindow = 0;
    m_breakHandler = 0;
    m_returnWindow = 0;
    m_localsWindow = 0;
    m_watchersWindow = 0;
    m_registerWindow = 0;
    m_modulesWindow = 0;
    m_snapshotWindow = 0;
    m_sourceFilesWindow = 0;
    m_stackWindow = 0;
    m_threadsWindow = 0;
    m_logWindow = 0;
    m_scriptConsoleWindow = 0;

    m_continuableContext = Context(0);
    m_interruptibleContext = Context(0);
    m_undisturbableContext = Context(0);
    m_finishedContext = Context(0);
    m_anyContext = Context(0);

    m_mainWindow = 0;
    m_state = DebuggerNotReady;
    m_snapshotHandler = 0;
    m_currentEngine = 0;
    m_debuggerSettings = 0;

    m_gdbBinariesChanged = true;
    m_cmdLineEnabledEngines = AllEngineTypes;

    m_reverseToolButton = 0;
    m_debugAction = 0;
    m_startExternalAction = 0;
    m_startRemoteAction = 0;
    m_startRemoteCdbAction = 0;
    m_startRemoteLldbAction = 0;
    m_attachExternalAction = 0;
    m_attachCoreAction = 0;
    m_attachTcfAction = 0;
    m_detachAction = 0;
}

DebuggerPluginPrivate::~DebuggerPluginPrivate()
{
    delete m_debuggerSettings;
    m_debuggerSettings = 0;

    delete m_mainWindow;
    m_mainWindow = 0;

    delete m_snapshotHandler;
    m_snapshotHandler = 0;
}

DebuggerCore *debuggerCore()
{
    return theDebuggerCore;
}

bool DebuggerPluginPrivate::initialize(const QStringList &arguments,
    QString *errorMessage)
{
    // Do not fail to load the whole plugin if something goes wrong here.
    if (!parseArguments(arguments, &m_attachRemoteParameters,
            &m_cmdLineEnabledEngines, errorMessage)) {
        *errorMessage = tr("Error evaluating command line arguments: %1")
            .arg(*errorMessage);
        qWarning("%s\n", qPrintable(*errorMessage));
        errorMessage->clear();
    }

    // Cpp/Qml ui setup
    m_mainWindow = new DebuggerMainWindow;

    return true;
}

void DebuggerPluginPrivate::setConfigValue(const QString &name, const QVariant &value)
{
    m_coreSettings->setValue(_("DebugMode/") + name, value);
}

QVariant DebuggerPluginPrivate::configValue(const QString &name) const
{
    const QVariant value = m_coreSettings->value(_("DebugMode/") + name);
    if (value.isValid())
        return value;
    // Legacy (pre-2.1): Check old un-namespaced-settings.
    return m_coreSettings->value(name);
}

void DebuggerPluginPrivate::onCurrentProjectChanged(Project *project)
{
    RunConfiguration *activeRc = 0;
    if (project) {
        Target *target = project->activeTarget();
        QTC_ASSERT(target, return);
        activeRc = target->activeRunConfiguration();
        QTC_ASSERT(activeRc, /**/);
    }
    for (int i = 0, n = m_snapshotHandler->size(); i != n; ++i) {
        // Run controls might be deleted during exit.
        if (DebuggerEngine *engine = m_snapshotHandler->at(i)) {
            DebuggerRunControl *runControl = engine->runControl();
            RunConfiguration *rc = runControl->runConfiguration();
            if (rc == activeRc) {
                m_snapshotHandler->setCurrentIndex(i);
                updateState(engine);
                return;
            }
        }
    }
    // No corresponding debugger found. So we are ready to start one.
    ICore *core = ICore::instance();
    core->updateAdditionalContexts(m_anyContext, Context());
}

void DebuggerPluginPrivate::languagesChanged()
{
    const bool debuggerIsCPP =
        m_mainWindow->activeDebugLanguages() & CppLanguage;
    //qDebug() << "DEBUGGER IS CPP: " << debuggerIsCPP;
    m_startExternalAction->setVisible(debuggerIsCPP);
    m_attachExternalAction->setVisible(debuggerIsCPP);
    m_attachCoreAction->setVisible(debuggerIsCPP);
    m_startRemoteAction->setVisible(debuggerIsCPP);
    m_detachAction->setVisible(debuggerIsCPP);
}

void DebuggerPluginPrivate::debugProject()
{
    ProjectExplorerPlugin *pe = ProjectExplorerPlugin::instance();
    if (Project *pro = pe->startupProject())
        pe->runProject(pro, Constants::DEBUGMODE);
}

void DebuggerPluginPrivate::startExternalApplication()
{
    DebuggerStartParameters sp;
    StartExternalDialog dlg(mainWindow());
    dlg.setExecutableFile(
            configValue(_("LastExternalExecutableFile")).toString());
    dlg.setExecutableArguments(
            configValue(_("LastExternalExecutableArguments")).toString());
    dlg.setWorkingDirectory(
            configValue(_("LastExternalWorkingDirectory")).toString());
    if (dlg.exec() != QDialog::Accepted)
        return;

    setConfigValue(_("LastExternalExecutableFile"),
                   dlg.executableFile());
    setConfigValue(_("LastExternalExecutableArguments"),
                   dlg.executableArguments());
    setConfigValue(_("LastExternalWorkingDirectory"),
                   dlg.workingDirectory());
    sp.executable = dlg.executableFile();
    sp.startMode = StartExternal;
    sp.workingDirectory = dlg.workingDirectory();
    if (!dlg.executableArguments().isEmpty())
        sp.processArgs = dlg.executableArguments();
    // Fixme: 1 of 3 testing hacks.
    if (sp.processArgs.startsWith(__("@tcf@ ")) || sp.processArgs.startsWith(__("@sym@ ")))
        sp.toolChainType = ToolChain_RVCT2_ARMV5;

    if (dlg.breakAtMain()) {
#ifdef Q_OS_WIN
        // FIXME: wrong on non-Qt based binaries
        breakHandler()->breakByFunction("qMain");
#else
        breakHandler()->breakByFunction("main");
#endif
    }

    if (RunControl *rc = m_debuggerRunControlFactory->create(sp))
        startDebugger(rc);
}

void DebuggerPluginPrivate::attachExternalApplication()
{
    AttachExternalDialog dlg(mainWindow());
    if (dlg.exec() == QDialog::Accepted)
        attachExternalApplication(dlg.attachPID(), dlg.executable(), QString());
}

void DebuggerPluginPrivate::attachExternalApplication
    (qint64 pid, const QString &binary, const QString &crashParameter)
{
    if (pid == 0) {
        QMessageBox::warning(mainWindow(), tr("Warning"),
            tr("Cannot attach to PID 0"));
        return;
    }
    DebuggerStartParameters sp;
    sp.attachPID = pid;
    sp.displayName = tr("Process %1").arg(pid);
    sp.executable = binary;
    sp.crashParameter = crashParameter;
    sp.startMode = crashParameter.isEmpty() ? AttachExternal : AttachCrashedExternal;
    if (DebuggerRunControl *rc = createDebugger(sp))
        startDebugger(rc);
}

void DebuggerPluginPrivate::attachCore()
{
    AttachCoreDialog dlg(mainWindow());
    dlg.setExecutableFile(configValue(_("LastExternalExecutableFile")).toString());
    dlg.setCoreFile(configValue(_("LastExternalCoreFile")).toString());
    if (dlg.exec() != QDialog::Accepted)
        return;
    setConfigValue(_("LastExternalExecutableFile"), dlg.executableFile());
    setConfigValue(_("LastExternalCoreFile"), dlg.coreFile());
    attachCore(dlg.coreFile(), dlg.executableFile());
}

void DebuggerPluginPrivate::attachCore(const QString &core, const QString &exe)
{
    DebuggerStartParameters sp;
    sp.executable = exe;
    sp.coreFile = core;
    sp.displayName = tr("Core file \"%1\"").arg(core);
    sp.startMode = AttachCore;
    if (DebuggerRunControl *rc = createDebugger(sp))
        startDebugger(rc);
}

void DebuggerPluginPrivate::attachRemote(const QString &spec)
{
    // spec is:  executable@server:port@architecture
    DebuggerStartParameters sp;
    sp.executable = spec.section('@', 0, 0);
    sp.remoteChannel = spec.section('@', 1, 1);
    sp.remoteArchitecture = spec.section('@', 2, 2);
    sp.displayName = tr("Remote: \"%1\"").arg(sp.remoteChannel);
    sp.startMode = AttachToRemote;
    if (DebuggerRunControl *rc = createDebugger(sp))
        startDebugger(rc);
}

void DebuggerPluginPrivate::startRemoteCdbSession()
{
    const QString connectionKey = _("CdbRemoteConnection");
    DebuggerStartParameters sp;
    sp.toolChainType = ToolChain_MSVC;
    sp.startMode = AttachToRemote;
    StartRemoteCdbDialog dlg(mainWindow());
    QString previousConnection = configValue(connectionKey).toString();
    if (previousConnection.isEmpty())
        previousConnection = QLatin1String("localhost:1234");
    dlg.setConnection(previousConnection);
    if (dlg.exec() != QDialog::Accepted)
        return;
    sp.remoteChannel = dlg.connection();
    setConfigValue(connectionKey, sp.remoteChannel);
    if (RunControl *rc = createDebugger(sp))
        startDebugger(rc);
}

void DebuggerPluginPrivate::startRemoteApplication()
{
    DebuggerStartParameters sp;
    StartRemoteDialog dlg(mainWindow());
    QStringList arches;
    arches.append(_("i386:x86-64:intel"));
    arches.append(_("i386"));
    arches.append(_("arm"));
    QString lastUsed = configValue(_("LastRemoteArchitecture")).toString();
    if (!arches.contains(lastUsed))
        arches.prepend(lastUsed);
    dlg.setRemoteArchitectures(arches);
    QStringList gnuTargets;
    gnuTargets.append(_("auto"));
    gnuTargets.append(_("i686-linux-gnu"));
    gnuTargets.append(_("x86_64-linux-gnu"));
    gnuTargets.append(_("arm-none-linux-gnueabi"));
    const QString lastUsedGnuTarget
        = configValue(_("LastGnuTarget")).toString();
    if (!gnuTargets.contains(lastUsedGnuTarget))
        gnuTargets.prepend(lastUsedGnuTarget);
    dlg.setGnuTargets(gnuTargets);
    dlg.setRemoteChannel(
            configValue(_("LastRemoteChannel")).toString());
    dlg.setLocalExecutable(
            configValue(_("LastLocalExecutable")).toString());
    dlg.setDebugger(configValue(_("LastDebugger")).toString());
    dlg.setRemoteArchitecture(lastUsed);
    dlg.setGnuTarget(lastUsedGnuTarget);
    dlg.setServerStartScript(
            configValue(_("LastServerStartScript")).toString());
    dlg.setUseServerStartScript(
            configValue(_("LastUseServerStartScript")).toBool());
    dlg.setSysRoot(configValue(_("LastSysroot")).toString());
    if (dlg.exec() != QDialog::Accepted)
        return;
    setConfigValue(_("LastRemoteChannel"), dlg.remoteChannel());
    setConfigValue(_("LastLocalExecutable"), dlg.localExecutable());
    setConfigValue(_("LastDebugger"), dlg.debugger());
    setConfigValue(_("LastRemoteArchitecture"), dlg.remoteArchitecture());
    setConfigValue(_("LastGnuTarget"), dlg.gnuTarget());
    setConfigValue(_("LastServerStartScript"), dlg.serverStartScript());
    setConfigValue(_("LastUseServerStartScript"), dlg.useServerStartScript());
    setConfigValue(_("LastSysroot"), dlg.sysRoot());
    sp.remoteChannel = dlg.remoteChannel();
    sp.remoteArchitecture = dlg.remoteArchitecture();
    sp.gnuTarget = dlg.gnuTarget();
    sp.executable = dlg.localExecutable();
    sp.displayName = dlg.localExecutable();
    sp.debuggerCommand = dlg.debugger(); // Override toolchain-detection.
    if (!sp.debuggerCommand.isEmpty())
        sp.toolChainType = ToolChain_INVALID;
    sp.startMode = AttachToRemote;
    sp.useServerStartScript = dlg.useServerStartScript();
    sp.serverStartScript = dlg.serverStartScript();
    sp.sysRoot = dlg.sysRoot();
    if (RunControl *rc = createDebugger(sp))
        startDebugger(rc);
}

void DebuggerPluginPrivate::startRemoteEngine()
{
    DebuggerStartParameters sp;
    StartRemoteEngineDialog dlg(mainWindow());
    if (dlg.exec() != QDialog::Accepted)
        return;

    sp.connParams.host = dlg.host();
    sp.connParams.uname = dlg.username();
    sp.connParams.pwd = dlg.password();

    sp.connParams.timeout = 5;
    sp.connParams.authType = SshConnectionParameters::AuthByPwd;
    sp.connParams.port = 22;
    sp.connParams.proxyType = SshConnectionParameters::NoProxy;

    sp.executable = dlg.inferiorPath();
    sp.serverStartScript = dlg.enginePath();
    sp.startMode = StartRemoteEngine;
    if (RunControl *rc = createDebugger(sp))
        startDebugger(rc);
}

void DebuggerPluginPrivate::enableReverseDebuggingTriggered(const QVariant &value)
{
    QTC_ASSERT(m_reverseToolButton, return);
    m_reverseToolButton->setVisible(value.toBool());
    m_actions.reverseDirectionAction->setChecked(false);
    m_actions.reverseDirectionAction->setEnabled(value.toBool());
}

void DebuggerPluginPrivate::attachRemoteTcf()
{
    DebuggerStartParameters sp;
    AttachTcfDialog dlg(mainWindow());
    QStringList arches;
    arches.append(_("i386:x86-64:intel"));
    dlg.setRemoteArchitectures(arches);
    dlg.setRemoteChannel(
            configValue(_("LastTcfRemoteChannel")).toString());
    dlg.setRemoteArchitecture(
            configValue(_("LastTcfRemoteArchitecture")).toString());
    dlg.setServerStartScript(
            configValue(_("LastTcfServerStartScript")).toString());
    dlg.setUseServerStartScript(
            configValue(_("LastTcfUseServerStartScript")).toBool());
    if (dlg.exec() != QDialog::Accepted)
        return;
    setConfigValue(_("LastTcfRemoteChannel"), dlg.remoteChannel());
    setConfigValue(_("LastTcfRemoteArchitecture"), dlg.remoteArchitecture());
    setConfigValue(_("LastTcfServerStartScript"), dlg.serverStartScript());
    setConfigValue(_("LastTcfUseServerStartScript"), dlg.useServerStartScript());
    sp.remoteChannel = dlg.remoteChannel();
    sp.remoteArchitecture = dlg.remoteArchitecture();
    sp.serverStartScript = dlg.serverStartScript();
    sp.startMode = AttachTcf;
    if (dlg.useServerStartScript())
        sp.serverStartScript = dlg.serverStartScript();
    if (RunControl *rc = createDebugger(sp))
        startDebugger(rc);
}

bool DebuggerPluginPrivate::attachCmdLine()
{
    if (m_attachRemoteParameters.attachPid) {
        showStatusMessage(tr("Attaching to PID %1.")
            .arg(m_attachRemoteParameters.attachPid));
        const QString crashParameter = m_attachRemoteParameters.winCrashEvent
            ? QString::number(m_attachRemoteParameters.winCrashEvent) : QString();
        attachExternalApplication(m_attachRemoteParameters.attachPid,
            QString(), crashParameter);
        return true;
    }
    const QString target = m_attachRemoteParameters.attachTarget;
    if (target.isEmpty())
        return false;
    if (target.indexOf(':') > 0) {
        showStatusMessage(tr("Attaching to remote server %1.").arg(target));
        attachRemote(target);
    } else {
        showStatusMessage(tr("Attaching to core %1.").arg(target));
        attachCore(target, QString());
    }
    return true;
}

void DebuggerPluginPrivate::editorOpened(Core::IEditor *editor)
{
    if (!isDebuggable(editor))
        return;
    ITextEditor *textEditor = qobject_cast<ITextEditor *>(editor);
    if (!textEditor)
        return;
    connect(textEditor,
        SIGNAL(markRequested(TextEditor::ITextEditor*,int)),
        SLOT(requestMark(TextEditor::ITextEditor*,int)));
    connect(editor,
        SIGNAL(tooltipRequested(TextEditor::ITextEditor*,QPoint,int)),
        SLOT(showToolTip(TextEditor::ITextEditor*,QPoint,int)));
    connect(textEditor,
        SIGNAL(markContextMenuRequested(TextEditor::ITextEditor*,int,QMenu*)),
        SLOT(requestContextMenu(TextEditor::ITextEditor*,int,QMenu*)));
}

void DebuggerPluginPrivate::editorAboutToClose(Core::IEditor *editor)
{
    if (!isDebuggable(editor))
        return;
    ITextEditor *textEditor = qobject_cast<ITextEditor *>(editor);
    if (!textEditor)
        return;
    disconnect(textEditor,
        SIGNAL(markRequested(TextEditor::ITextEditor*,int)),
        this, SLOT(requestMark(TextEditor::ITextEditor*,int)));
    disconnect(editor,
        SIGNAL(tooltipRequested(TextEditor::ITextEditor*,QPoint,int)),
        this, SLOT(showToolTip(TextEditor::ITextEditor*,QPoint,int)));
    disconnect(textEditor,
        SIGNAL(markContextMenuRequested(TextEditor::ITextEditor*,int,QMenu*)),
        this, SLOT(requestContextMenu(TextEditor::ITextEditor*,int,QMenu*)));
}

void DebuggerPluginPrivate::requestContextMenu(TextEditor::ITextEditor *editor,
    int lineNumber, QMenu *menu)
{
    if (!isDebuggable(editor))
        return;

    BreakpointId id = BreakpointId();
    QString fileName;
    quint64 address = 0;

    if (editor->property("DisassemblerView").toBool()) {
        fileName = editor->file()->fileName();
        QString line = editor->contents()
            .section('\n', lineNumber - 1, lineNumber - 1);
        BreakpointResponse needle;
        needle.type = BreakpointByAddress;
        needle.address = DisassemblerAgent::addressFromDisassemblyLine(line);
        address = needle.address;
        needle.lineNumber = -1;
        id = breakHandler()->findSimilarBreakpoint(needle);
    } else {
        fileName = editor->file()->fileName();
        id = breakHandler()->findBreakpointByFileAndLine(fileName, lineNumber);
    }

    QList<QVariant> args;
    args.append(fileName);
    args.append(lineNumber);
    args.append(address);

    if (id) {
        // Remove existing breakpoint.
        QAction *act = new QAction(menu);
        act->setData(int(id));
        act->setText(tr("Remove Breakpoint %1").arg(id));
        connect(act, SIGNAL(triggered()),
            SLOT(breakpointRemoveMarginActionTriggered()));
        menu->addAction(act);

        // Enable/disable existing breakpoint.
        act = new QAction(menu);
        act->setData(int(id));
        if (breakHandler()->isEnabled(id)) {
            act->setText(tr("Disable Breakpoint %1").arg(id));
            connect(act, SIGNAL(triggered()),
                SLOT(breakpointDisableMarginActionTriggered()));
        } else {
            act->setText(tr("Enable Breakpoint %1").arg(id));
            connect(act, SIGNAL(triggered()),
                SLOT(breakpointEnableMarginActionTriggered()));
        }
        menu->addAction(act);

        // Edit existing breakpoint.
        act = new QAction(menu);
        act->setText(tr("Edit Breakpoint %1...").arg(id));
        connect(act, SIGNAL(triggered()), SLOT(slotEditBreakpoint()));
        act->setData(int(id));
        menu->addAction(act);
    } else {
        // Handle non-existing breakpoint.
        const QString text = address ?
                    tr("Set Breakpoint at 0x%1").arg(address, 0, 16) :
                    tr("Set Breakpoint at line %1").arg(lineNumber);
        QAction *act = new QAction(text, menu);
        act->setData(args);
        connect(act, SIGNAL(triggered()),
            SLOT(breakpointSetMarginActionTriggered()));
        menu->addAction(act);
    }
    // Run to, jump to line below in stopped state.
    if (state() == InferiorStopOk) {
        menu->addSeparator();
        const QString runText =
            DebuggerEngine::tr("Run to Line %1").arg(lineNumber);
        QAction *runToLineAction  = new QAction(runText, menu);
        runToLineAction->setData(args);
        connect(runToLineAction, SIGNAL(triggered()), SLOT(slotRunToLine()));
        menu->addAction(runToLineAction);
        if (currentEngine()->debuggerCapabilities() & JumpToLineCapability) {
            const QString jumpText =
                DebuggerEngine::tr("Jump to Line %1").arg(lineNumber);
            QAction *jumpToLineAction  = new QAction(jumpText, menu);
            menu->addAction(runToLineAction);
            jumpToLineAction->setData(args);
            connect(jumpToLineAction, SIGNAL(triggered()), SLOT(slotJumpToLine()));
            menu->addAction(jumpToLineAction);
        }
    }
}

void DebuggerPluginPrivate::toggleBreakpoint()
{
    ITextEditor *textEditor = currentTextEditor();
    QTC_ASSERT(textEditor, return);
    const int lineNumber = textEditor->currentLine();
    if (textEditor->property("DisassemblerView").toBool()) {
        QString line = textEditor->contents()
            .section('\n', lineNumber - 1, lineNumber - 1);
        quint64 address = DisassemblerAgent::addressFromDisassemblyLine(line);
        toggleBreakpointByAddress(address);
    } else if (lineNumber >= 0) {
        toggleBreakpointByFileAndLine(textEditor->file()->fileName(), lineNumber);
    }
}

void DebuggerPluginPrivate::toggleBreakpointByFileAndLine(const QString &fileName,
    int lineNumber)
{
    BreakHandler *handler = m_breakHandler;
    BreakpointId id =
        handler->findBreakpointByFileAndLine(fileName, lineNumber, true);
    if (!id)
        id = handler->findBreakpointByFileAndLine(fileName, lineNumber, false);

    if (id) {
        handler->removeBreakpoint(id);
    } else {
        BreakpointParameters data(BreakpointByFileAndLine);
        data.fileName = fileName;
        data.lineNumber = lineNumber;
        handler->appendBreakpoint(data);
    }
    synchronizeBreakpoints();
}

void DebuggerPluginPrivate::toggleBreakpointByAddress(quint64 address)
{
    BreakHandler *handler = m_breakHandler;
    BreakpointId id = handler->findBreakpointByAddress(address);

    if (id) {
        handler->removeBreakpoint(id);
    } else {
        BreakpointParameters data(BreakpointByAddress);
        data.address = address;
        handler->appendBreakpoint(data);
    }
    synchronizeBreakpoints();
}

void DebuggerPluginPrivate::requestMark(ITextEditor *editor, int lineNumber)
{
    if (editor->property("DisassemblerView").toBool()) {
        QString line = editor->contents()
            .section('\n', lineNumber - 1, lineNumber - 1);
        quint64 address = DisassemblerAgent::addressFromDisassemblyLine(line);
        toggleBreakpointByAddress(address);
    } else if (editor->file()) {
        toggleBreakpointByFileAndLine(editor->file()->fileName(), lineNumber);
    }
}

void DebuggerPluginPrivate::showToolTip(ITextEditor *editor,
    const QPoint &point, int pos)
{
    if (!isDebuggable(editor))
        return;
    if (!boolSetting(UseToolTipsInMainEditor))
        return;
    if (state() != InferiorStopOk)
        return;
    currentEngine()->setToolTipExpression(point, editor, pos);
}

DebuggerRunControl *DebuggerPluginPrivate::createDebugger
    (const DebuggerStartParameters &sp, RunConfiguration *rc)
{
    return m_debuggerRunControlFactory->create(sp, rc);
}

// If updateEngine is set, the engine will update its threads/modules and so forth.
void DebuggerPluginPrivate::displayDebugger(DebuggerEngine *engine, bool updateEngine)
{
    QTC_ASSERT(engine, return);
    disconnectEngine();
    connectEngine(engine);
    if (updateEngine)
        engine->updateAll();
    engine->updateViews();
}

void DebuggerPluginPrivate::startDebugger(RunControl *rc)
{
    QTC_ASSERT(rc, return);
    ProjectExplorerPlugin::instance()->startRunControl(rc, Constants::DEBUGMODE);
}


void DebuggerPluginPrivate::connectEngine(DebuggerEngine *engine)
{
    if (!engine)
        engine = dummyEngine();

    if (m_currentEngine == engine)
        return;

    if (m_currentEngine)
        m_currentEngine->resetLocation();
    m_currentEngine = engine;

    m_localsWindow->setModel(engine->localsModel());
    m_modulesWindow->setModel(engine->modulesModel());
    m_registerWindow->setModel(engine->registerModel());
    m_returnWindow->setModel(engine->returnModel());
    m_sourceFilesWindow->setModel(engine->sourceFilesModel());
    m_stackWindow->setModel(engine->stackModel());
    m_threadsWindow->setModel(engine->threadsModel());
    m_threadBox->setModel(engine->threadsModel());
    m_threadBox->setModelColumn(ThreadData::NameColumn);
    m_watchersWindow->setModel(engine->watchersModel());
}

static void changeFontSize(QWidget *widget, qreal size)
{
    QFont font = widget->font();
    font.setPointSizeF(size);
    widget->setFont(font);
}

void DebuggerPluginPrivate::fontSettingsChanged
    (const TextEditor::FontSettings &settings)
{
    qreal size = settings.fontZoom() * settings.fontSize() / 100.;
    changeFontSize(m_breakWindow, size);
    changeFontSize(m_logWindow, size);
    changeFontSize(m_localsWindow, size);
    changeFontSize(m_modulesWindow, size);
    //changeFontSize(m_consoleWindow, size);
    changeFontSize(m_registerWindow, size);
    changeFontSize(m_returnWindow, size);
    changeFontSize(m_sourceFilesWindow, size);
    changeFontSize(m_stackWindow, size);
    changeFontSize(m_threadsWindow, size);
    changeFontSize(m_watchersWindow, size);
}

void DebuggerPluginPrivate::cleanupViews()
{
    m_actions.reverseDirectionAction->setChecked(false);
    m_actions.reverseDirectionAction->setEnabled(false);
    hideDebuggerToolTip();

    if (!boolSetting(CloseBuffersOnExit))
        return;

    EditorManager *editorManager = EditorManager::instance();
    QTC_ASSERT(editorManager, return);
    QList<IEditor *> toClose;
    foreach (IEditor *editor, editorManager->openedEditors()) {
        if (editor->property(Constants::OPENED_BY_DEBUGGER).toBool()) {
            // Close disassembly views. Close other opened files
            // if they are not modified and not current editor.
            if (editor->property(Constants::OPENED_WITH_DISASSEMBLY).toBool()
                    || (!editor->file()->isModified()
                        && editor != editorManager->currentEditor())) {
                toClose.append(editor);
            } else {
                editor->setProperty(Constants::OPENED_BY_DEBUGGER, false);
            }
        }
    }
    editorManager->closeEditors(toClose);
}

void DebuggerPluginPrivate::setBusyCursor(bool busy)
{
    //STATE_DEBUG("BUSY FROM: " << m_busy << " TO: " << busy);
    if (busy == m_busy)
        return;
    m_busy = busy;
    QCursor cursor(busy ? Qt::BusyCursor : Qt::ArrowCursor);
    m_breakWindow->setCursor(cursor);
    //m_consoleWindow->setCursor(cursor);
    m_localsWindow->setCursor(cursor);
    m_modulesWindow->setCursor(cursor);
    m_logWindow->setCursor(cursor);
    m_registerWindow->setCursor(cursor);
    m_returnWindow->setCursor(cursor);
    m_sourceFilesWindow->setCursor(cursor);
    m_stackWindow->setCursor(cursor);
    m_threadsWindow->setCursor(cursor);
    m_watchersWindow->setCursor(cursor);
    m_snapshotWindow->setCursor(cursor);
    m_scriptConsoleWindow->setCursor(cursor);
}

void DebuggerPluginPrivate::setInitialState()
{
    m_watchersWindow->setVisible(false);
    m_returnWindow->setVisible(false);
    setBusyCursor(false);
    m_actions.reverseDirectionAction->setChecked(false);
    m_actions.reverseDirectionAction->setEnabled(false);
    hideDebuggerToolTip();

    m_startExternalAction->setEnabled(true);
    m_attachExternalAction->setEnabled(true);
#ifdef Q_OS_WIN
    m_attachCoreAction->setEnabled(false);
#else
    m_attachCoreAction->setEnabled(true);
#endif
    m_startRemoteAction->setEnabled(true);
    m_detachAction->setEnabled(false);

    m_actions.watchAction1->setEnabled(true);
    m_actions.watchAction2->setEnabled(true);
    m_actions.breakAction->setEnabled(true);
    //m_actions.snapshotAction->setEnabled(false);
    action(OperateByInstruction)->setEnabled(false);

    m_actions.exitAction->setEnabled(false);
    m_actions.resetAction->setEnabled(false);

    m_actions.stepAction->setEnabled(false);
    m_actions.stepOutAction->setEnabled(false);
    m_actions.runToLineAction->setEnabled(false);
    m_actions.runToFunctionAction->setEnabled(false);
    m_actions.returnFromFunctionAction->setEnabled(false);
    m_actions.jumpToLineAction->setEnabled(false);
    m_actions.nextAction->setEnabled(false);

    action(AutoDerefPointers)->setEnabled(true);
    action(ExpandStack)->setEnabled(false);
    action(ExecuteCommand)->setEnabled(m_state == InferiorStopOk);

    m_scriptConsoleWindow->setEnabled(false);

    //emit m_plugin->stateChanged(m_state);
}

void DebuggerPluginPrivate::updateWatchersWindow()
{
    m_watchersWindow->setVisible(
        m_watchersWindow->model()->rowCount(QModelIndex()) > 0);
    m_returnWindow->setVisible(
        m_returnWindow->model()->rowCount(QModelIndex()) > 0);
}

void DebuggerPluginPrivate::updateState(DebuggerEngine *engine)
{
    QTC_ASSERT(engine, return);
    QTC_ASSERT(m_watchersWindow->model(), return);
    QTC_ASSERT(m_returnWindow->model(), return);
    QTC_ASSERT(!engine->isSlaveEngine(), return);

    m_threadBox->setCurrentIndex(engine->threadsHandler()->currentThread());

    updateWatchersWindow();

    //m_plugin->showMessage(QString("PLUGIN SET STATE: ")
    //    + DebuggerEngine::stateName(engine->state()), LogStatus);
    //qDebug() << "PLUGIN SET STATE: " << engine->state();

    if (m_state == engine->state())
        return;

    m_state = engine->state();
    bool actionsEnabled = DebuggerEngine::debuggerActionsEnabled(m_state);

    ICore *core = ICore::instance();
    ActionManager *am = core->actionManager();
    if (m_state == DebuggerNotReady) {
        QTC_ASSERT(false, /* We use the Core m_debugAction here */);
        // F5 starts debugging. It is "startable".
        m_actions.interruptAction->setEnabled(false);
        m_actions.continueAction->setEnabled(false);
        m_actions.exitAction->setEnabled(false);
        am->command(Constants::STOP)->setKeySequence(QKeySequence());
        am->command(Constants::DEBUG)->setKeySequence(QKeySequence(DEBUG_KEY));
        core->updateAdditionalContexts(m_anyContext, Context());
    } else if (m_state == InferiorStopOk) {
        // F5 continues, Shift-F5 kills. It is "continuable".
        m_actions.interruptAction->setEnabled(false);
        m_actions.continueAction->setEnabled(true);
        m_actions.exitAction->setEnabled(true);
        am->command(Constants::STOP)->setKeySequence(QKeySequence(STOP_KEY));
        am->command(Constants::DEBUG)->setKeySequence(QKeySequence(DEBUG_KEY));
        core->updateAdditionalContexts(m_anyContext, m_continuableContext);
    } else if (m_state == InferiorRunOk) {
        // Shift-F5 interrupts. It is also "interruptible".
        m_actions.interruptAction->setEnabled(true);
        m_actions.continueAction->setEnabled(false);
        m_actions.exitAction->setEnabled(false);
        am->command(Constants::STOP)->setKeySequence(QKeySequence());
        am->command(Constants::DEBUG)->setKeySequence(QKeySequence(STOP_KEY));
        core->updateAdditionalContexts(m_anyContext, m_interruptibleContext);
    } else if (m_state == DebuggerFinished) {
        // We don't want to do anything anymore.
        m_actions.interruptAction->setEnabled(false);
        m_actions.continueAction->setEnabled(false);
        m_actions.exitAction->setEnabled(false);
        am->command(Constants::STOP)->setKeySequence(QKeySequence());
        am->command(Constants::DEBUG)->setKeySequence(QKeySequence(DEBUG_KEY));
        //core->updateAdditionalContexts(m_anyContext, m_finishedContext);
        m_codeModelSnapshot = CPlusPlus::Snapshot();
        core->updateAdditionalContexts(m_anyContext, Context());
        setBusyCursor(false);
        cleanupViews();
    } else if (m_state == InferiorUnrunnable) {
        // We don't want to do anything anymore.
        m_actions.interruptAction->setEnabled(false);
        m_actions.continueAction->setEnabled(false);
        m_actions.exitAction->setEnabled(true);
        am->command(Constants::STOP)->setKeySequence(QKeySequence(STOP_KEY));
        am->command(Constants::DEBUG)->setKeySequence(QKeySequence(STOP_KEY));
        core->updateAdditionalContexts(m_anyContext, m_finishedContext);
    } else {
        // Everything else is "undisturbable".
        m_actions.interruptAction->setEnabled(false);
        m_actions.continueAction->setEnabled(false);
        m_actions.exitAction->setEnabled(false);
        am->command(Constants::STOP)->setKeySequence(QKeySequence());
        am->command(Constants::DEBUG)->setKeySequence(QKeySequence());
        core->updateAdditionalContexts(m_anyContext, m_undisturbableContext);
    }

    m_startExternalAction->setEnabled(true);
    m_attachExternalAction->setEnabled(true);
#ifdef Q_OS_WIN
    m_attachCoreAction->setEnabled(false);
#else
    m_attachCoreAction->setEnabled(true);
#endif
    m_startRemoteAction->setEnabled(true);

    const bool isCore = engine->startParameters().startMode == AttachCore;
    const bool stopped = m_state == InferiorStopOk;
    const bool detachable = stopped && !isCore;
    m_detachAction->setEnabled(detachable);

    if (stopped)
        QApplication::alert(mainWindow(), 3000);

    const uint caps = engine->debuggerCapabilities();
    const bool canReverse = (caps & ReverseSteppingCapability)
                && boolSetting(EnableReverseDebugging);
    m_actions.reverseDirectionAction->setEnabled(canReverse);

    m_actions.watchAction1->setEnabled(true);
    m_actions.watchAction2->setEnabled(true);
    m_actions.breakAction->setEnabled(true);
    //m_actions.snapshotAction->setEnabled(stopped && (caps & SnapshotCapability));

    action(OperateByInstruction)->setEnabled(stopped || isCore);

    m_actions.resetAction->setEnabled(m_state != DebuggerNotReady
                                      && m_state != DebuggerFinished);

    m_actions.stepAction->setEnabled(stopped);
    m_actions.stepOutAction->setEnabled(stopped);
    m_actions.runToLineAction->setEnabled(stopped);
    m_actions.runToFunctionAction->setEnabled(stopped);
    m_actions.returnFromFunctionAction->
        setEnabled(stopped && (caps & ReturnFromFunctionCapability));

    const bool canJump = stopped && (caps & JumpToLineCapability);
    m_actions.jumpToLineAction->setEnabled(canJump);

    m_actions.nextAction->setEnabled(stopped);

    const bool canDeref = actionsEnabled && (caps & AutoDerefPointersCapability);
    action(AutoDerefPointers)->setEnabled(canDeref);
    action(AutoDerefPointers)->setEnabled(true);
    action(ExpandStack)->setEnabled(actionsEnabled);
    action(ExecuteCommand)->setEnabled(m_state == InferiorStopOk);

    const bool notbusy = m_state == InferiorStopOk
        || m_state == DebuggerNotReady
        || m_state == DebuggerFinished
        || m_state == InferiorUnrunnable;
    setBusyCursor(!notbusy);

    m_scriptConsoleWindow->setEnabled(stopped);
}

void DebuggerPluginPrivate::updateDebugActions()
{
    ProjectExplorerPlugin *pe = ProjectExplorerPlugin::instance();
    Project *project = pe->startupProject();
    m_debugAction->setEnabled(pe->canRun(project, Constants::DEBUGMODE));
}

void DebuggerPluginPrivate::onModeChanged(IMode *mode)
{
     // FIXME: This one gets always called, even if switching between modes
     //        different then the debugger mode. E.g. Welcome and Help mode and
     //        also on shutdown.

    m_mainWindow->onModeChanged(mode);

    if (mode->id() != Constants::MODE_DEBUG)
        return;

    EditorManager *editorManager = EditorManager::instance();
    if (editorManager->currentEditor())
        editorManager->currentEditor()->widget()->setFocus();
}

void DebuggerPluginPrivate::showSettingsDialog()
{
    ICore::instance()->showOptionsDialog(
        _(DEBUGGER_SETTINGS_CATEGORY),
        _(DEBUGGER_COMMON_SETTINGS_ID));
}

void DebuggerPluginPrivate::dumpLog()
{
    QString fileName = QFileDialog::getSaveFileName(mainWindow(),
        tr("Save Debugger Log"), QDir::tempPath());
    if (fileName.isEmpty())
        return;
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly))
        return;
    QTextStream ts(&file);
    ts << m_logWindow->inputContents();
    ts << "\n\n=======================================\n\n";
    ts << m_logWindow->combinedContents();
}

void DebuggerPluginPrivate::clearStatusMessage()
{
    m_statusLabel->setText(m_lastPermanentStatusMessage);
}

/*! Activates the previous mode when the current mode is the debug mode. */
void DebuggerPluginPrivate::activatePreviousMode()
{
    ModeManager *modeManager = ICore::instance()->modeManager();

    if (modeManager->currentMode() == modeManager->mode(MODE_DEBUG)
            && !m_previousMode.isEmpty()) {
        modeManager->activateMode(m_previousMode);
        m_previousMode.clear();
    }
}

void DebuggerPluginPrivate::activateDebugMode()
{
    m_actions.reverseDirectionAction->setChecked(false);
    m_actions.reverseDirectionAction->setEnabled(false);
    ModeManager *modeManager = ModeManager::instance();
    m_previousMode = modeManager->currentMode()->id();
    modeManager->activateMode(_(MODE_DEBUG));
}

void DebuggerPluginPrivate::sessionLoaded()
{
    m_breakHandler->loadSessionData();
    dummyEngine()->watchHandler()->loadSessionData();
}

void DebuggerPluginPrivate::aboutToUnloadSession()
{
    m_breakHandler->removeSessionData();
    // Stop debugging the active project when switching sessions.
    // Note that at startup, session switches may occur, which interfere
    // with command-line debugging startup.
    // FIXME ABC: Still wanted? Iterate?
    //if (d->m_engine && state() != DebuggerNotReady
    //    && engine()->sp().startMode == StartInternal)
    //        d->m_engine->shutdown();
}

void DebuggerPluginPrivate::aboutToSaveSession()
{
    dummyEngine()->watchHandler()->loadSessionData();
    m_breakHandler->saveSessionData();
}

void DebuggerPluginPrivate::executeDebuggerCommand()
{
    if (QAction *action = qobject_cast<QAction *>(sender()))
        currentEngine()->executeDebuggerCommand(action->data().toString());
}

void DebuggerPluginPrivate::showStatusMessage(const QString &msg0, int timeout)
{
    showMessage(msg0, LogStatus);
    QString msg = msg0;
    msg.replace(QLatin1Char('\n'), QString());
    m_statusLabel->setText(msg);
    if (timeout > 0) {
        m_statusTimer.setSingleShot(true);
        m_statusTimer.start(timeout);
    } else {
        m_lastPermanentStatusMessage = msg;
        m_statusTimer.stop();
    }
}

void DebuggerPluginPrivate::scriptExpressionEntered(const QString &expression)
{
    currentEngine()->executeDebuggerCommand(expression);
}

void DebuggerPluginPrivate::openMemoryEditor()
{
    AddressDialog dialog;
    if (dialog.exec() == QDialog::Accepted)
        currentEngine()->openMemoryView(dialog.address());
}

void DebuggerPluginPrivate::coreShutdown()
{
    m_shuttingDown = true;
}

const CPlusPlus::Snapshot &DebuggerPluginPrivate::cppCodeModelSnapshot() const
{
    using namespace CppTools;
    if (m_codeModelSnapshot.isEmpty() && action(UseCodeModel)->isChecked())
        m_codeModelSnapshot = CppModelManagerInterface::instance()->snapshot();
    return m_codeModelSnapshot;
}

void DebuggerPluginPrivate::setSessionValue(const QString &name, const QVariant &value)
{
    QTC_ASSERT(sessionManager(), return);
    sessionManager()->setValue(name, value);
    //qDebug() << "SET SESSION VALUE: " << name;
}

QVariant DebuggerPluginPrivate::sessionValue(const QString &name)
{
    QTC_ASSERT(sessionManager(), return QVariant());
    //qDebug() << "GET SESSION VALUE: " << name;
    return sessionManager()->value(name);
}

void DebuggerPluginPrivate::openTextEditor(const QString &titlePattern0,
    const QString &contents)
{
    if (m_shuttingDown)
        return;
    QString titlePattern = titlePattern0;
    EditorManager *editorManager = EditorManager::instance();
    QTC_ASSERT(editorManager, return);
    IEditor *editor = editorManager->openEditorWithContents(
        CC::K_DEFAULT_TEXT_EDITOR_ID, &titlePattern, contents);
    QTC_ASSERT(editor, return);
    editorManager->activateEditor(editor, EditorManager::IgnoreNavigationHistory);
}


void DebuggerPluginPrivate::clearCppCodeModelSnapshot()
{
    m_codeModelSnapshot = CPlusPlus::Snapshot();
}

void DebuggerPluginPrivate::showMessage(const QString &msg, int channel, int timeout)
{
    //qDebug() << "PLUGIN OUTPUT: " << channel << msg;
    //ConsoleWindow *cw = m_consoleWindow;
    QTC_ASSERT(m_logWindow, return);
    switch (channel) {
        case StatusBar:
            // This will append to m_logWindow's output pane, too.
            showStatusMessage(msg, timeout);
            break;
        case LogMiscInput:
            m_logWindow->showInput(LogMisc, msg);
            m_logWindow->showOutput(LogMisc, msg);
            break;
        case LogInput:
            m_logWindow->showInput(LogInput, msg);
            m_logWindow->showOutput(LogInput, msg);
            break;
        case ScriptConsoleOutput:
            m_scriptConsoleWindow->appendResult(msg);
            break;
        case LogError: {
            m_logWindow->showOutput(channel, msg);
            QAction *action = m_mainWindow->dockWidget(_(DOCKWIDGET_OUTPUT))
                ->toggleViewAction();
            if (!action->isChecked())
                action->trigger();
            break;
        }
        default:
            m_logWindow->showOutput(channel, msg);
            break;
    }
}

void DebuggerPluginPrivate::showQtDumperLibraryWarning(const QString &details)
{
    QMessageBox dialog(mainWindow());
    QPushButton *qtPref = dialog.addButton(tr("Open Qt4 Options"),
        QMessageBox::ActionRole);
    QPushButton *helperOff = dialog.addButton(tr("Turn off Helper Usage"),
        QMessageBox::ActionRole);
    QPushButton *justContinue = dialog.addButton(tr("Continue Anyway"),
        QMessageBox::AcceptRole);
    dialog.setDefaultButton(justContinue);
    dialog.setWindowTitle(tr("Debugging Helper Missing"));
    dialog.setText(tr("The debugger could not load the debugging helper library."));
    dialog.setInformativeText(tr(
        "The debugging helper is used to nicely format the values of some Qt "
        "and Standard Library data types. "
        "It must be compiled for each used Qt version separately. "
        "On the Qt4 options page, select a Qt installation "
        "and click Rebuild."));
    if (!details.isEmpty())
        dialog.setDetailedText(details);
    dialog.exec();
    if (dialog.clickedButton() == qtPref) {
        ICore::instance()->showOptionsDialog(
            _(Qt4ProjectManager::Constants::QT_SETTINGS_CATEGORY),
            _(Qt4ProjectManager::Constants::QTVERSION_SETTINGS_PAGE_ID));
    } else if (dialog.clickedButton() == helperOff) {
        action(UseDebuggingHelpers)->setValue(qVariantFromValue(false), false);
    }
}

void DebuggerPluginPrivate::createNewDock(QWidget *widget)
{
    QDockWidget *dockWidget =
        m_mainWindow->createDockWidget(CppLanguage, widget);
    dockWidget->setWindowTitle(widget->windowTitle());
    dockWidget->setFeatures(QDockWidget::DockWidgetClosable);
    dockWidget->show();
}

void DebuggerPluginPrivate::runControlStarted(DebuggerEngine *engine)
{
    activateDebugMode();
    const QString message = tr("Starting debugger '%1' for tool chain '%2'...")
            .arg(engine->objectName())
            .arg(engine->startParameters().toolChainName());
    showMessage(message, StatusBar);
    showMessage(m_debuggerSettings->dump(), LogDebug);
    m_snapshotHandler->appendSnapshot(engine);
    connectEngine(engine);
}

void DebuggerPluginPrivate::runControlFinished(DebuggerEngine *engine)
{
    m_snapshotHandler->removeSnapshot(engine);
    if (m_snapshotHandler->size() == 0) {
        // Last engine quits.
        disconnectEngine();
        if (boolSetting(SwitchModeOnExit))
            activatePreviousMode();
    } else {
        // Connect to some existing engine.
        m_snapshotHandler->activateSnapshot(0);
    }
}

void DebuggerPluginPrivate::remoteCommand(const QStringList &options,
    const QStringList &)
{
    if (options.isEmpty())
        return;

    unsigned enabledEngines = 0;
    QString errorMessage;

    if (!parseArguments(options,
            &m_attachRemoteParameters, &enabledEngines, &errorMessage)) {
        qWarning("%s", qPrintable(errorMessage));
        return;
    }

    if (!attachCmdLine())
        qWarning("%s", qPrintable(
            _("Incomplete remote attach command received: %1").
               arg(options.join(QString(QLatin1Char(' '))))));
}

QString DebuggerPluginPrivate::gdbBinaryForToolChain(int toolChain) const
{
    return GdbOptionsPage::gdbBinaryToolChainMap.key(toolChain);
}

DebuggerLanguages DebuggerPluginPrivate::activeLanguages() const
{
    QTC_ASSERT(m_mainWindow, return AnyLanguage);
    return m_mainWindow->activeDebugLanguages();
}

bool DebuggerPluginPrivate::isReverseDebugging() const
{
    return m_actions.reverseDirectionAction->isChecked();
}

QMessageBox *showMessageBox(int icon, const QString &title,
    const QString &text, int buttons)
{
    QMessageBox *mb = new QMessageBox(QMessageBox::Icon(icon),
        title, text, QMessageBox::StandardButtons(buttons),
        debuggerCore()->mainWindow());
    mb->setAttribute(Qt::WA_DeleteOnClose);
    mb->show();
    return mb;
}

void DebuggerPluginPrivate::extensionsInitialized()
{
    ICore *core = ICore::instance();
    QTC_ASSERT(core, return);
    m_coreSettings = core->settings();
    m_debuggerSettings = new DebuggerSettings(m_coreSettings);

    m_continuableContext = Context("Gdb.Continuable");
    m_interruptibleContext = Context("Gdb.Interruptible");
    m_undisturbableContext = Context("Gdb.Undisturbable");
    m_finishedContext = Context("Gdb.Finished");
    m_anyContext.add(m_continuableContext);
    m_anyContext.add(m_interruptibleContext);
    m_anyContext.add(m_undisturbableContext);
    m_anyContext.add(m_finishedContext);

    connect(core, SIGNAL(coreAboutToClose()), this, SLOT(coreShutdown()));

    Core::ActionManager *am = core->actionManager();
    QTC_ASSERT(am, return);

    const Context globalcontext(CC::C_GLOBAL);
    const Context cppDebuggercontext(C_CPPDEBUGGER);
    const Context qmlDebuggerContext(C_QMLDEBUGGER);
    const Context cppeditorcontext(CppEditor::Constants::C_CPPEDITOR);

    m_startIcon = QIcon(_(":/debugger/images/debugger_start_small.png"));
    m_startIcon.addFile(__(":/debugger/images/debugger_start.png"));
    m_exitIcon = QIcon(_(":/debugger/images/debugger_stop_small.png"));
    m_exitIcon.addFile(__(":/debugger/images/debugger_stop.png"));
    m_continueIcon = QIcon(__(":/debugger/images/debugger_continue_small.png"));
    m_continueIcon.addFile(__(":/debugger/images/debugger_continue.png"));
    m_interruptIcon = QIcon(_(":/debugger/images/debugger_interrupt_small.png"));
    m_interruptIcon.addFile(__(":/debugger/images/debugger_interrupt.png"));
    m_locationMarkIcon = QIcon(_(":/debugger/images/location_16.png"));

    m_busy = false;

    m_statusLabel = new QLabel;
    m_statusLabel->setMinimumSize(QSize(30, 10));

    m_breakHandler = new BreakHandler;
    m_breakWindow = new BreakWindow;
    m_breakWindow->setObjectName(DOCKWIDGET_BREAK);
    m_breakWindow->setModel(m_breakHandler->model());

    //m_consoleWindow = new ConsoleWindow;
    //m_consoleWindow->setObjectName(QLatin1String("CppDebugConsole"));
    m_modulesWindow = new ModulesWindow;
    m_modulesWindow->setObjectName(DOCKWIDGET_MODULES);
    m_logWindow = new LogWindow;
    m_logWindow->setObjectName(DOCKWIDGET_OUTPUT);
    m_registerWindow = new RegisterWindow;
    m_registerWindow->setObjectName(DOCKWIDGET_REGISTER);
    m_stackWindow = new StackWindow;
    m_stackWindow->setObjectName(DOCKWIDGET_STACK);
    m_sourceFilesWindow = new SourceFilesWindow;
    m_sourceFilesWindow->setObjectName(DOCKWIDGET_SOURCE_FILES);
    m_threadsWindow = new ThreadsWindow;
    m_threadsWindow->setObjectName(DOCKWIDGET_THREADS);
    m_returnWindow = new WatchWindow(WatchWindow::ReturnType);
    m_returnWindow->setObjectName(QLatin1String("CppDebugReturn"));
    m_localsWindow = new WatchWindow(WatchWindow::LocalsType);
    m_localsWindow->setObjectName(QLatin1String("CppDebugLocals"));
    m_watchersWindow = new WatchWindow(WatchWindow::WatchersType);
    m_watchersWindow->setObjectName(QLatin1String("CppDebugWatchers"));
    m_scriptConsoleWindow = new ScriptConsole;
    m_scriptConsoleWindow->setWindowTitle(tr("QML Script Console"));
    m_scriptConsoleWindow->setObjectName(DOCKWIDGET_QML_SCRIPTCONSOLE);
    connect(m_scriptConsoleWindow, SIGNAL(expressionEntered(QString)),
        SLOT(scriptExpressionEntered(QString)));

    // Snapshot
    m_snapshotHandler = new SnapshotHandler;
    m_snapshotWindow = new SnapshotWindow(m_snapshotHandler);
    m_snapshotWindow->setObjectName(DOCKWIDGET_SNAPSHOTS);
    m_snapshotWindow->setModel(m_snapshotHandler->model());

    // Watchers
    connect(m_localsWindow->header(), SIGNAL(sectionResized(int,int,int)),
        SLOT(updateWatchersHeader(int,int,int)), Qt::QueuedConnection);

    QAction *act = 0;

    act = m_actions.continueAction = new QAction(tr("Continue"), this);
    act->setIcon(m_continueIcon);
    connect(act, SIGNAL(triggered()), SLOT(handleExecContinue()));

    act = m_actions.exitAction = new QAction(tr("Exit Debugger"), this);
    act->setIcon(m_exitIcon);
    connect(act, SIGNAL(triggered()), SLOT(handleExecExit()));

    act = m_actions.interruptAction = new QAction(tr("Interrupt"), this);
    act->setIcon(m_interruptIcon);
    connect(act, SIGNAL(triggered()), SLOT(handleExecInterrupt()));

    // A "disabled pause" seems to be a good choice.
    act = m_actions.undisturbableAction = new QAction(tr("Debugger is Busy"), this);
    act->setIcon(m_interruptIcon);
    act->setEnabled(false);

    act = m_actions.resetAction = new QAction(tr("Abort Debugging"), this);
    act->setToolTip(tr("Aborts debugging and "
        "resets the debugger to the initial state."));
    connect(act, SIGNAL(triggered()), SLOT(handleExecReset()));

    act = m_actions.nextAction = new QAction(tr("Step Over"), this);
    act->setIcon(QIcon(__(":/debugger/images/debugger_stepover_small.png")));
    connect(act, SIGNAL(triggered()), SLOT(handleExecNext()));

    act = m_actions.stepAction = new QAction(tr("Step Into"), this);
    act->setIcon(QIcon(__(":/debugger/images/debugger_stepinto_small.png")));
    connect(act, SIGNAL(triggered()), SLOT(handleExecStep()));

    act = m_actions.stepOutAction = new QAction(tr("Step Out"), this);
    act->setIcon(QIcon(__(":/debugger/images/debugger_stepout_small.png")));
    connect(act, SIGNAL(triggered()), SLOT(handleExecStepOut()));

    act = m_actions.runToLineAction = new QAction(tr("Run to Line"), this);
    connect(act, SIGNAL(triggered()), SLOT(handleExecRunToLine()));

    act = m_actions.runToFunctionAction =
        new QAction(tr("Run to Outermost Function"), this);
    connect(act, SIGNAL(triggered()), SLOT(handleExecRunToFunction()));

    act = m_actions.returnFromFunctionAction =
        new QAction(tr("Immediately Return From Inner Function"), this);
    connect(act, SIGNAL(triggered()), SLOT(handleExecReturn()));

    act = m_actions.jumpToLineAction = new QAction(tr("Jump to Line"), this);
    connect(act, SIGNAL(triggered()), SLOT(handleExecJumpToLine()));

    act = m_actions.breakAction = new QAction(tr("Toggle Breakpoint"), this);

    act = m_actions.watchAction1 = new QAction(tr("Add to Watch Window"), this);
    connect(act, SIGNAL(triggered()), SLOT(handleAddToWatchWindow()));

    act = m_actions.watchAction2 = new QAction(tr("Add to Watch Window"), this);
    connect(act, SIGNAL(triggered()), SLOT(handleAddToWatchWindow()));

    //m_actions.snapshotAction = new QAction(tr("Create Snapshot"), this);
    //m_actions.snapshotAction->setProperty(Role, RequestCreateSnapshotRole);
    //m_actions.snapshotAction->setIcon(
    //    QIcon(__(":/debugger/images/debugger_snapshot_small.png")));

    act = m_actions.reverseDirectionAction =
        new QAction(tr("Reverse Direction"), this);
    act->setCheckable(true);
    act->setChecked(false);
    act->setCheckable(false);
    act->setIcon(QIcon(__(":/debugger/images/debugger_reversemode_16.png")));
    act->setIconVisibleInMenu(false);

    act = m_actions.frameDownAction = new QAction(tr("Move to Called Frame"), this);
    connect(act, SIGNAL(triggered()), SLOT(handleFrameDown()));

    act = m_actions.frameUpAction = new QAction(tr("Move to Calling Frame"), this);
    connect(act, SIGNAL(triggered()), SLOT(handleFrameUp()));

    connect(action(OperateByInstruction), SIGNAL(triggered(bool)),
        SLOT(handleOperateByInstructionTriggered(bool)));

    connect(&m_statusTimer, SIGNAL(timeout()), SLOT(clearStatusMessage()));

    connect(action(ExecuteCommand), SIGNAL(triggered()),
        SLOT(executeDebuggerCommand()));

    ActionContainer *debugMenu =
        am->actionContainer(ProjectExplorer::Constants::M_DEBUG);

    // Dock widgets
    QDockWidget *dock = 0;
    dock = m_mainWindow->createDockWidget(CppLanguage, m_modulesWindow);
    connect(dock->toggleViewAction(), SIGNAL(toggled(bool)),
        SLOT(modulesDockToggled(bool)), Qt::QueuedConnection);

    dock = m_mainWindow->createDockWidget(CppLanguage, m_registerWindow);
    connect(dock->toggleViewAction(), SIGNAL(toggled(bool)),
        SLOT(registerDockToggled(bool)), Qt::QueuedConnection);

    dock = m_mainWindow->createDockWidget(CppLanguage, m_sourceFilesWindow);
    connect(dock->toggleViewAction(), SIGNAL(toggled(bool)),
        SLOT(sourceFilesDockToggled(bool)), Qt::QueuedConnection);

    dock = m_mainWindow->createDockWidget(AnyLanguage, m_logWindow);
    dock->setProperty(DOCKWIDGET_DEFAULT_AREA, Qt::TopDockWidgetArea);

    m_mainWindow->createDockWidget(CppLanguage, m_breakWindow);
    //m_mainWindow->createDockWidget(CppLanguage, m_consoleWindow);
    m_mainWindow->createDockWidget(CppLanguage, m_snapshotWindow);
    m_mainWindow->createDockWidget(CppLanguage, m_stackWindow);
    m_mainWindow->createDockWidget(CppLanguage, m_threadsWindow);
    m_mainWindow->createDockWidget(QmlLanguage, m_scriptConsoleWindow);

    QSplitter *localsAndWatchers = new Core::MiniSplitter(Qt::Vertical);
    localsAndWatchers->setObjectName(DOCKWIDGET_WATCHERS);
    localsAndWatchers->setWindowTitle(m_localsWindow->windowTitle());
    localsAndWatchers->addWidget(m_localsWindow);
    localsAndWatchers->addWidget(m_returnWindow);
    localsAndWatchers->addWidget(m_watchersWindow);
    localsAndWatchers->setStretchFactor(0, 3);
    localsAndWatchers->setStretchFactor(1, 1);
    localsAndWatchers->setStretchFactor(2, 1);

    dock = m_mainWindow->createDockWidget(CppLanguage, localsAndWatchers);
    dock->setProperty(DOCKWIDGET_DEFAULT_AREA, Qt::RightDockWidgetArea);

    m_debuggerSettings->readSettings();
    GdbOptionsPage::readGdbBinarySettings();

    // Register factory of DebuggerRunControl.
    m_debuggerRunControlFactory = new DebuggerRunControlFactory
        (m_plugin, DebuggerEngineType(m_cmdLineEnabledEngines));
    m_plugin->addAutoReleasedObject(m_debuggerRunControlFactory);

    // The main "Start Debugging" action.
    act = m_debugAction = new QAction(this);
    QIcon debuggerIcon(":/projectexplorer/images/debugger_start_small.png");
    debuggerIcon.addFile(":/projectexplorer/images/debugger_start.png");
    act->setIcon(debuggerIcon);
    act->setText(tr("Start Debugging"));
    connect(act, SIGNAL(triggered()), this, SLOT(debugProject()));

    // Handling of external applications.
    act = m_startExternalAction = new QAction(this);
    act->setText(tr("Start and Debug External Application..."));
    connect(act, SIGNAL(triggered()), SLOT(startExternalApplication()));

    act = m_startRemoteLldbAction = new QAction(this);
    act->setText(tr("Start and Debug External Application with External Engine..."));
    connect(act, SIGNAL(triggered()), SLOT(startRemoteEngine()));

    act = m_attachExternalAction = new QAction(this);
    act->setText(tr("Attach to Running External Application..."));
    connect(act, SIGNAL(triggered()), SLOT(attachExternalApplication()));

    act = m_attachCoreAction = new QAction(this);
    act->setText(tr("Attach to Core..."));
    connect(act, SIGNAL(triggered()), SLOT(attachCore()));

    act = m_attachTcfAction = new QAction(this);
    act->setText(tr("Attach to Running Tcf Agent..."));
    act->setToolTip(tr("This attaches to a running "
        "'Target Communication Framework' agent."));
    connect(act, SIGNAL(triggered()), SLOT(attachRemoteTcf()));

    act = m_startRemoteAction = new QAction(this);
    act->setText(tr("Start and Attach to Remote Application..."));
    connect(act, SIGNAL(triggered()), SLOT(startRemoteApplication()));

#ifdef Q_OS_WIN
    m_startRemoteCdbAction = new QAction(tr("Attach to Remote CDB Session..."), this);
    connect(m_startRemoteCdbAction, SIGNAL(triggered()), SLOT(startRemoteCdbSession()));
#endif

    act = m_detachAction = new QAction(this);
    act->setText(tr("Detach Debugger"));
    connect(act, SIGNAL(triggered()), SLOT(handleExecDetach()));

    Command *cmd = 0;
    ActionContainer *mstart = am->actionContainer(PE::M_DEBUG_STARTDEBUGGING);

    cmd = am->registerAction(m_debugAction, Constants::DEBUG, globalcontext);
    cmd->setAttribute(Core::Command::CA_UpdateText);
    cmd->setAttribute(Core::Command::CA_UpdateIcon);
    cmd->setDefaultText(tr("Start Debugging"));
    cmd->setDefaultKeySequence(QKeySequence(Constants::DEBUG_KEY));
    mstart->addAction(cmd, Core::Constants::G_DEFAULT_ONE);
    Core::ICore::instance()->modeManager()->addAction(cmd, Constants::P_ACTION_DEBUG);

    cmd = am->registerAction(m_actions.continueAction,
        Constants::DEBUG, m_continuableContext);
    mstart->addAction(cmd, CC::G_DEFAULT_ONE);

    cmd = am->registerAction(m_startExternalAction,
        Constants::STARTEXTERNAL, globalcontext);
    cmd->setAttribute(Command::CA_Hide);
    mstart->addAction(cmd, CC::G_DEFAULT_ONE);

    cmd = am->registerAction(m_startRemoteLldbAction,
        Constants::STARTREMOTELLDB, globalcontext);
    cmd->setAttribute(Command::CA_Hide);
    mstart->addAction(cmd, CC::G_DEFAULT_ONE);

    cmd = am->registerAction(m_attachExternalAction,
        Constants::ATTACHEXTERNAL, globalcontext);
    cmd->setAttribute(Command::CA_Hide);
    mstart->addAction(cmd, CC::G_DEFAULT_ONE);

    cmd = am->registerAction(m_attachCoreAction,
        Constants::ATTACHCORE, globalcontext);

    cmd->setAttribute(Command::CA_Hide);
    mstart->addAction(cmd, CC::G_DEFAULT_ONE);

    cmd = am->registerAction(m_attachTcfAction,
        Constants::ATTACHTCF, globalcontext);
    mstart->addAction(cmd, CC::G_DEFAULT_ONE);

    cmd = am->registerAction(m_startRemoteAction,
        Constants::ATTACHREMOTE, globalcontext);
    cmd->setAttribute(Command::CA_Hide);
    mstart->addAction(cmd, CC::G_DEFAULT_ONE);

    if (m_startRemoteCdbAction) {
        cmd = am->registerAction(m_startRemoteCdbAction,
                                 Constants::ATTACHREMOTECDB, globalcontext);
        cmd->setAttribute(Command::CA_Hide);
        mstart->addAction(cmd, CC::G_DEFAULT_ONE);
    }

    cmd = am->registerAction(m_detachAction,
        Constants::DETACH, globalcontext);
    cmd->setAttribute(Command::CA_Hide);
    debugMenu->addAction(cmd, CC::G_DEFAULT_ONE);

    cmd = am->registerAction(m_actions.exitAction,
        Constants::STOP, globalcontext);
    //cmd->setDefaultKeySequence(QKeySequence(Constants::STOP_KEY));
    cmd->setDefaultText(tr("Stop Debugger"));
    debugMenu->addAction(cmd, CC::G_DEFAULT_ONE);

    cmd = am->registerAction(m_actions.interruptAction,
        Constants::DEBUG, m_interruptibleContext);
    cmd->setDefaultText(tr("Interrupt Debugger"));

    cmd = am->registerAction(m_actions.undisturbableAction,
        Constants::DEBUG, m_undisturbableContext);
    cmd->setDefaultText(tr("Debugger is Busy"));

    cmd = am->registerAction(m_actions.resetAction,
        Constants::RESET, globalcontext);
    //cmd->setDefaultKeySequence(QKeySequence(Constants::RESET_KEY));
    cmd->setDefaultText(tr("Reset Debugger"));
    debugMenu->addAction(cmd, CC::G_DEFAULT_ONE);

    QAction *sep = new QAction(this);
    sep->setSeparator(true);
    cmd = am->registerAction(sep, _("Debugger.Sep.Step"), globalcontext);
    debugMenu->addAction(cmd);

    cmd = am->registerAction(m_actions.nextAction,
        Constants::NEXT, cppDebuggercontext);
    cmd->setDefaultKeySequence(QKeySequence(Constants::NEXT_KEY));
    cmd->setAttribute(Command::CA_Hide);
    debugMenu->addAction(cmd);

    cmd = am->registerAction(m_actions.stepAction,
        Constants::STEP, cppDebuggercontext);
    cmd->setDefaultKeySequence(QKeySequence(Constants::STEP_KEY));
    cmd->setAttribute(Command::CA_Hide);
    debugMenu->addAction(cmd);

    cmd = am->registerAction(m_actions.stepOutAction,
        Constants::STEPOUT, cppDebuggercontext);
    cmd->setDefaultKeySequence(QKeySequence(Constants::STEPOUT_KEY));
    cmd->setAttribute(Command::CA_Hide);
    debugMenu->addAction(cmd);

    cmd = am->registerAction(m_actions.runToLineAction,
        Constants::RUN_TO_LINE1, cppDebuggercontext);
    cmd->setDefaultKeySequence(QKeySequence(Constants::RUN_TO_LINE_KEY));
    cmd->setAttribute(Command::CA_Hide);
    debugMenu->addAction(cmd);

    cmd = am->registerAction(m_actions.runToFunctionAction,
        Constants::RUN_TO_FUNCTION, cppDebuggercontext);
    cmd->setDefaultKeySequence(QKeySequence(Constants::RUN_TO_FUNCTION_KEY));
    cmd->setAttribute(Command::CA_Hide);
    debugMenu->addAction(cmd);

    cmd = am->registerAction(m_actions.jumpToLineAction,
        Constants::JUMP_TO_LINE1, cppDebuggercontext);
    cmd->setAttribute(Command::CA_Hide);
    debugMenu->addAction(cmd);

    cmd = am->registerAction(m_actions.returnFromFunctionAction,
        Constants::RETURN_FROM_FUNCTION, cppDebuggercontext);
    cmd->setAttribute(Command::CA_Hide);
    debugMenu->addAction(cmd);

    cmd = am->registerAction(m_actions.reverseDirectionAction,
        Constants::REVERSE, cppDebuggercontext);
    cmd->setDefaultKeySequence(QKeySequence(Constants::REVERSE_KEY));
    cmd->setAttribute(Command::CA_Hide);
    debugMenu->addAction(cmd);

    sep = new QAction(this);
    sep->setSeparator(true);
    cmd = am->registerAction(sep, _("Debugger.Sep.Break"), globalcontext);
    debugMenu->addAction(cmd);

    //cmd = am->registerAction(m_actions.snapshotAction,
    //    Constants::SNAPSHOT, cppDebuggercontext);
    //cmd->setDefaultKeySequence(QKeySequence(Constants::SNAPSHOT_KEY));
    //cmd->setAttribute(Command::CA_Hide);
    //debugMenu->addAction(cmd);

    cmd = am->registerAction(m_actions.frameDownAction,
        Constants::FRAME_DOWN, cppDebuggercontext);
    cmd = am->registerAction(m_actions.frameUpAction,
        Constants::FRAME_UP, cppDebuggercontext);

    cmd = am->registerAction(action(OperateByInstruction),
        Constants::OPERATE_BY_INSTRUCTION, cppDebuggercontext);
    cmd->setAttribute(Command::CA_Hide);
    debugMenu->addAction(cmd);

    cmd = am->registerAction(m_actions.breakAction,
        Constants::TOGGLE_BREAK, globalcontext);
    cmd->setDefaultKeySequence(QKeySequence(Constants::TOGGLE_BREAK_KEY));
    debugMenu->addAction(cmd);
    connect(m_actions.breakAction, SIGNAL(triggered()),
        SLOT(toggleBreakpoint()));

    sep = new QAction(this);
    sep->setSeparator(true);
    cmd = am->registerAction(sep, _("Debugger.Sep.Watch"), globalcontext);
    debugMenu->addAction(cmd);

    cmd = am->registerAction(m_actions.watchAction1,
        Constants::ADD_TO_WATCH1, cppeditorcontext);
    cmd->action()->setEnabled(true);
    //cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+D,Ctrl+W")));
    debugMenu->addAction(cmd);


    // Editor context menu
    ActionContainer *editorContextMenu =
        am->actionContainer(CppEditor::Constants::M_CONTEXT);
    cmd = am->registerAction(sep, _("Debugger.Sep.Views"),
        cppDebuggercontext);
    editorContextMenu->addAction(cmd);
    cmd->setAttribute(Command::CA_Hide);

    cmd = am->registerAction(m_actions.watchAction2,
        Constants::ADD_TO_WATCH2, cppDebuggercontext);
    cmd->action()->setEnabled(true);
    editorContextMenu->addAction(cmd);
    cmd->setAttribute(Command::CA_Hide);

    m_plugin->addAutoReleasedObject(new CommonOptionsPage);
    QList<Core::IOptionsPage *> engineOptionPages;
    if (m_cmdLineEnabledEngines & GdbEngineType)
        addGdbOptionPages(&engineOptionPages);
#ifdef CDB_ENABLED
    if (m_cmdLineEnabledEngines & CdbEngineType)
        addCdbOptionPages(&engineOptionPages);
#endif
#ifdef Q_OS_WIN
    Debugger::Cdb::addCdb2OptionPages(&engineOptionPages);
#endif
#ifdef WITH_LLDB
    if (m_cmdLineEnabledEngines & LldbEngineType)
        addLldbOptionPages(&engineOptionPages);
#endif

    //if (m_cmdLineEnabledEngines & ScriptEngineType)
    //    addScriptOptionPages(&engineOptionPages);
    //if (m_cmdLineEnabledEngines & TcfEngineType)
    //    addTcfOptionPages(&engineOptionPages);
    foreach (Core::IOptionsPage *op, engineOptionPages)
        m_plugin->addAutoReleasedObject(op);
    m_plugin->addAutoReleasedObject(new DebuggingHelperOptionPage);

    connect(ModeManager::instance(), SIGNAL(currentModeChanged(Core::IMode*)),
        SLOT(onModeChanged(Core::IMode*)));


    // Debug mode setup
    m_plugin->addAutoReleasedObject(new DebugMode);

    //
    //  Connections
    //

    // TextEditor
    connect(TextEditorSettings::instance(),
        SIGNAL(fontSettingsChanged(TextEditor::FontSettings)),
        SLOT(fontSettingsChanged(TextEditor::FontSettings)));

    // ProjectExplorer
    connect(sessionManager(), SIGNAL(sessionLoaded()),
        SLOT(sessionLoaded()));
    connect(sessionManager(), SIGNAL(aboutToSaveSession()),
        SLOT(aboutToSaveSession()));
    connect(sessionManager(), SIGNAL(aboutToUnloadSession()),
        SLOT(aboutToUnloadSession()));
    connect(ProjectExplorerPlugin::instance(), SIGNAL(updateRunActions()),
        SLOT(updateDebugActions()));

    // EditorManager
    QObject *editorManager = core->editorManager();
    connect(editorManager, SIGNAL(editorAboutToClose(Core::IEditor*)),
        SLOT(editorAboutToClose(Core::IEditor*)));
    connect(editorManager, SIGNAL(editorOpened(Core::IEditor*)),
        SLOT(editorOpened(Core::IEditor*)));

    // Application interaction
    connect(action(SettingsDialog), SIGNAL(triggered()),
        SLOT(showSettingsDialog()));

    // Toolbar
    QWidget *toolbarContainer = new QWidget;

    QHBoxLayout *hbox = new QHBoxLayout(toolbarContainer);
    hbox->setMargin(0);
    hbox->setSpacing(0);
    hbox->addWidget(toolButton(am->command(Constants::DEBUG)->action()));
    hbox->addWidget(toolButton(am->command(STOP)->action()));
    hbox->addWidget(toolButton(am->command(NEXT)->action()));
    hbox->addWidget(toolButton(am->command(STEP)->action()));
    hbox->addWidget(toolButton(am->command(STEPOUT)->action()));
    hbox->addWidget(toolButton(am->command(OPERATE_BY_INSTRUCTION)->action()));

    //hbox->addWidget(new Utils::StyledSeparator);
    m_reverseToolButton = toolButton(am->command(REVERSE)->action());
    hbox->addWidget(m_reverseToolButton);
    //m_reverseToolButton->hide();

    hbox->addWidget(new Utils::StyledSeparator);
    hbox->addWidget(new QLabel(tr("Threads:")));

    m_threadBox = new QComboBox;
    connect(m_threadBox, SIGNAL(activated(int)), SLOT(selectThread(int)));

    hbox->addWidget(m_threadBox);
    hbox->addSpacerItem(new QSpacerItem(4, 0));
    hbox->addWidget(m_statusLabel, 10);

    m_mainWindow->setToolbar(CppLanguage, toolbarContainer);

    connect(action(EnableReverseDebugging),
        SIGNAL(valueChanged(QVariant)),
        SLOT(enableReverseDebuggingTriggered(QVariant)));

    setInitialState();
    connectEngine(0);

    connect(sessionManager(),
        SIGNAL(startupProjectChanged(ProjectExplorer::Project*)),
        SLOT(onCurrentProjectChanged(ProjectExplorer::Project*)));

    QTC_ASSERT(m_coreSettings, /**/);
    m_watchersWindow->setVisible(false);
    m_returnWindow->setVisible(false);

    // time gdb -i mi -ex 'debuggerplugin.cpp:800' -ex r -ex q bin/qtcreator.bin
    const QByteArray env = qgetenv("QTC_DEBUGGER_TEST");
    //qDebug() << "EXTENSIONS INITIALIZED:" << env;
    // if (!env.isEmpty())
    //    m_plugin->runTest(QString::fromLocal8Bit(env));
    if (m_attachRemoteParameters.attachPid
            || !m_attachRemoteParameters.attachTarget.isEmpty())
        QTimer::singleShot(0, this, SLOT(attachCmdLine()));
}

Utils::SavedAction *DebuggerPluginPrivate::action(int code) const
{
    return m_debuggerSettings->item(code);
}

bool DebuggerPluginPrivate::boolSetting(int code) const
{
    return m_debuggerSettings->item(code)->value().toBool();
}

QString DebuggerPluginPrivate::stringSetting(int code) const
{
    return m_debuggerSettings->item(code)->value().toString();
}

void DebuggerPluginPrivate::showModuleSymbols(const QString &moduleName,
    const Symbols &symbols)
{
    QTreeWidget *w = new QTreeWidget;
    w->setColumnCount(5);
    w->setRootIsDecorated(false);
    w->setAlternatingRowColors(true);
    w->setSortingEnabled(true);
    w->setObjectName("Symbols." + moduleName);
    QStringList header;
    header.append(tr("Symbol"));
    header.append(tr("Address"));
    header.append(tr("Code"));
    header.append(tr("Section"));
    header.append(tr("Name"));
    w->setHeaderLabels(header);
    w->setWindowTitle(tr("Symbols in \"%1\"").arg(moduleName));
    foreach (const Symbol &s, symbols) {
        QTreeWidgetItem *it = new QTreeWidgetItem;
        it->setData(0, Qt::DisplayRole, s.name);
        it->setData(1, Qt::DisplayRole, s.address);
        it->setData(2, Qt::DisplayRole, s.state);
        it->setData(3, Qt::DisplayRole, s.section);
        it->setData(4, Qt::DisplayRole, s.demangled);
        w->addTopLevelItem(it);
    }
    createNewDock(w);
}

void DebuggerPluginPrivate::aboutToShutdown()
{
    disconnect(sessionManager(),
        SIGNAL(startupProjectChanged(ProjectExplorer::Project*)),
        this, 0);
    m_debuggerSettings->writeSettings();
    m_mainWindow->writeSettings();
    if (GdbOptionsPage::gdbBinariesChanged)
        GdbOptionsPage::writeGdbBinarySettings();
}

} // namespace Internal


///////////////////////////////////////////////////////////////////////
//
// DebuggerPlugin
//
///////////////////////////////////////////////////////////////////////

using namespace Debugger::Internal;

DebuggerPlugin::DebuggerPlugin()
{
    theDebuggerCore = new DebuggerPluginPrivate(this);
}

DebuggerPlugin::~DebuggerPlugin()
{
    delete theDebuggerCore;
    theDebuggerCore = 0;
}

bool DebuggerPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    return theDebuggerCore->initialize(arguments, errorMessage);
}

ExtensionSystem::IPlugin::ShutdownFlag DebuggerPlugin::aboutToShutdown()
{
    theDebuggerCore->aboutToShutdown();
    return SynchronousShutdown;
}

void DebuggerPlugin::remoteCommand(const QStringList &options,
    const QStringList &list)
{
    theDebuggerCore->remoteCommand(options, list);
}


DebuggerRunControl *DebuggerPlugin::createDebugger
    (const DebuggerStartParameters &sp, RunConfiguration *rc)
{
    return theDebuggerCore->createDebugger(sp, rc);
}

void DebuggerPlugin::startDebugger(RunControl *runControl)
{
    theDebuggerCore->startDebugger(runControl);
}

void DebuggerPlugin::extensionsInitialized()
{
    theDebuggerCore->extensionsInitialized();
}

bool DebuggerPlugin::isActiveDebugLanguage(int language)
{
    return theDebuggerCore->isActiveDebugLanguage(language);
}

DebuggerMainWindow *DebuggerPlugin::mainWindow()
{
    return theDebuggerCore->m_mainWindow;
}

QWidget *DebugMode::widget()
{
    if (!m_widget) {
        //qDebug() << "CREATING DEBUG MODE WIDGET";
        m_widget = theDebuggerCore->m_mainWindow->createContents(this);
        m_widget->setFocusProxy(EditorManager::instance());
    }
    return m_widget;
}

//////////////////////////////////////////////////////////////////////
//
// Testing
//
//////////////////////////////////////////////////////////////////////

/*
void DebuggerPlugin::runTest(const QString &fileName)
{
    DebuggerStartParameters sp;
    sp.executable = fileName;
    sp.processArgs = QStringList() << "--run-debuggee";
    sp.workingDirectory.clear();
    startDebugger(m_debuggerRunControlFactory->create(sp));
}
*/

} // namespace Debugger

#include "debuggerplugin.moc"

Q_EXPORT_PLUGIN(Debugger::DebuggerPlugin)
