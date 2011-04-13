/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "debuggerplugin.h"

#include "debuggerstartparameters.h"
#include "debuggeractions.h"
#include "debuggerconstants.h"
#include "debuggercore.h"
#include "debuggerdialogs.h"
#include "debuggerengine.h"
#include "debuggermainwindow.h"
#include "debuggerrunner.h"
#include "debuggerruncontrolfactory.h"
#include "debuggerstringutils.h"

#include "breakpoint.h"
#include "breakhandler.h"
#include "breakwindow.h"
#include "consolewindow.h"
#include "disassembleragent.h"
#include "logwindow.h"
#include "moduleswindow.h"
#include "moduleshandler.h"
#include "registerwindow.h"
#include "snapshotwindow.h"
#include "stackhandler.h"
#include "stackwindow.h"
#include "sourcefileswindow.h"
#include "threadswindow.h"
#include "watchhandler.h"
#include "watchwindow.h"
#include "watchutils.h"
#include "debuggertooltipmanager.h"

#include "snapshothandler.h"
#include "threadshandler.h"
#include "commonoptionspage.h"

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
#include <coreplugin/minisplitter.h>
#include <coreplugin/modemanager.h>

#include <cppeditor/cppeditorconstants.h>
#include <cplusplus/ModelManagerInterface.h>

#include <extensionsystem/pluginmanager.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <projectexplorer/abi.h>

#include <qt4projectmanager/qt4projectmanagerconstants.h>

#include <texteditor/basetexteditor.h>
#include <texteditor/fontsettings.h>
#include <texteditor/texteditorsettings.h>

#include <utils/qtcassert.h>
#include <utils/savedaction.h>
#include <utils/styledbar.h>
#include <utils/proxyaction.h>
#include <utils/statuslabel.h>

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

/*!
    \namespace Debugger
    Debugger plugin namespace
*/

/*!
    \namespace Debugger::Internal
    Internal namespace of the Debugger plugin
    \internal
*/

/*!
    \class Debugger::DebuggerEngine

    \brief Base class of a debugger engine.

    Note: the Debugger process itself and any helper processes like
    gdbserver, the trk client etc are referred to as 'Engine',
    whereas the debugged process is referred to as 'Inferior'.

    Transitions marked by '---' are done in the individual engines.
    Transitions marked by '+-+' are done in the base DebuggerEngine.
    Transitions marked by '*' are done asynchronously.

    The GdbEngine->setupEngine() function is described in more detail below.

    The engines are responsible for local roll-back to the last
    acknowledged state before calling notify*Failed. I.e. before calling
    notifyEngineSetupFailed() any process started during setupEngine()
    so far must be terminated.
    \code

                        DebuggerNotReady
                         progressmanager/progressmanager.cpp      +
                      EngineSetupRequested
                               +
                  (calls *Engine->setupEngine())
                            |      |
                            |      |
                       {notify-  {notify-
                        Engine-   Engine-
                        SetupOk}  SetupFailed}
                            +      +
                            +      `+-+-+> EngineSetupFailed
                            +                   +
                            +    [calls RunControl->startFailed]
                            +                   +
                            +             DebuggerFinished
                            v
                      EngineSetupOk
                            +
             [calls RunControl->StartSuccessful]
                            +
                  InferiorSetupRequested
                            +
             (calls *Engine->setupInferior())
                         |       |
                         |       |
                    {notify-   {notify-
                     Inferior- Inferior-
                     SetupOk}  SetupFailed}
                         +       +
                         +       ` +-+-> InferiorSetupFailed +-+-+-+-+-+->.
                         +                                                +
                  InferiorSetupOk                                         +
                         +                                                +
                  EngineRunRequested                                      +
                         +                                                +
                 (calls *Engine->runEngine())                             +
               /       |            |        \                            +
             /         |            |          \                          +
            | (core)   | (attach)   |           |                         +
            |          |            |           |                         +
      {notify-    {notifyER&- {notifyER&-  {notify-                       +
      Inferior-     Inferior-   Inferior-  EngineRun-                     +
     Unrunnable}     StopOk}     RunOk}     Failed}                       +
           +           +            +           +                         +
   InferiorUnrunnable  +     InferiorRunOk      +                         +
                       +                        +                         +
                InferiorStopOk            EngineRunFailed                 +
                                                +                         v
                                                 `-+-+-+-+-+-+-+-+-+-+-+>-+
                                                                          +
                                                                          +
                       #Interrupt@InferiorRunOk#                          +
                                  +                                       +
                          InferiorStopRequested                           +
  #SpontaneousStop                +                                       +
   @InferiorRunOk#         (calls *Engine->                               +
          +               interruptInferior())                            +
      {notify-               |          |                                 +
     Spontaneous-       {notify-    {notify-                              +
      Inferior-          Inferior-   Inferior-                            +
       StopOk}           StopOk}    StopFailed}                           +
           +              +             +                                 +
            +            +              +                                 +
            InferiorStopOk              +                                 +
                  +                     +                                 +
                  +                    +                                  +
                  +                   +                                   +
        #Stop@InferiorUnrunnable#    +                                    +
          #Creator Close Event#     +                                     +
                       +           +                                      +
                InferiorShutdownRequested                                 +
                            +                                             +
           (calls *Engine->shutdownInferior())                            +
                         |        |                                       +
                    {notify-   {notify-                                   +
                     Inferior- Inferior-                                  +
                  ShutdownOk}  ShutdownFailed}                            +
                         +        +                                       +
                         +        +                                       +
  #Inferior exited#      +        +                                       +
         |               +        +                                       +
   {notifyInferior-      +        +                                       +
      Exited}            +        +                                       +
           +             +        +                                       +
     InferiorExitOk      +        +                                       +
             +           +        +                                       +
            InferiorShutdownOk InferiorShutdownFailed                     +
                      *          *                                        +
                  EngineShutdownRequested                                 +
                            +                                             +
           (calls *Engine->shutdownEngine())  <+-+-+-+-+-+-+-+-+-+-+-+-+-+'
                         |        |
                         |        |
                    {notify-   {notify-
                     Engine-    Engine-
                  ShutdownOk}  ShutdownFailed}
                         +       +
            EngineShutdownOk  EngineShutdownFailed
                         *       *
                     DebuggerFinished

\endcode */

/* Here is a matching graph as a GraphViz graph. View it using
 * \code
grep "^sg1:" debuggerplugin.cpp | cut -c5- | dot -osg1.ps -Tps && gv sg1.ps

sg1: digraph DebuggerStates {
sg1:   DebuggerNotReady -> EngineSetupRequested
sg1:   EngineSetupRequested -> EngineSetupOk [ label="notifyEngineSetupOk", style="dashed" ];
sg1:   EngineSetupRequested -> EngineSetupFailed [ label= "notifyEngineSetupFailed", style="dashed"];
sg1:   EngineSetupFailed -> DebuggerFinished [ label= "RunControl::StartFailed" ];
sg1:   EngineSetupOk -> InferiorSetupRequested [ label= "RunControl::StartSuccessful" ];
sg1:   InferiorSetupRequested -> InferiorSetupOk [ label="notifyInferiorSetupOk", style="dashed" ];
sg1:   InferiorSetupRequested -> InferiorSetupFailed [ label="notifyInferiorFailed", style="dashed" ];
sg1:   InferiorSetupOk -> EngineRunRequested
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
sg1:   InferiorStopOk -> InferiorShutdownRequested [ label="Close event" ];
sg1:   InferiorUnrunnable -> InferiorShutdownRequested [ label="Close event" ];
sg1:   InferiorShutdownRequested -> InferiorShutdownOk [ label= "Engine::shutdownInferior\nnotifyInferiorShutdownOk", style="dashed" ];
sg1:   InferiorShutdownRequested -> InferiorShutdownFailed [ label="Engine::shutdownInferior\nnotifyInferiorShutdownFailed", style="dashed" ];
sg1:   InferiorExited -> InferiorExitOk [ label="notifyInferiorExited", style="dashed"];
sg1:   InferiorExitOk -> InferiorShutdownOk
sg1:   InferiorShutdownOk -> EngineShutdownRequested
sg1:   InferiorShutdownFailed -> EngineShutdownRequested
sg1:   EngineShutdownRequested -> EngineShutdownOk [ label="Engine::shutdownEngine\nnotifyEngineShutdownOk", style="dashed" ];
sg1:   EngineShutdownRequested -> EngineShutdownFailed  [ label="Engine::shutdownEngine\nnotifyEngineShutdownFailed", style="dashed" ];
sg1:   EngineShutdownOk -> DebuggerFinished  [ style = "dotted" ];
sg1:   EngineShutdownFailed  -> DebuggerFinished [ style = "dotted" ];
sg1: }
* \endcode */
// Additional signalling:    {notifyInferiorIll}   {notifyEngineIll}


