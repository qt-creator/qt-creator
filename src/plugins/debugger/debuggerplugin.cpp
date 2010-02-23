/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "breakhandler.h"
#include "debuggeractions.h"
#include "debuggerdialogs.h"
#include "debuggerconstants.h"
#include "debuggermanager.h"
#include "debuggerrunner.h"
#include "debuggerstringutils.h"
#include "debuggeruiswitcher.h"
#include "debuggermainwindow.h"

#include "ui_commonoptionspage.h"
#include "ui_dumperoptionpage.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/basemode.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/findplaceholder.h>
#include <coreplugin/icore.h>
#include <coreplugin/icorelistener.h>
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

#include <coreplugin/manhattanstyle.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/session.h>
#include <projectexplorer/project.h>

#include <texteditor/basetexteditor.h>
#include <texteditor/basetextmark.h>
#include <texteditor/itexteditor.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditorplugin.h>

#include <utils/qtcassert.h>
#include <utils/styledbar.h>

#include <QtCore/QDebug>
#include <QtCore/QObject>
#include <QtCore/QPoint>
#include <QtCore/QSettings>
#include <QtCore/QtPlugin>
#include <QtCore/QCoreApplication>
#include <QtCore/QTimer>

#include <QtGui/QLineEdit>
#include <QtGui/QDockWidget>
#include <QtGui/QPlainTextEdit>
#include <QtGui/QTextBlock>
#include <QtGui/QTextCursor>
#include <QtGui/QToolButton>
#include <QtGui/QMessageBox>

#include <climits>

using namespace Core;
using namespace Debugger;
using namespace Debugger::Constants;
using namespace Debugger::Internal;
using namespace ProjectExplorer;
using namespace TextEditor;


namespace Debugger {
namespace Constants {

const char * const M_DEBUG_START_DEBUGGING = "QtCreator.Menu.Debug.StartDebugging";

const char * const STARTEXTERNAL        = "Debugger.StartExternal";
const char * const ATTACHEXTERNAL       = "Debugger.AttachExternal";
const char * const ATTACHCORE           = "Debugger.AttachCore";
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

#ifdef Q_WS_MAC
const char * const INTERRUPT_KEY            = "Shift+F5";
const char * const RESET_KEY                = "Ctrl+Shift+F5";
const char * const STEP_KEY                 = "F7";
const char * const STEPOUT_KEY              = "Shift+F7";
const char * const NEXT_KEY                 = "F6";
const char * const REVERSE_KEY              = "";
const char * const RUN_TO_LINE_KEY          = "Shift+F8";
const char * const RUN_TO_FUNCTION_KEY      = "Ctrl+F6";
const char * const JUMP_TO_LINE_KEY         = "Alt+D,Alt+L";
const char * const TOGGLE_BREAK_KEY         = "F8";
const char * const BREAK_BY_FUNCTION_KEY    = "Alt+D,Alt+F";
const char * const BREAK_AT_MAIN_KEY        = "Alt+D,Alt+M";
const char * const ADD_TO_WATCH_KEY         = "Alt+D,Alt+W";
const char * const SNAPSHOT_KEY             = "Alt+D,Alt+S";
#else
const char * const INTERRUPT_KEY            = "Shift+F5";
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
const char * const SNAPSHOT_KEY             = "Alt+D,Alt+S";
#endif

} // namespace Constants
} // namespace Debugger


static ProjectExplorer::SessionManager *sessionManager()
{
    return ProjectExplorer::ProjectExplorerPlugin::instance()->session();
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
};

DebugMode::DebugMode(QObject *parent)
  : BaseMode(parent)
{
    setDisplayName(tr("Debug"));
    setId(Constants::MODE_DEBUG);
    setIcon(QIcon(":/fancyactionbar/images/mode_Debug.png"));
    setPriority(Constants::P_MODE_DEBUG);
}

DebugMode::~DebugMode()
{
    // Make sure the editor manager does not get deleted
    EditorManager::instance()->setParent(0);
}

///////////////////////////////////////////////////////////////////////
//
// DebuggerListener: Close the debugging session if running.
//
///////////////////////////////////////////////////////////////////////

class DebuggerListener : public Core::ICoreListener {
    Q_OBJECT
public:
    explicit DebuggerListener(QObject *parent = 0);
    virtual bool coreAboutToClose();
};

DebuggerListener::DebuggerListener(QObject *parent) :
    Core::ICoreListener(parent)
{
}

