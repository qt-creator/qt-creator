/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "debuggerplugin.h"

#include "debuggerstartparameters.h"
#include "debuggeractions.h"
#include "debuggerinternalconstants.h"
#include "debuggercore.h"
#include "debuggerkitconfigwidget.h"
#include "debuggerdialogs.h"
#include "debuggerengine.h"
#include "debuggeritemmanager.h"
#include "debuggermainwindow.h"
#include "debuggerrunconfigurationaspect.h"
#include "debuggerruncontrol.h"
#include "debuggerstringutils.h"
#include "debuggeroptionspage.h"
#include "debuggerkitinformation.h"
#include "memoryagent.h"
#include "breakhandler.h"
#include "breakwindow.h"
#include "disassemblerlines.h"
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
#include "unstartedappwatcherdialog.h"
#include "debuggertooltipmanager.h"
#include "localsandexpressionswindow.h"
#include "loadcoredialog.h"
#include "sourceutils.h"
#include <debugger/shared/hostutils.h>

#include "snapshothandler.h"
#include "threadshandler.h"
#include "commonoptionspage.h"
#include "gdb/startgdbserverdialog.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/find/itemviewfind.h>
#include <coreplugin/imode.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagebox.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/documentmodel.h>

#include <cppeditor/cppeditorconstants.h>
#include <cpptools/cppmodelmanager.h>

#include <extensionsystem/invoker.h>

#include <projectexplorer/localapplicationrunconfiguration.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/devicesupport/deviceprocesslist.h>
#include <projectexplorer/devicesupport/deviceprocessesdialog.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/projectexplorersettings.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>

#include <texteditor/texteditor.h>
#include <texteditor/textdocument.h>
#include <texteditor/fontsettings.h>
#include <texteditor/texteditorsettings.h>

#include <utils/basetreeview.h>
#include <utils/hostosinfo.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/proxyaction.h>
#include <utils/qtcassert.h>
#include <utils/savedaction.h>
#include <utils/statuslabel.h>
#include <utils/styledbar.h>
#include <utils/winutils.h>

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDockWidget>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QMessageBox>
#include <QTextBlock>
#include <QToolButton>
#include <QtPlugin>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QMenu>

#ifdef WITH_TESTS
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

struct TestCallBack
{
    TestCallBack() : receiver(0), slot(0) {}
    TestCallBack(QObject *ob, const char *s) : receiver(ob), slot(s) {}

    QObject *receiver;
    const char *slot;
    QVariant cookie;
};


} // namespace Internal
} // namespace Debugger

Q_DECLARE_METATYPE(Debugger::Internal::TestCallBack)

namespace Debugger {
namespace Internal {

void addCdbOptionPages(QList<IOptionsPage*> *opts);
void addGdbOptionPages(QList<IOptionsPage*> *opts);

static QToolButton *toolButton(QAction *action)
{
    QToolButton *button = new QToolButton;
    button->setDefaultAction(action);
    return button;
}

static void setProxyAction(ProxyAction *proxy, Id id)
{
    proxy->setAction(ActionManager::command(id)->action());
}

static QToolButton *toolButton(Id id)
{
    return toolButton(ActionManager::command(id)->action());
}

///////////////////////////////////////////////////////////////////////
//
// DummyEngine
//
///////////////////////////////////////////////////////////////////////

class DummyEngine : public DebuggerEngine
{
public:
    DummyEngine() : DebuggerEngine(DebuggerStartParameters()) {}
    ~DummyEngine() {}

    void setupEngine() {}
    void setupInferior() {}
    void runEngine() {}
    void shutdownEngine() {}
    void shutdownInferior() {}
    bool hasCapability(unsigned cap) const;
    bool acceptsBreakpoint(Breakpoint) const { return false; }
    bool acceptsDebuggerCommands() const { return false; }
    void selectThread(ThreadId) {}
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
               | OperateNativeMixed
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
        setIcon(QIcon(QLatin1String(":/debugger/images/mode_debug.png")));
        setPriority(85);
        setId(MODE_DEBUG);
    }

    ~DebugMode()
    {
        delete m_widget;
    }
};

///////////////////////////////////////////////////////////////////////
//
// Misc
//
///////////////////////////////////////////////////////////////////////

static QWidget *addSearch(BaseTreeView *treeView, const QString &title,
    const char *objectName)
{
    QAction *act = action(UseAlternatingRowColors);
    treeView->setAlternatingRowColors(act->isChecked());
    QObject::connect(act, &QAction::toggled,
                     treeView, &BaseTreeView::setAlternatingRowColorsHelper);

    QWidget *widget = ItemViewFind::createSearchableWrapper(treeView);
    widget->setObjectName(QLatin1String(objectName));
    widget->setWindowTitle(title);
    return widget;
}

static std::function<bool(const Kit *)> cdbMatcher(char wordWidth = 0)
{
    return [wordWidth](const Kit *k) -> bool {
        if (DebuggerKitInformation::engineType(k) != CdbEngineType
            || !DebuggerKitInformation::isValidDebugger(k)) {
            return false;
        }
        if (wordWidth) {
            const ToolChain *tc = ToolChainKitInformation::toolChain(k);
            return tc && wordWidth == tc->targetAbi().wordWidth();
        }
        return true;
    };
}

// Find a CDB kit for debugging unknown processes.
// On a 64bit OS, prefer a 64bit debugger.
static Kit *findUniversalCdbKit()
{
    if (Utils::is64BitWindowsSystem()) {
        if (Kit *cdb64Kit = KitManager::find(cdbMatcher(64)))
            return cdb64Kit;
    }
    return KitManager::find(cdbMatcher());
}

static bool currentTextEditorPosition(ContextData *data)
{
    BaseTextEditor *textEditor = BaseTextEditor::currentTextEditor();
    if (!textEditor)
        return false;
    const TextDocument *document = textEditor->textDocument();
    QTC_ASSERT(document, return false);
    data->fileName = document->filePath().toString();
    if (document->property(Constants::OPENED_WITH_DISASSEMBLY).toBool()) {
        int lineNumber = textEditor->currentLine();
        QString line = textEditor->textDocument()->plainText()
            .section(QLatin1Char('\n'), lineNumber - 1, lineNumber - 1);
        data->address = DisassemblerLine::addressFromDisassemblyLine(line);
    } else {
        data->lineNumber = textEditor->currentLine();
    }
    return true;
}

///////////////////////////////////////////////////////////////////////
//
// DebuggerPluginPrivate
//
///////////////////////////////////////////////////////////////////////

static DebuggerPluginPrivate *dd = 0;

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

    void connectEngine(DebuggerEngine *engine);
    void disconnectEngine() { connectEngine(0); }
    DebuggerEngine *dummyEngine();

    void setThreads(const QStringList &list, int index)
    {
        const bool state = m_threadBox->blockSignals(true);
        m_threadBox->clear();
        foreach (const QString &item, list)
            m_threadBox->addItem(item);
        m_threadBox->setCurrentIndex(index);
        m_threadBox->blockSignals(state);
    }

    DebuggerRunControl *attachToRunningProcess(Kit *kit, DeviceProcessItem process, bool contAfterAttach);

    void writeSettings()
    {
        m_debuggerSettings->writeSettings();
        m_mainWindow->writeSettings();
    }

    void selectThread(int index)
    {
        ThreadId id = m_currentEngine->threadsHandler()->threadAt(index);
        m_currentEngine->selectThread(id);
    }

