// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "debuggeractions.h"
#include "debuggerinternalconstants.h"
#include "debuggercore.h"
#include "debuggerdialogs.h"
#include "debuggerengine.h"
#include "debuggericons.h"
#include "debuggeritemmanager.h"
#include "debuggermainwindow.h"
#include "debuggerrunconfigurationaspect.h"
#include "debuggerruncontrol.h"
#include "debuggerkitaspect.h"
#include "debuggertest.h"
#include "debuggertr.h"
#include "breakhandler.h"
#include "enginemanager.h"
#include "logwindow.h"
#include "stackframe.h"
#include "unstartedappwatcherdialog.h"
#include "loadcoredialog.h"
#include "sourceutils.h"
#include "shared/coredumputils.h"
#include "shared/hostutils.h"
#include "console/console.h"

#include "analyzer/analyzerutils.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/find/itemviewfind.h>
#include <coreplugin/findplaceholder.h>
#include <coreplugin/icore.h>
#include <coreplugin/imode.h>
#include <coreplugin/messagebox.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/navigationwidget.h>
#include <coreplugin/outputpane.h>
#include <coreplugin/rightpane.h>
#include <coreplugin/session.h>

#include <extensionsystem/iplugin.h>
#include <extensionsystem/pluginmanager.h>

#include <cppeditor/cppeditorconstants.h>
#include <qmljseditor/qmljseditorconstants.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/devicesupport/deviceprocessesdialog.h>
#include <projectexplorer/kitchooser.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorersettings.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainkitaspect.h>

#include <texteditor/texteditor.h>
#include <texteditor/textdocument.h>
#include <texteditor/fontsettings.h>
#include <texteditor/texteditorsettings.h>

#include <utils/algorithm.h>
#include <utils/basetreeview.h>
#include <utils/checkablemessagebox.h>
#include <utils/fancymainwindow.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/processinfo.h>
#include <utils/proxyaction.h>
#include <utils/qtcassert.h>
#include <utils/statuslabel.h>
#include <utils/stringutils.h>
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
#include <QHeaderView>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QMessageBox>
#include <QOperatingSystemVersion>
#include <QPointer>
#include <QPushButton>
#include <QScopeGuard>
#include <QStackedWidget>
#include <QTextBlock>
#include <QToolButton>
#include <QVBoxLayout>
#include <QVariant>

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
                         +  |       +
  EngineRunRequested <+-+'  |       `+-+-+> EngineSetupFailed
                            |                   +
                            |    [calls RunControl->startFailed]
                            |                   +
                            |             DebuggerFinished
                            |
                   ------------------------
                 /     |            |      \
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

Q_DECLARE_METATYPE(QString *)