bool DebuggerListener::coreAboutToClose()
{
    DebuggerManager *mgr = DebuggerManager::instance();
    if (!mgr)
        return true;
    // Ask to terminate the session.
    const QString title = tr("Close Debugging Session");
    bool cleanTermination = false;
    switch (mgr->state()) {
    case DebuggerNotReady:
        return true;
    case AdapterStarted:     // Most importantly, terminating a running
    case AdapterStartFailed: // debuggee can cause problems.
    case InferiorUnrunnable:
    case InferiorStartFailed:
    case InferiorStopped:
    case InferiorShutDown:
        cleanTermination = true;
        break;
    default:
        break;
    }
    const QString question = cleanTermination ?
        tr("A debugging session is still in progress.\nWould you like to terminate it?") :
        tr("A debugging session is still in progress. Terminating the session in the current"
           " state (%1) can leave the target in an inconsistent state."
           " Would you still like to terminate it?")
        .arg(QLatin1String(DebuggerManager::stateName(mgr->state())));
    QMessageBox::StandardButton answer = QMessageBox::question(DebuggerUISwitcher::instance()->mainWindow(),
                                         title, question,
                                         QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes);
    if (answer != QMessageBox::Yes)
        return false;
    mgr->exitDebugger();
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    return true;
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

// Used in "real" editors
class LocationMark : public TextEditor::BaseTextMark
{
    Q_OBJECT

public:
    LocationMark(const QString &fileName, int linenumber)
        : BaseTextMark(fileName, linenumber)
    {}

    QIcon icon() const { return DebuggerManager::instance()->locationMarkIcon(); }
    void updateLineNumber(int /*lineNumber*/) {}
    void updateBlock(const QTextBlock & /*block*/) {}
    void removedFromEditor() {}
};

} // namespace Internal
} // namespace Debugger


///////////////////////////////////////////////////////////////////////
//
// CommonOptionsPage
//
///////////////////////////////////////////////////////////////////////

namespace Debugger {
namespace Internal {

class CommonOptionsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    CommonOptionsPage() {}

    // IOptionsPage
    QString id() const
        { return QLatin1String(Debugger::Constants::DEBUGGER_COMMON_SETTINGS_ID); }
    QString displayName() const
        { return QCoreApplication::translate("Debugger", Debugger::Constants::DEBUGGER_COMMON_SETTINGS_NAME); }
    QString category() const
        { return QLatin1String(Debugger::Constants::DEBUGGER_SETTINGS_CATEGORY);  }
    QString displayCategory() const
        { return QCoreApplication::translate("Debugger", Debugger::Constants::DEBUGGER_SETTINGS_TR_CATEGORY); }

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
    m_group.insert(theDebuggerAction(UseMessageBoxForSignals),
        m_ui.checkBoxUseMessageBoxForSignals);
    m_group.insert(theDebuggerAction(SkipKnownFrames),
        m_ui.checkBoxSkipKnownFrames);
    m_group.insert(theDebuggerAction(UseToolTipsInMainEditor),
        m_ui.checkBoxUseToolTipsInMainEditor);
    m_group.insert(theDebuggerAction(AutoDerefPointers), 0);
    m_group.insert(theDebuggerAction(UseToolTipsInLocalsView), 0);
    m_group.insert(theDebuggerAction(UseToolTipsInBreakpointsView), 0);
    m_group.insert(theDebuggerAction(UseAddressInBreakpointsView), 0);
    m_group.insert(theDebuggerAction(UseAddressInStackView), 0);
    m_group.insert(theDebuggerAction(EnableReverseDebugging),
        m_ui.checkBoxEnableReverseDebugging);
    m_group.insert(theDebuggerAction(MaximalStackDepth),
        m_ui.spinBoxMaximalStackDepth);
    m_group.insert(theDebuggerAction(GdbWatchdogTimeout), 0);
    m_group.insert(theDebuggerAction(ShowStdNamespace), 0);
    m_group.insert(theDebuggerAction(ShowQtNamespace), 0);
    m_group.insert(theDebuggerAction(LogTimeStamps), 0);
    m_group.insert(theDebuggerAction(VerboseLog), 0);
    m_group.insert(theDebuggerAction(UsePreciseBreakpoints), 0);
    m_group.insert(theDebuggerAction(BreakOnThrow), 0);
    m_group.insert(theDebuggerAction(BreakOnCatch), 0);

#ifdef USE_REVERSE_DEBUGGING
    m_ui.checkBoxEnableReverseDebugging->hide();
#endif

    if (m_searchKeywords.isEmpty()) {
        QTextStream(&m_searchKeywords) << ' ' << m_ui.checkBoxListSourceFiles->text()
                << ' ' << m_ui.checkBoxUseMessageBoxForSignals->text()
                << ' ' << m_ui.checkBoxUseAlternatingRowColors->text()
                << ' ' << m_ui.checkBoxUseToolTipsInMainEditor->text()
                << ' ' << m_ui.checkBoxSkipKnownFrames->text()
                << ' ' << m_ui.checkBoxEnableReverseDebugging->text()
                << ' ' << m_ui.labelMaximalStackDepth->text();
        m_searchKeywords.remove(QLatin1Char('&'));
    }

    return w;
}

bool CommonOptionsPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}

} // namespace Internal
} // namespace Debugger


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

namespace Debugger {
namespace Internal {

class DebuggingHelperOptionPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    DebuggingHelperOptionPage() {}