    void breakpointSetMarginActionTriggered(bool isMessageOnly, const ContextData &data)
    {
        QString message;
        if (isMessageOnly) {
            if (data.address) {
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
        if (data.address)
            toggleBreakpointByAddress(data.address, message);
        else
            toggleBreakpointByFileAndLine(data.fileName, data.lineNumber, message);
    }

    void updateWatchersHeader(int section, int, int newSize)
    {
        m_watchersView->header()->resizeSection(section, newSize);
        m_returnView->header()->resizeSection(section, newSize);
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
        showMessage(QLatin1String("ATTEMPT SYNC"), LogDebug);
        for (int i = 0, n = m_snapshotHandler->size(); i != n; ++i) {
            if (DebuggerEngine *engine = m_snapshotHandler->at(i))
                engine->attemptBreakpointSynchronization();
        }
    }

    void editorOpened(IEditor *editor);
    void updateBreakMenuItem(IEditor *editor);
    void setBusyCursor(bool busy);
    void requestMark(TextEditorWidget *widget, int lineNumber,
                     TextMarkRequestKind kind);
    void requestContextMenu(TextEditorWidget *widget,
                            int lineNumber, QMenu *menu);

    void activatePreviousMode();
    void activateDebugMode();
    void toggleBreakpoint();
    void toggleBreakpointByFileAndLine(const QString &fileName, int lineNumber,
                                       const QString &tracePointMessage = QString());
    void toggleBreakpointByAddress(quint64 address,
                                   const QString &tracePointMessage = QString());
    void onModeChanged(IMode *mode);
    void onCoreAboutToOpen();
    void updateDebugWithoutDeployMenu();

    void startAndDebugApplication();
    void startRemoteCdbSession();
    void startRemoteServerAndAttachToProcess();
    void attachToRemoteServer();
    void attachToRunningApplication();
    void attachToUnstartedApplicationDialog();
    void attachToQmlPort();
    Q_SLOT void runScheduled();
    void attachCore();

    void enableReverseDebuggingTriggered(const QVariant &value);
    void showStatusMessage(const QString &msg, int timeout = -1);

    void runControlStarted(DebuggerEngine *engine);
    void runControlFinished(DebuggerEngine *engine);
    void remoteCommand(const QStringList &options);

    void displayDebugger(DebuggerEngine *engine, bool updateEngine = true);

    void dumpLog();
    void cleanupViews();
    void setInitialState();

    void fontSettingsChanged(const FontSettings &settings);

    void updateState(DebuggerEngine *engine);
    void onCurrentProjectChanged(Project *project);

    void sessionLoaded();
    void aboutToUnloadSession();
    void aboutToSaveSession();

    void coreShutdown();

#ifdef WITH_TESTS
public slots:
    void testLoadProject(const QString &proFile, const TestCallBack &cb);
    void testProjectLoaded(Project *project);
    void testProjectEvaluated();
    void testProjectBuilt(bool success);
    void testUnloadProject();
    void testFinished();

    void testRunProject(const DebuggerStartParameters &sp, const TestCallBack &cb);
    void testRunControlFinished();

//    void testStateMachine1();
//    void testStateMachine2();
//    void testStateMachine3();

    void testBenchmark1();

public:
    Project *m_testProject;
    bool m_testSuccess;
    QList<TestCallBack> m_testCallbacks;

#endif

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

    void handleAbort()
    {
        currentEngine()->resetLocation();
        currentEngine()->abortDebugger();
    }

    void handleReset()
    {
        currentEngine()->resetLocation();
        currentEngine()->resetInferior();
    }

    void handleExecStep()
    {
        if (currentEngine()->state() == DebuggerNotReady) {
            ProjectExplorerPlugin::runStartupProject(DebugRunModeWithBreakOnMain);
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
            ProjectExplorerPlugin::runStartupProject(DebugRunModeWithBreakOnMain);
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
        ContextData data;
        if (currentTextEditorPosition(&data))
            currentEngine()->executeJumpToLine(data);
    }

    void handleExecRunToLine()
    {
        currentEngine()->resetLocation();
        ContextData data;
        if (currentTextEditorPosition(&data))
            currentEngine()->executeRunToLine(data);
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
        if (exp.isEmpty())
            return;
        currentEngine()->watchHandler()->watchVariable(exp);
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


public:
    DebuggerMainWindow *m_mainWindow;
    DebuggerRunControlFactory *m_debuggerRunControlFactory;

    Id m_previousMode;
    QList<DebuggerStartParameters> m_scheduledStarts;

    ProxyAction *m_visibleStartAction;
    ProxyAction *m_hiddenStopAction;
    QAction *m_startAction;
    QAction *m_debugWithoutDeployAction;
    QAction *m_startAndDebugApplicationAction;
    QAction *m_startRemoteServerAction;
    QAction *m_attachToRunningApplication;
    QAction *m_attachToUnstartedApplication;
    QAction *m_attachToQmlPortAction;
    QAction *m_attachToRemoteServerAction;
    QAction *m_startRemoteCdbAction;
    QAction *m_attachToCoreAction;
    QAction *m_detachAction;
    QAction *m_continueAction;
    QAction *m_exitAction; // On application output button if "Stop" is possible
    QAction *m_interruptAction; // On the fat debug button if "Pause" is possible
    QAction *m_undisturbableAction; // On the fat debug button if nothing can be done
    QAction *m_abortAction;
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
    QAction *m_resetAction;

    QToolButton *m_reverseToolButton;

    QIcon m_startIcon;
    QIcon m_exitIcon;
    QIcon m_continueIcon;
    QIcon m_interruptIcon;
    QIcon m_locationMarkIcon;
    QIcon m_resetIcon;

    StatusLabel *m_statusLabel;
    QComboBox *m_threadBox;

    BaseTreeView *m_breakView;
    BaseTreeView *m_returnView;
    BaseTreeView *m_localsView;
    BaseTreeView *m_watchersView;
    BaseTreeView *m_inspectorView;
    BaseTreeView *m_registerView;
    BaseTreeView *m_modulesView;
    BaseTreeView *m_snapshotView;
    BaseTreeView *m_sourceFilesView;
    BaseTreeView *m_stackView;
    BaseTreeView *m_threadsView;

    QWidget *m_breakWindow;
    BreakHandler *m_breakHandler;
    QWidget *m_returnWindow;
    QWidget *m_localsWindow;
    QWidget *m_watchersWindow;
    QWidget *m_inspectorWindow;
    QWidget *m_registerWindow;
    QWidget *m_modulesWindow;
    QWidget *m_snapshotWindow;
    QWidget *m_sourceFilesWindow;
    QWidget *m_stackWindow;
    QWidget *m_threadsWindow;
    LogWindow *m_logWindow;
    LocalsAndExpressionsWindow *m_localsAndExpressionsWindow;

    bool m_busy;
    QString m_lastPermanentStatusMessage;

    mutable CPlusPlus::Snapshot m_codeModelSnapshot;
    DebuggerPlugin *m_plugin;

    SnapshotHandler *m_snapshotHandler;
    bool m_shuttingDown;
    DebuggerEngine *m_currentEngine;
    DebuggerSettings *m_debuggerSettings;
    QStringList m_arguments;
    DebuggerToolTipManager m_toolTipManager;
    CommonOptionsPage *m_commonOptionsPage;
    DummyEngine *m_dummyEngine;
    const QSharedPointer<GlobalDebuggerOptions> m_globalDebuggerOptions;
};

DebuggerPluginPrivate::DebuggerPluginPrivate(DebuggerPlugin *plugin) :
    m_dummyEngine(0),
    m_globalDebuggerOptions(new GlobalDebuggerOptions)
{
    qRegisterMetaType<WatchData>("WatchData");
    qRegisterMetaType<ContextData>("ContextData");
    qRegisterMetaType<DebuggerStartParameters>("DebuggerStartParameters");

    QTC_CHECK(!dd);
    dd = this;

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
    m_inspectorWindow = 0;
    m_registerWindow = 0;
    m_modulesWindow = 0;
    m_snapshotWindow = 0;
    m_sourceFilesWindow = 0;
    m_stackWindow = 0;
    m_threadsWindow = 0;
    m_logWindow = 0;
    m_localsAndExpressionsWindow = 0;

    m_mainWindow = 0;
    m_snapshotHandler = 0;
    m_currentEngine = 0;
    m_debuggerSettings = 0;

    m_reverseToolButton = 0;
    m_startAction = 0;
    m_debugWithoutDeployAction = 0;
    m_startAndDebugApplicationAction = 0;
    m_attachToRemoteServerAction = 0;
    m_attachToRunningApplication = 0;
    m_attachToUnstartedApplication = 0;
    m_attachToQmlPortAction = 0;
    m_startRemoteCdbAction = 0;
    m_attachToCoreAction = 0;
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

static QString msgParameterMissing(const QString &a)
{
    return DebuggerPlugin::tr("Option \"%1\" is missing the parameter.").arg(a);
}

bool DebuggerPluginPrivate::parseArgument(QStringList::const_iterator &it,
    const QStringList::const_iterator &cend, QString *errorMessage)
{
    const QString &option = *it;
    // '-debug <pid>'
    // '-debug <exe>[,server=<server:port>][,core=<core>][,kit=<kit>]'
    if (*it == _("-debug")) {
        ++it;
        if (it == cend) {
            *errorMessage = msgParameterMissing(*it);
            return false;
        }
        Kit *kit = 0;
        DebuggerStartParameters sp;
        qulonglong pid = it->toULongLong();
        if (pid) {
            sp.startMode = AttachExternal;
            sp.closeMode = DetachAtClose;
            sp.attachPID = pid;
            sp.displayName = tr("Process %1").arg(sp.attachPID);
            sp.startMessage = tr("Attaching to local process %1.").arg(sp.attachPID);
        } else {
            sp.startMode = StartExternal;
            QStringList args = it->split(QLatin1Char(','));
            foreach (const QString &arg, args) {
                QString key = arg.section(QLatin1Char('='), 0, 0);
                QString val = arg.section(QLatin1Char('='), 1, 1);
                if (val.isEmpty()) {
                    if (key.isEmpty()) {
                        continue;
                    } else if (sp.executable.isEmpty()) {
                        sp.executable = key;
                    } else {
                        *errorMessage = DebuggerPlugin::tr("Only one executable allowed.");
                        return false;
                    }
                }
                if (key == QLatin1String("server")) {
                    sp.startMode = AttachToRemoteServer;
                    sp.remoteChannel = val;
                    sp.displayName = tr("Remote: \"%1\"").arg(sp.remoteChannel);
                    sp.startMessage = tr("Attaching to remote server %1.").arg(sp.remoteChannel);
                } else if (key == QLatin1String("core")) {
                    sp.startMode = AttachCore;
                    sp.closeMode = DetachAtClose;
                    sp.coreFile = val;
                    sp.displayName = tr("Core file \"%1\"").arg(sp.coreFile);
                    sp.startMessage = tr("Attaching to core file %1.").arg(sp.coreFile);
                } else if (key == QLatin1String("kit")) {
                    kit = KitManager::find(Id::fromString(val));
                }
            }
        }
        if (!DebuggerRunControlFactory::fillParametersFromKit(&sp, kit, errorMessage))
            return false;
        if (sp.startMode == StartExternal) {
            sp.displayName = tr("Executable file \"%1\"").arg(sp.executable);
            sp.startMessage = tr("Debugging file %1.").arg(sp.executable);
        }
        m_scheduledStarts.append(sp);
        return true;
    }
    // -wincrashevent <event-handle>:<pid>. A handle used for
    // a handshake when attaching to a crashed Windows process.
    // This is created by $QTC/src/tools/qtcdebugger/main.cpp:
    // args << QLatin1String("-wincrashevent")
    //   << QString::fromLatin1("%1:%2").arg(argWinCrashEvent).arg(argProcessId);
    if (*it == _("-wincrashevent")) {
        ++it;
        if (it == cend) {
            *errorMessage = msgParameterMissing(*it);
            return false;
        }
        DebuggerStartParameters sp;
        if (!DebuggerRunControlFactory::fillParametersFromKit(&sp, findUniversalCdbKit(), errorMessage))
            return false;
        sp.startMode = AttachCrashedExternal;
        sp.crashParameter = it->section(QLatin1Char(':'), 0, 0);
        sp.attachPID = it->section(QLatin1Char(':'), 1, 1).toULongLong();
        sp.displayName = tr("Crashed process %1").arg(sp.attachPID);
        sp.startMessage = tr("Attaching to crashed process %1").arg(sp.attachPID);
        if (!sp.attachPID) {
            *errorMessage = DebuggerPlugin::tr("The parameter \"%1\" of option \"%2\" "
                "does not match the pattern <handle>:<pid>.").arg(*it, option);
            return false;
        }
        m_scheduledStarts.append(sp);
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
        QTimer::singleShot(0, this, SLOT(runScheduled()));
}

bool DebuggerPluginPrivate::initialize(const QStringList &arguments,
    QString *errorMessage)
{
    Q_UNUSED(errorMessage);
    Utils::MimeDatabase::addMimeTypes(QLatin1String(":/debugger/Debugger.mimetypes.xml"));

    m_arguments = arguments;
    if (!m_arguments.isEmpty())
        connect(KitManager::instance(), &KitManager::kitsLoaded,
                this, &DebuggerPluginPrivate::parseCommandLineArguments);
    // Cpp/Qml ui setup
    m_mainWindow = new DebuggerMainWindow;

    TaskHub::addCategory(TASK_CATEGORY_DEBUGGER_DEBUGINFO,
                         tr("Debug Information"));
    TaskHub::addCategory(TASK_CATEGORY_DEBUGGER_RUNTIME,
                         tr("Debugger Runtime"));

    return true;
}

void setConfigValue(const QByteArray &name, const QVariant &value)
{
    ICore::settings()->setValue(_("DebugMode/" + name), value);
}

QVariant configValue(const QByteArray &name)
{
    return ICore::settings()->value(_("DebugMode/" + name));
}

void DebuggerPluginPrivate::onCurrentProjectChanged(Project *project)
{
    RunConfiguration *activeRc = 0;
    if (project) {
        Target *target = project->activeTarget();
        if (target)
            activeRc = target->activeRunConfiguration();
        if (!activeRc)
            return;
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

    // If we have a running debugger, don't touch it.
    if (m_snapshotHandler->size())
        return;

    // No corresponding debugger found. So we are ready to start one.
    m_interruptAction->setEnabled(false);
    m_continueAction->setEnabled(false);
    m_exitAction->setEnabled(false);
    QString whyNot;
    const bool canRun = ProjectExplorerPlugin::canRun(project, DebugRunMode, &whyNot);
    m_startAction->setEnabled(canRun);
    m_startAction->setToolTip(whyNot);
    m_debugWithoutDeployAction->setEnabled(canRun);
    setProxyAction(m_visibleStartAction, Id(Constants::DEBUG));
}

void DebuggerPluginPrivate::startAndDebugApplication()
{
    DebuggerStartParameters sp;
    if (StartApplicationDialog::run(ICore::dialogParent(), &sp))
        DebuggerRunControlFactory::createAndScheduleRun(sp);
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

    QString display = dlg.useLocalCoreFile() ? dlg.localCoreFile() : dlg.remoteCoreFile();
    DebuggerStartParameters sp;
    bool res = DebuggerRunControlFactory::fillParametersFromKit(&sp, dlg.kit());
    QTC_ASSERT(res, return);
    sp.masterEngineType = DebuggerKitInformation::engineType(dlg.kit());
    sp.executable = dlg.localExecutableFile();
    sp.coreFile = dlg.localCoreFile();
    sp.displayName = tr("Core file \"%1\"").arg(display);
    sp.startMode = AttachCore;
    sp.closeMode = DetachAtClose;
    sp.overrideStartScript = dlg.overrideStartScript();
    DebuggerRunControlFactory::createAndScheduleRun(sp);
}

void DebuggerPluginPrivate::startRemoteCdbSession()
{
    const QByteArray connectionKey = "CdbRemoteConnection";
    DebuggerStartParameters sp;
    Kit *kit = findUniversalCdbKit();
    QTC_ASSERT(kit, return);
    bool res = DebuggerRunControlFactory::fillParametersFromKit(&sp, kit);
    QTC_ASSERT(res, return);
    sp.startMode = AttachToRemoteServer;
    sp.closeMode = KillAtClose;
    StartRemoteCdbDialog dlg(ICore::dialogParent());
    QString previousConnection = configValue(connectionKey).toString();
    if (previousConnection.isEmpty())
        previousConnection = QLatin1String("localhost:1234");
    dlg.setConnection(previousConnection);
    if (dlg.exec() != QDialog::Accepted)
        return;
    sp.remoteChannel = dlg.connection();
    setConfigValue(connectionKey, sp.remoteChannel);
    DebuggerRunControlFactory::createAndScheduleRun(sp);
}

void DebuggerPluginPrivate::attachToRemoteServer()
{
    DebuggerStartParameters sp;
    sp.startMode = AttachToRemoteServer;
    if (StartApplicationDialog::run(ICore::dialogParent(), &sp)) {
        sp.closeMode = KillAtClose;
        sp.serverStartScript.clear();
        DebuggerRunControlFactory::createAndScheduleRun(sp);
    }
}

void DebuggerPluginPrivate::startRemoteServerAndAttachToProcess()
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

    GdbServerStarter *starter = new GdbServerStarter(dlg, true);
    starter->run();
}

void DebuggerPluginPrivate::attachToRunningApplication()
{
    auto kitChooser = new DebuggerKitChooser(DebuggerKitChooser::LocalDebugging);

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

    if (device->type() == PE::DESKTOP_DEVICE_TYPE) {
        attachToRunningProcess(kit, dlg->currentProcess(), false);
    } else {
        GdbServerStarter *starter = new GdbServerStarter(dlg, true);
        starter->run();
    }
}

void DebuggerPluginPrivate::attachToUnstartedApplicationDialog()
{
    auto dlg = new UnstartedAppWatcherDialog(ICore::dialogParent());

    connect(dlg, &QDialog::finished, dlg, &QObject::deleteLater);
    connect(dlg, &UnstartedAppWatcherDialog::processFound, this, [this, dlg] {
        DebuggerRunControl *rc = attachToRunningProcess(dlg->currentKit(),
                                                        dlg->currentProcess(),
                                                        dlg->continueOnAttach());
        if (!rc)
            return;

        if (dlg->hideOnAttach())
            connect(rc, &RunControl::finished, dlg, &UnstartedAppWatcherDialog::startWatching);
    });

    dlg->show();
}

DebuggerRunControl *DebuggerPluginPrivate::attachToRunningProcess(Kit *kit,
    DeviceProcessItem process, bool contAfterAttach)
{
    QTC_ASSERT(kit, return 0);
    IDevice::ConstPtr device = DeviceKitInformation::device(kit);
    QTC_ASSERT(device, return 0);
    if (process.pid == 0) {
        AsynchronousMessageBox::warning(tr("Warning"), tr("Cannot attach to process with PID 0"));
        return 0;
    }

    bool isWindows = false;
    if (const ToolChain *tc = ToolChainKitInformation::toolChain(kit))
        isWindows = tc->targetAbi().os() == Abi::WindowsOS;
    if (isWindows && isWinProcessBeingDebugged(process.pid)) {
        AsynchronousMessageBox::warning(tr("Process Already Under Debugger Control"),
                             tr("The process %1 is already under the control of a debugger.\n"
                                "Qt Creator cannot attach to it.").arg(process.pid));
        return 0;
    }

    if (device->type() != PE::DESKTOP_DEVICE_TYPE) {
        AsynchronousMessageBox::warning(tr("Not a Desktop Device Type"),
                             tr("It is only possible to attach to a locally running process."));
        return 0;
    }

    DebuggerStartParameters sp;
    bool res = DebuggerRunControlFactory::fillParametersFromKit(&sp, kit);
    QTC_ASSERT(res, return 0);
    sp.attachPID = process.pid;
    sp.displayName = tr("Process %1").arg(process.pid);
    sp.executable = process.exe;
    sp.startMode = AttachExternal;
    sp.closeMode = DetachAtClose;
    sp.continueAfterAttach = contAfterAttach;
    return DebuggerRunControlFactory::createAndScheduleRun(sp);
}

void DebuggerPlugin::attachExternalApplication(RunControl *rc)
{
    DebuggerStartParameters sp;
    sp.attachPID = rc->applicationProcessHandle().pid();
    sp.displayName = tr("Process %1").arg(sp.attachPID);
    sp.startMode = AttachExternal;
    sp.closeMode = DetachAtClose;
    sp.toolChainAbi = rc->abi();
    Kit *kit = 0;
    if (const RunConfiguration *runConfiguration = rc->runConfiguration())
        if (const Target *target = runConfiguration->target())
            kit = target->kit();
    bool res = DebuggerRunControlFactory::fillParametersFromKit(&sp, kit);
    QTC_ASSERT(res, return);
    DebuggerRunControlFactory::createAndScheduleRun(sp);
}

void DebuggerPluginPrivate::attachToQmlPort()
{
    DebuggerStartParameters sp;
    AttachToQmlPortDialog dlg(ICore::mainWindow());

    const QVariant qmlServerPort = configValue("LastQmlServerPort");
    if (qmlServerPort.isValid())
        dlg.setPort(qmlServerPort.toInt());
    else
        dlg.setPort(sp.qmlServerPort);

    const Id kitId = Id::fromSetting(configValue("LastProfile"));
    if (kitId.isValid())
        dlg.setKitId(kitId);

    if (dlg.exec() != QDialog::Accepted)
        return;

    Kit *kit = dlg.kit();
    QTC_ASSERT(kit, return);
    bool res = DebuggerRunControlFactory::fillParametersFromKit(&sp, kit);
    QTC_ASSERT(res, return);
    setConfigValue("LastQmlServerPort", dlg.port());
    setConfigValue("LastProfile", kit->id().toSetting());

    IDevice::ConstPtr device = DeviceKitInformation::device(kit);
    if (device) {
        sp.connParams = device->sshParameters();
        sp.qmlServerAddress = device->qmlProfilerHost();
    }
    sp.qmlServerPort = dlg.port();
    sp.startMode = AttachToRemoteProcess;
    sp.closeMode = KillAtClose;
    sp.languages = QmlLanguage;
    sp.masterEngineType = QmlEngineType;

    //
    // get files from all the projects in the session
    //
    QList<Project *> projects = SessionManager::projects();
    if (Project *startupProject = SessionManager::startupProject()) {
        // startup project first
        projects.removeOne(startupProject);
        projects.insert(0, startupProject);
    }
    QStringList sourceFiles;
    foreach (Project *project, projects)
        sourceFiles << project->files(Project::ExcludeGeneratedFiles);

    sp.projectSourceDirectory =
            !projects.isEmpty() ? projects.first()->projectDirectory().toString() : QString();
    sp.projectSourceFiles = sourceFiles;
    sp.sysRoot = SysRootKitInformation::sysRoot(kit).toString();
    DebuggerRunControlFactory::createAndScheduleRun(sp);
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
    foreach (const DebuggerStartParameters &sp, m_scheduledStarts)
        DebuggerRunControlFactory::createAndScheduleRun(sp);
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
    ContextData args;
    args.lineNumber = lineNumber;
    bool contextUsable = true;

    Breakpoint bp;
    TextDocument *document = widget->textDocument();
    args.fileName = document->filePath().toString();
    if (document->property(Constants::OPENED_WITH_DISASSEMBLY).toBool()) {
        QString line = document->plainText()
            .section(QLatin1Char('\n'), lineNumber - 1, lineNumber - 1);
        BreakpointResponse needle;
        needle.type = BreakpointByAddress;
        needle.address = DisassemblerLine::addressFromDisassemblyLine(line);
        args.address = needle.address;
        needle.lineNumber = -1;
        bp = breakHandler()->findSimilarBreakpoint(needle);
        contextUsable = args.address != 0;
    } else {
        bp = breakHandler()
            ->findBreakpointByFileAndLine(args.fileName, lineNumber);
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
            BreakTreeView::editBreakpoint(bp, ICore::dialogParent());
        });

    } else {
        // Handle non-existing breakpoint.
        const QString text = args.address
            ? tr("Set Breakpoint at 0x%1").arg(args.address, 0, 16)
            : tr("Set Breakpoint at Line %1").arg(lineNumber);
        auto act = menu->addAction(text);
        act->setEnabled(contextUsable);
        connect(act, &QAction::triggered, [this, args] {
            breakpointSetMarginActionTriggered(false, args);
        });

        // Message trace point
        const QString tracePointText = args.address
            ? tr("Set Message Tracepoint at 0x%1...").arg(args.address, 0, 16)
            : tr("Set Message Tracepoint at Line %1...").arg(lineNumber);
        act = menu->addAction(tracePointText);
        act->setEnabled(contextUsable);
        connect(act, &QAction::triggered, [this, args] {
            breakpointSetMarginActionTriggered(true, args);
        });
    }

    // Run to, jump to line below in stopped state.
    if (currentEngine()->state() == InferiorStopOk && contextUsable) {
        menu->addSeparator();
        if (currentEngine()->hasCapability(RunToLineCapability)) {
            auto act = menu->addAction(args.address
                ? DebuggerEngine::tr("Run to Address 0x%1").arg(args.address, 0, 16)
                : DebuggerEngine::tr("Run to Line %1").arg(args.lineNumber));
            connect(act, &QAction::triggered, [this, args] {
                currentEngine()->executeRunToLine(args);
            });
        }
        if (currentEngine()->hasCapability(JumpToLineCapability)) {
            auto act = menu->addAction(args.address
                ? DebuggerEngine::tr("Jump to Address 0x%1").arg(args.address, 0, 16)
                : DebuggerEngine::tr("Jump to Line %1").arg(args.lineNumber));
            connect(act, &QAction::triggered, [this, args] {
                currentEngine()->executeJumpToLine(args);
            });
        }
        // Disassemble current function in stopped state.
        if (currentEngine()->hasCapability(DisassemblerCapability)) {
            StackFrame frame;
            frame.function = cppFunctionAt(args.fileName, lineNumber, 1);
            frame.line = 42; // trick gdb into mixed mode.
            if (!frame.function.isEmpty()) {
                const QString text = tr("Disassemble Function \"%1\"")
                    .arg(frame.function);
                auto act = new QAction(text, menu);
                connect(act, &QAction::triggered, [this, frame] {
                    currentEngine()->openDisassemblerView(Location(frame));
                });
                menu->addAction(act);
            }
        }
    }
}

void DebuggerPluginPrivate::toggleBreakpoint()
{
    BaseTextEditor *textEditor = BaseTextEditor::currentTextEditor();
    QTC_ASSERT(textEditor, return);
    const int lineNumber = textEditor->currentLine();
    if (textEditor->property(Constants::OPENED_WITH_DISASSEMBLY).toBool()) {
        QString line = textEditor->textDocument()->plainText()
            .section(QLatin1Char('\n'), lineNumber - 1, lineNumber - 1);
        quint64 address = DisassemblerLine::addressFromDisassemblyLine(line);
        toggleBreakpointByAddress(address);
    } else if (lineNumber >= 0) {
        toggleBreakpointByFileAndLine(textEditor->document()->filePath().toString(), lineNumber);
    }
}

void DebuggerPluginPrivate::toggleBreakpointByFileAndLine(const QString &fileName,
    int lineNumber, const QString &tracePointMessage)
{
    BreakHandler *handler = m_breakHandler;
    Breakpoint bp = handler->findBreakpointByFileAndLine(fileName, lineNumber, true);
    if (!bp)
        bp = handler->findBreakpointByFileAndLine(fileName, lineNumber, false);

    if (bp) {
        bp.removeBreakpoint();
    } else {
        BreakpointParameters data(BreakpointByFileAndLine);
        if (boolSetting(BreakpointsFullPathByDefault))
            data.pathUsage = BreakpointUseFullPath;
        data.tracepoint = !tracePointMessage.isEmpty();
        data.message = tracePointMessage;
        data.fileName = fileName;
        data.lineNumber = lineNumber;
        handler->appendBreakpoint(data);
    }
}

void DebuggerPluginPrivate::toggleBreakpointByAddress(quint64 address,
                                                      const QString &tracePointMessage)
{
    BreakHandler *handler = m_breakHandler;
    if (Breakpoint bp = handler->findBreakpointByAddress(address)) {
        bp.removeBreakpoint();
    } else {
        BreakpointParameters data(BreakpointByAddress);
        data.tracepoint = !tracePointMessage.isEmpty();
        data.message = tracePointMessage;
        data.address = address;
        handler->appendBreakpoint(data);
    }
}

void DebuggerPluginPrivate::requestMark(TextEditorWidget *widget, int lineNumber,
                                        TextMarkRequestKind kind)
{
    if (kind != BreakpointRequest)
        return;

    TextDocument *document = widget->textDocument();
    if (document->property(Constants::OPENED_WITH_DISASSEMBLY).toBool()) {
        QString line = document->plainText()
                .section(QLatin1Char('\n'), lineNumber - 1, lineNumber - 1);
        quint64 address = DisassemblerLine::addressFromDisassemblyLine(line);
        toggleBreakpointByAddress(address);
    } else {
        toggleBreakpointByFileAndLine(document->filePath().toString(), lineNumber);
    }
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

void DebuggerPluginPrivate::connectEngine(DebuggerEngine *engine)
{
    if (!engine)
        engine = dummyEngine();

    if (m_currentEngine == engine)
        return;

    if (m_currentEngine)
        m_currentEngine->resetLocation();
    m_currentEngine = engine;

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

    m_mainWindow->setEngineDebugLanguages(engine->startParameters().languages);
}

static void changeFontSize(QWidget *widget, qreal size)
{
    QFont font = widget->font();
    font.setPointSizeF(size);
    widget->setFont(font);
}

void DebuggerPluginPrivate::fontSettingsChanged
    (const FontSettings &settings)
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
            else if (document->filePath().toString().contains(_("qeventdispatcher")))
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
    m_watchersWindow->setVisible(false);
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

    m_watchAction1->setEnabled(true);
    m_watchAction2->setEnabled(true);
    m_breakAction->setEnabled(false);
    //m_snapshotAction->setEnabled(false);
    action(OperateByInstruction)->setEnabled(false);

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
}

void DebuggerPluginPrivate::updateState(DebuggerEngine *engine)
{
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
        m_localsAndExpressionsWindow->setShowLocals(true);
    } else if (state == InferiorRunOk) {
        // Shift-F5 interrupts. It is also "interruptible".
        m_interruptAction->setEnabled(true);
        m_continueAction->setEnabled(false);
        m_exitAction->setEnabled(true);
        m_startAction->setEnabled(false);
        m_debugWithoutDeployAction->setEnabled(false);
        setProxyAction(m_visibleStartAction, Id(Constants::INTERRUPT));
        m_hiddenStopAction->setAction(m_interruptAction);
        m_localsAndExpressionsWindow->setShowLocals(false);
    } else if (state == DebuggerFinished) {
        Project *project = SessionManager::startupProject();
        const bool canRun = ProjectExplorerPlugin::canRun(project, DebugRunMode);
        // We don't want to do anything anymore.
        m_interruptAction->setEnabled(false);
        m_continueAction->setEnabled(false);
        m_exitAction->setEnabled(false);
        m_startAction->setEnabled(canRun);
        m_debugWithoutDeployAction->setEnabled(canRun);
        setProxyAction(m_visibleStartAction, Id(Constants::DEBUG));
        m_hiddenStopAction->setAction(m_undisturbableAction);
        m_codeModelSnapshot = CPlusPlus::Snapshot();
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
        m_localsAndExpressionsWindow->setShowLocals(true);
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

    const bool isCore = engine->startParameters().startMode == AttachCore;
    const bool stopped = state == InferiorStopOk;
    const bool detachable = stopped && !isCore;
    m_detachAction->setEnabled(detachable);

    if (stopped)
        QApplication::alert(m_mainWindow, 3000);

    const bool canReverse = engine->hasCapability(ReverseSteppingCapability)
                && boolSetting(EnableReverseDebugging);
    m_reverseDirectionAction->setEnabled(canReverse);

    m_watchAction1->setEnabled(true);
    m_watchAction2->setEnabled(true);
    m_breakAction->setEnabled(true);

    const bool canOperateByInstruction = engine->hasCapability(OperateByInstructionCapability)
            && (stopped || isCore);
    action(OperateByInstruction)->setEnabled(canOperateByInstruction);

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
    //if we're currently debugging the actions are controlled by engine
    if (m_currentEngine->state() != DebuggerNotReady)
        return;

    Project *project = SessionManager::startupProject();
    QString whyNot;
    const bool canRun = ProjectExplorerPlugin::canRun(project, DebugRunMode, &whyNot);
    m_startAction->setEnabled(canRun);
    m_startAction->setToolTip(whyNot);
    m_debugWithoutDeployAction->setEnabled(canRun);

    // Step into/next: Start and break at 'main' unless a debugger is running.
    if (m_snapshotHandler->currentIndex() < 0) {
        QString toolTip;
        const bool canRunAndBreakMain
                = ProjectExplorerPlugin::canRun(project, DebugRunModeWithBreakOnMain, &toolTip);
        m_stepAction->setEnabled(canRunAndBreakMain);
        m_nextAction->setEnabled(canRunAndBreakMain);
        if (canRunAndBreakMain) {
            QTC_ASSERT(project, return ; );
            toolTip = tr("Start \"%1\" and break at function \"main()\"")
                      .arg(project->displayName());
        }
        m_stepAction->setToolTip(toolTip);
        m_nextAction->setToolTip(toolTip);
    }
}

void DebuggerPluginPrivate::onCoreAboutToOpen()
{
    m_mainWindow->onModeChanged(ModeManager::currentMode());
}

void DebuggerPluginPrivate::onModeChanged(IMode *mode)
{
     // FIXME: This one gets always called, even if switching between modes
     //        different then the debugger mode. E.g. Welcome and Help mode and
     //        also on shutdown.

    m_mainWindow->onModeChanged(mode);

    if (mode->id() != Constants::MODE_DEBUG) {
        m_toolTipManager.leavingDebugMode();
        return;
    }

    if (IEditor *editor = EditorManager::currentEditor())
        editor->widget()->setFocus();

    m_toolTipManager.debugModeEntered();
}

void DebuggerPluginPrivate::updateDebugWithoutDeployMenu()
{
    const bool state = ProjectExplorerPlugin::projectExplorerSettings().deployBeforeRun;
    m_debugWithoutDeployAction->setVisible(state);
}

void DebuggerPluginPrivate::dumpLog()
{
    QString fileName = QFileDialog::getSaveFileName(ICore::mainWindow(),
        tr("Save Debugger Log"), QDir::tempPath());
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
    if (ModeManager::currentMode() == ModeManager::mode(MODE_DEBUG)
            && m_previousMode.isValid()) {
        ModeManager::activateMode(m_previousMode);
        m_previousMode = Id();
    }
}

void DebuggerPluginPrivate::activateDebugMode()
{
    m_reverseDirectionAction->setChecked(false);
    m_reverseDirectionAction->setEnabled(false);
    m_previousMode = ModeManager::currentMode()->id();
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
    showMessage(msg0, LogStatus);
    QString msg = msg0;
    msg.remove(QLatin1Char('\n'));
    m_statusLabel->showStatusMessage(msg, timeout);
}

void DebuggerPluginPrivate::coreShutdown()
{
    m_shuttingDown = true;
}

const CPlusPlus::Snapshot &cppCodeModelSnapshot()
{
    if (dd->m_codeModelSnapshot.isEmpty() && action(UseCodeModel)->isChecked())
        dd->m_codeModelSnapshot = CppTools::CppModelManager::instance()->snapshot();
    return dd->m_codeModelSnapshot;
}

void setSessionValue(const QByteArray &key, const QVariant &value)
{
    SessionManager::setValue(QString::fromUtf8(key), value);
}

QVariant sessionValue(const QByteArray &key)
{
    return SessionManager::value(QString::fromUtf8(key));
}

QTreeView *inspectorView()
{
    return dd->m_inspectorView;
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
        case LogError:
            m_logWindow->showInput(LogError, QLatin1String("ERROR: ") + msg);
            m_logWindow->showOutput(LogError, QLatin1String("ERROR: ") + msg);
            break;
        default:
            m_logWindow->showOutput(channel, msg);
            break;
    }
}

void createNewDock(QWidget *widget)
{
    QDockWidget *dockWidget = dd->m_mainWindow->createDockWidget(CppLanguage, widget);
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
    str << "Languages: ";
    if (sp.languages == AnyLanguage)
        str << "any";
    if (sp.languages & CppLanguage)
        str << "c++ ";
    if (sp.languages & QmlLanguage)
        str << "qml";
    str << '\n';
    if (!sp.executable.isEmpty()) {
        str << "Executable: " << QDir::toNativeSeparators(sp.executable)
            << ' ' << sp.processArgs;
        if (sp.useTerminal)
            str << " [terminal]";
        str << '\n';
        if (!sp.workingDirectory.isEmpty())
            str << "Directory: " << QDir::toNativeSeparators(sp.workingDirectory)
                << '\n';
    }
    QString cmd = sp.debuggerCommand;
    if (!cmd.isEmpty())
        str << "Debugger: " << QDir::toNativeSeparators(cmd) << '\n';
    if (!sp.coreFile.isEmpty())
        str << "Core: " << QDir::toNativeSeparators(sp.coreFile) << '\n';
    if (sp.attachPID > 0)
        str << "PID: " << sp.attachPID << ' ' << sp.crashParameter << '\n';
    if (!sp.projectSourceDirectory.isEmpty()) {
        str << "Project: " << QDir::toNativeSeparators(sp.projectSourceDirectory);
        if (!sp.projectBuildDirectory.isEmpty())
            str << " (built: " << QDir::toNativeSeparators(sp.projectBuildDirectory)
                << ')';
        str << '\n';
    }
    if (!sp.qmlServerAddress.isEmpty())
        str << "QML server: " << sp.qmlServerAddress << ':'
            << sp.qmlServerPort << '\n';
    if (!sp.remoteChannel.isEmpty()) {
        str << "Remote: " << sp.remoteChannel << '\n';
        if (!sp.remoteSourcesDir.isEmpty())
            str << "Remote sources: " << sp.remoteSourcesDir << '\n';
        if (!sp.remoteMountPoint.isEmpty())
            str << "Remote mount point: " << sp.remoteMountPoint
                << " Local: " << sp.localMountDir << '\n';
    }
    str << "Sysroot: " << sp.sysRoot << '\n';
    str << "Debug Source Location: " << sp.debugSourceLocation.join(QLatin1Char(':')) << '\n';
    return rc;
}

void DebuggerPluginPrivate::runControlStarted(DebuggerEngine *engine)
{
    activateDebugMode();
    const QString message = tr("Starting debugger \"%1\" for ABI \"%2\"...")
            .arg(engine->objectName())
            .arg(engine->startParameters().toolChainAbi.toString());
    showStatusMessage(message);
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
    action(OperateByInstruction)->setValue(QVariant(false));
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

bool isNativeMixedEnabled()
{
    static bool enabled = qEnvironmentVariableIsSet("QTC_DEBUGGER_NATIVE_MIXED");
    return enabled;
}

bool isNativeMixedActive()
{
    return isNativeMixedEnabled() && boolSetting(OperateNativeMixed);
}

void DebuggerPluginPrivate::extensionsInitialized()
{
    const QKeySequence debugKey = QKeySequence(UseMacShortcuts ? tr("Ctrl+Y") : tr("F5"));

    QSettings *settings = ICore::settings();

    m_debuggerSettings = new DebuggerSettings;
    m_debuggerSettings->readSettings();

    connect(ICore::instance(), &ICore::coreAboutToClose, this, &DebuggerPluginPrivate::coreShutdown);

    const Context cppDebuggercontext(C_CPPDEBUGGER);
    const Context cppeditorcontext(CppEditor::Constants::CPPEDITOR_ID);

    m_startIcon = QIcon(_(":/debugger/images/debugger_start_small.png"));
    m_startIcon.addFile(QLatin1String(":/debugger/images/debugger_start.png"));
    m_exitIcon = QIcon(_(":/debugger/images/debugger_stop_small.png"));
    m_exitIcon.addFile(QLatin1String(":/debugger/images/debugger_stop.png"));
    m_continueIcon = QIcon(QLatin1String(":/debugger/images/debugger_continue_small.png"));
    m_continueIcon.addFile(QLatin1String(":/debugger/images/debugger_continue.png"));
    m_interruptIcon = QIcon(_(Core::Constants::ICON_PAUSE));
    m_interruptIcon.addFile(QLatin1String(":/debugger/images/debugger_interrupt.png"));
    m_locationMarkIcon = QIcon(_(":/debugger/images/location_16.png"));
    m_resetIcon = QIcon(_(":/debugger/images/debugger_restart_small.png:"));
    m_resetIcon.addFile(QLatin1String(":/debugger/images/debugger_restart.png"));

    m_busy = false;

    m_statusLabel = new StatusLabel;

    m_logWindow = new LogWindow;
    m_logWindow->setObjectName(QLatin1String(DOCKWIDGET_OUTPUT));

    m_breakHandler = new BreakHandler;
    m_breakView = new BreakTreeView;
    m_breakView->setSettings(settings, "Debugger.BreakWindow");
    m_breakView->setModel(m_breakHandler->model());
    m_breakWindow = addSearch(m_breakView, tr("Breakpoints"), DOCKWIDGET_BREAK);

    m_modulesView = new ModulesTreeView;
    m_modulesView->setSettings(settings, "Debugger.ModulesView");
    m_modulesWindow = addSearch(m_modulesView, tr("Modules"), DOCKWIDGET_MODULES);

    m_registerView = new RegisterTreeView;
    m_registerView->setSettings(settings, "Debugger.RegisterView");
    m_registerWindow = addSearch(m_registerView, tr("Registers"), DOCKWIDGET_REGISTER);

    m_stackView = new StackTreeView;
    m_stackView->setSettings(settings, "Debugger.StackView");
    m_stackWindow = addSearch(m_stackView, tr("Stack"), DOCKWIDGET_STACK);

    m_sourceFilesView = new SourceFilesTreeView;
    m_sourceFilesView->setSettings(settings, "Debugger.SourceFilesView");
    m_sourceFilesWindow = addSearch(m_sourceFilesView, tr("Source Files"), DOCKWIDGET_SOURCE_FILES);

    m_threadsView = new ThreadsTreeView;
    m_threadsView->setSettings(settings, "Debugger.ThreadsView");
    m_threadsWindow = addSearch(m_threadsView, tr("Threads"), DOCKWIDGET_THREADS);

    m_returnView = new WatchTreeView(ReturnType); // No settings.
    m_returnWindow = addSearch(m_returnView, tr("Locals and Expressions"), "CppDebugReturn");

    m_localsView = new WatchTreeView(LocalsType);
    m_localsView->setSettings(settings, "Debugger.LocalsView");
    m_localsWindow = addSearch(m_localsView, tr("Locals and Expressions"), "CppDebugLocals");

    m_watchersView = new WatchTreeView(WatchersType); // No settings.
    m_watchersWindow = addSearch(m_watchersView, tr("Locals and Expressions"), "CppDebugWatchers");

    m_inspectorView = new WatchTreeView(InspectType);
    m_inspectorView->setSettings(settings, "Debugger.LocalsView"); // sic! same as locals view.
    m_inspectorWindow = addSearch(m_inspectorView, tr("Locals and Expressions"), "Inspector");

    // Snapshot
    m_snapshotHandler = new SnapshotHandler;
    m_snapshotView = new SnapshotTreeView(m_snapshotHandler);
    m_snapshotView->setSettings(settings, "Debugger.SnapshotView");
    m_snapshotView->setModel(m_snapshotHandler->model());
    m_snapshotWindow = addSearch(m_snapshotView, tr("Snapshots"), DOCKWIDGET_SNAPSHOTS);

    // Watchers
    connect(m_localsView->header(), &QHeaderView::sectionResized,
        this, &DebuggerPluginPrivate::updateWatchersHeader, Qt::QueuedConnection);

    QAction *act = 0;

    act = m_continueAction = new QAction(tr("Continue"), this);
    act->setIcon(m_continueIcon);
    connect(act, &QAction::triggered, this, &DebuggerPluginPrivate::handleExecContinue);

    act = m_exitAction = new QAction(tr("Stop Debugger"), this);
    act->setIcon(m_exitIcon);
    connect(act, &QAction::triggered, this, &DebuggerPluginPrivate::handleExecExit);

    act = m_interruptAction = new QAction(tr("Interrupt"), this);
    act->setIcon(m_interruptIcon);
    connect(act, &QAction::triggered, this, &DebuggerPluginPrivate::handleExecInterrupt);

    // A "disabled pause" seems to be a good choice.
    act = m_undisturbableAction = new QAction(tr("Debugger is Busy"), this);
    act->setIcon(m_interruptIcon);
    act->setEnabled(false);

    act = m_abortAction = new QAction(tr("Abort Debugging"), this);
    act->setToolTip(tr("Aborts debugging and "
        "resets the debugger to the initial state."));
    connect(act, &QAction::triggered, this, &DebuggerPluginPrivate::handleAbort);

    act = m_resetAction = new QAction(tr("Restart Debugging"),this);
    act->setToolTip(tr("Restart the debugging session."));
    act->setIcon(m_resetIcon);
    connect(act, &QAction::triggered, this, &DebuggerPluginPrivate::handleReset);

    act = m_nextAction = new QAction(tr("Step Over"), this);
    act->setIcon(QIcon(QLatin1String(":/debugger/images/debugger_stepover_small.png")));
    connect(act, &QAction::triggered, this, &DebuggerPluginPrivate::handleExecNext);

    act = m_stepAction = new QAction(tr("Step Into"), this);
    act->setIcon(QIcon(QLatin1String(":/debugger/images/debugger_stepinto_small.png")));
    connect(act, &QAction::triggered, this, &DebuggerPluginPrivate::handleExecStep);

    act = m_stepOutAction = new QAction(tr("Step Out"), this);
    act->setIcon(QIcon(QLatin1String(":/debugger/images/debugger_stepout_small.png")));
    connect(act, &QAction::triggered, this, &DebuggerPluginPrivate::handleExecStepOut);

    act = m_runToLineAction = new QAction(tr("Run to Line"), this);
    connect(act, &QAction::triggered, this, &DebuggerPluginPrivate::handleExecRunToLine);

    act = m_runToSelectedFunctionAction =
        new QAction(tr("Run to Selected Function"), this);
    connect(act, &QAction::triggered, this, &DebuggerPluginPrivate::handleExecRunToSelectedFunction);

    act = m_returnFromFunctionAction =
        new QAction(tr("Immediately Return From Inner Function"), this);
    connect(act, &QAction::triggered, this, &DebuggerPluginPrivate::handleExecReturn);

    act = m_jumpToLineAction = new QAction(tr("Jump to Line"), this);
    connect(act, &QAction::triggered, this, &DebuggerPluginPrivate::handleExecJumpToLine);

    m_breakAction = new QAction(tr("Toggle Breakpoint"), this);

    act = m_watchAction1 = new QAction(tr("Add Expression Evaluator"), this);
    connect(act, &QAction::triggered, this, &DebuggerPluginPrivate::handleAddToWatchWindow);

    act = m_watchAction2 = new QAction(tr("Add Expression Evaluator"), this);
    connect(act, &QAction::triggered, this, &DebuggerPluginPrivate::handleAddToWatchWindow);

    //m_snapshotAction = new QAction(tr("Create Snapshot"), this);
    //m_snapshotAction->setProperty(Role, RequestCreateSnapshotRole);
    //m_snapshotAction->setIcon(
    //    QIcon(__(":/debugger/images/debugger_snapshot_small.png")));

    act = m_reverseDirectionAction =
        new QAction(tr("Reverse Direction"), this);
    act->setCheckable(true);
    act->setChecked(false);
    act->setCheckable(false);
    act->setIcon(QIcon(QLatin1String(":/debugger/images/debugger_reversemode_16.png")));
    act->setIconVisibleInMenu(false);

    act = m_frameDownAction = new QAction(tr("Move to Called Frame"), this);
    connect(act, &QAction::triggered, this, &DebuggerPluginPrivate::handleFrameDown);

    act = m_frameUpAction = new QAction(tr("Move to Calling Frame"), this);
    connect(act, &QAction::triggered, this, &DebuggerPluginPrivate::handleFrameUp);

    connect(action(OperateByInstruction), SIGNAL(triggered(bool)),
        SLOT(handleOperateByInstructionTriggered(bool)));

    ActionContainer *debugMenu = ActionManager::actionContainer(PE::M_DEBUG);

    // Dock widgets
    QDockWidget *dock = 0;
    dock = m_mainWindow->createDockWidget(CppLanguage, m_modulesWindow);
    connect(dock->toggleViewAction(), &QAction::toggled,
        this, &DebuggerPluginPrivate::modulesDockToggled, Qt::QueuedConnection);

    dock = m_mainWindow->createDockWidget(CppLanguage, m_registerWindow);
    connect(dock->toggleViewAction(), &QAction::toggled,
        this, &DebuggerPluginPrivate::registerDockToggled, Qt::QueuedConnection);

    dock = m_mainWindow->createDockWidget(CppLanguage, m_sourceFilesWindow);
    connect(dock->toggleViewAction(), &QAction::toggled,
        this, &DebuggerPluginPrivate::sourceFilesDockToggled, Qt::QueuedConnection);

    dock = m_mainWindow->createDockWidget(AnyLanguage, m_logWindow);
    dock->setProperty(DOCKWIDGET_DEFAULT_AREA, Qt::TopDockWidgetArea);

    m_mainWindow->createDockWidget(CppLanguage, m_breakWindow);
    m_mainWindow->createDockWidget(CppLanguage, m_snapshotWindow);
    m_mainWindow->createDockWidget(CppLanguage, m_stackWindow);
    m_mainWindow->createDockWidget(CppLanguage, m_threadsWindow);

    m_localsAndExpressionsWindow = new LocalsAndExpressionsWindow(
                m_localsWindow, m_inspectorWindow, m_returnWindow,
                m_watchersWindow);
    m_localsAndExpressionsWindow->setObjectName(QLatin1String(DOCKWIDGET_WATCHERS));
    m_localsAndExpressionsWindow->setWindowTitle(m_localsWindow->windowTitle());

    dock = m_mainWindow->createDockWidget(CppLanguage, m_localsAndExpressionsWindow);
    dock->setProperty(DOCKWIDGET_DEFAULT_AREA, Qt::RightDockWidgetArea);

    m_mainWindow->addStagedMenuEntries();

    // Register factory of DebuggerRunControl.
    m_debuggerRunControlFactory = new DebuggerRunControlFactory(m_plugin);
    m_plugin->addAutoReleasedObject(m_debuggerRunControlFactory);

    // The main "Start Debugging" action.
    act = m_startAction = new QAction(this);
    QIcon debuggerIcon(QLatin1String(":/projectexplorer/images/debugger_start_small.png"));
    debuggerIcon.addFile(QLatin1String(":/projectexplorer/images/debugger_start.png"));
    act->setIcon(debuggerIcon);
    act->setText(tr("Start Debugging"));
    connect(act, &QAction::triggered, [] { ProjectExplorerPlugin::runStartupProject(DebugRunMode); });

    act = m_debugWithoutDeployAction = new QAction(this);
    act->setText(tr("Start Debugging Without Deployment"));
    connect(act, &QAction::triggered, [] { ProjectExplorerPlugin::runStartupProject(DebugRunMode, true); });

    act = m_startAndDebugApplicationAction = new QAction(this);
    act->setText(tr("Start and Debug External Application..."));
    connect(act, &QAction::triggered, this, &DebuggerPluginPrivate::startAndDebugApplication);

    act = m_attachToCoreAction = new QAction(this);
    act->setText(tr("Load Core File..."));
    connect(act, &QAction::triggered, this, &DebuggerPluginPrivate::attachCore);

    act = m_attachToRemoteServerAction = new QAction(this);
    act->setText(tr("Attach to Running Debug Server..."));
    connect(act, &QAction::triggered, this, &DebuggerPluginPrivate::attachToRemoteServer);

    act = m_startRemoteServerAction = new QAction(this);
    act->setText(tr("Start Debug Server Attached to Process..."));
    connect(act, &QAction::triggered, this, &DebuggerPluginPrivate::startRemoteServerAndAttachToProcess);

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

    Command *cmd = 0;
    ActionContainer *mstart = ActionManager::actionContainer(PE::M_DEBUG_STARTDEBUGGING);

    cmd = ActionManager::registerAction(m_startAction, Constants::DEBUG);
    cmd->setDescription(tr("Start Debugging"));
    cmd->setDefaultKeySequence(debugKey);
    cmd->setAttribute(Command::CA_UpdateText);
    mstart->addAction(cmd, CC::G_DEFAULT_ONE);
    m_visibleStartAction = new ProxyAction(this);
    m_visibleStartAction->initialize(cmd->action());
    m_visibleStartAction->setAttribute(ProxyAction::UpdateText);
    m_visibleStartAction->setAttribute(ProxyAction::UpdateIcon);
    m_visibleStartAction->setAction(cmd->action());

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

    cmd = ActionManager::registerAction(m_startRemoteServerAction,
         "Debugger.StartRemoteServer");
    cmd->setDescription(tr("Start Gdbserver"));
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
    cmd->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Shift+Ctrl+Y") : tr("Shift+F5")));

    cmd = ActionManager::registerAction(m_abortAction, Constants::ABORT);
    cmd->setDescription(tr("Reset Debugger"));
    debugMenu->addAction(cmd, CC::G_DEFAULT_ONE);

    cmd = ActionManager::registerAction(m_resetAction, Constants::RESET);
    cmd->setDescription(tr("Restart Debugging"));
    debugMenu->addAction(cmd, CC::G_DEFAULT_ONE);

    debugMenu->addSeparator();

    cmd = ActionManager::registerAction(m_nextAction, Constants::NEXT);
    cmd->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Ctrl+Shift+O") : tr("F10")));
    cmd->setAttribute(Command::CA_Hide);
    cmd->setAttribute(Command::CA_UpdateText);
    debugMenu->addAction(cmd);

    cmd = ActionManager::registerAction(m_stepAction, Constants::STEP);
    cmd->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Ctrl+Shift+I") : tr("F11")));
    cmd->setAttribute(Command::CA_Hide);
    cmd->setAttribute(Command::CA_UpdateText);
    debugMenu->addAction(cmd);

    cmd = ActionManager::registerAction(m_stepOutAction,
        Constants::STEPOUT, cppDebuggercontext);
    cmd->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Ctrl+Shift+T") : tr("Shift+F11")));
    cmd->setAttribute(Command::CA_Hide);
    debugMenu->addAction(cmd);

    cmd = ActionManager::registerAction(m_runToLineAction,
        "Debugger.RunToLine", cppDebuggercontext);
    cmd->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Shift+F8") : tr("Ctrl+F10")));
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

    cmd = ActionManager::registerAction(m_reverseDirectionAction,
        Constants::REVERSE, cppDebuggercontext);
    cmd->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? QString() : tr("F12")));
    cmd->setAttribute(Command::CA_Hide);
    debugMenu->addAction(cmd);

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

    cmd = ActionManager::registerAction(action(OperateByInstruction),
        Constants::OPERATE_BY_INSTRUCTION, cppDebuggercontext);
    cmd->setAttribute(Command::CA_Hide);
    debugMenu->addAction(cmd);

    if (isNativeMixedEnabled()) {
        SavedAction *act = action(OperateNativeMixed);
        act->setValue(true);
        cmd = ActionManager::registerAction(act, Constants::OPERATE_NATIVE_MIXED);
        cmd->setAttribute(Command::CA_Hide);
        debugMenu->addAction(cmd);
        connect(cmd->action(), &QAction::triggered,
            [this] { currentEngine()->updateAll(); });
    }

    cmd = ActionManager::registerAction(m_breakAction, "Debugger.ToggleBreak");
    cmd->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("F8") : tr("F9")));
    debugMenu->addAction(cmd);
    connect(m_breakAction, &QAction::triggered,
        this, &DebuggerPluginPrivate::toggleBreakpoint);