namespace Debugger {
namespace Internal {

const char DEBUGGER_START[] = "Debugger.Start";

// Menu Groups
const char MENU_GROUP_GENERAL[]              = "Debugger.Group.General";
const char MENU_GROUP_SPECIAL[]              = "Debugger.Group.Special";
const char MENU_GROUP_START_QML[]            = "Debugger.Group.Start.Qml";

void addCdbOptionPages(QList<IOptionsPage*> *opts);

static QIcon startIcon(bool toolBarStyle)
{
    const static QIcon sidebarIcon =
            Icon::sideBarIcon(ProjectExplorer::Icons::DEBUG_START, ProjectExplorer::Icons::DEBUG_START_FLAT);
    const static QIcon icon =
            Icon::combinedIcon({ProjectExplorer::Icons::DEBUG_START_SMALL.icon(), sidebarIcon});
    const static QIcon iconToolBar =
            Icon::combinedIcon({ProjectExplorer::Icons::DEBUG_START_SMALL_TOOLBAR.icon(), sidebarIcon});
    return toolBarStyle ? iconToolBar : icon;
}

static QIcon continueIcon(bool toolBarStyle)
{
    const static QIcon sidebarIcon =
            Icon::sideBarIcon(Icons::CONTINUE, Icons::CONTINUE_FLAT);
    const static QIcon icon =
            Icon::combinedIcon({Icons::DEBUG_CONTINUE_SMALL.icon(), sidebarIcon});
    const static QIcon iconToolBar =
            Icon::combinedIcon({Icons::DEBUG_CONTINUE_SMALL_TOOLBAR.icon(), sidebarIcon});
    return toolBarStyle ? iconToolBar : icon;
}

static QIcon interruptIcon(bool toolBarStyle)
{
    const static QIcon sidebarIcon =
            Icon::sideBarIcon(Icons::INTERRUPT, Icons::INTERRUPT_FLAT);
    const static QIcon icon =
            Icon::combinedIcon({Icons::DEBUG_INTERRUPT_SMALL.icon(), sidebarIcon});
    const static QIcon iconToolBar =
            Icon::combinedIcon({Icons::DEBUG_INTERRUPT_SMALL_TOOLBAR.icon(), sidebarIcon});
    return toolBarStyle ? iconToolBar : icon;
}

static bool hideAnalyzeMenu()
{
    return Core::ICore::settings()
        ->value(ProjectExplorer::Constants::SETTINGS_MENU_HIDE_ANALYZE, false)
        .toBool();
}

static bool hideDebugMenu()
{
    return Core::ICore::settings()
        ->value(ProjectExplorer::Constants::SETTINGS_MENU_HIDE_DEBUG, false)
        .toBool();
}

QAction *addAction(const QObject *parent, QMenu *menu, const QString &display, bool on,
                   const std::function<void()> &onTriggered)
{
    QAction *act = menu->addAction(display);
    act->setEnabled(on);
    // Always queue the connection to prevent the following sequence of events if the menu cleans
    // itself up on dismissal:
    // 1. The menu is dismissed when selecting a menu item.
    // 2. The deletion gets queued via deleteLater().
    // 2. The onTriggered action gets invoked and opens a dialog box.
    // 3. The dialog box triggers the events to be processed.
    // 4. The menu is deleted when processing the events, while still in the event function to
    //    handle the dismissal.
    QObject::connect(act, &QAction::triggered, parent, onTriggered, Qt::QueuedConnection);
    return act;
};

QAction *addAction(const QObject *parent, QMenu *menu, const QString &d1, const QString &d2, bool on,
                   const std::function<void()> &onTriggered)
{
    return on ? addAction(parent, menu, d1, true, onTriggered) : addAction(parent, menu, d2, false);
};

QAction *addCheckableAction(const QObject *parent, QMenu *menu, const QString &display, bool on,
                            bool checked, const std::function<void()> &onTriggered)
{
    QAction *act = addAction(parent, menu, display, on, onTriggered);
    act->setCheckable(true);
    act->setChecked(checked);
    return act;
}

void addStandardActions(QWidget *treeView, QMenu *menu)
{
    BaseTreeView *view = qobject_cast<BaseTreeView *>(treeView);
    QTC_ASSERT(treeView, return);
    QTC_ASSERT(menu, return);

    menu->addSeparator();

    addAction(view, menu, Tr::tr("Copy Selected Items to Clipboard"), true, [view] {
        setClipboardAndSelection(view->selectionAsText());
    });

    addAction(view, menu, Tr::tr("Copy Selected Items to New Editor"), true, [view] {
        openTextEditor("View", view->selectionAsText());
    });

    menu->addSeparator();

    menu->addAction(settings().settingsDialog.action());
}

///////////////////////////////////////////////////////////////////////
//
// DebugMode
//
///////////////////////////////////////////////////////////////////////

class DebugModeWidget final : public MiniSplitter
{
public:
    DebugModeWidget()
    {
        DebuggerMainWindow *mainWindow = DebuggerMainWindow::instance();

        auto editorHolderLayout = new QVBoxLayout;
        editorHolderLayout->setContentsMargins(0, 0, 0, 0);
        editorHolderLayout->setSpacing(0);

        auto editorAndFindWidget = new QWidget;
        editorAndFindWidget->setLayout(editorHolderLayout);
        editorHolderLayout->addWidget(DebuggerMainWindow::centralWidgetStack());
        editorHolderLayout->addWidget(new FindToolBarPlaceHolder(editorAndFindWidget));

        auto documentAndRightPane = new MiniSplitter;
        documentAndRightPane->addWidget(editorAndFindWidget);
        documentAndRightPane->addWidget(new RightPanePlaceHolder(MODE_DEBUG));
        documentAndRightPane->setStretchFactor(0, 1);
        documentAndRightPane->setStretchFactor(1, 0);

        auto centralEditorWidget = mainWindow->centralWidget();
        auto centralLayout = new QVBoxLayout(centralEditorWidget);
        centralEditorWidget->setLayout(centralLayout);
        centralLayout->setContentsMargins(0, 0, 0, 0);
        centralLayout->setSpacing(0);
        centralLayout->addWidget(documentAndRightPane);
        centralLayout->setStretch(0, 1);
        centralLayout->setStretch(1, 0);

        // Right-side window with editor, output etc.
        auto mainWindowSplitter = new MiniSplitter;
        mainWindowSplitter->addWidget(mainWindow);
        mainWindowSplitter->addWidget(new OutputPanePlaceHolder(MODE_DEBUG, mainWindowSplitter));
        auto outputPane = new OutputPanePlaceHolder(MODE_DEBUG, mainWindowSplitter);
        outputPane->setObjectName("DebuggerOutputPanePlaceHolder");
        mainWindowSplitter->addWidget(outputPane);
        mainWindowSplitter->setStretchFactor(0, 10);
        mainWindowSplitter->setStretchFactor(1, 0);
        mainWindowSplitter->setOrientation(Qt::Vertical);

        // Navigation and right-side window.
        setFocusProxy(DebuggerMainWindow::centralWidgetStack());
        addWidget(new NavigationWidgetPlaceHolder(MODE_DEBUG, Side::Left));
        addWidget(mainWindowSplitter);
        setStretchFactor(0, 0);
        setStretchFactor(1, 1);
        setObjectName("DebugModeWidget");

        mainWindow->addSubPerspectiveSwitcher(EngineManager::engineChooser());
        mainWindow->addSubPerspectiveSwitcher(EngineManager::dapEngineChooser());

        IContext::attach(this, Context(CC::C_EDITORMANAGER));
    }
};

class DebugMode final : public IMode
{
public:
    DebugMode()
    {
        setObjectName("DebugMode");
        setContext(Context(C_DEBUGMODE, CC::C_NAVIGATION_PANE));
        setDisplayName(Tr::tr("Debug"));
        setIcon(Icon::sideBarIcon(Icons::MODE_DEBUGGER_CLASSIC, Icons::MODE_DEBUGGER_FLAT));
        setPriority(85);
        setId(MODE_DEBUG);

        setWidgetCreator([] { return new DebugModeWidget; });
        setMainWindow(DebuggerMainWindow::instance());

        setMenu(&DebuggerMainWindow::addPerspectiveMenu);
    }
};

///////////////////////////////////////////////////////////////////////
//
// Misc
//
///////////////////////////////////////////////////////////////////////

static Kit::Predicate cdbPredicate(char wordWidth = 0)
{
    return [wordWidth](const Kit *k) -> bool {
        if (DebuggerKitAspect::engineType(k) != CdbEngineType
            || DebuggerKitAspect::configurationErrors(k)) {
            return false;
        }
        if (wordWidth)
            return ToolchainKitAspect::targetAbi(k).wordWidth() == wordWidth;
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
// DebuggerPluginPrivate
//
///////////////////////////////////////////////////////////////////////

class DebuggerPlugin;
static DebuggerPlugin *m_instance = nullptr;
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
    explicit DebuggerPluginPrivate(const QStringList &arguments);
    ~DebuggerPluginPrivate() override;

    void extensionsInitialized();

    RunControl *attachToRunningProcess(Kit *kit, const ProcessInfo &process, bool contAfterAttach);

    void breakpointSetMarginActionTriggered(bool isMessageOnly, const ContextData &data)
    {
        QString message;
        if (isMessageOnly) {
            if (data.type == LocationByAddress) {
                //: Message tracepoint: Address hit.
                message = Tr::tr("0x%1 hit").arg(data.address, 0, 16);
            } else {
                //: Message tracepoint: %1 file, %2 line %3 function hit.
                message = Tr::tr("%1:%2 %3() hit").arg(data.fileName.fileName()).
                        arg(data.textPosition.line).
                        arg(cppFunctionAt(data.fileName, data.textPosition.line));
            }
            QInputDialog dialog; // Create wide input dialog.
            dialog.setWindowFlags(dialog.windowFlags() & ~(Qt::MSWindowsFixedSizeDialogHint));
            dialog.resize(600, dialog.height());
            dialog.setWindowTitle(Tr::tr("Add Message Tracepoint"));
            dialog.setLabelText(Tr::tr("Message:"));
            dialog.setTextValue(message);
            if (dialog.exec() != QDialog::Accepted || dialog.textValue().isEmpty())
                return;
            message = dialog.textValue();
        }
        BreakpointManager::setOrRemoveBreakpoint(data, message);
    }

    void addFontSizeAdaptation(QWidget *widget);
    BaseTreeView *createBreakpointManagerView(const QByteArray &settingsKey);
    QWidget *createBreakpointManagerWindow(BaseTreeView *breakpointManagerView,
                                           const QString &title,
                                           const QString &objectName);

    BaseTreeView *createEngineManagerView(QAbstractItemModel *model,
                                          const QString &title,
                                          const QByteArray &settingsKey);
    QWidget *createEngineManagerWindow(BaseTreeView *engineManagerView,
                                       const QString &title,
                                       const QString &objectName);

    void createDapDebuggerPerspective(QWidget *globalLogWindow);

    void editorOpened(IEditor *editor);
    void updateBreakMenuItem(IEditor *editor);
    void requestMark(TextEditorWidget *widget, int lineNumber,
                     TextMarkRequestKind kind);
    void requestContextMenu(TextEditorWidget *widget,
                            int lineNumber, QMenu *menu);

    void setOrRemoveBreakpoint();
    void enableOrDisableBreakpoint();

    void attachToRunningApplication();
    void attachToUnstartedApplicationDialog();
    void attachToQmlPort();
    void runScheduled();
    void attachCore();
    void attachToLastCore();
    void reloadDebuggingHelpers();

    void remoteCommand(const QStringList &options);

    void dumpLog();
    void setInitialState();

    void onStartupProjectChanged();

    bool parseArgument(QStringList::const_iterator &it,
        const QStringList::const_iterator &cend, QString *errorMessage);
    bool parseArguments(const QStringList &args, QString *errorMessage);
    void parseCommandLineArguments();

    void updatePresetState();
    QWidget *addSearch(BaseTreeView *treeView);

public:
    QPointer<DebugMode> m_mode;

    ActionContainer *m_menu = nullptr;

    QList<RunControl *> m_scheduledStarts;

    ProxyAction m_visibleStartAction; // The fat debug button
    ProxyAction m_hiddenStopAction;
    QAction m_undisturbableAction;
    OptionalAction m_startAction;
    OptionalAction m_startDapAction;
    QAction m_debugWithoutDeployAction{Tr::tr("Start Debugging Without Deployment")};
    QAction m_startAndDebugApplicationAction{Tr::tr("Start and Debug External Application...")};
    QAction m_attachToRunningApplication{Tr::tr("Attach to Running Application...")};
    QAction m_attachToUnstartedApplication{Tr::tr("Attach to Unstarted Application...")};
    QAction m_attachToQmlPortAction{Tr::tr("Attach to QML Port...")};
    QAction m_attachToRemoteServerAction{Tr::tr("Attach to Running Debug Server...")};
    QAction m_startRemoteCdbAction{Tr::tr("Attach to Remote CDB Session...")};
    QAction m_attachToCoreAction{Tr::tr("Load Core File...")};
    QAction m_attachToLastCoreAction{Tr::tr("Load Last Core File")};

    // In the Debug menu.
    QAction m_startAndBreakOnMain{Tr::tr("Start and Break on Main")};
    QAction m_watchAction{Tr::tr("Add Expression Evaluator")};
    Command *m_watchCommand = nullptr;
    QAction m_setOrRemoveBreakpointAction{Tr::tr("Set or Remove Breakpoint")};
    QAction m_enableOrDisableBreakpointAction{Tr::tr("Enable or Disable Breakpoint")};
    QAction m_reloadDebuggingHelpersAction{Tr::tr("Reload Debugging Helpers")};

    BreakpointManager m_breakpointManager;
    QString m_lastPermanentStatusMessage;

    EngineManager m_engineManager;
    QTimer m_shutdownTimer;

    Console m_console; // ensure Debugger Console is created before settings are taken into account
    QStringList m_arguments;

    QList<IOptionsPage *> m_optionPages;

    Perspective m_perspective{Constants::PRESET_PERSPECTIVE_ID, Tr::tr("Debugger")};
    Perspective m_perspectiveDap{Constants::DAP_PERSPECTIVE_ID, Tr::tr("DAP")};

    std::optional<QPoint> attachToUnstartedApplicationDialogLastPosition;

    // FIXME: Needed?
//            QString mainScript = runConfig->property("mainScript").toString();
//            const bool isDebuggableScript = mainScript.endsWith(".py"); // Only Python for now.
//            return isDebuggableScript;
};

void DebuggerPluginPrivate::addFontSizeAdaptation(QWidget *widget)
{
    QObject::connect(TextEditorSettings::instance(),
                     &TextEditorSettings::fontSettingsChanged,
                     this,
                     [widget](const FontSettings &fs) {
                         if (!settings().fontSizeFollowsEditor())
                             return;
                         qreal size = fs.fontZoom() * fs.fontSize() / 100.;
                         QFont font = widget->font();
                         font.setPointSizeF(size);
                         widget->setFont(font);
                     });
};

BaseTreeView *DebuggerPluginPrivate::createBreakpointManagerView(const QByteArray &settingsKey)
{
    auto breakpointManagerView = new BaseTreeView;
    breakpointManagerView->setActivationMode(Utils::DoubleClickActivation);
    breakpointManagerView->setIconSize(QSize(10, 10));
    breakpointManagerView->setWindowIcon(Icons::BREAKPOINTS.icon());
    breakpointManagerView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    breakpointManagerView->setSettings(ICore::settings(), settingsKey);
    breakpointManagerView->setRootIsDecorated(true);
    breakpointManagerView->setModel(BreakpointManager::model());
    breakpointManagerView->setSpanColumn(BreakpointFunctionColumn);
    breakpointManagerView->enableColumnHiding();
    return breakpointManagerView;
}

QWidget *DebuggerPluginPrivate::createBreakpointManagerWindow(BaseTreeView *breakpointManagerView,
                                                              const QString &title,
                                                              const QString &objectName)
{
    auto breakpointManagerWindow = addSearch(breakpointManagerView);
    breakpointManagerWindow->setWindowTitle(title);
    breakpointManagerWindow->setObjectName(objectName);
    addFontSizeAdaptation(breakpointManagerWindow);
    return breakpointManagerWindow;
}

BaseTreeView *DebuggerPluginPrivate::createEngineManagerView(QAbstractItemModel *model,
                                                             const QString &title,
                                                             const QByteArray &settingsKey)
{
    auto engineManagerView = new BaseTreeView;
    engineManagerView->setWindowTitle(title);
    engineManagerView->setSettings(ICore::settings(), settingsKey);
    engineManagerView->setIconSize(QSize(10, 10));
    engineManagerView->setModel(model);
    engineManagerView->setSelectionMode(QAbstractItemView::SingleSelection);
    engineManagerView->enableColumnHiding();
    return engineManagerView;
}

QWidget *DebuggerPluginPrivate::createEngineManagerWindow(BaseTreeView *engineManagerView,
                                                          const QString &title,
                                                          const QString &objectName)
{
    auto engineManagerWindow = addSearch(engineManagerView);
    engineManagerWindow->setWindowTitle(title);
    engineManagerWindow->setObjectName(objectName);
    addFontSizeAdaptation(engineManagerWindow);
    return engineManagerWindow;
}

DebuggerPluginPrivate::DebuggerPluginPrivate(const QStringList &arguments)
{
    setupDebuggerRunWorker();

    qRegisterMetaType<ContextData>("ContextData");
    qRegisterMetaType<DebuggerRunParameters>("DebuggerRunParameters");
    qRegisterMetaType<QString *>();

    // Menu groups
    ActionContainer *mstart = ActionManager::actionContainer(PE::M_DEBUG_STARTDEBUGGING);
    mstart->appendGroup(MENU_GROUP_GENERAL);
    mstart->appendGroup(MENU_GROUP_SPECIAL);
    mstart->appendGroup(MENU_GROUP_START_QML);

    // Separators
    mstart->addSeparator(MENU_GROUP_GENERAL);
    mstart->addSeparator(MENU_GROUP_SPECIAL);

    // Task integration.
    //: Category under which Analyzer tasks are listed in Issues view
    TaskHub::addCategory({ANALYZERTASK_ID,
                          Tr::tr("Valgrind"),
                          Tr::tr("Issues that the Valgrind tools found when analyzing the code.")});

    const Context debuggerNotRunning(C_DEBUGGER_NOTRUNNING);
    ICore::addAdditionalContext(debuggerNotRunning);

    m_arguments = arguments;
    if (!m_arguments.isEmpty()) {
        connect(SessionManager::instance(), &SessionManager::startupSessionRestored,
                this, &DebuggerPluginPrivate::parseCommandLineArguments);
    }

    // Menus
    m_menu = ActionManager::createMenu(M_DEBUG_ANALYZER);
    m_menu->menu()->setTitle(Tr::tr("&Analyze"));
    m_menu->menu()->setEnabled(true);

    m_menu->appendGroup(G_ANALYZER_CONTROL);
    m_menu->appendGroup(G_ANALYZER_TOOLS);
    m_menu->appendGroup(G_ANALYZER_REMOTE_TOOLS);
    m_menu->appendGroup(G_ANALYZER_OPTIONS);

    ActionContainer *touchBar = ActionManager::createTouchBar("Debugger.TouchBar",
                                                              Icons::MACOS_TOUCHBAR_DEBUG.icon());
    ActionManager::actionContainer(Core::Constants::TOUCH_BAR)
        ->addMenu(touchBar, Core::Constants::G_TOUCHBAR_OTHER);

    ActionContainer *menubar = ActionManager::actionContainer(MENU_BAR);
    ActionContainer *mtools = ActionManager::actionContainer(M_TOOLS);
    if (!hideAnalyzeMenu())
        menubar->addMenu(mtools, m_menu);

    m_menu->addSeparator(G_ANALYZER_TOOLS);
    m_menu->addSeparator(G_ANALYZER_REMOTE_TOOLS);
    m_menu->addSeparator(G_ANALYZER_OPTIONS);

    QAction *act;

    // Populate Windows->Views menu with standard actions.
    act = new QAction(Tr::tr("Memory..."), this);
    act->setVisible(false);
    act->setEnabled(false);
    ActionManager::registerAction(act, Constants::OPEN_MEMORY_EDITOR);

    TaskHub::addCategory({TASK_CATEGORY_DEBUGGER_RUNTIME,
                          Tr::tr("Debugger Runtime"),
                          Tr::tr("Issues with starting the debugger.")});

    auto breakpointManagerView = createBreakpointManagerView("Debugger.BreakWindow");
    auto breakpointManagerWindow
        = createBreakpointManagerWindow(breakpointManagerView,
                                        Tr::tr("Breakpoint Preset"),
                                        "Debugger.Docks.BreakpointManager");

    // Snapshot
    auto engineManagerView = createEngineManagerView(EngineManager::model(),
                                                     Tr::tr("Running Debuggers"),
                                                     "Debugger.SnapshotView");
    auto engineManagerWindow = createEngineManagerWindow(engineManagerView,
                                                         Tr::tr("Debugger Perspectives"),
                                                         "Debugger.Docks.Snapshots");

    // Logging
    auto globalLogWindow = new GlobalLogWindow;
    addFontSizeAdaptation(globalLogWindow);

    ActionContainer *debugMenu = ActionManager::actionContainer(PE::M_DEBUG);

    RunConfiguration::registerAspect<DebuggerRunConfigurationAspect>();

    // The main "Start Debugging" action. Acts as "Continue" at times.
    connect(&m_startAction, &QAction::triggered, this, [] {
        ProjectExplorerPlugin::runStartupProject(ProjectExplorer::Constants::DEBUG_RUN_MODE, false);
    });

    connect(&m_debugWithoutDeployAction, &QAction::triggered, this, [] {
        ProjectExplorerPlugin::runStartupProject(ProjectExplorer::Constants::DEBUG_RUN_MODE, true);
    });

    connect(&m_startAndDebugApplicationAction, &QAction::triggered,
            this, [] { runStartAndDebugApplicationDialog(); });

    connect(&m_attachToCoreAction, &QAction::triggered,
            this, [] { runAttachToCoreDialog(); });

    connect(&m_attachToLastCoreAction, &QAction::triggered,
            this, &DebuggerPluginPrivate::attachToLastCore);

    connect(&m_attachToRemoteServerAction, &QAction::triggered,
            this, [] { runAttachToRemoteServerDialog(); });

    connect(&m_attachToRunningApplication, &QAction::triggered,
            this, &DebuggerPluginPrivate::attachToRunningApplication);

    connect(&m_attachToUnstartedApplication, &QAction::triggered,
            this, &DebuggerPluginPrivate::attachToUnstartedApplicationDialog);

    connect(&m_attachToQmlPortAction, &QAction::triggered,
            this, [] { runAttachToQmlPortDialog(); });

    connect(&m_startRemoteCdbAction, &QAction::triggered,
            this, [] { runStartRemoteCdbSessionDialog(findUniversalCdbKit()); });

    // "Start Debugging" sub-menu
    // groups:
    //   G_DEFAULT_ONE
    //   MENU_GROUP_START_LOCAL
    //   MENU_GROUP_START_REMOTE
    //   MENU_GROUP_START_QML

    const QKeySequence startShortcut(useMacShortcuts ? Tr::tr("Ctrl+Y") : Tr::tr("F5"));

    Command *cmd = ActionManager::registerAction(&m_visibleStartAction, "Debugger.Debug");

    cmd->setDescription(Tr::tr("Start Debugging or Continue"));
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setAttribute(Command::CA_UpdateIcon);
    //mstart->addAction(cmd, CC::G_DEFAULT_ONE);

    cmd = ActionManager::registerAction(&m_startAction, DEBUGGER_START);
    cmd->setDescription(Tr::tr("Start Debugging"));
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDefaultKeySequence(startShortcut);
    mstart->addAction(cmd, CC::G_DEFAULT_ONE);

    m_visibleStartAction.initialize(&m_startAction);
    m_visibleStartAction.setAttribute(ProxyAction::UpdateText);
    m_visibleStartAction.setAttribute(ProxyAction::UpdateIcon);
    m_visibleStartAction.setAction(&m_startAction);

    m_visibleStartAction.setObjectName("Debug"); // used for UI introduction

    if (!hideDebugMenu())
        ModeManager::addAction(&m_visibleStartAction, /*priority*/ 90);

    m_undisturbableAction.setIcon(interruptIcon(false));
    m_undisturbableAction.setEnabled(false);

    cmd = ActionManager::registerAction(&m_debugWithoutDeployAction,
        "Debugger.DebugWithoutDeploy");
    cmd->setAttribute(Command::CA_Hide);
    mstart->addAction(cmd, CC::G_DEFAULT_ONE);

    cmd = ActionManager::registerAction(&m_attachToRunningApplication,
         "Debugger.AttachToRemoteProcess");
    cmd->setDescription(Tr::tr("Attach to Running Application"));
    mstart->addAction(cmd, MENU_GROUP_GENERAL);

    cmd = ActionManager::registerAction(&m_attachToUnstartedApplication,
          "Debugger.AttachToUnstartedProcess");
    cmd->setDescription(Tr::tr("Attach to Unstarted Application"));
    mstart->addAction(cmd, MENU_GROUP_GENERAL);

    cmd = ActionManager::registerAction(&m_startAndDebugApplicationAction,
        "Debugger.StartAndDebugApplication");
    cmd->setAttribute(Command::CA_Hide);
    mstart->addAction(cmd, MENU_GROUP_GENERAL);

    cmd = ActionManager::registerAction(&m_attachToCoreAction,
         "Debugger.AttachCore");
    cmd->setAttribute(Command::CA_Hide);
    mstart->addAction(cmd, MENU_GROUP_GENERAL);

    cmd = ActionManager::registerAction(&m_attachToLastCoreAction,
                                        "Debugger.AttachLastCore");
    cmd->setAttribute(Command::CA_Hide);
    mstart->addAction(cmd, MENU_GROUP_GENERAL);

    cmd = ActionManager::registerAction(&m_attachToRemoteServerAction,
          "Debugger.AttachToRemoteServer");
    cmd->setAttribute(Command::CA_Hide);
    mstart->addAction(cmd, MENU_GROUP_SPECIAL);

    if (HostOsInfo::isWindowsHost()) {
        cmd = ActionManager::registerAction(&m_startRemoteCdbAction,
             "Debugger.AttachRemoteCdb");
        cmd->setAttribute(Command::CA_Hide);
        mstart->addAction(cmd, MENU_GROUP_SPECIAL);
    }

    mstart->addSeparator(Context(CC::C_GLOBAL), MENU_GROUP_START_QML);

    cmd = ActionManager::registerAction(&m_attachToQmlPortAction, "Debugger.AttachToQmlPort");
    cmd->setAttribute(Command::CA_Hide);
    mstart->addAction(cmd, MENU_GROUP_START_QML);

    act = new QAction(Tr::tr("Detach Debugger"), this);
    act->setEnabled(false);
    cmd = ActionManager::registerAction(act, Constants::DETACH);
    debugMenu->addAction(cmd, CC::G_DEFAULT_ONE);

    act = new QAction(interruptIcon(false), Tr::tr("Interrupt"), this);
    act->setEnabled(false);
    cmd = ActionManager::registerAction(act, Constants::INTERRUPT);
    cmd->setDescription(Tr::tr("Interrupt Debugger"));
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDefaultKeySequence(startShortcut);
    cmd->setTouchBarIcon(Icons::MACOS_TOUCHBAR_DEBUG_INTERRUPT.icon());
    touchBar->addAction(cmd);
    debugMenu->addAction(cmd, CC::G_DEFAULT_ONE);

    act = new QAction(continueIcon(false), Tr::tr("Continue"), this);
    act->setEnabled(false);
    cmd = ActionManager::registerAction(act, Constants::CONTINUE);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDefaultKeySequence(startShortcut);
    cmd->setTouchBarIcon(Icons::MACOS_TOUCHBAR_DEBUG_CONTINUE.icon());
    touchBar->addAction(cmd);
    debugMenu->addAction(cmd, CC::G_DEFAULT_ONE);

    const QIcon sidebarStopIcon = Icon::sideBarIcon(Icons::STOP, Icons::STOP_FLAT);
    const QIcon stopIcon = Icon::combinedIcon({Icons::DEBUG_EXIT_SMALL.icon(), sidebarStopIcon});
    act = new QAction(stopIcon, Tr::tr("Stop Debugger"), this);
    act->setEnabled(false);
    cmd = ActionManager::registerAction(act, Constants::STOP);
    cmd->setTouchBarIcon(Icons::MACOS_TOUCHBAR_DEBUG_EXIT.icon());
    touchBar->addAction(cmd);
    debugMenu->addAction(cmd, CC::G_DEFAULT_ONE);
    m_hiddenStopAction.initialize(cmd->action());
    m_hiddenStopAction.setAttribute(ProxyAction::UpdateText);
    m_hiddenStopAction.setAttribute(ProxyAction::UpdateIcon);

    cmd = ActionManager::registerAction(&m_hiddenStopAction, "Debugger.HiddenStop");
    cmd->setDefaultKeySequence(QKeySequence(useMacShortcuts ? Tr::tr("Shift+Ctrl+Y") : Tr::tr("Shift+F5")));

    act = new QAction(Tr::tr("Abort Debugging"), this);
    act->setEnabled(false);
    cmd = ActionManager::registerAction(act, Constants::ABORT);
    cmd->setDescription(Tr::tr("Reset Debugger"));
    debugMenu->addAction(cmd, CC::G_DEFAULT_ONE);

    act = new QAction(Icons::RESTART_TOOLBAR.icon(), Tr::tr("Restart Debugging"), this);
    act->setEnabled(false);
    act->setToolTip(Tr::tr("Restart the debugging session."));
    cmd = ActionManager::registerAction(act, Constants::RESET);
    cmd->setDescription(Tr::tr("Restart Debugging"));
    debugMenu->addAction(cmd, CC::G_DEFAULT_ONE);

    debugMenu->addSeparator();

    cmd = ActionManager::registerAction(&m_startAndBreakOnMain,
                                        "Debugger.StartAndBreakOnMain",
                                        debuggerNotRunning);
    cmd->setDefaultKeySequence(QKeySequence(useMacShortcuts ? Tr::tr("Ctrl+Shift+O") : Tr::tr("F10")));
    cmd->setAttribute(Command::CA_Hide);
    debugMenu->addAction(cmd);
    connect(&m_startAndBreakOnMain, &QAction::triggered, this, [] {
        DebuggerRunParameters::setBreakOnMainNextTime();
        ProjectExplorerPlugin::runStartupProject(ProjectExplorer::Constants::DEBUG_RUN_MODE, false);
    });

    act = new QAction(Icons::STEP_OVER.icon(), Tr::tr("Step Over"), this);
    act->setEnabled(false);
    cmd = ActionManager::registerAction(act, Constants::NEXT);
    cmd->setDefaultKeySequence(QKeySequence(useMacShortcuts ? Tr::tr("Ctrl+Shift+O") : Tr::tr("F10")));
    cmd->setTouchBarIcon(Icons::MACOS_TOUCHBAR_DEBUG_STEP_OVER.icon());
    touchBar->addAction(cmd);
    debugMenu->addAction(cmd);

    act = new QAction(Icons::STEP_INTO.icon(), Tr::tr("Step Into"), this);
    act->setEnabled(false);
    cmd = ActionManager::registerAction(act, Constants::STEP);
    cmd->setDefaultKeySequence(QKeySequence(useMacShortcuts ? Tr::tr("Ctrl+Shift+I") : Tr::tr("F11")));
    cmd->setTouchBarIcon(Icons::MACOS_TOUCHBAR_DEBUG_STEP_INTO.icon());
    touchBar->addAction(cmd);
    debugMenu->addAction(cmd);

    act = new QAction(Icons::STEP_OUT.icon(), Tr::tr("Step Out"), this);
    act->setEnabled(false);
    cmd = ActionManager::registerAction(act, Constants::STEPOUT);
    cmd->setDefaultKeySequence(QKeySequence(useMacShortcuts ? Tr::tr("Ctrl+Shift+T") : Tr::tr("Shift+F11")));
    cmd->setTouchBarIcon(Icons::MACOS_TOUCHBAR_DEBUG_STEP_OUT.icon());
    touchBar->addAction(cmd);
    debugMenu->addAction(cmd);

    act = new QAction(Tr::tr("Run to Line"), this);
    act->setEnabled(false);
    act->setVisible(false);
    cmd = ActionManager::registerAction(act, Constants::RUNTOLINE);
    cmd->setDefaultKeySequence(QKeySequence(useMacShortcuts ? Tr::tr("Shift+F8") : Tr::tr("Ctrl+F10")));
    debugMenu->addAction(cmd);

    act = new QAction(Tr::tr("Run to Selected Function"), this);
    act->setEnabled(false);
    act->setEnabled(false);
    cmd = ActionManager::registerAction(act, Constants::RUNTOSELECTEDFUNCTION);
    cmd->setDefaultKeySequence(QKeySequence(Tr::tr("Ctrl+F6")));
    // Don't add to menu by default as keeping its enabled state
    // and text up-to-date is a lot of hassle.
    // debugMenu->addAction(cmd);

    act = new QAction(Tr::tr("Jump to Line"), this);
    act->setEnabled(false);
    act->setVisible(false);
    cmd = ActionManager::registerAction(act, Constants::JUMPTOLINE);
    debugMenu->addAction(cmd);

    act = new QAction(Tr::tr("Immediately Return From Inner Function"), this);
    act->setEnabled(false);
    act->setVisible(false);
    cmd = ActionManager::registerAction(act, Constants::RETURNFROMFUNCTION);
    debugMenu->addAction(cmd);

    debugMenu->addSeparator();

    act = new QAction(this);
    act->setText(Tr::tr("Move to Calling Frame"));
    act->setEnabled(false);
    act->setVisible(false);
    ActionManager::registerAction(act, Constants::FRAME_UP);

    act = new QAction(this);
    act->setText(Tr::tr("Move to Called Frame"));
    act->setEnabled(false);
    act->setVisible(false);
    ActionManager::registerAction(act, Constants::FRAME_DOWN);

    act = new QAction(this);
    act->setText(Tr::tr("Operate by Instruction"));
    act->setEnabled(false);
    act->setVisible(false);
    act->setCheckable(true);
    act->setChecked(false);
    cmd = ActionManager::registerAction(act, Constants::OPERATE_BY_INSTRUCTION);
    debugMenu->addAction(cmd);

    cmd = ActionManager::registerAction(&m_setOrRemoveBreakpointAction, "Debugger.ToggleBreak");
    cmd->setDefaultKeySequence(QKeySequence(useMacShortcuts ? Tr::tr("F8") : Tr::tr("F9")));
    debugMenu->addAction(cmd);
    connect(&m_setOrRemoveBreakpointAction, &QAction::triggered,
            this, &DebuggerPluginPrivate::setOrRemoveBreakpoint);

    cmd = ActionManager::registerAction(&m_enableOrDisableBreakpointAction,
                                        "Debugger.EnableOrDisableBreakpoint");
    cmd->setDefaultKeySequence(QKeySequence(useMacShortcuts ? Tr::tr("Ctrl+F8") : Tr::tr("Ctrl+F9")));
    debugMenu->addAction(cmd);
    connect(&m_enableOrDisableBreakpointAction, &QAction::triggered,
            this, &DebuggerPluginPrivate::enableOrDisableBreakpoint);

    debugMenu->addSeparator();

    auto qmlShowAppOnTopDummyAction = new QAction(Tr::tr("Show Application on Top"), this);
    qmlShowAppOnTopDummyAction->setCheckable(true);
    qmlShowAppOnTopDummyAction->setIcon(Icons::APP_ON_TOP.icon());
    qmlShowAppOnTopDummyAction->setEnabled(false);
    cmd = ActionManager::registerAction(qmlShowAppOnTopDummyAction, Constants::QML_SHOW_APP_ON_TOP);
    debugMenu->addAction(cmd);

    auto qmlSelectDummyAction = new QAction(Tr::tr("Select"), this);
    qmlSelectDummyAction->setCheckable(true);
    qmlSelectDummyAction->setIcon(Icons::SELECT.icon());
    qmlSelectDummyAction->setEnabled(false);
    cmd = ActionManager::registerAction(qmlSelectDummyAction, Constants::QML_SELECTTOOL);
    debugMenu->addAction(cmd);

    debugMenu->addSeparator();

    ActionManager::registerAction(&m_reloadDebuggingHelpersAction, Constants::RELOAD_DEBUGGING_HELPERS);
    connect(&m_reloadDebuggingHelpersAction, &QAction::triggered,
            this, &DebuggerPluginPrivate::reloadDebuggingHelpers);

    cmd = m_watchCommand = ActionManager::registerAction(&m_watchAction, Constants::WATCH);
    debugMenu->addAction(cmd);

 // FIXME: Re-vive watcher creation before engine runs.
//    connect(&m_watchAction, &QAction::triggered, this, [&] {
//        QTC_CHECK(false);
//    });

    addCdbOptionPages(&m_optionPages);

    connect(
        ModeManager::instance(),
        &ModeManager::currentModeAboutToChange,
        DebuggerMainWindow::instance(),
        [] {
            if (ModeManager::currentModeId() == MODE_DEBUG)
                DebuggerMainWindow::leaveDebugMode();
        });

    connect(
        ModeManager::instance(),
        &ModeManager::currentModeChanged,
        DebuggerMainWindow::instance(),
        [](Id mode, Id oldMode) {
            QTC_ASSERT(mode != oldMode, return);
            if (mode == MODE_DEBUG) {
                DebuggerMainWindow::enterDebugMode();
                if (IEditor *editor = EditorManager::currentEditor())
                    editor->widget()->setFocus();
            }
        });

    globalProjectExplorerSettings().deployBeforeRun.addOnChanged(this, [this] {
        m_debugWithoutDeployAction.setVisible(globalProjectExplorerSettings().deployBeforeRun());
    });

    // Debug mode setup
    m_mode = new DebugMode;

    //
    //  Connections
    //

    // ProjectExplorer
    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::runActionsUpdated,
            this, &DebuggerPluginPrivate::updatePresetState);

    // EditorManager
    connect(EditorManager::instance(), &EditorManager::editorOpened,
            this, &DebuggerPluginPrivate::editorOpened);
    connect(EditorManager::instance(), &EditorManager::currentEditorChanged,
            this, &DebuggerPluginPrivate::updateBreakMenuItem);

    // Application interaction
    // Use a queued connection so the dialog isn't triggered in the same event.
    connect(settings().settingsDialog.action(), &QAction::triggered, this,
            [] { ICore::showOptionsDialog(DEBUGGER_COMMON_SETTINGS_ID); }, Qt::QueuedConnection);

    EngineManager::registerDefaultPerspective(Tr::tr("Debugger Preset"),
                                              {},
                                              Constants::PRESET_PERSPECTIVE_ID);

    m_perspective.useSubPerspectiveSwitcher(EngineManager::engineChooser());
    m_perspective.addToolBarAction(&m_startAction);

    m_perspective.addWindow(engineManagerWindow, Perspective::SplitVertical, nullptr);
    m_perspective.addWindow(breakpointManagerWindow, Perspective::SplitHorizontal, engineManagerWindow);
    m_perspective.addWindow(globalLogWindow, Perspective::AddToTab, nullptr, false, Qt::TopDockWidgetArea);

    createDapDebuggerPerspective(globalLogWindow);
    setInitialState();

    connect(ProjectManager::instance(), &ProjectManager::startupProjectChanged,
        this, &DebuggerPluginPrivate::onStartupProjectChanged);
    connect(EngineManager::instance(), &EngineManager::engineStateChanged,
        this, &DebuggerPluginPrivate::updatePresetState);
    connect(EngineManager::instance(), &EngineManager::currentEngineChanged,
        this, &DebuggerPluginPrivate::updatePresetState);
}

void DebuggerPluginPrivate::createDapDebuggerPerspective(QWidget *globalLogWindow)
{
    struct DapPerspective
    {
        QString name;
        Id runMode;
        bool forceSkipDeploy = false;
    };

    const QList<DapPerspective> perspectiveList = {
        DapPerspective{
            Tr::tr("CMake Preset"),
            ProjectExplorer::Constants::DAP_CMAKE_DEBUG_RUN_MODE,
            /*forceSkipDeploy=*/true},
        DapPerspective{Tr::tr("Python Preset"), ProjectExplorer::Constants::DAP_PY_DEBUG_RUN_MODE},
    };

    for (const DapPerspective &dp : perspectiveList)
        EngineManager::registerDefaultPerspective(dp.name, "DAP", Constants::DAP_PERSPECTIVE_ID);

    connect(&m_startDapAction, &QAction::triggered, this, [perspectiveList] {
        QComboBox *combo = qobject_cast<QComboBox *>(EngineManager::dapEngineChooser());
        if (perspectiveList.size() > combo->currentIndex()) {
            const DapPerspective dapPerspective = perspectiveList.at(combo->currentIndex());
            ProjectExplorerPlugin::runStartupProject(dapPerspective.runMode,
                                                     dapPerspective.forceSkipDeploy);
        }
    });

    auto breakpointManagerView = createBreakpointManagerView("DAPDebugger.BreakWindow");
    auto breakpointManagerWindow
        = createBreakpointManagerWindow(breakpointManagerView,
                                        Tr::tr("DAP Breakpoint Preset"),
                                        "DAPDebugger.Docks.BreakpointManager");

    // Snapshot
    auto engineManagerView = createEngineManagerView(EngineManager::dapModel(),
                                                     Tr::tr("Running Debuggers"),
                                                     "DAPDebugger.SnapshotView");
    auto engineManagerWindow = createEngineManagerWindow(engineManagerView,
                                                         Tr::tr("DAP Debugger Perspectives"),
                                                         "DAPDebugger.Docks.Snapshots");

    m_perspectiveDap.addToolBarAction(&m_startDapAction);
    m_startDapAction.setToolTip(Tr::tr("Start DAP Debugging"));
    m_startDapAction.setText(Tr::tr("Start DAP Debugging"));
    m_startDapAction.setEnabled(true);
    m_startDapAction.setIcon(startIcon(true));
    m_startDapAction.setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_startDapAction.setVisible(true);

    m_perspectiveDap.useSubPerspectiveSwitcher(EngineManager::dapEngineChooser());

    m_perspectiveDap.addWindow(engineManagerWindow, Perspective::SplitVertical, nullptr);
    m_perspectiveDap.addWindow(breakpointManagerWindow,
                               Perspective::SplitHorizontal,
                               engineManagerWindow);
    m_perspectiveDap.addWindow(globalLogWindow,
                               Perspective::AddToTab,
                               nullptr,
                               false,
                               Qt::TopDockWidgetArea);
}

DebuggerPluginPrivate::~DebuggerPluginPrivate()
{
    qDeleteAll(m_optionPages);
    m_optionPages.clear();
}

static QString msgParameterMissing(const QString &a)
{
    return Tr::tr("Option \"%1\" is missing the parameter.").arg(a);
}

static Kit *guessKitFromAbis(const Abis &abis)
{
    Kit *kit = nullptr;

    if (!KitManager::waitForLoaded())
        return kit;

    // Try to find a kit via ABI.
    if (!abis.isEmpty()) {
        // Try exact abis.
        kit = KitManager::kit([abis](const Kit *k) {
            const Abi tcAbi = ToolchainKitAspect::targetAbi(k);
            return abis.contains(tcAbi) && !DebuggerKitAspect::configurationErrors(k);
        });
        if (!kit) {
            // Or something compatible.
            kit = KitManager::kit([abis](const Kit *k) {
                const Abi tcAbi = ToolchainKitAspect::targetAbi(k);
                return !DebuggerKitAspect::configurationErrors(k)
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
        const qint64 pid = it->toLongLong();
        const QStringList args = it->split(',');

        Kit *kit = nullptr;
        DebuggerStartMode startMode = StartExternal;
        FilePath executable;
        QString remoteChannel;
        FilePath coreFile;
        QString sysRoot;
        bool useTerminal = false;

        if (!pid) {
            for (const QString &arg : args) {
                const QString key = arg.section('=', 0, 0);
                const QString val = arg.section('=', 1, 1);
                if (val.isEmpty()) {
                    if (key.isEmpty()) {
                        continue;
                    } else if (executable.isEmpty()) {
                        executable = FilePath::fromString(key);
                    } else {
                        *errorMessage = Tr::tr("Only one executable allowed.");
                        return false;
                    }
                } else if (key == "kit") {
                    if (KitManager::waitForLoaded()) {
                        kit = KitManager::kit(Id::fromString(val));
                        if (!kit)
                            kit = KitManager::kit(Utils::equal(&Kit::displayName, val));
                    }
                } else if (key == "server") {
                    startMode = AttachToRemoteServer;
                    remoteChannel = val;
                } else if (key == "core") {
                    startMode = AttachToCore;
                    coreFile = FilePath::fromUserInput(val);
                } else if (key == "terminal") {
                    useTerminal = true;
                } else if (key == "sysroot") {
                    sysRoot = val;
                }
            }
        }
        if (!kit)
            kit = guessKitFromAbis(Abi::abisOfBinary(executable));

        auto runControl = new RunControl(ProjectExplorer::Constants::DEBUG_RUN_MODE);
        runControl->setKit(kit);
        DebuggerRunParameters rp = DebuggerRunParameters::fromRunControl(runControl);
        rp.setInferiorExecutable(executable);
        if (!sysRoot.isEmpty())
            rp.setSysRoot(FilePath::fromUserInput(sysRoot));
        if (pid) {
            rp.setStartMode(AttachToLocalProcess);
            rp.setCloseMode(DetachAtClose);
            runControl->setAttachPid(ProcessHandle(pid));
            rp.setDisplayName(Tr::tr("Process %1").arg(pid));
            rp.setStartMessage(Tr::tr("Attaching to local process %1.").arg(pid));
        } else if (startMode == AttachToRemoteServer) {
            rp.setStartMode(AttachToRemoteServer);
            rp.setRemoteChannel(remoteChannel);
            rp.setDisplayName(Tr::tr("Remote: \"%1\"").arg(remoteChannel));
            rp.setStartMessage(Tr::tr("Attaching to remote server %1.").arg(remoteChannel));
        } else if (startMode == AttachToCore) {
            rp.setStartMode(AttachToCore);
            rp.setCloseMode(DetachAtClose);
            rp.setCoreFilePath(coreFile);
            rp.setDisplayName(Tr::tr("Core file \"%1\"").arg(coreFile.toUserOutput()));
            rp.setStartMessage(Tr::tr("Attaching to core file %1.").arg(coreFile.toUserOutput()));
        } else {
            rp.setStartMode(StartExternal);
            rp.setDisplayName(Tr::tr("Executable file \"%1\"").arg(executable.toUserOutput()));
            rp.setStartMessage(Tr::tr("Debugging file %1.").arg(executable.toUserOutput()));
        }
        rp.setUseTerminal(useTerminal);

        runControl->setRunRecipe(debuggerRecipe(runControl, rp));
        m_scheduledStarts.append(runControl);
        return true;
    }
    // -wincrashevent <event-handle>:<pid>. A handle used for
    // a handshake when attaching to a crashed Windows process.
    // This is created by $QTC/src/tools/qtcdebugger/main.cpp:
    // args << "-wincrashevent"
    //   << QString::fromLatin1("%1:%2").arg(argWinCrashEvent).arg(argProcessId);
    if (*it == "-wincrashevent") {
        ++it;
        if (it == cend) {
            *errorMessage = msgParameterMissing(*it);
            return false;
        }
        qint64 pid = it->section(':', 1, 1).toLongLong();
        auto runControl = new RunControl(ProjectExplorer::Constants::DEBUG_RUN_MODE);
        runControl->setKit(findUniversalCdbKit());
        runControl->setAttachPid(ProcessHandle(pid));
        DebuggerRunParameters rp = DebuggerRunParameters::fromRunControl(runControl);
        rp.setStartMode(AttachToCrashedProcess);
        rp.setCrashParameter(it->section(':', 0, 0));
        rp.setDisplayName(Tr::tr("Crashed process %1").arg(pid));
        rp.setStartMessage(Tr::tr("Attaching to crashed process %1").arg(pid));
        if (pid < 1) {
            *errorMessage = Tr::tr("The parameter \"%1\" of option \"%2\" "
                "does not match the pattern <handle>:<pid>.").arg(*it, option);
            return false;
        }

        runControl->setRunRecipe(debuggerRecipe(runControl, rp));
        m_scheduledStarts.append(runControl);
        return true;
    }

    *errorMessage = Tr::tr("Invalid debugger option: %1").arg(option);
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
        errorMessage = Tr::tr("Error evaluating command line arguments: %1")
            .arg(errorMessage);
        qWarning("%s\n", qPrintable(errorMessage));
        MessageManager::writeDisrupting(errorMessage);
    }
    if (!m_scheduledStarts.isEmpty())
        QTimer::singleShot(0, this, &DebuggerPluginPrivate::runScheduled);
}

void DebuggerPluginPrivate::updatePresetState()
{
    if (PluginManager::isShuttingDown())
        return;

    Project *startupProject = ProjectManager::startupProject();
    RunConfiguration *startupRunConfig = activeRunConfigForActiveProject();
    DebuggerEngine *currentEngine = EngineManager::currentEngine();

    const auto canRun = ProjectExplorerPlugin::canRunStartupProject(
        ProjectExplorer::Constants::DEBUG_RUN_MODE);

    QString startupRunConfigName;
    if (startupRunConfig)
        startupRunConfigName = startupRunConfig->displayName();
    if (startupRunConfigName.isEmpty() && startupProject)
        startupRunConfigName = startupProject->displayName();

    // Restrict width, otherwise Creator gets too wide, see QTCREATORBUG-21885
    const QString startToolTip = canRun ? Tr::tr("Start debugging of startup project")
                                        : canRun.error();

    m_startAction.setToolTip(startToolTip);
    m_startAction.setText(Tr::tr("Start Debugging of Startup Project"));

    if (!currentEngine) {
        // No engine running  -- or -- we have a running engine but it does not
        // correspond to the current start up project.
        m_startAction.setEnabled(canRun.has_value());
        m_startAction.setIcon(startIcon(true));
        m_startAction.setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        m_startAction.setVisible(true);
        m_debugWithoutDeployAction.setEnabled(canRun.has_value());
        m_visibleStartAction.setAction(&m_startAction);
        m_hiddenStopAction.setAction(&m_undisturbableAction);
        return;
    }

    QTC_ASSERT(currentEngine, return);

    // We have a current engine, and it belongs to the startup runconfig.
    // The 'state' bits only affect the fat debug button, not the preset start button.
    m_startAction.setIcon(startIcon(false));
    m_startAction.setEnabled(false);
    m_startAction.setVisible(false);

    m_debugWithoutDeployAction.setEnabled(canRun.has_value());

    const DebuggerState state = currentEngine->state();

    if (state == InferiorStopOk) {
        // F5 continues, Shift-F5 kills. It is "continuable".
        m_startAction.setEnabled(false);
        m_debugWithoutDeployAction.setEnabled(false);
        m_visibleStartAction.setAction(ActionManager::command(Constants::CONTINUE)->action());
        m_hiddenStopAction.setAction(ActionManager::command(Constants::STOP)->action());
    } else if (state == InferiorRunOk) {
        // Shift-F5 interrupts. It is also "interruptible".
        m_startAction.setEnabled(false);
        m_debugWithoutDeployAction.setEnabled(false);
        m_visibleStartAction.setAction(ActionManager::command(Constants::INTERRUPT)->action());
        m_hiddenStopAction.setAction(ActionManager::command(Constants::INTERRUPT)->action());
    } else if (state == DebuggerFinished) {
        // We don't want to do anything anymore.
        m_startAction.setEnabled(canRun.has_value());
        m_debugWithoutDeployAction.setEnabled(canRun.has_value());
        m_visibleStartAction.setAction(ActionManager::command(DEBUGGER_START)->action());
        m_hiddenStopAction.setAction(&m_undisturbableAction);
    } else if (state == InferiorUnrunnable) {
        // We don't want to do anything anymore.
        m_startAction.setEnabled(false);
        m_debugWithoutDeployAction.setEnabled(false);
        m_visibleStartAction.setAction(ActionManager::command(Constants::STOP)->action());
        m_hiddenStopAction.setAction(ActionManager::command(Constants::STOP)->action());
    } else {
        // The startup phase should be over once we are here.
        // But treat it as 'undisturbable if we are here by accident.
        //QTC_CHECK(state != DebuggerNotReady);
        // Everything else is "undisturbable".
        m_startAction.setEnabled(false);
        m_debugWithoutDeployAction.setEnabled(false);
        m_visibleStartAction.setAction(&m_undisturbableAction);
        m_hiddenStopAction.setAction(&m_undisturbableAction);
    }

// FIXME: Decentralize the actions below
    const bool actionsEnabled = currentEngine->debuggerActionsEnabled();
    const bool canDeref = actionsEnabled && currentEngine->hasCapability(AutoDerefPointersCapability);
    DebuggerSettings &s = settings();
    s.autoDerefPointers.setEnabled(canDeref);
    s.autoDerefPointers.setEnabled(true);
    s.expandStack.setEnabled(actionsEnabled);

    m_startAndDebugApplicationAction.setEnabled(true);
    m_attachToQmlPortAction.setEnabled(true);
    m_attachToCoreAction.setEnabled(true);
    m_attachToLastCoreAction.setEnabled(Utils::HostOsInfo::isLinuxHost());

    m_attachToRemoteServerAction.setEnabled(true);
    m_attachToRunningApplication.setEnabled(true);
    m_attachToUnstartedApplication.setEnabled(true);

    m_watchAction.setEnabled(state != DebuggerFinished && state != DebuggerNotReady);
    m_setOrRemoveBreakpointAction.setEnabled(true);
    m_enableOrDisableBreakpointAction.setEnabled(true);
}

void DebuggerPluginPrivate::onStartupProjectChanged()
{
    for (DebuggerEngine *engine : EngineManager::engines()) {
        // Run controls might be deleted during exit. We disconnect
        // in aboutToShutdown(). Nevertheless double-check.
        QTC_ASSERT(engine, continue);
        engine->updateState();
    }

    updatePresetState();
}

void DebuggerPluginPrivate::attachToLastCore()
{
    QGuiApplication::setOverrideCursor(Qt::WaitCursor);
    LastCore lastCore = getLastCore();
    QGuiApplication::restoreOverrideCursor();
    if (!lastCore) {
        AsynchronousMessageBox::warning(
            Tr::tr("Warning"),
            Tr::tr("coredumpctl did not find any cores created by systemd-coredump."));
        return;
    }

    auto runControl = new RunControl(ProjectExplorer::Constants::DEBUG_RUN_MODE);
    runControl->setKit(KitManager::defaultKit());
    runControl->setDisplayName(Tr::tr("Last Core file \"%1\"").arg(lastCore.coreFile.toUserOutput()));

    DebuggerRunParameters rp = DebuggerRunParameters::fromRunControl(runControl);
    rp.setInferiorExecutable(lastCore.binary);
    rp.setCoreFilePath(lastCore.coreFile);
    rp.setStartMode(AttachToCore);
    rp.setCloseMode(DetachAtClose);

    runControl->setRunRecipe(debuggerRecipe(runControl, rp));
    runControl->start();
}

void DebuggerPluginPrivate::reloadDebuggingHelpers()
{
    if (DebuggerEngine *engine = EngineManager::currentEngine())
        engine->reloadDebuggingHelpers();
    else
        DebuggerMainWindow::showStatusMessage(
            Tr::tr("Reload debugging helpers skipped as no engine is running."), 5000);
}

void DebuggerPluginPrivate::attachToRunningApplication()
{
    auto kitChooser = new KitChooser;
    kitChooser->setShowIcons(true);

    auto dlg = new DeviceProcessesDialog(kitChooser, ICore::dialogParent());
    dlg->addAcceptButton(Tr::tr("&Attach to Process"));
    dlg->showAllDevices();
    if (dlg->exec() == QDialog::Rejected) {
        delete dlg;
        return;
    }

    dlg->setAttribute(Qt::WA_DeleteOnClose);
    Kit *kit = kitChooser->currentKit();
    QTC_ASSERT(kit, return);
    IDevice::ConstPtr device = RunDeviceKitAspect::device(kit);
    QTC_ASSERT(device, return);

    const ProcessInfo processInfo = dlg->currentProcess();

    if (device->type() == PE::DESKTOP_DEVICE_TYPE) {
        attachToRunningProcess(kit, processInfo, false);
    } else {
        auto runControl = new RunControl(ProjectExplorer::Constants::DEBUG_RUN_MODE);
        runControl->setKit(kit);
        //: %1: PID
        runControl->setDisplayName(Tr::tr("Process %1").arg(processInfo.processId));
        runControl->requestDebugChannel();

        DebuggerRunParameters rp = DebuggerRunParameters::fromRunControl(runControl);
        rp.setServerAttachPid(ProcessHandle(processInfo.processId));
        rp.setServerUseMulti(false);
        rp.setServerEssential(false);
        rp.setStartMode(AttachToRemoteProcess);
        rp.setCloseMode(DetachAtClose);
        rp.setUseContinueInsteadOfRun(true);
        rp.setContinueAfterAttach(false);

        runControl->setRunRecipe(debuggerRecipe(runControl, rp));
        runControl->start();
    }
}

void DebuggerPluginPrivate::attachToUnstartedApplicationDialog()
{
    auto dlg = new UnstartedAppWatcherDialog(
        attachToUnstartedApplicationDialogLastPosition, ICore::dialogParent());

    connect(dlg, &QDialog::finished, this, [this, dlg]() {
        this->attachToUnstartedApplicationDialogLastPosition = dlg->pos();
        dlg->deleteLater();
    });
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
    const ProcessInfo &processInfo, bool contAfterAttach)
{
    QTC_ASSERT(kit, return nullptr);
    IDevice::ConstPtr device = RunDeviceKitAspect::device(kit);
    QTC_ASSERT(device, return nullptr);
    if (processInfo.processId == 0) {
        AsynchronousMessageBox::warning(Tr::tr("Warning"), Tr::tr("Cannot attach to process with PID 0"));
        return nullptr;
    }

    const Abi tcAbi = ToolchainKitAspect::targetAbi(kit);
    const bool isWindows = (tcAbi.os() == Abi::WindowsOS);
    if (isWindows && isWinProcessBeingDebugged(processInfo.processId)) {
        AsynchronousMessageBox::warning(
            Tr::tr("Process Already Under Debugger Control"),
            Tr::tr("The process %1 is already under the control of a debugger.\n"
                   "%2 cannot attach to it.")
                .arg(processInfo.processId)
                .arg(QGuiApplication::applicationDisplayName()));
        return nullptr;
    }

    if (device->type() != PE::DESKTOP_DEVICE_TYPE) {
        AsynchronousMessageBox::warning(Tr::tr("Not a Desktop Device Type"),
                             Tr::tr("It is only possible to attach to a locally running process."));
        return nullptr;
    }

    auto runControl = new RunControl(ProjectExplorer::Constants::DEBUG_RUN_MODE);
    runControl->setKit(kit);
    //: %1: PID
    runControl->setDisplayName(Tr::tr("Process %1").arg(processInfo.processId));
    runControl->setAttachPid(ProcessHandle(processInfo.processId));

    DebuggerRunParameters rp = DebuggerRunParameters::fromRunControl(runControl);
    rp.setInferiorExecutable(device->filePath(processInfo.executable));
    rp.setStartMode(AttachToLocalProcess);
    rp.setCloseMode(DetachAtClose);
    rp.setContinueAfterAttach(contAfterAttach);

    runControl->setRunRecipe(debuggerRecipe(runControl, rp));
    runControl->start();
    return runControl;
}

void DebuggerPluginPrivate::runScheduled()
{
    for (RunControl *runControl : std::as_const(m_scheduledStarts))
        runControl->start();
}

void DebuggerPluginPrivate::editorOpened(IEditor *editor)
{
    if (auto widget = TextEditorWidget::fromEditor(editor)) {
        connect(widget, &TextEditorWidget::markRequested,
                this, &DebuggerPluginPrivate::requestMark);

        connect(widget, &TextEditorWidget::markContextMenuRequested,
                this, &DebuggerPluginPrivate::requestContextMenu);
    }
}

void DebuggerPluginPrivate::updateBreakMenuItem(IEditor *editor)
{
    BaseTextEditor *textEditor = qobject_cast<BaseTextEditor *>(editor);
    m_setOrRemoveBreakpointAction.setEnabled(textEditor != nullptr);
    m_enableOrDisableBreakpointAction.setEnabled(textEditor != nullptr);
}

void DebuggerPluginPrivate::requestContextMenu(TextEditorWidget *widget,
    int lineNumber, QMenu *menu)
{
    TextDocument *document = widget->textDocument();

    const ContextData args = getLocationContext(document, lineNumber);
    const GlobalBreakpoint gbp = BreakpointManager::findBreakpointFromContext(args);

    if (gbp) {

        // Remove existing breakpoint.
        auto act = menu->addAction(Tr::tr("Remove Breakpoint"));
        connect(act, &QAction::triggered, [gbp] { gbp->deleteBreakpoint(); });

        // Enable/disable existing breakpoint.
        if (gbp->isEnabled()) {
            act = menu->addAction(Tr::tr("Disable Breakpoint"));
            connect(act, &QAction::triggered, [gbp] { gbp->setEnabled(false); });
        } else {
            act = menu->addAction(Tr::tr("Enable Breakpoint"));
            connect(act, &QAction::triggered, [gbp] { gbp->setEnabled(true); });
        }

        // Edit existing breakpoint.
        act = menu->addAction(Tr::tr("Edit Breakpoint..."));
        connect(act, &QAction::triggered, [gbp] {
            BreakpointManager::editBreakpoint(gbp, ICore::dialogParent());
        });

    } else {
        // Handle non-existing breakpoint.
        const QString text = args.address
            ? Tr::tr("Set Breakpoint at 0x%1").arg(args.address, 0, 16)
            : Tr::tr("Set Breakpoint at Line %1").arg(lineNumber);
        auto act = menu->addAction(text);
        act->setEnabled(args.isValid());
        connect(act, &QAction::triggered, this, [this, args] {
            breakpointSetMarginActionTriggered(false, args);
        });

        // Message trace point
        const QString tracePointText = args.address
            ? Tr::tr("Set Message Tracepoint at 0x%1...").arg(args.address, 0, 16)
            : Tr::tr("Set Message Tracepoint at Line %1...").arg(lineNumber);
        act = menu->addAction(tracePointText);
        act->setEnabled(args.isValid());
        connect(act, &QAction::triggered, this, [this, args] {
            breakpointSetMarginActionTriggered(true, args);
        });
    }

    // Run to, jump to line below in stopped state.
    for (const QPointer<DebuggerEngine> &engine : EngineManager::engines()) {
        if (engine->state() == InferiorStopOk && args.isValid()) {
            menu->addSeparator();
            if (engine->hasCapability(RunToLineCapability)) {
                auto act = menu->addAction(args.address
                                           ? Tr::tr("Run to Address 0x%1").arg(args.address, 0, 16)
                                           : Tr::tr("Run to Line %1").arg(args.textPosition.line));
                connect(act, &QAction::triggered, this, [args, engine] {
                    QTC_ASSERT(engine, return);
                    engine->executeRunToLine(args);
                });
            }
            if (engine->hasCapability(JumpToLineCapability)) {
                auto act = menu->addAction(args.address
                                           ? Tr::tr("Jump to Address 0x%1").arg(args.address, 0, 16)
                                           : Tr::tr("Jump to Line %1").arg(args.textPosition.line));
                connect(act, &QAction::triggered, this, [args, engine] {
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
                    const QString text = Tr::tr("Disassemble Function \"%1\"")
                            .arg(frame.function);
                    auto act = new QAction(text, menu);
                    connect(act, &QAction::triggered, this, [frame, engine] {
                        QTC_ASSERT(engine, return);
                        engine->openDisassemblerView(Location(frame));
                    });
                    menu->addAction(act);
                }
            }
        }
    }
}

void DebuggerPluginPrivate::setOrRemoveBreakpoint()
{
    const BaseTextEditor *textEditor = BaseTextEditor::currentTextEditor();
    QTC_ASSERT(textEditor, return);
    const int lineNumber = textEditor->currentLine();
    ContextData location = getLocationContext(textEditor->textDocument(), lineNumber);
    if (location.isValid())
        BreakpointManager::setOrRemoveBreakpoint(location);
}

void DebuggerPluginPrivate::enableOrDisableBreakpoint()
{
    const BaseTextEditor *textEditor = BaseTextEditor::currentTextEditor();
    QTC_ASSERT(textEditor, return);
    const int lineNumber = textEditor->currentLine();
    ContextData location = getLocationContext(textEditor->textDocument(), lineNumber);
    if (location.isValid())
        BreakpointManager::enableOrDisableBreakpoint(location);
}

void DebuggerPluginPrivate::requestMark(TextEditorWidget *widget, int lineNumber,
                                        TextMarkRequestKind kind)
{
    if (kind == BreakpointRequest) {
        ContextData location = getLocationContext(widget->textDocument(), lineNumber);
        if (location.isValid())
            BreakpointManager::setOrRemoveBreakpoint(location);
    }
}

void DebuggerPluginPrivate::setInitialState()
{
    m_startAndDebugApplicationAction.setEnabled(true);
    m_attachToQmlPortAction.setEnabled(true);
    m_attachToCoreAction.setEnabled(true);
    m_attachToRemoteServerAction.setEnabled(true);
    m_attachToRunningApplication.setEnabled(true);
    m_attachToUnstartedApplication.setEnabled(true);

    m_watchAction.setEnabled(false);
    m_setOrRemoveBreakpointAction.setEnabled(false);
    m_enableOrDisableBreakpointAction.setEnabled(false);
    //m_snapshotAction.setEnabled(false);

    settings().autoDerefPointers.setEnabled(true);
    settings().expandStack.setEnabled(false);
}

void DebuggerPluginPrivate::dumpLog()
{
    DebuggerEngine *engine = EngineManager::currentEngine();
    if (!engine)
        return;
    LogWindow *logWindow = engine->logWindow();
    QTC_ASSERT(logWindow, return);

    const FilePath filePath = FileUtils::getSaveFilePath(Tr::tr("Save Debugger Log"),
                              TemporaryDirectory::masterDirectoryFilePath());
    if (filePath.isEmpty())
        return;
    FileSaver saver(filePath);
    if (!saver.hasError()) {
        QTextStream ts(saver.file());
        ts << logWindow->inputContents();
        ts << "\n\n=======================================\n\n";
        ts << logWindow->combinedContents();
        saver.setResult(&ts);
    }
    if (const Result<> res = saver.finalize(); !res)
        FileUtils::showError(res.error());
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

void DebuggerPluginPrivate::extensionsInitialized()
{
    QTimer::singleShot(0, this, &DebuggerItemManager::restoreDebuggers);

    // If the CppEditor or QmlJS editor plugin is there, we want to add something to
    // the editor context menu.
    for (Id menuId : {Id(CppEditor::Constants::M_CONTEXT), Id(QmlJSEditor::Constants::M_CONTEXT)}) {
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

    DebuggerMainWindow::ensureMainWindowExists();
}

QWidget *DebuggerPluginPrivate::addSearch(BaseTreeView *treeView)
{
    BoolAspect &act = settings().useAlternatingRowColors;
    treeView->setAlternatingRowColors(act());
    treeView->setProperty(PerspectiveState::savesHeaderKey(), true);
    connect(&act, &BaseAspect::changed, treeView, [treeView] {
        treeView->setAlternatingRowColors(settings().useAlternatingRowColors());
    });

    return ItemViewFind::createSearchableWrapper(treeView);
}

Console *debuggerConsole()
{
    return &dd->m_console;
}

QWidget *addSearch(BaseTreeView *treeView)
{
    return dd->addSearch(treeView);
}

void openTextEditor(const QString &titlePattern0, const QString &contents)
{
    if (PluginManager::isShuttingDown())
        return;
    QString titlePattern = titlePattern0;
    IEditor *editor = EditorManager::openEditorWithContents(
                CC::K_DEFAULT_TEXT_EDITOR_ID, &titlePattern, contents.toUtf8(), QString(),
                EditorManager::IgnoreNavigationHistory);
    if (auto textEditor = qobject_cast<BaseTextEditor *>(editor)) {
        QString suggestion = titlePattern;
        if (!suggestion.contains('.'))
            suggestion.append(".txt");
        textEditor->textDocument()->setFallbackSaveAsFileName(suggestion);
    }
    QTC_ASSERT(editor, return);
}

///////////////////////////////////////////////////////////////////////
//
// DebuggerPlugin
//
///////////////////////////////////////////////////////////////////////

class DebuggerPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Debugger.json")

public:
    DebuggerPlugin();
    ~DebuggerPlugin() final;

private:
    // IPlugin implementation.
    Utils::Result<> initialize(const QStringList &arguments) final;
    QObject *remoteCommand(const QStringList &options,
                           const QString &workingDirectory,
                           const QStringList &arguments) final;
    ShutdownFlag aboutToShutdown() final;
    void extensionsInitialized() final;

    // Called from AppOutputPane::attachToRunControl().
    Q_SLOT void attachExternalApplication(ProjectExplorer::RunControl *rc);

    // Called from GammaRayIntegration
    Q_SLOT void getEnginesState(QByteArray *json) const;

    // Called from DockerDevice
    Q_SLOT void autoDetectDebuggersForDevice(const Utils::FilePaths &searchPaths,
                                             const QString &detectionId,
                                             QString *logMessage);
    Q_SLOT void removeDetectedDebuggers(const QString &detectionId, QString *logMessage);
    Q_SLOT void listDetectedDebuggers(const QString &detectionId, QString *logMessage);

    Q_SLOT void attachToProcess(const qint64 processId, const Utils::FilePath &executable);
};

DebuggerPlugin::DebuggerPlugin()
{
    setObjectName("DebuggerPlugin");
    m_instance = this;

    qRegisterMetaType<PerspectiveState>("Utils::PerspectiveState");
}

DebuggerPlugin::~DebuggerPlugin()
{
    delete dd;
    dd = nullptr;
    m_instance = nullptr;
}

IPlugin::ShutdownFlag DebuggerPlugin::aboutToShutdown()
{
    ExtensionSystem::PluginManager::removeObject(this);

    disconnect(ProjectManager::instance(), &ProjectManager::startupProjectChanged, dd, nullptr);

    dd->m_shutdownTimer.setInterval(0);
    dd->m_shutdownTimer.setSingleShot(true);

    const auto doShutdown = [this] {
        DebuggerMainWindow::doShutdown();

        dd->m_shutdownTimer.stop();
        disconnect(EngineManager::instance(), &EngineManager::shutDownCompleted, this, nullptr);

        delete dd->m_mode;
        dd->m_mode = nullptr;
        emit asynchronousShutdownFinished();
    };

    connect(&dd->m_shutdownTimer, &QTimer::timeout, this, doShutdown);

    if (EngineManager::shutDown()) {
        // If any engine is aborting we give them extra three seconds.
        dd->m_shutdownTimer.setInterval(3000);
        connect(EngineManager::instance(), &EngineManager::shutDownCompleted, this, doShutdown,
                Qt::QueuedConnection);
    }
    dd->m_shutdownTimer.start();

    return AsynchronousShutdown;
}

QObject *DebuggerPlugin::remoteCommand(const QStringList &options,
                                       const QString &workingDirectory,
                                       const QStringList &list)
{
    Q_UNUSED(workingDirectory)
    Q_UNUSED(list)
    dd->remoteCommand(options);
    return nullptr;
}

void DebuggerPlugin::extensionsInitialized()
{
    dd->extensionsInitialized();
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
    if (RunConfiguration *runConfig = activeRunConfigForActiveProject()) {
        if (const BuildConfiguration *buildConfig = runConfig->buildConfiguration())
            buildType = buildConfig->buildType();
    }
    return buildType;
}

void showCannotStartDialog(const QString &text)
{
    auto errorDialog = new QMessageBox(ICore::dialogParent());
    errorDialog->setAttribute(Qt::WA_DeleteOnClose);
    errorDialog->setIcon(QMessageBox::Warning);
    errorDialog->setWindowTitle(text);
    errorDialog->setText(Tr::tr("Cannot start %1 without a project. Please open the project "
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
                currentMode = Tr::tr("Debug");
                break;
            case BuildConfiguration::Profile:
                currentMode = Tr::tr("Profile");
                break;
            case BuildConfiguration::Release:
                currentMode = Tr::tr("Release");
                break;
            default:
                QTC_CHECK(false);
        }

        QString toolModeString;
        switch (toolMode) {
            case DebugMode:
                toolModeString = Tr::tr("in Debug mode");
                break;
            case ProfileMode:
                toolModeString = Tr::tr("in Profile mode");
                break;
            case ReleaseMode:
                toolModeString = Tr::tr("in Release mode");
                break;
            case SymbolsMode:
                toolModeString = Tr::tr("with debug symbols (Debug or Profile mode)");
                break;
            case OptimizedMode:
                toolModeString = Tr::tr("on optimized code (Profile or Release mode)");
                break;
            default:
                QTC_CHECK(false);
        }
        const QString title = Tr::tr("Run %1 in %2 Mode?").arg(toolName).arg(currentMode);
        const QString message = Tr::tr("<html><head/><body><p>You are trying "
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
        if (Utils::CheckableMessageBox::question(title,
                                                 message,
                                                 Key("AnalyzerCorrectModeWarning"))
            != QMessageBox::Yes)
                return false;
    }

    return true;
}

QAction *createStartAction()
{
    auto action = new QAction(Tr::tr("Start"), m_instance);
    action->setIcon(ProjectExplorer::Icons::ANALYZER_START_SMALL_TOOLBAR.icon());
    action->setEnabled(true);
    return action;
}

QAction *createStopAction()
{
    auto action = new QAction(Tr::tr("Stop"), m_instance);
    action->setIcon(Utils::Icons::STOP_SMALL_TOOLBAR.icon());
    action->setEnabled(true);
    return action;
}

void enableMainWindow(bool on)
{
    DebuggerMainWindow::instance()->setEnabled(on);
}

void showPermanentStatusMessage(const QString &message)
{
    DebuggerMainWindow::showStatusMessage(message, -1);
}

namespace Internal {

Result<> DebuggerPlugin::initialize(const QStringList &arguments)
{
    IOptionsPage::registerCategory(
        DEBUGGER_SETTINGS_CATEGORY,
        Tr::tr("Debugger"),
        ":/debugger/images/settingscategory_debugger.png");

    IOptionsPage::registerCategory(
        "T.Analyzer",
        Tr::tr("Analyzer"),
        ":/debugger/images/settingscategory_analyzer.png");

    // Needed for call from AppOutputPane::attachToRunControl() and GammarayIntegration.
    ExtensionSystem::PluginManager::addObject(this);

    dd = new DebuggerPluginPrivate(arguments);

#ifdef WITH_TESTS
    addTestCreator(createDebuggerTest);
#endif

    return ResultOk;
}

void DebuggerPlugin::attachToProcess(const qint64 processId, const Utils::FilePath &executable)
{
    ProcessInfo processInfo;
    processInfo.processId = processId;
    processInfo.executable = executable.toUrlishString();

    auto kitChooser = new KitChooser;
    kitChooser->setShowIcons(true);
    kitChooser->populate();
    Kit *kit = kitChooser->currentKit();

    dd->attachToRunningProcess(kit, processInfo, false);
}

void DebuggerPlugin::attachExternalApplication(RunControl *rc)
{
    const ProcessHandle pid = rc->applicationProcessHandle();

    auto runControl = new RunControl(ProjectExplorer::Constants::DEBUG_RUN_MODE);
    runControl->setBuildConfiguration(rc->buildConfiguration());
    runControl->setDisplayName(Tr::tr("Process %1").arg(pid.pid()));
    runControl->setAttachPid(pid);

    DebuggerRunParameters rp = DebuggerRunParameters::fromRunControl(runControl);
    rp.setInferiorExecutable(rc->targetFilePath());
    rp.setStartMode(AttachToLocalProcess);
    rp.setCloseMode(DetachAtClose);

    runControl->setRunRecipe(debuggerRecipe(runControl, rp));
    runControl->start();
}

void DebuggerPlugin::getEnginesState(QByteArray *json) const
{
    QTC_ASSERT(json, return);
    QVariantMap result {
        {"version", 1}
    };
    QVariantMap states;

    int i = 0;
    DebuggerEngine *currentEngine = EngineManager::currentEngine();
    for (DebuggerEngine *engine : EngineManager::engines()) {
        states[QString::number(i)] = QVariantMap({
                   {"current", engine == currentEngine},
                   {"pid", engine->inferiorPid()},
                   {"state", engine->state()}
        });
        ++i;
    }

    if (!states.isEmpty())
        result["states"] = states;

    *json = QJsonDocument(QJsonObject::fromVariantMap(result)).toJson();
}

void DebuggerPlugin::autoDetectDebuggersForDevice(const FilePaths &searchPaths,
                                                  const QString &detectionSource,
                                                  QString *logMessage)
{
    DebuggerItemManager::autoDetectDebuggersForDevice(searchPaths, detectionSource, logMessage);
}

void DebuggerPlugin::removeDetectedDebuggers(const QString &detectionSource, QString *logMessage)
{
    DebuggerItemManager::removeDetectedDebuggers(detectionSource, logMessage);
}

void DebuggerPlugin::listDetectedDebuggers(const QString &detectionSource, QString *logMessage)
{
    DebuggerItemManager::listDetectedDebuggers(detectionSource, logMessage);
}

} // Internal
} // Debugger

#include "debuggerplugin.moc"
