/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "debuggerplugin.h"

#include "debuggerconstants.h"
#include "debuggermanager.h"
#include "debuggerrunner.h"
#include "gdboptionpage.h"
#include "gdbengine.h"
#include "mode.h"

#include <coreplugin/actionmanager/actionmanagerinterface.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/uniqueidmanager.h>

#include <cplusplus/ExpressionUnderCursor.h>

#include <cppeditor/cppeditorconstants.h>

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

#include <QtGui/QPlainTextEdit>
#include <QtGui/QTextBlock>
#include <QtGui/QTextCursor>


using namespace Debugger::Internal;
using namespace Debugger::Constants;
using namespace TextEditor;
using namespace Core;
using namespace ProjectExplorer;
using namespace CPlusPlus;


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
const char * const DEBUG_DUMPERS        = "Debugger.DebugDumpers";
const char * const ADD_TO_WATCH         = "Debugger.AddToWatch";
const char * const USE_CUSTOM_DUMPERS   = "Debugger.UseCustomDumpers";
const char * const USE_FAST_START       = "Debugger.UseFastStart";
const char * const USE_TOOL_TIPS        = "Debugger.UseToolTips";
const char * const SKIP_KNOWN_FRAMES    = "Debugger.SkipKnownFrames";
const char * const DUMP_LOG             = "Debugger.DumpLog";

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


///////////////////////////////////////////////////////////////////////
//
// LocationMark
//
///////////////////////////////////////////////////////////////////////

class Debugger::Internal::LocationMark
  : public TextEditor::BaseTextMark
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

///////////////////////////////////////////////////////////////////////
//
// DebuggerPlugin
//
///////////////////////////////////////////////////////////////////////

DebuggerPlugin::DebuggerPlugin()
{
    m_pm = 0;
    m_generalOptionPage = 0;
    m_locationMark = 0;
    m_manager = 0;
}

DebuggerPlugin::~DebuggerPlugin()
{}