    // IOptionsPage
    QString id() const { return QLatin1String("B.DebuggingHelper"); }
    QString displayName() const { return tr("Debugging Helper"); }
    QString category() const { return QLatin1String(Debugger::Constants::DEBUGGER_SETTINGS_CATEGORY); }
    QString displayCategory() const { return QCoreApplication::translate("Debugger", Debugger::Constants::DEBUGGER_SETTINGS_TR_CATEGORY); }

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
    // Suppress Oxygen style's giving flat group boxes bold titles
    if (oxygenStyle())
        m_ui.customLocationGroupBox->setStyleSheet(QLatin1String("QGroupBox::title { font: ; }"));

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
        Constants::DUMP_LOG, globalcontext);
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

} // namespace Internal
} // namespace Debugger


///////////////////////////////////////////////////////////////////////
//
// DebuggerPlugin
//
///////////////////////////////////////////////////////////////////////


DebuggerPlugin::AttachRemoteParameters::AttachRemoteParameters() :
    attachPid(0),
    winCrashEvent(0)
{
}

DebuggerPlugin::DebuggerPlugin()
  : m_manager(0),
    m_debugMode(0),
    m_locationMark(0),
    m_gdbRunningContext(0),
    m_cmdLineEnabledEngines(AllEngineTypes)
{}

DebuggerPlugin::~DebuggerPlugin()
{}

void DebuggerPlugin::shutdown()
{
    QTC_ASSERT(m_manager, /**/);
    if (m_manager)
        m_manager->shutdown();

    writeSettings();

    if (m_uiSwitcher)
        m_uiSwitcher->shutdown();

    delete DebuggerSettings::instance();

    removeObject(m_debugMode);

    // FIXME: when using the line below, BreakWindow etc gets deleted twice.
    // so better leak for now...
    delete m_debugMode;
    m_debugMode = 0;

    delete m_locationMark;
    m_locationMark = 0;

    removeObject(m_manager);
    delete m_manager;
    m_manager = 0;

    removeObject(m_uiSwitcher);
    delete m_uiSwitcher;
    m_uiSwitcher = 0;
}

static QString msgParameterMissing(const QString &a)
{
    return DebuggerPlugin::tr("Option '%1' is missing the parameter.").arg(a);
}

static QString msgInvalidNumericParameter(const QString &a, const QString &number)
{
    return DebuggerPlugin::tr("The parameter '%1' of option '%2' is not a number.").arg(number, a);
}

