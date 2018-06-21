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

#include "debuggerplugin.h"

#include "debuggermainwindow.h"
#include "debuggeractions.h"
#include "debuggerinternalconstants.h"
#include "debuggercore.h"
#include "debuggerkitconfigwidget.h"
#include "debuggerdialogs.h"
#include "debuggerengine.h"
#include "debuggericons.h"
#include "debuggeritem.h"
#include "debuggeritemmanager.h"
#include "debuggermainwindow.h"
#include "debuggerrunconfigurationaspect.h"
#include "debuggerruncontrol.h"
#include "debuggerkitinformation.h"
#include "memoryagent.h"
#include "breakhandler.h"
#include "disassemblerlines.h"
#include "logwindow.h"
#include "moduleshandler.h"
#include "snapshotwindow.h"
#include "stackhandler.h"
#include "stackwindow.h"
#include "watchhandler.h"
#include "watchwindow.h"
#include "watchutils.h"
#include "unstartedappwatcherdialog.h"
#include "debuggertooltipmanager.h"
#include "localsandexpressionswindow.h"
#include "loadcoredialog.h"
#include "sourceutils.h"
#include "shared/hostutils.h"
#include "console/console.h"

#include "snapshothandler.h"
#include "threadshandler.h"
#include "commonoptionspage.h"

#include "analyzer/analyzerconstants.h"
#include "analyzer/analyzermanager.h"

#include <app/app_version.h>

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/find/itemviewfind.h>
#include <coreplugin/icore.h>
#include <coreplugin/imode.h>
#include <coreplugin/messagebox.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/navigationwidget.h>
#include <coreplugin/outputpane.h>
#include <coreplugin/rightpane.h>

#include <extensionsystem/pluginmanager.h>

#include <cppeditor/cppeditorconstants.h>
#include <qmljseditor/qmljseditorconstants.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/devicesupport/deviceprocessesdialog.h>
#include <projectexplorer/devicesupport/deviceprocesslist.h>
#include <projectexplorer/itaskhandler.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/projectexplorersettings.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/toolchain.h>
#include <ssh/sshconnection.h>

#include <texteditor/texteditor.h>
#include <texteditor/textdocument.h>
#include <texteditor/fontsettings.h>
#include <texteditor/texteditorsettings.h>

#include <utils/algorithm.h>
#include <utils/appmainwindow.h>
#include <utils/basetreeview.h>
#include <utils/checkablemessagebox.h>
#include <utils/fancymainwindow.h>
#include <utils/hostosinfo.h>
#include <utils/proxyaction.h>
#include <utils/qtcassert.h>
#include <utils/savedaction.h>
#include <utils/statuslabel.h>
#include <utils/styledbar.h>
#include <utils/temporarydirectory.h>
#include <utils/utilsicons.h>
#include <utils/winutils.h>

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QPointer>
#include <QPushButton>
#include <QSettings>
#include <QStackedWidget>
#include <QTextBlock>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QVariant>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtPlugin>

#ifdef WITH_TESTS

#include <cpptools/cpptoolstestcase.h>
#include <cpptools/projectinfo.h>

#include <utils/executeondestruction.h>

#include <QTest>
#include <QSignalSpy>
#include <QTestEventLoop>

//#define WITH_BENCHMARK
#ifdef WITH_BENCHMARK
#include <valgrind/callgrind.h>
#endif

#endif // WITH_TESTS

#include <climits>

#define DEBUG_STATE 1
#ifdef DEBUG_STATE
//#   define STATE_DEBUG(s)
//    do { QString msg; QTextStream ts(&msg); ts << s;
//      showMessage(msg, LogDebug); } while (0)
#   define STATE_DEBUG(s) do { qDebug() << s; } while (0)
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

    \brief The DebuggerEngine class is the base class of a debugger engine.

    \note The Debugger process itself and any helper processes like
    gdbserver are referred to as 'Engine', whereas the debugged process
    is referred to as 'Inferior'.

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
                         +
                  EngineRunRequested
                         +
                 (calls *Engine->runEngine())
               /       |            |        \
             /         |            |          \
            | (core)   | (attach)   |           |
            |          |            |           |
      {notify-    {notifyER&- {notifyER&-  {notify-
      Inferior-     Inferior-   Inferior-  EngineRun-
     Unrunnable}     StopOk}     RunOk}     Failed}
           +           +            +           +
   InferiorUnrunnable  +     InferiorRunOk      +
                       +                        +
                InferiorStopOk            EngineRunFailed
                                                +
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
                            |                                             +
             {notifyInferiorShutdownFinished}                             +
                            +                                             +
                            +                                             +
  #Inferior exited#         +                                             +
         |                  +                                             +
   {notifyInferior-         +                                             +
      Exited}               +                                             +
           +                +                                             +
             +              +                                             +
               +            +                                             +
                 InferiorShutdownFinished                                 +
                            *                                             +
                  EngineShutdownRequested                                 +
                            +                                             +
           (calls *Engine->shutdownEngine())  <+-+-+-+-+-+-+-+-+-+-+-+-+-+'
                            |
                            |
              {notifyEngineShutdownFinished}
                            +
                  EngineShutdownFinished
                            *
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
sg1:   EngineSetupOk -> EngineRunRequested [ label= "RunControl::StartSuccessful" ];
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
sg1:   InferiorShutdownRequested -> InferiorShutdownFinished [ label= "Engine::shutdownInferior\nnotifyInferiorShutdownFinished", style="dashed" ];
sg1:   InferiorExited -> InferiorExitOk [ label="notifyInferiorExited", style="dashed"];
sg1:   InferiorExitOk -> InferiorShutdownOk
sg1:   InferiorShutdownFinished -> EngineShutdownRequested
sg1:   EngineShutdownRequested -> EngineShutdownFinished [ label="Engine::shutdownEngine\nnotifyEngineShutdownFinished", style="dashed" ];
sg1:   EngineShutdownFinished -> DebuggerFinished  [ style = "dotted" ];
sg1: }
* \endcode */
// Additional signalling:    {notifyInferiorIll}   {notifyEngineIll}


