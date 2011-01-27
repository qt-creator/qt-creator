/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include "cdbengine.h"
#include "debuggerstartparameters.h"
#include "disassemblerlines.h"
#include "cdboptions.h"
#include "cdboptionspage.h"
#include "bytearrayinputstream.h"
#include "breakpoint.h"
#include "breakhandler.h"
#include "stackframe.h"
#include "stackhandler.h"
#include "watchhandler.h"
#include "threadshandler.h"
#include "moduleshandler.h"
#include "debuggeractions.h"
#include "debuggercore.h"
#include "registerhandler.h"
#include "disassembleragent.h"
#include "memoryagent.h"
#include "debuggerrunner.h"
#include "debuggertooltip.h"
#include "cdbparsehelpers.h"
#include "watchutils.h"
#include "gdb/gdbmi.h"
#include "shared/cdbsymbolpathlisteditor.h"

#include <coreplugin/icore.h>
#include <texteditor/itexteditor.h>
#include <projectexplorer/toolchain.h>

#include <utils/synchronousprocess.h>
#include <utils/winutils.h>
#include <utils/qtcassert.h>
#include <utils/savedaction.h>
#include <utils/consoleprocess.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QDebug>
#include <QtCore/QTextStream>
#include <QtCore/QDateTime>
#include <QtGui/QToolTip>
#include <QtGui/QMainWindow>
#include <QtGui/QMessageBox>

#ifdef Q_OS_WIN
#    include <utils/winutils.h>
#    include "dbgwinutils.h"
#endif

#include <cctype>

Q_DECLARE_METATYPE(Debugger::Internal::DisassemblerAgent*)
Q_DECLARE_METATYPE(Debugger::Internal::MemoryAgent*)

enum { debug = 0 };
enum { debugLocals = 0 };
enum { debugWatches = 0 };
enum { debugBreakpoints = 0 };

#if 0
#  define STATE_DEBUG(state, func, line, notifyFunc) qDebug("%s in %s at %s:%d", notifyFunc, stateName(state), func, line);
#else
#  define STATE_DEBUG(state, func, line, notifyFunc)
#endif

/* CdbEngine version 2: Run the CDB process on pipes and parse its output.
 * The engine relies on a CDB extension Qt Creator provides as an extension
 * library (32/64bit), which is loaded into cdb.exe. It serves to:
 * - Notify the engine about the state of the debugging session:
 *   + idle: (hooked up with .idle_cmd) debuggee stopped
 *   + accessible: Debuggee stopped, cdb.exe accepts commands
 *   + inaccessible: Debuggee runs, no way to post commands
 *   + session active/inactive: Lost debuggee, terminating.
 * - Hook up with output/event callbacks and produce formatted output to be able
 *   to catch application output and exceptions.
 * - Provide some extension commands that produce output in a standardized (GDBMI)
 *   format that ends up in handleExtensionMessage().
 *   + pid     Return debuggee pid for interrupting.
 *   + locals  Print locals from SymbolGroup
 *   + expandLocals Expand locals in symbol group
 *   + registers, modules, threads
 * Commands can be posted by calling:
 * 1) postCommand(): Does not expect a reply
 * 2) postBuiltinCommand(): Run a builtin-command producing free-format, multiline output
 *    that is captured by enclosing it in special tokens using the 'echo' command and
 *    then invokes a callback with a CdbBuiltinCommand structure.
 * 3) postExtensionCommand(): Run a command provided by the extension producing
 *    one-line output and invoke a callback with a CdbExtensionCommand structure
 *    (output is potentially split up in chunks).
 *
 * Startup sequence:
 * [Console: The console stub launches the process. On process startup,
 *           launchCDB() is called with AttachExternal.
 * setupEngine() calls launchCDB() with the startparameters. The debuggee
 * runs into the initial breakpoint (session idle). EngineSetupOk is
 * notified (inferior still stopped). setupInferior() is then called
 * which does breakpoint synchronization and issues the extension 'pid'
 * command to obtain the inferior pid. handlePid() notifies notifyInferiorSetupOk.
 * runEngine() is then called which issues 'g' to continue the inferior.
 * Shutdown mostly uses notifyEngineSpontaneousShutdown() as cdb just quits
 * when the inferior exits (except attach modes).  */

using namespace ProjectExplorer;

namespace Debugger {
namespace Internal {

static const char localsPrefixC[] = "local.";

struct MemoryViewCookie {
    explicit MemoryViewCookie(MemoryAgent *a = 0, QObject *e = 0,
                              quint64 addr = 0, quint64 l = 0) :
        agent(a), editorToken(e), address(addr), length(l) {}

    MemoryAgent *agent;
    QObject *editorToken;
    quint64 address;
    quint64 length;
};

struct SourceLocationCookie {
    explicit SourceLocationCookie(const QString &f = QString(), int l = 0) :
             fileName(f), lineNumber(l) {}

    QString fileName;
    int lineNumber;
};

} // namespace Internal
} // namespace Debugger

Q_DECLARE_METATYPE(Debugger::Internal::MemoryViewCookie)
Q_DECLARE_METATYPE(Debugger::Internal::SourceLocationCookie)

namespace Debugger {
namespace Internal {

static inline bool isConsole(const DebuggerStartParameters &sp)
{ return (sp.startMode == StartInternal || sp.startMode == StartExternal) && sp.useTerminal; }

static QMessageBox
    *nonModalMessageBox(QMessageBox::Icon icon, const QString &title, const QString &text)
{
    QMessageBox *mb = new QMessageBox(icon, title, text, QMessageBox::Ok,
                                      debuggerCore()->mainWindow());
    mb->setAttribute(Qt::WA_DeleteOnClose);
    mb->show();
    return mb;
}

// Base data structure for command queue entries with callback
struct CdbCommandBase
{
    typedef CdbEngine::BuiltinCommandHandler CommandHandler;

    CdbCommandBase();
    CdbCommandBase(const QByteArray  &cmd, int token, unsigned flags,
                   unsigned nc, const QVariant &cookie);

    int token;
    unsigned flags;
    QByteArray command;
    QVariant cookie;
    // Continue with another commands as specified in CommandSequenceFlags
    unsigned commandSequence;
};

CdbCommandBase::CdbCommandBase() :
    token(0), flags(0), commandSequence(0)
{
}

CdbCommandBase::CdbCommandBase(const QByteArray  &cmd, int t, unsigned f,
                               unsigned nc, const QVariant &c) :
    token(t), flags(f), command(cmd), cookie(c), commandSequence(nc)
{
}

// Queue entry for builtin commands producing free-format
// line-by-line output.
struct CdbBuiltinCommand : public CdbCommandBase
{
    typedef CdbEngine::BuiltinCommandHandler CommandHandler;

    CdbBuiltinCommand() {}
    CdbBuiltinCommand(const QByteArray  &cmd, int token, unsigned flags,
                      CommandHandler h,
                      unsigned nc, const QVariant &cookie) :
        CdbCommandBase(cmd, token, flags, nc, cookie), handler(h) {}


    QByteArray joinedReply() const;

    CommandHandler handler;
    QList<QByteArray> reply;
};

QByteArray CdbBuiltinCommand::joinedReply() const
{
    if (reply.isEmpty())
        return QByteArray();
    QByteArray answer;
    answer.reserve(120  * reply.size());
    foreach (const QByteArray &l, reply) {
        answer += l;
        answer += '\n';
    }
    return answer;
}

// Queue entry for Qt Creator extension commands producing one-line
// output with success flag and error message.
struct CdbExtensionCommand : public CdbCommandBase
{
    typedef CdbEngine::ExtensionCommandHandler CommandHandler;

    CdbExtensionCommand() : success(false) {}
    CdbExtensionCommand(const QByteArray  &cmd, int token, unsigned flags,
                      CommandHandler h,
                      unsigned nc, const QVariant &cookie) :
        CdbCommandBase(cmd, token, flags, nc, cookie), handler(h),success(false) {}