// Parse arguments
static bool parseArgument(QStringList::const_iterator &it,
                          const QStringList::const_iterator &cend,
                          DebuggerPlugin::AttachRemoteParameters *attachRemoteParameters,
                          unsigned *enabledEngines, QString *errorMessage)
{
    const QString &option = *it;
    // '-debug <pid>'
    if (*it == QLatin1String("-debug")) {
        ++it;
        if (it == cend) {
            *errorMessage = msgParameterMissing(*it);
            return false;
        }
        bool ok;
        attachRemoteParameters->attachPid = it->toULongLong(&ok);
        if (!ok) {
            attachRemoteParameters->attachPid = 0;
            attachRemoteParameters->attachCore = *it;
        }
        return true;
    }
    // -wincrashevent <event-handle>. A handle used for
    // a handshake when attaching to a crashed Windows process.
    if (*it == QLatin1String("-wincrashevent")) {
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
    // engine disabling
    if (option == QLatin1String("-disable-cdb")) {
        *enabledEngines &= ~Debugger::CdbEngineType;
        return true;
    }
    if (option == QLatin1String("-disable-gdb")) {
        *enabledEngines &= ~Debugger::GdbEngineType;
        return true;
    }
    if (option == QLatin1String("-disable-sdb")) {
        *enabledEngines &= ~Debugger::ScriptEngineType;
        return true;
    }

    *errorMessage = DebuggerPlugin::tr("Invalid debugger option: %1").arg(option);
    return false;
}

static bool parseArguments(const QStringList &args,
                           DebuggerPlugin::AttachRemoteParameters *attachRemoteParameters,
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
            << " core" << attachRemoteParameters->attachCore << '\n';
    return true;
}

void DebuggerPlugin::remoteCommand(const QStringList &options, const QStringList &)
{
    QString errorMessage;
    AttachRemoteParameters parameters;
    unsigned dummy = 0;
    // Did we receive a request for debugging (unless it is ourselves)?
    if (parseArguments(options, &parameters, &dummy, &errorMessage)
        && parameters.attachPid != quint64(QCoreApplication::applicationPid())) {
        m_attachRemoteParameters = parameters;
        attachCmdLine();
    }
}

bool DebuggerPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    // Do not fail the whole plugin if something goes wrong here
    if (!parseArguments(arguments, &m_attachRemoteParameters, &m_cmdLineEnabledEngines, errorMessage)) {
        *errorMessage = tr("Error evaluating command line arguments: %1")
            .arg(*errorMessage);
        qWarning("%s\n", qPrintable(*errorMessage));
        errorMessage->clear();
    }

    // Debug mode setup
    m_debugMode = new DebugMode(this);
    //addAutoReleasedObject(m_debugMode);
    m_uiSwitcher = new DebuggerUISwitcher(m_debugMode, this);
    ExtensionSystem::PluginManager::instance()->addObject(m_uiSwitcher);
    m_uiSwitcher->addLanguage(LANG_CPP);

    m_manager = new DebuggerManager;
    ExtensionSystem::PluginManager::instance()->addObject(m_manager);
    const QList<Core::IOptionsPage *> engineOptionPages =
        m_manager->initializeEngines(m_cmdLineEnabledEngines);

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

    // register factory of DebuggerRunControl
    m_debuggerRunControlFactory = new DebuggerRunControlFactory(m_manager);
    addAutoReleasedObject(m_debuggerRunControlFactory);

    QList<int> context;
    context.append(uidm->uniqueIdentifier(Core::Constants::C_EDITORMANAGER));
    context.append(uidm->uniqueIdentifier(Debugger::Constants::C_GDBDEBUGGER));
    context.append(uidm->uniqueIdentifier(Core::Constants::C_NAVIGATION_PANE));
    m_debugMode->setContext(context);

    //Core::ActionContainer *mcppcontext =
    //    am->actionContainer(CppEditor::Constants::M_CONTEXT);

    // External apps
    m_startExternalAction = new QAction(this);
    m_startExternalAction->setText(tr("Start and Debug External Application..."));
    connect(m_startExternalAction, SIGNAL(triggered()),
        this, SLOT(startExternalApplication()));

    m_attachExternalAction = new QAction(this);
    m_attachExternalAction->setText(tr("Attach to Running External Application..."));
    connect(m_attachExternalAction, SIGNAL(triggered()),
        this, SLOT(attachExternalApplication()));

    m_attachCoreAction = new QAction(this);
    m_attachCoreAction->setText(tr("Attach to Core..."));
    connect(m_attachCoreAction, SIGNAL(triggered()), this, SLOT(attachCore()));

    m_startRemoteAction = new QAction(this);
    m_startRemoteAction->setText(tr("Start and Attach to Remote Application..."));
    connect(m_startRemoteAction, SIGNAL(triggered()),
        this, SLOT(startRemoteApplication()));

    m_detachAction = new QAction(this);
    m_detachAction->setText(tr("Detach Debugger"));
    connect(m_detachAction, SIGNAL(triggered()),
        m_manager, SLOT(detachDebugger()));

   // Core::ActionContainer *mdebug =
    //    am->actionContainer(ProjectExplorer::Constants::M_DEBUG);

    Core::Command *cmd = 0;
    const DebuggerManagerActions actions = m_manager->debuggerManagerActions();


    Core::ActionContainer *mstart =
        am->actionContainer(ProjectExplorer::Constants::M_DEBUG_STARTDEBUGGING);

    cmd = am->registerAction(actions.continueAction,
        ProjectExplorer::Constants::DEBUG, QList<int>() << m_gdbRunningContext);
    mstart->addAction(cmd, Core::Constants::G_DEFAULT_ONE);

    cmd = am->registerAction(m_startExternalAction,
        Constants::STARTEXTERNAL, globalcontext);
    mstart->addAction(cmd, Core::Constants::G_DEFAULT_ONE);

    cmd = am->registerAction(m_attachExternalAction,
        Constants::ATTACHEXTERNAL, globalcontext);
    mstart->addAction(cmd, Core::Constants::G_DEFAULT_ONE);

    cmd = am->registerAction(m_attachCoreAction,
        Constants::ATTACHCORE, globalcontext);
    mstart->addAction(cmd, Core::Constants::G_DEFAULT_ONE);

    cmd = am->registerAction(m_startRemoteAction,
        Constants::ATTACHREMOTE, globalcontext);
    mstart->addAction(cmd, Core::Constants::G_DEFAULT_ONE);

    cmd = am->registerAction(m_detachAction,
        Constants::DETACH, globalcontext);
    m_uiSwitcher->addMenuAction(cmd, Core::Constants::G_DEFAULT_ONE);

    cmd = am->registerAction(actions.stopAction,
        Constants::INTERRUPT, globalcontext);
    cmd->setAttribute(Core::Command::CA_UpdateText);
    cmd->setAttribute(Core::Command::CA_UpdateIcon);
    cmd->setDefaultKeySequence(QKeySequence(Constants::INTERRUPT_KEY));
    cmd->setDefaultText(tr("Stop Debugger/Interrupt Debugger"));
    m_uiSwitcher->addMenuAction(cmd, Core::Constants::G_DEFAULT_ONE);

    cmd = am->registerAction(actions.resetAction,
        Constants::RESET, globalcontext);
    cmd->setAttribute(Core::Command::CA_UpdateText);
    //cmd->setDefaultKeySequence(QKeySequence(Constants::RESET_KEY));
    cmd->setDefaultText(tr("Reset Debugger"));
    m_uiSwitcher->addMenuAction(cmd, Core::Constants::G_DEFAULT_ONE);

    QAction *sep = new QAction(this);
    sep->setSeparator(true);
    cmd = am->registerAction(sep, QLatin1String("Debugger.Sep.Step"), globalcontext);
    m_uiSwitcher->addMenuAction(cmd);

    cmd = am->registerAction(actions.nextAction,
        Constants::NEXT, debuggercontext);
    cmd->setDefaultKeySequence(QKeySequence(Constants::NEXT_KEY));
    m_uiSwitcher->addMenuAction(cmd);

    cmd = am->registerAction(actions.stepAction,
        Constants::STEP, debuggercontext);
    cmd->setDefaultKeySequence(QKeySequence(Constants::STEP_KEY));
    m_uiSwitcher->addMenuAction(cmd);

    cmd = am->registerAction(actions.stepOutAction,
        Constants::STEPOUT, debuggercontext);
    cmd->setDefaultKeySequence(QKeySequence(Constants::STEPOUT_KEY));
    m_uiSwitcher->addMenuAction(cmd);

    cmd = am->registerAction(actions.runToLineAction1,
        Constants::RUN_TO_LINE1, debuggercontext);
    cmd->setDefaultKeySequence(QKeySequence(Constants::RUN_TO_LINE_KEY));
    m_uiSwitcher->addMenuAction(cmd);

    cmd = am->registerAction(actions.runToFunctionAction,
        Constants::RUN_TO_FUNCTION, debuggercontext);
    cmd->setDefaultKeySequence(QKeySequence(Constants::RUN_TO_FUNCTION_KEY));
    m_uiSwitcher->addMenuAction(cmd);

    cmd = am->registerAction(actions.jumpToLineAction1,
        Constants::JUMP_TO_LINE1, debuggercontext);
    m_uiSwitcher->addMenuAction(cmd);

    cmd = am->registerAction(actions.returnFromFunctionAction,
        Constants::RETURN_FROM_FUNCTION, debuggercontext);
    m_uiSwitcher->addMenuAction(cmd);

#ifdef USE_REVERSE_DEBUGGING
    cmd = am->registerAction(actions.reverseDirectionAction,
        Constants::REVERSE, debuggercontext);
    cmd->setDefaultKeySequence(QKeySequence(Constants::REVERSE_KEY));
    m_uiSwitcher->addMenuAction(cmd);
#endif

    sep = new QAction(this);
    sep->setSeparator(true);
    cmd = am->registerAction(sep, QLatin1String("Debugger.Sep.Break"), globalcontext);
    m_uiSwitcher->addMenuAction(cmd);

    cmd = am->registerAction(actions.snapshotAction,
        Constants::SNAPSHOT, debuggercontext);
    cmd->setDefaultKeySequence(QKeySequence(Constants::SNAPSHOT_KEY));
    m_uiSwitcher->addMenuAction(cmd);

    cmd = am->registerAction(theDebuggerAction(OperateByInstruction),
        Constants::OPERATE_BY_INSTRUCTION, debuggercontext);
    m_uiSwitcher->addMenuAction(cmd);

    cmd = am->registerAction(actions.breakAction,
        Constants::TOGGLE_BREAK, cppeditorcontext);
    cmd->setDefaultKeySequence(QKeySequence(Constants::TOGGLE_BREAK_KEY));
    m_uiSwitcher->addMenuAction(cmd);
    //mcppcontext->addAction(cmd);

    sep = new QAction(this);
    sep->setSeparator(true);
    cmd = am->registerAction(sep, QLatin1String("Debugger.Sep.Watch"), globalcontext);
    m_uiSwitcher->addMenuAction(cmd);

    cmd = am->registerAction(actions.watchAction1,
        Constants::ADD_TO_WATCH1, cppeditorcontext);
    cmd->action()->setEnabled(true);
    //cmd->setDefaultKeySequence(QKeySequence(tr("ALT+D,ALT+W")));
    m_uiSwitcher->addMenuAction(cmd);

    // Editor context menu
    ActionContainer *editorContextMenu =
        am->actionContainer(CppEditor::Constants::M_CONTEXT);
    cmd = am->registerAction(sep, QLatin1String("Debugger.Sep.Views"),
        debuggercontext);
    editorContextMenu->addAction(cmd);
    cmd->setAttribute(Command::CA_Hide);

    cmd = am->registerAction(actions.watchAction2,
        Constants::ADD_TO_WATCH2, debuggercontext);
    cmd->action()->setEnabled(true);
    editorContextMenu->addAction(cmd);
    cmd->setAttribute(Command::CA_Hide);

    cmd = am->registerAction(actions.runToLineAction2,
        Constants::RUN_TO_LINE2, debuggercontext);
    cmd->action()->setEnabled(true);
    editorContextMenu->addAction(cmd);
    cmd->setAttribute(Command::CA_Hide);

    cmd = am->registerAction(actions.jumpToLineAction2,
        Constants::JUMP_TO_LINE2, debuggercontext);
    cmd->action()->setEnabled(true);
    editorContextMenu->addAction(cmd);
    cmd->setAttribute(Command::CA_Hide);

    // FIXME:
    addAutoReleasedObject(new CommonOptionsPage);
    addAutoReleasedObject(new DebuggingHelperOptionPage);
    foreach (Core::IOptionsPage* op, engineOptionPages)
        addAutoReleasedObject(op);
    addAutoReleasedObject(new DebuggerListener);
    m_locationMark = 0;

    m_manager->setSimpleDockWidgetArrangement(LANG_CPP);
    readSettings();

    m_uiSwitcher->setToolbar(LANG_CPP, createToolbar());
    connect(m_uiSwitcher, SIGNAL(dockArranged(QString)), m_manager,
            SLOT(setSimpleDockWidgetArrangement(QString)));

    connect(ModeManager::instance(), SIGNAL(currentModeChanged(Core::IMode*)),
            this, SLOT(onModeChanged(Core::IMode*)));
    m_debugMode->widget()->setFocusProxy(EditorManager::instance());
    addObject(m_debugMode);

    //
    //  Connections
    //

    // TextEditor
    connect(TextEditor::TextEditorPlugin::instance(),
        SIGNAL(fontSettingsChanged(TextEditor::FontSettings)),
        m_manager, SLOT(fontSettingsChanged(TextEditor::FontSettings)));

    // ProjectExplorer
    connect(sessionManager(), SIGNAL(sessionLoaded()),
       m_manager, SLOT(sessionLoaded()));
    connect(sessionManager(), SIGNAL(aboutToSaveSession()),
       m_manager, SLOT(aboutToSaveSession()));
    connect(sessionManager(), SIGNAL(aboutToUnloadSession()),
       m_manager, SLOT(aboutToUnloadSession()));

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

    connect(m_manager, SIGNAL(resetLocationRequested()),
        this, SLOT(resetLocation()));
    connect(m_manager, SIGNAL(gotoLocationRequested(QString,int,bool)),
        this, SLOT(gotoLocation(QString,int,bool)));
    connect(m_manager, SIGNAL(stateChanged(int)),
        this, SLOT(handleStateChanged(int)));
    connect(m_manager, SIGNAL(previousModeRequested()),
        this, SLOT(activatePreviousMode()));
    connect(m_manager, SIGNAL(debugModeRequested()),
        this, SLOT(activateDebugMode()));

    connect(theDebuggerAction(SettingsDialog), SIGNAL(triggered()),
        this, SLOT(showSettingsDialog()));

    handleStateChanged(DebuggerNotReady);
    return true;
}