/*!
    \class Debugger::Internal::GdbEngine
    \brief Implementation of Debugger::Engine driving a gdb executable.

    GdbEngine specific startup. All happens in EngineSetupRequested state:

    Transitions marked by '---' are done in the individual adapters.

    Transitions marked by '+-+' are done in the GdbEngine.

    \code
                  GdbEngine::setupEngine()
                          +
            (calls *Adapter->startAdapter())
                          |      |
                          |      `---> handleAdapterStartFailed()
                          |                   +
                          |             {notifyEngineSetupFailed}
                          |
                 handleAdapterStarted()
                          +
                 {notifyEngineSetupOk}



                GdbEngine::setupInferior()
                          +
            (calls *Adapter->prepareInferior())
                          |      |
                          |      `---> handlePrepareInferiorFailed()
                          |                   +
                          |             {notifyInferiorSetupFailed}
                          |
                handleInferiorPrepared()
                          +
                {notifyInferiorSetupOk}

\endcode */

using namespace Core;
using namespace Debugger::Constants;
using namespace ProjectExplorer;
using namespace TextEditor;

namespace CC = Core::Constants;
namespace PE = ProjectExplorer::Constants;


namespace Debugger {
namespace Constants {

const char * const M_DEBUG_START_DEBUGGING = "QtCreator.Menu.Debug.StartDebugging";

const char * const STARTEXTERNAL            = "Debugger.StartExternal";
const char * const ATTACHEXTERNAL           = "Debugger.AttachExternal";
const char * const ATTACHCORE               = "Debugger.AttachCore";
const char * const ATTACHREMOTE             = "Debugger.AttachRemote";
const char * const ATTACHREMOTECDB          = "Debugger.AttachRemoteCDB";
const char * const STARTREMOTELLDB          = "Debugger.StartRemoteLLDB";
const char * const DETACH                   = "Debugger.Detach";

const char * const RUN_TO_LINE              = "Debugger.RunToLine";
const char * const RUN_TO_SELECTED_FUNCTION = "Debugger.RunToSelectedFunction";
const char * const JUMP_TO_LINE             = "Debugger.JumpToLine";
const char * const RETURN_FROM_FUNCTION     = "Debugger.ReturnFromFunction";
const char * const SNAPSHOT                 = "Debugger.Snapshot";
const char * const TOGGLE_BREAK             = "Debugger.ToggleBreak";
const char * const BREAK_BY_FUNCTION        = "Debugger.BreakByFunction";
const char * const BREAK_AT_MAIN            = "Debugger.BreakAtMain";
// Don't add '1' to the string as it shows up in the shortcut dialog.
const char * const ADD_TO_WATCH1            = "Debugger.AddToWatch";
const char * const ADD_TO_WATCH2            = "Debugger.AddToWatch2";
const char * const FRAME_UP                 = "Debugger.FrameUp";
const char * const FRAME_DOWN               = "Debugger.FrameDown";

#ifdef Q_WS_MAC
const char * const STOP_KEY                     = "Shift+Ctrl+Y";
const char * const RESET_KEY                    = "Ctrl+Shift+F5";
const char * const STEP_KEY                     = "Ctrl+Shift+I";
const char * const STEPOUT_KEY                  = "Ctrl+Shift+T";
const char * const NEXT_KEY                     = "Ctrl+Shift+O";
const char * const REVERSE_KEY                  = "";
const char * const RUN_TO_LINE_KEY              = "Shift+F8";
const char * const RUN_TO_SELECTED_FUNCTION_KEY = "Ctrl+F6";
const char * const JUMP_TO_LINE_KEY             = "Ctrl+D,Ctrl+L";
const char * const TOGGLE_BREAK_KEY             = "F8";
const char * const BREAK_BY_FUNCTION_KEY        = "Ctrl+D,Ctrl+F";
const char * const BREAK_AT_MAIN_KEY            = "Ctrl+D,Ctrl+M";
const char * const ADD_TO_WATCH_KEY             = "Ctrl+D,Ctrl+W";
const char * const SNAPSHOT_KEY                 = "Ctrl+D,Ctrl+S";
#else
const char * const STOP_KEY                     = "Shift+F5";
const char * const RESET_KEY                    = "Ctrl+Shift+F5";
const char * const STEP_KEY                     = "F11";
const char * const STEPOUT_KEY                  = "Shift+F11";
const char * const NEXT_KEY                     = "F10";
const char * const REVERSE_KEY                  = "F12";
const char * const RUN_TO_LINE_KEY              = "";
const char * const RUN_TO_SELECTED_FUNCTION_KEY = "Ctrl+F6";
const char * const JUMP_TO_LINE_KEY             = "";
const char * const TOGGLE_BREAK_KEY             = "F9";
const char * const BREAK_BY_FUNCTION_KEY        = "";
const char * const BREAK_AT_MAIN_KEY            = "";
const char * const ADD_TO_WATCH_KEY             = "Ctrl+Alt+Q";
const char * const SNAPSHOT_KEY                 = "Ctrl+D,Ctrl+S";
#endif

} // namespace Constants


namespace Internal {

// FIXME: Outdated?
// The createCdbEngine function takes a list of options pages it can add to.
// This allows for having a "enabled" toggle on the page independently
// of the engine. That's good for not enabling the related ActiveX control
// unnecessarily.

void addCdbOptionPages(QList<IOptionsPage*> *opts);
void addGdbOptionPages(QList<IOptionsPage*> *opts);
void addScriptOptionPages(QList<IOptionsPage*> *opts);
void addTcfOptionPages(QList<IOptionsPage*> *opts);

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

static Abi abiOfBinary(const QString &fileName)
{
    QList<Abi> abis = Abi::abisOfBinary(fileName);
    if (abis.isEmpty())
        return Abi();
    return abis.at(0);
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
    unsigned debuggerCapabilities() const { return AddWatcherCapability; }
    bool acceptsBreakpoint(BreakpointId) const { return false; }
    bool acceptsDebuggerCommands() const { return false; }
};

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
// Misc
//
///////////////////////////////////////////////////////////////////////

static TextEditor::ITextEditor *currentTextEditor()
{
    if (const Core::EditorManager *editorManager = Core::EditorManager::instance())
            if (Core::IEditor *editor = editorManager->currentEditor())
                return qobject_cast<TextEditor::ITextEditor*>(editor);
    return 0;
}

static bool currentTextEditorPosition(ContextData *data)
{
    if (TextEditor::ITextEditor *textEditor = currentTextEditor()) {
        if (const Core::IFile *file = textEditor->file()) {
            data->fileName = file->fileName();
            data->lineNumber = textEditor->currentLine();
            return true;
        }
    }
    return false;
}

///////////////////////////////////////////////////////////////////////
//
// DebuggerPluginPrivate
//
///////////////////////////////////////////////////////////////////////

static DebuggerPluginPrivate *theDebuggerCore = 0;

/*!
    \class Debugger::Internal::DebuggerCore

    This is the "internal" interface of the debugger plugin that's
    used by debugger views and debugger engines. The interface is
    implemented in DebuggerPluginPrivate.
*/

/*!
    \class Debugger::Internal::DebuggerPluginPrivate

    Implementation of DebuggerCore.
*/

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
    DebuggerEngine *dummyEngine();

public slots:
    void writeSettings()
    {
        m_debuggerSettings->writeSettings();
        m_mainWindow->writeSettings();
    }

    void selectThread(int index)
    {
        currentEngine()->selectThread(index);
    }

    void breakpointSetMarginActionTriggered()
    {
        const QAction *action = qobject_cast<const QAction *>(sender());
        QTC_ASSERT(action, return);
        const ContextData data = action->data().value<ContextData>();
        if (data.address)
            toggleBreakpointByAddress(data.address);
        else
            toggleBreakpointByFileAndLine(data.fileName, data.lineNumber);
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
    void updateBreakMenuItem(Core::IEditor *editor);
    void setBusyCursor(bool busy);
    void requestMark(TextEditor::ITextEditor *editor, int lineNumber);
    void requestContextMenu(TextEditor::ITextEditor *editor,
        int lineNumber, QMenu *menu);

    void activatePreviousMode();
    void activateDebugMode();
    void toggleBreakpoint();
    void toggleBreakpointByFileAndLine(const QString &fileName, int lineNumber);
    void toggleBreakpointByAddress(quint64 address);
    void onModeChanged(Core::IMode *mode);
    void onCoreAboutToOpen();
    void showSettingsDialog();

    void debugProject();
    void startExternalApplication();
    void startRemoteCdbSession();
    void startRemoteApplication();
    void startRemoteEngine();
    void attachExternalApplication();
    void attachExternalApplication(qint64 pid, const QString &binary,
                                   const ProjectExplorer::Abi &abi = ProjectExplorer::Abi(),
                                   const QString &debuggerCommand = QString());
    void runScheduled();
    void attachCore();
    void attachCore(const QString &core, const QString &exeFileName,
                    const ProjectExplorer::Abi &abi = ProjectExplorer::Abi(),
                    const QString &debuggerCommand = QString());
    void attachRemote(const QString &spec);

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
    unsigned enabledEngines() const { return m_cmdLineEnabledEngines; }
    QString debuggerForAbi(const Abi &abi, DebuggerEngineType et = NoEngineType) const;
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

    void updateState(DebuggerEngine *engine);
    void updateWatchersWindow();
    void onCurrentProjectChanged(ProjectExplorer::Project *project);

    void sessionLoaded();
    void aboutToUnloadSession();
    void aboutToSaveSession();

    void executeDebuggerCommand(const QString &command);
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
        ContextData data;
        if (currentTextEditorPosition(&data))
            currentEngine()->executeJumpToLine(data);
    }