    CommandHandler handler;
    QByteArray reply;
    QByteArray errorMessage;
    bool success;
};

template <class CommandPtrType>
int indexOfCommand(const QList<CommandPtrType> &l, int token)
{
    const int count = l.size();
    for (int i = 0; i < count; i++)
        if (l.at(i)->token == token)
            return i;
    return -1;
}

static inline bool validMode(DebuggerStartMode sm)
{
    switch (sm) {
    case NoStartMode:
    case AttachTcf:
    case AttachCore:
    case StartRemoteGdb:
        return false;
    default:
        break;
    }
    return true;
}

static inline QString msgCdbDisabled(ToolChainType tc)
{
    return CdbEngine::tr("The CDB debug engine required for %1 is currently disabled.").
                      arg(ToolChain::toolChainName(tc));
}

// Accessed by RunControlFactory
DebuggerEngine *createCdbEngine(const DebuggerStartParameters &sp,
    DebuggerEngine *masterEngine, QString *errorMessage)
{
#ifdef Q_OS_WIN
    CdbOptionsPage *op = CdbOptionsPage::instance();
    if (!op || !op->options()->isValid()) {
        *errorMessage = msgCdbDisabled(sp.toolChainType);
        return 0;
    }
    if (!validMode(sp.startMode)) {
        *errorMessage = CdbEngine::tr("The CDB debug engine does not support start mode %1.").arg(sp.startMode);
        return 0;
    }
    return new CdbEngine(sp, masterEngine, op->options());
#else
    Q_UNUSED(masterEngine)
    Q_UNUSED(sp)
#endif
    *errorMessage = QString::fromLatin1("Unsupported debug mode");
    return 0;
}

bool isCdbEngineEnabled()
{
#ifdef Q_OS_WIN
    return CdbOptionsPage::instance() && CdbOptionsPage::instance()->options()->isValid();
#else
    return false;
#endif
}

ConfigurationCheck checkCdbConfiguration(ToolChainType toolChain)
{
    ConfigurationCheck check;
    switch (toolChain) {
    case ToolChain_MinGW: // Do our best
    case ToolChain_MSVC:
    case ToolChain_WINCE:
    case ToolChain_OTHER:
    case ToolChain_UNKNOWN:
    case ToolChain_INVALID:
        if (!isCdbEngineEnabled()) {
            check.errorMessage = msgCdbDisabled(toolChain);
            check.settingsPage = CdbOptionsPage::settingsId();
        }
        break;
    default:
        //: %1 is something like "GCCE" or "Intel C++ Compiler (Linux)" (see ToolChain context)
        check.errorMessage = CdbEngine::tr("The CDB debug engine does not support the %1 toolchain.").
                    arg(ToolChain::toolChainName(toolChain));
        check.settingsPage = CdbOptionsPage::settingsId();
    }
    return check;
}

void addCdbOptionPages(QList<Core::IOptionsPage *> *opts)
{
#ifdef Q_OS_WIN
    opts->push_back(new CdbOptionsPage);
#else
    Q_UNUSED(opts);
#endif
}

#define QT_CREATOR_CDB_EXT "qtcreatorcdbext"

static inline Utils::SavedAction *theAssemblerAction()
{
    return debuggerCore()->action(OperateByInstruction);
}

CdbEngine::CdbEngine(const DebuggerStartParameters &sp,
        DebuggerEngine *masterEngine, const OptionsPtr &options) :
    DebuggerEngine(sp, masterEngine),
    m_creatorExtPrefix("<qtcreatorcdbext>|"),
    m_tokenPrefix("<token>"),
    m_options(options),
    m_effectiveStartMode(NoStartMode),
    m_inferiorPid(0),
    m_accessible(false),
    m_specialStopMode(NoSpecialStop),
    m_nextCommandToken(0),
    m_nextBreakpointNumber(1),
    m_currentBuiltinCommandIndex(-1),
    m_extensionCommandPrefixBA("!"QT_CREATOR_CDB_EXT"."),
    m_operateByInstructionPending(true),
    m_operateByInstruction(true), // Default CDB setting
    m_notifyEngineShutdownOnTermination(false),
    m_hasDebuggee(false),
    m_elapsedLogTime(0),
    m_sourceStepInto(false),
    m_wX86BreakpointCount(0),
    m_watchPointX(0),
    m_watchPointY(0)
{
    Utils::SavedAction *assemblerAction = theAssemblerAction();
    m_operateByInstructionPending = assemblerAction->isChecked();
    connect(assemblerAction, SIGNAL(triggered(bool)), this, SLOT(operateByInstructionTriggered(bool)));

    setObjectName(QLatin1String("CdbEngine"));
    connect(&m_process, SIGNAL(finished(int)), this, SLOT(processFinished()));
    connect(&m_process, SIGNAL(error(QProcess::ProcessError)), this, SLOT(processError()));
    connect(&m_process, SIGNAL(readyReadStandardOutput()), this, SLOT(readyReadStandardOut()));
    connect(&m_process, SIGNAL(readyReadStandardError()), this, SLOT(readyReadStandardOut()));
}

CdbEngine::~CdbEngine()
{
}

void CdbEngine::operateByInstructionTriggered(bool operateByInstruction)
{
    if (state() == InferiorStopOk) {
        syncOperateByInstruction(operateByInstruction);
    } else {
        // To be set next time session becomes accessible
        m_operateByInstructionPending = operateByInstruction;
    }
}

void CdbEngine::syncOperateByInstruction(bool operateByInstruction)
{
    if (m_operateByInstruction == operateByInstruction)
        return;
    QTC_ASSERT(m_accessible, return; )
    m_operateByInstruction = operateByInstruction;
    postCommand(m_operateByInstruction ? QByteArray("l-t") : QByteArray("l+t"), 0);
    postCommand(m_operateByInstruction ? QByteArray("l-s") : QByteArray("l+s"), 0);
}

void CdbEngine::setToolTipExpression(const QPoint &mousePos, TextEditor::ITextEditor *editor, int cursorPos)
{
    if (debug)
        qDebug() << Q_FUNC_INFO;
    // Need a stopped debuggee and a cpp file in a valid frame
    if (state() != InferiorStopOk || !isCppEditor(editor) || stackHandler()->currentIndex() < 0)
        return;
    // Determine expression and function
    int line;
    int column;
    QString function;
    const QString exp = cppExpressionAt(editor, cursorPos, &line, &column, &function);
    // Are we in the current stack frame
    if (function.isEmpty() || exp.isEmpty() || function != stackHandler()->currentFrame().function)
        return;
    // No numerical or any other expressions [yet]
    if (!(exp.at(0).isLetter() || exp.at(0) == QLatin1Char('_')))
        return;
    const QByteArray iname = QByteArray(localsPrefixC) + exp.toAscii();
    const QModelIndex index = watchHandler()->itemIndex(iname);
    if (index.isValid()) {
        showDebuggerToolTip(mousePos, watchHandler()->modelForIName(iname), index.row());
    } else {
        hideDebuggerToolTip();
    }
}

// Determine full path to the CDB extension library.
QString CdbEngine::extensionLibraryName(bool is64Bit)
{
    // Determine extension lib name and path to use
    QString rc;
    QTextStream(&rc) << QFileInfo(QCoreApplication::applicationDirPath()).path()
                     << "/lib/" << (is64Bit ? QT_CREATOR_CDB_EXT"64" : QT_CREATOR_CDB_EXT"32")
                     << '/' << QT_CREATOR_CDB_EXT << ".dll";
    return rc;
}

// Determine environment for CDB.exe, start out with run config and
// add CDB extension path merged with system value should there be one.
static QStringList mergeEnvironment(QStringList runConfigEnvironment,
                                    QString cdbExtensionPath)
{
    // Determine CDB extension path from Qt Creator
    static const char cdbExtensionPathVariableC[] = "_NT_DEBUGGER_EXTENSION_PATH";
    const QByteArray oldCdbExtensionPath = qgetenv(cdbExtensionPathVariableC);
    if (!oldCdbExtensionPath.isEmpty()) {
        cdbExtensionPath.append(QLatin1Char(';'));
        cdbExtensionPath.append(QString::fromLocal8Bit(oldCdbExtensionPath));
    }
    // We do not assume someone sets _NT_DEBUGGER_EXTENSION_PATH in the run
    // config, just to make sure, delete any existing entries
    const QString cdbExtensionPathVariableAssign =
            QLatin1String(cdbExtensionPathVariableC) + QLatin1Char('=');
    for (QStringList::iterator it = runConfigEnvironment.begin(); it != runConfigEnvironment.end() ; ) {
        if (it->startsWith(cdbExtensionPathVariableAssign)) {
            it = runConfigEnvironment.erase(it);
            break;
        } else {
            ++it;
        }
    }
    runConfigEnvironment.append(cdbExtensionPathVariableAssign +
                                QDir::toNativeSeparators(cdbExtensionPath));
    return runConfigEnvironment;
}

int CdbEngine::elapsedLogTime() const
{
    const int elapsed = m_logTime.elapsed();
    const int delta = elapsed - m_elapsedLogTime;
    m_elapsedLogTime = elapsed;
    return delta;
}

// Start the console stub with the sub process. Continue in consoleStubProcessStarted.
bool CdbEngine::startConsole(const DebuggerStartParameters &sp, QString *errorMessage)
{
    if (debug)
        qDebug("startConsole %s", qPrintable(sp.executable));
    m_consoleStub.reset(new Utils::ConsoleProcess);
    m_consoleStub->setMode(Utils::ConsoleProcess::Suspend);
    connect(m_consoleStub.data(), SIGNAL(processMessage(QString, bool)),
            SLOT(consoleStubMessage(QString, bool)));
    connect(m_consoleStub.data(), SIGNAL(processStarted()),
            SLOT(consoleStubProcessStarted()));
    connect(m_consoleStub.data(), SIGNAL(wrapperStopped()),
            SLOT(consoleStubExited()));
    m_consoleStub->setWorkingDirectory(sp.workingDirectory);
    m_consoleStub->setEnvironment(sp.environment);
    if (!m_consoleStub->start(sp.executable, sp.processArgs)) {
        *errorMessage = tr("The console process '%1' could not be started.").arg(sp.executable);
        return false;
    }
    return true;
}

void CdbEngine::consoleStubMessage(const QString &msg, bool isError)
{
    if (debug)
        qDebug("consoleStubProcessMessage() in %s error=%d %s", stateName(state()), isError, qPrintable(msg));
    if (isError) {
        if (state() == EngineSetupRequested) {
            STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyEngineSetupFailed")
            notifyEngineSetupFailed();
        } else {
            STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyEngineIll")
            notifyEngineIll();
        }
        nonModalMessageBox(QMessageBox::Critical, tr("Debugger Error"), msg);
    } else {
        showMessage(msg, AppOutput);
    }
}

void CdbEngine::consoleStubProcessStarted()
{
    if (debug)
        qDebug("consoleStubProcessStarted() PID=%lld", m_consoleStub->applicationPID());
    // Attach to console process.
    DebuggerStartParameters attachParameters = startParameters();
    attachParameters.executable.clear();
    attachParameters.processArgs.clear();
    attachParameters.attachPID = m_consoleStub->applicationPID();
    attachParameters.startMode = AttachExternal;
    showMessage(QString::fromLatin1("Attaching to %1...").arg(attachParameters.attachPID), LogMisc);
    QString errorMessage;
    if (!launchCDB(attachParameters, &errorMessage)) {
        showMessage(errorMessage, LogError);
        STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyEngineSetupFailed")
        notifyEngineSetupFailed();
    }
}

void CdbEngine::consoleStubExited()
{
}

void CdbEngine::setupEngine()
{
    if (debug)
        qDebug(">setupEngine");
    // Nag to add symbol server
    if (CdbSymbolPathListEditor::promptToAddSymbolServer(CdbOptions::settingsGroup(),
                                                         &(m_options->symbolPaths)))
        m_options->toSettings(Core::ICore::instance()->settings());

    m_logTime.start();
    QString errorMessage;
    // Console: Launch the stub with the suspended application and attach to it
    // CDB in theory has a command line option '-2' that launches a
    // console, too, but that immediately closes when the debuggee quits.
    // Use the Creator stub instead.
    const DebuggerStartParameters &sp = startParameters();
    const bool launchConsole = isConsole(sp);
    m_effectiveStartMode = launchConsole ? AttachExternal : sp.startMode;
    const bool ok = launchConsole ?
                startConsole(startParameters(), &errorMessage) :
                launchCDB(startParameters(), &errorMessage);
    if (debug)
        qDebug("<setupEngine ok=%d", ok);
    if (!ok) {
        showMessage(errorMessage, LogError);
        STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyEngineSetupFailed")
        notifyEngineSetupFailed();
    }
}

bool CdbEngine::launchCDB(const DebuggerStartParameters &sp, QString *errorMessage)
{
    if (debug)
        qDebug("launchCDB startMode=%d", sp.startMode);
    const QChar blank(QLatin1Char(' '));
    // Start engine which will run until initial breakpoint:
    // Determine extension lib name and path to use
    // The extension is passed as relative name with the path variable set
    //(does not work with absolute path names)
    const QFileInfo extensionFi(CdbEngine::extensionLibraryName(m_options->is64bit));
    if (!extensionFi.isFile()) {
        *errorMessage = QString::fromLatin1("Internal error: The extension %1 cannot be found.").
                arg(QDir::toNativeSeparators(extensionFi.absoluteFilePath()));
        return false;
    }
    const QString extensionFileName = extensionFi.fileName();
    // Prepare arguments
    QStringList arguments;
    const bool isRemote = sp.startMode == AttachToRemote;
    if (isRemote) { // Must be first
        arguments << QLatin1String("-remote") << sp.remoteChannel;
    } else {
        arguments << (QLatin1String("-a") + extensionFileName);
    }
    // Source line info/No terminal breakpoint / Pull extension
    arguments << QLatin1String("-lines") << QLatin1String("-G")
    // register idle (debuggee stop) notification
              << QLatin1String("-c")
              << QString::fromAscii(".idle_cmd " + m_extensionCommandPrefixBA + "idle");
    if (sp.useTerminal) // Separate console
        arguments << QLatin1String("-2");
    if (!m_options->symbolPaths.isEmpty())
        arguments << QLatin1String("-y") << m_options->symbolPaths.join(QString(QLatin1Char(';')));
    if (!m_options->sourcePaths.isEmpty())
        arguments << QLatin1String("-srcpath") << m_options->sourcePaths.join(QString(QLatin1Char(';')));
    // Compile argument string preserving quotes
    QString nativeArguments = m_options->additionalArguments;
    switch (sp.startMode) {
    case StartInternal:
    case StartExternal:
        if (!nativeArguments.isEmpty())
            nativeArguments.push_back(blank);
        nativeArguments += QDir::toNativeSeparators(sp.executable);
        break;
    case AttachToRemote:
        break;
    case AttachExternal:
    case AttachCrashedExternal:
        arguments << QLatin1String("-p") << QString::number(sp.attachPID);
        if (sp.startMode == AttachCrashedExternal)
            arguments << QLatin1String("-e") << sp.crashParameter << QLatin1String("-g");
        break;
    default:
        *errorMessage = QString::fromLatin1("Internal error: Unsupported start mode %1.").arg(sp.startMode);
        return false;
    }
    if (!sp.processArgs.isEmpty()) { // Complete native argument string.
        if (!nativeArguments.isEmpty())
            nativeArguments.push_back(blank);
        nativeArguments += sp.processArgs;
    }

    const QString executable = m_options->executable;
    const QString msg = QString::fromLatin1("Launching %1 %2\nusing %3 of %4.").
            arg(QDir::toNativeSeparators(executable),
                arguments.join(QString(blank)) + blank + nativeArguments,
                QDir::toNativeSeparators(extensionFi.absoluteFilePath()),
                extensionFi.lastModified().toString(Qt::SystemLocaleShortDate));
    showMessage(msg, LogMisc);

    m_outputBuffer.clear();
    m_process.setEnvironment(mergeEnvironment(sp.environment.toStringList(), extensionFi.absolutePath()));
    if (!sp.workingDirectory.isEmpty())
        m_process.setWorkingDirectory(sp.workingDirectory);

#ifdef Q_OS_WIN
    if (!nativeArguments.isEmpty()) // Appends
        m_process.setNativeArguments(nativeArguments);
#endif
    m_process.start(executable, arguments);
    if (!m_process.waitForStarted()) {
        *errorMessage = QString::fromLatin1("Internal error: Cannot start process %1: %2").
                arg(QDir::toNativeSeparators(executable), m_process.errorString());
        return false;
    }
#ifdef Q_OS_WIN
    const unsigned long pid = Utils::winQPidToPid(m_process.pid());
#else
    const unsigned long pid = 0;
#endif
    showMessage(QString::fromLatin1("%1 running as %2").
                arg(QDir::toNativeSeparators(executable)).arg(pid), LogMisc);
    m_hasDebuggee = true;
    if (isRemote) { // We do not get an 'idle' in a remote session, but are accessible
        m_accessible = true;
        const QByteArray loadCommand = QByteArray(".load ")
                + extensionFileName.toLocal8Bit();
        postCommand(loadCommand, 0);
        STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyEngineSetupOk")
        notifyEngineSetupOk();
    }
    return true;
}

void CdbEngine::setupInferior()
{
    if (debug)
        qDebug("setupInferior");
    attemptBreakpointSynchronization();
    postExtensionCommand("pid", QByteArray(), 0, &CdbEngine::handlePid);
}

void CdbEngine::runEngine()
{
    if (debug)
        qDebug("runEngine");
    // Resume the threads frozen by the console stub.
    if (isConsole(startParameters()))
        postCommand("~* m", 0);
    foreach (const QString &breakEvent, m_options->breakEvents)
            postCommand(QByteArray("sxe ") + breakEvent.toAscii(), 0);
    postCommand("g", 0);
}

bool CdbEngine::commandsPending() const
{
    return !m_builtinCommandQueue.isEmpty() || !m_extensionCommandQueue.isEmpty();
}

void CdbEngine::shutdownInferior()
{
    if (debug)
        qDebug("CdbEngine::shutdownInferior in state '%s', process running %d", stateName(state()),
               isCdbProcessRunning());

    if (!isCdbProcessRunning()) { // Direct launch: Terminated with process.
        if (debug)
            qDebug("notifyInferiorShutdownOk");
        STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyInferiorShutdownOk")
        notifyInferiorShutdownOk();
        return;
    }
    if (!canInterruptInferior() || commandsPending()) {
        STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyInferiorShutdownFailed")
        notifyInferiorShutdownFailed();
        return;
    }

    if (m_accessible) {
        if (m_effectiveStartMode == AttachExternal || m_effectiveStartMode == AttachCrashedExternal)
            detachDebugger();
        STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyInferiorShutdownOk")
        notifyInferiorShutdownOk();
    } else {
        interruptInferior(); // Calls us again
    }
}

/* shutdownEngine/processFinished:
 * Note that in the case of launching a process by the debugger, the debugger
 * automatically quits a short time after reporting the session becoming
 * inaccessible without debuggee (notifyInferiorExited). In that case,
 * processFinished() must not report any arbitrarily notifyEngineShutdownOk()
 * as not to confuse the state engine.
 */

void CdbEngine::shutdownEngine()
{
    if (debug)
        qDebug("CdbEngine::shutdownEngine in state '%s', process running %d,"
               "accessible=%d,commands pending=%d",
               stateName(state()), isCdbProcessRunning(), m_accessible,
               commandsPending());

    if (!isCdbProcessRunning()) { // Direct launch: Terminated with process.
        STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyEngineShutdownOk")
        notifyEngineShutdownOk();
        return;
    }

    // No longer trigger anything from messages
    disconnect(&m_process, SIGNAL(readyReadStandardOutput()), this, 0);
    disconnect(&m_process, SIGNAL(readyReadStandardError()), this, 0);
    // Go for kill if there are commands pending.
    if (m_accessible && !commandsPending()) {
        // detach: Wait for debugger to finish.
        if (m_effectiveStartMode == AttachExternal)
            detachDebugger();
        // Remote requires a bit more force to quit.
        if (m_effectiveStartMode == AttachToRemote) {
            postCommand(m_extensionCommandPrefixBA + "shutdownex", 0);
            postCommand("qq", 0);
        } else {
            postCommand("q", 0);
        }
        m_notifyEngineShutdownOnTermination = true;
        return;
    } else {
        // Remote process. No can do, currently
        m_notifyEngineShutdownOnTermination = true;
        Utils::SynchronousProcess::stopProcess(m_process);
        return;
    }
    // Lost debuggee, debugger should quit anytime now
    if (!m_hasDebuggee) {
        m_notifyEngineShutdownOnTermination = true;
        return;
    }
    interruptInferior();
}

void CdbEngine::processFinished()
{
    if (debug)
        qDebug("CdbEngine::processFinished %dms '%s' notify=%d (exit state=%d, ex=%d)",
               elapsedLogTime(), stateName(state()), m_notifyEngineShutdownOnTermination,
               m_process.exitStatus(), m_process.exitCode());

    const bool crashed = m_process.exitStatus() == QProcess::CrashExit;
    if (crashed) {
        showMessage(tr("CDB crashed"), LogError); // not in your life.
    } else {
        showMessage(tr("CDB exited (%1)").arg(m_process.exitCode()), LogMisc);
    }

    if (m_notifyEngineShutdownOnTermination) {
        if (crashed) {
            if (debug)
                qDebug("notifyEngineIll");
            STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyEngineIll")
            notifyEngineIll();
        } else {
            STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyEngineShutdownOk")
            notifyEngineShutdownOk();
        }
    } else {
        STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyEngineSpontaneousShutdown")
        notifyEngineSpontaneousShutdown();
    }
}

void CdbEngine::detachDebugger()
{
    postCommand(".detach", 0);
}

static inline bool isWatchIName(const QByteArray &iname)
{
    return iname.startsWith("watch");
}

void CdbEngine::updateWatchData(const WatchData &dataIn,
                                const WatchUpdateFlags & flags)
{
    if (debug || debugLocals || debugWatches)
        qDebug("CdbEngine::updateWatchData() %dms accessible=%d %s incr=%d: %s",
               elapsedLogTime(), m_accessible, stateName(state()),
               flags.tryIncremental,
               qPrintable(dataIn.toString()));

    if (!m_accessible) // Add watch data while running?
        return;

    // New watch item?
    if (isWatchIName(dataIn.iname) && dataIn.isValueNeeded()) {
        QByteArray args;
        ByteArrayInputStream str(args);
        str << dataIn.iname << " \"" << dataIn.exp << '"';
        postExtensionCommand("addwatch", args, 0,
                             &CdbEngine::handleAddWatch, 0,
                             qVariantFromValue(dataIn));
        return;
    }

    if (!dataIn.hasChildren && !dataIn.isValueNeeded()) {
        WatchData data = dataIn;
        data.setAllUnneeded();
        watchHandler()->insertData(data);
        return;
    }
    updateLocalVariable(dataIn.iname);
}

void CdbEngine::handleAddWatch(const CdbExtensionCommandPtr &reply)
{
    WatchData item = qvariant_cast<WatchData>(reply->cookie);
    if (debugWatches)
        qDebug() << "handleAddWatch ok="  << reply->success << item.iname;
    if (reply->success) {
        updateLocalVariable(item.iname);
    } else {
        item.setError(tr("Unable to add expression"));
        watchHandler()->insertData(item);
        showMessage(QString::fromLatin1("Unable to add watch item '%1'/'%2': %3").
                    arg(QString::fromAscii(item.iname), QString::fromAscii(item.exp),
                        reply->errorMessage), LogError);
    }
}

void CdbEngine::addLocalsOptions(ByteArrayInputStream &str) const
{
    if (debuggerCore()->boolSetting(VerboseLog))
        str << blankSeparator << "-v";
    if (debuggerCore()->boolSetting(UseDebuggingHelpers))
        str << blankSeparator << "-c";
    const QByteArray typeFormats = watchHandler()->typeFormatRequests();
    if (!typeFormats.isEmpty())
        str << blankSeparator << "-T " << typeFormats;
    const QByteArray individualFormats = watchHandler()->individualFormatRequests();
    if (!individualFormats.isEmpty())
        str << blankSeparator << "-I " << individualFormats;
}

void CdbEngine::updateLocalVariable(const QByteArray &iname)
{
    const bool isWatch = isWatchIName(iname);
    if (debugWatches)
        qDebug() << "updateLocalVariable watch=" << isWatch << iname;
    QByteArray localsArguments;
    ByteArrayInputStream str(localsArguments);
    addLocalsOptions(str);
    if (!isWatch) {
        const int stackFrame = stackHandler()->currentIndex();
        if (stackFrame < 0) {
            qWarning("Internal error; no stack frame in updateLocalVariable");
            return;
        }
        str << blankSeparator << stackFrame;
    }
    str << blankSeparator << iname;
    postExtensionCommand(isWatch ? "watches" : "locals", localsArguments, 0, &CdbEngine::handleLocals);
}

unsigned CdbEngine::debuggerCapabilities() const
{
    return DisassemblerCapability | RegisterCapability | ShowMemoryCapability
           |WatchpointCapability|JumpToLineCapability|AddWatcherCapability
           |BreakOnThrowAndCatchCapability; // Sort-of: Can break on throw().
}

void CdbEngine::executeStep()
{
    if (!m_operateByInstruction)
        m_sourceStepInto = true; // See explanation at handleStackTrace().
    postCommand(QByteArray("t"), 0); // Step into-> t (trace)
    STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyInferiorRunRequested")
    notifyInferiorRunRequested();
}

void CdbEngine::executeStepOut()
{
    postCommand(QByteArray("gu"), 0); // Step out-> gu (go up)
    STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyInferiorRunRequested")
    notifyInferiorRunRequested();
}

void CdbEngine::executeNext()
{
    postCommand(QByteArray("p"), 0); // Step over -> p
    STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyInferiorRunRequested")
    notifyInferiorRunRequested();
}

void CdbEngine::executeStepI()
{
    executeStep();
}

void CdbEngine::executeNextI()
{
    executeNext();
}

void CdbEngine::continueInferior()
{
    STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyInferiorRunRequested")
    notifyInferiorRunRequested();
    doContinueInferior();
}

void CdbEngine::doContinueInferior()
{
    postCommand(QByteArray("g"), 0);
}

bool CdbEngine::canInterruptInferior() const
{
    return m_effectiveStartMode != AttachToRemote;
}

void CdbEngine::interruptInferior()
{
    if (canInterruptInferior()) {
        doInterruptInferior(NoSpecialStop);
    } else {
        showMessage(tr("Interrupting is not possible in remote sessions."), LogError);
        STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyInferiorStopOk")
        notifyInferiorStopOk();
        STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyInferiorRunRequested")
        notifyInferiorRunRequested();
        STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyInferiorRunOk")
        notifyInferiorRunOk();
    }
}

void CdbEngine::doInterruptInferior(SpecialStopMode sm)
{
#ifdef Q_OS_WIN
    const SpecialStopMode oldSpecialMode = m_specialStopMode;
    m_specialStopMode = sm;
    QString errorMessage;
    if (!winDebugBreakProcess(m_inferiorPid, &errorMessage)) {
        m_specialStopMode = oldSpecialMode;
        showMessage(errorMessage, LogError);
    }
#else
    Q_UNUSED(sm)
#endif
}

void CdbEngine::executeRunToLine(const QString &fileName, int lineNumber)
{
    // Add one-shot breakpoint
    BreakpointParameters bp(BreakpointByFileAndLine);
    bp.fileName = fileName;
    bp.lineNumber = lineNumber;
    postCommand(cdbAddBreakpointCommand(bp, true), 0);
    continueInferior();
}

void CdbEngine::executeRunToFunction(const QString &functionName)
{
    // Add one-shot breakpoint
    BreakpointParameters bp(BreakpointByFunction);
    bp.functionName = functionName;

    postCommand(cdbAddBreakpointCommand(bp, true), 0);
    continueInferior();
}

void CdbEngine::setRegisterValue(int regnr, const QString &value)
{
    const Registers registers = registerHandler()->registers();
    QTC_ASSERT(regnr < registers.size(), return)
    // Value is decimal or 0x-hex-prefixed
    QByteArray cmd;
    ByteArrayInputStream str(cmd);
    str << "r " << registers.at(regnr).name << '=' << value;
    postCommand(cmd, 0);
    reloadRegisters();
}

void CdbEngine::executeJumpToLine(const QString & fileName, int lineNumber)
{
    QByteArray cmd;
    ByteArrayInputStream str(cmd);
    // Resolve source line address and go to that location
    str << "? `" << QDir::toNativeSeparators(fileName) << ':' << lineNumber << '`';
    const QVariant cookie = qVariantFromValue(SourceLocationCookie(fileName, lineNumber));
    postBuiltinCommand(cmd, 0, &CdbEngine::handleJumpToLineAddressResolution, 0, cookie);
}

void CdbEngine::handleJumpToLineAddressResolution(const CdbBuiltinCommandPtr &cmd)
{
    if (cmd->reply.isEmpty())
        return;
    // Evaluate expression: 5365511549 = 00000001`3fcf357d
    // Set register 'rip' to hex address and goto lcoation
    QByteArray answer = cmd->reply.front();
    const int equalPos = answer.indexOf(" = ");
    if (equalPos == -1)
        return;
    answer.remove(0, equalPos + 3);
    QTC_ASSERT(qVariantCanConvert<SourceLocationCookie>(cmd->cookie), return ; )
    const SourceLocationCookie cookie = qvariant_cast<SourceLocationCookie>(cmd->cookie);

    QByteArray registerCmd;
    ByteArrayInputStream str(registerCmd);
    // PC-register depending on 64/32bit.
    str << "r " << (m_options->is64bit ? "rip" : "eip") << "=0x" << answer;
    postCommand(registerCmd, 0);
    gotoLocation(Location(cookie.fileName, cookie.lineNumber));
}

void CdbEngine::assignValueInDebugger(const WatchData *w, const QString &expr, const QVariant &value)
{
    if (debug)
        qDebug() << "CdbEngine::assignValueInDebugger" << w->iname << expr << value;

    if (state() != InferiorStopOk || stackHandler()->currentIndex() < 0) {
        qWarning("Internal error: assignValueInDebugger: Invalid state or no stack frame.");
        return;
    }

    QByteArray cmd;
    ByteArrayInputStream str(cmd);
    str << m_extensionCommandPrefixBA << "assign " << w->iname << '=' << value.toString();
    postCommand(cmd, 0);
    // Update all locals in case we change a union or something pointed to
    // that affects other variables, too.
    updateLocals();
}

void CdbEngine::parseThreads(const GdbMi &data, int forceCurrentThreadId /* = -1 */)
{
    int currentThreadId;
    Threads threads = ThreadsHandler::parseGdbmiThreads(data, &currentThreadId);
    threadsHandler()->setThreads(threads);
    threadsHandler()->setCurrentThreadId(forceCurrentThreadId >= 0 ?
                                         forceCurrentThreadId : currentThreadId);
}

void CdbEngine::handleThreads(const CdbExtensionCommandPtr &reply)
{
    if (debug)
        qDebug("CdbEngine::handleThreads success=%d", reply->success);
    if (reply->success) {
        GdbMi data;
        data.fromString(reply->reply);
        parseThreads(data);
        // Continue sequence
        postCommandSequence(reply->commandSequence);
    } else {
        showMessage(QString::fromLatin1(reply->errorMessage), LogError);
    }
}

void CdbEngine::executeDebuggerCommand(const QString &command)
{
    postCommand(command.toLocal8Bit(), QuietCommand);
}

// Post command without callback
void CdbEngine::postCommand(const QByteArray &cmd, unsigned flags)
{
    if (debug)
        qDebug("CdbEngine::postCommand %dms '%s' %u %s\n",
               elapsedLogTime(), cmd.constData(), flags, stateName(state()));
    if (!(flags & QuietCommand))
        showMessage(QString::fromLocal8Bit(cmd), LogInput);
    m_process.write(cmd + '\n');
}

// Post a built-in-command producing free-format output with a callback.
// In order to catch the output, it is enclosed in 'echo' commands
// printing a specially formatted token to be identifiable in the output.
void CdbEngine::postBuiltinCommand(const QByteArray &cmd, unsigned flags,
                                   BuiltinCommandHandler handler,
                                   unsigned nextCommandFlag,
                                   const QVariant &cookie)
{
    if (!m_accessible) {
        const QString msg = QString::fromLatin1("Attempt to issue builtin command '%1' to non-accessible session (%2)")
                .arg(QString::fromLocal8Bit(cmd), QString::fromAscii(stateName(state())));
        showMessage(msg, LogError);
        return;
    }
    if (!flags & QuietCommand)
        showMessage(QString::fromLocal8Bit(cmd), LogInput);

    const int token = m_nextCommandToken++;
    CdbBuiltinCommandPtr pendingCommand(new CdbBuiltinCommand(cmd, token, flags, handler, nextCommandFlag, cookie));

    m_builtinCommandQueue.push_back(pendingCommand);
    // Enclose command in echo-commands for token
    QByteArray fullCmd;
    ByteArrayInputStream str(fullCmd);
    str << ".echo \"" << m_tokenPrefix << token << "<\"\n"
            << cmd << "\n.echo \"" << m_tokenPrefix << token << ">\"\n";
    if (debug)
        qDebug("CdbEngine::postBuiltinCommand %dms '%s' flags=%u token=%d %s next=%u, cookie='%s', pending=%d, sequence=0x%x",
               elapsedLogTime(), cmd.constData(), flags, token, stateName(state()), nextCommandFlag, qPrintable(cookie.toString()),
               m_builtinCommandQueue.size(), nextCommandFlag);
    if (debug > 1)
        qDebug("CdbEngine::postBuiltinCommand: resulting command '%s'\n",
               fullCmd.constData());
    m_process.write(fullCmd);
}

// Post an extension command producing one-line output with a callback,
// pass along token for identification in queue.
void CdbEngine::postExtensionCommand(const QByteArray &cmd,
                                     const QByteArray &arguments,
                                     unsigned flags,
                                     ExtensionCommandHandler handler,
                                     unsigned nextCommandFlag,
                                     const QVariant &cookie)
{
    if (!m_accessible) {
        const QString msg = QString::fromLatin1("Attempt to issue extension command '%1' to non-accessible session (%2)")
                .arg(QString::fromLocal8Bit(cmd), QString::fromAscii(stateName(state())));
        showMessage(msg, LogError);
        return;
    }

    const int token = m_nextCommandToken++;

    // Format full command with token to be recognizeable in the output
    QByteArray fullCmd;
    ByteArrayInputStream str(fullCmd);
    str << m_extensionCommandPrefixBA << cmd << " -t " << token;
    if (!arguments.isEmpty())
        str <<  ' ' << arguments;

    if (!flags & QuietCommand)
        showMessage(QString::fromLocal8Bit(fullCmd), LogInput);

    CdbExtensionCommandPtr pendingCommand(new CdbExtensionCommand(fullCmd, token, flags, handler, nextCommandFlag, cookie));

    m_extensionCommandQueue.push_back(pendingCommand);
    // Enclose command in echo-commands for token
    if (debug)
        qDebug("CdbEngine::postExtensionCommand %dms '%s' flags=%u token=%d %s next=%u, cookie='%s', pending=%d, sequence=0x%x",
               elapsedLogTime(), fullCmd.constData(), flags, token, stateName(state()), nextCommandFlag, qPrintable(cookie.toString()),
               m_extensionCommandQueue.size(), nextCommandFlag);
    m_process.write(fullCmd + '\n');
}

void CdbEngine::activateFrame(int index)
{
    // TODO: assembler,etc
    if (index < 0)
        return;
    const StackFrames &frames = stackHandler()->frames();
    QTC_ASSERT(index < frames.size(), return; )

    const StackFrame frame = frames.at(index);
    if (debug || debugLocals)
        qDebug("activateFrame idx=%d '%s' %d", index,
               qPrintable(frame.file), frame.line);
    stackHandler()->setCurrentIndex(index);
    const bool showAssembler = !frames.at(index).isUsable();
    if (showAssembler) { // Assembly code: Clean out model and force instruction mode.
        watchHandler()->beginCycle();
        watchHandler()->endCycle();
        QAction *assemblerAction = theAssemblerAction();
        if (assemblerAction->isChecked()) {
            gotoLocation(frame);
        } else {
            assemblerAction->trigger(); // Seems to trigger update
        }
    } else {
        gotoLocation(frame);
        updateLocals();
    }
}

void CdbEngine::updateLocals()
{
    typedef QHash<QByteArray, int> WatcherHash;

    const int frameIndex = stackHandler()->currentIndex();
    if (frameIndex < 0) {
        watchHandler()->beginCycle();
        watchHandler()->endCycle();
        return;
    }
    const StackFrame frame = stackHandler()->currentFrame();
    if (!frame.isUsable()) {
        watchHandler()->beginCycle();
        watchHandler()->endCycle();
        return;
    }
    /* Watchers: Forcibly discard old symbol group as switching from
     * thread 0/frame 0 -> thread 1/assembly -> thread 0/frame 0 will otherwise re-use it
     * and cause errors as it seems to go 'stale' when switching threads.
     * Initial expand, get uninitialized and query */
    QByteArray arguments;
    ByteArrayInputStream str(arguments);
    str << "-D";
    // Pre-expand
    const QSet<QByteArray> expanded = watchHandler()->expandedINames();
    if (!expanded.isEmpty()) {
        str << blankSeparator << "-e ";
        int i = 0;
        foreach(const QByteArray &e, expanded) {
            if (i++)
                str << ',';
            str << e;
        }
    }
    addLocalsOptions(str);
    // Uninitialized variables if desired
    if (debuggerCore()->boolSetting(UseCodeModel)) {
        QStringList uninitializedVariables;
        getUninitializedVariables(debuggerCore()->cppCodeModelSnapshot(),
                                  frame.function, frame.file, frame.line, &uninitializedVariables);
        if (!uninitializedVariables.isEmpty()) {
            str << blankSeparator << "-u ";
            int i = 0;
            foreach(const QString &u, uninitializedVariables) {
                if (i++)
                    str << ',';
                str << localsPrefixC << u;
            }
        }
    }
    // Perform watches synchronization
    str << blankSeparator << "-W";
    const WatcherHash watcherHash = WatchHandler::watcherNames();
    if (!watcherHash.isEmpty()) {
        const WatcherHash::const_iterator cend = watcherHash.constEnd();
        for (WatcherHash::const_iterator it = watcherHash.constBegin(); it != cend; ++it) {
            str << blankSeparator << "-w " << it.value() << " \"" << it.key() << '"';
        }
    }

    // Required arguments: frame
    str << blankSeparator << frameIndex;
    watchHandler()->beginCycle();
    postExtensionCommand("locals", arguments, 0, &CdbEngine::handleLocals);
}

void CdbEngine::selectThread(int index)
{
    if (index < 0 || index == threadsHandler()->currentThread())
        return;

    resetLocation();
    const int newThreadId = threadsHandler()->threads().at(index).id;
    threadsHandler()->setCurrentThread(index);

    const QByteArray cmd = "~" + QByteArray::number(newThreadId) + " s";
    postBuiltinCommand(cmd, 0, &CdbEngine::dummyHandler, CommandListStack);
}

void CdbEngine::fetchDisassembler(DisassemblerAgent *agent)
{
    QTC_ASSERT(m_accessible, return;)
    QByteArray cmd;
    ByteArrayInputStream str(cmd);
    str <<  "u " << hex << hexPrefixOn << agent->address() << " L40";
    const QVariant cookie = qVariantFromValue<DisassemblerAgent*>(agent);
    postBuiltinCommand(cmd, 0, &CdbEngine::handleDisassembler, 0, cookie);
}

// Parse: "00000000`77606060 cc              int     3"
void CdbEngine::handleDisassembler(const CdbBuiltinCommandPtr &command)
{
    QTC_ASSERT(qVariantCanConvert<DisassemblerAgent*>(command->cookie), return;)
    DisassemblerAgent *agent = qvariant_cast<DisassemblerAgent*>(command->cookie);
    DisassemblerLines disassemblerLines;
    foreach(const QByteArray &line, command->reply)
        disassemblerLines.appendLine(DisassemblerLine(QString::fromLatin1(line)));
    agent->setContents(disassemblerLines);
}

void CdbEngine::fetchMemory(MemoryAgent *agent, QObject *editor, quint64 addr, quint64 length)
{
    if (!m_accessible) {
        qWarning("Internal error: Attempt to read memory from inaccessible session: %s", Q_FUNC_INFO);
        return;
    }
    if (debug)
        qDebug("CdbEngine::fetchMemory %llu bytes from 0x%llx", length, addr);

    QByteArray args;
    ByteArrayInputStream str(args);
    str << addr << ' ' << length;
    const QVariant cookie = qVariantFromValue<MemoryViewCookie>(MemoryViewCookie(agent, editor, addr, length));
    postExtensionCommand("memory", args, 0, &CdbEngine::handleMemory, 0, cookie);
}

void CdbEngine::handleMemory(const CdbExtensionCommandPtr &command)
{
    QTC_ASSERT(qVariantCanConvert<MemoryViewCookie>(command->cookie), return;)
    const MemoryViewCookie memViewCookie = qvariant_cast<MemoryViewCookie>(command->cookie);
    if (command->success) {
        const QByteArray data = QByteArray::fromBase64(command->reply);
        if (unsigned(data.size()) == memViewCookie.length)
            memViewCookie.agent->addLazyData(memViewCookie.editorToken,
                                             memViewCookie.address, data);
    } else {
        showMessage(QString::fromLocal8Bit(command->errorMessage), LogError);
    }
}

void CdbEngine::reloadModules()
{
    postCommandSequence(CommandListModules);
}

void CdbEngine::loadSymbols(const QString & /* moduleName */)
{
}

void CdbEngine::loadAllSymbols()
{
}

void CdbEngine::requestModuleSymbols(const QString &moduleName)
{
    Q_UNUSED(moduleName)
}

void CdbEngine::reloadRegisters()
{
    postCommandSequence(CommandListRegisters);
}

void CdbEngine::reloadSourceFiles()
{
}

void CdbEngine::reloadFullStack()
{
    if (debug)
        qDebug("%s", Q_FUNC_INFO);
    postCommandSequence(CommandListStack);
}

void CdbEngine::handlePid(const CdbExtensionCommandPtr &reply)
{
    if (reply->success) {
        m_inferiorPid = reply->reply.toUInt();
        showMessage(QString::fromLatin1("Inferior pid: %1.").arg(m_inferiorPid), LogMisc);
        STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyInferiorSetupOk")
        notifyInferiorSetupOk();
    }  else {
        showMessage(QString::fromLatin1("Failed to determine inferior pid: %1").
                    arg(QLatin1String(reply->errorMessage)), LogError);
        STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyInferiorSetupFailed")
        notifyInferiorSetupFailed();
    }
}

// Parse CDB gdbmi register syntax
static inline Register parseRegister(const GdbMi &gdbmiReg)
{
    Register reg;
    reg.name = gdbmiReg.findChild("name").data();
    const GdbMi description = gdbmiReg.findChild("description");
    if (description.type() != GdbMi::Invalid) {
        reg.name += " (";
        reg.name += description.data();
        reg.name += ')';
    }
    reg.value = QString::fromAscii(gdbmiReg.findChild("value").data());
    return reg;
}

void CdbEngine::handleModules(const CdbExtensionCommandPtr &reply)
{
    if (reply->success) {
        GdbMi value;
        value.fromString(reply->reply);
        if (value.type() == GdbMi::List) {
            Modules modules;
            modules.reserve(value.childCount());
            foreach (const GdbMi &gdbmiModule, value.children()) {
                Module module;
                module.moduleName = QString::fromAscii(gdbmiModule.findChild("name").data());
                module.modulePath = QString::fromAscii(gdbmiModule.findChild("image").data());
                module.startAddress = gdbmiModule.findChild("start").data().toULongLong(0, 0);
                module.endAddress = gdbmiModule.findChild("end").data().toULongLong(0, 0);
                if (gdbmiModule.findChild("deferred").type() == GdbMi::Invalid)
                    module.symbolsRead = Module::ReadOk;
                modules.push_back(module);
            }
            modulesHandler()->setModules(modules);
        } else {
            showMessage(QString::fromLatin1("Parse error in modules response."), LogError);
            qWarning("Parse error in modules response:\n%s", reply->reply.constData());
        }
    }  else {
        showMessage(QString::fromLatin1("Failed to determine modules: %1").
                    arg(QLatin1String(reply->errorMessage)), LogError);
    }
    postCommandSequence(reply->commandSequence);

}

void CdbEngine::handleRegisters(const CdbExtensionCommandPtr &reply)
{
    if (reply->success) {
        GdbMi value;
        value.fromString(reply->reply);
        if (value.type() == GdbMi::List) {
            Registers registers;
            registers.reserve(value.childCount());
            foreach (const GdbMi &gdbmiReg, value.children())
                registers.push_back(parseRegister(gdbmiReg));
            registerHandler()->setRegisters(registers);
        } else {
            showMessage(QString::fromLatin1("Parse error in registers response."), LogError);
            qWarning("Parse error in registers response:\n%s", reply->reply.constData());
        }
    }  else {
        showMessage(QString::fromLatin1("Failed to determine registers: %1").
                    arg(QLatin1String(reply->errorMessage)), LogError);
    }
    postCommandSequence(reply->commandSequence);
}

void CdbEngine::handleLocals(const CdbExtensionCommandPtr &reply)
{
    if (reply->success) {
        QList<WatchData> watchData;
        GdbMi root;
        root.fromString(reply->reply);
        QTC_ASSERT(root.isList(), return ; )
        if (debugLocals) {
            qDebug() << root.toString(true, 4);
        }
        // Courtesy of GDB engine
        foreach (const GdbMi &child, root.children()) {
            WatchData dummy;
            dummy.iname = child.findChild("iname").data();
            dummy.name = QLatin1String(child.findChild("name").data());
            parseWatchData(watchHandler()->expandedINames(), dummy, child, &watchData);
        }
        watchHandler()->insertBulkData(watchData);
        watchHandler()->endCycle();
        if (debugLocals) {
            QDebug nsp = qDebug().nospace();
            nsp << "Obtained " << watchData.size() << " items:\n";
            foreach (const WatchData &wd, watchData)
                nsp << wd.toString() <<'\n';
        }
    } else {
        showMessage(QString::fromLatin1(reply->errorMessage), LogError);
    }
}

void CdbEngine::handleExpandLocals(const CdbExtensionCommandPtr &reply)
{
    if (!reply->success)
        showMessage(QString::fromLatin1(reply->errorMessage), LogError);
}

enum CdbExecutionStatus {
CDB_STATUS_NO_CHANGE=0, CDB_STATUS_GO = 1, CDB_STATUS_GO_HANDLED = 2,
CDB_STATUS_GO_NOT_HANDLED = 3, CDB_STATUS_STEP_OVER = 4,
CDB_STATUS_STEP_INTO = 5, CDB_STATUS_BREAK = 6, CDB_STATUS_NO_DEBUGGEE = 7,
CDB_STATUS_STEP_BRANCH = 8, CDB_STATUS_IGNORE_EVENT = 9,
CDB_STATUS_RESTART_REQUESTED = 10, CDB_STATUS_REVERSE_GO = 11,
CDB_STATUS_REVERSE_STEP_BRANCH = 12, CDB_STATUS_REVERSE_STEP_OVER = 13,
CDB_STATUS_REVERSE_STEP_INTO = 14 };

static const char *cdbStatusName(unsigned long s)
{
    switch (s) {
    case CDB_STATUS_NO_CHANGE:
        return "No change";
    case CDB_STATUS_GO:
        return "go";
    case CDB_STATUS_GO_HANDLED:
        return "go_handled";
    case CDB_STATUS_GO_NOT_HANDLED:
        return "go_not_handled";
    case CDB_STATUS_STEP_OVER:
        return "step_over";
    case CDB_STATUS_STEP_INTO:
        return "step_into";
    case CDB_STATUS_BREAK:
        return "break";
    case CDB_STATUS_NO_DEBUGGEE:
        return "no_debuggee";
    case CDB_STATUS_STEP_BRANCH:
        return "step_branch";
    case CDB_STATUS_IGNORE_EVENT:
        return "ignore_event";
    case CDB_STATUS_RESTART_REQUESTED:
        return "restart_requested";
    case CDB_STATUS_REVERSE_GO:
        return "reverse_go";
    case CDB_STATUS_REVERSE_STEP_BRANCH:
        return "reverse_step_branch";
    case CDB_STATUS_REVERSE_STEP_OVER:
        return "reverse_step_over";
    case CDB_STATUS_REVERSE_STEP_INTO:
        return "reverse_step_into";
    }
    return "unknown";
}

/* Examine how to react to a stop. */
enum StopActionFlags
{
    // Report options
    StopReportLog = 0x1,
    StopReportStatusMessage = 0x2,
    StopReportParseError = 0x4,
    StopShowExceptionMessageBox = 0x8,
    // Notify stop or just continue
    StopNotifyStop = 0x10,
    StopIgnoreContinue = 0x20,
    // Hit on break in artificial stop thread (created by DebugBreak()).
    StopInArtificialThread = 0x40
};

unsigned CdbEngine::examineStopReason(const GdbMi &stopReason,
                                      QString *message,
                                      QString *exceptionBoxMessage)
{
    // Report stop reason (GDBMI)
    if (debug)
        qDebug("%s", stopReason.toString(true, 4).constData());
    const QByteArray reason = stopReason.findChild("reason").data();
    if (reason.isEmpty()) {
        *message = tr("Malformed stop response received.");
        return StopReportParseError|StopNotifyStop;
    }
    // Additional stop messages occurring for debuggee function calls (widgetAt, etc). Just log.
    if (state() == InferiorStopOk) {
        *message = QString::fromLatin1("Ignored stop notification from function call (%1).").
                    arg(QString::fromAscii(reason));
        return StopReportLog;
    }
    const int threadId = stopReason.findChild("threadId").data().toInt();
    if (reason == "breakpoint") {
        const int number = stopReason.findChild("breakpointId").data().toInt();
        const BreakpointId id = breakHandler()->findBreakpointByNumber(number);
        if (id && breakHandler()->type(id) == Watchpoint) {
            *message = msgWatchpointTriggered(id, number, breakHandler()->address(id), QString::number(threadId));
        } else {
            *message = msgBreakpointTriggered(id, number, QString::number(threadId));
        }
        return StopReportStatusMessage|StopNotifyStop;
    }
    if (reason == "exception") {
        WinException exception;
        exception.fromGdbMI(stopReason);
        QString description = exception.toString();
#ifdef Q_OS_WIN
        // It is possible to hit on a startup trap while stepping (if something
        // pulls DLLs. Avoid showing a 'stopped' Message box.
        if (exception.exceptionCode == winExceptionStartupCompleteTrap)
            return StopNotifyStop;
        // WOW 64 breakpoint: Report in log and continue the first one,
        // subsequent ones are caused by interrupting the application.
        if (exception.exceptionCode == winExceptionWX86Breakpoint) {
            if (m_wX86BreakpointCount++) {
                *message = tr("Interrupted (%1)").arg(description);
                return StopReportStatusMessage|StopNotifyStop;
            } else {
                *message = description;
                return StopIgnoreContinue|StopReportLog;
            }
        }
        if (exception.exceptionCode == winExceptionCtrlPressed) {
            // Detect interruption by pressing Ctrl in a console and force a switch to thread 0.
            *message = msgInterrupted();
            return StopReportStatusMessage|StopNotifyStop|StopInArtificialThread;
        }
        if (isDebuggerWinException(exception.exceptionCode)) {
            unsigned rc = StopReportStatusMessage|StopNotifyStop;
            // Detect interruption by DebugBreak() and force a switch to thread 0.
            if (exception.function == "ntdll!DbgBreakPoint")
                rc |= StopInArtificialThread;
            *message = msgInterrupted();
            return rc;
        }
#endif
        *exceptionBoxMessage = msgStoppedByException(description, QString::number(threadId));
        *message = description;
        return StopShowExceptionMessageBox|StopReportStatusMessage|StopNotifyStop;
    }
    *message = msgStopped(QLatin1String(reason));
    return StopReportStatusMessage|StopNotifyStop;
}

void CdbEngine::handleSessionIdle(const QByteArray &messageBA)
{
    if (!m_hasDebuggee)
        return;

    if (debug)
        qDebug("CdbEngine::handleSessionIdle %dms '%s' in state '%s', special mode %d",
               elapsedLogTime(), messageBA.constData(),
               stateName(state()), m_specialStopMode);

    // Switch source level debugging
    syncOperateByInstruction(m_operateByInstructionPending);

    // Engine-special stop reasons: Breakpoints and setup
    const SpecialStopMode specialStopMode =  m_specialStopMode;
    m_specialStopMode = NoSpecialStop;

    switch(specialStopMode) {
    case SpecialStopSynchronizeBreakpoints:
        if (debug)
            qDebug("attemptBreakpointSynchronization in special stop");
        attemptBreakpointSynchronization();
        doContinueInferior();
        return;
    case SpecialStopGetWidgetAt:
        postWidgetAtCommand();
        return;
    case NoSpecialStop:
        break;
    }

    if (state() == EngineSetupRequested) { // Temporary stop at beginning
        STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyEngineSetupOk")
        notifyEngineSetupOk();
        return;
    }

    // Further examine stop and report to user
    QString message;
    QString exceptionBoxMessage;
    GdbMi stopReason;
    stopReason.fromString(messageBA);
    int forcedThreadId = -1;
    const unsigned stopFlags = examineStopReason(stopReason, &message, &exceptionBoxMessage);
    // Do the non-blocking log reporting
    if (stopFlags & StopReportLog)
        showMessage(message, LogMisc);
    if (stopFlags & StopReportStatusMessage)
        showStatusMessage(message);
    if (stopFlags & StopReportParseError)
        showMessage(message, LogError);
    // Ignore things like WOW64
    if (stopFlags & StopIgnoreContinue) {
        postCommand("g", 0);
        return;
    }
    // Notify about state and send off command sequence to get stack, etc.
    if (stopFlags & StopNotifyStop) {
        if (state() == InferiorStopRequested) {
            STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyInferiorStopOk")
            notifyInferiorStopOk();
        } else {
            STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyInferiorSpontaneousStop")
            notifyInferiorSpontaneousStop();
        }
        const bool sourceStepInto = m_sourceStepInto;
        m_sourceStepInto = false;
        // Start sequence to get all relevant data.
        if (stopFlags & StopInArtificialThread) {
            showMessage(tr("Switching to main thread..."), LogMisc);
            postCommand("~0 s", 0);
            forcedThreadId = 0;
            // Re-fetch stack again.
            postCommandSequence(CommandListStack);
        } else {
            const GdbMi stack = stopReason.findChild("stack");
            if (stack.isValid()) {
                if (parseStackTrace(stack, sourceStepInto) & ParseStackStepInto) {
                    executeStep(); // Hit on a frame while step into, see parseStackTrace().
                    return;
                }
            } else {
                showMessage(QString::fromAscii(stopReason.findChild("stackerror").data()), LogError);
            }
        }
        const GdbMi threads = stopReason.findChild("threads");
        if (threads.isValid()) {
            parseThreads(threads, forcedThreadId);
        } else {
            showMessage(QString::fromAscii(stopReason.findChild("threaderror").data()), LogError);
        }
        // Fire off remaining commands asynchronously
        if (debuggerCore()->isDockVisible(QLatin1String(Constants::DOCKWIDGET_REGISTER)))
            postCommandSequence(CommandListRegisters);
        if (debuggerCore()->isDockVisible(QLatin1String(Constants::DOCKWIDGET_MODULES)))
            postCommandSequence(CommandListModules);
    }
    // After the sequence has been sent off and CDB is pondering the commands,
    // pop up a message box for exceptions.
    if (stopFlags & StopShowExceptionMessageBox)
        showStoppedByExceptionMessageBox(exceptionBoxMessage);
}

void CdbEngine::handleSessionAccessible(unsigned long cdbExState)
{
    const DebuggerState s = state();
    if (!m_hasDebuggee || s == InferiorRunOk) // suppress reports
        return;

    if (debug)
        qDebug("CdbEngine::handleSessionAccessible %dms in state '%s'/'%s', special mode %d",
               elapsedLogTime(), cdbStatusName(cdbExState), stateName(state()), m_specialStopMode);

    switch(s) {
    case EngineShutdownRequested:
        shutdownEngine();
        break;
    case InferiorShutdownRequested:
        shutdownInferior();
        break;
    default:
        break;
    }
}

void CdbEngine::handleSessionInaccessible(unsigned long cdbExState)
{
    const DebuggerState s = state();

    // suppress reports
    if (!m_hasDebuggee || (s == InferiorRunOk && cdbExState != CDB_STATUS_NO_DEBUGGEE))
        return;

    if (debug)
        qDebug("CdbEngine::handleSessionInaccessible %dms in state '%s', '%s', special mode %d",
               elapsedLogTime(), cdbStatusName(cdbExState), stateName(state()), m_specialStopMode);

    switch (state()) {
    case EngineSetupRequested:
        break;
    case EngineRunRequested:
        STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyEngineRunAndInferiorRunOk")
        notifyEngineRunAndInferiorRunOk();
        break;
    case InferiorRunOk:
    case InferiorStopOk:
        // Inaccessible without debuggee (exit breakpoint)
        // We go for spontaneous engine shutdown instead.
        if (cdbExState == CDB_STATUS_NO_DEBUGGEE) {
            if (debug)
                qDebug("Lost debuggeee");
            m_hasDebuggee = false;
        }
        break;
    case InferiorRunRequested:
        STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyInferiorRunOk")
        notifyInferiorRunOk();
        resetLocation();
        break;
    case EngineShutdownRequested:
        break;
    default:
        break;
    }
}

void CdbEngine::handleExtensionMessage(char t, int token, const QByteArray &what, const QByteArray &message)
{
    if (debug > 1) {
        QDebug nospace = qDebug().nospace();
        nospace << "handleExtensionMessage " << t << ' ' << token << ' ' << what
                << ' ' << stateName(state());
        if (t == 'N' || debug > 1) {
            nospace << ' ' << message;
        } else {
            nospace << ' ' << message.size() << " bytes";
        }
    }

    // Is there a reply expected, some command queued?
    if (t == 'R' || t == 'N') {
        if (token == -1) { // Default token, user typed in extension command
            showMessage(QString::fromLatin1(message), LogMisc);
            return;
        }
        const int index = indexOfCommand(m_extensionCommandQueue, token);
        if (index != -1) {
            // Did the command finish? Take off queue and complete, invoke CB
            const CdbExtensionCommandPtr command = m_extensionCommandQueue.takeAt(index);
            if (t == 'R') {
                command->success = true;
                command->reply = message;
            } else {
                command->success = false;
                command->errorMessage = message;
            }
            if (debug)
                qDebug("### Completed extension command '%s', token=%d, pending=%d",
                       command->command.constData(), command->token, m_extensionCommandQueue.size());
            if (command->handler)
                (this->*(command->handler))(command);
            return;
        }
    }

    if (what == "debuggee_output") {
        showMessage(StringFromBase64EncodedUtf16(message), AppOutput);
        return;
    }

    if (what == "event") {
        showStatusMessage(QString::fromAscii(message),  5000);
        return;
    }

    if (what == "session_accessible") {
        if (!m_accessible) {
            m_accessible = true;
            handleSessionAccessible(message.toULong());
        }
        return;
    }

    if (what == "session_inaccessible") {
        if (m_accessible) {
            m_accessible = false;
            handleSessionInaccessible(message.toULong());
        }
        return;
    }

    if (what == "session_idle") {
        handleSessionIdle(message);
        return;
    }

    if (what == "exception") {
        WinException exception;
        GdbMi gdbmi;
        gdbmi.fromString(message);
        exception.fromGdbMI(gdbmi);
        const QString message = exception.toString(true);
        showStatusMessage(message);
#ifdef Q_OS_WIN // Report C++ exception in application output as well.
        if (exception.exceptionCode == winExceptionCppException)
            showMessage(message + QLatin1Char('\n'), AppOutput);
#endif
        return;
    }

    return;
}

// Check for a CDB prompt '0:000> ' ('process:thread> ')..no regexps for QByteArray...
enum { CdbPromptLength = 7 };

static inline bool isCdbPrompt(const QByteArray &c)
{
    return c.size() >= CdbPromptLength && c.at(6) == ' ' && c.at(5) == '>' && c.at(1) == ':'
            && std::isdigit(c.at(0)) &&  std::isdigit(c.at(2)) && std::isdigit(c.at(3))
            && std::isdigit(c.at(4));
}

// Check for '<token>32>' or '<token>32<'
static inline bool checkCommandToken(const QByteArray &tokenPrefix, const QByteArray &c,
                                  int *token, bool *isStart)
{
    *token = 0;
    *isStart = false;
    const int tokenPrefixSize = tokenPrefix.size();
    const int size = c.size();
    if (size < tokenPrefixSize + 2 || !std::isdigit(c.at(tokenPrefixSize)))
        return false;
    switch (c.at(size - 1)) {
    case '>':
        *isStart = false;
        break;
    case '<':
        *isStart = true;
        break;
    default:
        return false;
    }
    if (!c.startsWith(tokenPrefix))
        return false;
    bool ok;
    *token = c.mid(tokenPrefixSize, size - tokenPrefixSize - 1).toInt(&ok);
    return ok;
}

void CdbEngine::parseOutputLine(QByteArray line)
{
    // The hooked output callback in the extension suppresses prompts,
    // it should happen only in initial and exit stages. Note however that
    // if the output is not hooked, sequences of prompts are possible which
    // can mix things up.
    while (isCdbPrompt(line))
        line.remove(0, CdbPromptLength);
    // An extension notification (potentially consisting of several chunks)
    if (line.startsWith(m_creatorExtPrefix)) {
        // "<qtcreatorcdbext>|type_char|token|remainingChunks|serviceName|message"
        const char type = line.at(m_creatorExtPrefix.size());
        // integer token
        const int tokenPos = m_creatorExtPrefix.size() + 2;
        const int tokenEndPos = line.indexOf('|', tokenPos);
        QTC_ASSERT(tokenEndPos != -1, return)
        const int token = line.mid(tokenPos, tokenEndPos - tokenPos).toInt();
        // remainingChunks
        const int remainingChunksPos = tokenEndPos + 1;
        const int remainingChunksEndPos = line.indexOf('|', remainingChunksPos);
        QTC_ASSERT(remainingChunksEndPos != -1, return)
        const int remainingChunks = line.mid(remainingChunksPos, remainingChunksEndPos - remainingChunksPos).toInt();
        // const char 'serviceName'
        const int whatPos = remainingChunksEndPos + 1;
        const int whatEndPos = line.indexOf('|', whatPos);
        QTC_ASSERT(whatEndPos != -1, return)
        const QByteArray what = line.mid(whatPos, whatEndPos - whatPos);
        // Build up buffer, call handler once last chunk was encountered
        m_extensionMessageBuffer += line.mid(whatEndPos + 1);
        if (remainingChunks == 0) {
            handleExtensionMessage(type, token, what, m_extensionMessageBuffer);
            m_extensionMessageBuffer.clear();
        }
        return;
    }
    // Check for command start/end tokens within which the builtin command
    // output is enclosed
    int token = 0;
    bool isStartToken = false;
    const bool isCommandToken = checkCommandToken(m_tokenPrefix, line, &token, &isStartToken);
    if (debug > 1)
        qDebug("Reading CDB stdout '%s',\n  isCommand=%d, token=%d, isStart=%d, current=%d",
               line.constData(), isCommandToken, token, isStartToken, m_currentBuiltinCommandIndex);

    // If there is a current command, wait for end of output indicated by token,
    // command, trigger handler and finish, else append to its output.
    if (m_currentBuiltinCommandIndex != -1) {
        QTC_ASSERT(!isStartToken && m_currentBuiltinCommandIndex < m_builtinCommandQueue.size(), return; );
        const CdbBuiltinCommandPtr &currentCommand = m_builtinCommandQueue.at(m_currentBuiltinCommandIndex);
        if (isCommandToken) {
            // Did the command finish? Invoke callback and remove from queue.
            if (debug)
                qDebug("### Completed builtin command '%s', token=%d, %d lines, pending=%d",
                       currentCommand->command.constData(), currentCommand->token,
                       currentCommand->reply.size(), m_builtinCommandQueue.size() - 1);
            QTC_ASSERT(token == currentCommand->token, return; );
            if (currentCommand->handler)
                (this->*(currentCommand->handler))(currentCommand);
            m_builtinCommandQueue.removeAt(m_currentBuiltinCommandIndex);
            m_currentBuiltinCommandIndex = -1;
        } else {
            // Record output of current command
            currentCommand->reply.push_back(line);
        }
        return;
    } // m_currentCommandIndex
    if (isCommandToken) {
        // Beginning command token encountered, start to record output.
        const int index = indexOfCommand(m_builtinCommandQueue, token);
        QTC_ASSERT(isStartToken && index != -1, return; );
        m_currentBuiltinCommandIndex = index;
        const CdbBuiltinCommandPtr &currentCommand = m_builtinCommandQueue.at(m_currentBuiltinCommandIndex);
        if (debug)
            qDebug("### Gathering output for '%s' token %d", currentCommand->command.constData(), currentCommand->token);
        return;
    }

    showMessage(QString::fromLocal8Bit(line), LogMisc);
}

void CdbEngine::readyReadStandardOut()
{
    m_outputBuffer += m_process.readAllStandardOutput();
    // Split into lines and parse line by line.
    while (true) {
        const int endOfLinePos = m_outputBuffer.indexOf('\n');
        if (endOfLinePos == -1) {
            break;
        } else {
            // Check for '\r\n'
            QByteArray line = m_outputBuffer.left(endOfLinePos);
            if (!line.isEmpty() && line.at(line.size() - 1) == '\r')
                line.truncate(line.size() - 1);
            parseOutputLine(line);
            m_outputBuffer.remove(0, endOfLinePos + 1);
        }
    }
}

void CdbEngine::readyReadStandardError()
{
    showMessage(QString::fromLocal8Bit(m_process.readAllStandardError()), LogError);
}

void CdbEngine::processError()
{
    showMessage(m_process.errorString(), LogError);
}

#if 0
// Join breakpoint ids for a multi-breakpoint id commands like 'bc', 'be', 'bd'
static QByteArray multiBreakpointCommand(const char *cmdC, const Breakpoints &bps)
{
    QByteArray cmd(cmdC);
    ByteArrayInputStream str(cmd);
    foreach(const BreakpointData *bp, bps)
        str << ' ' << bp->bpNumber;
    return cmd;
}
#endif

// Figure out what kind of changes are required to synchronize
enum BreakPointSyncType {
    BreakpointsUnchanged, BreakpointsAdded, BreakpointsRemovedChanged
};

static inline BreakPointSyncType breakPointSyncType(const BreakHandler *handler,
                                                    const BreakpointIds ids)
{
    bool added = false;
    foreach (BreakpointId id, ids) {
        const BreakpointState state = handler->state(id);
        if (debugBreakpoints > 1)
            qDebug("    Checking on breakpoint %llu, state %d\n", id, state);
        switch (state) {
        case BreakpointInsertRequested:
            added = true;
            break;
        case BreakpointChangeRequested:
        case BreakpointRemoveRequested:
            return BreakpointsRemovedChanged;
        default:
            break;
        }
    }
    return added ? BreakpointsAdded : BreakpointsUnchanged;
}

bool CdbEngine::stateAcceptsBreakpointChanges() const
{
    switch (state()) {
    case InferiorRunOk:
    case InferiorStopOk:
    return true;
    default:
        break;
    }
    return false;
}

bool CdbEngine::acceptsBreakpoint(BreakpointId id) const
{
    return DebuggerEngine::isCppBreakpoint(breakHandler()->breakpointData(id));
}

void CdbEngine::attemptBreakpointSynchronization()
{
    if (debug)
        qDebug("attemptBreakpointSynchronization in %s", stateName(state()));
    // Check if there is anything to be done at all.
    BreakHandler *handler = breakHandler();
    // Take ownership of the breakpoint. Requests insertion. TODO: Cpp only?
    foreach (BreakpointId id, handler->unclaimedBreakpointIds())
        if (acceptsBreakpoint(id))
            handler->setEngine(id, this);

    // Find out if there is a need to synchronize again
    const BreakpointIds ids = handler->engineBreakpointIds(this);
    const BreakPointSyncType syncType = breakPointSyncType(handler, ids);
    if (debugBreakpoints)
        qDebug("attemptBreakpointSynchronizationI %dms accessible=%d, %s %d breakpoints, syncType=%d",
               elapsedLogTime(), m_accessible, stateName(state()), ids.size(), syncType);
    if (syncType == BreakpointsUnchanged)
        return;

    if (!m_accessible) {
        // No nested calls.
        if (m_specialStopMode != SpecialStopSynchronizeBreakpoints)
            doInterruptInferior(SpecialStopSynchronizeBreakpoints);
        return;
    }

    // If there are changes/removals, delete all breakpoints and re-insert
    // all enabled breakpoints. This is the simplest
    // way to apply changes since CDB ids shift when removing breakpoints and there is no
    // easy way to re-match them.

    if (syncType == BreakpointsRemovedChanged) { // Need to clear out all?
        postCommand("bc *", 0);
        m_nextBreakpointNumber = 0;
    }

    foreach (BreakpointId id, ids) {
        BreakpointResponse response;
        const BreakpointParameters &p = handler->breakpointData(id);
        response.fromParameters(p);
        switch (handler->state(id)) {
        case BreakpointInsertRequested:
            response.number = m_nextBreakpointNumber++;
            postCommand(cdbAddBreakpointCommand(p, false, response.number), 0);
            handler->notifyBreakpointInsertProceeding(id);
            handler->notifyBreakpointInsertOk(id);
            handler->setResponse(id, response);
            break;
        case BreakpointChangeRequested:
            // Skip disabled breakpoints, else add.
            handler->notifyBreakpointChangeProceeding(id);
            if (p.enabled) {
                response.number = m_nextBreakpointNumber++;
                postCommand(cdbAddBreakpointCommand(p, false, response.number), 0);
                handler->notifyBreakpointChangeOk(id);
                handler->setResponse(id, response);
            }
            break;
        case BreakpointRemoveRequested:
            handler->notifyBreakpointRemoveProceeding(id);
            handler->notifyBreakpointRemoveOk(id);
            break;
        case BreakpointInserted:
            // Existing breakpoints were deleted due to change/removal, re-set
            if (syncType == BreakpointsRemovedChanged) {
                response.number = m_nextBreakpointNumber++;;
                postCommand(cdbAddBreakpointCommand(p, false, response.number), 0);
                handler->setResponse(id, response);
            }
            break;
        default:
            break;
        }
    }
}

QString CdbEngine::normalizeFileName(const QString &f)
{
    QMap<QString, QString>::const_iterator it = m_normalizedFileCache.constFind(f);
    if (it != m_normalizedFileCache.constEnd())
        return it.value();
    const QString winF = QDir::toNativeSeparators(f);
#ifdef Q_OS_WIN
    QString normalized = winNormalizeFileName(winF);
#else
    QString normalized = winF;
#endif
    if (normalized.isEmpty()) { // At least upper case drive letter
        normalized = winF;
        if (normalized.size() > 2 && normalized.at(1) == QLatin1Char(':'))
            normalized[0] = normalized.at(0).toUpper();
    }
    m_normalizedFileCache.insert(f, normalized);
    return normalized;
}

// Parse frame from GDBMI. Duplicate of the gdb code, but that
// has more processing.
static StackFrames parseFrames(const GdbMi &gdbmi)
{
    StackFrames rc;
    const int count = gdbmi.childCount();
    rc.reserve(count);
    for (int i = 0; i  < count; i++) {
        const GdbMi &frameMi = gdbmi.childAt(i);
        StackFrame frame;
        frame.level = i;
        const GdbMi fullName = frameMi.findChild("fullname");
        if (fullName.isValid()) {
            frame.file = QFile::decodeName(fullName.data());
            frame.line = frameMi.findChild("line").data().toInt();
            frame.usable = QFileInfo(frame.file).isFile();
        }
        frame.function = QLatin1String(frameMi.findChild("func").data());
        frame.from = QLatin1String(frameMi.findChild("from").data());
        frame.address = frameMi.findChild("addr").data().toULongLong(0, 16);
        rc.push_back(frame);
    }
    return rc;
}

unsigned CdbEngine::parseStackTrace(const GdbMi &data, bool sourceStepInto)
{
    // Parse frames, find current. Special handling for step into:
    // When stepping into on an actual function (source mode) by executing 't', an assembler
    // frame pointing at the jmp instruction is hit (noticeable by top function being
    // 'ILT+'). If that is the case, execute another 't' to step into the actual function.    .
    // Note that executing 't 2' does not work since it steps 2 instructions on a non-call code line.
    int current = -1;
    StackFrames frames = parseFrames(data);
    const int count = frames.size();
    for (int i = 0; i < count; i++) {
        const bool hasFile = !frames.at(i).file.isEmpty();
        // jmp-frame hit by step into, do another 't' and abort sequence.
        if (!hasFile && i == 0 && sourceStepInto && frames.at(i).function.contains(QLatin1String("ILT+"))) {
            showMessage(QString::fromAscii("Step into: Call instruction hit, performing additional step..."), LogMisc);
            return ParseStackStepInto;
        }
        if (hasFile) {
            frames[i].file = QDir::cleanPath(normalizeFileName(frames.at(i).file));
            if (current == -1 && frames[i].usable)
                current = i;
        }
    }
    if (count && current == -1) // No usable frame, use assembly.
        current = 0;
    // Set
    stackHandler()->setFrames(frames);
    activateFrame(current);
    return 0;
}

void CdbEngine::handleStackTrace(const CdbExtensionCommandPtr &command)
{
    if (command->success) {
        GdbMi data;
        data.fromString(command->reply);
        parseStackTrace(data, false);
        postCommandSequence(command->commandSequence);
    } else {
        showMessage(command->errorMessage, LogError);
    }
}

void CdbEngine::dummyHandler(const CdbBuiltinCommandPtr &command)
{
    postCommandSequence(command->commandSequence);
}

// Post a sequence of standard commands: Trigger next once one completes successfully
void CdbEngine::postCommandSequence(unsigned mask)
{
    if (debug)
        qDebug("postCommandSequence 0x%x\n", mask);

    if (!mask)
        return;
    if (mask & CommandListThreads) {
        postExtensionCommand("threads", QByteArray(), 0, &CdbEngine::handleThreads, mask & ~CommandListThreads);
        return;
    }
    if (mask & CommandListStack) {
        postExtensionCommand("stack", QByteArray(), 0, &CdbEngine::handleStackTrace, mask & ~CommandListStack);
        return;
    }
    if (mask & CommandListRegisters) {
        QTC_ASSERT(threadsHandler()->currentThread() >= 0,  return; )
        postExtensionCommand("registers", QByteArray(), 0, &CdbEngine::handleRegisters, mask & ~CommandListRegisters);
        return;
    }
    if (mask & CommandListModules) {
        postExtensionCommand("modules", QByteArray(), 0, &CdbEngine::handleModules, mask & ~CommandListModules);
        return;
    }
}

void CdbEngine::handleWidgetAt(const CdbExtensionCommandPtr &reply)
{
    bool success = false;
    QString message;
    do {
        if (!reply->success) {
            message = QString::fromAscii(reply->errorMessage);
            break;
        }
        // Should be "namespace::QWidget:0x555"
        QString watchExp = QString::fromAscii(reply->reply);
        const int sepPos = watchExp.lastIndexOf(QLatin1Char(':'));
        if (sepPos == -1) {
            message = QString::fromAscii("Invalid output: %1").arg(watchExp);
            break;
        }
        // 0x000 -> nothing found
        if (!watchExp.mid(sepPos + 1).toULongLong(0, 0)) {
            message = QString::fromAscii("No widget could be found at %1, %2.").arg(m_watchPointX).arg(m_watchPointY);
            break;
        }
        // Turn into watch expression: "*(namespace::QWidget*)0x555"
        watchExp.replace(sepPos, 1, QLatin1String("*)"));
        watchExp.insert(0, QLatin1String("*("));
        watchHandler()->watchExpression(watchExp);
        success = true;
    } while (false);
    if (!success)
        showMessage(message, LogWarning);
    m_watchPointX = m_watchPointY = 0;
}

void CdbEngine::watchPoint(const QPoint &p)
{
    m_watchPointX = p.x();
    m_watchPointY = p.y();
    switch (state()) {
    case InferiorStopOk:
        postWidgetAtCommand();
        break;
    case InferiorRunOk:
        // "Select Widget to Watch" from a running application is currently not
        // supported. It could be implemented via SpecialStopGetWidgetAt-mode,
        // but requires some work as not to confuse the engine by state-change notifications
        // emitted by the debuggee function call.
        showMessage(tr("\"Select Widget to Watch\": Please stop the application first."), LogWarning);
        break;
    default:
        showMessage(tr("\"Select Widget to Watch\": Not supported in state '%1'.").
                    arg(QString::fromAscii(stateName(state()))), LogWarning);
        break;
    }
}

void CdbEngine::postWidgetAtCommand()
{
    QByteArray arguments = QByteArray::number(m_watchPointX);
    arguments.append(' ');
    arguments.append(QByteArray::number(m_watchPointY));
    postExtensionCommand("widgetat", arguments, 0, &CdbEngine::handleWidgetAt, 0);
}

} // namespace Internal
} // namespace Debugger