QWidget *DebuggerPlugin::createToolbar() const
{
    Core::ActionManager *am = ICore::instance()->actionManager();

    QWidget *toolbarContainer = new QWidget;
    QHBoxLayout *debugToolBarLayout = new QHBoxLayout(toolbarContainer);

    debugToolBarLayout->setMargin(0);
    debugToolBarLayout->setSpacing(0);
    debugToolBarLayout->addWidget(toolButton(am->command(ProjectExplorer::Constants::DEBUG)->action()));
    debugToolBarLayout->addWidget(toolButton(am->command(Constants::INTERRUPT)->action()));
    debugToolBarLayout->addWidget(toolButton(am->command(Constants::NEXT)->action()));
    debugToolBarLayout->addWidget(toolButton(am->command(Constants::STEP)->action()));
    debugToolBarLayout->addWidget(toolButton(am->command(Constants::STEPOUT)->action()));
    debugToolBarLayout->addWidget(toolButton(am->command(Constants::OPERATE_BY_INSTRUCTION)->action()));
#ifdef USE_REVERSE_DEBUGGING
    debugToolBarLayout->addWidget(new Utils::StyledSeparator);
    debugToolBarLayout->addWidget(toolButton(am->command(Constants::REVERSE)->action()));
#endif
    debugToolBarLayout->addWidget(new Utils::StyledSeparator);
    debugToolBarLayout->addWidget(new QLabel(tr("Threads:")));

    QComboBox *threadBox = new QComboBox;
    threadBox->setModel(m_manager->threadsModel());
    connect(threadBox, SIGNAL(activated(int)),
        m_manager->threadsWindow(), SIGNAL(threadSelected(int)));
    debugToolBarLayout->addWidget(threadBox);
    debugToolBarLayout->addWidget(m_manager->statusLabel(), 10);

    return toolbarContainer;
}