/*!
    \class Debugger::Internal::GdbEngine
    \brief The GdbEngine class implements Debugger::Engine driving a GDB
    executable.

    GdbEngine specific startup. All happens in EngineSetupRequested state:

    \list
        \li Transitions marked by '---' are done in the individual adapters.

        \li Transitions marked by '+-+' are done in the GdbEngine.
    \endlist

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
using namespace Core::Constants;
using namespace Debugger::Constants;
using namespace Debugger::Internal;
using namespace ExtensionSystem;
using namespace ProjectExplorer;
using namespace TextEditor;
using namespace Utils;

namespace CC = Core::Constants;
namespace PE = ProjectExplorer::Constants;

namespace Debugger {
namespace Internal {

void addCdbOptionPages(QList<IOptionsPage*> *opts);
void addGdbOptionPages(QList<IOptionsPage*> *opts);

static QIcon visibleStartIcon(Id id, bool toolBarStyle)
{
    if (id == Id(Constants::DEBUG)) {
        const static QIcon sidebarIcon =
                Icon::sideBarIcon(ProjectExplorer::Icons::DEBUG_START, ProjectExplorer::Icons::DEBUG_START_FLAT);
        const static QIcon icon =
                Icon::combinedIcon({ProjectExplorer::Icons::DEBUG_START_SMALL.icon(), sidebarIcon});
        const static QIcon iconToolBar =
                Icon::combinedIcon({ProjectExplorer::Icons::DEBUG_START_SMALL_TOOLBAR.icon(), sidebarIcon});
        return toolBarStyle ? iconToolBar : icon;
    } else if (id == Id(Constants::CONTINUE)) {
        const static QIcon sidebarIcon =
                Icon::sideBarIcon(Icons::CONTINUE, Icons::CONTINUE_FLAT);
        const static QIcon icon =
                Icon::combinedIcon({Icons::DEBUG_CONTINUE_SMALL.icon(), sidebarIcon});
        const static QIcon iconToolBar =
                Icon::combinedIcon({Icons::DEBUG_CONTINUE_SMALL_TOOLBAR.icon(), sidebarIcon});
        return toolBarStyle ? iconToolBar : icon;
    } else if (id == Id(Constants::INTERRUPT)) {
        const static QIcon sidebarIcon =
                Icon::sideBarIcon(Icons::INTERRUPT, Icons::INTERRUPT_FLAT);
        const static QIcon icon =
                Icon::combinedIcon({Icons::DEBUG_INTERRUPT_SMALL.icon(), sidebarIcon});
        const static QIcon iconToolBar =
                Icon::combinedIcon({Icons::DEBUG_INTERRUPT_SMALL_TOOLBAR.icon(), sidebarIcon});
        return toolBarStyle ? iconToolBar : icon;
    }
    return QIcon();
}

static void setProxyAction(ProxyAction *proxy, Id id)
{
    proxy->setAction(ActionManager::command(id)->action());
    proxy->setIcon(visibleStartIcon(id, true));
}

QAction *addAction(QMenu *menu, const QString &display, bool on,
                   const std::function<void()> &onTriggered)
{
    QAction *act = menu->addAction(display);
    act->setEnabled(on);
    QObject::connect(act, &QAction::triggered, onTriggered);
    return act;
};

QAction *addAction(QMenu *menu, const QString &d1, const QString &d2, bool on,
                   const std::function<void()> &onTriggered)
{
    return on ? addAction(menu, d1, true, onTriggered) : addAction(menu, d2, false);
};

QAction *addCheckableAction(QMenu *menu, const QString &display, bool on, bool checked,
                            const std::function<void()> &onTriggered)
{
    QAction *act = addAction(menu, display, on, onTriggered);
    act->setCheckable(true);
    act->setChecked(checked);
    return act;
}

///////////////////////////////////////////////////////////////////////
//
// DummyEngine
//
///////////////////////////////////////////////////////////////////////

class DummyEngine : public DebuggerEngine
{
public:
    DummyEngine() {}
    ~DummyEngine() override {}

    void setupEngine() override {}
    void runEngine() override {}
    void shutdownEngine() override {}
    void shutdownInferior() override {}
    bool hasCapability(unsigned cap) const override;
    bool acceptsBreakpoint(Breakpoint) const override { return false; }
    bool acceptsDebuggerCommands() const override { return false; }
    void selectThread(ThreadId) override {}
};

bool DummyEngine::hasCapability(unsigned cap) const
{
    // This can only be a first approximation of what to expect when running.
    Project *project = ProjectTree::currentProject();
    if (!project)
        return 0;
    Target *target = project->activeTarget();
    QTC_ASSERT(target, return 0);
    RunConfiguration *activeRc = target->activeRunConfiguration();
    QTC_ASSERT(activeRc, return 0);

    // This is a non-started Cdb or Gdb engine:
    if (activeRc->extraAspect<Debugger::DebuggerRunConfigurationAspect>()->useCppDebugger())
        return cap & (WatchpointByAddressCapability
               | BreakConditionCapability
               | TracePointCapability
               | OperateByInstructionCapability);

    // This is a Qml or unknown engine.
    return cap & AddWatcherCapability;
}

///////////////////////////////////////////////////////////////////////
//
// DebugMode
//
///////////////////////////////////////////////////////////////////////

class DebugMode : public IMode
{
public:
    DebugMode()
    {
        setObjectName(QLatin1String("DebugMode"));
        setContext(Context(C_DEBUGMODE, CC::C_NAVIGATION_PANE));
        setDisplayName(DebuggerPlugin::tr("Debug"));
        setIcon(Utils::Icon::modeIcon(Icons::MODE_DEBUGGER_CLASSIC,
                                      Icons::MODE_DEBUGGER_FLAT, Icons::MODE_DEBUGGER_FLAT_ACTIVE));
        setPriority(85);
        setId(MODE_DEBUG);
    }
};

///////////////////////////////////////////////////////////////////////
//
// Misc
//
///////////////////////////////////////////////////////////////////////

static QWidget *addSearch(BaseTreeView *treeView, const QString &title,
    const QString &objectName)
{
    QAction *act = action(UseAlternatingRowColors);
    treeView->setAlternatingRowColors(act->isChecked());
    QObject::connect(act, &QAction::toggled,
                     treeView, &BaseTreeView::setAlternatingRowColors);

    QWidget *widget = ItemViewFind::createSearchableWrapper(treeView);
    widget->setObjectName(objectName);
    widget->setWindowTitle(title);
    return widget;
}

static Kit::Predicate cdbPredicate(char wordWidth = 0)
{
    return [wordWidth](const Kit *k) -> bool {
        if (DebuggerKitInformation::engineType(k) != CdbEngineType
            || DebuggerKitInformation::configurationErrors(k)) {
            return false;
        }
        if (wordWidth)
            return ToolChainKitInformation::targetAbi(k).wordWidth() == wordWidth;
        return true;
    };
}

// Find a CDB kit for debugging unknown processes.
// On a 64bit OS, prefer a 64bit debugger.
static Kit *findUniversalCdbKit()
{
    if (Utils::is64BitWindowsSystem()) {
        if (Kit *cdb64Kit = KitManager::kit(cdbPredicate(64)))
            return cdb64Kit;
    }
    return KitManager::kit(cdbPredicate());
}

///////////////////////////////////////////////////////////////////////
//
// Debuginfo Taskhandler
//
///////////////////////////////////////////////////////////////////////

class DebugInfoTaskHandler : public ITaskHandler
{
public:
    bool canHandle(const Task &task) const final
    {
        return m_debugInfoTasks.contains(task.taskId);
    }

    void handle(const Task &task) final
    {
        QString cmd = m_debugInfoTasks.value(task.taskId);
        QProcess::startDetached(cmd);
    }

    void addTask(unsigned id, const QString &cmd)
    {
        m_debugInfoTasks[id] = cmd;
    }

    QAction *createAction(QObject *parent) const final
    {
        QAction *action = new QAction(DebuggerPlugin::tr("Install &Debug Information"), parent);
        action->setToolTip(DebuggerPlugin::tr("Tries to install missing debug information."));
        return action;
    }

private:
    QHash<unsigned, QString> m_debugInfoTasks;
};

///////////////////////////////////////////////////////////////////////
//
// DebuggerPluginPrivate
//
///////////////////////////////////////////////////////////////////////

static DebuggerPluginPrivate *dd = nullptr;

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

class DebuggerPluginPrivate : public QObject
{
    Q_OBJECT

public:
    explicit DebuggerPluginPrivate(DebuggerPlugin *plugin);
    ~DebuggerPluginPrivate();

    bool initialize(const QStringList &arguments, QString *errorMessage);
    void extensionsInitialized();
    void aboutToShutdown();
    void doShutdown();

    void connectEngine(DebuggerRunTool *runTool);
    void disconnectEngine() { connectEngine(0); }
    DebuggerEngine *dummyEngine();

    void setThreadBoxContents(const QStringList &list, int index)
    {
        QSignalBlocker blocker(m_threadBox);
        m_threadBox->clear();
        for (const QString &item : list)
            m_threadBox->addItem(item);
        m_threadBox->setCurrentIndex(index);
    }

    RunControl *attachToRunningProcess(Kit *kit, DeviceProcessItem process, bool contAfterAttach);

    void writeSettings()
    {
        m_debuggerSettings->writeSettings();
//        writeWindowSettings();
    }

    void selectThread(int index)
    {
        QTC_ASSERT(m_currentRunTool, return);
        DebuggerEngine *engine = m_currentRunTool->activeEngine();
        QTC_ASSERT(engine, return);
        ThreadId id = engine->threadsHandler()->threadAt(index);
        engine->selectThread(id);
    }

    void breakpointSetMarginActionTriggered(bool isMessageOnly, const ContextData &data)
    {
        QString message;
        if (isMessageOnly) {
            if (data.type == LocationByAddress) {
                //: Message tracepoint: Address hit.
                message = tr("0x%1 hit").arg(data.address, 0, 16);
            } else {
                //: Message tracepoint: %1 file, %2 line %3 function hit.
                message = tr("%1:%2 %3() hit").arg(FileName::fromString(data.fileName).fileName()).
                        arg(data.lineNumber).
                        arg(cppFunctionAt(data.fileName, data.lineNumber));
            }
            QInputDialog dialog; // Create wide input dialog.
            dialog.setWindowFlags(dialog.windowFlags()
              & ~(Qt::WindowContextHelpButtonHint|Qt::MSWindowsFixedSizeDialogHint));
            dialog.resize(600, dialog.height());
            dialog.setWindowTitle(tr("Add Message Tracepoint"));
            dialog.setLabelText (tr("Message:"));
            dialog.setTextValue(message);
            if (dialog.exec() != QDialog::Accepted || dialog.textValue().isEmpty())
                return;
            message = dialog.textValue();
        }
        toggleBreakpoint(data, message);
    }

    void updateReturnViewHeader(int section, int, int newSize)
    {
        if (m_shuttingDown)
            return;
        m_returnView->header()->resizeSection(section, newSize);
    }

    void synchronizeBreakpoints()
    {
        showMessage(QLatin1String("ATTEMPT SYNC"), LogDebug);
        for (int i = 0, n = m_snapshotHandler->size(); i != n; ++i) {
            if (DebuggerEngine *engine = m_snapshotHandler->at(i)->engine())
                engine->attemptBreakpointSynchronization();
        }
    }

    void reloadSourceFiles() { if (m_currentRunTool) m_currentRunTool->engine()->reloadSourceFiles(); }
    void reloadRegisters() { if (m_currentRunTool) m_currentRunTool->engine()->reloadRegisters(); }
    void reloadModules() { if (m_currentRunTool) m_currentRunTool->engine()->reloadModules(); }

    void editorOpened(IEditor *editor);
    void updateBreakMenuItem(IEditor *editor);
    void setBusyCursor(bool busy);
    void requestMark(TextEditorWidget *widget, int lineNumber,
                     TextMarkRequestKind kind);
    void requestContextMenu(TextEditorWidget *widget,
                            int lineNumber, QMenu *menu);

    void activatePreviousMode();
    void activateDebugMode();
    void toggleBreakpointHelper();
    void toggleBreakpoint(const ContextData &location, const QString &tracePointMessage = QString());
    void onModeChanged(Id mode);
    void updateDebugWithoutDeployMenu();

    void startRemoteCdbSession();
    void attachToRunningApplication();
    void attachToUnstartedApplicationDialog();
    void attachToQmlPort();
    void runScheduled();
    void attachCore();

    void enableReverseDebuggingTriggered(const QVariant &value);
    void showStatusMessage(const QString &msg, int timeout = -1);

    void runControlStarted(DebuggerRunTool *runTool);
    void runControlFinished(DebuggerRunTool *runTool);
    void remoteCommand(const QStringList &options);

    void displayDebugger(DebuggerRunTool *runTool);

    void dumpLog();
    void cleanupViews();
    void setInitialState();

    void fontSettingsChanged(const FontSettings &settings);

    void updateState(DebuggerRunTool *runTool);
    void onCurrentProjectChanged(Project *project);

    void sessionLoaded();
    void aboutToUnloadSession();
    void aboutToSaveSession();

public:
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

    void handleAbort()
    {
        if (dd->m_currentRunTool)
            dd->m_currentRunTool->abortDebugger();
    }

    void handleReset()
    {
        currentEngine()->resetLocation();
        currentEngine()->resetInferior();
    }

    void handleExecStep()
    {
        if (currentEngine()->state() == DebuggerNotReady) {
            DebuggerRunTool::setBreakOnMainNextTime();
            ProjectExplorerPlugin::runStartupProject(ProjectExplorer::Constants::DEBUG_RUN_MODE);
        } else {
            currentEngine()->resetLocation();
            if (boolSetting(OperateByInstruction))
                currentEngine()->executeStepI();
            else
                currentEngine()->executeStep();
        }
    }

    void handleExecNext()
    {
        if (currentEngine()->state() == DebuggerNotReady) {
            DebuggerRunTool::setBreakOnMainNextTime();
            ProjectExplorerPlugin::runStartupProject(ProjectExplorer::Constants::DEBUG_RUN_MODE);
        } else {
            currentEngine()->resetLocation();
            if (boolSetting(OperateByInstruction))
                currentEngine()->executeNextI();
            else
                currentEngine()->executeNext();
        }
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
        currentEngine()->resetLocation();
        if (BaseTextEditor *textEditor = BaseTextEditor::currentTextEditor()) {
            ContextData location = getLocationContext(textEditor->textDocument(),
                                                      textEditor->currentLine());
            if (location.isValid())
                currentEngine()->executeJumpToLine(location);
        }
    }

    void handleExecRunToLine()
    {
        currentEngine()->resetLocation();
        if (BaseTextEditor *textEditor = BaseTextEditor::currentTextEditor()) {
            ContextData location = getLocationContext(textEditor->textDocument(),
                                                      textEditor->currentLine());
            if (location.isValid())
                currentEngine()->executeRunToLine(location);
        }
    }

    void handleExecRunToSelectedFunction()
    {
        BaseTextEditor *textEditor = BaseTextEditor::currentTextEditor();
        QTC_ASSERT(textEditor, return);
        QTextCursor cursor = textEditor->textCursor();
        QString functionName = cursor.selectedText();
        if (functionName.isEmpty()) {
            const QTextBlock block = cursor.block();
            const QString line = block.text();
            foreach (const QString &str, line.trimmed().split(QLatin1Char('('))) {
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

    void handleAddToWatchWindow()
    {
        // Requires a selection, but that's the only case we want anyway.
        BaseTextEditor *textEditor = BaseTextEditor::currentTextEditor();
        if (!textEditor)
            return;
        QTextCursor tc = textEditor->textCursor();
        QString exp;
        if (tc.hasSelection()) {
            exp = tc.selectedText();
        } else {
            int line, column;
            exp = cppExpressionAt(textEditor->editorWidget(), tc.position(), &line, &column);
        }
        if (currentEngine()->hasCapability(WatchComplexExpressionsCapability))
            exp = removeObviousSideEffects(exp);
        else
            exp = fixCppExpression(exp);
        exp = exp.trimmed();
        if (exp.isEmpty()) {
            // Happens e.g. when trying to evaluate 'char' or 'return'.
            AsynchronousMessageBox::warning(tr("Warning"),
                tr("Select a valid expression to evaluate."));
            return;
        }
        currentEngine()->watchHandler()->watchVariable(exp);
    }

    void stopDebugger()
    {
        QTC_ASSERT(dd->m_currentRunTool, return);
        dd->m_currentRunTool->runControl()->initiateStop();
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
        if (DebuggerEngine *cppEngine = currentEngine()->cppEngine()) {
            if (cppEngine->stackHandler()->currentIndex() >= 0) {
                const StackFrame frame = cppEngine->stackHandler()->currentFrame();
                if (operateByInstructionTriggered || frame.isUsable())
                    cppEngine->gotoLocation(Location(frame, true));
            }
        }
    }

    void showMessage(const QString &msg, int channel, int timeout = -1);

    bool parseArgument(QStringList::const_iterator &it,
        const QStringList::const_iterator &cend, QString *errorMessage);
    bool parseArguments(const QStringList &args, QString *errorMessage);
    void parseCommandLineArguments();

    // Called when all dependent plugins have loaded.
    void initialize();

    void updateUiForProject(ProjectExplorer::Project *project);
    void updateUiForTarget(ProjectExplorer::Target *target);
    void updateActiveLanguages();

public:
    QPointer<DebuggerMainWindow> m_mainWindow;
    QPointer<QWidget> m_modeWindow;
    QPointer<DebugMode> m_mode;

    ActionContainer *m_menu = nullptr;

    Project *m_previousProject = nullptr;
    QPointer<Target> m_previousTarget;
    QPointer<RunConfiguration> m_previousRunConfiguration;

    Id m_previousMode;
    QVector<DebuggerRunTool *> m_scheduledStarts;

    ProxyAction *m_visibleStartAction = nullptr;
    ProxyAction *m_hiddenStopAction = nullptr;
    QAction *m_startAction = nullptr;
    QAction *m_debugWithoutDeployAction = nullptr;
    QAction *m_startAndDebugApplicationAction = nullptr;
    QAction *m_attachToRunningApplication = nullptr;
    QAction *m_attachToUnstartedApplication = nullptr;
    QAction *m_attachToQmlPortAction = nullptr;
    QAction *m_attachToRemoteServerAction = nullptr;
    QAction *m_startRemoteCdbAction = nullptr;
    QAction *m_attachToCoreAction = nullptr;
    QAction *m_detachAction = nullptr;
    QAction *m_continueAction = nullptr;
    QAction *m_exitAction = nullptr; // On application output button if "Stop" is possible
    QAction *m_interruptAction = nullptr; // On the fat debug button if "Pause" is possible
    QAction *m_undisturbableAction = nullptr; // On the fat debug button if nothing can be done
    QAction *m_abortAction = nullptr;
    QAction *m_stepAction = nullptr;
    QAction *m_stepOutAction = nullptr;
    QAction *m_runToLineAction = nullptr; // In the debug menu
    QAction *m_runToSelectedFunctionAction = nullptr;
    QAction *m_jumpToLineAction = nullptr; // In the Debug menu.
    QAction *m_returnFromFunctionAction = nullptr;
    QAction *m_nextAction = nullptr;
    QAction *m_watchAction = nullptr;
    Command *m_watchCommand = nullptr;
    QAction *m_breakAction = nullptr;
    QAction *m_reverseDirectionAction = nullptr;
    QAction *m_frameUpAction = nullptr;
    QAction *m_frameDownAction = nullptr;
    QAction *m_resetAction = nullptr;
    QAction *m_operateByInstructionAction = nullptr;

    QToolButton *m_reverseToolButton = nullptr;

    QLabel *m_threadLabel = nullptr;
    QComboBox *m_threadBox = nullptr;

    BaseTreeView *m_breakView = nullptr;
    BaseTreeView *m_returnView = nullptr;
    BaseTreeView *m_localsView = nullptr;
    BaseTreeView *m_watchersView = nullptr;
    WatchTreeView *m_inspectorView = nullptr;
    BaseTreeView *m_registerView = nullptr;
    BaseTreeView *m_modulesView = nullptr;
    BaseTreeView *m_snapshotView = nullptr;
    BaseTreeView *m_sourceFilesView = nullptr;
    BaseTreeView *m_stackView = nullptr;
    BaseTreeView *m_threadsView = nullptr;

    QWidget *m_breakWindow = nullptr;
    BreakHandler *m_breakHandler = nullptr;
    QWidget *m_returnWindow = nullptr;
    QWidget *m_localsWindow = nullptr;
    QWidget *m_watchersWindow = nullptr;
    QWidget *m_inspectorWindow = nullptr;
    QWidget *m_registerWindow = nullptr;
    QWidget *m_modulesWindow = nullptr;
    QWidget *m_snapshotWindow = nullptr;
    QWidget *m_sourceFilesWindow = nullptr;
    QWidget *m_stackWindow = nullptr;
    QWidget *m_threadsWindow = nullptr;
    LogWindow *m_logWindow = nullptr;
    LocalsAndInspectorWindow *m_localsAndInspectorWindow = nullptr;

    bool m_busy = false;
    QString m_lastPermanentStatusMessage;

    DebuggerPlugin *m_plugin = nullptr;

    SnapshotHandler *m_snapshotHandler = nullptr;
    QTimer m_shutdownTimer;
    bool m_shuttingDown = false;
    QPointer<DebuggerEngine> m_previouslyActiveEngine;
    QPointer<DebuggerRunTool> m_currentRunTool;
    DebuggerSettings *m_debuggerSettings = nullptr;
    QStringList m_arguments;
    DebuggerToolTipManager m_toolTipManager;
    DummyEngine *m_dummyEngine = nullptr;
    const QSharedPointer<GlobalDebuggerOptions> m_globalDebuggerOptions;

    DebuggerItemManager m_debuggerItemManager;
    QList<IOptionsPage *> m_optionPages;
    IContext m_debugModeContext;

    DebugInfoTaskHandler m_debugInfoTaskHandler;
};

DebuggerPluginPrivate::DebuggerPluginPrivate(DebuggerPlugin *plugin)
    : m_globalDebuggerOptions(new GlobalDebuggerOptions)
{
    qRegisterMetaType<ContextData>("ContextData");
    qRegisterMetaType<DebuggerRunParameters>("DebuggerRunParameters");

    QTC_CHECK(!dd);
    dd = this;

    m_plugin = plugin;
    debuggerConsole(); // ensure Debugger Console is created before settings are taken into account
}

DebuggerPluginPrivate::~DebuggerPluginPrivate()
{
    destroyDebuggerConsole();

    qDeleteAll(m_optionPages);
    m_optionPages.clear();

    delete m_debuggerSettings;
    m_debuggerSettings = nullptr;

    delete m_snapshotHandler;
    m_snapshotHandler = nullptr;

    delete m_breakHandler;
    m_breakHandler = nullptr;
}

DebuggerEngine *DebuggerPluginPrivate::dummyEngine()
{
    if (!m_dummyEngine) {
        m_dummyEngine = new DummyEngine;
        m_dummyEngine->setParent(this);
        m_dummyEngine->setObjectName("DummyEngine");
    }
    return m_dummyEngine;
}

static QString msgParameterMissing(const QString &a)
{
    return DebuggerPlugin::tr("Option \"%1\" is missing the parameter.").arg(a);
}

static Kit *guessKitFromAbis(const QList<Abi> &abis)
{
    Kit *kit = nullptr;

    // Try to find a kit via ABI.
    if (!abis.isEmpty()) {
        // Try exact abis.
        kit = KitManager::kit([abis](const Kit *k) {
            const Abi tcAbi = ToolChainKitInformation::targetAbi(k);
            return abis.contains(tcAbi) && !DebuggerKitInformation::configurationErrors(k);
        });
        if (!kit) {
            // Or something compatible.
            kit = KitManager::kit([abis](const Kit *k) {
                const Abi tcAbi = ToolChainKitInformation::targetAbi(k);
                return !DebuggerKitInformation::configurationErrors(k)
                        && Utils::contains(abis, [tcAbi](const Abi &a) { return a.isCompatibleWith(tcAbi); });
            });
        }
    }

    if (!kit)
        kit = KitManager::defaultKit();

    return kit;
}

bool DebuggerPluginPrivate::parseArgument(QStringList::const_iterator &it,
    const QStringList::const_iterator &cend, QString *errorMessage)
{
    const QString &option = *it;
    // '-debug <pid>'
    // '-debug <exe>[,server=<server:port>][,core=<core>][,kit=<kit>][,terminal={0,1}]'
    if (*it == "-debug") {
        ++it;
        if (it == cend) {
            *errorMessage = msgParameterMissing(*it);
            return false;
        }
        const qulonglong pid = it->toULongLong();
        const QStringList args = it->split(',');

        Kit *kit = nullptr;
        DebuggerStartMode startMode = StartExternal;
        QString executable;
        QString remoteChannel;
        QString coreFile;
        bool useTerminal = false;

        if (!pid) {
            for (const QString &arg : args) {
                const QString key = arg.section('=', 0, 0);
                const QString val = arg.section('=', 1, 1);
                if (val.isEmpty()) {
                    if (key.isEmpty()) {
                        continue;
                    } else if (executable.isEmpty()) {
                        executable = key;
                    } else {
                        *errorMessage = DebuggerPlugin::tr("Only one executable allowed.");
                        return false;
                    }
                } else if (key == "kit") {
                    kit = KitManager::kit(Id::fromString(val));
                    if (!kit)
                        kit = KitManager::kit(Utils::equal(&Kit::displayName, val));
                } else if (key == "server") {
                    startMode = AttachToRemoteServer;
                    remoteChannel = remoteChannel;
                } else if (key == "core") {
                    startMode = AttachCore;
                    coreFile = val;
                } else if (key == "terminal") {
                    useTerminal = true;
                }
            }
        }
        if (!kit)
            kit = guessKitFromAbis(Abi::abisOfBinary(FileName::fromString(executable)));

        IDevice::ConstPtr device = DeviceKitInformation::device(kit);
        auto runControl = new RunControl(device, ProjectExplorer::Constants::DEBUG_RUN_MODE);
        auto debugger = new DebuggerRunTool(runControl, kit);
        debugger->setInferiorExecutable(executable);
        if (pid) {
            debugger->setStartMode(AttachExternal);
            debugger->setCloseMode(DetachAtClose);
            debugger->setAttachPid(pid);
            debugger->setRunControlName(tr("Process %1").arg(pid));
            debugger->setStartMessage(tr("Attaching to local process %1.").arg(pid));
        } else if (startMode == AttachToRemoteServer) {
            debugger->setStartMode(AttachToRemoteServer);
            debugger->setRemoteChannel(remoteChannel);
            debugger->setRunControlName(tr("Remote: \"%1\"").arg(remoteChannel));
            debugger->setStartMessage(tr("Attaching to remote server %1.").arg(remoteChannel));
        } else if (startMode == AttachCore) {
            debugger->setStartMode(AttachCore);
            debugger->setCloseMode(DetachAtClose);
            debugger->setCoreFileName(coreFile);
            debugger->setRunControlName(tr("Core file \"%1\"").arg(coreFile));
            debugger->setStartMessage(tr("Attaching to core file %1.").arg(coreFile));
        } else {
            debugger->setStartMode(StartExternal);
            debugger->setRunControlName(tr("Executable file \"%1\"").arg(executable));
            debugger->setStartMessage(tr("Debugging file %1.").arg(executable));
        }
        debugger->setUseTerminal(useTerminal);

        m_scheduledStarts.append(debugger);
        return true;
    }
    // -wincrashevent <event-handle>:<pid>. A handle used for
    // a handshake when attaching to a crashed Windows process.
    // This is created by $QTC/src/tools/qtcdebugger/main.cpp:
    // args << QLatin1String("-wincrashevent")
    //   << QString::fromLatin1("%1:%2").arg(argWinCrashEvent).arg(argProcessId);
    if (*it == "-wincrashevent") {
        ++it;
        if (it == cend) {
            *errorMessage = msgParameterMissing(*it);
            return false;
        }
        qint64 pid = it->section(':', 1, 1).toULongLong();
        auto runControl = new RunControl(nullptr, ProjectExplorer::Constants::DEBUG_RUN_MODE);
        auto debugger = new DebuggerRunTool(runControl, findUniversalCdbKit());
        debugger->setStartMode(AttachCrashedExternal);
        debugger->setCrashParameter(it->section(':', 0, 0));
        debugger->setAttachPid(pid);
        debugger->setRunControlName(tr("Crashed process %1").arg(pid));
        debugger->setStartMessage(tr("Attaching to crashed process %1").arg(pid));
        if (pid < 1) {
            *errorMessage = DebuggerPlugin::tr("The parameter \"%1\" of option \"%2\" "
                "does not match the pattern <handle>:<pid>.").arg(*it, option);
            return false;
        }
        m_scheduledStarts.append(debugger);
        return true;
    }

    *errorMessage = DebuggerPlugin::tr("Invalid debugger option: %1").arg(option);
    return false;
}

bool DebuggerPluginPrivate::parseArguments(const QStringList &args,
    QString *errorMessage)
{
    const QStringList::const_iterator cend = args.constEnd();
    for (QStringList::const_iterator it = args.constBegin(); it != cend; ++it)
        if (!parseArgument(it, cend, errorMessage))
            return false;
    return true;
}

void DebuggerPluginPrivate::parseCommandLineArguments()
{
    QString errorMessage;
    if (!parseArguments(m_arguments, &errorMessage)) {
        errorMessage = tr("Error evaluating command line arguments: %1")
            .arg(errorMessage);
        qWarning("%s\n", qPrintable(errorMessage));
        MessageManager::write(errorMessage);
    }
    if (!m_scheduledStarts.isEmpty())
        QTimer::singleShot(0, this, &DebuggerPluginPrivate::runScheduled);
}

bool DebuggerPluginPrivate::initialize(const QStringList &arguments,
    QString *errorMessage)
{
    Q_UNUSED(errorMessage);
    m_arguments = arguments;
    if (!m_arguments.isEmpty())
        connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::finishedInitialization,
                this, &DebuggerPluginPrivate::parseCommandLineArguments);

    m_mainWindow = new DebuggerMainWindow;

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

    m_menu->addSeparator(G_ANALYZER_TOOLS);
    m_menu->addSeparator(G_ANALYZER_REMOTE_TOOLS);
    m_menu->addSeparator(G_ANALYZER_OPTIONS);

    // Populate Windows->Views menu with standard actions.
    Context debugcontext(Constants::C_DEBUGMODE);

    auto openMemoryEditorAction = new QAction(this);
    openMemoryEditorAction->setText(DebuggerPluginPrivate::tr("Memory..."));
    connect(openMemoryEditorAction, &QAction::triggered,
            this, &Internal::openMemoryEditor);

    Command *cmd = ActionManager::registerAction(openMemoryEditorAction,
        "Debugger.Views.OpenMemoryEditor", debugcontext);
    cmd->setAttribute(Command::CA_Hide);

    TaskHub::addCategory(TASK_CATEGORY_DEBUGGER_DEBUGINFO,
                         tr("Debug Information"));
    TaskHub::addCategory(TASK_CATEGORY_DEBUGGER_RUNTIME,
                         tr("Debugger Runtime"));

    const QKeySequence debugKey = QKeySequence(useMacShortcuts ? tr("Ctrl+Y") : tr("F5"));

    QSettings *settings = ICore::settings();

    m_debuggerSettings = new DebuggerSettings;
    m_debuggerSettings->readSettings();

    const Context cppDebuggercontext(C_CPPDEBUGGER);
    const Context qmljsDebuggercontext(C_QMLDEBUGGER);

    m_busy = false;

    m_logWindow = new LogWindow;
    m_logWindow->setObjectName(QLatin1String(DOCKWIDGET_OUTPUT));

    m_breakHandler = new BreakHandler;
    m_breakView = new BaseTreeView;
    m_breakView->setIconSize(QSize(10, 10));
    m_breakView->setWindowIcon(Icons::BREAKPOINTS.icon());
    m_breakView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    connect(action(UseAddressInBreakpointsView), &QAction::toggled,
            this, [this](bool on) { m_breakView->setColumnHidden(BreakpointAddressColumn, !on); });
    m_breakView->setSettings(settings, "Debugger.BreakWindow");
    m_breakView->setModel(m_breakHandler->model());
    m_breakWindow = addSearch(m_breakView, tr("&Breakpoints"), DOCKWIDGET_BREAK);
    m_breakView->setRootIsDecorated(true);

    m_modulesView = new BaseTreeView;
    m_modulesView->setSortingEnabled(true);
    m_modulesView->setSettings(settings, "Debugger.ModulesView");
    connect(m_modulesView, &BaseTreeView::aboutToShow,
            this, &DebuggerPluginPrivate::reloadModules,
            Qt::QueuedConnection);
    m_modulesWindow = addSearch(m_modulesView, tr("&Modules"), DOCKWIDGET_MODULES);

    m_registerView = new BaseTreeView;
    m_registerView->setRootIsDecorated(true);
    m_registerView->setSettings(settings, "Debugger.RegisterView");
    connect(m_registerView, &BaseTreeView::aboutToShow,
            this, &DebuggerPluginPrivate::reloadRegisters,
            Qt::QueuedConnection);
    m_registerWindow = addSearch(m_registerView, tr("Reg&isters"), DOCKWIDGET_REGISTER);

    m_stackView = new StackTreeView;
    m_stackView->setSettings(settings, "Debugger.StackView");
    m_stackView->setIconSize(QSize(10, 10));
    m_stackWindow = addSearch(m_stackView, tr("&Stack"), DOCKWIDGET_STACK);

    m_sourceFilesView = new BaseTreeView;
    m_sourceFilesView->setSortingEnabled(true);
    m_sourceFilesView->setSettings(settings, "Debugger.SourceFilesView");
    connect(m_sourceFilesView, &BaseTreeView::aboutToShow,
            this, &DebuggerPluginPrivate::reloadSourceFiles,
            Qt::QueuedConnection);
    m_sourceFilesWindow = addSearch(m_sourceFilesView, tr("Source Files"), DOCKWIDGET_SOURCE_FILES);

    m_threadsView = new BaseTreeView;
    m_threadsView->setSortingEnabled(true);
    m_threadsView->setSettings(settings, "Debugger.ThreadsView");
    m_threadsView->setIconSize(QSize(10, 10));
    m_threadsWindow = addSearch(m_threadsView, tr("&Threads"), DOCKWIDGET_THREADS);

    m_returnView = new WatchTreeView(ReturnType); // No settings.
    m_returnWindow = addSearch(m_returnView, tr("Locals"), "CppDebugReturn");

    m_localsView = new WatchTreeView(LocalsType);
    m_localsView->setSettings(settings, "Debugger.LocalsView");
    m_localsWindow = addSearch(m_localsView, tr("Locals"), "CppDebugLocals");

    m_inspectorView = new WatchTreeView(InspectType); // No settings.
    m_inspectorView->setSettings(settings, "Debugger.LocalsView"); // sic! same as locals view.
    m_inspectorWindow = addSearch(m_inspectorView, tr("Locals"), "Inspector");

    m_watchersView = new WatchTreeView(WatchersType);
    m_watchersView->setSettings(settings, "Debugger.WatchersView");
    m_watchersWindow = addSearch(m_watchersView, tr("&Expressions"), "CppDebugWatchers");

    // Snapshot
    m_snapshotHandler = new SnapshotHandler;
    m_snapshotView = new SnapshotTreeView(m_snapshotHandler);
    m_snapshotView->setSettings(settings, "Debugger.SnapshotView");
    m_snapshotView->setIconSize(QSize(10, 10));
    m_snapshotView->setModel(m_snapshotHandler->model());
    m_snapshotWindow = addSearch(m_snapshotView, tr("Snapshots"), DOCKWIDGET_SNAPSHOTS);

    // Locals
    connect(m_localsView->header(), &QHeaderView::sectionResized,
        this, &DebuggerPluginPrivate::updateReturnViewHeader, Qt::QueuedConnection);

    auto act = m_continueAction = new QAction(tr("Continue"), this);
    act->setIcon(visibleStartIcon(Id(Constants::CONTINUE), false));
    connect(act, &QAction::triggered, this, &DebuggerPluginPrivate::handleExecContinue);

    act = m_exitAction = new QAction(tr("Stop Debugger"), this);
    act->setIcon(Icons::DEBUG_EXIT_SMALL.icon());
    connect(act, &QAction::triggered, this, &DebuggerPluginPrivate::stopDebugger);

    act = m_interruptAction = new QAction(tr("Interrupt"), this);
    act->setIcon(visibleStartIcon(Id(Constants::INTERRUPT), false));
    connect(act, &QAction::triggered, this, &DebuggerPluginPrivate::handleExecInterrupt);

    // A "disabled pause" seems to be a good choice.
    act = m_undisturbableAction = new QAction(tr("Debugger is Busy"), this);
    act->setIcon(visibleStartIcon(Id(Constants::INTERRUPT), false));
    act->setEnabled(false);

    act = m_abortAction = new QAction(tr("Abort Debugging"), this);
    act->setToolTip(tr("Aborts debugging and "
        "resets the debugger to the initial state."));
    connect(act, &QAction::triggered, this, &DebuggerPluginPrivate::handleAbort);

    act = m_resetAction = new QAction(tr("Restart Debugging"),this);
    act->setToolTip(tr("Restart the debugging session."));
    act->setIcon(Icons::RESTART_TOOLBAR.icon());
    connect(act, &QAction::triggered, this, &DebuggerPluginPrivate::handleReset);

    act = m_nextAction = new QAction(tr("Step Over"), this);
    act->setIcon(Icons::STEP_OVER.icon());
    connect(act, &QAction::triggered, this, &DebuggerPluginPrivate::handleExecNext);

    act = m_stepAction = new QAction(tr("Step Into"), this);
    act->setIcon(Icons::STEP_INTO.icon());
    connect(act, &QAction::triggered, this, &DebuggerPluginPrivate::handleExecStep);

    act = m_stepOutAction = new QAction(tr("Step Out"), this);
    act->setIcon(Icons::STEP_OUT.icon());
    connect(act, &QAction::triggered, this, &DebuggerPluginPrivate::handleExecStepOut);

    act = m_runToLineAction = new QAction(tr("Run to Line"), this);
    connect(act, &QAction::triggered, this, &DebuggerPluginPrivate::handleExecRunToLine);

    act = m_runToSelectedFunctionAction = new QAction(tr("Run to Selected Function"), this);
    connect(act, &QAction::triggered, this, &DebuggerPluginPrivate::handleExecRunToSelectedFunction);

    act = m_returnFromFunctionAction =
        new QAction(tr("Immediately Return From Inner Function"), this);
    connect(act, &QAction::triggered, this, &DebuggerPluginPrivate::handleExecReturn);

    act = m_jumpToLineAction = new QAction(tr("Jump to Line"), this);
    connect(act, &QAction::triggered, this, &DebuggerPluginPrivate::handleExecJumpToLine);

    m_breakAction = new QAction(tr("Toggle Breakpoint"), this);

    act = m_watchAction = new QAction(tr("Add Expression Evaluator"), this);
    connect(act, &QAction::triggered, this, &DebuggerPluginPrivate::handleAddToWatchWindow);

    //act = m_snapshotAction = new QAction(tr("Create Snapshot"), this);
    //act->setProperty(Role, RequestCreateSnapshotRole);
    //act->setIcon(Icons::SNAPSHOT.icon());

    act = m_reverseDirectionAction = new QAction(tr("Reverse Direction"), this);
    act->setCheckable(true);
    act->setChecked(false);
    act->setCheckable(false);
    act->setIcon(Icons::REVERSE_MODE.icon());
    act->setIconVisibleInMenu(false);

    act = m_frameDownAction = new QAction(tr("Move to Called Frame"), this);
    connect(act, &QAction::triggered, this, &DebuggerPluginPrivate::handleFrameDown);

    act = m_frameUpAction = new QAction(tr("Move to Calling Frame"), this);
    connect(act, &QAction::triggered, this, &DebuggerPluginPrivate::handleFrameUp);

    act = m_operateByInstructionAction = action(OperateByInstruction);
    connect(act, &QAction::triggered,
            this, &DebuggerPluginPrivate::handleOperateByInstructionTriggered);

    ActionContainer *debugMenu = ActionManager::actionContainer(PE::M_DEBUG);

    m_localsAndInspectorWindow = new LocalsAndInspectorWindow(
                m_localsWindow, m_inspectorWindow, m_returnWindow);
    m_localsAndInspectorWindow->setObjectName(DOCKWIDGET_LOCALS_AND_INSPECTOR);
    m_localsAndInspectorWindow->setWindowTitle(m_localsWindow->windowTitle());

    RunConfiguration::registerAspect<DebuggerRunConfigurationAspect>();

    // The main "Start Debugging" action.
    act = m_startAction = new QAction(this);
    act->setIcon(visibleStartIcon(Id(Constants::DEBUG), false));
    act->setText(tr("Start Debugging"));
    connect(act, &QAction::triggered, [] { ProjectExplorerPlugin::runStartupProject(ProjectExplorer::Constants::DEBUG_RUN_MODE); });

    act = m_debugWithoutDeployAction = new QAction(this);
    act->setText(tr("Start Debugging Without Deployment"));
    connect(act, &QAction::triggered, [] { ProjectExplorerPlugin::runStartupProject(ProjectExplorer::Constants::DEBUG_RUN_MODE, true); });

    act = m_startAndDebugApplicationAction = new QAction(this);
    act->setText(tr("Start and Debug External Application..."));
    connect(act, &QAction::triggered, this, &StartApplicationDialog::startAndDebugApplication);

    act = m_attachToCoreAction = new QAction(this);
    act->setText(tr("Load Core File..."));
    connect(act, &QAction::triggered, this, &DebuggerPluginPrivate::attachCore);

    act = m_attachToRemoteServerAction = new QAction(this);
    act->setText(tr("Attach to Running Debug Server..."));
    connect(act, &QAction::triggered, this, &StartApplicationDialog::attachToRemoteServer);

    act = m_attachToRunningApplication = new QAction(this);
    act->setText(tr("Attach to Running Application..."));
    connect(act, &QAction::triggered, this, &DebuggerPluginPrivate::attachToRunningApplication);

    act = m_attachToUnstartedApplication = new QAction(this);
    act->setText(tr("Attach to Unstarted Application..."));
    connect(act, &QAction::triggered, this, &DebuggerPluginPrivate::attachToUnstartedApplicationDialog);

    act = m_attachToQmlPortAction = new QAction(this);
    act->setText(tr("Attach to QML Port..."));
    connect(act, &QAction::triggered, this, &DebuggerPluginPrivate::attachToQmlPort);

    if (HostOsInfo::isWindowsHost()) {
        m_startRemoteCdbAction = new QAction(tr("Attach to Remote CDB Session..."), this);
        connect(m_startRemoteCdbAction, &QAction::triggered,
                this, &DebuggerPluginPrivate::startRemoteCdbSession);
    }

    act = m_detachAction = new QAction(this);
    act->setText(tr("Detach Debugger"));
    connect(act, &QAction::triggered, this, &DebuggerPluginPrivate::handleExecDetach);

    // "Start Debugging" sub-menu
    // groups:
    //   G_DEFAULT_ONE
    //   G_START_LOCAL
    //   G_START_REMOTE
    //   G_START_QML

    ActionContainer *mstart = ActionManager::actionContainer(PE::M_DEBUG_STARTDEBUGGING);

    cmd = ActionManager::registerAction(m_startAction, Constants::DEBUG);
    cmd->setDescription(tr("Start Debugging"));
    cmd->setDefaultKeySequence(debugKey);
    cmd->setAttribute(Command::CA_UpdateText);
    mstart->addAction(cmd, CC::G_DEFAULT_ONE);
    m_visibleStartAction = new ProxyAction(this);
    m_visibleStartAction->initialize(m_startAction);
    m_visibleStartAction->setAttribute(ProxyAction::UpdateText);
    m_visibleStartAction->setAction(m_startAction);

    ModeManager::addAction(m_visibleStartAction, Constants::P_ACTION_DEBUG);

    cmd = ActionManager::registerAction(m_debugWithoutDeployAction,
        "Debugger.DebugWithoutDeploy");
    cmd->setAttribute(Command::CA_Hide);
    mstart->addAction(cmd, CC::G_DEFAULT_ONE);

    cmd = ActionManager::registerAction(m_attachToRunningApplication,
         "Debugger.AttachToRemoteProcess");
    cmd->setDescription(tr("Attach to Running Application"));
    mstart->addAction(cmd, G_GENERAL);

    cmd = ActionManager::registerAction(m_attachToUnstartedApplication,
          "Debugger.AttachToUnstartedProcess");
    cmd->setDescription(tr("Attach to Unstarted Application"));
    mstart->addAction(cmd, G_GENERAL);

    cmd = ActionManager::registerAction(m_startAndDebugApplicationAction,
        "Debugger.StartAndDebugApplication");
    cmd->setAttribute(Command::CA_Hide);
    mstart->addAction(cmd, G_GENERAL);

    cmd = ActionManager::registerAction(m_attachToCoreAction,
        "Debugger.AttachCore");
    cmd->setAttribute(Command::CA_Hide);
    mstart->addAction(cmd, Constants::G_GENERAL);

    cmd = ActionManager::registerAction(m_attachToRemoteServerAction,
        "Debugger.AttachToRemoteServer");
    cmd->setAttribute(Command::CA_Hide);
    mstart->addAction(cmd, Constants::G_SPECIAL);

    if (m_startRemoteCdbAction) {
        cmd = ActionManager::registerAction(m_startRemoteCdbAction,
             "Debugger.AttachRemoteCdb");
        cmd->setAttribute(Command::CA_Hide);
        mstart->addAction(cmd, Constants::G_SPECIAL);
    }

    mstart->addSeparator(Context(CC::C_GLOBAL), Constants::G_START_QML);

    cmd = ActionManager::registerAction(m_attachToQmlPortAction, "Debugger.AttachToQmlPort");
    cmd->setAttribute(Command::CA_Hide);
    mstart->addAction(cmd, Constants::G_START_QML);

    cmd = ActionManager::registerAction(m_detachAction, "Debugger.Detach");
    cmd->setAttribute(Command::CA_Hide);
    debugMenu->addAction(cmd, CC::G_DEFAULT_ONE);

    cmd = ActionManager::registerAction(m_interruptAction, Constants::INTERRUPT);
    cmd->setDescription(tr("Interrupt Debugger"));
    debugMenu->addAction(cmd, CC::G_DEFAULT_ONE);

    cmd = ActionManager::registerAction(m_continueAction, Constants::CONTINUE);
    cmd->setDefaultKeySequence(debugKey);
    debugMenu->addAction(cmd, CC::G_DEFAULT_ONE);

    cmd = ActionManager::registerAction(m_exitAction, Constants::STOP);
    debugMenu->addAction(cmd, CC::G_DEFAULT_ONE);
    m_hiddenStopAction = new ProxyAction(this);
    m_hiddenStopAction->initialize(cmd->action());
    m_hiddenStopAction->setAttribute(ProxyAction::UpdateText);
    m_hiddenStopAction->setAttribute(ProxyAction::UpdateIcon);

    cmd = ActionManager::registerAction(m_hiddenStopAction, Constants::HIDDEN_STOP);
    cmd->setDefaultKeySequence(QKeySequence(useMacShortcuts ? tr("Shift+Ctrl+Y") : tr("Shift+F5")));

    cmd = ActionManager::registerAction(m_abortAction, Constants::ABORT);
    cmd->setDescription(tr("Reset Debugger"));
    debugMenu->addAction(cmd, CC::G_DEFAULT_ONE);

    cmd = ActionManager::registerAction(m_resetAction, Constants::RESET);
    cmd->setDescription(tr("Restart Debugging"));
    debugMenu->addAction(cmd, CC::G_DEFAULT_ONE);

    debugMenu->addSeparator();

    cmd = ActionManager::registerAction(m_nextAction, Constants::NEXT);
    cmd->setDefaultKeySequence(QKeySequence(useMacShortcuts ? tr("Ctrl+Shift+O") : tr("F10")));
    cmd->setAttribute(Command::CA_Hide);
    cmd->setAttribute(Command::CA_UpdateText);
    debugMenu->addAction(cmd);

    cmd = ActionManager::registerAction(m_stepAction, Constants::STEP);
    cmd->setDefaultKeySequence(QKeySequence(useMacShortcuts ? tr("Ctrl+Shift+I") : tr("F11")));
    cmd->setAttribute(Command::CA_Hide);
    cmd->setAttribute(Command::CA_UpdateText);
    debugMenu->addAction(cmd);

    cmd = ActionManager::registerAction(m_stepOutAction, Constants::STEPOUT);
    cmd->setDefaultKeySequence(QKeySequence(useMacShortcuts ? tr("Ctrl+Shift+T") : tr("Shift+F11")));
    cmd->setAttribute(Command::CA_Hide);
    debugMenu->addAction(cmd);

    cmd = ActionManager::registerAction(m_runToLineAction,
        "Debugger.RunToLine", cppDebuggercontext);
    cmd->setDefaultKeySequence(QKeySequence(useMacShortcuts ? tr("Shift+F8") : tr("Ctrl+F10")));
    cmd->setAttribute(Command::CA_Hide);
    debugMenu->addAction(cmd);

    cmd = ActionManager::registerAction(m_runToSelectedFunctionAction,
        "Debugger.RunToSelectedFunction", cppDebuggercontext);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+F6")));
    cmd->setAttribute(Command::CA_Hide);
    // Don't add to menu by default as keeping its enabled state
    // and text up-to-date is a lot of hassle.
    // debugMenu->addAction(cmd);

    cmd = ActionManager::registerAction(m_jumpToLineAction,
        "Debugger.JumpToLine", cppDebuggercontext);
    cmd->setAttribute(Command::CA_Hide);
    debugMenu->addAction(cmd);

    cmd = ActionManager::registerAction(m_returnFromFunctionAction,
        "Debugger.ReturnFromFunction", cppDebuggercontext);
    cmd->setAttribute(Command::CA_Hide);
    debugMenu->addAction(cmd);

    if (isReverseDebuggingEnabled()) {
        cmd = ActionManager::registerAction(m_reverseDirectionAction,
                                            Constants::REVERSE, cppDebuggercontext);
        cmd->setDefaultKeySequence(QKeySequence(useMacShortcuts ? QString() : tr("F12")));
        cmd->setAttribute(Command::CA_Hide);
        debugMenu->addAction(cmd);
    }

    debugMenu->addSeparator();

    //cmd = ActionManager::registerAction(m_snapshotAction,
    //    "Debugger.Snapshot", cppDebuggercontext);
    //cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+D,Ctrl+S")));
    //cmd->setAttribute(Command::CA_Hide);
    //debugMenu->addAction(cmd);

    ActionManager::registerAction(m_frameDownAction,
        "Debugger.FrameDown", cppDebuggercontext);
    ActionManager::registerAction(m_frameUpAction,
        "Debugger.FrameUp", cppDebuggercontext);

    cmd = ActionManager::registerAction(m_operateByInstructionAction,
        Constants::OPERATE_BY_INSTRUCTION, cppDebuggercontext);
    cmd->setAttribute(Command::CA_Hide);
    debugMenu->addAction(cmd);

    cmd = ActionManager::registerAction(m_breakAction, "Debugger.ToggleBreak");
    cmd->setDefaultKeySequence(QKeySequence(useMacShortcuts ? tr("F8") : tr("F9")));
    debugMenu->addAction(cmd);
    connect(m_breakAction, &QAction::triggered,
        this, &DebuggerPluginPrivate::toggleBreakpointHelper);

    debugMenu->addSeparator();

    // currently broken
//    auto qmlUpdateOnSaveDummyAction = new QAction(tr("Apply Changes on Save"), this);
//    qmlUpdateOnSaveDummyAction->setCheckable(true);
//    qmlUpdateOnSaveDummyAction->setIcon(Icons::APPLY_ON_SAVE.icon());
//    qmlUpdateOnSaveDummyAction->setEnabled(false);
//    cmd = ActionManager::registerAction(qmlUpdateOnSaveDummyAction, Constants::QML_UPDATE_ON_SAVE);
//    debugMenu->addAction(cmd);

    auto qmlShowAppOnTopDummyAction = new QAction(tr("Show Application on Top"), this);
    qmlShowAppOnTopDummyAction->setCheckable(true);
    qmlShowAppOnTopDummyAction->setIcon(Icons::APP_ON_TOP.icon());
    qmlShowAppOnTopDummyAction->setEnabled(false);
    cmd = ActionManager::registerAction(qmlShowAppOnTopDummyAction, Constants::QML_SHOW_APP_ON_TOP);
    debugMenu->addAction(cmd);

    auto qmlSelectDummyAction = new QAction(tr("Select"), this);
    qmlSelectDummyAction->setCheckable(true);
    qmlSelectDummyAction->setIcon(Icons::SELECT.icon());
    qmlSelectDummyAction->setEnabled(false);
    cmd = ActionManager::registerAction(qmlSelectDummyAction, Constants::QML_SELECTTOOL);
    debugMenu->addAction(cmd);

    debugMenu->addSeparator();

    cmd = m_watchCommand = ActionManager::registerAction(m_watchAction, "Debugger.AddToWatch",
            Context(CppEditor::Constants::CPPEDITOR_ID,  QmlJSEditor::Constants::C_QMLJSEDITOR_ID));
    //cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+D,Ctrl+W")));
    debugMenu->addAction(cmd);

    addGdbOptionPages(&m_optionPages);
    addCdbOptionPages(&m_optionPages);
    m_optionPages.append(new LocalsAndExpressionsOptionsPage);

    connect(ModeManager::instance(), &ModeManager::currentModeChanged,
        this, &DebuggerPluginPrivate::onModeChanged);
    connect(ModeManager::instance(), &ModeManager::currentModeChanged,
            m_mainWindow.data(), &DebuggerMainWindow::onModeChanged);
    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::settingsChanged,
        this, &DebuggerPluginPrivate::updateDebugWithoutDeployMenu);

    m_mainWindow->finalizeSetup();

    // Debug mode setup
    m_mode = new DebugMode;
    m_modeWindow = createModeWindow(Constants::MODE_DEBUG, m_mainWindow);
    m_mode->setWidget(m_modeWindow);

    m_debugModeContext.setContext(Context(CC::C_EDITORMANAGER));
    m_debugModeContext.setWidget(m_modeWindow);
    ICore::addContextObject(&m_debugModeContext);

    connect(SessionManager::instance(), &SessionManager::startupProjectChanged,
            this, &DebuggerPluginPrivate::updateUiForProject);

    //
    //  Connections
    //

    // Core
    connect(ICore::instance(), &ICore::saveSettingsRequested,
            this, &DebuggerPluginPrivate::writeSettings);

    // TextEditor
    connect(TextEditorSettings::instance(), &TextEditorSettings::fontSettingsChanged,
            this, &DebuggerPluginPrivate::fontSettingsChanged);

    // ProjectExplorer
    connect(SessionManager::instance(), &SessionManager::sessionLoaded,
            this, &DebuggerPluginPrivate::sessionLoaded);
    connect(SessionManager::instance(), &SessionManager::aboutToSaveSession,
            this, &DebuggerPluginPrivate::aboutToSaveSession);
    connect(SessionManager::instance(), &SessionManager::aboutToUnloadSession,
            this, &DebuggerPluginPrivate::aboutToUnloadSession);
    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::updateRunActions,
            this, &DebuggerPluginPrivate::updateDebugActions);

    // EditorManager
    connect(EditorManager::instance(), &EditorManager::editorOpened,
            this, &DebuggerPluginPrivate::editorOpened);
    connect(EditorManager::instance(), &EditorManager::currentEditorChanged,
            this, &DebuggerPluginPrivate::updateBreakMenuItem);

    // Application interaction
    connect(action(SettingsDialog), &QAction::triggered,
            [] { ICore::showOptionsDialog(DEBUGGER_COMMON_SETTINGS_ID); });

    // Toolbar
    ToolbarDescription toolbar;
    toolbar.addAction(m_visibleStartAction);
    toolbar.addAction(ActionManager::command(Constants::STOP)->action(), Icons::DEBUG_EXIT_SMALL_TOOLBAR.icon());
    toolbar.addAction(ActionManager::command(Constants::NEXT)->action(), Icons::STEP_OVER_TOOLBAR.icon());
    toolbar.addAction(ActionManager::command(Constants::STEP)->action(), Icons::STEP_INTO_TOOLBAR.icon());
    toolbar.addAction(ActionManager::command(Constants::STEPOUT)->action(), Icons::STEP_OUT_TOOLBAR.icon());
    toolbar.addAction(ActionManager::command(Constants::RESET)->action(), Icons::RESTART_TOOLBAR.icon());
    toolbar.addAction(ActionManager::command(Constants::OPERATE_BY_INSTRUCTION)->action());

    if (isReverseDebuggingEnabled()) {
        m_reverseToolButton = new QToolButton;
        m_reverseToolButton->setDefaultAction(m_reverseDirectionAction);
        toolbar.addWidget(m_reverseToolButton);
    }

    toolbar.addWidget(new StyledSeparator);

    m_threadLabel = new QLabel(tr("Threads:"));
    toolbar.addWidget(m_threadLabel);

    m_threadBox = new QComboBox;
    m_threadBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    connect(m_threadBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::activated),
            this, &DebuggerPluginPrivate::selectThread);

    toolbar.addWidget(m_threadBox);
//    toolbar.addSpacerItem(new QSpacerItem(4, 0));

//    ToolbarDescription qmlToolbar
//    qmlToolbar.addAction(qmlUpdateOnSaveDummyAction);
//    qmlToolbar.addAction(qmlShowAppOnTopDummyAction, Icons::APP_ON_TOP_TOOLBAR.icon());
//    qmlToolbar.addWidget(new StyledSeparator);
//    qmlToolbar.addAction(qmlSelectDummyAction, Icons::SELECT_TOOLBAR.icon());
//    qmlToolbar.addWidget(new StyledSeparator);

    auto createBasePerspective = [this] { return new Perspective({}, {
        {DOCKWIDGET_STACK, m_stackWindow, {}, Perspective::SplitVertical},
        {DOCKWIDGET_BREAK, m_breakWindow, DOCKWIDGET_STACK, Perspective::SplitHorizontal},
        {DOCKWIDGET_THREADS, m_threadsWindow, DOCKWIDGET_BREAK, Perspective::AddToTab, false},
        {DOCKWIDGET_MODULES, m_modulesWindow, DOCKWIDGET_THREADS, Perspective::AddToTab, false},
        {DOCKWIDGET_SOURCE_FILES, m_sourceFilesWindow, DOCKWIDGET_MODULES, Perspective::AddToTab, false},
        {DOCKWIDGET_SNAPSHOTS, m_snapshotWindow, DOCKWIDGET_SOURCE_FILES, Perspective::AddToTab, false},
        {DOCKWIDGET_LOCALS_AND_INSPECTOR, m_localsAndInspectorWindow, {}, Perspective::AddToTab, true,
         Qt::RightDockWidgetArea},
        {DOCKWIDGET_WATCHERS, m_watchersWindow, DOCKWIDGET_LOCALS_AND_INSPECTOR, Perspective::AddToTab, true,
         Qt::RightDockWidgetArea},
        {DOCKWIDGET_OUTPUT, m_logWindow, {}, Perspective::AddToTab, false, Qt::TopDockWidgetArea},
        {DOCKWIDGET_BREAK, 0, {}, Perspective::Raise}
    }); };

    Perspective *cppPerspective = createBasePerspective();
    cppPerspective->setName(tr("Debugger"));
    cppPerspective->addOperation({DOCKWIDGET_REGISTER, m_registerWindow, DOCKWIDGET_SNAPSHOTS,
                                  Perspective::AddToTab, false});

    Debugger::registerToolbar(CppPerspectiveId, toolbar);
    Debugger::registerPerspective(CppPerspectiveId, cppPerspective);

//    Perspective *qmlPerspective = createBasePerspective();
//    qmlPerspective->setName(tr("QML Debugger"));
//    qmlPerspective->addOperation({ DOCKWIDGET_REGISTER, DOCKWIDGET_SNAPSHOTS,
//                                  Perspective::AddToTab, false });
//
//    Debugger::registerToolbar(QmlPerspectiveId, toolbarContainer);
//    Debugger::registerPerspective(QmlPerspectiveId, qmlPerspective);

    connect(action(EnableReverseDebugging), &SavedAction::valueChanged,
            this, &DebuggerPluginPrivate::enableReverseDebuggingTriggered);

    setInitialState();
    connectEngine(0);

    connect(SessionManager::instance(), &SessionManager::startupProjectChanged,
        this, &DebuggerPluginPrivate::onCurrentProjectChanged);

    m_optionPages.append(new CommonOptionsPage(m_globalDebuggerOptions));

    m_globalDebuggerOptions->fromSettings();
    m_returnWindow->setVisible(false);

    return true;
}

void setConfigValue(const QString &name, const QVariant &value)
{
    ICore::settings()->setValue("DebugMode/" + name, value);
}

QVariant configValue(const QString &name)
{
    return ICore::settings()->value("DebugMode/" + name);
}

void DebuggerPluginPrivate::onCurrentProjectChanged(Project *project)
{
    RunConfiguration *activeRc = nullptr;
    if (project) {
        Target *target = project->activeTarget();
        if (target)
            activeRc = target->activeRunConfiguration();
        if (!activeRc)
            return;
    }
    for (int i = 0, n = m_snapshotHandler->size(); i != n; ++i) {
        // Run controls might be deleted during exit.
        if (DebuggerRunTool *runTool = m_snapshotHandler->at(i)) {
            if (RunControl *runControl = runTool->runControl()) {
                RunConfiguration *rc = runControl->runConfiguration();
                if (rc == activeRc) {
                    m_snapshotHandler->setCurrentIndex(i);
                    updateState(runTool);
                    return;
                }
            }
        }
    }

    // If we have a running debugger, don't touch it.
    if (m_snapshotHandler->size())
        return;

    // No corresponding debugger found. So we are ready to start one.
    m_interruptAction->setEnabled(false);
    m_continueAction->setEnabled(false);
    m_exitAction->setEnabled(false);
    QString whyNot;
    const bool canRun = ProjectExplorerPlugin::canRunStartupProject(ProjectExplorer::Constants::DEBUG_RUN_MODE, &whyNot);
    m_startAction->setEnabled(canRun);
    m_startAction->setToolTip(whyNot);
    m_debugWithoutDeployAction->setEnabled(canRun);
    setProxyAction(m_visibleStartAction, Id(Constants::DEBUG));
}

void DebuggerPluginPrivate::attachCore()
{
    AttachCoreDialog dlg(ICore::dialogParent());

    const QString lastExternalKit = configValue("LastExternalKit").toString();
    if (!lastExternalKit.isEmpty())
        dlg.setKitId(Id::fromString(lastExternalKit));
    dlg.setLocalExecutableFile(configValue("LastExternalExecutableFile").toString());
    dlg.setLocalCoreFile(configValue("LastLocalCoreFile").toString());
    dlg.setRemoteCoreFile(configValue("LastRemoteCoreFile").toString());
    dlg.setOverrideStartScript(configValue("LastExternalStartScript").toString());
    dlg.setForceLocalCoreFile(configValue("LastForceLocalCoreFile").toBool());

    if (dlg.exec() != QDialog::Accepted)
        return;

    setConfigValue("LastExternalExecutableFile", dlg.localExecutableFile());
    setConfigValue("LastLocalCoreFile", dlg.localCoreFile());
    setConfigValue("LastRemoteCoreFile", dlg.remoteCoreFile());
    setConfigValue("LastExternalKit", dlg.kit()->id().toSetting());
    setConfigValue("LastExternalStartScript", dlg.overrideStartScript());
    setConfigValue("LastForceLocalCoreFile", dlg.forcesLocalCoreFile());

    IDevice::ConstPtr device = DeviceKitInformation::device(dlg.kit());
    auto runControl = new RunControl(device, ProjectExplorer::Constants::DEBUG_RUN_MODE);
    auto debugger = new DebuggerRunTool(runControl, dlg.kit());
    debugger->setInferiorExecutable(dlg.localExecutableFile());
    debugger->setCoreFileName(dlg.localCoreFile());
    debugger->setRunControlName(tr("Core file \"%1\"")
        .arg(dlg.useLocalCoreFile() ? dlg.localCoreFile() : dlg.remoteCoreFile()));
    debugger->setStartMode(AttachCore);
    debugger->setCloseMode(DetachAtClose);
    debugger->setOverrideStartScript(dlg.overrideStartScript());
    debugger->startRunControl();
}

void DebuggerPluginPrivate::startRemoteCdbSession()
{
    const QString connectionKey = "CdbRemoteConnection";
    Kit *kit = findUniversalCdbKit();
    QTC_ASSERT(kit, return);

    StartRemoteCdbDialog dlg(ICore::dialogParent());
    QString previousConnection = configValue(connectionKey).toString();
    if (previousConnection.isEmpty())
        previousConnection = "localhost:1234";
    dlg.setConnection(previousConnection);
    if (dlg.exec() != QDialog::Accepted)
        return;
    setConfigValue(connectionKey, dlg.connection());

    IDevice::ConstPtr device = DeviceKitInformation::device(kit);
    auto runControl = new RunControl(device, ProjectExplorer::Constants::DEBUG_RUN_MODE);
    auto debugger = new DebuggerRunTool(runControl, kit);
    debugger->setStartMode(AttachToRemoteServer);
    debugger->setCloseMode(KillAtClose);
    debugger->setRemoteChannel(dlg.connection());
    debugger->startRunControl();
}

class RemoteAttachRunner : public DebuggerRunTool
{
public:
    RemoteAttachRunner(RunControl *runControl, Kit *kit, int pid)
        : DebuggerRunTool(runControl, kit)
    {
        IDevice::ConstPtr device = DeviceKitInformation::device(kit);
        setDisplayName("AttachToRunningProcess");
        setUsePortsGatherer(true, false);
        portsGatherer()->setDevice(device);

        auto gdbServer = new GdbServerRunner(runControl, portsGatherer());
        gdbServer->setUseMulti(false);
        gdbServer->setDevice(device);
        gdbServer->setAttachPid(ProcessHandle(pid));

        addStartDependency(gdbServer);

        setStartMode(AttachToRemoteProcess);
        setCloseMode(DetachAtClose);

        //    setInferiorExecutable(localExecutable);
        setUseContinueInsteadOfRun(true);
        setContinueAfterAttach(false);
    }
};

void DebuggerPluginPrivate::attachToRunningApplication()
{
    auto kitChooser = new DebuggerKitChooser(DebuggerKitChooser::AnyDebugging);

    auto dlg = new DeviceProcessesDialog(kitChooser, ICore::dialogParent());
    dlg->addAcceptButton(DeviceProcessesDialog::tr("&Attach to Process"));
    dlg->showAllDevices();
    if (dlg->exec() == QDialog::Rejected) {
        delete dlg;
        return;
    }

    dlg->setAttribute(Qt::WA_DeleteOnClose);
    Kit *kit = kitChooser->currentKit();
    QTC_ASSERT(kit, return);
    IDevice::ConstPtr device = DeviceKitInformation::device(kit);
    QTC_ASSERT(device, return);

    DeviceProcessItem process = dlg->currentProcess();

    if (device->type() == PE::DESKTOP_DEVICE_TYPE) {
        attachToRunningProcess(kit, process, false);
    } else {
        auto runControl = new RunControl(device, ProjectExplorer::Constants::DEBUG_RUN_MODE);
        auto debugger = new RemoteAttachRunner(runControl, kit, process.pid);
        debugger->startRunControl();
    }
}

void DebuggerPluginPrivate::attachToUnstartedApplicationDialog()
{
    auto dlg = new UnstartedAppWatcherDialog(ICore::dialogParent());

    connect(dlg, &QDialog::finished, dlg, &QObject::deleteLater);
    connect(dlg, &UnstartedAppWatcherDialog::processFound, this, [this, dlg] {
        RunControl *rc = attachToRunningProcess(dlg->currentKit(),
                                                dlg->currentProcess(),
                                                dlg->continueOnAttach());
        if (!rc)
            return;

        if (dlg->hideOnAttach())
            connect(rc, &RunControl::stopped, dlg, &UnstartedAppWatcherDialog::startWatching);
    });

    dlg->show();
}

RunControl *DebuggerPluginPrivate::attachToRunningProcess(Kit *kit,
    DeviceProcessItem process, bool contAfterAttach)
{
    QTC_ASSERT(kit, return 0);
    IDevice::ConstPtr device = DeviceKitInformation::device(kit);
    QTC_ASSERT(device, return 0);
    if (process.pid == 0) {
        AsynchronousMessageBox::warning(tr("Warning"), tr("Cannot attach to process with PID 0"));
        return 0;
    }

    const Abi tcAbi = ToolChainKitInformation::targetAbi(kit);
    const bool isWindows = (tcAbi.os() == Abi::WindowsOS);
    if (isWindows && isWinProcessBeingDebugged(process.pid)) {
        AsynchronousMessageBox::warning(
                    tr("Process Already Under Debugger Control"),
                    tr("The process %1 is already under the control of a debugger.\n"
                       "%2 cannot attach to it.").arg(process.pid)
                    .arg(Core::Constants::IDE_DISPLAY_NAME));
        return 0;
    }

    if (device->type() != PE::DESKTOP_DEVICE_TYPE) {
        AsynchronousMessageBox::warning(tr("Not a Desktop Device Type"),
                             tr("It is only possible to attach to a locally running process."));
        return 0;
    }

    auto runControl = new RunControl(nullptr, ProjectExplorer::Constants::DEBUG_RUN_MODE);
    auto debugger = new DebuggerRunTool(runControl, kit);
    debugger->setAttachPid(ProcessHandle(process.pid));
    debugger->setRunControlName(tr("Process %1").arg(process.pid));
    debugger->setInferiorExecutable(process.exe);
    debugger->setInferiorDevice(device);
    debugger->setStartMode(AttachExternal);
    debugger->setCloseMode(DetachAtClose);
    debugger->setContinueAfterAttach(contAfterAttach);

    debugger->startRunControl();

    return debugger->runControl();
}

void DebuggerPlugin::attachExternalApplication(RunControl *rc)
{
    ProcessHandle pid = rc->applicationProcessHandle();
    RunConfiguration *runConfig = rc->runConfiguration();
    auto runControl = new RunControl(runConfig, ProjectExplorer::Constants::DEBUG_RUN_MODE);
    auto debugger = new DebuggerRunTool(runControl, guessKitFromAbis({rc->abi()}), false);
    debugger->setAttachPid(pid);
    debugger->setRunControlName(tr("Process %1").arg(pid.pid()));
    debugger->setStartMode(AttachExternal);
    debugger->setCloseMode(DetachAtClose);
    debugger->startRunControl();
}

void DebuggerPlugin::getEnginesState(QByteArray *json) const
{
    QTC_ASSERT(json, return);
    QVariantMap result {
        {"version", 1}
    };
    QVariantMap states;

    for (int i = 0; i < dd->m_snapshotHandler->size(); ++i) {
        const DebuggerEngine *engine = dd->m_snapshotHandler->at(i)->engine();
        states[QString::number(i)] = QVariantMap({
                   {"current", dd->m_snapshotHandler->currentIndex() == i},
                   {"pid", engine->inferiorPid()},
                   {"state", engine->state()}
        });
    }

    if (!states.isEmpty())
        result["states"] = states;

    *json = QJsonDocument(QJsonObject::fromVariantMap(result)).toJson();
}

void DebuggerPluginPrivate::attachToQmlPort()
{
    AttachToQmlPortDialog dlg(ICore::mainWindow());

    const QVariant qmlServerPort = configValue("LastQmlServerPort");
    if (qmlServerPort.isValid())
        dlg.setPort(qmlServerPort.toInt());
    else
        dlg.setPort(-1);

    const Id kitId = Id::fromSetting(configValue("LastProfile"));
    if (kitId.isValid())
        dlg.setKitId(kitId);

    if (dlg.exec() != QDialog::Accepted)
        return;

    Kit *kit = dlg.kit();
    QTC_ASSERT(kit, return);
    setConfigValue("LastQmlServerPort", dlg.port());
    setConfigValue("LastProfile", kit->id().toSetting());

    IDevice::ConstPtr device = DeviceKitInformation::device(kit);
    QTC_ASSERT(device, return);

    auto runControl = new RunControl(nullptr, ProjectExplorer::Constants::DEBUG_RUN_MODE);
    auto debugger = new DebuggerRunTool(runControl, kit);

    QUrl qmlServer = device->toolControlChannel(IDevice::QmlControlChannel);
    qmlServer.setPort(dlg.port());
    debugger->setQmlServer(qmlServer);

    QSsh::SshConnectionParameters sshParameters = device->sshParameters();
    debugger->setRemoteChannel(sshParameters.host(), sshParameters.port());
    debugger->setStartMode(AttachToQmlServer);

    debugger->startRunControl();
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
    for (DebuggerRunTool *debugger : m_scheduledStarts)
        debugger->startRunControl();
}

void DebuggerPluginPrivate::editorOpened(IEditor *editor)
{
    if (auto widget = qobject_cast<TextEditorWidget *>(editor->widget())) {
        connect(widget, &TextEditorWidget::markRequested,
                this, &DebuggerPluginPrivate::requestMark);

        connect(widget, &TextEditorWidget::markContextMenuRequested,
                this, &DebuggerPluginPrivate::requestContextMenu);
    }
}

void DebuggerPluginPrivate::updateBreakMenuItem(IEditor *editor)
{
    BaseTextEditor *textEditor = qobject_cast<BaseTextEditor *>(editor);
    m_breakAction->setEnabled(textEditor != 0);
}

void DebuggerPluginPrivate::requestContextMenu(TextEditorWidget *widget,
    int lineNumber, QMenu *menu)
{
    Breakpoint bp;
    TextDocument *document = widget->textDocument();

    ContextData args = getLocationContext(document, lineNumber);
    if (args.type == LocationByAddress) {
        BreakpointResponse needle;
        needle.type = BreakpointByAddress;
        needle.address = args.address;
        needle.lineNumber = -1;
        bp = breakHandler()->findSimilarBreakpoint(needle);
    } else if (args.type == LocationByFile) {
        bp = breakHandler()->findBreakpointByFileAndLine(args.fileName, lineNumber);
        if (!bp)
            bp = breakHandler()->findBreakpointByFileAndLine(args.fileName, lineNumber, false);
    }

    if (bp) {
        QString id = bp.id().toString();

        // Remove existing breakpoint.
        auto act = menu->addAction(tr("Remove Breakpoint %1").arg(id));
        connect(act, &QAction::triggered, [bp] { bp.removeBreakpoint(); });

        // Enable/disable existing breakpoint.
        if (bp.isEnabled()) {
            act = menu->addAction(tr("Disable Breakpoint %1").arg(id));
            connect(act, &QAction::triggered, [bp] { bp.setEnabled(false); });
        } else {
            act = menu->addAction(tr("Enable Breakpoint %1").arg(id));
            connect(act, &QAction::triggered, [bp] { bp.setEnabled(true); });
        }

        // Edit existing breakpoint.
        act = menu->addAction(tr("Edit Breakpoint %1...").arg(id));
        connect(act, &QAction::triggered, [bp] {
            breakHandler()->editBreakpoint(bp, ICore::dialogParent());
        });

    } else {
        // Handle non-existing breakpoint.
        const QString text = args.address
            ? tr("Set Breakpoint at 0x%1").arg(args.address, 0, 16)
            : tr("Set Breakpoint at Line %1").arg(lineNumber);
        auto act = menu->addAction(text);
        act->setEnabled(args.isValid());
        connect(act, &QAction::triggered, [this, args] {
            breakpointSetMarginActionTriggered(false, args);
        });

        // Message trace point
        const QString tracePointText = args.address
            ? tr("Set Message Tracepoint at 0x%1...").arg(args.address, 0, 16)
            : tr("Set Message Tracepoint at Line %1...").arg(lineNumber);
        act = menu->addAction(tracePointText);
        act->setEnabled(args.isValid());
        connect(act, &QAction::triggered, [this, args] {
            breakpointSetMarginActionTriggered(true, args);
        });
    }

    // Run to, jump to line below in stopped state.
    DebuggerEngine *engine = currentEngine();
    QTC_ASSERT(engine, return);
    if (engine->state() == InferiorStopOk && args.isValid()) {
        menu->addSeparator();
        if (engine->hasCapability(RunToLineCapability)) {
            auto act = menu->addAction(args.address
                ? DebuggerEngine::tr("Run to Address 0x%1").arg(args.address, 0, 16)
                : DebuggerEngine::tr("Run to Line %1").arg(args.lineNumber));
            connect(act, &QAction::triggered, this, [args] {
                DebuggerEngine *engine = currentEngine();
                QTC_ASSERT(engine, return);
                engine->executeRunToLine(args);
            });
        }
        if (engine->hasCapability(JumpToLineCapability)) {
            auto act = menu->addAction(args.address
                ? DebuggerEngine::tr("Jump to Address 0x%1").arg(args.address, 0, 16)
                : DebuggerEngine::tr("Jump to Line %1").arg(args.lineNumber));
            connect(act, &QAction::triggered, this, [args] {
                DebuggerEngine *engine = currentEngine();
                QTC_ASSERT(engine, return);
                engine->executeJumpToLine(args);
            });
        }
        // Disassemble current function in stopped state.
        if (engine->hasCapability(DisassemblerCapability)) {
            StackFrame frame;
            frame.function = cppFunctionAt(args.fileName, lineNumber, 1);
            frame.line = 42; // trick gdb into mixed mode.
            if (!frame.function.isEmpty()) {
                const QString text = tr("Disassemble Function \"%1\"")
                    .arg(frame.function);
                auto act = new QAction(text, menu);
                connect(act, &QAction::triggered, this, [frame] {
                    DebuggerEngine *engine = currentEngine();
                    QTC_ASSERT(engine, return);
                    engine->openDisassemblerView(Location(frame));
                });
                menu->addAction(act);
            }
        }
    }
}

void DebuggerPluginPrivate::toggleBreakpoint(const ContextData &location, const QString &tracePointMessage)
{
    QTC_ASSERT(location.isValid(), return);
    BreakHandler *handler = m_breakHandler;
    Breakpoint bp;
    if (location.type == LocationByFile) {
        bp = handler->findBreakpointByFileAndLine(location.fileName, location.lineNumber, true);
        if (!bp)
            bp = handler->findBreakpointByFileAndLine(location.fileName, location.lineNumber, false);
    } else if (location.type == LocationByAddress) {
        bp = handler->findBreakpointByAddress(location.address);
    }

    if (bp) {
        bp.removeBreakpoint();
    } else {
        BreakpointParameters data;
        if (location.type == LocationByFile) {
            data.type = BreakpointByFileAndLine;
            if (boolSetting(BreakpointsFullPathByDefault))
                data.pathUsage = BreakpointUseFullPath;
            data.tracepoint = !tracePointMessage.isEmpty();
            data.message = tracePointMessage;
            data.fileName = location.fileName;
            data.lineNumber = location.lineNumber;
        } else if (location.type == LocationByAddress) {
            data.type = BreakpointByAddress;
            data.tracepoint = !tracePointMessage.isEmpty();
            data.message = tracePointMessage;
            data.address = location.address;
        }
        handler->appendBreakpoint(data);
    }
}

void DebuggerPluginPrivate::toggleBreakpointHelper()
{
    BaseTextEditor *textEditor = BaseTextEditor::currentTextEditor();
    QTC_ASSERT(textEditor, return);
    const int lineNumber = textEditor->currentLine();
    ContextData location = getLocationContext(textEditor->textDocument(), lineNumber);
    if (location.isValid())
        toggleBreakpoint(location);
}

void DebuggerPluginPrivate::requestMark(TextEditorWidget *widget, int lineNumber,
                                        TextMarkRequestKind kind)
{
    if (kind == BreakpointRequest) {
        ContextData location = getLocationContext(widget->textDocument(), lineNumber);
        if (location.isValid())
            toggleBreakpoint(location);
    }
}

// If updateEngine is set, the engine will update its threads/modules and so forth.
void DebuggerPluginPrivate::displayDebugger(DebuggerRunTool *runTool)
{
    QTC_ASSERT(runTool, return);
    DebuggerEngine *engine = runTool ? runTool->engine() : dummyEngine();
    QTC_ASSERT(engine, return);

    disconnectEngine();
    connectEngine(runTool);
    engine->updateAll();
    engine->updateViews();
}

void DebuggerPluginPrivate::connectEngine(DebuggerRunTool *runTool)
{
    if (m_shuttingDown)
        return;

    m_currentRunTool = runTool;
    DebuggerEngine *engine = currentEngine();
    QTC_ASSERT(engine, return);

    if (m_previouslyActiveEngine == engine)
        return;

    if (m_previouslyActiveEngine)
        m_previouslyActiveEngine->resetLocation();

    m_previouslyActiveEngine = engine;

    m_localsView->setModel(engine->watchModel());
    m_modulesView->setModel(engine->modulesModel());
    m_registerView->setModel(engine->registerModel());
    m_returnView->setModel(engine->watchModel());
    m_sourceFilesView->setModel(engine->sourceFilesModel());
    m_stackView->setModel(engine->stackModel());
    m_threadsView->setModel(engine->threadsModel());
    m_watchersView->setModel(engine->watchModel());
    m_inspectorView->setModel(engine->watchModel());

    engine->watchHandler()->resetWatchers();
    m_localsView->hideProgressIndicator();
    updateActiveLanguages();
}

static void changeFontSize(QWidget *widget, qreal size)
{
    QFont font = widget->font();
    font.setPointSizeF(size);
    widget->setFont(font);
}

void DebuggerPluginPrivate::fontSettingsChanged(const FontSettings &settings)
{
    if (!boolSetting(FontSizeFollowsEditor))
        return;
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
    changeFontSize(m_inspectorWindow, size);
}

void DebuggerPluginPrivate::cleanupViews()
{
    m_reverseDirectionAction->setChecked(false);
    m_reverseDirectionAction->setEnabled(false);

    const bool closeSource = boolSetting(CloseSourceBuffersOnExit);
    const bool closeMemory = boolSetting(CloseMemoryBuffersOnExit);

    QList<IDocument *> toClose;
    foreach (IDocument *document, DocumentModel::openedDocuments()) {
        const bool isMemory = document->property(Constants::OPENED_WITH_DISASSEMBLY).toBool();
        if (document->property(Constants::OPENED_BY_DEBUGGER).toBool()) {
            bool keepIt = true;
            if (document->isModified())
                keepIt = true;
            else if (document->filePath().toString().contains("qeventdispatcher"))
                keepIt = false;
            else if (isMemory)
                keepIt = !closeMemory;
            else
                keepIt = !closeSource;

            if (keepIt)
                document->setProperty(Constants::OPENED_BY_DEBUGGER, false);
            else
                toClose.append(document);
        }
    }
    EditorManager::closeDocuments(toClose);
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
}

void DebuggerPluginPrivate::setInitialState()
{
    m_returnWindow->setVisible(false);
    setBusyCursor(false);
    m_reverseDirectionAction->setChecked(false);
    m_reverseDirectionAction->setEnabled(false);
    m_toolTipManager.closeAllToolTips();

    m_startAndDebugApplicationAction->setEnabled(true);
    m_attachToQmlPortAction->setEnabled(true);
    m_attachToCoreAction->setEnabled(true);
    m_attachToRemoteServerAction->setEnabled(true);
    m_attachToRunningApplication->setEnabled(true);
    m_attachToUnstartedApplication->setEnabled(true);
    m_detachAction->setEnabled(false);

    m_watchAction->setEnabled(true);
    m_breakAction->setEnabled(false);
    //m_snapshotAction->setEnabled(false);
    m_operateByInstructionAction->setEnabled(false);

    m_exitAction->setEnabled(false);
    m_abortAction->setEnabled(false);
    m_resetAction->setEnabled(false);

    m_interruptAction->setEnabled(false);
    m_continueAction->setEnabled(false);

    m_stepAction->setEnabled(true);
    m_stepOutAction->setEnabled(false);
    m_runToLineAction->setEnabled(false);
    m_runToSelectedFunctionAction->setEnabled(true);
    m_returnFromFunctionAction->setEnabled(false);
    m_jumpToLineAction->setEnabled(false);
    m_nextAction->setEnabled(true);

    action(AutoDerefPointers)->setEnabled(true);
    action(ExpandStack)->setEnabled(false);

    m_threadLabel->setEnabled(false);
}

void DebuggerPluginPrivate::updateState(DebuggerRunTool *runTool)
{
    if (m_shuttingDown)
        return;
    QTC_ASSERT(runTool, return);
    DebuggerEngine *engine = runTool->engine();
    QTC_ASSERT(engine, return);
    QTC_ASSERT(m_watchersView->model(), return);
    QTC_ASSERT(m_returnView->model(), return);
    QTC_ASSERT(!engine->isSlaveEngine(), return);

    m_threadBox->setCurrentIndex(engine->threadsHandler()->currentThreadIndex());

    const DebuggerState state = engine->state();
    //showMessage(QString::fromLatin1("PLUGIN SET STATE: ")
    //    + DebuggerEngine::stateName(state), LogStatus);
    //qDebug() << "PLUGIN SET STATE: " << state;

    static DebuggerState previousState = DebuggerNotReady;
    if (state == previousState)
        return;

    bool actionsEnabled = DebuggerEngine::debuggerActionsEnabled(state);

    if (state == DebuggerNotReady) {
        QTC_ASSERT(false, /* We use the Core's m_debugAction here */);
        // F5 starts debugging. It is "startable".
        m_interruptAction->setEnabled(false);
        m_continueAction->setEnabled(false);
        m_exitAction->setEnabled(false);
        m_startAction->setEnabled(true);
        m_debugWithoutDeployAction->setEnabled(true);
        setProxyAction(m_visibleStartAction, Id(Constants::DEBUG));
        m_hiddenStopAction->setAction(m_undisturbableAction);
    } else if (state == InferiorStopOk) {
        // F5 continues, Shift-F5 kills. It is "continuable".
        m_interruptAction->setEnabled(false);
        m_continueAction->setEnabled(true);
        m_exitAction->setEnabled(true);
        m_startAction->setEnabled(false);
        m_debugWithoutDeployAction->setEnabled(false);
        setProxyAction(m_visibleStartAction, Id(Constants::CONTINUE));
        m_hiddenStopAction->setAction(m_exitAction);
        m_localsAndInspectorWindow->setShowLocals(true);
    } else if (state == InferiorRunOk) {
        // Shift-F5 interrupts. It is also "interruptible".
        m_interruptAction->setEnabled(true);
        m_continueAction->setEnabled(false);
        m_exitAction->setEnabled(true);
        m_startAction->setEnabled(false);
        m_debugWithoutDeployAction->setEnabled(false);
        setProxyAction(m_visibleStartAction, Id(Constants::INTERRUPT));
        m_hiddenStopAction->setAction(m_interruptAction);
        m_localsAndInspectorWindow->setShowLocals(false);
    } else if (state == DebuggerFinished) {
        const bool canRun = ProjectExplorerPlugin::canRunStartupProject(ProjectExplorer::Constants::DEBUG_RUN_MODE);
        // We don't want to do anything anymore.
        m_interruptAction->setEnabled(false);
        m_continueAction->setEnabled(false);
        m_exitAction->setEnabled(false);
        m_startAction->setEnabled(canRun);
        m_debugWithoutDeployAction->setEnabled(canRun);
        setProxyAction(m_visibleStartAction, Id(Constants::DEBUG));
        m_hiddenStopAction->setAction(m_undisturbableAction);
        setBusyCursor(false);
        cleanupViews();
    } else if (state == InferiorUnrunnable) {
        // We don't want to do anything anymore.
        m_interruptAction->setEnabled(false);
        m_continueAction->setEnabled(false);
        m_exitAction->setEnabled(true);
        m_startAction->setEnabled(false);
        m_debugWithoutDeployAction->setEnabled(false);
        m_visibleStartAction->setAction(m_undisturbableAction);
        m_hiddenStopAction->setAction(m_exitAction);
        // show locals in core dumps
        m_localsAndInspectorWindow->setShowLocals(true);
    } else {
        // Everything else is "undisturbable".
        m_interruptAction->setEnabled(false);
        m_continueAction->setEnabled(false);
        m_exitAction->setEnabled(false);
        m_startAction->setEnabled(false);
        m_debugWithoutDeployAction->setEnabled(false);
        m_visibleStartAction->setAction(m_undisturbableAction);
        m_hiddenStopAction->setAction(m_undisturbableAction);
    }

    m_startAndDebugApplicationAction->setEnabled(true);
    m_attachToQmlPortAction->setEnabled(true);
    m_attachToCoreAction->setEnabled(true);
    m_attachToRemoteServerAction->setEnabled(true);
    m_attachToRunningApplication->setEnabled(true);
    m_attachToUnstartedApplication->setEnabled(true);

    m_threadBox->setEnabled(state == InferiorStopOk || state == InferiorUnrunnable);
    m_threadLabel->setEnabled(m_threadBox->isEnabled());

    const bool isCore = runTool->runParameters().startMode == AttachCore;
    const bool stopped = state == InferiorStopOk;
    const bool detachable = stopped && !isCore;
    m_detachAction->setEnabled(detachable);

    if (stopped)
        QApplication::alert(mainWindow(), 3000);

    const bool canReverse = engine->hasCapability(ReverseSteppingCapability)
                && boolSetting(EnableReverseDebugging);
    m_reverseDirectionAction->setEnabled(canReverse);

    m_watchAction->setEnabled(true);
    m_breakAction->setEnabled(true);

    const bool canOperateByInstruction = engine->hasCapability(OperateByInstructionCapability)
            && (stopped || isCore);
    m_operateByInstructionAction->setEnabled(canOperateByInstruction);

    m_abortAction->setEnabled(state != DebuggerNotReady
                                      && state != DebuggerFinished);
    m_resetAction->setEnabled((stopped || state == DebuggerNotReady)
                              && engine->hasCapability(ResetInferiorCapability));

    m_stepAction->setEnabled(stopped || state == DebuggerNotReady);
    m_nextAction->setEnabled(stopped || state == DebuggerNotReady);
    m_stepAction->setToolTip(QString());
    m_nextAction->setToolTip(QString());

    m_stepOutAction->setEnabled(stopped);
    m_runToLineAction->setEnabled(stopped && engine->hasCapability(RunToLineCapability));
    m_runToSelectedFunctionAction->setEnabled(stopped);
    m_returnFromFunctionAction->
        setEnabled(stopped && engine->hasCapability(ReturnFromFunctionCapability));

    const bool canJump = stopped && engine->hasCapability(JumpToLineCapability);
    m_jumpToLineAction->setEnabled(canJump);

    const bool canDeref = actionsEnabled && engine->hasCapability(AutoDerefPointersCapability);
    action(AutoDerefPointers)->setEnabled(canDeref);
    action(AutoDerefPointers)->setEnabled(true);
    action(ExpandStack)->setEnabled(actionsEnabled);

    const bool notbusy = state == InferiorStopOk
        || state == DebuggerNotReady
        || state == DebuggerFinished
        || state == InferiorUnrunnable;
    setBusyCursor(!notbusy);
}