void DebuggerPlugin::shutdown()
{
    if (m_debugMode)
        m_debugMode->shutdown(); // saves state including manager information
    QTC_ASSERT(m_manager, /**/);
    if (m_manager)
        m_manager->shutdown();

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

    m_pm = ExtensionSystem::PluginManager::instance();

    ICore *core = m_pm->getObject<Core::ICore>();
    QTC_ASSERT(core, return false);

    Core::ActionManagerInterface *actionManager = core->actionManager();
    QTC_ASSERT(actionManager, return false);

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

    //Core::IActionContainer *mcppcontext =
    //    actionManager->actionContainer(CppEditor::Constants::M_CONTEXT);

    Core::IActionContainer *mdebug =
        actionManager->actionContainer(ProjectExplorer::Constants::M_DEBUG);

    Core::ICommand *cmd = 0;
    cmd = actionManager->registerAction(m_manager->m_startExternalAction,
        Constants::STARTEXTERNAL, globalcontext);
    mdebug->addAction(cmd, Core::Constants::G_DEFAULT_ONE);

#ifndef Q_OS_WIN
    cmd = actionManager->registerAction(m_manager->m_attachExternalAction,
        Constants::ATTACHEXTERNAL, globalcontext);
    mdebug->addAction(cmd, Core::Constants::G_DEFAULT_ONE);
#endif

    cmd = actionManager->registerAction(m_manager->m_continueAction,
        ProjectExplorer::Constants::DEBUG, QList<int>()<< m_gdbRunningContext);

    cmd = actionManager->registerAction(m_manager->m_stopAction,
        Constants::INTERRUPT, globalcontext);
    cmd->setAttribute(Core::ICommand::CA_UpdateText);
    cmd->setAttribute(Core::ICommand::CA_UpdateIcon);
    cmd->setDefaultKeySequence(QKeySequence(Constants::INTERRUPT_KEY));
    cmd->setDefaultText(tr("Stop Debugger/Interrupt Debugger"));
    mdebug->addAction(cmd, Core::Constants::G_DEFAULT_ONE);

    cmd = actionManager->registerAction(m_manager->m_resetAction,
        Constants::RESET, globalcontext);
    cmd->setAttribute(Core::ICommand::CA_UpdateText);
    cmd->setDefaultKeySequence(QKeySequence(Constants::RESET_KEY));
    cmd->setDefaultText(tr("Reset Debugger"));
    //disabled mdebug->addAction(cmd, Core::Constants::G_DEFAULT_ONE);

    QAction *sep = new QAction(this);
    sep->setSeparator(true);
    cmd = actionManager->registerAction(sep,
        QLatin1String("GdbDebugger.Sep1"), globalcontext);
    mdebug->addAction(cmd);

    cmd = actionManager->registerAction(m_manager->m_nextAction,
        Constants::NEXT, debuggercontext);
    cmd->setDefaultKeySequence(QKeySequence(Constants::NEXT_KEY));
    mdebug->addAction(cmd);

    cmd = actionManager->registerAction(m_manager->m_stepAction,
        Constants::STEP, debuggercontext);
    cmd->setDefaultKeySequence(QKeySequence(Constants::STEP_KEY));
    mdebug->addAction(cmd);

    cmd = actionManager->registerAction(m_manager->m_stepOutAction,
        Constants::STEPOUT, debuggercontext);
    cmd->setDefaultKeySequence(QKeySequence(Constants::STEPOUT_KEY));
    mdebug->addAction(cmd);

    cmd = actionManager->registerAction(m_manager->m_nextIAction,
        Constants::NEXTI, debuggercontext);
    cmd->setDefaultKeySequence(QKeySequence(Constants::NEXTI_KEY));
    mdebug->addAction(cmd);

    cmd = actionManager->registerAction(m_manager->m_stepIAction,
        Constants::STEPI, debuggercontext);
    cmd->setDefaultKeySequence(QKeySequence(Constants::STEPI_KEY));
    mdebug->addAction(cmd);

    cmd = actionManager->registerAction(m_manager->m_runToLineAction,
        Constants::RUN_TO_LINE, debuggercontext);
    cmd->setDefaultKeySequence(QKeySequence(Constants::RUN_TO_LINE_KEY));
    mdebug->addAction(cmd);

    cmd = actionManager->registerAction(m_manager->m_runToFunctionAction,
        Constants::RUN_TO_FUNCTION, debuggercontext);
    cmd->setDefaultKeySequence(QKeySequence(Constants::RUN_TO_FUNCTION_KEY));
    mdebug->addAction(cmd);

    cmd = actionManager->registerAction(m_manager->m_jumpToLineAction,
        Constants::JUMP_TO_LINE, debuggercontext);
    mdebug->addAction(cmd);

    sep = new QAction(this);
    sep->setSeparator(true);
    cmd = actionManager->registerAction(sep,
        QLatin1String("GdbDebugger.Sep3"), globalcontext);
    mdebug->addAction(cmd);

    cmd = actionManager->registerAction(m_manager->m_breakAction,
        Constants::TOGGLE_BREAK, cppeditorcontext);
    cmd->setDefaultKeySequence(QKeySequence(Constants::TOGGLE_BREAK_KEY));
    mdebug->addAction(cmd);
    //mcppcontext->addAction(cmd);

    cmd = actionManager->registerAction(m_manager->m_breakByFunctionAction,
        Constants::BREAK_BY_FUNCTION, globalcontext);
    mdebug->addAction(cmd);

    cmd = actionManager->registerAction(m_manager->m_breakAtMainAction,
        Constants::BREAK_AT_MAIN, globalcontext);
    mdebug->addAction(cmd);

    sep = new QAction(this);
    sep->setSeparator(true);
    cmd = actionManager->registerAction(sep,
        QLatin1String("GdbDebugger.Sep2"), globalcontext);
    mdebug->addAction(cmd);

    cmd = actionManager->registerAction(m_manager->m_skipKnownFramesAction,
        Constants::SKIP_KNOWN_FRAMES, globalcontext);
    mdebug->addAction(cmd);

    cmd = actionManager->registerAction(m_manager->m_useCustomDumpersAction,
        Constants::USE_CUSTOM_DUMPERS, globalcontext);
    mdebug->addAction(cmd);

    cmd = actionManager->registerAction(m_manager->m_useFastStartAction,
        Constants::USE_FAST_START, globalcontext);
    mdebug->addAction(cmd);

    cmd = actionManager->registerAction(m_manager->m_useToolTipsAction,
        Constants::USE_TOOL_TIPS, globalcontext);
    mdebug->addAction(cmd);

#ifdef QT_DEBUG
    cmd = actionManager->registerAction(m_manager->m_dumpLogAction,
        Constants::DUMP_LOG, globalcontext);
    //cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+D,Ctrl+L")));
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+F11")));
    mdebug->addAction(cmd);

    cmd = actionManager->registerAction(m_manager->m_debugDumpersAction,
        Constants::DEBUG_DUMPERS, debuggercontext);
    mdebug->addAction(cmd);
#endif

    sep = new QAction(this);
    sep->setSeparator(true);
    cmd = actionManager->registerAction(sep,
        QLatin1String("GdbDebugger.Sep4"), globalcontext);
    mdebug->addAction(cmd);

    cmd = actionManager->registerAction(m_manager->m_watchAction,
        Constants::ADD_TO_WATCH, cppeditorcontext);
    //cmd->setDefaultKeySequence(QKeySequence(tr("ALT+D,ALT+W")));
    mdebug->addAction(cmd);

    m_generalOptionPage = 0;

    // FIXME:
    m_generalOptionPage = new GdbOptionPage(&theGdbSettings());
    addObject(m_generalOptionPage);

    m_locationMark = 0;

    m_debugMode = new DebugMode(m_manager, this);
    //addAutoReleasedObject(m_debugMode);
    addObject(m_debugMode);

    addAutoReleasedObject(new DebuggerRunner(m_manager));

    // ProjectExplorer
    connect(projectExplorer()->session(), SIGNAL(sessionLoaded()),
       m_manager, SLOT(sessionLoaded()));
    connect(projectExplorer()->session(), SIGNAL(aboutToSaveSession()),
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

ProjectExplorer::ProjectExplorerPlugin *DebuggerPlugin::projectExplorer() const
{
    return m_pm->getObject<ProjectExplorer::ProjectExplorerPlugin>();
}

/*! Activates the previous mode when the current mode is the debug mode. */
void DebuggerPlugin::activatePreviousMode()
{
    ICore *core = m_pm->getObject<Core::ICore>();
    Core::ModeManager *const modeManager = core->modeManager();

    if (modeManager->currentMode() == modeManager->mode(Constants::MODE_DEBUG)
            && !m_previousMode.isEmpty()) {
        modeManager->activateMode(m_previousMode);
        m_previousMode.clear();
    }
}

void DebuggerPlugin::activateDebugMode()
{
    ICore *core = m_pm->getObject<Core::ICore>();
    Core::ModeManager *modeManager = core->modeManager();
    m_previousMode = QLatin1String(modeManager->currentMode()->uniqueModeName());
    modeManager->activateMode(QLatin1String(MODE_DEBUG));
}

void DebuggerPlugin::queryCurrentTextEditor(QString *fileName, int *lineNumber, QObject **object)
{
    ICore *core = m_pm->getObject<Core::ICore>();
    if (!core || !core->editorManager())
        return;
    Core::IEditor *editor = core->editorManager()->currentEditor();
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
    if (!m_manager->useToolTipsAction()->isChecked())
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
        ExpressionUnderCursor expressionUnderCursor;
        expr = expressionUnderCursor(tc);
    }
    //qDebug() << " TOOLTIP  EXPR " << expr;
    m_manager->setToolTipExpression(point, expr);
}

void DebuggerPlugin::setSessionValue(const QString &name, const QVariant &value)
{
    //qDebug() << "SET SESSION VALUE" << name << value;
    ProjectExplorerPlugin *pe = projectExplorer();
    if (pe->session())
        pe->session()->setValue(name, value);
    else
        qDebug() << "FIXME: Session does not exist yet";
}

void DebuggerPlugin::querySessionValue(const QString &name, QVariant *value)
{
    ProjectExplorerPlugin *pe = projectExplorer();
    *value = pe->session()->value(name);
    //qDebug() << "GET SESSION VALUE: " << name << value;
}


void DebuggerPlugin::setConfigValue(const QString &name, const QVariant &value)
{
    QTC_ASSERT(m_debugMode, return);
    m_debugMode->settings()->setValue(name, value);
}

void DebuggerPlugin::queryConfigValue(const QString &name, QVariant *value)
{
    QTC_ASSERT(m_debugMode, return);
    *value = m_debugMode->settings()->value(name);
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
    ICore *core = m_pm->getObject<Core::ICore>();
    if (startIsContinue) {
        core->addAdditionalContext(m_gdbRunningContext);
        core->updateContext();
    } else {
        core->removeAdditionalContext(m_gdbRunningContext);
        core->updateContext();
    }
}

#include "debuggerplugin.moc"

Q_EXPORT_PLUGIN(DebuggerPlugin)