    void handleExecRunToLine()
    {
        //removeTooltip();
        currentEngine()->resetLocation();
        ContextData data;
        if (currentTextEditorPosition(&data))
            currentEngine()->executeRunToLine(data);
    }

    void handleExecRunToSelectedFunction()
    {
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

        if (functionName.isEmpty()) {
            showStatusMessage(tr("No function selected."));
        } else {
            showStatusMessage(tr("Running to function \"%1\".")
                .arg(functionName));
            currentEngine()->resetLocation();
            currentEngine()->executeRunToFunction(functionName);
        }
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
        const QAction *action = qobject_cast<const QAction *>(sender());
        QTC_ASSERT(action, return);
        const ContextData data = action->data().value<ContextData>();
        currentEngine()->executeRunToLine(data);
    }

    void slotJumpToLine()
    {
        const QAction *action = qobject_cast<const QAction *>(sender());
        QTC_ASSERT(action, return);
        const ContextData data = action->data().value<ContextData>();
        currentEngine()->executeJumpToLine(data);
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

    bool parseArgument(QStringList::const_iterator &it,
        const QStringList::const_iterator &cend,
        unsigned *enabledEngines, QString *errorMessage);
    bool parseArguments(const QStringList &args,
        unsigned *enabledEngines, QString *errorMessage);

    DebuggerToolTipManager *toolTipManager() const { return m_toolTipManager; }
    virtual QSharedPointer<GlobalDebuggerOptions> globalDebuggerOptions() const { return m_globalDebuggerOptions; }

public:
    DebuggerMainWindow *m_mainWindow;
    DebuggerRunControlFactory *m_debuggerRunControlFactory;

    QString m_previousMode;
    QList<DebuggerStartParameters> m_scheduledStarts;

    Utils::ProxyAction *m_visibleDebugAction;
    QAction *m_debugAction;
    QAction *m_startExternalAction;
    QAction *m_startRemoteAction;
    QAction *m_startRemoteCdbAction;
    QAction *m_startRemoteLldbAction;
    QAction *m_attachExternalAction;
    QAction *m_attachCoreAction;
    QAction *m_detachAction;
    QAction *m_continueAction;
    QAction *m_exitAction; // On application output button if "Stop" is possible
    QAction *m_interruptAction; // On the fat debug button if "Pause" is possible
    QAction *m_undisturbableAction; // On the fat debug button if nothing can be done
    QAction *m_resetAction;
    QAction *m_stepAction;
    QAction *m_stepOutAction;
    QAction *m_runToLineAction; // In the debug menu
    QAction *m_runToSelectedFunctionAction;
    QAction *m_jumpToLineAction; // In the Debug menu.
    QAction *m_returnFromFunctionAction;
    QAction *m_nextAction;
    QAction *m_watchAction1; // In the Debug menu.
    QAction *m_watchAction2; // In the text editor context menu.
    QAction *m_breakAction;
    QAction *m_reverseDirectionAction;
    QAction *m_frameUpAction;
    QAction *m_frameDownAction;

    QToolButton *m_reverseToolButton;

    QIcon m_startIcon;
    QIcon m_exitIcon;
    QIcon m_continueIcon;
    QIcon m_interruptIcon;
    QIcon m_locationMarkIcon;

    Utils::StatusLabel *m_statusLabel;
    QComboBox *m_threadBox;

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
    QStringList m_arguments;
    DebuggerToolTipManager *m_toolTipManager;
    CommonOptionsPage *m_commonOptionsPage;
    DummyEngine *m_dummyEngine;
    const QSharedPointer<GlobalDebuggerOptions> m_globalDebuggerOptions;
};

DebuggerPluginPrivate::DebuggerPluginPrivate(DebuggerPlugin *plugin) :
    m_toolTipManager(new DebuggerToolTipManager(this)),
    m_dummyEngine(0),
    m_globalDebuggerOptions(new GlobalDebuggerOptions)
{
    qRegisterMetaType<WatchData>("WatchData");
    qRegisterMetaType<ContextData>("ContextData");
    qRegisterMetaType<DebuggerStartParameters>("DebuggerStartParameters");

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

    m_mainWindow = 0;
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
    m_detachAction = 0;

    m_commonOptionsPage = 0;
}

DebuggerPluginPrivate::~DebuggerPluginPrivate()
{
    delete m_debuggerSettings;
    m_debuggerSettings = 0;

    // Mainwindow will be deleted by debug mode.

    delete m_snapshotHandler;
    m_snapshotHandler = 0;

    delete m_breakHandler;
    m_breakHandler = 0;
}

DebuggerEngine *DebuggerPluginPrivate::dummyEngine()
{
    if (!m_dummyEngine) {
        m_dummyEngine = new DummyEngine;
        m_dummyEngine->setParent(this);
        m_dummyEngine->setObjectName(_("DummyEngine"));
    }
    return m_dummyEngine;
}

DebuggerCore *debuggerCore()
{
    return theDebuggerCore;
}

static QString msgParameterMissing(const QString &a)
{
    return DebuggerPlugin::tr("Option '%1' is missing the parameter.").arg(a);
}

bool DebuggerPluginPrivate::parseArgument(QStringList::const_iterator &it,
    const QStringList::const_iterator &cend,
    unsigned *enabledEngines, QString *errorMessage)
{
    const QString &option = *it;
    // '-debug <pid>'
    // '-debug <corefile>'
    // '-debug <server:port>@<exe>@<arch>'
    if (*it == _("-debug")) {
        ++it;
        if (it == cend) {
            *errorMessage = msgParameterMissing(*it);
            return false;
        }
        DebuggerStartParameters sp;
        qulonglong pid = it->toULongLong();
        QString remoteChannel = it->contains('@') ? it->section('@', 0, 0) : *it;
        uint port = 0;
        int pos = remoteChannel.indexOf(QLatin1Char(':'));
        if (pos != -1)
            port = remoteChannel.mid(pos + 1).toUInt();
        if (pid) {
            sp.startMode = AttachExternal;
            sp.attachPID = pid;
            sp.displayName = tr("Process %1").arg(sp.attachPID);
            sp.startMessage = tr("Attaching to local process %1.").arg(sp.attachPID);
            sp.toolChainAbi = Abi::hostAbi();
        } else if (port) {
            sp.startMode = AttachToRemote;
            sp.remoteChannel = remoteChannel;
            sp.executable = it->section('@', 1, 1);
            if (sp.remoteChannel.isEmpty()) {
                *errorMessage = DebuggerPlugin::tr("The parameter '%1' of option "
                    "'%2' does not match the pattern <server:port>@<executable>@<architecture>.")
                        .arg(*it, option);
                return false;
            }
            sp.remoteArchitecture = it->section('@', 2, 2);
            sp.displayName = tr("Remote: \"%1\"").arg(sp.remoteChannel);
            sp.startMessage = tr("Attaching to remote server %1.")
                .arg(sp.remoteChannel);
            sp.toolChainAbi = abiOfBinary(sp.executable);
        } else {
            sp.startMode = AttachCore;
            sp.coreFile = *it;
            sp.displayName = tr("Core file \"%1\"").arg(sp.coreFile);
            sp.startMessage = tr("Attaching to core file %1.").arg(sp.coreFile);
            sp.toolChainAbi = abiOfBinary(sp.coreFile);
        }
        m_scheduledStarts.append(sp);
        return true;
    }
    // -wincrashevent <event-handle>:<pid>. A handle used for
    // a handshake when attaching to a crashed Windows process.
    // This is created by $QTC/src/tools/qtcdebugger/main.cpp:
    // args << QLatin1String("-wincrashevent")
    //   << QString("%1:%2").arg(argWinCrashEvent).arg(argProcessId);
    if (*it == _("-wincrashevent")) {
        ++it;
        if (it == cend) {
            *errorMessage = msgParameterMissing(*it);
            return false;
        }
        DebuggerStartParameters sp;
        sp.startMode = AttachCrashedExternal;
        sp.crashParameter = it->section(':', 0, 0);
        sp.attachPID = it->section(':', 1, 1).toULongLong();
        sp.displayName = tr("Crashed process %1").arg(sp.attachPID);
        sp.startMessage = tr("Attaching to crashed process %1").arg(sp.attachPID);
        sp.toolChainAbi = Abi::hostAbi();
        if (!sp.attachPID) {
            *errorMessage = DebuggerPlugin::tr("The parameter '%1' of option '%2' "
                "does not match the pattern <handle>:<pid>.").arg(*it, option);
            return false;
        }
        m_scheduledStarts.append(sp);
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

bool DebuggerPluginPrivate::parseArguments(const QStringList &args,
   unsigned *enabledEngines, QString *errorMessage)
{
    const QStringList::const_iterator cend = args.constEnd();
    for (QStringList::const_iterator it = args.constBegin(); it != cend; ++it)
        if (!parseArgument(it, cend, enabledEngines, errorMessage))
            return false;
    if (Constants::Internal::debug)
        qDebug().nospace() << args << "engines=0x"
            << QString::number(*enabledEngines, 16) << '\n';
    return true;
}


bool DebuggerPluginPrivate::initialize(const QStringList &arguments,
    QString *errorMessage)
{
    Q_UNUSED(errorMessage);
    m_arguments = arguments;
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
    ActionManager *am = core->actionManager();
    m_interruptAction->setEnabled(false);
    m_continueAction->setEnabled(false);
    m_exitAction->setEnabled(false);
    m_debugAction->setEnabled(true);
    m_visibleDebugAction->setAction(am->command(Constants::DEBUG)->action());
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
    dlg.setAbiIndex(configValue(_("LastExternalAbiIndex")).toInt());

    if (dlg.exec() != QDialog::Accepted)
        return;

    setConfigValue(_("LastExternalExecutableFile"),
                   dlg.executableFile());
    setConfigValue(_("LastExternalExecutableArguments"),
                   dlg.executableArguments());
    setConfigValue(_("LastExternalWorkingDirectory"),
                   dlg.workingDirectory());
    setConfigValue(_("LastExternalAbiIndex"), QVariant(dlg.abiIndex()));

    sp.executable = dlg.executableFile();
    sp.startMode = StartExternal;
    sp.toolChainAbi = dlg.abi();
    sp.debuggerCommand = dlg.debuggerCommand();
    sp.workingDirectory = dlg.workingDirectory();
    sp.displayName = sp.executable;
    if (!dlg.executableArguments().isEmpty())
        sp.processArgs = dlg.executableArguments();
    // Fixme: 1 of 3 testing hacks.
    if (sp.processArgs.startsWith(__("@tcf@ ")) || sp.processArgs.startsWith(__("@sym@ ")))
        // Set up an ARM Symbian Abi
        sp.toolChainAbi = Abi(Abi::ArmArchitecture, Abi::SymbianOS, Abi::SymbianDeviceFlavor, Abi::ElfFormat, false);

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
    dlg.setAbiIndex(configValue(_("LastAttachExternalAbiIndex")).toInt());

    if (dlg.exec() != QDialog::Accepted)
        return;

    setConfigValue(_("LastAttachExternalAbiIndex"), QVariant(dlg.abiIndex()));
    attachExternalApplication(dlg.attachPID(), dlg.executable(), dlg.abi(), dlg.debuggerCommand());
}

void DebuggerPluginPrivate::attachExternalApplication(qint64 pid, const QString &binary,
                                                      const ProjectExplorer::Abi &abi,
                                                      const QString &debuggerCommand)
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
    sp.startMode = AttachExternal;
    sp.toolChainAbi = abi.isValid() ? abi : abiOfBinary(sp.executable);
    sp.debuggerCommand = debuggerCommand;
    if (DebuggerRunControl *rc = createDebugger(sp))
        startDebugger(rc);
}

void DebuggerPluginPrivate::attachCore()
{
    AttachCoreDialog dlg(mainWindow());
    dlg.setExecutableFile(configValue(_("LastExternalExecutableFile")).toString());
    dlg.setCoreFile(configValue(_("LastExternalCoreFile")).toString());
    dlg.setAbiIndex(configValue(_("LastExternalCoreAbiIndex")).toInt());

    if (dlg.exec() != QDialog::Accepted)
        return;

    setConfigValue(_("LastExternalExecutableFile"), dlg.executableFile());
    setConfigValue(_("LastExternalCoreFile"), dlg.coreFile());
    setConfigValue(_("LastExternalCoreAbiIndex"), QVariant(dlg.abiIndex()));
    attachCore(dlg.coreFile(), dlg.executableFile(), dlg.abi());
}

void DebuggerPluginPrivate::attachCore(const QString &core,
                                       const QString &exe,
                                       const ProjectExplorer::Abi &abi,
                                       const QString &debuggerCommand)
{
    DebuggerStartParameters sp;
    sp.executable = exe;
    sp.coreFile = core;
    sp.displayName = tr("Core file \"%1\"").arg(core);
    sp.startMode = AttachCore;
    sp.debuggerCommand = debuggerCommand;
    sp.toolChainAbi = abi.isValid() ? abi : abiOfBinary(sp.coreFile);
    if (DebuggerRunControl *rc = createDebugger(sp))
        startDebugger(rc);
}

void DebuggerPluginPrivate::attachRemote(const QString &spec)
{
    // spec is: server:port@executable@architecture
    DebuggerStartParameters sp;
    sp.remoteChannel = spec.section('@', 0, 0);
    sp.executable = spec.section('@', 1, 1);
    sp.remoteArchitecture = spec.section('@', 2, 2);
    sp.displayName = tr("Remote: \"%1\"").arg(sp.remoteChannel);
    sp.startMode = AttachToRemote;
    sp.toolChainAbi = abiOfBinary(sp.executable);
    if (DebuggerRunControl *rc = createDebugger(sp))
        startDebugger(rc);
}

void DebuggerPluginPrivate::startRemoteCdbSession()
{
    const QString connectionKey = _("CdbRemoteConnection");
    DebuggerStartParameters sp;
    Abi hostAbi = Abi::hostAbi();
    sp.toolChainAbi = ProjectExplorer::Abi(hostAbi.architecture(),
                                           ProjectExplorer::Abi::WindowsOS,
                                           ProjectExplorer::Abi::WindowsMsvc2010Flavor,
                                           ProjectExplorer::Abi::PEFormat,
                                           true);
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
        sp.toolChainAbi = ProjectExplorer::Abi();
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
    sp.connParams.userName = dlg.username();
    sp.connParams.password = dlg.password();

    sp.connParams.timeout = 5;
    sp.connParams.authenticationType = Utils::SshConnectionParameters::AuthenticationByPassword;
    sp.connParams.port = 22;
    sp.connParams.proxyType = Utils::SshConnectionParameters::NoProxy;

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
    m_reverseDirectionAction->setChecked(false);
    m_reverseDirectionAction->setEnabled(value.toBool());
}

void DebuggerPluginPrivate::runScheduled()
{
    foreach (const DebuggerStartParameters &sp, m_scheduledStarts) {
        RunControl *rc = createDebugger(sp);
        QTC_ASSERT(rc, qDebug() << "CANNOT CREATE RUN CONTROL"; continue);
        showStatusMessage(sp.startMessage);
        startDebugger(rc);
    }
}

void DebuggerPluginPrivate::editorOpened(IEditor *editor)
{
    if (!isEditorDebuggable(editor))
        return;
    ITextEditor *textEditor = qobject_cast<ITextEditor *>(editor);
    if (!textEditor)
        return;
    connect(textEditor,
        SIGNAL(markRequested(TextEditor::ITextEditor*,int)),
        SLOT(requestMark(TextEditor::ITextEditor*,int)));
    connect(textEditor,
        SIGNAL(markContextMenuRequested(TextEditor::ITextEditor*,int,QMenu*)),
        SLOT(requestContextMenu(TextEditor::ITextEditor*,int,QMenu*)));
}

void DebuggerPluginPrivate::updateBreakMenuItem(Core::IEditor *editor)
{
    ITextEditor *textEditor = qobject_cast<ITextEditor *>(editor);
    m_breakAction->setEnabled(textEditor != 0);
}

void DebuggerPluginPrivate::requestContextMenu(ITextEditor *editor,
    int lineNumber, QMenu *menu)
{
    if (!isEditorDebuggable(editor))
        return;

    ContextData args;
    args.lineNumber = lineNumber;
    bool contextUsable = true;

    BreakpointId id = BreakpointId();
    if (editor->property("DisassemblerView").toBool()) {
        args.fileName = editor->file()->fileName();
        QString line = editor->contents()
            .section('\n', lineNumber - 1, lineNumber - 1);
        BreakpointResponse needle;
        needle.type = BreakpointByAddress;
        needle.address = DisassemblerAgent::addressFromDisassemblyLine(line);
        args.address = needle.address;
        needle.lineNumber = -1;
        id = breakHandler()->findSimilarBreakpoint(needle);
        contextUsable = args.address != 0;
    } else {
        args.fileName = editor->file()->fileName();
        id = breakHandler()
            ->findBreakpointByFileAndLine(args.fileName, lineNumber);
    }

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
        const QString text = args.address
            ? tr("Set Breakpoint at 0x%1").arg(args.address, 0, 16)
            : tr("Set Breakpoint at line %1").arg(lineNumber);
        QAction *act = new QAction(text, menu);
        act->setData(QVariant::fromValue(args));
        act->setEnabled(contextUsable);
        connect(act, SIGNAL(triggered()),
            SLOT(breakpointSetMarginActionTriggered()));
        menu->addAction(act);
    }
    // Run to, jump to line below in stopped state.
    if (currentEngine()->state() == InferiorStopOk && contextUsable) {
        menu->addSeparator();
        const QString runText = args.address
            ? DebuggerEngine::tr("Run to Address 0x%1").arg(args.address, 0, 16)
            : DebuggerEngine::tr("Run to Line %1").arg(args.lineNumber);
        QAction *runToLineAction  = new QAction(runText, menu);
        runToLineAction->setData(QVariant::fromValue(args));
        connect(runToLineAction, SIGNAL(triggered()), SLOT(slotRunToLine()));
        menu->addAction(runToLineAction);
        if (currentEngine()->debuggerCapabilities() & JumpToLineCapability) {
            const QString jumpText = args.address
                ? DebuggerEngine::tr("Jump to Address 0x%1").arg(args.address, 0, 16)
                : DebuggerEngine::tr("Jump to Line %1").arg(args.lineNumber);
            QAction *jumpToLineAction  = new QAction(jumpText, menu);
            menu->addAction(runToLineAction);
            jumpToLineAction->setData(QVariant::fromValue(args));
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
    engine->watchHandler()->rebuildModel();
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
    m_reverseDirectionAction->setChecked(false);
    m_reverseDirectionAction->setEnabled(false);

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
    m_reverseDirectionAction->setChecked(false);
    m_reverseDirectionAction->setEnabled(false);
    m_toolTipManager->closeAllToolTips();

    m_startExternalAction->setEnabled(true);
    m_attachExternalAction->setEnabled(true);
#ifdef Q_OS_WIN
    m_attachCoreAction->setEnabled(false);
#else
    m_attachCoreAction->setEnabled(true);
#endif
    m_startRemoteAction->setEnabled(true);
    m_detachAction->setEnabled(false);

    m_watchAction1->setEnabled(true);
    m_watchAction2->setEnabled(true);
    m_breakAction->setEnabled(false);
    //m_snapshotAction->setEnabled(false);
    action(OperateByInstruction)->setEnabled(false);

    m_exitAction->setEnabled(false);
    m_resetAction->setEnabled(false);

    m_interruptAction->setEnabled(false);
    m_continueAction->setEnabled(false);

    m_stepAction->setEnabled(false);
    m_stepOutAction->setEnabled(false);
    m_runToLineAction->setEnabled(false);
    m_runToSelectedFunctionAction->setEnabled(true);
    m_returnFromFunctionAction->setEnabled(false);
    m_jumpToLineAction->setEnabled(false);
    m_nextAction->setEnabled(false);

    action(AutoDerefPointers)->setEnabled(true);
    action(ExpandStack)->setEnabled(false);

    m_scriptConsoleWindow->setEnabled(false);
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

    const DebuggerState state = engine->state();
    //showMessage(QString("PLUGIN SET STATE: ")
    //    + DebuggerEngine::stateName(state), LogStatus);
    //qDebug() << "PLUGIN SET STATE: " << state;

    static DebuggerState previousState = DebuggerNotReady;
    if (state == previousState)
        return;

    bool actionsEnabled = DebuggerEngine::debuggerActionsEnabled(state);

    ICore *core = ICore::instance();
    ActionManager *am = core->actionManager();
    if (state == DebuggerNotReady) {
        QTC_ASSERT(false, /* We use the Core m_debugAction here */);
        // F5 starts debugging. It is "startable".
        m_interruptAction->setEnabled(false);
        m_continueAction->setEnabled(false);
        m_exitAction->setEnabled(false);
        m_debugAction->setEnabled(true);
        m_visibleDebugAction->setAction(am->command(Constants::DEBUG)->action());
    } else if (state == InferiorStopOk) {
        // F5 continues, Shift-F5 kills. It is "continuable".
        m_interruptAction->setEnabled(false);
        m_continueAction->setEnabled(true);
        m_exitAction->setEnabled(true);
        m_debugAction->setEnabled(false);
        m_visibleDebugAction->setAction(am->command(Constants::CONTINUE)->action());
    } else if (state == InferiorRunOk) {
        // Shift-F5 interrupts. It is also "interruptible".
        m_interruptAction->setEnabled(true);
        m_continueAction->setEnabled(false);
        m_exitAction->setEnabled(false);
        m_debugAction->setEnabled(false);
        m_visibleDebugAction->setAction(am->command(Constants::INTERRUPT)->action());
    } else if (state == DebuggerFinished) {
        // We don't want to do anything anymore.
        m_interruptAction->setEnabled(false);
        m_continueAction->setEnabled(false);
        m_exitAction->setEnabled(false);
        m_debugAction->setEnabled(true);
        m_visibleDebugAction->setAction(am->command(Constants::DEBUG)->action());
        m_codeModelSnapshot = CPlusPlus::Snapshot();
        setBusyCursor(false);
        cleanupViews();
    } else if (state == InferiorUnrunnable) {
        // We don't want to do anything anymore.
        m_interruptAction->setEnabled(false);
        m_continueAction->setEnabled(false);
        m_exitAction->setEnabled(true);
        m_debugAction->setEnabled(false);
        m_visibleDebugAction->setAction(am->command(Constants::DEBUG)->action());
    } else {
        // Everything else is "undisturbable".
        m_interruptAction->setEnabled(false);
        m_continueAction->setEnabled(false);
        m_exitAction->setEnabled(false);
        m_debugAction->setEnabled(false);
        m_visibleDebugAction->setAction(m_undisturbableAction);
    }

    m_startExternalAction->setEnabled(true);
    m_attachExternalAction->setEnabled(true);
    m_attachCoreAction->setEnabled(true);
    m_startRemoteAction->setEnabled(true);

    const bool isCore = engine->startParameters().startMode == AttachCore;
    const bool stopped = state == InferiorStopOk;
    const bool detachable = stopped && !isCore;
    m_detachAction->setEnabled(detachable);

    if (stopped)
        QApplication::alert(mainWindow(), 3000);

    const uint caps = engine->debuggerCapabilities();
    const bool canReverse = (caps & ReverseSteppingCapability)
                && boolSetting(EnableReverseDebugging);
    m_reverseDirectionAction->setEnabled(canReverse);

    m_watchAction1->setEnabled(true);
    m_watchAction2->setEnabled(true);
    m_breakAction->setEnabled(true);

    action(OperateByInstruction)->setEnabled(stopped || isCore);

    m_resetAction->setEnabled(state != DebuggerNotReady
                                      && state != DebuggerFinished);

    m_stepAction->setEnabled(stopped);
    m_stepOutAction->setEnabled(stopped);
    m_runToLineAction->setEnabled(stopped);
    m_runToSelectedFunctionAction->setEnabled(stopped);
    m_returnFromFunctionAction->
        setEnabled(stopped && (caps & ReturnFromFunctionCapability));

    const bool canJump = stopped && (caps & JumpToLineCapability);
    m_jumpToLineAction->setEnabled(canJump);

    m_nextAction->setEnabled(stopped);

    const bool canDeref = actionsEnabled && (caps & AutoDerefPointersCapability);
    action(AutoDerefPointers)->setEnabled(canDeref);
    action(AutoDerefPointers)->setEnabled(true);
    action(ExpandStack)->setEnabled(actionsEnabled);

    const bool notbusy = state == InferiorStopOk
        || state == DebuggerNotReady
        || state == DebuggerFinished
        || state == InferiorUnrunnable;
    setBusyCursor(!notbusy);

    m_scriptConsoleWindow->setEnabled(stopped);
}

void DebuggerPluginPrivate::updateDebugActions()
{
    ProjectExplorerPlugin *pe = ProjectExplorerPlugin::instance();
    Project *project = pe->startupProject();
    m_debugAction->setEnabled(pe->canRun(project, Constants::DEBUGMODE));
}

void DebuggerPluginPrivate::onCoreAboutToOpen()
{
    m_mainWindow->onModeChanged(ModeManager::instance()->currentMode());
}

void DebuggerPluginPrivate::onModeChanged(IMode *mode)
{
     // FIXME: This one gets always called, even if switching between modes
     //        different then the debugger mode. E.g. Welcome and Help mode and
     //        also on shutdown.

    m_mainWindow->onModeChanged(mode);

    if (mode->id() != Constants::MODE_DEBUG) {
        m_toolTipManager->leavingDebugMode();
        return;
    }

    EditorManager *editorManager = EditorManager::instance();
    if (editorManager->currentEditor())
        editorManager->currentEditor()->widget()->setFocus();
    m_toolTipManager->debugModeEntered();
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

/*! Activates the previous mode when the current mode is the debug mode. */
void DebuggerPluginPrivate::activatePreviousMode()
{
    ModeManager *modeManager = ModeManager::instance();
    if (modeManager->currentMode() == modeManager->mode(MODE_DEBUG)
            && !m_previousMode.isEmpty()) {
        modeManager->activateMode(m_previousMode);
        m_previousMode.clear();
    }
}

void DebuggerPluginPrivate::activateDebugMode()
{
    m_reverseDirectionAction->setChecked(false);
    m_reverseDirectionAction->setEnabled(false);
    ModeManager *modeManager = ModeManager::instance();
    m_previousMode = modeManager->currentMode()->id();
    modeManager->activateMode(_(MODE_DEBUG));
}

void DebuggerPluginPrivate::sessionLoaded()
{
    m_breakHandler->loadSessionData();
    dummyEngine()->watchHandler()->loadSessionData();
    m_toolTipManager->loadSessionData();
}

void DebuggerPluginPrivate::aboutToUnloadSession()
{
    m_breakHandler->removeSessionData();
    m_toolTipManager->sessionAboutToChange();
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
    dummyEngine()->watchHandler()->saveSessionData();
    m_toolTipManager->saveSessionData();
    m_breakHandler->saveSessionData();
}

void DebuggerPluginPrivate::executeDebuggerCommand(const QString &command)
{
    if (currentEngine()->acceptsDebuggerCommands())
        currentEngine()->executeDebuggerCommand(command);
    else
        showStatusMessage(tr("User commands are not accepted in the current state."));
}

void DebuggerPluginPrivate::showStatusMessage(const QString &msg0, int timeout)
{
    showMessage(msg0, LogStatus);
    QString msg = msg0;
    msg.remove(QLatin1Char('\n'));
    m_statusLabel->showStatusMessage(msg, timeout);
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
    if (m_codeModelSnapshot.isEmpty() && action(UseCodeModel)->isChecked())
        m_codeModelSnapshot = CPlusPlus::CppModelManagerInterface::instance()->snapshot();
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

static QString formatStartParameters(DebuggerStartParameters &sp)
{
    QString rc;
    QTextStream str(&rc);
    str << "Start parameters: '" << sp.displayName << "' mode: " << sp.startMode
        << "\nABI: " << sp.toolChainAbi.toString() << '\n';
    if (!sp.executable.isEmpty()) {
        str << "Executable: " << QDir::toNativeSeparators(sp.executable) << ' ' << sp.processArgs;
        if (sp.useTerminal)
            str << " [terminal]";
        str << '\n';
        if (!sp.workingDirectory.isEmpty())
            str << "Directory: " << QDir::toNativeSeparators(sp.workingDirectory) << '\n';
        if (sp.executableUid) {
            str << "UID: 0x";
            str.setIntegerBase(16);
            str << sp.executableUid << '\n';
            str.setIntegerBase(10);
        }
    }
    if (!sp.debuggerCommand.isEmpty())
        str << "Debugger: " << QDir::toNativeSeparators(sp.debuggerCommand) << '\n';
    if (!sp.coreFile.isEmpty())
        str << "Core: " << QDir::toNativeSeparators(sp.coreFile) << '\n';
    if (sp.attachPID > 0)
        str << "PID: " << sp.attachPID << ' ' << sp.crashParameter << '\n';
    if (!sp.projectDir.isEmpty()) {
        str << "Project: " << QDir::toNativeSeparators(sp.projectDir);
        if (!sp.projectBuildDir.isEmpty())
            str << " (built: " << QDir::toNativeSeparators(sp.projectBuildDir) << ')';
        str << '\n';
    }
    if (!sp.qtInstallPath.isEmpty())
        str << "Qt: " << QDir::toNativeSeparators(sp.qtInstallPath) << '\n';
    if (!sp.qmlServerAddress.isEmpty())
        str << "QML server: " << sp.qmlServerAddress << ':' << sp.qmlServerPort << '\n';
    if (!sp.remoteChannel.isEmpty()) {
        str << "Remote: " << sp.remoteChannel << ", " << sp.remoteArchitecture << '\n';
        if (!sp.remoteDumperLib.isEmpty())
            str << "Remote dumpers: " << sp.remoteDumperLib << '\n';
        if (!sp.remoteSourcesDir.isEmpty())
            str << "Remote sources: " << sp.remoteSourcesDir << '\n';
        if (!sp.remoteMountPoint.isEmpty())
            str << "Remote mount point: " << sp.remoteMountPoint << " Local: " << sp.localMountDir << '\n';
    }
    if (!sp.gnuTarget.isEmpty())
        str << "Gnu target: " << sp.gnuTarget << '\n';
    if (!sp.sysRoot.isEmpty())
        str << "Sysroot: " << sp.sysRoot << '\n';
    if (!sp.symbolFileName.isEmpty())
        str << "Symbol file: " << sp.symbolFileName << '\n';
    if (sp.useServerStartScript)
        str << "Using server start script: " << sp.serverStartScript;
    if (!sp.dumperLibrary.isEmpty()) {
        str << "Dumper libraries: " << QDir::toNativeSeparators(sp.dumperLibrary);
        foreach (const QString &dl, sp.dumperLibraryLocations)
            str << ' ' << QDir::toNativeSeparators(dl);
        str << '\n';
    }
    return rc;
}

void DebuggerPluginPrivate::runControlStarted(DebuggerEngine *engine)
{
    activateDebugMode();
    const QString message = tr("Starting debugger '%1' for ABI '%2'...")
            .arg(engine->objectName())
            .arg(engine->startParameters().toolChainAbi.toString());
    showMessage(message, StatusBar);
    showMessage(formatStartParameters(engine->startParameters()), LogDebug);
    showMessage(m_debuggerSettings->dump(), LogDebug);
    m_snapshotHandler->appendSnapshot(engine);
    connectEngine(engine);
}

void DebuggerPluginPrivate::runControlFinished(DebuggerEngine *engine)
{
    showStatusMessage(tr("Debugger finished."));
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
    action(OperateByInstruction)->setChecked(false);
}

void DebuggerPluginPrivate::remoteCommand(const QStringList &options,
    const QStringList &)
{
    if (options.isEmpty())
        return;

    unsigned enabledEngines = 0;
    QString errorMessage;

    if (!parseArguments(options, &enabledEngines, &errorMessage)) {
        qWarning("%s", qPrintable(errorMessage));
        return;
    }
    runScheduled();
}

QString DebuggerPluginPrivate::debuggerForAbi(const Abi &abi, DebuggerEngineType et) const
{
    enum { debug = 0 };
    QList<Abi> searchAbis;
    searchAbis.push_back(abi);
    // Pick the right tool chain in case cdb/gdb were started with other tool chains.
    // Also, lldb should be preferred over gdb.
    if (abi.os() == ProjectExplorer::Abi::WindowsOS) {
        switch (et) {
        case CdbEngineType:
            searchAbis.clear();
            searchAbis.push_back(Abi(abi.architecture(), abi.os(), Abi::WindowsMsvc2010Flavor,
                                     abi.binaryFormat(), abi.wordWidth()));
            searchAbis.push_back(Abi(abi.architecture(), abi.os(), Abi::WindowsMsvc2008Flavor,
                                     abi.binaryFormat(), abi.wordWidth()));
            searchAbis.push_back(Abi(abi.architecture(), abi.os(), Abi::WindowsMsvc2005Flavor,
                                     abi.binaryFormat(), abi.wordWidth()));
            break;
        case GdbEngineType:
            searchAbis.clear();
            searchAbis.push_back(Abi(abi.architecture(), abi.os(), Abi::WindowsMSysFlavor,
                                     abi.binaryFormat(), abi.wordWidth()));
            break;
        default:
            break;
        }
    }
    if (debug)
        qDebug() << "debuggerForAbi" << abi.toString() << searchAbis.size()
                 << searchAbis.front().toString() << et;

    foreach (const Abi &searchAbi, searchAbis) {
        const QList<ToolChain *> toolchains = ToolChainManager::instance()->findToolChains(searchAbi);
        // Find manually configured ones first
        for (int i = toolchains.size() - 1; i >= 0; i--) {
            const QString debugger = toolchains.at(i)->debuggerCommand();
            if (debug)
                qDebug() << i << toolchains.at(i)->displayName() << debugger;
            if (!debugger.isEmpty())
                return debugger;
        }
    }
    return QString();
}

DebuggerLanguages DebuggerPluginPrivate::activeLanguages() const
{
    QTC_ASSERT(m_mainWindow, return AnyLanguage);
    return m_mainWindow->activeDebugLanguages();
}

bool DebuggerPluginPrivate::isReverseDebugging() const
{
    return m_reverseDirectionAction->isChecked();
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

    m_statusLabel = new Utils::StatusLabel;

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

    act = m_continueAction = new QAction(tr("Continue"), this);
    act->setIcon(m_continueIcon);
    connect(act, SIGNAL(triggered()), SLOT(handleExecContinue()));

    act = m_exitAction = new QAction(tr("Exit Debugger"), this);
    act->setIcon(m_exitIcon);
    connect(act, SIGNAL(triggered()), SLOT(handleExecExit()));

    act = m_interruptAction = new QAction(tr("Interrupt"), this);
    act->setIcon(m_interruptIcon);
    connect(act, SIGNAL(triggered()), SLOT(handleExecInterrupt()));

    // A "disabled pause" seems to be a good choice.
    act = m_undisturbableAction = new QAction(tr("Debugger is Busy"), this);
    act->setIcon(m_interruptIcon);
    act->setEnabled(false);

    act = m_resetAction = new QAction(tr("Abort Debugging"), this);
    act->setToolTip(tr("Aborts debugging and "
        "resets the debugger to the initial state."));
    connect(act, SIGNAL(triggered()), SLOT(handleExecReset()));

    act = m_nextAction = new QAction(tr("Step Over"), this);
    act->setIcon(QIcon(__(":/debugger/images/debugger_stepover_small.png")));
    connect(act, SIGNAL(triggered()), SLOT(handleExecNext()));

    act = m_stepAction = new QAction(tr("Step Into"), this);
    act->setIcon(QIcon(__(":/debugger/images/debugger_stepinto_small.png")));
    connect(act, SIGNAL(triggered()), SLOT(handleExecStep()));

    act = m_stepOutAction = new QAction(tr("Step Out"), this);
    act->setIcon(QIcon(__(":/debugger/images/debugger_stepout_small.png")));
    connect(act, SIGNAL(triggered()), SLOT(handleExecStepOut()));

    act = m_runToLineAction = new QAction(tr("Run to Line"), this);
    connect(act, SIGNAL(triggered()), SLOT(handleExecRunToLine()));

    act = m_runToSelectedFunctionAction =
        new QAction(tr("Run to Selected Function"), this);
    connect(act, SIGNAL(triggered()), SLOT(handleExecRunToSelectedFunction()));

    act = m_returnFromFunctionAction =
        new QAction(tr("Immediately Return From Inner Function"), this);
    connect(act, SIGNAL(triggered()), SLOT(handleExecReturn()));

    act = m_jumpToLineAction = new QAction(tr("Jump to Line"), this);
    connect(act, SIGNAL(triggered()), SLOT(handleExecJumpToLine()));

    act = m_breakAction = new QAction(tr("Toggle Breakpoint"), this);

    act = m_watchAction1 = new QAction(tr("Add to Watch Window"), this);
    connect(act, SIGNAL(triggered()), SLOT(handleAddToWatchWindow()));

    act = m_watchAction2 = new QAction(tr("Add to Watch Window"), this);
    connect(act, SIGNAL(triggered()), SLOT(handleAddToWatchWindow()));

    //m_snapshotAction = new QAction(tr("Create Snapshot"), this);
    //m_snapshotAction->setProperty(Role, RequestCreateSnapshotRole);
    //m_snapshotAction->setIcon(
    //    QIcon(__(":/debugger/images/debugger_snapshot_small.png")));

    act = m_reverseDirectionAction =
        new QAction(tr("Reverse Direction"), this);
    act->setCheckable(true);
    act->setChecked(false);
    act->setCheckable(false);
    act->setIcon(QIcon(__(":/debugger/images/debugger_reversemode_16.png")));
    act->setIconVisibleInMenu(false);

    act = m_frameDownAction = new QAction(tr("Move to Called Frame"), this);
    connect(act, SIGNAL(triggered()), SLOT(handleFrameDown()));

    act = m_frameUpAction = new QAction(tr("Move to Calling Frame"), this);
    connect(act, SIGNAL(triggered()), SLOT(handleFrameUp()));

    connect(action(OperateByInstruction), SIGNAL(triggered(bool)),
        SLOT(handleOperateByInstructionTriggered(bool)));

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

    m_commonOptionsPage = new CommonOptionsPage(m_globalDebuggerOptions);
    m_plugin->addAutoReleasedObject(m_commonOptionsPage);

    m_debuggerSettings->readSettings();

    // Do not fail to load the whole plugin if something goes wrong here.
    QString errorMessage;
    if (!parseArguments(m_arguments, &m_cmdLineEnabledEngines, &errorMessage)) {
        errorMessage = tr("Error evaluating command line arguments: %1")
            .arg(errorMessage);
        qWarning("%s\n", qPrintable(errorMessage));
    }

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

#ifdef WITH_LLDB
    act = m_startRemoteLldbAction = new QAction(this);
    act->setText(tr("Start and Debug External Application with External Engine..."));
    connect(act, SIGNAL(triggered()), SLOT(startRemoteEngine()));
#endif

    act = m_attachExternalAction = new QAction(this);
    act->setText(tr("Attach to Running External Application..."));
    connect(act, SIGNAL(triggered()), SLOT(attachExternalApplication()));

    act = m_attachCoreAction = new QAction(this);
    act->setText(tr("Attach to Core..."));
    connect(act, SIGNAL(triggered()), SLOT(attachCore()));

    act = m_startRemoteAction = new QAction(this);
    act->setText(tr("Start and Attach to Remote Application..."));
    connect(act, SIGNAL(triggered()), SLOT(startRemoteApplication()));

#ifdef Q_OS_WIN
    m_startRemoteCdbAction = new QAction(tr("Attach to Remote CDB Session..."), this);
    connect(m_startRemoteCdbAction, SIGNAL(triggered()),
        SLOT(startRemoteCdbSession()));
#endif

    act = m_detachAction = new QAction(this);
    act->setText(tr("Detach Debugger"));
    connect(act, SIGNAL(triggered()), SLOT(handleExecDetach()));

    Command *cmd = 0;
    ActionContainer *mstart = am->actionContainer(PE::M_DEBUG_STARTDEBUGGING);

    cmd = am->registerAction(m_debugAction, Constants::DEBUG, globalcontext);
    cmd->setDefaultText(tr("Start Debugging"));
    cmd->setDefaultKeySequence(QKeySequence(Constants::DEBUG_KEY));
    mstart->addAction(cmd, Core::Constants::G_DEFAULT_ONE);

    m_visibleDebugAction = new Utils::ProxyAction(this);
    m_visibleDebugAction->initialize(m_debugAction);
    m_visibleDebugAction->setAttribute(Utils::ProxyAction::UpdateText);
    m_visibleDebugAction->setAttribute(Utils::ProxyAction::UpdateIcon);
    m_visibleDebugAction->setAction(cmd->action());

    ModeManager *modeManager = ModeManager::instance();
    modeManager->addAction(m_visibleDebugAction, Constants::P_ACTION_DEBUG);

    cmd = am->registerAction(m_startExternalAction,
        Constants::STARTEXTERNAL, globalcontext);
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

    cmd = am->registerAction(m_startRemoteAction,
        Constants::ATTACHREMOTE, globalcontext);
    cmd->setAttribute(Command::CA_Hide);
    mstart->addAction(cmd, CC::G_DEFAULT_ONE);

#ifdef WITH_LLDB
    cmd = am->registerAction(m_startRemoteLldbAction,
        Constants::STARTREMOTELLDB, globalcontext);
    cmd->setAttribute(Command::CA_Hide);
    mstart->addAction(cmd, CC::G_DEFAULT_ONE);
#endif

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

    cmd = am->registerAction(m_interruptAction,
        Constants::INTERRUPT, globalcontext);
    cmd->setDefaultText(tr("Interrupt Debugger"));
    cmd->setDefaultKeySequence(QKeySequence(Constants::STOP_KEY));
    debugMenu->addAction(cmd, CC::G_DEFAULT_ONE);

    cmd = am->registerAction(m_continueAction,
        Constants::CONTINUE, globalcontext);
    cmd->setDefaultKeySequence(QKeySequence(Constants::DEBUG_KEY));
    debugMenu->addAction(cmd, CC::G_DEFAULT_ONE);

    cmd = am->registerAction(m_exitAction,
        Constants::STOP, globalcontext);
    cmd->setDefaultKeySequence(QKeySequence(Constants::STOP_KEY));
    cmd->setDefaultText(tr("Stop Debugger"));
    debugMenu->addAction(cmd, CC::G_DEFAULT_ONE);

    cmd = am->registerAction(m_resetAction,
        Constants::RESET, globalcontext);
    //cmd->setDefaultKeySequence(QKeySequence(Constants::RESET_KEY));
    cmd->setDefaultText(tr("Reset Debugger"));
    debugMenu->addAction(cmd, CC::G_DEFAULT_ONE);

    QAction *sep = new QAction(this);
    sep->setSeparator(true);
    cmd = am->registerAction(sep, _("Debugger.Sep.Step"), globalcontext);
    debugMenu->addAction(cmd);

    cmd = am->registerAction(m_nextAction,
        Constants::NEXT, cppDebuggercontext);
    cmd->setDefaultKeySequence(QKeySequence(Constants::NEXT_KEY));
    cmd->setAttribute(Command::CA_Hide);
    debugMenu->addAction(cmd);

    cmd = am->registerAction(m_stepAction,
        Constants::STEP, cppDebuggercontext);
    cmd->setDefaultKeySequence(QKeySequence(Constants::STEP_KEY));
    cmd->setAttribute(Command::CA_Hide);
    debugMenu->addAction(cmd);

    cmd = am->registerAction(m_stepOutAction,
        Constants::STEPOUT, cppDebuggercontext);
    cmd->setDefaultKeySequence(QKeySequence(Constants::STEPOUT_KEY));
    cmd->setAttribute(Command::CA_Hide);
    debugMenu->addAction(cmd);

    cmd = am->registerAction(m_runToLineAction,
        Constants::RUN_TO_LINE, cppDebuggercontext);
    cmd->setDefaultKeySequence(QKeySequence(Constants::RUN_TO_LINE_KEY));
    cmd->setAttribute(Command::CA_Hide);
    debugMenu->addAction(cmd);

    cmd = am->registerAction(m_runToSelectedFunctionAction,
        Constants::RUN_TO_SELECTED_FUNCTION, cppDebuggercontext);
    cmd->setDefaultKeySequence(QKeySequence(
        Constants::RUN_TO_SELECTED_FUNCTION_KEY));
    cmd->setAttribute(Command::CA_Hide);
    // Don't add to menu by default as keeping its enabled state
    // and text up-to-date is a lot of hassle.
    // debugMenu->addAction(cmd);

    cmd = am->registerAction(m_jumpToLineAction,
        Constants::JUMP_TO_LINE, cppDebuggercontext);
    cmd->setAttribute(Command::CA_Hide);
    debugMenu->addAction(cmd);

    cmd = am->registerAction(m_returnFromFunctionAction,
        Constants::RETURN_FROM_FUNCTION, cppDebuggercontext);
    cmd->setAttribute(Command::CA_Hide);
    debugMenu->addAction(cmd);

    cmd = am->registerAction(m_reverseDirectionAction,
        Constants::REVERSE, cppDebuggercontext);
    cmd->setDefaultKeySequence(QKeySequence(Constants::REVERSE_KEY));
    cmd->setAttribute(Command::CA_Hide);
    debugMenu->addAction(cmd);

    sep = new QAction(this);
    sep->setSeparator(true);
    cmd = am->registerAction(sep, _("Debugger.Sep.Break"), globalcontext);
    debugMenu->addAction(cmd);

    //cmd = am->registerAction(m_snapshotAction,
    //    Constants::SNAPSHOT, cppDebuggercontext);
    //cmd->setDefaultKeySequence(QKeySequence(Constants::SNAPSHOT_KEY));
    //cmd->setAttribute(Command::CA_Hide);
    //debugMenu->addAction(cmd);

    cmd = am->registerAction(m_frameDownAction,
        Constants::FRAME_DOWN, cppDebuggercontext);
    cmd = am->registerAction(m_frameUpAction,
        Constants::FRAME_UP, cppDebuggercontext);

    cmd = am->registerAction(action(OperateByInstruction),
        Constants::OPERATE_BY_INSTRUCTION, cppDebuggercontext);
    cmd->setAttribute(Command::CA_Hide);
    debugMenu->addAction(cmd);

    cmd = am->registerAction(m_breakAction,
        Constants::TOGGLE_BREAK, globalcontext);
    cmd->setDefaultKeySequence(QKeySequence(Constants::TOGGLE_BREAK_KEY));
    debugMenu->addAction(cmd);
    connect(m_breakAction, SIGNAL(triggered()),
        SLOT(toggleBreakpoint()));

    sep = new QAction(this);
    sep->setSeparator(true);
    cmd = am->registerAction(sep, _("Debugger.Sep.Watch"), globalcontext);
    debugMenu->addAction(cmd);

    cmd = am->registerAction(m_watchAction1,
        Constants::ADD_TO_WATCH1, cppeditorcontext);
    //cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+D,Ctrl+W")));
    debugMenu->addAction(cmd);

    // If the CppEditor plugin is there, we want to add something to
    // the editor context menu.
    if (ActionContainer *editorContextMenu =
            am->actionContainer(CppEditor::Constants::M_CONTEXT)) {
        cmd = am->registerAction(sep, _("Debugger.Sep.Views"),
            cppDebuggercontext);
        editorContextMenu->addAction(cmd);
        cmd->setAttribute(Command::CA_Hide);

        cmd = am->registerAction(m_watchAction2,
            Constants::ADD_TO_WATCH2, cppDebuggercontext);
        cmd->action()->setEnabled(true);
        editorContextMenu->addAction(cmd);
        cmd->setAttribute(Command::CA_Hide);
        cmd->setAttribute(Command::CA_NonConfigurable); // ADD_TO_WATCH1 is enough.
    }

    QList<Core::IOptionsPage *> engineOptionPages;
    if (m_cmdLineEnabledEngines & GdbEngineType)
        addGdbOptionPages(&engineOptionPages);
   addCdbOptionPages(&engineOptionPages);
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
    connect(ICore::instance(), SIGNAL(coreAboutToOpen()),
        SLOT(onCoreAboutToOpen()));


    // Debug mode setup
    m_plugin->addAutoReleasedObject(new DebugMode);

    //
    //  Connections
    //

    // Core
    connect(core, SIGNAL(saveSettingsRequested()), SLOT(writeSettings()));

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
    connect(editorManager, SIGNAL(editorOpened(Core::IEditor*)),
        SLOT(editorOpened(Core::IEditor*)));
    connect(editorManager, SIGNAL(currentEditorChanged(Core::IEditor*)),
            SLOT(updateBreakMenuItem(Core::IEditor*)));

    // Application interaction
    connect(action(SettingsDialog), SIGNAL(triggered()),
        SLOT(showSettingsDialog()));

    // Toolbar
    QWidget *toolbarContainer = new QWidget;

    QHBoxLayout *hbox = new QHBoxLayout(toolbarContainer);
    hbox->setMargin(0);
    hbox->setSpacing(0);
    hbox->addWidget(toolButton(m_visibleDebugAction));
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

    m_mainWindow->setToolBar(CppLanguage, toolbarContainer);

    connect(action(EnableReverseDebugging),
        SIGNAL(valueChanged(QVariant)),
        SLOT(enableReverseDebuggingTriggered(QVariant)));

    setInitialState();
    connectEngine(0);

    connect(sessionManager(),
        SIGNAL(startupProjectChanged(ProjectExplorer::Project*)),
        SLOT(onCurrentProjectChanged(ProjectExplorer::Project*)));

    QTC_ASSERT(m_coreSettings, /**/);
    m_globalDebuggerOptions->fromSettings(m_coreSettings);
    m_watchersWindow->setVisible(false);
    m_returnWindow->setVisible(false);

    // time gdb -i mi -ex 'b debuggerplugin.cpp:800' -ex r -ex q bin/qtcreator.bin
    if (!m_scheduledStarts.isEmpty())
        QTimer::singleShot(0, this, SLOT(runScheduled()));
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
}

} // namespace Internal


///////////////////////////////////////////////////////////////////////
//
// DebuggerPlugin
//
///////////////////////////////////////////////////////////////////////

using namespace Debugger::Internal;

/*!
    \class Debugger::DebuggerPlugin

    This is the "external" interface of the debugger plugin that's visible
    from Qt Creator core. The internal interface to global debugger
    functionality that is used by debugger views and debugger engines
    is DebuggerCore, implemented in DebuggerPluginPrivate. */

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

QAction *DebuggerPlugin::visibleDebugAction()
{
    return theDebuggerCore->m_visibleDebugAction;
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

} // namespace Debugger

#include "debuggerplugin.moc"

Q_EXPORT_PLUGIN(Debugger::DebuggerPlugin)