void DebuggerPluginPrivate::updateDebugActions()
{
    if (m_shuttingDown)
        return;
    //if we're currently debugging the actions are controlled by engine
    if (m_currentRunTool && m_currentRunTool->engine()->state() != DebuggerNotReady)
        return;

    QString whyNot;
    const bool canRun = ProjectExplorerPlugin::canRunStartupProject(ProjectExplorer::Constants::DEBUG_RUN_MODE, &whyNot);
    m_startAction->setEnabled(canRun);
    m_startAction->setToolTip(whyNot);
    m_debugWithoutDeployAction->setEnabled(canRun);

    // Step into/next: Start and break at 'main' unless a debugger is running.
    if (m_snapshotHandler->currentIndex() < 0) {
        m_stepAction->setEnabled(canRun);
        m_nextAction->setEnabled(canRun);
        if (canRun) {
            Project *project = SessionManager::startupProject();
            QTC_ASSERT(project, return);
            whyNot = tr("Start \"%1\" and break at function \"main()\"")
                    .arg(project->displayName());
        }
        m_stepAction->setToolTip(whyNot);
        m_nextAction->setToolTip(whyNot);
    }
}

void DebuggerPluginPrivate::updateDebugWithoutDeployMenu()
{
    const bool state = ProjectExplorerPlugin::projectExplorerSettings().deployBeforeRun;
    m_debugWithoutDeployAction->setVisible(state);
}

