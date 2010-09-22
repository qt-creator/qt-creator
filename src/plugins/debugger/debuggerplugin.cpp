/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "debuggerplugin.h"

#include "debuggeractions.h"
#include "debuggeragents.h"
#include "debuggerconstants.h"
#include "debuggerdialogs.h"
#include "debuggerengine.h"
#include "debuggermainwindow.h"
#include "debuggeroutputwindow.h"
#include "debuggerplugin.h"
#include "debuggerrunner.h"
#include "debuggerstringutils.h"
#include "debuggertooltip.h"
#include "debuggeruiswitcher.h"

#include "breakwindow.h"
#include "moduleswindow.h"
#include "registerwindow.h"
#include "snapshotwindow.h"
#include "stackwindow.h"
#include "sourcefileswindow.h"
#include "threadswindow.h"
#include "watchwindow.h"

#include "breakhandler.h"
#include "sessionengine.h"
#include "snapshothandler.h"
#include "threadshandler.h"
#include "watchutils.h"

#ifdef Q_OS_WIN
#  include "shared/peutils.h"
#endif


#include "ui_commonoptionspage.h"
#include "ui_dumperoptionpage.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/basemode.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/findplaceholder.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/imode.h>
#include <coreplugin/icorelistener.h>
#include <coreplugin/manhattanstyle.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/minisplitter.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/navigationwidget.h>
#include <coreplugin/outputpane.h>
#include <coreplugin/rightpane.h>

#include <cplusplus/ExpressionUnderCursor.h>
#include <cplusplus/CppDocument.h>

#include <cppeditor/cppeditorconstants.h>
#include <cpptools/cppmodelmanagerinterface.h>

#include <extensionsystem/pluginmanager.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <texteditor/basetexteditor.h>
#include <texteditor/basetextmark.h>
#include <texteditor/fontsettings.h>
#include <texteditor/itexteditor.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditorsettings.h>

//#include <qt4projectmanager/qt4projectmanagerconstants.h>

#include <utils/qtcassert.h>
#include <utils/savedaction.h>
#include <utils/styledbar.h>

#include <qml/scriptconsole.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QObject>
#include <QtCore/QPoint>
#include <QtCore/QSettings>
#include <QtCore/QTextStream>
#include <QtCore/QTime>
#include <QtCore/QTimer>
#include <QtCore/QVariant>
#include <QtCore/QtPlugin>

#include <QtGui/QAbstractItemView>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QComboBox>
#include <QtGui/QDockWidget>
#include <QtGui/QErrorMessage>
#include <QtGui/QFileDialog>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QMenu>
#include <QtGui/QMessageBox>
#include <QtGui/QPlainTextEdit>
#include <QtGui/QPushButton>
#include <QtGui/QStatusBar>
#include <QtGui/QTextBlock>
#include <QtGui/QTextCursor>
#include <QtGui/QToolButton>
#include <QtGui/QToolTip>
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
//      Inferior-     Inferior-   Inferior-  RunEngine-                     +
//     Unrunnable}     StopOk}     RunOk}     Failed}                       +
//           +           +            +           +                         +
//   InferiorUnrunnable  +     InferiorRunOk      +                         +
//                       +                        +                         +
//                InferiorStopOk            EngineRunFailed                 +
//                                                +                         v
//                                                 `-+-+-+-+-+-+-+-+-+-+-+>-+
//                                                                          +
//                                                                          +
//                       #Interupt@InferiorRunOk#                           +
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
using namespace Debugger::Internal;
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
const char * const STOP_KEY                 = "Shift+F5";
const char * const RESET_KEY                = "Ctrl+Shift+F5";
const char * const STEP_KEY                 = "F7";
const char * const STEPOUT_KEY              = "Shift+F7";
const char * const NEXT_KEY                 = "F6";
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
} // namespace Debugger



static SessionManager *sessionManager()
{
    return ProjectExplorerPlugin::instance()->session();
}

static QSettings *settings()
{
    return ICore::instance()->settings();
}

static QToolButton *toolButton(QAction *action)
{
    QToolButton *button = new QToolButton;
    button->setDefaultAction(action);
    return button;
}

namespace Debugger {
namespace Internal {

static const char *Role = "ROLE";

// FIXME: Outdated?
// The createCdbEngine function takes a list of options pages it can add to.
// This allows for having a "enabled" toggle on the page independently
// of the engine. That's good for not enabling the related ActiveX control
// unnecessarily.

void addGdbOptionPages(QList<Core::IOptionsPage*> *opts);
void addScriptOptionPages(QList<Core::IOptionsPage*> *opts);
void addTcfOptionPages(QList<Core::IOptionsPage*> *opts);
#ifdef CDB_ENABLED
void addCdbOptionPages(QList<Core::IOptionsPage*> *opts);
#endif


struct AttachRemoteParameters
{
    AttachRemoteParameters() : attachPid(0), winCrashEvent(0) {}

    quint64 attachPid;
    QString attachTarget;  // core file name or  server:port
    // Event handle for attaching to crashed Windows processes. 
    quint64 winCrashEvent;
};


///////////////////////////////////////////////////////////////////////
//
// DebugMode
//
///////////////////////////////////////////////////////////////////////

class DebugMode : public Core::BaseMode
{
public:
    DebugMode(QObject *parent = 0) : BaseMode(parent)
    {
        setDisplayName(QCoreApplication::translate("Debugger::Internal::DebugMode", "Debug"));
        setType(Core::Constants::MODE_EDIT_TYPE);
        setId(MODE_DEBUG);
        setIcon(QIcon(__(":/fancyactionbar/images/mode_Debug.png")));
        setPriority(P_MODE_DEBUG);
    }

    ~DebugMode()
    {
        // Make sure the editor manager does not get deleted.
        EditorManager::instance()->setParent(0);
    }
};

///////////////////////////////////////////////////////////////////////
//
// LocationMark
//
///////////////////////////////////////////////////////////////////////

// Used in "real" editors
class LocationMark : public TextEditor::BaseTextMark
{
public:
    LocationMark(const QString &fileName, int linenumber)
        : BaseTextMark(fileName, linenumber)
    {}

    QIcon icon() const { return DebuggerPlugin::instance()->locationMarkIcon(); }
    void updateLineNumber(int /*lineNumber*/) {}
    void updateBlock(const QTextBlock & /*block*/) {}
    void removedFromEditor() {}
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
    void apply() { m_group.apply(settings()); }
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

    m_group.insert(theDebuggerAction(ListSourceFiles),
        m_ui.checkBoxListSourceFiles);
    m_group.insert(theDebuggerAction(UseAlternatingRowColors),
        m_ui.checkBoxUseAlternatingRowColors);
    m_group.insert(theDebuggerAction(UseToolTipsInMainEditor),
        m_ui.checkBoxUseToolTipsInMainEditor);
    m_group.insert(theDebuggerAction(AutoDerefPointers), 0);
    m_group.insert(theDebuggerAction(UseToolTipsInLocalsView), 0);
    m_group.insert(theDebuggerAction(UseToolTipsInBreakpointsView), 0);
    m_group.insert(theDebuggerAction(UseAddressInBreakpointsView), 0);
    m_group.insert(theDebuggerAction(UseAddressInStackView), 0);
    m_group.insert(theDebuggerAction(MaximalStackDepth),
        m_ui.spinBoxMaximalStackDepth);
    m_group.insert(theDebuggerAction(ShowStdNamespace), 0);
    m_group.insert(theDebuggerAction(ShowQtNamespace), 0);
    m_group.insert(theDebuggerAction(LogTimeStamps), 0);
    m_group.insert(theDebuggerAction(VerboseLog), 0);
    m_group.insert(theDebuggerAction(UsePreciseBreakpoints), 0);
    m_group.insert(theDebuggerAction(BreakOnThrow), 0);
    m_group.insert(theDebuggerAction(BreakOnCatch), 0);
#ifdef Q_OS_WIN
    Utils::SavedAction *registerAction = theDebuggerAction(RegisterForPostMortem);
    m_group.insert(registerAction,
        m_ui.checkBoxRegisterForPostMortem);
    connect(registerAction, SIGNAL(toggled(bool)),
            m_ui.checkBoxRegisterForPostMortem, SLOT(setChecked(bool)));
#endif