    debugMenu->addSeparator();

    // currently broken
//    QAction *qmlUpdateOnSaveDummyAction = new QAction(tr("Apply Changes on Save"), this);
//    qmlUpdateOnSaveDummyAction->setCheckable(true);
//    qmlUpdateOnSaveDummyAction->setIcon(QIcon(_(":/debugger/images/qml/apply-on-save.png")));
//    qmlUpdateOnSaveDummyAction->setEnabled(false);
//    cmd = ActionManager::registerAction(qmlUpdateOnSaveDummyAction, Constants::QML_UPDATE_ON_SAVE);
//    debugMenu->addAction(cmd);

    QAction *qmlShowAppOnTopDummyAction = new QAction(tr("Show Application on Top"), this);
    qmlShowAppOnTopDummyAction->setCheckable(true);
    qmlShowAppOnTopDummyAction->setIcon(QIcon(_(":/debugger/images/qml/app-on-top.png")));
    qmlShowAppOnTopDummyAction->setEnabled(false);
    cmd = ActionManager::registerAction(qmlShowAppOnTopDummyAction, Constants::QML_SHOW_APP_ON_TOP);
    debugMenu->addAction(cmd);

    QAction *qmlSelectDummyAction = new QAction(tr("Select"), this);
    qmlSelectDummyAction->setCheckable(true);
    qmlSelectDummyAction->setIcon(QIcon(_(":/debugger/images/qml/select.png")));
    qmlSelectDummyAction->setEnabled(false);
    cmd = ActionManager::registerAction(qmlSelectDummyAction, Constants::QML_SELECTTOOL);
    debugMenu->addAction(cmd);