void DebuggerPluginPrivate::dumpLog()
{
    QString fileName = QFileDialog::getSaveFileName(ICore::mainWindow(),
        tr("Save Debugger Log"), Utils::TemporaryDirectory::masterDirectoryPath());
    if (fileName.isEmpty())
        return;
    FileSaver saver(fileName);
    if (!saver.hasError()) {
        QTextStream ts(saver.file());
        ts << m_logWindow->inputContents();
        ts << "\n\n=======================================\n\n";
        ts << m_logWindow->combinedContents();
        saver.setResult(&ts);
    }
    saver.finalize(ICore::mainWindow());
}

/*! Activates the previous mode when the current mode is the debug mode. */
void DebuggerPluginPrivate::activatePreviousMode()
{
    if (ModeManager::currentMode() == MODE_DEBUG && m_previousMode.isValid()) {
        // If stopping the application also makes Qt Creator active (as the
        // "previously active application"), doing the switch synchronously
        // leads to funny effects with floating dock widgets
        const Core::Id mode = m_previousMode;
        QTimer::singleShot(0, this, [mode]() { ModeManager::activateMode(mode); });
        m_previousMode = Id();
    }
}

void DebuggerPluginPrivate::activateDebugMode()
{
    m_reverseDirectionAction->setChecked(false);
    m_reverseDirectionAction->setEnabled(false);
    ModeManager::activateMode(MODE_DEBUG);
}