void DebuggerPlugin::extensionsInitialized()
{
    // time gdb -i mi -ex 'debuggerplugin.cpp:800' -ex r -ex q bin/qtcreator.bin
    const QByteArray env = qgetenv("QTC_DEBUGGER_TEST");
    //qDebug() << "EXTENSIONS INITIALIZED:" << env;
    if (!env.isEmpty())
        m_manager->runTest(QString::fromLocal8Bit(env));
    if (m_attachRemoteParameters.attachPid || !m_attachRemoteParameters.attachCore.isEmpty())
        QTimer::singleShot(0, this, SLOT(attachCmdLine()));

    readSettings();
    m_uiSwitcher->initialize();
}

void DebuggerPlugin::attachCmdLine()
{
    if (m_manager->state() != DebuggerNotReady)
        return;
    if (m_attachRemoteParameters.attachPid) {
        m_manager->showStatusMessage(tr("Attaching to PID %1.").arg(m_attachRemoteParameters.attachPid));
        const QString crashParameter =
                m_attachRemoteParameters.winCrashEvent ? QString::number(m_attachRemoteParameters.winCrashEvent) : QString();
        attachExternalApplication(m_attachRemoteParameters.attachPid, crashParameter);
        return;
    }
    if (!m_attachRemoteParameters.attachCore.isEmpty()) {
        m_manager->showStatusMessage(tr("Attaching to core %1.").arg(m_attachRemoteParameters.attachCore));
        attachCore(m_attachRemoteParameters.attachCore, QString());
    }
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
    m_previousMode = modeManager->currentMode()->id();
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
    QString fileName = editor->file()->fileName();
    QString position = fileName + QString(":%1").arg(lineNumber);
    BreakpointData *data = m_manager->findBreakpoint(fileName, lineNumber);

    if (data) {
        // existing breakpoint
        QAction *act = new QAction(tr("Remove Breakpoint"), menu);
        act->setData(position);
        connect(act, SIGNAL(triggered()),
            this, SLOT(breakpointSetRemoveMarginActionTriggered()));
        menu->addAction(act);

        QAction *act2;
        if (data->enabled)
            act2 = new QAction(tr("Disable Breakpoint"), menu);
        else
            act2 = new QAction(tr("Enable Breakpoint"), menu);
        act2->setData(position);
        connect(act2, SIGNAL(triggered()),
            this, SLOT(breakpointEnableDisableMarginActionTriggered()));
        menu->addAction(act2);
    } else {
        // non-existing
        QAction *act = new QAction(tr("Set Breakpoint"), menu);
        act->setData(position);
        connect(act, SIGNAL(triggered()),
            this, SLOT(breakpointSetRemoveMarginActionTriggered()));
        menu->addAction(act);
    }
}