    if (m_searchKeywords.isEmpty()) {
        QTextStream(&m_searchKeywords) << ' '
                << m_ui.checkBoxListSourceFiles->text()
                << ' ' << m_ui.checkBoxUseAlternatingRowColors->text()
                << ' ' << m_ui.checkBoxUseToolTipsInMainEditor->text()
#ifdef Q_OS_WIN
                << ' ' << m_ui.checkBoxRegisterForPostMortem->text()
#endif
                << ' ' << m_ui.labelMaximalStackDepth->text();
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

static inline bool oxygenStyle()
{
    if (const ManhattanStyle *ms = qobject_cast<const ManhattanStyle *>(qApp->style()))
        return !qstrcmp("OxygenStyle", ms->baseStyle()->metaObject()->className());
    return false;
}

class DebuggingHelperOptionPage : public Core::IOptionsPage
{   // Needs tr - context
    Q_OBJECT
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
    void apply() { m_group.apply(settings()); }
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
        Core::ICore::instance()->resourcePath() + "../../lib");

    m_group.clear();
    m_group.insert(theDebuggerAction(UseDebuggingHelpers),
        m_ui.debuggingHelperGroupBox);
    m_group.insert(theDebuggerAction(UseCustomDebuggingHelperLocation),
        m_ui.customLocationGroupBox);
    // Suppress Oxygen style's giving flat group boxes bold titles.
    if (oxygenStyle())
        m_ui.customLocationGroupBox->setStyleSheet(_("QGroupBox::title { font: ; }"));

    m_group.insert(theDebuggerAction(CustomDebuggingHelperLocation),
        m_ui.dumperLocationChooser);

    m_group.insert(theDebuggerAction(UseCodeModel),
        m_ui.checkBoxUseCodeModel);

#ifdef QT_DEBUG
    m_group.insert(theDebuggerAction(DebugDebuggingHelpers),
        m_ui.checkBoxDebugDebuggingHelpers);
#else
    m_ui.checkBoxDebugDebuggingHelpers->hide();
#endif

#ifndef QT_DEBUG
#if 0
    cmd = am->registerAction(m_manager->m_dumpLogAction,
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
                << ' ' << m_ui.checkBoxDebugDebuggingHelpers->text();
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
        *enabledEngines &= ~Debugger::CdbEngineType;
        return true;
    }
    if (option == _("-disable-gdb")) {
        *enabledEngines &= ~Debugger::GdbEngineType;
        return true;
    }
    if (option == _("-disable-qmldb")) {
        *enabledEngines &= ~Debugger::QmlEngineType;
        return true;
    }
    if (option == _("-disable-sdb")) {
        *enabledEngines &= ~Debugger::ScriptEngineType;
        return true;
    }
    if (option == _("-disable-tcf")) {
        *enabledEngines &= ~TcfEngineType;
        return true;
    }

    *errorMessage = DebuggerPlugin::tr("Invalid debugger option: %1").arg(option);
    return false;
}

static bool parseArguments(const QStringList &args,
   AttachRemoteParameters *attachRemoteParameters,
   unsigned *enabledEngines, QString *errorMessage)
{
    const QStringList::const_iterator cend = args.constEnd();
    for (QStringList::const_iterator it = args.constBegin(); it != cend; ++it)
        if (!parseArgument(it, cend, attachRemoteParameters, enabledEngines, errorMessage))
            return false;
    if (Debugger::Constants::Internal::debug)
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

static bool isDebuggable(Core::IEditor *editor)
{
    // Only blacklist Qml. Whitelisting would fail on C++ code in files
    // with strange names, more harm would be done this way.
    //Core::IFile *file = editor->file();
    //return !(file && file->mimeType() == "application/x-qml");

    // Nowadays, even Qml is debuggable.
    Q_UNUSED(editor);
    return true;
}

static TextEditor::ITextEditor *currentTextEditor()
{
    EditorManager *editorManager = EditorManager::instance();
    if (!editorManager)
        return 0;
    Core::IEditor *editor = editorManager->currentEditor();
    return qobject_cast<ITextEditor*>(editor);
}

///////////////////////////////////////////////////////////////////////
//
// DebuggerPluginPrivate
//
///////////////////////////////////////////////////////////////////////

struct DebuggerActions
{
    QAction *continueAction;
    QAction *stopAction;
    QAction *interruptAction; // on the fat debug button
    QAction *resetAction; // FIXME: Should not be needed in a stable release
    QAction *stepAction;
    QAction *stepOutAction;
    QAction *runToLineAction1; // in the Debug menu
    QAction *runToLineAction2; // in the text editor context menu
    QAction *runToFunctionAction;
    QAction *jumpToLineAction1; // in the Debug menu
    QAction *jumpToLineAction2; // in the text editor context menu
    QAction *returnFromFunctionAction;
    QAction *nextAction;
    QAction *snapshotAction;
    QAction *watchAction1; // in the Debug menu
    QAction *watchAction2; // in the text editor context menu
    QAction *breakAction;
    QAction *sepAction;
    QAction *reverseDirectionAction;
    QAction *frameUpAction;
    QAction *frameDownAction;
};

} // namespace Internal

using namespace Debugger::Internal;

///////////////////////////////////////////////////////////////////////
//
// DebuggerPluginPrivate
//
///////////////////////////////////////////////////////////////////////

class DebuggerPluginPrivate : public QObject
{
    Q_OBJECT

public:
    explicit DebuggerPluginPrivate(DebuggerPlugin *plugin);

    bool initialize(const QStringList &arguments, QString *errorMessage);
    void notifyCurrentEngine(int role, const QVariant &value = QVariant());
    void connectEngine(DebuggerEngine *engine, bool notify = true);
    void disconnectEngine() { connectEngine(m_sessionEngine); }

public slots:
    void updateWatchersHeader(int section, int, int newSize)
        { m_watchersWindow->header()->resizeSection(section, newSize); }

    void sourceFilesDockToggled(bool on)
        { if (on) notifyCurrentEngine(RequestReloadSourceFilesRole); }
    void modulesDockToggled(bool on)
        { if (on) notifyCurrentEngine(RequestReloadModulesRole); }
    void registerDockToggled(bool on)
        { if (on) notifyCurrentEngine(RequestReloadRegistersRole); }

    void onAction();
    void setSimpleDockWidgetArrangement(const Debugger::DebuggerLanguages &activeLanguages);

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
    void toggleBreakpoint(const QString &fileName, int lineNumber);
    void onModeChanged(Core::IMode *mode);
    void showSettingsDialog();

    void startExternalApplication();
    void startRemoteApplication();
    void attachExternalApplication();
    void attachExternalApplication
        (qint64 pid, const QString &binary, const QString &crashParameter);
    void attachCore();
    void attachCore(const QString &core, const QString &exeFileName);
    void attachCmdLine();
    void attachRemote(const QString &spec);
    void attachRemoteTcf();

    void interruptDebuggingRequest();
    void exitDebugger();

    void enableReverseDebuggingTriggered(const QVariant &value);
    void languagesChanged(const Debugger::DebuggerLanguages &languages);
    void showStatusMessage(const QString &msg, int timeout = -1);
    void openMemoryEditor();

    DebuggerMainWindow *mainWindow()
        { return qobject_cast<DebuggerMainWindow*>
            (DebuggerUISwitcher::instance()->mainWindow()); }

    void setConfigValue(const QString &name, const QVariant &value)
        { settings()->setValue(name, value); }
    QVariant configValue(const QString &name) const
        { return settings()->value(name); }

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
    void onCurrentProjectChanged(ProjectExplorer::Project *project);

    void resetLocation();
    void gotoLocation(const QString &file, int line, bool setMarker);

    void clearStatusMessage();

    void sessionLoaded();
    void aboutToUnloadSession();
    void aboutToSaveSession();

    void executeDebuggerCommand();
    void scriptExpressionEntered(const QString &expression);

public:
    DebuggerState m_state;
    uint m_capabilities;
    DebuggerUISwitcher *m_uiSwitcher;
    DebuggerPlugin *m_manager;
    DebugMode *m_debugMode;
    DebuggerRunControlFactory *m_debuggerRunControlFactory;

    QString m_previousMode;
    TextEditor::BaseTextMark *m_locationMark;
    Core::Context m_continuableContext;
    Core::Context m_interruptibleContext;
    AttachRemoteParameters m_attachRemoteParameters;

    QAction *m_startExternalAction;
    QAction *m_startRemoteAction;
    QAction *m_attachExternalAction;
    QAction *m_attachCoreAction;
    QAction *m_attachTcfAction;
    QAction *m_detachAction;
    QComboBox *m_langBox;
    QToolButton *m_reverseToolButton;

    QIcon m_stopIcon;
    QIcon m_interruptIcon;
    QIcon m_locationMarkIcon;

    QLabel *m_statusLabel;
    QComboBox *m_threadBox;

    QDockWidget *m_breakDock;
    QDockWidget *m_modulesDock;
    QDockWidget *m_outputDock;
    QDockWidget *m_registerDock;
    QDockWidget *m_snapshotDock;
    QDockWidget *m_sourceFilesDock;
    QDockWidget *m_stackDock;
    QDockWidget *m_threadsDock;
    QDockWidget *m_watchDock;
    QDockWidget* m_scriptConsoleDock;

    DebuggerActions m_actions;

    BreakWindow *m_breakWindow;
    QTreeView *m_returnWindow;
    QTreeView *m_localsWindow;
    QTreeView *m_watchersWindow;
    QTreeView *m_commandWindow;
    QAbstractItemView *m_registerWindow;
    QAbstractItemView *m_modulesWindow;
    QAbstractItemView *m_snapshotWindow;
    SourceFilesWindow *m_sourceFilesWindow;
    QAbstractItemView *m_stackWindow;
    QAbstractItemView *m_threadsWindow;
    DebuggerOutputWindow *m_outputWindow;
    ScriptConsole *m_scriptConsoleWindow;

    SessionEngine *m_sessionEngine;

    bool m_busy;
    QTimer m_statusTimer;
    QString m_lastPermanentStatusMessage;

    CPlusPlus::Snapshot m_codeModelSnapshot;
    DebuggerPlugin *m_plugin;