void DebuggerPluginPrivate::sessionLoaded()
{
    m_breakHandler->loadSessionData();
    dummyEngine()->watchHandler()->loadSessionData();
    DebuggerToolTipManager::loadSessionData();
}

void DebuggerPluginPrivate::aboutToUnloadSession()
{
    m_toolTipManager.sessionAboutToChange();
}

void DebuggerPluginPrivate::aboutToSaveSession()
{
    dummyEngine()->watchHandler()->saveSessionData();
    m_breakHandler->saveSessionData();
    DebuggerToolTipManager::saveSessionData();
}

void DebuggerPluginPrivate::showStatusMessage(const QString &msg0, int timeout)
{
    if (m_shuttingDown)
        return;
    showMessage(msg0, LogStatus);
    QString msg = msg0;
    msg.replace(QChar::LineFeed, QLatin1String("; "));
    m_mainWindow->showStatusMessage(msg, timeout);
}

void DebuggerPluginPrivate::aboutToShutdown()
{
    m_shuttingDown = true;

    disconnect(SessionManager::instance(), &SessionManager::startupProjectChanged, this, nullptr);

    m_mainWindow->saveCurrentPerspective();
    m_shutdownTimer.setInterval(0);
    m_shutdownTimer.setSingleShot(true);
    connect(&m_shutdownTimer, &QTimer::timeout, this, &DebuggerPluginPrivate::doShutdown);
    if (dd->m_currentRunTool) {
        if (dd->m_currentRunTool->engine()->state() != Debugger::DebuggerNotReady) {
            dd->m_currentRunTool->abortDebugger();
            m_shutdownTimer.setInterval(3000);
        }
    }
    m_shutdownTimer.start();
}