    QAction *qmlZoomDummyAction = new QAction(tr("Zoom"), this);
    qmlZoomDummyAction->setCheckable(true);
    qmlZoomDummyAction->setIcon(QIcon(_(":/debugger/images/qml/zoom.png")));
    qmlZoomDummyAction->setEnabled(false);
    cmd = ActionManager::registerAction(qmlZoomDummyAction, Constants::QML_ZOOMTOOL);
    debugMenu->addAction(cmd);

    debugMenu->addSeparator();

    // Don't add '1' to the string as it shows up in the shortcut dialog.
    cmd = ActionManager::registerAction(m_watchAction1,
        "Debugger.AddToWatch", cppeditorcontext);
    //cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+D,Ctrl+W")));
    debugMenu->addAction(cmd);

    // If the CppEditor plugin is there, we want to add something to
    // the editor context menu.
    if (ActionContainer *editorContextMenu =
            ActionManager::actionContainer(CppEditor::Constants::M_CONTEXT)) {
        cmd = editorContextMenu->addSeparator(cppDebuggercontext);
        cmd->setAttribute(Command::CA_Hide);

        cmd = ActionManager::registerAction(m_watchAction2,
            "Debugger.AddToWatch2", cppDebuggercontext);
        cmd->action()->setEnabled(true);
        editorContextMenu->addAction(cmd);
        cmd->setAttribute(Command::CA_Hide);
        cmd->setAttribute(Command::CA_NonConfigurable);
        // Debugger.AddToWatch is enough.
    }