    SnapshotHandler *m_snapshotHandler;
};

DebuggerPluginPrivate::DebuggerPluginPrivate(DebuggerPlugin *plugin)
{
    m_plugin = plugin;

    m_statusLabel = 0;
    m_threadBox = 0;

    m_breakDock = 0;
    m_modulesDock = 0;
    m_outputDock = 0;
    m_registerDock = 0;
    m_snapshotDock = 0;
    m_sourceFilesDock = 0;
    m_stackDock = 0;
    m_threadsDock = 0;
    m_watchDock = 0;

    m_breakWindow = 0;
    m_returnWindow = 0;
    m_localsWindow = 0;
    m_watchersWindow = 0;
    m_registerWindow = 0;
    m_modulesWindow = 0;
    m_snapshotWindow = 0;
    m_sourceFilesWindow = 0;
    m_stackWindow = 0;
    m_threadsWindow = 0;
    m_outputWindow = 0;
    m_scriptConsoleWindow = 0;

    m_sessionEngine = 0;
    m_debugMode = 0;
    m_locationMark = 0;

    m_continuableContext = Core::Context(0);
    m_interruptibleContext = Core::Context(0);

    m_debugMode = 0;
    m_uiSwitcher = 0;
    m_state = DebuggerNotReady;
    m_snapshotHandler = 0;
}

bool DebuggerPluginPrivate::initialize(const QStringList &arguments, QString *errorMessage)
{
    m_continuableContext = Core::Context("Gdb.Continuable");
    m_interruptibleContext = Core::Context("Gdb.Interruptible");

    // FIXME: Move part of this to extensionsInitialized()?
    ICore *core = ICore::instance();
    QTC_ASSERT(core, return false);

    Core::ActionManager *am = core->actionManager();
    QTC_ASSERT(am, return false);

    const Core::Context globalcontext(CC::C_GLOBAL);
    const Core::Context cppDebuggercontext(C_CPPDEBUGGER);
    const Core::Context qmlDebuggerContext(C_QMLDEBUGGER);
    const Core::Context cppeditorcontext(CppEditor::Constants::C_CPPEDITOR);

    m_stopIcon = QIcon(_(":/debugger/images/debugger_stop_small.png"));
    m_stopIcon.addFile(__(":/debugger/images/debugger_stop.png"));
    m_interruptIcon = QIcon(_(":/debugger/images/debugger_interrupt_small.png"));
    m_interruptIcon.addFile(__(":/debugger/images/debugger_interrupt.png"));
    m_locationMarkIcon = QIcon(_(":/debugger/images/location_16.png"));

    m_busy = false;

    m_statusLabel = new QLabel;
    m_statusLabel->setMinimumSize(QSize(30, 10));

    m_breakWindow = new BreakWindow;
    m_breakWindow->setObjectName(QLatin1String("CppDebugBreakpoints"));
    m_modulesWindow = new ModulesWindow;
    m_modulesWindow->setObjectName(QLatin1String("CppDebugModules"));
    m_outputWindow = new DebuggerOutputWindow;
    m_outputWindow->setObjectName(QLatin1String("CppDebugOutput"));

    m_registerWindow = new RegisterWindow;
    m_registerWindow->setObjectName(QLatin1String("CppDebugRegisters"));
    m_snapshotWindow = new SnapshotWindow;
    m_snapshotWindow->setObjectName(QLatin1String("CppDebugSnapshots"));
    m_stackWindow = new StackWindow;
    m_stackWindow->setObjectName(QLatin1String("CppDebugStack"));
    m_sourceFilesWindow = new SourceFilesWindow;
    m_sourceFilesWindow->setObjectName(QLatin1String("CppDebugSources"));
    m_threadsWindow = new ThreadsWindow;
    m_threadsWindow->setObjectName(QLatin1String("CppDebugThreads"));
    m_returnWindow = new WatchWindow(WatchWindow::ReturnType);
    m_returnWindow->setObjectName(QLatin1String("CppDebugReturn"));
    m_localsWindow = new WatchWindow(WatchWindow::LocalsType);
    m_localsWindow->setObjectName(QLatin1String("CppDebugLocals"));
    m_watchersWindow = new WatchWindow(WatchWindow::WatchersType);
    m_watchersWindow->setObjectName(QLatin1String("CppDebugWatchers"));
    m_commandWindow = new QTreeView;
    m_scriptConsoleWindow = new ScriptConsole;
    m_scriptConsoleWindow->setWindowTitle(tr("QML Script Console"));
    m_scriptConsoleWindow->setObjectName(QLatin1String("QMLScriptConsole"));
    connect(m_scriptConsoleWindow, SIGNAL(expressionEntered(QString)),
        SLOT(scriptExpressionEntered(QString)));

    // Session related data
    m_sessionEngine = new SessionEngine;

    // Snapshot
    m_snapshotHandler = new SnapshotHandler;
    m_snapshotWindow->setModel(m_snapshotHandler->model());

    // Debug mode setup
    m_debugMode = new DebugMode(this);

    // Watchers
    connect(m_localsWindow->header(), SIGNAL(sectionResized(int,int,int)),
        this, SLOT(updateWatchersHeader(int,int,int)), Qt::QueuedConnection);

    m_actions.continueAction = new QAction(tr("Continue"), this);
    QIcon continueIcon = QIcon(__(":/debugger/images/debugger_continue_small.png"));
    continueIcon.addFile(__(":/debugger/images/debugger_continue.png"));
    m_actions.continueAction->setIcon(continueIcon);
    m_actions.continueAction->setProperty(Role, RequestExecContinueRole);

    m_actions.stopAction = new QAction(tr("Stop Debugger"), this);
    m_actions.stopAction->setProperty(Role, RequestExecExitRole);
    m_actions.stopAction->setIcon(m_stopIcon);

    m_actions.interruptAction = new QAction(tr("Interrupt"), this);
    m_actions.interruptAction->setIcon(m_interruptIcon);
    m_actions.interruptAction->setProperty(Role, RequestExecInterruptRole);

    m_actions.resetAction = new QAction(tr("Abort Debugging"), this);
    m_actions.resetAction->setProperty(Role, RequestExecResetRole);
    m_actions.resetAction->setToolTip(tr("Aborts debugging and "
        "resets the debugger to the initial state."));

    m_actions.nextAction = new QAction(tr("Step Over"), this);
    m_actions.nextAction->setProperty(Role, RequestExecNextRole);
    m_actions.nextAction->setIcon(
        QIcon(__(":/debugger/images/debugger_stepover_small.png")));

    m_actions.stepAction = new QAction(tr("Step Into"), this);
    m_actions.stepAction->setProperty(Role, RequestExecStepRole);
    m_actions.stepAction->setIcon(
        QIcon(__(":/debugger/images/debugger_stepinto_small.png")));

    m_actions.stepOutAction = new QAction(tr("Step Out"), this);
    m_actions.stepOutAction->setProperty(Role, RequestExecStepOutRole);
    m_actions.stepOutAction->setIcon(
        QIcon(__(":/debugger/images/debugger_stepout_small.png")));

    m_actions.runToLineAction1 = new QAction(tr("Run to Line"), this);
    m_actions.runToLineAction1->setProperty(Role, RequestExecRunToLineRole);
    m_actions.runToLineAction2 = new QAction(tr("Run to Line"), this);
    m_actions.runToLineAction2->setProperty(Role, RequestExecRunToLineRole);

    m_actions.runToFunctionAction =
        new QAction(tr("Run to Outermost Function"), this);
    m_actions.runToFunctionAction->setProperty(Role, RequestExecRunToFunctionRole);

    m_actions.returnFromFunctionAction =
        new QAction(tr("Immediately Return From Inner Function"), this);
    m_actions.returnFromFunctionAction->setProperty(Role, RequestExecReturnFromFunctionRole);

    m_actions.jumpToLineAction1 = new QAction(tr("Jump to Line"), this);
    m_actions.jumpToLineAction1->setProperty(Role, RequestExecJumpToLineRole);
    m_actions.jumpToLineAction2 = new QAction(tr("Jump to Line"), this);
    m_actions.jumpToLineAction2->setProperty(Role, RequestExecJumpToLineRole);

    m_actions.breakAction = new QAction(tr("Toggle Breakpoint"), this);

    m_actions.watchAction1 = new QAction(tr("Add to Watch Window"), this);
    m_actions.watchAction1->setProperty(Role, RequestExecWatchRole);
    m_actions.watchAction2 = new QAction(tr("Add to Watch Window"), this);
    m_actions.watchAction2->setProperty(Role, RequestExecWatchRole);

    m_actions.snapshotAction = new QAction(tr("Create Snapshot"), this);
    m_actions.snapshotAction->setProperty(Role, RequestCreateSnapshotRole);
    m_actions.snapshotAction->setIcon(
        QIcon(__(":/debugger/images/debugger_snapshot_small.png")));

    m_actions.reverseDirectionAction =
        new QAction(tr("Reverse Direction"), this);
    m_actions.reverseDirectionAction->setCheckable(true);
    m_actions.reverseDirectionAction->setChecked(false);
    m_actions.reverseDirectionAction->setIcon(
        QIcon(__(":/debugger/images/debugger_reversemode_16.png")));
    m_actions.reverseDirectionAction->setIconVisibleInMenu(false);

    m_actions.frameDownAction =
        new QAction(tr("Move to Called Frame"), this);
    m_actions.frameDownAction->setProperty(Role, RequestExecFrameDownRole);
    m_actions.frameUpAction =
        new QAction(tr("Move to Calling Frame"), this);
    m_actions.frameUpAction->setProperty(Role, RequestExecFrameUpRole);

    m_actions.reverseDirectionAction->setCheckable(false);
    theDebuggerAction(OperateByInstruction)->
        setProperty(Role, RequestOperatedByInstructionTriggeredRole);

    connect(m_actions.continueAction, SIGNAL(triggered()), SLOT(onAction()));
    connect(m_actions.nextAction, SIGNAL(triggered()), SLOT(onAction()));
    connect(m_actions.stepAction, SIGNAL(triggered()), SLOT(onAction()));
    connect(m_actions.stepOutAction, SIGNAL(triggered()), SLOT(onAction()));
    connect(m_actions.runToLineAction1, SIGNAL(triggered()), SLOT(onAction()));
    connect(m_actions.runToLineAction2, SIGNAL(triggered()), SLOT(onAction()));
    connect(m_actions.runToFunctionAction, SIGNAL(triggered()), SLOT(onAction()));
    connect(m_actions.jumpToLineAction1, SIGNAL(triggered()), SLOT(onAction()));
    connect(m_actions.jumpToLineAction2, SIGNAL(triggered()), SLOT(onAction()));
    connect(m_actions.returnFromFunctionAction, SIGNAL(triggered()), SLOT(onAction()));
    connect(m_actions.watchAction1, SIGNAL(triggered()), SLOT(onAction()));
    connect(m_actions.watchAction2, SIGNAL(triggered()), SLOT(onAction()));
    connect(m_actions.snapshotAction, SIGNAL(triggered()), SLOT(onAction()));
    connect(m_actions.frameDownAction, SIGNAL(triggered()), SLOT(onAction()));
    connect(m_actions.frameUpAction, SIGNAL(triggered()), SLOT(onAction()));
    connect(m_actions.stopAction, SIGNAL(triggered()), SLOT(onAction()));
    connect(m_actions.interruptAction, SIGNAL(triggered()), SLOT(interruptDebuggingRequest()));
    connect(m_actions.resetAction, SIGNAL(triggered()), SLOT(onAction()));
    connect(&m_statusTimer, SIGNAL(timeout()), SLOT(clearStatusMessage()));

    connect(theDebuggerAction(ExecuteCommand), SIGNAL(triggered()),
        SLOT(executeDebuggerCommand()));

    connect(theDebuggerAction(OperateByInstruction), SIGNAL(triggered()),
        SLOT(onAction()));

    m_plugin->readSettings();

    // Cpp/Qml ui setup
    m_uiSwitcher = new DebuggerUISwitcher(m_debugMode, this);
    ExtensionSystem::PluginManager::instance()->addObject(m_uiSwitcher);
    m_uiSwitcher->addLanguage(CppLanguage, tr("C++"), cppDebuggercontext);
    m_uiSwitcher->addLanguage(QmlLanguage, tr("QML/JavaScript"), qmlDebuggerContext);

    // Dock widgets
    m_breakDock = m_uiSwitcher->createDockWidget(CppLanguage, m_breakWindow);
    m_breakDock->setObjectName(QString(DOCKWIDGET_BREAK));
    m_modulesDock = m_uiSwitcher->createDockWidget(CppLanguage, m_modulesWindow,
                                                    Qt::TopDockWidgetArea);
    m_modulesDock->setObjectName(QString(DOCKWIDGET_MODULES));
    connect(m_modulesDock->toggleViewAction(), SIGNAL(toggled(bool)),
        SLOT(modulesDockToggled(bool)), Qt::QueuedConnection);

    m_registerDock = m_uiSwitcher->createDockWidget(CppLanguage, m_registerWindow,
        Qt::TopDockWidgetArea);
    m_registerDock->setObjectName(QString(DOCKWIDGET_REGISTER));
    connect(m_registerDock->toggleViewAction(), SIGNAL(toggled(bool)),
        SLOT(registerDockToggled(bool)), Qt::QueuedConnection);

    m_outputDock = m_uiSwitcher->createDockWidget(AnyLanguage, m_outputWindow,
        Qt::TopDockWidgetArea);
    m_outputDock->setObjectName(QString(DOCKWIDGET_OUTPUT));
    m_snapshotDock = m_uiSwitcher->createDockWidget(CppLanguage, m_snapshotWindow);
    m_snapshotDock->setObjectName(QString(DOCKWIDGET_SNAPSHOTS));

    m_stackDock = m_uiSwitcher->createDockWidget(CppLanguage, m_stackWindow);
    m_stackDock->setObjectName(QString(DOCKWIDGET_STACK));

    m_sourceFilesDock = m_uiSwitcher->createDockWidget(CppLanguage,
        m_sourceFilesWindow, Qt::TopDockWidgetArea);
    m_sourceFilesDock->setObjectName(QString(DOCKWIDGET_SOURCE_FILES));
    connect(m_sourceFilesDock->toggleViewAction(), SIGNAL(toggled(bool)),
        SLOT(sourceFilesDockToggled(bool)), Qt::QueuedConnection);

    m_threadsDock = m_uiSwitcher->createDockWidget(CppLanguage, m_threadsWindow);
    m_threadsDock->setObjectName(QString(DOCKWIDGET_THREADS));

    QSplitter *localsAndWatchers = new Core::MiniSplitter(Qt::Vertical);
    localsAndWatchers->setObjectName(QLatin1String("CppDebugLocalsAndWatchers"));
    localsAndWatchers->setWindowTitle(m_localsWindow->windowTitle());
    localsAndWatchers->addWidget(m_localsWindow);
    localsAndWatchers->addWidget(m_returnWindow);
    localsAndWatchers->addWidget(m_watchersWindow);
    localsAndWatchers->setStretchFactor(0, 3);
    localsAndWatchers->setStretchFactor(1, 1);
    localsAndWatchers->setStretchFactor(2, 1);

    m_watchDock = m_uiSwitcher->createDockWidget(CppLanguage, localsAndWatchers);
    m_watchDock->setObjectName(QString(DOCKWIDGET_WATCHERS));

    m_scriptConsoleDock =
        m_uiSwitcher->createDockWidget(QmlLanguage, m_scriptConsoleWindow);
    m_scriptConsoleDock->setObjectName(QString(DOCKWIDGET_QML_SCRIPTCONSOLE));

    // Do not fail the whole plugin if something goes wrong here.
    uint cmdLineEnabledEngines = AllEngineTypes;
    if (!parseArguments(arguments, &m_attachRemoteParameters,
            &cmdLineEnabledEngines, errorMessage)) {
        *errorMessage = tr("Error evaluating command line arguments: %1")
            .arg(*errorMessage);
        qWarning("%s\n", qPrintable(*errorMessage));
        errorMessage->clear();
    }

    // Register factory of DebuggerRunControl.
    m_debuggerRunControlFactory = new DebuggerRunControlFactory
        (m_plugin, DebuggerEngineType(cmdLineEnabledEngines));
    m_plugin->addAutoReleasedObject(m_debuggerRunControlFactory);

    m_debugMode->setContext(
        Core::Context(CC::C_EDITORMANAGER, C_DEBUGMODE, CC::C_NAVIGATION_PANE));

    m_reverseToolButton = 0;

    // Handling of external applications.
    m_startExternalAction = new QAction(this);
    m_startExternalAction->setText(tr("Start and Debug External Application..."));
    connect(m_startExternalAction, SIGNAL(triggered()),
       SLOT(startExternalApplication()));

    m_attachExternalAction = new QAction(this);
    m_attachExternalAction->setText(tr("Attach to Running External Application..."));
    connect(m_attachExternalAction, SIGNAL(triggered()),
        SLOT(attachExternalApplication()));

    m_attachCoreAction = new QAction(this);
    m_attachCoreAction->setText(tr("Attach to Core..."));
    connect(m_attachCoreAction, SIGNAL(triggered()), this, SLOT(attachCore()));

    m_attachTcfAction = new QAction(this);
    m_attachTcfAction->setText(tr("Attach to Running Tcf Agent..."));
    m_attachTcfAction->setToolTip(tr("This attaches to a running "
        "'Target Communication Framework' agent."));
    connect(m_attachTcfAction, SIGNAL(triggered()), SLOT(attachRemoteTcf()));

    m_startRemoteAction = new QAction(this);
    m_startRemoteAction->setText(tr("Start and Attach to Remote Application..."));
    connect(m_startRemoteAction, SIGNAL(triggered()),
        SLOT(startRemoteApplication()));

    m_detachAction = new QAction(this);
    m_detachAction->setText(tr("Detach Debugger"));
    m_detachAction->setProperty(Role, RequestExecDetachRole);
    connect(m_detachAction, SIGNAL(triggered()), SLOT(onAction()));


    Core::Command *cmd = 0;

    Core::ActionContainer *mstart =
        am->actionContainer(PE::M_DEBUG_STARTDEBUGGING);

    cmd = am->registerAction(m_actions.continueAction,
        PE::DEBUG, m_continuableContext);
    mstart->addAction(cmd, CC::G_DEFAULT_ONE);

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

    cmd = am->registerAction(m_attachTcfAction,
        Constants::ATTACHTCF, globalcontext);
    mstart->addAction(cmd, Core::Constants::G_DEFAULT_ONE);

    cmd = am->registerAction(m_startRemoteAction,
        Constants::ATTACHREMOTE, globalcontext);
    cmd->setAttribute(Command::CA_Hide);
    mstart->addAction(cmd, CC::G_DEFAULT_ONE);

    cmd = am->registerAction(m_detachAction,
        Constants::DETACH, globalcontext);
    cmd->setAttribute(Command::CA_Hide);
    m_uiSwitcher->addMenuAction(cmd, AnyLanguage, CC::G_DEFAULT_ONE);

    cmd = am->registerAction(m_actions.stopAction,
        Constants::STOP, globalcontext);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setAttribute(Command::CA_UpdateIcon);
    //cmd->setDefaultKeySequence(QKeySequence(Constants::STOP_KEY));
    cmd->setDefaultText(tr("Stop Debugger"));
    m_uiSwitcher->addMenuAction(cmd, AnyLanguage, CC::G_DEFAULT_ONE);

    cmd = am->registerAction(m_actions.interruptAction,
        PE::DEBUG, m_interruptibleContext);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setAttribute(Command::CA_UpdateIcon);
    cmd->setDefaultKeySequence(QKeySequence(Constants::STOP_KEY));
    cmd->setDefaultText(tr("Interrupt Debugger"));

    cmd = am->registerAction(m_actions.resetAction,
        Constants::RESET, globalcontext);
    cmd->setAttribute(Core::Command::CA_UpdateText);
    //cmd->setDefaultKeySequence(QKeySequence(Constants::RESET_KEY));
    cmd->setDefaultText(tr("Reset Debugger"));
    m_uiSwitcher->addMenuAction(cmd, AnyLanguage, CC::G_DEFAULT_ONE);

    QAction *sep = new QAction(this);
    sep->setSeparator(true);
    cmd = am->registerAction(sep, _("Debugger.Sep.Step"), globalcontext);
    m_uiSwitcher->addMenuAction(cmd, CppLanguage);

    cmd = am->registerAction(m_actions.nextAction,
        Constants::NEXT, cppDebuggercontext);
    cmd->setDefaultKeySequence(QKeySequence(Constants::NEXT_KEY));
    cmd->setAttribute(Command::CA_Hide);
    m_uiSwitcher->addMenuAction(cmd, CppLanguage);

    cmd = am->registerAction(m_actions.stepAction,
        Constants::STEP, cppDebuggercontext);
    cmd->setDefaultKeySequence(QKeySequence(Constants::STEP_KEY));
    cmd->setAttribute(Command::CA_Hide);
    m_uiSwitcher->addMenuAction(cmd, CppLanguage);


    cmd = am->registerAction(m_actions.stepOutAction,
        Constants::STEPOUT, cppDebuggercontext);
    cmd->setDefaultKeySequence(QKeySequence(Constants::STEPOUT_KEY));
    cmd->setAttribute(Command::CA_Hide);
    m_uiSwitcher->addMenuAction(cmd, CppLanguage);


    cmd = am->registerAction(m_actions.runToLineAction1,
        Constants::RUN_TO_LINE1, cppDebuggercontext);
    cmd->setDefaultKeySequence(QKeySequence(Constants::RUN_TO_LINE_KEY));
    cmd->setAttribute(Command::CA_Hide);
    m_uiSwitcher->addMenuAction(cmd, CppLanguage);


    cmd = am->registerAction(m_actions.runToFunctionAction,
        Constants::RUN_TO_FUNCTION, cppDebuggercontext);
    cmd->setDefaultKeySequence(QKeySequence(Constants::RUN_TO_FUNCTION_KEY));
    cmd->setAttribute(Command::CA_Hide);
    m_uiSwitcher->addMenuAction(cmd, CppLanguage);


    cmd = am->registerAction(m_actions.jumpToLineAction1,
        Constants::JUMP_TO_LINE1, cppDebuggercontext);
    cmd->setAttribute(Command::CA_Hide);
    m_uiSwitcher->addMenuAction(cmd, CppLanguage);


    cmd = am->registerAction(m_actions.returnFromFunctionAction,
        Constants::RETURN_FROM_FUNCTION, cppDebuggercontext);
    cmd->setAttribute(Command::CA_Hide);
    m_uiSwitcher->addMenuAction(cmd, CppLanguage);


    cmd = am->registerAction(m_actions.reverseDirectionAction,
        Constants::REVERSE, cppDebuggercontext);
    cmd->setDefaultKeySequence(QKeySequence(Constants::REVERSE_KEY));
    cmd->setAttribute(Command::CA_Hide);
    m_uiSwitcher->addMenuAction(cmd, CppLanguage);


    sep = new QAction(this);
    sep->setSeparator(true);
    cmd = am->registerAction(sep, _("Debugger.Sep.Break"), globalcontext);
    m_uiSwitcher->addMenuAction(cmd, CppLanguage);


    cmd = am->registerAction(m_actions.snapshotAction,
        Constants::SNAPSHOT, cppDebuggercontext);
    cmd->setDefaultKeySequence(QKeySequence(Constants::SNAPSHOT_KEY));
    cmd->setAttribute(Command::CA_Hide);
    m_uiSwitcher->addMenuAction(cmd, CppLanguage);

    cmd = am->registerAction(m_actions.frameDownAction,
        Constants::FRAME_DOWN, cppDebuggercontext);
    cmd = am->registerAction(m_actions.frameUpAction,
        Constants::FRAME_UP, cppDebuggercontext);


    cmd = am->registerAction(theDebuggerAction(OperateByInstruction),
        Constants::OPERATE_BY_INSTRUCTION, cppDebuggercontext);
    cmd->setAttribute(Command::CA_Hide);
    m_uiSwitcher->addMenuAction(cmd, CppLanguage);


    cmd = am->registerAction(m_actions.breakAction,
        Constants::TOGGLE_BREAK, globalcontext);
    cmd->setDefaultKeySequence(QKeySequence(Constants::TOGGLE_BREAK_KEY));
    m_uiSwitcher->addMenuAction(cmd, CppLanguage);
    connect(m_actions.breakAction, SIGNAL(triggered()),
        this, SLOT(toggleBreakpoint()));

    //mcppcontext->addAction(cmd);

    sep = new QAction(this);
    sep->setSeparator(true);
    cmd = am->registerAction(sep, _("Debugger.Sep.Watch"), globalcontext);
    m_uiSwitcher->addMenuAction(cmd, CppLanguage);


    cmd = am->registerAction(m_actions.watchAction1,
        Constants::ADD_TO_WATCH1, cppeditorcontext);
    cmd->action()->setEnabled(true);
    //cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+D,Ctrl+W")));
    m_uiSwitcher->addMenuAction(cmd, CppLanguage);


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

    cmd = am->registerAction(m_actions.runToLineAction2,
        Constants::RUN_TO_LINE2, cppDebuggercontext);
    cmd->action()->setEnabled(true);
    editorContextMenu->addAction(cmd);
    cmd->setAttribute(Command::CA_Hide);

    cmd = am->registerAction(m_actions.jumpToLineAction2,
        Constants::JUMP_TO_LINE2, cppDebuggercontext);
    cmd->action()->setEnabled(true);
    editorContextMenu->addAction(cmd);
    cmd->setAttribute(Command::CA_Hide);

    m_plugin->addAutoReleasedObject(new CommonOptionsPage);
    QList<Core::IOptionsPage *> engineOptionPages;
    if (cmdLineEnabledEngines & GdbEngineType)
        addGdbOptionPages(&engineOptionPages);
#ifdef CDB_ENABLED
    if (cmdLineEnabledEngines & CdbEngineType)
        addCdbOptionPages(&engineOptionPages);
#endif
    //if (cmdLineEnabledEngines & ScriptEngineType)
    //    addScriptOptionPages(&engineOptionPages);
    //if (cmdLineEnabledEngines & TcfEngineType)
    //    addTcfOptionPages(&engineOptionPages);
    foreach (Core::IOptionsPage *op, engineOptionPages)
        m_plugin->addAutoReleasedObject(op);
    m_plugin->addAutoReleasedObject(new DebuggingHelperOptionPage);

    m_locationMark = 0;

    //setSimpleDockWidgetArrangement(Lang_Cpp);

    connect(ModeManager::instance(), SIGNAL(currentModeChanged(Core::IMode*)),
            this, SLOT(onModeChanged(Core::IMode*)));
    m_debugMode->widget()->setFocusProxy(EditorManager::instance());
    m_plugin->addObject(m_debugMode);


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

    // EditorManager
    QObject *editorManager = core->editorManager();
    connect(editorManager, SIGNAL(editorAboutToClose(Core::IEditor*)),
        SLOT(editorAboutToClose(Core::IEditor*)));
    connect(editorManager, SIGNAL(editorOpened(Core::IEditor*)),
        SLOT(editorOpened(Core::IEditor*)));

    // Application interaction
    connect(theDebuggerAction(SettingsDialog), SIGNAL(triggered()),
        SLOT(showSettingsDialog()));

    // Toolbar
    QWidget *toolbarContainer = new QWidget;

    QHBoxLayout *hbox = new QHBoxLayout(toolbarContainer);
    hbox->setMargin(0);
    hbox->setSpacing(0);
    hbox->addWidget(toolButton(am->command(PE::DEBUG)->action()));
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
    connect(m_threadBox, SIGNAL(activated(int)),
        m_threadsWindow, SLOT(selectThread(int)));

    hbox->addWidget(m_threadBox);
    hbox->addSpacerItem(new QSpacerItem(4, 0));
    hbox->addWidget(m_statusLabel, 10);

    m_uiSwitcher->setToolbar(CppLanguage, toolbarContainer);
    connect(m_uiSwitcher,
        SIGNAL(dockResetRequested(Debugger::DebuggerLanguages)),
        SLOT(setSimpleDockWidgetArrangement(Debugger::DebuggerLanguages)));

    connect(theDebuggerAction(EnableReverseDebugging),
        SIGNAL(valueChanged(QVariant)),
        SLOT(enableReverseDebuggingTriggered(QVariant)));

    // UI Switcher
    connect(m_uiSwitcher,
        SIGNAL(activeLanguagesChanged(Debugger::DebuggerLanguages)),
        SLOT(languagesChanged(Debugger::DebuggerLanguages)));

    setInitialState();
    connectEngine(m_sessionEngine, false);

    connect(sessionManager(),
        SIGNAL(startupProjectChanged(ProjectExplorer::Project*)),
        SLOT(onCurrentProjectChanged(ProjectExplorer::Project*)));

    return true;
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
        if (DebuggerRunControl *runControl = m_snapshotHandler->at(i)) {
            RunConfiguration *rc = runControl->runConfiguration();
            if (rc == activeRc) {
                m_snapshotHandler->setCurrentIndex(i);
                DebuggerEngine *engine = runControl->engine();
                updateState(engine);
                return;
            }
        }
    }
    // No corresponding debugger found. So we are ready to start one.
    ICore *core = ICore::instance();
    core->updateAdditionalContexts(m_continuableContext, Core::Context());
    core->updateAdditionalContexts(m_interruptibleContext, Core::Context());
}

void DebuggerPluginPrivate::onAction()
{
    QAction *act = qobject_cast<QAction *>(sender());
    QTC_ASSERT(act, return);
    const int role = act->property(Role).toInt();
    notifyCurrentEngine(role);
}

void DebuggerPluginPrivate::languagesChanged(const DebuggerLanguages &languages)
{
    const bool debuggerIsCPP = (languages & CppLanguage);
    //qDebug() << "DEBUGGER IS CPP: " << debuggerIsCPP;

    m_startExternalAction->setVisible(debuggerIsCPP);
    m_attachExternalAction->setVisible(debuggerIsCPP);
    m_attachCoreAction->setVisible(debuggerIsCPP);
    m_startRemoteAction->setVisible(debuggerIsCPP);
    m_detachAction->setVisible(debuggerIsCPP);
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
    sp.breakAtMain = dlg.breakAtMain();
    if (!dlg.executableArguments().isEmpty())
        sp.processArgs = dlg.executableArguments().split(QLatin1Char(' '));
    // Fixme: 1 of 3 testing hacks.
    if (!sp.processArgs.isEmpty()
        && (sp.processArgs.front() == _("@tcf@") || sp.processArgs.front() == _("@sym@")))
        sp.toolChainType = ToolChain::RVCT_ARMV5;

    startDebugger(m_debuggerRunControlFactory->create(sp));
}

void DebuggerPluginPrivate::notifyCurrentEngine(int role, const QVariant &value)
{
    QTC_ASSERT(m_commandWindow && m_commandWindow->model(), return);
    m_commandWindow->model()->setData(QModelIndex(), value, role);
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
    DebuggerRunControl *rc = createDebugger(sp);
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
    DebuggerRunControl *rc = createDebugger(sp);
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
    DebuggerRunControl *rc = createDebugger(sp);
    startDebugger(rc);
}

void DebuggerPluginPrivate::startRemoteApplication()
{
    DebuggerStartParameters sp;
    StartRemoteDialog dlg(mainWindow());
    QStringList arches;
    arches.append(_("i386:x86-64:intel"));
    arches.append(_("i386"));
    QString lastUsed = configValue(_("LastRemoteArchitecture")).toString();
    if (!arches.contains(lastUsed))
        arches.prepend(lastUsed);
    dlg.setRemoteArchitectures(arches);
    dlg.setRemoteChannel(
            configValue(_("LastRemoteChannel")).toString());
    dlg.setLocalExecutable(
            configValue(_("LastLocalExecutable")).toString());
    dlg.setDebugger(configValue(_("LastDebugger")).toString());
    dlg.setRemoteArchitecture(lastUsed);
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
    setConfigValue(_("LastServerStartScript"), dlg.serverStartScript());
    setConfigValue(_("LastUseServerStartScript"), dlg.useServerStartScript());
    setConfigValue(_("LastSysroot"), dlg.sysRoot());
    sp.remoteChannel = dlg.remoteChannel();
    sp.remoteArchitecture = dlg.remoteArchitecture();
    sp.executable = dlg.localExecutable();
    sp.displayName = dlg.localExecutable();
    sp.debuggerCommand = dlg.debugger(); // Override toolchain-detection.
    if (!sp.debuggerCommand.isEmpty())
        sp.toolChainType = ToolChain::INVALID;
    sp.startMode = AttachToRemote;
    sp.useServerStartScript = dlg.useServerStartScript();
    sp.serverStartScript = dlg.serverStartScript();
    sp.sysRoot = dlg.sysRoot();
    startDebugger(createDebugger(sp));
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
    startDebugger(createDebugger(sp));
}

void DebuggerPluginPrivate::attachCmdLine()
{
    if (m_attachRemoteParameters.attachPid) {
        showStatusMessage(tr("Attaching to PID %1.")
            .arg(m_attachRemoteParameters.attachPid));
        const QString crashParameter = m_attachRemoteParameters.winCrashEvent
            ? QString::number(m_attachRemoteParameters.winCrashEvent) : QString();
        attachExternalApplication(m_attachRemoteParameters.attachPid,
            QString(), crashParameter);
        return;
    }
    const QString target = m_attachRemoteParameters.attachTarget;
    if (!target.isEmpty()) {
        if (target.indexOf(':') > 0) {
            showStatusMessage(tr("Attaching to remote server %1.").arg(target));
            attachRemote(target);
        } else {
            showStatusMessage(tr("Attaching to core %1.").arg(target));
            attachCore(target, QString());
        }
    }
}

void DebuggerPluginPrivate::editorOpened(Core::IEditor *editor)
{
    if (!isDebuggable(editor))
        return;
    ITextEditor *textEditor = qobject_cast<ITextEditor *>(editor);
    if (!textEditor)
        return;
    connect(textEditor, SIGNAL(markRequested(TextEditor::ITextEditor*,int)),
        this, SLOT(requestMark(TextEditor::ITextEditor*,int)));
    connect(editor, SIGNAL(tooltipRequested(TextEditor::ITextEditor*,QPoint,int)),
        this, SLOT(showToolTip(TextEditor::ITextEditor*,QPoint,int)));
    connect(textEditor,
        SIGNAL(markContextMenuRequested(TextEditor::ITextEditor*,int,QMenu*)),
        this, SLOT(requestContextMenu(TextEditor::ITextEditor*,int,QMenu*)));
}

void DebuggerPluginPrivate::editorAboutToClose(Core::IEditor *editor)
{
    if (!isDebuggable(editor))
        return;
    ITextEditor *textEditor = qobject_cast<ITextEditor *>(editor);
    if (!textEditor)
        return;
    disconnect(textEditor, SIGNAL(markRequested(TextEditor::ITextEditor*,int)),
        this, SLOT(requestMark(TextEditor::ITextEditor*,int)));
    disconnect(editor, SIGNAL(tooltipRequested(TextEditor::ITextEditor*,QPoint,int)),
        this, SLOT(showToolTip(TextEditor::ITextEditor*,QPoint,int)));
    disconnect(textEditor, SIGNAL(markContextMenuRequested(TextEditor::ITextEditor*,int,QMenu*)),
        this, SLOT(requestContextMenu(TextEditor::ITextEditor*,int,QMenu*)));
}

void DebuggerPluginPrivate::requestContextMenu(TextEditor::ITextEditor *editor,
    int lineNumber, QMenu *menu)
{
    if (!isDebuggable(editor))
        return;

    QList<QVariant> list;
    list.append(quint64(editor));
    list.append(lineNumber);
    list.append(quint64(menu));
    notifyCurrentEngine(RequestContextMenuRole, list);
}

void DebuggerPluginPrivate::toggleBreakpoint()
{
    ITextEditor *textEditor = currentTextEditor();
    QTC_ASSERT(textEditor, return);
    int lineNumber = textEditor->currentLine();
    if (lineNumber >= 0)
        toggleBreakpoint(textEditor->file()->fileName(), lineNumber);
}

void DebuggerPluginPrivate::toggleBreakpoint(const QString &fileName, int lineNumber)
{
    QList<QVariant> list;
    list.append(fileName);
    list.append(lineNumber);
    notifyCurrentEngine(RequestToggleBreakpointRole, list);
}

void DebuggerPluginPrivate::requestMark(ITextEditor *editor, int lineNumber)
{
    if (isDebuggable(editor) && editor && editor->file())
        toggleBreakpoint(editor->file()->fileName(), lineNumber);
}

void DebuggerPluginPrivate::showToolTip(ITextEditor *editor, const QPoint &point, int pos)
{
    if (!isDebuggable(editor))
        return;
    if (!theDebuggerBoolSetting(UseToolTipsInMainEditor))
        return;
    if (state() == DebuggerNotReady)
        return;

    QList<QVariant> list;
    list.append(point);
    list.append(quint64(editor));
    list.append(pos);
    notifyCurrentEngine(RequestToolTipByExpressionRole, list);
}

DebuggerRunControl *
DebuggerPluginPrivate::createDebugger(const DebuggerStartParameters &sp,
    RunConfiguration *rc)
{
    return m_debuggerRunControlFactory->create(sp, rc);
}

void DebuggerPluginPrivate::displayDebugger(DebuggerEngine *engine, bool updateEngine)
{
    QTC_ASSERT(engine, return);
    disconnectEngine();
    connectEngine(engine);
    if (updateEngine)
        engine->updateAll();
    updateState(engine);
}

void DebuggerPluginPrivate::startDebugger(RunControl *rc)
{
    QTC_ASSERT(rc, return);
    ProjectExplorerPlugin::instance()->startRunControl(rc, PE::DEBUGMODE);
}

void DebuggerPluginPrivate::connectEngine(DebuggerEngine *engine, bool notify)
{
    const QAbstractItemModel *oldCommandModel = m_commandWindow->model();
    if (oldCommandModel == engine->commandModel()) {
        // qDebug("RECONNECTING ENGINE %s", qPrintable(engine->objectName()));
        return;
    }

    if (notify)
        notifyCurrentEngine(RequestActivationRole, false);

    // qDebug("CONNECTING ENGINE %s (OLD ENGINE: %s)", qPrintable(engine->objectName()),
    //       (oldCommandModel ? qPrintable(oldCommandModel->objectName()) : ""));

    m_breakWindow->setModel(engine->breakModel());
    m_commandWindow->setModel(engine->commandModel());
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
    m_capabilities = engine->debuggerCapabilities();
    if (notify)
        notifyCurrentEngine(RequestActivationRole, true);
}

static void changeFontSize(QWidget *widget, int size)
{
    QFont font = widget->font();
    font.setPointSize(size);
    widget->setFont(font);
}

void DebuggerPluginPrivate::fontSettingsChanged
    (const TextEditor::FontSettings &settings)
{
    int size = settings.fontZoom() * settings.fontSize() / 100;
    changeFontSize(m_breakWindow, size);
    changeFontSize(m_localsWindow, size);
    changeFontSize(m_modulesWindow, size);
    changeFontSize(m_outputWindow, size);
    changeFontSize(m_registerWindow, size);
    changeFontSize(m_returnWindow, size);
    changeFontSize(m_sourceFilesWindow, size);
    changeFontSize(m_stackWindow, size);
    changeFontSize(m_threadsWindow, size);
    changeFontSize(m_watchersWindow, size);
}

void DebuggerPluginPrivate::cleanupViews()
{
    resetLocation();
    m_actions.reverseDirectionAction->setChecked(false);
    m_actions.reverseDirectionAction->setEnabled(false);
    hideDebuggerToolTip();

    // FIXME ABC: Delete run control / engine?
    //if (d->m_engine)
    //    d->m_engine->cleanup();

    if (EditorManager *editorManager = EditorManager::instance()) {
        QList<IEditor *> toClose;
        foreach (IEditor *editor, editorManager->openedEditors())
            if (editor->property("OpenedByDebugger").toBool())
                toClose.append(editor);
        editorManager->closeEditors(toClose);
    }
}

void DebuggerPluginPrivate::setBusyCursor(bool busy)
{
    //STATE_DEBUG("BUSY FROM: " << m_busy << " TO: " << busy);
    if (busy == m_busy)
        return;
    m_busy = busy;
    QCursor cursor(busy ? Qt::BusyCursor : Qt::ArrowCursor);
    m_breakWindow->setCursor(cursor);
    m_localsWindow->setCursor(cursor);
    m_modulesWindow->setCursor(cursor);
    m_outputWindow->setCursor(cursor);
    m_registerWindow->setCursor(cursor);
    m_returnWindow->setCursor(cursor);
    m_sourceFilesWindow->setCursor(cursor);
    m_stackWindow->setCursor(cursor);
    m_threadsWindow->setCursor(cursor);
    m_watchersWindow->setCursor(cursor);
    m_snapshotWindow->setCursor(cursor);
    m_scriptConsoleWindow->setCursor(cursor);
}

void DebuggerPluginPrivate::setSimpleDockWidgetArrangement
    (const DebuggerLanguages &activeLanguages)
{
    Debugger::DebuggerUISwitcher *uiSwitcher = DebuggerUISwitcher::instance();
    DebuggerMainWindow *mw = mainWindow();
    mw->setTrackingEnabled(false);

    QList<QDockWidget *> dockWidgets = mw->dockWidgets();
    foreach (QDockWidget *dockWidget, dockWidgets) {
        dockWidget->setFloating(false);
        mw->removeDockWidget(dockWidget);
    }

    foreach (QDockWidget *dockWidget, dockWidgets) {
        if (dockWidget == m_outputDock) {
            mw->addDockWidget(Qt::TopDockWidgetArea, dockWidget);
        } else {
            mw->addDockWidget(Qt::BottomDockWidgetArea, dockWidget);
        }
        dockWidget->hide();
    }

    if ((activeLanguages.testFlag(CppLanguage)
                && !activeLanguages.testFlag(QmlLanguage))
            || activeLanguages == AnyLanguage
            || !uiSwitcher->qmlInspectorWindow()) {
        m_stackDock->show();
        m_breakDock->show();
        m_watchDock->show();
        m_threadsDock->show();
        m_snapshotDock->show();
    } else {
        m_stackDock->show();
        m_breakDock->show();
        m_watchDock->show();
        m_scriptConsoleDock->show();
        uiSwitcher->qmlInspectorWindow()->show();
    }
    mw->splitDockWidget(mw->toolBarDockWidget(), m_stackDock, Qt::Vertical);
    mw->splitDockWidget(m_stackDock, m_watchDock, Qt::Horizontal);
    mw->tabifyDockWidget(m_watchDock, m_breakDock);
    mw->tabifyDockWidget(m_watchDock, m_modulesDock);
    mw->tabifyDockWidget(m_watchDock, m_registerDock);
    mw->tabifyDockWidget(m_watchDock, m_threadsDock);
    mw->tabifyDockWidget(m_watchDock, m_sourceFilesDock);
    mw->tabifyDockWidget(m_watchDock, m_snapshotDock);
    mw->tabifyDockWidget(m_watchDock, m_scriptConsoleDock);
    if (uiSwitcher->qmlInspectorWindow())
        mw->tabifyDockWidget(m_watchDock, uiSwitcher->qmlInspectorWindow());

    mw->setTrackingEnabled(true);
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
    m_actions.snapshotAction->setEnabled(false);
    theDebuggerAction(OperateByInstruction)->setEnabled(false);

    m_actions.stopAction->setEnabled(false);
    m_actions.resetAction->setEnabled(false);

    m_actions.stepAction->setEnabled(false);
    m_actions.stepOutAction->setEnabled(false);
    m_actions.runToLineAction1->setEnabled(false);
    m_actions.runToLineAction2->setEnabled(false);
    m_actions.runToFunctionAction->setEnabled(false);
    m_actions.returnFromFunctionAction->setEnabled(false);
    m_actions.jumpToLineAction1->setEnabled(false);
    m_actions.jumpToLineAction2->setEnabled(false);
    m_actions.nextAction->setEnabled(false);

    theDebuggerAction(RecheckDebuggingHelpers)->setEnabled(false);
    theDebuggerAction(AutoDerefPointers)->setEnabled(true);
    theDebuggerAction(ExpandStack)->setEnabled(false);
    theDebuggerAction(ExecuteCommand)->setEnabled(m_state == InferiorStopOk);

    //emit m_plugin->stateChanged(m_state);
}

void DebuggerPluginPrivate::updateState(DebuggerEngine *engine)
{
    QTC_ASSERT(engine != 0 && m_watchersWindow->model() != 0 && m_returnWindow->model() != 0, return);
    m_threadBox->setCurrentIndex(engine->threadsHandler()->currentThread());

    m_watchersWindow->setVisible(
        m_watchersWindow->model()->rowCount(QModelIndex()) > 0);
    m_returnWindow->setVisible(
        m_returnWindow->model()->rowCount(QModelIndex()) > 0);

    if (m_state == engine->state())
        return;

    m_state = engine->state();
    bool actionsEnabled = DebuggerEngine::debuggerActionsEnabled(m_state);

    //if (m_state == InferiorStopOk)
    //    resetLocation();
    //qDebug() << "PLUGIN SET STATE: " << m_state;

    if (m_state == DebuggerNotReady) {
        setBusyCursor(false);
        cleanupViews();
    }

    const bool isContinuable = (engine->state() == InferiorStopOk);
    const bool isInterruptible = (engine->state() == InferiorRunOk);
    ICore *core = ICore::instance();

    if (isContinuable) {
        core->updateAdditionalContexts(m_interruptibleContext, m_continuableContext);
    } else if (isInterruptible) {
        core->updateAdditionalContexts(m_continuableContext, m_interruptibleContext);
    } else {
        core->updateAdditionalContexts(m_continuableContext, Core::Context());
        core->updateAdditionalContexts(m_interruptibleContext, Core::Context());
    }

    const bool started = m_state == InferiorRunOk
        || m_state == InferiorRunRequested
        || m_state == InferiorStopRequested
        || m_state == InferiorStopOk;

    const bool starting = m_state == EngineSetupRequested;

    m_startExternalAction->setEnabled(!started && !starting);
    m_attachExternalAction->setEnabled(true);
#ifdef Q_OS_WIN
    m_attachCoreAction->setEnabled(false);
#else
    m_attachCoreAction->setEnabled(!started && !starting);
#endif
    m_startRemoteAction->setEnabled(!started && !starting);

    const bool detachable = m_state == InferiorStopOk
        && engine->startParameters().startMode != AttachCore;
    m_detachAction->setEnabled(detachable);

    const bool stoppable = m_state == InferiorRunOk
        || m_state == InferiorStopOk
        || m_state == InferiorUnrunnable;

    const bool running = m_state == InferiorRunOk;
    const bool stopped = m_state == InferiorStopOk;

    if (stopped)
        QApplication::alert(mainWindow(), 3000);

    m_actions.watchAction1->setEnabled(true);
    m_actions.watchAction2->setEnabled(true);
    m_actions.breakAction->setEnabled(true);
    m_actions.snapshotAction->
        setEnabled(stopped && (m_capabilities & SnapshotCapability));

    theDebuggerAction(OperateByInstruction)->setEnabled(!running);

    m_actions.stopAction->setEnabled(stopped);
    m_actions.interruptAction->setEnabled(stoppable);
    m_actions.resetAction->setEnabled(m_state != DebuggerNotReady);

#if 1
    // This is only needed when we insist on using Shift-F5 for Interrupt.
    // Removing the block makes F5 interrupt when running and continue when stopped.
    Core::ActionManager *am = core->actionManager();
    bool stopIsKill = m_state == InferiorStopOk
        || m_state == InferiorUnrunnable
        || m_state == DebuggerFinished;
    if (stopIsKill) {
        am->command(Constants::STOP)->setKeySequence(QKeySequence(STOP_KEY));
        am->command(PE::DEBUG)->setKeySequence(QKeySequence("F5"));
    } else {
        am->command(Constants::STOP)->setKeySequence(QKeySequence());
        am->command(PE::DEBUG)->setKeySequence(QKeySequence(STOP_KEY));
    }
#endif

    m_actions.stepAction->setEnabled(stopped);
    m_actions.stepOutAction->setEnabled(stopped);
    m_actions.runToLineAction1->setEnabled(stopped);
    m_actions.runToLineAction2->setEnabled(stopped);
    m_actions.runToFunctionAction->setEnabled(stopped);
    m_actions.returnFromFunctionAction->
        setEnabled(stopped && (m_capabilities & ReturnFromFunctionCapability));

    const bool canJump = stopped && (m_capabilities & JumpToLineCapability);
    m_actions.jumpToLineAction1->setEnabled(canJump);
    m_actions.jumpToLineAction2->setEnabled(canJump);

    m_actions.nextAction->setEnabled(stopped);

    theDebuggerAction(RecheckDebuggingHelpers)->setEnabled(actionsEnabled);
    const bool canDeref = actionsEnabled
        && (m_capabilities & AutoDerefPointersCapability);
    theDebuggerAction(AutoDerefPointers)->setEnabled(canDeref);
    theDebuggerAction(ExpandStack)->setEnabled(actionsEnabled);
    theDebuggerAction(ExecuteCommand)->setEnabled(m_state == InferiorStopOk);

    const bool notbusy = m_state == InferiorStopOk
        || m_state == DebuggerNotReady
        || m_state == DebuggerFinished
        || m_state == InferiorUnrunnable;
    setBusyCursor(!notbusy);
}

void DebuggerPluginPrivate::resetLocation()
{
    delete m_locationMark;
    m_locationMark = 0;
}

void DebuggerPluginPrivate::gotoLocation(const QString &file, int line, bool setMarker)
{
    bool newEditor = false;
    ITextEditor *editor =
        BaseTextEditor::openEditorAt(file, line, 0, QString(),
            EditorManager::IgnoreNavigationHistory, &newEditor);
    if (!editor)
        return;
    if (newEditor)
        editor->setProperty("OpenedByDebugger", true);
    if (setMarker) {
        resetLocation();
        m_locationMark = new LocationMark(file, line);
    }
}

void DebuggerPluginPrivate::onModeChanged(IMode *mode)
{
     // FIXME: This one gets always called, even if switching between modes
     //        different then the debugger mode. E.g. Welcome and Help mode and
     //        also on shutdown.

    m_uiSwitcher->onModeChanged(mode);

    if (mode != m_debugMode)
        return;

    EditorManager *editorManager = EditorManager::instance();
    if (editorManager->currentEditor())
        editorManager->currentEditor()->widget()->setFocus();
}

void DebuggerPluginPrivate::showSettingsDialog()
{
    Core::ICore::instance()->showOptionsDialog(
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
    ts << m_outputWindow->inputContents();
    ts << "\n\n=======================================\n\n";
    ts << m_outputWindow->combinedContents();
}

void DebuggerPluginPrivate::clearStatusMessage()
{
    m_statusLabel->setText(m_lastPermanentStatusMessage);
}

/*! Activates the previous mode when the current mode is the debug mode. */
void DebuggerPluginPrivate::activatePreviousMode()
{
    Core::ModeManager *modeManager = ICore::instance()->modeManager();

    if (modeManager->currentMode() == modeManager->mode(MODE_DEBUG)
            && !m_previousMode.isEmpty()) {
        modeManager->activateMode(m_previousMode);
        m_previousMode.clear();
    }
}

void DebuggerPluginPrivate::activateDebugMode()
{
    const bool canReverse = (m_capabilities & ReverseSteppingCapability)
                && theDebuggerBoolSetting(EnableReverseDebugging);
    m_actions.reverseDirectionAction->setChecked(false);
    m_actions.reverseDirectionAction->setEnabled(canReverse);
    m_actions.reverseDirectionAction->setEnabled(false);
    ModeManager *modeManager = ModeManager::instance();
    m_previousMode = modeManager->currentMode()->id();
    modeManager->activateMode(_(MODE_DEBUG));
}

void DebuggerPluginPrivate::sessionLoaded()
{
    m_sessionEngine->loadSessionData();
}

void DebuggerPluginPrivate::aboutToUnloadSession()
{
    // Stop debugging the active project when switching sessions.
    // Note that at startup, session switches may occur, which interfer
    // with command-line debugging startup.
    // FIXME ABC: Still wanted? Iterate?
    //if (d->m_engine && state() != DebuggerNotReady
    //    && runControl()->sp().startMode == StartInternal)
    //        d->m_engine->shutdown();
}

void DebuggerPluginPrivate::aboutToSaveSession()
{
    m_sessionEngine->saveSessionData();
}

void DebuggerPluginPrivate::interruptDebuggingRequest()
{
    if (state() == InferiorRunOk)
        notifyCurrentEngine(RequestExecInterruptRole);
    else
        exitDebugger();
}

void DebuggerPluginPrivate::exitDebugger()
{
    // The engine will finally call setState(DebuggerFinished) which
    // in turn will handle the cleanup.
    notifyCurrentEngine(RequestExecExitRole);
    m_codeModelSnapshot = CPlusPlus::Snapshot();
}

void DebuggerPluginPrivate::executeDebuggerCommand()
{
    if (QAction *action = qobject_cast<QAction *>(sender()))
        notifyCurrentEngine(RequestExecuteCommandRole, action->data().toString());
}

void DebuggerPluginPrivate::showStatusMessage(const QString &msg0, int timeout)
{
    m_plugin->showMessage(msg0, LogStatus);
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
    notifyCurrentEngine(RequestExecuteCommandRole, expression);
}

void DebuggerPluginPrivate::openMemoryEditor()
{ 
    AddressDialog dialog;
    if (dialog.exec() != QDialog::Accepted)
        return;
    QTC_ASSERT(m_watchersWindow, return);
    m_watchersWindow->model()->setData(
        QModelIndex(), dialog.address(), RequestShowMemoryRole);
}


///////////////////////////////////////////////////////////////////////
//
// DebuggerPlugin
//
///////////////////////////////////////////////////////////////////////

DebuggerPlugin *theInstance = 0;

DebuggerPlugin *DebuggerPlugin::instance()
{
    return theInstance;
}

DebuggerPlugin::DebuggerPlugin()
{
    d = new DebuggerPluginPrivate(this);
    theInstance = this;
}

DebuggerPlugin::~DebuggerPlugin()
{
    delete d->m_sessionEngine;
    d->m_sessionEngine = 0;

    theInstance = 0;
    delete DebuggerSettings::instance();

    removeObject(d->m_debugMode);

    delete d->m_debugMode;
    d->m_debugMode = 0;

    delete d->m_locationMark;
    d->m_locationMark = 0;

    removeObject(d->m_uiSwitcher);
    delete d->m_uiSwitcher;
    d->m_uiSwitcher = 0;

    delete d->m_commandWindow;
    d->m_commandWindow = 0;

    delete d->m_snapshotHandler;
    d->m_snapshotHandler = 0;

    delete d;
}

bool DebuggerPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    return d->initialize(arguments, errorMessage);
}

void DebuggerPlugin::setSessionValue(const QString &name, const QVariant &value)
{
    QTC_ASSERT(sessionManager(), return);
    sessionManager()->setValue(name, value);
    //qDebug() << "SET SESSION VALUE: " << name;
}

QVariant DebuggerPlugin::sessionValue(const QString &name)
{
    QTC_ASSERT(sessionManager(), return QVariant());
    //qDebug() << "GET SESSION VALUE: " << name;
    return sessionManager()->value(name);
}

void DebuggerPlugin::setConfigValue(const QString &name, const QVariant &value)
{
    QTC_ASSERT(d->m_debugMode, return);
    settings()->setValue(name, value);
}

QVariant DebuggerPlugin::configValue(const QString &name) const
{
    QTC_ASSERT(d->m_debugMode, return QVariant());
    return settings()->value(name);
}

void DebuggerPlugin::resetLocation()
{
    d->resetLocation();
    //qDebug() << "RESET_LOCATION: current:"  << currentTextEditor();
    //qDebug() << "RESET_LOCATION: locations:"  << m_locationMark;
    //qDebug() << "RESET_LOCATION: stored:"  << m_locationMark->editor();
    delete d->m_locationMark;
    d->m_locationMark = 0;
}

void DebuggerPlugin::gotoLocation(const QString &file, int line, bool setMarker)
{
    bool newEditor = false;
    ITextEditor *editor =
        BaseTextEditor::openEditorAt(file, line, 0, QString(),
            EditorManager::IgnoreNavigationHistory,
            &newEditor);
    if (!editor)
        return;
    if (newEditor)
        editor->setProperty("OpenedByDebugger", true);
    if (setMarker) {
        resetLocation();
        d->m_locationMark = new LocationMark(file, line);
    }
}

void DebuggerPlugin::openTextEditor(const QString &titlePattern0,
    const QString &contents)
{
    QString titlePattern = titlePattern0;
    EditorManager *editorManager = EditorManager::instance();
    QTC_ASSERT(editorManager, return);
    IEditor *editor = editorManager->openEditorWithContents(
        Core::Constants::K_DEFAULT_TEXT_EDITOR_ID, &titlePattern, contents);
    QTC_ASSERT(editor, return);
    editorManager->activateEditor(editor, EditorManager::IgnoreNavigationHistory);
}

void DebuggerPlugin::writeSettings() const
{
    QSettings *s = settings();
    DebuggerSettings::instance()->writeSettings(s);
}

void DebuggerPlugin::readSettings()
{
    //qDebug() << "PLUGIN READ SETTINGS";
    QSettings *s = settings();
    DebuggerSettings::instance()->readSettings(s);
}

const CPlusPlus::Snapshot &DebuggerPlugin::cppCodeModelSnapshot() const
{
    if (d->m_codeModelSnapshot.isEmpty() && theDebuggerAction(UseCodeModel)->isChecked())
        d->m_codeModelSnapshot = CppTools::CppModelManagerInterface::instance()->snapshot();
    return d->m_codeModelSnapshot;
}

void DebuggerPlugin::clearCppCodeModelSnapshot()
{
    d->m_codeModelSnapshot = CPlusPlus::Snapshot();
}

ExtensionSystem::IPlugin::ShutdownFlag DebuggerPlugin::aboutToShutdown()
{
    disconnect(sessionManager(),
        SIGNAL(startupProjectChanged(ProjectExplorer::Project*)), d, 0);
    writeSettings();
    if (d->m_uiSwitcher)
        d->m_uiSwitcher->aboutToShutdown();
    return SynchronousShutdown;
}

void DebuggerPlugin::showMessage(const QString &msg, int channel, int timeout)
{
    //qDebug() << "PLUGIN OUTPUT: " << channel << msg;
    DebuggerOutputWindow *ow = d->m_outputWindow;
    QTC_ASSERT(ow, return);
    switch (channel) {
        case StatusBar:
            // This will append to ow's output pane, too.
            d->showStatusMessage(msg, timeout);
            break;
        case LogMiscInput:
            ow->showInput(LogMisc, msg);
            ow->showOutput(LogMisc, msg);
            break;
        case LogInput:
            ow->showInput(LogInput, msg);
            ow->showOutput(LogInput, msg);
            break;
        case ScriptConsoleOutput:
            d->m_scriptConsoleWindow->appendResult(msg);
            break;
        default:
            ow->showOutput(channel, msg);
            break;
    }
}


//////////////////////////////////////////////////////////////////////
//
// Register specific stuff
//
//////////////////////////////////////////////////////////////////////

bool DebuggerPlugin::isReverseDebugging() const
{
    return d->m_actions.reverseDirectionAction->isChecked();
}

QMessageBox *DebuggerPlugin::showMessageBox(int icon, const QString &title,
    const QString &text, int buttons)
{
    QMessageBox *mb = new QMessageBox(QMessageBox::Icon(icon),
        title, text, QMessageBox::StandardButtons(buttons), mainWindow());
    mb->setAttribute(Qt::WA_DeleteOnClose);
    mb->show();
    return mb;
}

void DebuggerPlugin::ensureLogVisible()
{
    QAction *action = d->m_outputDock->toggleViewAction();
    if (!action->isChecked())
        action->trigger();
}

QIcon DebuggerPlugin::locationMarkIcon() const
{
    return d->m_locationMarkIcon;
}

void DebuggerPlugin::extensionsInitialized()
{
    d->m_uiSwitcher->initialize();
    d->m_watchersWindow->setVisible(false);
    d->m_returnWindow->setVisible(false);
    connect(d->m_uiSwitcher, SIGNAL(memoryEditorRequested()),
        d, SLOT(openMemoryEditor()));

    // time gdb -i mi -ex 'debuggerplugin.cpp:800' -ex r -ex q bin/qtcreator.bin
    const QByteArray env = qgetenv("QTC_DEBUGGER_TEST");
    //qDebug() << "EXTENSIONS INITIALIZED:" << env;
    // if (!env.isEmpty())
    //    m_plugin->runTest(QString::fromLocal8Bit(env));
    if (d->m_attachRemoteParameters.attachPid
            || !d->m_attachRemoteParameters.attachTarget.isEmpty())
        QTimer::singleShot(0, d, SLOT(attachCmdLine()));
}

QWidget *DebuggerPlugin::mainWindow() const
{
    return d->m_uiSwitcher->mainWindow();
}

DebuggerRunControl *
DebuggerPlugin::createDebugger(const DebuggerStartParameters &sp,
    RunConfiguration *rc)
{
    return instance()->d->createDebugger(sp, rc);
}

void DebuggerPlugin::startDebugger(RunControl *runControl)
{
    instance()->d->startDebugger(runControl);
}

void DebuggerPlugin::displayDebugger(RunControl *runControl)
{
    DebuggerRunControl *rc = qobject_cast<DebuggerRunControl *>(runControl);
    QTC_ASSERT(rc, return);
    instance()->d->displayDebugger(rc->engine());
}

// if updateEngine is set, the engine will update its threads/modules and so forth.
void DebuggerPlugin::displayDebugger(DebuggerEngine *engine, bool updateEngine)
{
    instance()->d->displayDebugger(engine, updateEngine);
}

void DebuggerPlugin::updateState(DebuggerEngine *engine)
{
    d->updateState(engine);
}

void DebuggerPlugin::activateDebugMode()
{
    d->activateDebugMode();
}

void DebuggerPlugin::createNewDock(QWidget *widget)
{
    QDockWidget *dockWidget =
        DebuggerUISwitcher::instance()->createDockWidget(CppLanguage, widget);
    dockWidget->setWindowTitle(widget->windowTitle());
    dockWidget->setObjectName(widget->windowTitle());
    dockWidget->setFeatures(QDockWidget::DockWidgetClosable);
    //dockWidget->setWidget(widget);
    //mainWindow()->addDockWidget(Qt::TopDockWidgetArea, dockWidget);
    dockWidget->show();
}

void DebuggerPlugin::runControlStarted(DebuggerRunControl *runControl)
{
    d->connectEngine(runControl->engine());
    d->m_snapshotHandler->appendSnapshot(runControl);
}

void DebuggerPlugin::runControlFinished(DebuggerRunControl *runControl)
{
    Q_UNUSED(runControl);
    d->m_snapshotHandler->removeSnapshot(runControl);
    d->disconnectEngine();
    if (d->m_snapshotHandler->size() == 0)
        d->activatePreviousMode();
}

DebuggerLanguages DebuggerPlugin::activeLanguages() const
{
    return DebuggerUISwitcher::instance()->activeDebugLanguages();
}

DebuggerEngine *DebuggerPlugin::sessionTemplate()
{
    return d->m_sessionEngine;
}

bool DebuggerPlugin::isRegisterViewVisible() const
{
    return d->m_registerDock->toggleViewAction()->isChecked();
}

bool DebuggerPlugin::hasSnapsnots() const
{
    return d->m_snapshotHandler->size();
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