void setSessionValue(const QByteArray &key, const QVariant &value)
{
    SessionManager::setValue(QString::fromUtf8(key), value);
}

QVariant sessionValue(const QByteArray &key)
{
    return SessionManager::value(QString::fromUtf8(key));
}

WatchTreeView *inspectorView()
{
    return dd->m_inspectorView;
}

void DebuggerPluginPrivate::showMessage(const QString &msg, int channel, int timeout)
{
    if (m_shuttingDown)
        return;
    //qDebug() << "PLUGIN OUTPUT: " << channel << msg;
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
        case LogError:
            m_logWindow->showInput(LogError, QLatin1String("ERROR: ") + msg);
            m_logWindow->showOutput(LogError, QLatin1String("ERROR: ") + msg);
            break;
        default:
            m_logWindow->showOutput(channel, msg);
            break;
    }
}

static void createNewDock(QWidget *widget)
{
    auto dockWidget = new QDockWidget;
    dockWidget->setWidget(widget);
    dockWidget->setWindowTitle(widget->windowTitle());
    dockWidget->setFeatures(QDockWidget::DockWidgetClosable);
    dockWidget->show();
}

static QString formatStartParameters(const DebuggerRunTool *debugger)
{
    const DebuggerRunParameters &sp = debugger->runParameters();
    QString rc;
    QTextStream str(&rc);
    str << "Start parameters: '" << sp.displayName << "' mode: " << sp.startMode
        << "\nABI: " << sp.toolChainAbi.toString() << '\n';
    str << "Languages: ";
    if (sp.isCppDebugging)
        str << "c++ ";
    if (sp.isQmlDebugging)
        str << "qml";
    str << '\n';
    if (!sp.inferior.executable.isEmpty()) {
        str << "Executable: " << QDir::toNativeSeparators(sp.inferior.executable)
            << ' ' << sp.inferior.commandLineArguments;
        if (debugger->terminalRunner())
            str << " [terminal]";
        str << '\n';
        if (!sp.inferior.workingDirectory.isEmpty())
            str << "Directory: " << QDir::toNativeSeparators(sp.inferior.workingDirectory)
                << '\n';
    }
    QString cmd = sp.debugger.executable;
    if (!cmd.isEmpty())
        str << "Debugger: " << QDir::toNativeSeparators(cmd) << '\n';
    if (!sp.coreFile.isEmpty())
        str << "Core: " << QDir::toNativeSeparators(sp.coreFile) << '\n';
    if (sp.attachPID.isValid())
        str << "PID: " << sp.attachPID.pid() << ' ' << sp.crashParameter << '\n';
    if (!sp.projectSourceDirectory.isEmpty()) {
        str << "Project: " << QDir::toNativeSeparators(sp.projectSourceDirectory) << '\n';
        str << "Additional Search Directories:"
            << sp.additionalSearchDirectories.join(QLatin1Char(' ')) << '\n';
    }
    if (!sp.remoteChannel.isEmpty())
        str << "Remote: " << sp.remoteChannel << '\n';
    if (!sp.qmlServer.host().isEmpty())
        str << "QML server: " << sp.qmlServer.host() << ':' << sp.qmlServer.port() << '\n';
    str << "Sysroot: " << sp.sysRoot << '\n';
    str << "Debug Source Location: " << sp.debugSourceLocation.join(QLatin1Char(':')) << '\n';
    return rc;
}