    QList<IOptionsPage *> engineOptionPages;
    addGdbOptionPages(&engineOptionPages);
    addCdbOptionPages(&engineOptionPages);

    foreach (IOptionsPage *op, engineOptionPages)
        m_plugin->addAutoReleasedObject(op);
    m_plugin->addAutoReleasedObject(new LocalsAndExpressionsOptionsPage);
    m_plugin->addAutoReleasedObject(new DebuggerOptionsPage);

    connect(ModeManager::instance(), &ModeManager::currentModeChanged,
        this, &DebuggerPluginPrivate::onModeChanged);
    connect(ICore::instance(), &ICore::coreAboutToOpen,
        this, &DebuggerPluginPrivate::onCoreAboutToOpen);
    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::settingsChanged,
        this, &DebuggerPluginPrivate::updateDebugWithoutDeployMenu);

    // Debug mode setup
    DebugMode *debugMode = new DebugMode;
    QWidget *widget = m_mainWindow->createContents(debugMode);
    IContext *modeContextObject = new IContext(this);
    modeContextObject->setContext(Context(CC::C_EDITORMANAGER));
    modeContextObject->setWidget(widget);
    ICore::addContextObject(modeContextObject);
    debugMode->setWidget(widget);

    m_plugin->addAutoReleasedObject(debugMode);

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
    QWidget *toolbarContainer = new QWidget;

    QHBoxLayout *hbox = new QHBoxLayout(toolbarContainer);
    hbox->setMargin(0);
    hbox->setSpacing(0);
    hbox->addWidget(toolButton(m_visibleStartAction));
    hbox->addWidget(toolButton(Constants::STOP));
    hbox->addWidget(toolButton(Constants::NEXT));
    hbox->addWidget(toolButton(Constants::STEP));
    hbox->addWidget(toolButton(Constants::STEPOUT));
    hbox->addWidget(toolButton(Constants::RESET));
    hbox->addWidget(toolButton(Constants::OPERATE_BY_INSTRUCTION));
    if (isNativeMixedEnabled())
        hbox->addWidget(toolButton(Constants::OPERATE_NATIVE_MIXED));

    //hbox->addWidget(new StyledSeparator);
    m_reverseToolButton = toolButton(Constants::REVERSE);
    hbox->addWidget(m_reverseToolButton);
    //m_reverseToolButton->hide();

    hbox->addWidget(new StyledSeparator);
    hbox->addWidget(new QLabel(tr("Threads:")));

    m_threadBox = new QComboBox;
    m_threadBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    connect(m_threadBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::activated),
            this, &DebuggerPluginPrivate::selectThread);

    hbox->addWidget(m_threadBox);
    hbox->addSpacerItem(new QSpacerItem(4, 0));

    m_mainWindow->setToolBar(CppLanguage, toolbarContainer);

    QWidget *qmlToolbar = new QWidget(m_mainWindow);
    hbox = new QHBoxLayout(qmlToolbar);
    hbox->setMargin(0);
    hbox->setSpacing(0);
    // currently broken
    //hbox->addWidget(toolButton(Constants::QML_UPDATE_ON_SAVE));
    hbox->addWidget(toolButton(Constants::QML_SHOW_APP_ON_TOP));
    hbox->addWidget(new StyledSeparator);
    hbox->addWidget(toolButton(Constants::QML_SELECTTOOL));
    hbox->addWidget(toolButton(Constants::QML_ZOOMTOOL));
    hbox->addWidget(new StyledSeparator);
    m_mainWindow->setToolBar(QmlLanguage, qmlToolbar);

    m_mainWindow->setToolBar(AnyLanguage, m_statusLabel);

    connect(action(EnableReverseDebugging), &SavedAction::valueChanged,
            this, &DebuggerPluginPrivate::enableReverseDebuggingTriggered);

    setInitialState();
    connectEngine(0);

    connect(SessionManager::instance(), &SessionManager::startupProjectChanged,
        this, &DebuggerPluginPrivate::onCurrentProjectChanged);

    m_commonOptionsPage = new CommonOptionsPage(m_globalDebuggerOptions);
    m_plugin->addAutoReleasedObject(m_commonOptionsPage);

    m_globalDebuggerOptions->fromSettings();
    m_watchersWindow->setVisible(false);
    m_returnWindow->setVisible(false);

    // time gdb -i mi -ex 'b debuggerplugin.cpp:800' -ex r -ex q bin/qtcreator.bin
}