void DebuggerPlugin::breakpointSetRemoveMarginActionTriggered()
{
    if (QAction *act = qobject_cast<QAction *>(sender())) {
        QString str = act->data().toString();
        int pos = str.lastIndexOf(':');
        m_manager->toggleBreakpoint(str.left(pos), str.mid(pos + 1).toInt());
    }
}

void DebuggerPlugin::breakpointEnableDisableMarginActionTriggered()
{
    if (QAction *act = qobject_cast<QAction *>(sender())) {
        QString str = act->data().toString();
        int pos = str.lastIndexOf(':');
        m_manager->toggleBreakpointEnabled(str.left(pos), str.mid(pos + 1).toInt());
    }
}

void DebuggerPlugin::requestMark(TextEditor::ITextEditor *editor, int lineNumber)
{
    m_manager->toggleBreakpoint(editor->file()->fileName(), lineNumber);
}

void DebuggerPlugin::showToolTip(TextEditor::ITextEditor *editor,
    const QPoint &point, int pos)
{
    if (!theDebuggerBoolSetting(UseToolTipsInMainEditor)
            || m_manager->state() == DebuggerNotReady)
        return;

    m_manager->setToolTipExpression(point, editor, pos);
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

QVariant DebuggerPlugin::configValue(const QString &name) const
{
    QTC_ASSERT(m_debugMode, return QVariant());
    return settings()->value(name);
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

void DebuggerPlugin::gotoLocation(const QString &file, int line, bool setMarker)
{
    TextEditor::BaseTextEditor::openEditorAt(file, line);
    if (setMarker) {
        resetLocation();
        m_locationMark = new LocationMark(file, line);
    }
}

void DebuggerPlugin::handleStateChanged(int state)
{
    const bool startIsContinue = (state == InferiorStopped);
    ICore *core = ICore::instance();
    if (startIsContinue) {
        core->addAdditionalContext(m_gdbRunningContext);
        core->updateContext();
    } else {
        core->removeAdditionalContext(m_gdbRunningContext);
        core->updateContext();
    }

    const bool started = state == InferiorRunning
        || state == InferiorRunningRequested
        || state == InferiorStopping
        || state == InferiorStopped;

    const bool starting = state == EngineStarting;
    //const bool running = state == InferiorRunning;

    const bool detachable = state == InferiorStopped
        && m_manager->startParameters()->startMode != AttachCore;

    m_startExternalAction->setEnabled(!started && !starting);
    m_attachExternalAction->setEnabled(!started && !starting);
#ifdef Q_OS_WIN
    m_attachCoreAction->setEnabled(false);
#else
    m_attachCoreAction->setEnabled(!started && !starting);
#endif
    m_startRemoteAction->setEnabled(!started && !starting);
    m_detachAction->setEnabled(detachable);
}

void DebuggerPlugin::writeSettings() const
{
    QSettings *s = settings();
    DebuggerSettings::instance()->writeSettings(s);
}

void DebuggerPlugin::readSettings()
{
    QSettings *s = settings();
    DebuggerSettings::instance()->readSettings(s);
}

void DebuggerPlugin::onModeChanged(IMode *mode)
{
     // FIXME: This one gets always called, even if switching between modes
     //        different then the debugger mode. E.g. Welcome and Help mode and
     //        also on shutdown.

    if (mode != m_debugMode)
        return;

    EditorManager *editorManager = EditorManager::instance();
    if (editorManager->currentEditor()) {
        editorManager->currentEditor()->widget()->setFocus();

        if (editorManager->currentEditor()->id() == CppEditor::Constants::C_CPPEDITOR) {
            m_uiSwitcher->setActiveLanguage(Debugger::Constants::LANG_CPP);
        }

    }
}

void DebuggerPlugin::showSettingsDialog()
{
    Core::ICore::instance()->showOptionsDialog(
        QLatin1String(Debugger::Constants::DEBUGGER_SETTINGS_CATEGORY),
        QLatin1String(Debugger::Constants::DEBUGGER_COMMON_SETTINGS_ID));
}

void DebuggerPlugin::startExternalApplication()
{
    const DebuggerStartParametersPtr sp(new DebuggerStartParameters);
    StartExternalDialog dlg(m_uiSwitcher->mainWindow());
    dlg.setExecutableFile(
            configValue(_("LastExternalExecutableFile")).toString());
    dlg.setExecutableArguments(
            configValue(_("LastExternalExecutableArguments")).toString());
    if (dlg.exec() != QDialog::Accepted)
        return;

    setConfigValue(_("LastExternalExecutableFile"),
                   dlg.executableFile());
    setConfigValue(_("LastExternalExecutableArguments"),
                   dlg.executableArguments());
    sp->executable = dlg.executableFile();
    sp->startMode = StartExternal;
    if (!dlg.executableArguments().isEmpty())
        sp->processArgs = dlg.executableArguments().split(QLatin1Char(' '));

    if (dlg.breakAtMain())
        m_manager->breakByFunctionMain();

    if (RunControl *runControl = m_debuggerRunControlFactory->create(sp))
        ProjectExplorerPlugin::instance()->startRunControl(runControl, ProjectExplorer::Constants::DEBUGMODE);
}

void DebuggerPlugin::attachExternalApplication()
{
    AttachExternalDialog dlg(m_uiSwitcher->mainWindow());
    if (dlg.exec() == QDialog::Accepted)
        attachExternalApplication(dlg.attachPID());
}

void DebuggerPlugin::attachExternalApplication(qint64 pid, const QString &crashParameter)
{
    if (pid == 0) {
        QMessageBox::warning(m_uiSwitcher->mainWindow(), tr("Warning"), tr("Cannot attach to PID 0"));
        return;
    }
    const DebuggerStartParametersPtr sp(new DebuggerStartParameters);
    sp->attachPID = pid;
    sp->crashParameter = crashParameter;
    sp->startMode = crashParameter.isEmpty() ? AttachExternal : AttachCrashedExternal;
    if (RunControl *runControl = m_debuggerRunControlFactory->create(sp))
        ProjectExplorerPlugin::instance()->startRunControl(runControl, ProjectExplorer::Constants::DEBUGMODE);
}

void DebuggerPlugin::attachCore()
{
    AttachCoreDialog dlg(m_uiSwitcher->mainWindow());
    dlg.setExecutableFile(
            configValue(_("LastExternalExecutableFile")).toString());
    dlg.setCoreFile(
            configValue(_("LastExternalCoreFile")).toString());
    if (dlg.exec() != QDialog::Accepted)
        return;
    setConfigValue(_("LastExternalExecutableFile"),
                   dlg.executableFile());
    setConfigValue(_("LastExternalCoreFile"),
                   dlg.coreFile());
    attachCore(dlg.coreFile(), dlg.executableFile());
}

void DebuggerPlugin::attachCore(const QString &core, const QString &exe)
{
    const DebuggerStartParametersPtr sp(new DebuggerStartParameters);
    sp->executable = exe;
    sp->coreFile = core;
    sp->startMode = AttachCore;
    if (RunControl *runControl = m_debuggerRunControlFactory->create(sp))
        ProjectExplorerPlugin::instance()->
            startRunControl(runControl, ProjectExplorer::Constants::DEBUGMODE);
}

void DebuggerPlugin::startRemoteApplication()
{
    const DebuggerStartParametersPtr sp(new DebuggerStartParameters);
    StartRemoteDialog dlg(m_uiSwitcher->mainWindow());
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
    sp->remoteChannel = dlg.remoteChannel();
    sp->remoteArchitecture = dlg.remoteArchitecture();
    sp->executable = dlg.localExecutable();
    sp->debuggerCommand = dlg.debugger();
    sp->startMode = StartRemote;
    if (dlg.useServerStartScript())
        sp->serverStartScript = dlg.serverStartScript();
    sp->sysRoot = dlg.sysRoot();

    if (RunControl *runControl = m_debuggerRunControlFactory->create(sp))
        ProjectExplorerPlugin::instance()
            ->startRunControl(runControl, ProjectExplorer::Constants::DEBUGMODE);
}

#include "debuggerplugin.moc"

Q_EXPORT_PLUGIN(DebuggerPlugin)