void DebuggerPluginPrivate::runControlStarted(DebuggerRunTool *runTool)
{
    activateDebugMode();
    const QString message = tr("Starting debugger \"%1\" for ABI \"%2\"...")
            .arg(runTool->engine()->objectName())
            .arg(runTool->runParameters().toolChainAbi.toString());
    showStatusMessage(message);
    showMessage(formatStartParameters(runTool), LogDebug);
    showMessage(m_debuggerSettings->dump(), LogDebug);
    m_snapshotHandler->appendSnapshot(runTool);
    connectEngine(runTool);
}

void DebuggerPluginPrivate::runControlFinished(DebuggerRunTool *runTool)
{
    showStatusMessage(tr("Debugger finished."));
    m_snapshotHandler->removeSnapshot(runTool);
    if (m_snapshotHandler->size() == 0) {
        if (m_shuttingDown) {
            doShutdown();
            return;
        }
        // Last engine quits.
        disconnectEngine();
        if (boolSetting(SwitchModeOnExit))
            activatePreviousMode();
    } else {
        // Connect to some existing engine.
        m_snapshotHandler->activateSnapshot(0);
    }
    m_operateByInstructionAction->setChecked(false);
    m_logWindow->clearUndoRedoStacks();
}

void DebuggerPluginPrivate::remoteCommand(const QStringList &options)
{
    if (options.isEmpty())
        return;

    QString errorMessage;

    if (!parseArguments(options, &errorMessage)) {
        qWarning("%s", qPrintable(errorMessage));
        return;
    }
    runScheduled();
}

QMessageBox *showMessageBox(int icon, const QString &title,
    const QString &text, int buttons)
{
    QMessageBox *mb = new QMessageBox(QMessageBox::Icon(icon),
        title, text, QMessageBox::StandardButtons(buttons),
        ICore::mainWindow());
    mb->setAttribute(Qt::WA_DeleteOnClose);
    mb->setTextInteractionFlags(Qt::TextSelectableByMouse);
    mb->show();
    return mb;
}

void addDebugInfoTask(unsigned id, const QString &cmd)
{
    dd->m_debugInfoTaskHandler.addTask(id, cmd);
}

bool isReverseDebuggingEnabled()
{
    static bool enabled = qEnvironmentVariableIsSet("QTC_DEBUGGER_ENABLE_REVERSE");
    return enabled;
}

bool isReverseDebugging()
{
    return isReverseDebuggingEnabled() && dd->m_reverseDirectionAction->isChecked();
}

void DebuggerPluginPrivate::extensionsInitialized()
{
    // If the CppEditor or QmlJS editor plugin is there, we want to add something to
    // the editor context menu.
    for (Id menuId : { CppEditor::Constants::M_CONTEXT, QmlJSEditor::Constants::M_CONTEXT }) {
        if (ActionContainer *editorContextMenu = ActionManager::actionContainer(menuId)) {
            auto cmd = editorContextMenu->addSeparator(m_watchCommand->context());
            cmd->setAttribute(Command::CA_Hide);
            cmd = m_watchCommand;
            cmd->action()->setEnabled(true);
            editorContextMenu->addAction(cmd);
            cmd->setAttribute(Command::CA_Hide);
            cmd->setAttribute(Command::CA_NonConfigurable);
        }
    }

    auto constraint = [](RunConfiguration *runConfig) {
        Runnable runnable = runConfig->runnable();
        if (runnable.device && runnable.device->type() == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE)
            return true;

        if (DeviceTypeKitInformation::deviceTypeId(runConfig->target()->kit())
                    == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE)
            return true;

        QString mainScript = runConfig->property("mainScript").toString();
        const bool isDebuggableScript = mainScript.endsWith(".py"); // Only Python for now.
        return isDebuggableScript;
    };

    RunControl::registerWorker<DebuggerRunTool>
        (ProjectExplorer::Constants::DEBUG_RUN_MODE, constraint);
}

DebuggerEngine *currentEngine()
{
    DebuggerEngine *engine = nullptr;
    if (dd->m_currentRunTool)
        engine = dd->m_currentRunTool->engine();
    return engine ? engine : dd->dummyEngine();
}

SavedAction *action(int code)
{
    return dd->m_debuggerSettings->item(code);
}

bool boolSetting(int code)
{
    return dd->m_debuggerSettings->item(code)->value().toBool();
}

QString stringSetting(int code)
{
    return dd->m_debuggerSettings->item(code)->value().toString();
}

QStringList stringListSetting(int code)
{
    return dd->m_debuggerSettings->item(code)->value().toStringList();
}

BreakHandler *breakHandler()
{
    return dd->m_breakHandler;
}

void showModuleSymbols(const QString &moduleName, const Symbols &symbols)
{
    QTreeWidget *w = new QTreeWidget;
    w->setUniformRowHeights(true);
    w->setColumnCount(5);
    w->setRootIsDecorated(false);
    w->setAlternatingRowColors(true);
    w->setSortingEnabled(true);
    w->setObjectName(QLatin1String("Symbols.") + moduleName);
    QStringList header;
    header.append(DebuggerPlugin::tr("Symbol"));
    header.append(DebuggerPlugin::tr("Address"));
    header.append(DebuggerPlugin::tr("Code"));
    header.append(DebuggerPlugin::tr("Section"));
    header.append(DebuggerPlugin::tr("Name"));
    w->setHeaderLabels(header);
    w->setWindowTitle(DebuggerPlugin::tr("Symbols in \"%1\"").arg(moduleName));
    for (const Symbol &s : symbols) {
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

void showModuleSections(const QString &moduleName, const Sections &sections)
{
    QTreeWidget *w = new QTreeWidget;
    w->setUniformRowHeights(true);
    w->setColumnCount(5);
    w->setRootIsDecorated(false);
    w->setAlternatingRowColors(true);
    w->setSortingEnabled(true);
    w->setObjectName(QLatin1String("Sections.") + moduleName);
    QStringList header;
    header.append(DebuggerPlugin::tr("Name"));
    header.append(DebuggerPlugin::tr("From"));
    header.append(DebuggerPlugin::tr("To"));
    header.append(DebuggerPlugin::tr("Address"));
    header.append(DebuggerPlugin::tr("Flags"));
    w->setHeaderLabels(header);
    w->setWindowTitle(DebuggerPlugin::tr("Sections in \"%1\"").arg(moduleName));
    for (const Section &s : sections) {
        QTreeWidgetItem *it = new QTreeWidgetItem;
        it->setData(0, Qt::DisplayRole, s.name);
        it->setData(1, Qt::DisplayRole, s.from);
        it->setData(2, Qt::DisplayRole, s.to);
        it->setData(3, Qt::DisplayRole, s.address);
        it->setData(4, Qt::DisplayRole, s.flags);
        w->addTopLevelItem(it);
    }
    createNewDock(w);
}

void DebuggerPluginPrivate::doShutdown()
{
    m_shutdownTimer.stop();
    delete m_mainWindow;
    m_mainWindow = 0;

    delete m_modeWindow;
    m_modeWindow = 0;

    delete m_mode;
    m_mode = 0;
    emit m_plugin->asynchronousShutdownFinished();
}

void updateState(DebuggerRunTool *runTool)
{
    dd->updateState(runTool);
}

void updateLocalsWindow(bool showReturn)
{
    dd->m_returnWindow->setVisible(showReturn);
    dd->m_localsView->resizeColumns();
}

bool hasSnapshots()
{
     return dd->m_snapshotHandler->size();
}

void openTextEditor(const QString &titlePattern0, const QString &contents)
{
    if (dd->m_shuttingDown)
        return;
    QString titlePattern = titlePattern0;
    IEditor *editor = EditorManager::openEditorWithContents(
                CC::K_DEFAULT_TEXT_EDITOR_ID, &titlePattern, contents.toUtf8(), QString(),
                EditorManager::IgnoreNavigationHistory);
    if (auto textEditor = qobject_cast<BaseTextEditor *>(editor)) {
        QString suggestion = titlePattern;
        if (!suggestion.contains(QLatin1Char('.')))
            suggestion.append(QLatin1String(".txt"));
        textEditor->textDocument()->setFallbackSaveAsFileName(suggestion);
    }
    QTC_ASSERT(editor, return);
}

// void runTest(const QString &fileName);
void showMessage(const QString &msg, int channel, int timeout)
{
    dd->showMessage(msg, channel, timeout);
}

void runControlStarted(DebuggerRunTool *runTool)
{
    dd->runControlStarted(runTool);
}

void runControlFinished(DebuggerRunTool *runTool)
{
    dd->runControlFinished(runTool);
}

void displayDebugger(DebuggerRunTool *runTool)
{
    dd->displayDebugger(runTool);
}

void synchronizeBreakpoints()
{
    dd->synchronizeBreakpoints();
}

QWidget *mainWindow()
{
    return dd->m_mainWindow;
}

void raiseWatchersWindow()
{
    if (currentEngine()->state() != DebuggerNotReady)
        dd->m_mainWindow->raiseDock(DOCKWIDGET_WATCHERS);
}

bool isRegistersWindowVisible()
{
    return dd->m_registerWindow->isVisible();
}

bool isModulesWindowVisible()
{
    return dd->m_modulesWindow->isVisible();
}

void openMemoryEditor()
{
    AddressDialog dialog;
    if (dialog.exec() == QDialog::Accepted) {
        MemoryViewSetupData data;
        data.startAddress = dialog.address();
        currentEngine()->openMemoryView(data);
    }
}

void setThreadBoxContents(const QStringList &list, int index)
{
    dd->setThreadBoxContents(list, index);
}

QSharedPointer<Internal::GlobalDebuggerOptions> globalDebuggerOptions()
{
    return dd->m_globalDebuggerOptions;
}

///////////////////////////////////////////////////////////////////////
//
// DebuggerPlugin
//
///////////////////////////////////////////////////////////////////////

/*!
    \class Debugger::DebuggerPlugin

    This is the "external" interface of the debugger plugin that's visible
    from Qt Creator core. The internal interface to global debugger
    functionality that is used by debugger views and debugger engines
    is DebuggerCore, implemented in DebuggerPluginPrivate.
*/

static DebuggerPlugin *m_instance = 0;

DebuggerPlugin::DebuggerPlugin()
{
    setObjectName(QLatin1String("DebuggerPlugin"));
    m_instance = this;
}

DebuggerPlugin::~DebuggerPlugin()
{
    delete dd;
    dd = 0;
    m_instance = 0;
}

DebuggerPlugin *DebuggerPlugin::instance()
{
    return m_instance;
}

bool DebuggerPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    dd = new DebuggerPluginPrivate(this);

    // Needed for call from AppOutputPane::attachToRunControl() and GammarayIntegration.
    ExtensionSystem::PluginManager::addObject(this);

    // Menu groups
    ActionContainer *mstart = ActionManager::actionContainer(PE::M_DEBUG_STARTDEBUGGING);
    mstart->appendGroup(Constants::G_GENERAL);
    mstart->appendGroup(Constants::G_SPECIAL);
    mstart->appendGroup(Constants::G_START_QML);

    // Separators
    mstart->addSeparator(Constants::G_GENERAL);
    mstart->addSeparator(Constants::G_SPECIAL);

    KitManager::registerKitInformation(new DebuggerKitInformation);

    // Task integration.
    //: Category under which Analyzer tasks are listed in Issues view
    ProjectExplorer::TaskHub::addCategory(Debugger::Constants::ANALYZERTASK_ID, tr("Debugger"));

    return dd->initialize(arguments, errorMessage);
}

IPlugin::ShutdownFlag DebuggerPlugin::aboutToShutdown()
{
    ExtensionSystem::PluginManager::removeObject(this);
    dd->aboutToShutdown();
    return AsynchronousShutdown;
}

QObject *DebuggerPlugin::remoteCommand(const QStringList &options,
                                       const QString &workingDirectory,
                                       const QStringList &list)
{
    Q_UNUSED(workingDirectory);
    Q_UNUSED(list);
    dd->remoteCommand(options);
    return 0;
}

void DebuggerPlugin::extensionsInitialized()
{
    dd->extensionsInitialized();
}

void DebuggerPluginPrivate::updateUiForProject(Project *project)
{
    if (m_previousProject) {
        disconnect(m_previousProject, &Project::activeTargetChanged,
            this, &DebuggerPluginPrivate::updateUiForTarget);
    }
    m_previousProject = project;
    if (!project) {
        updateUiForTarget(nullptr);
        return;
    }
    connect(project, &Project::activeTargetChanged,
            this, &DebuggerPluginPrivate::updateUiForTarget);
    updateUiForTarget(project->activeTarget());
}

void DebuggerPluginPrivate::updateUiForTarget(Target *target)
{
    if (m_previousTarget) {
         disconnect(m_previousTarget.data(), &Target::activeRunConfigurationChanged,
                    this, &DebuggerPluginPrivate::updateActiveLanguages);
    }

    m_previousTarget = target;

    if (!target) {
        updateActiveLanguages();
        return;
    }

    connect(target, &Target::activeRunConfigurationChanged,
            this, &DebuggerPluginPrivate::updateActiveLanguages);
    updateActiveLanguages();
}

void DebuggerPluginPrivate::updateActiveLanguages()
{
    if (!dd->m_currentRunTool)
        return;
    const DebuggerRunParameters &rp = dd->m_currentRunTool->runParameters();
//    Id perspective = (languages & QmlLanguage) && !(languages & CppLanguage)
//            ? QmlPerspectiveId : CppPerspectiveId;
//    m_mainWindow->restorePerspective(perspective);
    if (rp.isCppDebugging)
        ICore::addAdditionalContext(Context(C_CPPDEBUGGER));
    else
        ICore::removeAdditionalContext(Context(C_CPPDEBUGGER));

    if (rp.isQmlDebugging)
        ICore::addAdditionalContext(Context(C_QMLDEBUGGER));
    else
        ICore::removeAdditionalContext(Context(C_QMLDEBUGGER));
}

void DebuggerPluginPrivate::onModeChanged(Id mode)
{
    // FIXME: This one gets always called, even if switching between modes
    //        different then the debugger mode. E.g. Welcome and Help mode and
    //        also on shutdown.

    if (mode == MODE_DEBUG) {
        if (IEditor *editor = EditorManager::currentEditor())
            editor->widget()->setFocus();

        m_toolTipManager.debugModeEntered();
        updateActiveLanguages();
    } else {
        m_toolTipManager.leavingDebugMode();
    }
}

void saveModeToRestore()
{
    dd->m_previousMode = ModeManager::currentMode();
}

} // namespace Internal

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