DebuggerEngine *currentEngine()
{
    return dd->m_currentEngine;
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
    QString raw = dd->m_debuggerSettings->item(code)->value().toString();
    return globalMacroExpander()->expand(raw);
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
    foreach (const Section &s, sections) {
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

void DebuggerPluginPrivate::aboutToShutdown()
{
    disconnect(SessionManager::instance(),
        SIGNAL(startupProjectChanged(ProjectExplorer::Project*)),
        this, 0);
}

void updateState(DebuggerEngine *engine)
{
    dd->updateState(engine);
}

void updateWatchersWindow(bool showWatch, bool showReturn)
{
    dd->m_watchersWindow->setVisible(showWatch);
    dd->m_returnWindow->setVisible(showReturn);
}

QIcon locationMarkIcon()
{
    return dd->m_locationMarkIcon;
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
    QTC_ASSERT(editor, return);
}

bool isActiveDebugLanguage(int language)
{
    return dd->m_mainWindow->activeDebugLanguages() & language;
}

// void runTest(const QString &fileName);
void showMessage(const QString &msg, int channel, int timeout)
{
    dd->showMessage(msg, channel, timeout);
}

bool isReverseDebugging()
{
    return dd->m_reverseDirectionAction->isChecked();
}

void runControlStarted(DebuggerEngine *engine)
{
    dd->runControlStarted(engine);
}

void runControlFinished(DebuggerEngine *engine)
{
    dd->runControlFinished(engine);
}

void displayDebugger(DebuggerEngine *engine, bool updateEngine)
{
    dd->displayDebugger(engine, updateEngine);
}

DebuggerLanguages activeLanguages()
{
    QTC_ASSERT(dd->m_mainWindow, return AnyLanguage);
    return dd->m_mainWindow->activeDebugLanguages();
}

void synchronizeBreakpoints()
{
    dd->synchronizeBreakpoints();
}

QWidget *mainWindow()
{
    return dd->m_mainWindow;
}

bool isDockVisible(const QString &objectName)
{
    QDockWidget *dock = dd->m_mainWindow->findChild<QDockWidget *>(objectName);
    return dock && dock->toggleViewAction()->isChecked();
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

void setThreads(const QStringList &list, int index)
{
    dd->setThreads(list, index);
}

QSharedPointer<Internal::GlobalDebuggerOptions> globalDebuggerOptions()
{
    return dd->m_globalDebuggerOptions;
}

} // namespace Internal

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

DebuggerPlugin::DebuggerPlugin()
{
    setObjectName(QLatin1String("DebuggerPlugin"));
    addObject(this);
    dd = new DebuggerPluginPrivate(this);
}

DebuggerPlugin::~DebuggerPlugin()
{
    delete dd;
    dd = 0;
}

bool DebuggerPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    // Menu groups
    ActionContainer *mstart = ActionManager::actionContainer(PE::M_DEBUG_STARTDEBUGGING);
    mstart->appendGroup(Constants::G_GENERAL);
    mstart->appendGroup(Constants::G_SPECIAL);
    mstart->appendGroup(Constants::G_START_QML);

    // Separators
    mstart->addSeparator(Constants::G_GENERAL);
    mstart->addSeparator(Constants::G_SPECIAL);

    addAutoReleasedObject(new DebuggerItemManager);
    DebuggerItemManager::restoreDebuggers();

    KitManager::registerKitInformation(new DebuggerKitInformation);

    return dd->initialize(arguments, errorMessage);
}

IPlugin::ShutdownFlag DebuggerPlugin::aboutToShutdown()
{
    removeObject(this);
    dd->aboutToShutdown();
    return SynchronousShutdown;
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

#ifdef WITH_TESTS

void DebuggerPluginPrivate::testLoadProject(const QString &proFile, const TestCallBack &cb)
{
    connect(ProjectTree::instance(), &ProjectTree::currentProjectChanged,
            this, &DebuggerPluginPrivate::testProjectLoaded);

    m_testCallbacks.append(cb);
    QString error;
    if (ProjectExplorerPlugin::openProject(proFile, &error)) {
        // Will end up in callback below due to the connections to
        // signal currentProjectChanged().
        return;
    }

    // Project opening failed. Eat the unused callback.
    qWarning("Cannot open %s: %s", qPrintable(proFile), qPrintable(error));
    QVERIFY(false);
    m_testCallbacks.pop_back();
}

void DebuggerPluginPrivate::testProjectLoaded(Project *project)
{
    if (!project) {
        qWarning("Changed to null project.");
        return;
    }
    m_testProject = project;
    connect(project, SIGNAL(proFilesEvaluated()), SLOT(testProjectEvaluated()));
    project->configureAsExampleProject(QStringList());
}

void DebuggerPluginPrivate::testProjectEvaluated()
{
    QString fileName = m_testProject->projectFilePath().toUserOutput();
    QVERIFY(!fileName.isEmpty());
    qWarning("Project %s loaded", qPrintable(fileName));
    connect(BuildManager::instance(), SIGNAL(buildQueueFinished(bool)),
            this, SLOT(testProjectBuilt(bool)));
    ProjectExplorerPlugin::buildProject(m_testProject);
}

void DebuggerPluginPrivate::testProjectBuilt(bool success)
{
    QVERIFY(success);
    QVERIFY(!m_testCallbacks.isEmpty());
    TestCallBack cb = m_testCallbacks.takeLast();
    invoke<void>(cb.receiver, cb.slot);
}

void DebuggerPluginPrivate::testUnloadProject()
{
    ProjectExplorerPlugin *pe = ProjectExplorerPlugin::instance();
    invoke<void>(pe, "unloadProject");
}

//static Target *activeTarget()
//{
//    Project *project = ProjectExplorerPlugin::instance()->currentProject();
//    return project->activeTarget();
//}

//static Kit *currentKit()
//{
//    Target *t = activeTarget();
//    if (!t || !t->isEnabled())
//        return 0;
//    return t->kit();
//}

//static LocalApplicationRunConfiguration *activeLocalRunConfiguration()
//{
//    Target *t = activeTarget();
//    return t ? qobject_cast<LocalApplicationRunConfiguration *>(t->activeRunConfiguration()) : 0;
//}

void DebuggerPluginPrivate::testRunProject(const DebuggerStartParameters &sp, const TestCallBack &cb)
{
    m_testCallbacks.append(cb);
    RunControl *rc = DebuggerRunControlFactory::createAndScheduleRun(sp);
    connect(rc, &RunControl::finished, this, &DebuggerPluginPrivate::testRunControlFinished);
}

void DebuggerPluginPrivate::testRunControlFinished()
{
    QVERIFY(!m_testCallbacks.isEmpty());
    TestCallBack cb = m_testCallbacks.takeLast();
    ExtensionSystem::invoke<void>(cb.receiver, cb.slot);
}

void DebuggerPluginPrivate::testFinished()
{
    QTestEventLoop::instance().exitLoop();
    QVERIFY(m_testSuccess);
}


///////////////////////////////////////////////////////////////////////////

//void DebuggerPlugin::testStateMachine()
//{
//    dd->testStateMachine1();
//}

//void DebuggerPluginPrivate::testStateMachine1()
//{
//    m_testSuccess = true;
//    QString proFile = ICore::resourcePath();
//    if (Utils::HostOsInfo::isMacHost())
//        proFile += QLatin1String("/../..");
//    proFile += QLatin1String("/../../tests/manual/debugger/simple/simple.pro");
//    testLoadProject(proFile, TestCallBack(this,  "testStateMachine2"));
//    QVERIFY(m_testSuccess);
//    QTestEventLoop::instance().enterLoop(20);
//}

//void DebuggerPluginPrivate::testStateMachine2()
//{
//    DebuggerStartParameters sp;
//    fillParameters(&sp, currentKit());
//    sp.executable = activeLocalRunConfiguration()->executable();
//    sp.testCase = TestNoBoundsOfCurrentFunction;
//    testRunProject(sp, TestCallBack(this, "testStateMachine3"));
//}

//void DebuggerPluginPrivate::testStateMachine3()
//{
//    testUnloadProject();
//    testFinished();
//}


///////////////////////////////////////////////////////////////////////////

void DebuggerPlugin::testBenchmark()
{
    dd->testBenchmark1();
}

enum FakeEnum { FakeDebuggerCommonSettingsId };

void DebuggerPluginPrivate::testBenchmark1()
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


#endif // if  WITH_TESTS

} // namespace Debugger

#include "debuggerplugin.moc"