static BuildConfiguration::BuildType startupBuildType()
{
    BuildConfiguration::BuildType buildType = BuildConfiguration::Unknown;
    if (RunConfiguration *runConfig = RunConfiguration::startupRunConfiguration()) {
        if (const BuildConfiguration *buildConfig = runConfig->target()->activeBuildConfiguration())
            buildType = buildConfig->buildType();
    }
    return buildType;
}

void showCannotStartDialog(const QString &text)
{
    QMessageBox *errorDialog = new QMessageBox(ICore::mainWindow());
    errorDialog->setAttribute(Qt::WA_DeleteOnClose);
    errorDialog->setIcon(QMessageBox::Warning);
    errorDialog->setWindowTitle(text);
    errorDialog->setText(DebuggerPlugin::tr("Cannot start %1 without a project. Please open the project "
                                               "and try again.").arg(text));
    errorDialog->setStandardButtons(QMessageBox::Ok);
    errorDialog->setDefaultButton(QMessageBox::Ok);
    errorDialog->show();
}

bool wantRunTool(ToolMode toolMode, const QString &toolName)
{
    // Check the project for whether the build config is in the correct mode
    // if not, notify the user and urge him to use the correct mode.
    BuildConfiguration::BuildType buildType = startupBuildType();
    if (!buildTypeAccepted(toolMode, buildType)) {
        QString currentMode;
        switch (buildType) {
            case BuildConfiguration::Debug:
                currentMode = DebuggerPlugin::tr("Debug");
                break;
            case BuildConfiguration::Profile:
                currentMode = DebuggerPlugin::tr("Profile");
                break;
            case BuildConfiguration::Release:
                currentMode = DebuggerPlugin::tr("Release");
                break;
            default:
                QTC_CHECK(false);
        }

        QString toolModeString;
        switch (toolMode) {
            case DebugMode:
                toolModeString = DebuggerPlugin::tr("in Debug mode");
                break;
            case ProfileMode:
                toolModeString = DebuggerPlugin::tr("in Profile mode");
                break;
            case ReleaseMode:
                toolModeString = DebuggerPlugin::tr("in Release mode");
                break;
            case SymbolsMode:
                toolModeString = DebuggerPlugin::tr("with debug symbols (Debug or Profile mode)");
                break;
            case OptimizedMode:
                toolModeString = DebuggerPlugin::tr("on optimized code (Profile or Release mode)");
                break;
            default:
                QTC_CHECK(false);
        }
        const QString title = DebuggerPlugin::tr("Run %1 in %2 Mode?").arg(toolName).arg(currentMode);
        const QString message = DebuggerPlugin::tr("<html><head/><body><p>You are trying "
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
            return false;
    }

    return true;
}

void registerToolbar(const QByteArray &perspectiveId, const ToolbarDescription &desc)
{
    auto toolbar = new QWidget;
    toolbar->setObjectName(QString::fromLatin1(perspectiveId + ".Toolbar"));
    auto hbox = new QHBoxLayout(toolbar);
    hbox->setMargin(0);
    hbox->setSpacing(0);
    for (QWidget *widget : desc.widgets())
        hbox->addWidget(widget);
    hbox->addStretch();
    toolbar->setLayout(hbox);

    dd->m_mainWindow->registerToolbar(perspectiveId, toolbar);
}

QAction *createStartAction()
{
    auto action = new QAction(DebuggerMainWindow::tr("Start"), DebuggerPlugin::instance());
    action->setIcon(ProjectExplorer::Icons::ANALYZER_START_SMALL_TOOLBAR.icon());
    action->setEnabled(true);
    return action;
}

QAction *createStopAction()
{
    auto action = new QAction(DebuggerMainWindow::tr("Stop"), DebuggerPlugin::instance());
    action->setIcon(Utils::Icons::STOP_SMALL_TOOLBAR.icon());
    action->setEnabled(true);
    return action;
}

void registerPerspective(const QByteArray &perspectiveId, const Perspective *perspective)
{
    dd->m_mainWindow->registerPerspective(perspectiveId, perspective);
}

void setPerspectiveEnabled(const QByteArray &perspectiveId, bool enabled)
{
    dd->m_mainWindow->setPerspectiveEnabled(perspectiveId, enabled);
}

void selectPerspective(const QByteArray &perspectiveId)
{
    if (ModeManager::currentMode() == MODE_DEBUG
            && dd->m_mainWindow->currentPerspective() == perspectiveId) {
        return;
    }

    // FIXME: Work-around aslong as the GammaRay integration does not use the same setup,
    if (perspectiveId.isEmpty())
        return;
    ModeManager::activateMode(MODE_DEBUG);
    dd->m_mainWindow->restorePerspective(perspectiveId);
}

QByteArray currentPerspective()
{
    return dd->m_mainWindow->currentPerspective();
}

QWidget *mainWindow()
{
    return dd->m_mainWindow;
}

void enableMainWindow(bool on)
{
    dd->m_mainWindow->setEnabled(on);
}

void showStatusMessage(const QString &message, int timeoutMS)
{
    dd->m_mainWindow->showStatusMessage(message, timeoutMS);
}

void showPermanentStatusMessage(const QString &message)
{
    dd->m_mainWindow->showStatusMessage(message, -1);
}

namespace Internal {

static bool s_testRun = false;
bool isTestRun() { return s_testRun; }

#ifdef WITH_TESTS

class DebuggerUnitTests : public QObject
{
    Q_OBJECT

public:
    DebuggerUnitTests() {}

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testDebuggerMatching_data();
    void testDebuggerMatching();

    void testBenchmark();
    void testStateMachine();

private:
    CppTools::Tests::TemporaryCopiedDir *m_tmpDir = 0;
};

void DebuggerUnitTests::initTestCase()
{
//    const QList<Kit *> allKits = KitManager::kits();
//    if (allKits.count() != 1)
//        QSKIP("This test requires exactly one kit to be present");
//    const ToolChain * const toolchain = ToolChainKitInformation::toolChain(allKits.first());
//    if (!toolchain)
//        QSKIP("This test requires that there is a kit with a toolchain.");
//    bool hasClangExecutable;
//    clangExecutableFromSettings(toolchain->typeId(), &hasClangExecutable);
//    if (!hasClangExecutable)
//        QSKIP("No clang suitable for analyzing found");

    s_testRun = true;
    m_tmpDir = new CppTools::Tests::TemporaryCopiedDir(QLatin1String(":/unit-tests"));
    QVERIFY(m_tmpDir->isValid());
}

void DebuggerUnitTests::cleanupTestCase()
{
    delete m_tmpDir;
}

void DebuggerUnitTests::testStateMachine()
{
    QString proFile = m_tmpDir->absolutePath("simple/simple.pro");

    CppTools::Tests::ProjectOpenerAndCloser projectManager;
    const CppTools::ProjectInfo projectInfo = projectManager.open(proFile, true);
    QVERIFY(projectInfo.isValid());

    QEventLoop loop;
    connect(BuildManager::instance(), &BuildManager::buildQueueFinished,
            &loop, &QEventLoop::quit);
    ProjectExplorerPlugin::buildProject(SessionManager::startupProject());
    loop.exec();

    ExecuteOnDestruction guard([] { EditorManager::closeAllEditors(false); });

    Target *t = SessionManager::startupProject()->activeTarget();
    QVERIFY(t);
    RunConfiguration *rc = t->activeRunConfiguration();
    QVERIFY(rc);

    auto runControl = new RunControl(rc, ProjectExplorer::Constants::DEBUG_RUN_MODE);
    auto debugger = new DebuggerRunTool(runControl);

    debugger->setInferior(rc->runnable());
    debugger->setTestCase(TestNoBoundsOfCurrentFunction);

    connect(debugger, &DebuggerRunTool::stopped,
            &QTestEventLoop::instance(), &QTestEventLoop::exitLoop);

    debugger->startRunControl();

    QTestEventLoop::instance().enterLoop(5);
}


enum FakeEnum { FakeDebuggerCommonSettingsId };

void DebuggerUnitTests::testBenchmark()
{
#ifdef WITH_BENCHMARK
    CALLGRIND_START_INSTRUMENTATION;
    volatile Id id1 = Id(DEBUGGER_COMMON_SETTINGS_ID);
    CALLGRIND_STOP_INSTRUMENTATION;
    CALLGRIND_DUMP_STATS;

    CALLGRIND_START_INSTRUMENTATION;
    volatile FakeEnum id2 = FakeDebuggerCommonSettingsId;
    CALLGRIND_STOP_INSTRUMENTATION;
    CALLGRIND_DUMP_STATS;
#endif
}

void DebuggerUnitTests::testDebuggerMatching_data()
{
    QTest::addColumn<QStringList>("debugger");
    QTest::addColumn<QString>("target");
    QTest::addColumn<int>("result");

    QTest::newRow("Invalid data")
            << QStringList()
            << QString()
            << int(DebuggerItem::DoesNotMatch);
    QTest::newRow("Invalid debugger")
            << QStringList()
            << QString::fromLatin1("x86-linux-generic-elf-32bit")
            << int(DebuggerItem::DoesNotMatch);
    QTest::newRow("Invalid target")
            << QStringList("x86-linux-generic-elf-32bit")
            << QString()
            << int(DebuggerItem::DoesNotMatch);

    QTest::newRow("Fuzzy match 1")
            << QStringList("unknown-unknown-unknown-unknown-0bit")
            << QString::fromLatin1("x86-linux-generic-elf-32bit")
            << int(DebuggerItem::MatchesWell); // Is this the expected behavior?
    QTest::newRow("Fuzzy match 2")
            << QStringList("unknown-unknown-unknown-unknown-0bit")
            << QString::fromLatin1("arm-windows-msys-pe-64bit")
            << int(DebuggerItem::MatchesWell); // Is this the expected behavior?

    QTest::newRow("Architecture mismatch")
            << QStringList("x86-linux-generic-elf-32bit")
            << QString::fromLatin1("arm-linux-generic-elf-32bit")
            << int(DebuggerItem::DoesNotMatch);
    QTest::newRow("OS mismatch")
            << QStringList("x86-linux-generic-elf-32bit")
            << QString::fromLatin1("x86-macosx-generic-elf-32bit")
            << int(DebuggerItem::DoesNotMatch);
    QTest::newRow("Format mismatch")
            << QStringList("x86-linux-generic-elf-32bit")
            << QString::fromLatin1("x86-linux-generic-pe-32bit")
            << int(DebuggerItem::DoesNotMatch);

    QTest::newRow("Linux perfect match")
            << QStringList("x86-linux-generic-elf-32bit")
            << QString::fromLatin1("x86-linux-generic-elf-32bit")
            << int(DebuggerItem::MatchesWell);
    QTest::newRow("Linux match")
            << QStringList("x86-linux-generic-elf-64bit")
            << QString::fromLatin1("x86-linux-generic-elf-32bit")
            << int(DebuggerItem::MatchesSomewhat);

    QTest::newRow("Windows perfect match 1")
            << QStringList("x86-windows-msvc2013-pe-64bit")
            << QString::fromLatin1("x86-windows-msvc2013-pe-64bit")
            << int(DebuggerItem::MatchesWell);
    QTest::newRow("Windows perfect match 2")
            << QStringList("x86-windows-msvc2013-pe-64bit")
            << QString::fromLatin1("x86-windows-msvc2012-pe-64bit")
            << int(DebuggerItem::MatchesWell);
    QTest::newRow("Windows match 1")
            << QStringList("x86-windows-msvc2013-pe-64bit")
            << QString::fromLatin1("x86-windows-msvc2013-pe-32bit")
            << int(DebuggerItem::MatchesSomewhat);
    QTest::newRow("Windows match 2")
            << QStringList("x86-windows-msvc2013-pe-64bit")
            << QString::fromLatin1("x86-windows-msvc2012-pe-32bit")
            << int(DebuggerItem::MatchesSomewhat);
    QTest::newRow("Windows mismatch on word size")
            << QStringList("x86-windows-msvc2013-pe-32bit")
            << QString::fromLatin1("x86-windows-msvc2013-pe-64bit")
            << int(DebuggerItem::DoesNotMatch);
    QTest::newRow("Windows mismatch on osflavor 1")
            << QStringList("x86-windows-msvc2013-pe-32bit")
            << QString::fromLatin1("x86-windows-msys-pe-64bit")
            << int(DebuggerItem::DoesNotMatch);
    QTest::newRow("Windows mismatch on osflavor 2")
            << QStringList("x86-windows-msys-pe-32bit")
            << QString::fromLatin1("x86-windows-msvc2010-pe-64bit")
            << int(DebuggerItem::DoesNotMatch);
}

void DebuggerUnitTests::testDebuggerMatching()
{
    QFETCH(QStringList, debugger);
    QFETCH(QString, target);
    QFETCH(int, result);

    DebuggerItem::MatchLevel expectedLevel = static_cast<DebuggerItem::MatchLevel>(result);

    QList<Abi> debuggerAbis;
    foreach (const QString &abi, debugger)
        debuggerAbis << Abi::fromString(abi);

    DebuggerItem item;
    item.setAbis(debuggerAbis);

    DebuggerItem::MatchLevel level = item.matchTarget(Abi::fromString(target));
    if (level == DebuggerItem::MatchesPerfectly)
        level = DebuggerItem::MatchesWell;

    QCOMPARE(expectedLevel, level);
}


QList<QObject *> DebuggerPlugin::createTestObjects() const
{
    return {new DebuggerUnitTests};
}

#else // ^-- if WITH_TESTS else --v

QList<QObject *> DebuggerPlugin::createTestObjects() const
{
    return {};
}

#endif // if  WITH_TESTS

} // namespace Internal
} // namespace Debugger

#include "debuggerplugin.moc"
