/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cdbengine.h"

#include "breakhandler.h"
#include "breakpoint.h"
#include "bytearrayinputstream.h"
#include "cdboptions.h"
#include "cdboptionspage.h"
#include "cdbparsehelpers.h"
#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerinternalconstants.h"
#include "debuggerprotocol.h"
#include "debuggerrunner.h"
#include "debuggerstartparameters.h"
#include "debuggertooltipmanager.h"
#include "disassembleragent.h"
#include "disassemblerlines.h"
#include "memoryagent.h"
#include "moduleshandler.h"
#include "registerhandler.h"
#include "stackframe.h"
#include "stackhandler.h"
#include "threadshandler.h"
#include "watchhandler.h"
#include "watchutils.h"
#include "shared/cdbsymbolpathlisteditor.h"
#include "shared/hostutils.h"
#include "procinterrupt.h"
#include "sourceutils.h"

#include <TranslationUnit.h>

#include <coreplugin/icore.h>
#include <texteditor/itexteditor.h>
#include <projectexplorer/abi.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/task.h>
#include <projectexplorer/taskhub.h>

#include <utils/synchronousprocess.h>
#include <utils/qtcprocess.h>
#include <utils/winutils.h>
#include <utils/qtcassert.h>
#include <utils/savedaction.h>
#include <utils/consoleprocess.h>
#include <utils/fileutils.h>

#include <cplusplus/findcdbbreakpoint.h>
#include <cplusplus/CppDocument.h>
#include <cpptools/ModelManagerInterface.h>

#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QTextStream>
#include <QDateTime>
#include <QToolTip>
#include <QMainWindow>
#include <QMessageBox>

#include <cctype>

Q_DECLARE_METATYPE(Debugger::Internal::DisassemblerAgent*)
Q_DECLARE_METATYPE(Debugger::Internal::MemoryAgent*)

enum { debug = 0 };
enum { debugLocals = 0 };
enum { debugSourceMapping = 0 };
enum { debugWatches = 0 };
enum { debugBreakpoints = 0 };

enum HandleLocalsFlags
{
    PartialLocalsUpdate = 0x1,
    LocalsUpdateForNewFrame = 0x2
};

#if 0
#  define STATE_DEBUG(state, func, line, notifyFunc) qDebug("%s in %s at %s:%d", notifyFunc, stateName(state), func, line);
#else
#  define STATE_DEBUG(state, func, line, notifyFunc)
#endif

/*!
    \class Debugger::Internal::CdbEngine

    Cdb engine version 2: Run the CDB process on pipes and parse its output.
    The engine relies on a CDB extension Qt Creator provides as an extension
    library (32/64bit), which is loaded into cdb.exe. It serves to:

    \list
    \o Notify the engine about the state of the debugging session:
        \list
        \o idle: (hooked up with .idle_cmd) debuggee stopped
        \o accessible: Debuggee stopped, cdb.exe accepts commands
        \o inaccessible: Debuggee runs, no way to post commands
        \o session active/inactive: Lost debuggee, terminating.
        \endlist
    \o Hook up with output/event callbacks and produce formatted output to be able
       to catch application output and exceptions.
    \o Provide some extension commands that produce output in a standardized (GDBMI)
      format that ends up in handleExtensionMessage(), for example:
      \list
      \o pid     Return debuggee pid for interrupting.
      \o locals  Print locals from SymbolGroup
      \o expandLocals Expand locals in symbol group
      \o registers, modules, threads
      \endlist
   \endlist

   Debugger commands can be posted by calling:

   \list

    \o postCommand(): Does not expect a reply
    \o postBuiltinCommand(): Run a builtin-command producing free-format, multiline output
       that is captured by enclosing it in special tokens using the 'echo' command and
       then invokes a callback with a CdbBuiltinCommand structure.
    \o postExtensionCommand(): Run a command provided by the extension producing
       one-line output and invoke a callback with a CdbExtensionCommand structure
       (output is potentially split up in chunks).
    \endlist


    Startup sequence:
    [Console: The console stub launches the process. On process startup,
              launchCDB() is called with AttachExternal].
    setupEngine() calls launchCDB() with the startparameters. The debuggee
    runs into the initial breakpoint (session idle). EngineSetupOk is
    notified (inferior still stopped). setupInferior() is then called
    which does breakpoint synchronization and issues the extension 'pid'
    command to obtain the inferior pid (which also hooks up the output callbacks).
     handlePid() notifies notifyInferiorSetupOk.
    runEngine() is then called which issues 'g' to continue the inferior.
    Shutdown mostly uses notifyEngineSpontaneousShutdown() as cdb just quits
    when the inferior exits (except attach modes).
*/

using namespace ProjectExplorer;

namespace Debugger {
namespace Internal {

static const char localsPrefixC[] = "local.";

struct MemoryViewCookie
{
    explicit MemoryViewCookie(MemoryAgent *a = 0, QObject *e = 0,
                              quint64 addr = 0, quint64 l = 0) :
        agent(a), editorToken(e), address(addr), length(l)
    {}

    MemoryAgent *agent;
    QObject *editorToken;
    quint64 address;
    quint64 length;
};

struct MemoryChangeCookie
{
    explicit MemoryChangeCookie(quint64 addr = 0, const QByteArray &d = QByteArray()) :
                               address(addr), data(d) {}

    quint64 address;
    QByteArray data;
};

struct ConditionalBreakPointCookie
{
    ConditionalBreakPointCookie(BreakpointModelId i = BreakpointModelId()) : id(i) {}
    BreakpointModelId id;
    GdbMi stopReason;
};

} // namespace Internal
} // namespace Debugger

Q_DECLARE_METATYPE(Debugger::Internal::MemoryViewCookie)
Q_DECLARE_METATYPE(Debugger::Internal::MemoryChangeCookie)
Q_DECLARE_METATYPE(Debugger::Internal::ConditionalBreakPointCookie)

namespace Debugger {
namespace Internal {

static inline bool isCreatorConsole(const DebuggerStartParameters &sp, const CdbOptions &o)
{
    return !o.cdbConsole && sp.useTerminal
           && (sp.startMode == StartInternal || sp.startMode == StartExternal);
}

static QMessageBox *
nonModalMessageBox(QMessageBox::Icon icon, const QString &title, const QString &text)
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
        CdbCommandBase(cmd, token, flags, nc, cookie), handler(h)
    {}


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
    case StartRemoteGdb:
        return false;
    default:
        break;
    }
    return true;
}

// Accessed by RunControlFactory
DebuggerEngine *createCdbEngine(const DebuggerStartParameters &sp, QString *errorMessage)
{
#ifdef Q_OS_WIN
    CdbOptionsPage *op = CdbOptionsPage::instance();
    if (!op || !op->options()->isValid() || !validMode(sp.startMode)) {
        *errorMessage = QLatin1String("Internal error: Invalid start parameters passed for thre CDB engine.");
        return 0;
    }
    return new CdbEngine(sp, op->options());
#else
    Q_UNUSED(sp)
#endif
    *errorMessage = QString::fromLatin1("Unsupported debug mode");
    return 0;
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

CdbEngine::CdbEngine(const DebuggerStartParameters &sp, const OptionsPtr &options) :
    DebuggerEngine(sp),
    m_creatorExtPrefix("<qtcreatorcdbext>|"),
    m_tokenPrefix("<token>"),
    m_options(options),
    m_effectiveStartMode(NoStartMode),
    m_accessible(false),
    m_specialStopMode(NoSpecialStop),
    m_nextCommandToken(0),
    m_currentBuiltinCommandIndex(-1),
    m_extensionCommandPrefixBA("!" QT_CREATOR_CDB_EXT "."),
    m_operateByInstructionPending(true),
    m_operateByInstruction(true), // Default CDB setting
    m_notifyEngineShutdownOnTermination(false),
    m_hasDebuggee(false),
    m_cdbIs64Bit(false),
    m_elapsedLogTime(0),
    m_sourceStepInto(false),
    m_watchPointX(0),
    m_watchPointY(0),
    m_ignoreCdbOutput(false)
{
    connect(theAssemblerAction(), SIGNAL(triggered(bool)), this, SLOT(operateByInstructionTriggered(bool)));

    setObjectName(QLatin1String("CdbEngine"));
    connect(&m_process, SIGNAL(finished(int)), this, SLOT(processFinished()));
    connect(&m_process, SIGNAL(error(QProcess::ProcessError)), this, SLOT(processError()));
    connect(&m_process, SIGNAL(readyReadStandardOutput()), this, SLOT(readyReadStandardOut()));
    connect(&m_process, SIGNAL(readyReadStandardError()), this, SLOT(readyReadStandardOut()));
}

void CdbEngine::init()
{
    m_effectiveStartMode = NoStartMode;
    notifyInferiorPid(0);
    m_accessible = false;
    m_specialStopMode = NoSpecialStop;
    m_nextCommandToken  = 0;
    m_currentBuiltinCommandIndex = -1;
    m_operateByInstructionPending = theAssemblerAction()->isChecked();
    m_operateByInstruction = true; // Default CDB setting
    m_notifyEngineShutdownOnTermination = false;
    m_hasDebuggee = false;
    m_sourceStepInto = false;
    m_watchPointX = m_watchPointY = 0;
    m_ignoreCdbOutput = false;
    m_watchInameToName.clear();

    m_outputBuffer.clear();
    m_builtinCommandQueue.clear();
    m_extensionCommandQueue.clear();
    m_extensionMessageBuffer.clear();
    m_pendingBreakpointMap.clear();
    m_customSpecialStopData.clear();
    m_symbolAddressCache.clear();
    m_coreStopReason.reset();

    // Create local list of mappings in native separators
    m_sourcePathMappings.clear();
    const QSharedPointer<GlobalDebuggerOptions> globalOptions = debuggerCore()->globalDebuggerOptions();
    if (!globalOptions->sourcePathMap.isEmpty()) {
        typedef GlobalDebuggerOptions::SourcePathMap::const_iterator SourcePathMapIterator;
        m_sourcePathMappings.reserve(globalOptions->sourcePathMap.size());
        const SourcePathMapIterator cend = globalOptions->sourcePathMap.constEnd();
        for (SourcePathMapIterator it = globalOptions->sourcePathMap.constBegin(); it != cend; ++it) {
            m_sourcePathMappings.push_back(SourcePathMapping(QDir::toNativeSeparators(it.key()),
                                                             QDir::toNativeSeparators(it.value())));
        }
    }
    QTC_ASSERT(m_process.state() != QProcess::Running, Utils::SynchronousProcess::stopProcess(m_process));
}

CdbEngine::~CdbEngine()
{
}

void CdbEngine::operateByInstructionTriggered(bool operateByInstruction)
{
    // To be set next time session becomes accessible
    m_operateByInstructionPending = operateByInstruction;
    if (state() == InferiorStopOk)
        syncOperateByInstruction(operateByInstruction);
}

void CdbEngine::syncOperateByInstruction(bool operateByInstruction)
{
    if (debug)
        qDebug("syncOperateByInstruction current: %d new %d", m_operateByInstruction, operateByInstruction);
    if (m_operateByInstruction == operateByInstruction)
        return;
    QTC_ASSERT(m_accessible, return);
    m_operateByInstruction = operateByInstruction;
    postCommand(m_operateByInstruction ? QByteArray("l-t") : QByteArray("l+t"), 0);
    postCommand(m_operateByInstruction ? QByteArray("l-s") : QByteArray("l+s"), 0);
}

bool CdbEngine::setToolTipExpression(const QPoint &mousePos,
                                     TextEditor::ITextEditor *editor,
                                     const DebuggerToolTipContext &contextIn)
{
    if (debug)
        qDebug() << Q_FUNC_INFO;
    // Need a stopped debuggee and a cpp file in a valid frame
    if (state() != InferiorStopOk || !isCppEditor(editor) || stackHandler()->currentIndex() < 0)
        return false;
    // Determine expression and function
    int line;
    int column;
    DebuggerToolTipContext context = contextIn;
    QString exp = fixCppExpression(cppExpressionAt(editor, context.position, &line, &column, &context.function));
    // Are we in the current stack frame
    if (context.function.isEmpty() || exp.isEmpty() || context.function != stackHandler()->currentFrame().function)
        return false;
    // Show tooltips of local variables only. Anything else can slow debugging down.
    const WatchData *localVariable = watchHandler()->findCppLocalVariable(exp);
    if (!localVariable)
        return false;
    DebuggerToolTipWidget *tw = new DebuggerToolTipWidget;
    tw->setContext(context);
    tw->setIname(localVariable->iname);
    tw->acquireEngine(this);
    DebuggerToolTipManager::instance()->showToolTip(mousePos, editor, tw);
    return true;
}

// Determine full path to the CDB extension library.
QString CdbEngine::extensionLibraryName(bool is64Bit)
{
    // Determine extension lib name and path to use
    QString rc;
    QTextStream(&rc) << QFileInfo(QCoreApplication::applicationDirPath()).path()
                     << "/lib/" << (is64Bit ? QT_CREATOR_CDB_EXT "64" : QT_CREATOR_CDB_EXT "32")
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
    connect(m_consoleStub.data(), SIGNAL(processError(QString)),
            SLOT(consoleStubError(QString)));
    connect(m_consoleStub.data(), SIGNAL(processStarted()),
            SLOT(consoleStubProcessStarted()));
    connect(m_consoleStub.data(), SIGNAL(stubStopped()),
            SLOT(consoleStubExited()));
    m_consoleStub->setWorkingDirectory(sp.workingDirectory);
    if (sp.environment.size())
        m_consoleStub->setEnvironment(sp.environment);
    if (!m_consoleStub->start(sp.executable, sp.processArgs)) {
        *errorMessage = tr("The console process '%1' could not be started.").arg(sp.executable);
        return false;
    }
    return true;
}

void CdbEngine::consoleStubError(const QString &msg)
{
    if (debug)
        qDebug("consoleStubProcessMessage() in %s %s", stateName(state()), qPrintable(msg));
    if (state() == EngineSetupRequested) {
        STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyEngineSetupFailed")
        notifyEngineSetupFailed();
    } else {
        STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyEngineIll")
        notifyEngineIll();
    }
    nonModalMessageBox(QMessageBox::Critical, tr("Debugger Error"), msg);
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
    attachParameters.useTerminal = false;
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
        m_options->toSettings(Core::ICore::settings());

    init();
    if (!m_logTime.elapsed())
        m_logTime.start();
    QString errorMessage;
    // Console: Launch the stub with the suspended application and attach to it
    // CDB in theory has a command line option '-2' that launches a
    // console, too, but that immediately closes when the debuggee quits.
    // Use the Creator stub instead.
    const DebuggerStartParameters &sp = startParameters();
    const bool launchConsole = isCreatorConsole(sp, *m_options);
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
    const QString normalFormat = tr("Normal");
    const QStringList stringFormats = QStringList()
        << normalFormat << tr("Separate Window");
    WatchHandler *wh = watchHandler();
    wh->addTypeFormats("QString", stringFormats);
    wh->addTypeFormats("QString *", stringFormats);
    wh->addTypeFormats("QByteArray", stringFormats);
    wh->addTypeFormats("QByteArray *", stringFormats);
    wh->addTypeFormats("std__basic_string", stringFormats);  // Python dumper naming convention for std::[w]string
    const QStringList imageFormats = QStringList()
        << normalFormat << tr("Image");
    wh->addTypeFormats("QImage", imageFormats);
    wh->addTypeFormats("QImage *", imageFormats);
}

bool CdbEngine::launchCDB(const DebuggerStartParameters &sp, QString *errorMessage)
{
    if (debug)
        qDebug("launchCDB startMode=%d", sp.startMode);
    const QChar blank(QLatin1Char(' '));
    // Start engine which will run until initial breakpoint:
    // Determine binary (force MSVC), extension lib name and path to use
    // The extension is passed as relative name with the path variable set
    //(does not work with absolute path names)
    const QString executable = sp.debuggerCommand;
    if (executable.isEmpty()) {
        *errorMessage = tr("There is no CDB executable specified.");
        return false;
    }

    m_cdbIs64Bit =
#ifdef Q_OS_WIN
            Utils::winIs64BitBinary(executable);
#else
            false;
#endif
    const QFileInfo extensionFi(CdbEngine::extensionLibraryName(m_cdbIs64Bit));
    if (!extensionFi.isFile()) {
        *errorMessage = QString::fromLatin1("Internal error: The extension %1 cannot be found.").
                arg(QDir::toNativeSeparators(extensionFi.absoluteFilePath()));
        return false;
    }
    const QString extensionFileName = extensionFi.fileName();
    // Prepare arguments
    QStringList arguments;
    const bool isRemote = sp.startMode == AttachToRemoteServer;
    if (isRemote) { // Must be first
        arguments << QLatin1String("-remote") << sp.remoteChannel;
    } else {
        arguments << (QLatin1String("-a") + extensionFileName);
    }
    // Source line info/No terminal breakpoint / Pull extension
    arguments << QLatin1String("-lines") << QLatin1String("-G")
    // register idle (debuggee stop) notification
              << QLatin1String("-c")
              << QLatin1String(".idle_cmd ") + QString::fromLatin1(m_extensionCommandPrefixBA) + QLatin1String("idle");
    if (sp.useTerminal) // Separate console
        arguments << QLatin1String("-2");
    if (m_options->ignoreFirstChanceAccessViolation)
        arguments << QLatin1String("-x");
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
        Utils::QtcProcess::addArgs(&nativeArguments,
                                   QStringList(QDir::toNativeSeparators(sp.executable)));
        break;
    case AttachToRemoteServer:
        break;
    case AttachExternal:
    case AttachCrashedExternal:
        arguments << QLatin1String("-p") << QString::number(sp.attachPID);
        if (sp.startMode == AttachCrashedExternal) {
            arguments << QLatin1String("-e") << sp.crashParameter << QLatin1String("-g");
        } else {
            if (isCreatorConsole(startParameters(), *m_options))
                arguments << QLatin1String("-pr") << QLatin1String("-pb");
        }
        break;
    case AttachCore:
        arguments << QLatin1String("-z") << sp.coreFile;
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

    const QString msg = QString::fromLatin1("Launching %1 %2\nusing %3 of %4.").
            arg(QDir::toNativeSeparators(executable),
                arguments.join(QString(blank)) + blank + nativeArguments,
                QDir::toNativeSeparators(extensionFi.absoluteFilePath()),
                extensionFi.lastModified().toString(Qt::SystemLocaleShortDate));
    showMessage(msg, LogMisc);

    m_outputBuffer.clear();
    const QStringList environment = sp.environment.size() == 0 ?
                                    QProcessEnvironment::systemEnvironment().toStringList() :
                                    sp.environment.toStringList();
    m_process.setEnvironment(mergeEnvironment(environment, extensionFi.absolutePath()));
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
    // QmlCppEngine expects the QML engine to be connected before any breakpoints are hit
    // (attemptBreakpointSynchronization() will be directly called then)
    attemptBreakpointSynchronization();
    if (startParameters().breakOnMain) {
        const BreakpointParameters bp(BreakpointAtMain);
        postCommand(cdbAddBreakpointCommand(bp, m_sourcePathMappings,
                                            BreakpointModelId(quint16(-1)), true), 0);
    }
    postCommand("sxn 0x4000001f", 0); // Do not break on WowX86 exceptions.
    postCommand(".asm source_line", 0); // Source line in assembly
    postCommand(m_extensionCommandPrefixBA + "setparameter maxStringLength="
                + debuggerCore()->action(MaximalStringLength)->value().toByteArray()
                + " maxStackDepth="
                + debuggerCore()->action(MaximalStackDepth)->value().toByteArray()
                , 0);
    postExtensionCommand("pid", QByteArray(), 0, &CdbEngine::handlePid);
}

static QByteArray msvcRunTime(const Abi::OSFlavor flavour)
{
    switch (flavour)  {
    case Abi::WindowsMsvc2005Flavor:
        return "MSVCR80";
    case Abi::WindowsMsvc2008Flavor:
        return "MSVCR90";
    case Abi::WindowsMsvc2010Flavor:
        return "MSVCR100";
    case Abi::WindowsMsvc2012Flavor:
        return "MSVCR110"; // #FIXME: VS2012 beta, will probably be 12 in final?
    default:
        break;
    }
    return "MSVCRT"; // MinGW, others.
}

static QByteArray breakAtFunctionCommand(const QByteArray &function,
                                         const QByteArray &module = QByteArray())
{
     QByteArray result = "bu ";
     if (!module.isEmpty()) {
         result += module;
         result += '!';
     }
     result += function;
     return result;
}

void CdbEngine::runEngine()
{
    if (debug)
        qDebug("runEngine");
    foreach (const QString &breakEvent, m_options->breakEvents)
            postCommand(QByteArray("sxe ") + breakEvent.toLatin1(), 0);
    // Break functions: each function must be fully qualified,
    // else the debugger will slow down considerably.
    foreach (const QString &breakFunctionS, m_options->breakFunctions) {
        const QByteArray breakFunction = breakFunctionS.toLatin1();
        if (breakFunction == CdbOptions::crtDbgReport) {
            // CrtDbgReport(): Add MSVC runtime (debug, release)
            // and stop at Wide character version as well
            const QByteArray module = msvcRunTime(startParameters().toolChainAbi.osFlavor());
            const QByteArray debugModule = module + 'D';
            const QByteArray wideFunc = breakFunction + 'W';
            postCommand(breakAtFunctionCommand(breakFunction, module), 0);
            postCommand(breakAtFunctionCommand(wideFunc, module), 0);
            postCommand(breakAtFunctionCommand(breakFunction, debugModule), 0);
            postCommand(breakAtFunctionCommand(wideFunc, debugModule), 0);
        } else {
            postCommand(breakAtFunctionCommand(breakFunction), 0);
        }
    }
    if (debuggerCore()->boolSetting(BreakOnWarning)) {
        postCommand("bm /( QtCored4!qWarning", 0); // 'bm': All overloads.
        postCommand("bm /( Qt5Cored!QMessageLogger::warning", 0);
    }
    if (debuggerCore()->boolSetting(BreakOnFatal)) {
        postCommand("bm /( QtCored4!qFatal", 0); // 'bm': All overloads.
        postCommand("bm /( Qt5Cored!QMessageLogger::fatal", 0);
    }
    if (startParameters().startMode == AttachCore) {
        QTC_ASSERT(!m_coreStopReason.isNull(), return; );
        notifyInferiorUnrunnable();
        processStop(*m_coreStopReason, false);
    } else {
        postCommand("g", 0);
    }
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

    if (m_accessible) { // except console.
        if (startParameters().startMode == AttachExternal || startParameters().startMode == AttachCrashedExternal)
            detachDebugger();
        STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyInferiorShutdownOk")
        notifyInferiorShutdownOk();
    } else {
        // A command got stuck.
        if (commandsPending()) {
            showMessage(QLatin1String("Cannot shut down inferior due to pending commands."), LogWarning);
            STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyInferiorShutdownFailed")
            notifyInferiorShutdownFailed();
            return;
        }
        if (!canInterruptInferior()) {
            showMessage(QLatin1String("Cannot interrupt the inferior."), LogWarning);
            STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyInferiorShutdownFailed")
            notifyInferiorShutdownFailed();
            return;
        }
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
    m_ignoreCdbOutput = true;
    // Go for kill if there are commands pending.
    if (m_accessible && !commandsPending()) {
        // detach (except console): Wait for debugger to finish.
        if (startParameters().startMode == AttachExternal || startParameters().startMode == AttachCrashedExternal)
            detachDebugger();
        // Remote requires a bit more force to quit.
        if (m_effectiveStartMode == AttachToRemoteServer) {
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
    if (crashed)
        showMessage(tr("CDB crashed"), LogError); // not in your life.
    else
        showMessage(tr("CDB exited (%1)").arg(m_process.exitCode()), LogMisc);

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
        // The QML/CPP engine relies on the standard sequence of InferiorShutDown,etc.
        // Otherwise, we take a shortcut.
        if (isSlaveEngine()) {
            STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyInferiorExited")
            notifyInferiorExited();
        } else {
            STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyEngineSpontaneousShutdown")
            notifyEngineSpontaneousShutdown();
        }
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
        // Store the name since the CDB extension library
        // does not maintain the names of watches.
        if (!dataIn.name.isEmpty() && dataIn.name != QLatin1String(dataIn.exp))
            m_watchInameToName.insert(dataIn.iname, dataIn.name);
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
        watchHandler()->insertIncompleteData(item);
        showMessage(QString::fromLatin1("Unable to add watch item '%1'/'%2': %3").
                    arg(QString::fromLatin1(item.iname), QString::fromLatin1(item.exp),
                        QString::fromLocal8Bit(reply->errorMessage)), LogError);
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
    postExtensionCommand(isWatch ? "watches" : "locals",
                         localsArguments, 0,
                         &CdbEngine::handleLocals,
                         0, QVariant(int(PartialLocalsUpdate)));
}

bool CdbEngine::hasCapability(unsigned cap) const
{
    return cap & (DisassemblerCapability | RegisterCapability
           | ShowMemoryCapability
           |WatchpointByAddressCapability|JumpToLineCapability|AddWatcherCapability|WatchWidgetsCapability
           |ReloadModuleCapability
           |BreakOnThrowAndCatchCapability // Sort-of: Can break on throw().
           |BreakConditionCapability|TracePointCapability
           |BreakModuleCapability
           |OperateByInstructionCapability
           |RunToLineCapability
           |MemoryAddressCapability);
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
    return m_effectiveStartMode != AttachToRemoteServer && inferiorPid();
}

void CdbEngine::interruptInferior()
{
    if (debug)
        qDebug() << "CdbEngine::interruptInferior()" << stateName(state());

    bool ok = false;
    if (!canInterruptInferior())
        showMessage(tr("Interrupting is not possible in remote sessions."), LogError);
    else
        ok = doInterruptInferior(NoSpecialStop);
    // Restore running state if stop failed.
    if (!ok) {
        STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyInferiorStopOk")
        notifyInferiorStopOk();
        STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyInferiorRunRequested")
        notifyInferiorRunRequested();
        STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyInferiorRunOk")
        notifyInferiorRunOk();
    }
}

void CdbEngine::doInterruptInferiorCustomSpecialStop(const QVariant &v)
{
    if (m_specialStopMode == NoSpecialStop)
        doInterruptInferior(CustomSpecialStop);
    m_customSpecialStopData.push_back(v);
}

bool CdbEngine::doInterruptInferior(SpecialStopMode sm)
{
    const SpecialStopMode oldSpecialMode = m_specialStopMode;
    m_specialStopMode = sm;

    showMessage(QString::fromLatin1("Interrupting process %1...").arg(inferiorPid()), LogMisc);
    QString errorMessage;

    const bool ok = interruptProcess(inferiorPid(), CdbEngineType,
                                     &errorMessage, m_cdbIs64Bit);
    if (!ok) {
        m_specialStopMode = oldSpecialMode;
        showMessage(errorMessage, LogError);
    }
    return ok;
}

void CdbEngine::executeRunToLine(const ContextData &data)
{
    // Add one-shot breakpoint
    BreakpointParameters bp;
    if (data.address) {
        bp.type =BreakpointByAddress;
        bp.address = data.address;
    } else {
        bp.type =BreakpointByFileAndLine;
        bp.fileName = data.fileName;
        bp.lineNumber = data.lineNumber;
    }
    postCommand(cdbAddBreakpointCommand(bp, m_sourcePathMappings, BreakpointModelId(), true), 0);
    continueInferior();
}

void CdbEngine::executeRunToFunction(const QString &functionName)
{
    // Add one-shot breakpoint
    BreakpointParameters bp(BreakpointByFunction);
    bp.functionName = functionName;

    postCommand(cdbAddBreakpointCommand(bp, m_sourcePathMappings, BreakpointModelId(), true), 0);
    continueInferior();
}

void CdbEngine::setRegisterValue(int regnr, const QString &value)
{
    const Registers registers = registerHandler()->registers();
    QTC_ASSERT(regnr < registers.size(), return);
    // Value is decimal or 0x-hex-prefixed
    QByteArray cmd;
    ByteArrayInputStream str(cmd);
    str << "r " << registers.at(regnr).name << '=' << value;
    postCommand(cmd, 0);
    reloadRegisters();
}

void CdbEngine::executeJumpToLine(const ContextData &data)
{
    if (data.address) {
        // Goto address directly.
        jumpToAddress(data.address);
        gotoLocation(Location(data.address));
    } else {
        // Jump to source line: Resolve source line address and go to that location
        QByteArray cmd;
        ByteArrayInputStream str(cmd);
        str << "? `" << QDir::toNativeSeparators(data.fileName) << ':' << data.lineNumber << '`';
        const QVariant cookie = qVariantFromValue(data);
        postBuiltinCommand(cmd, 0, &CdbEngine::handleJumpToLineAddressResolution, 0, cookie);
    }
}

void CdbEngine::jumpToAddress(quint64 address)
{
    // Fake a jump to address by setting the PC register.
    QByteArray registerCmd;
    ByteArrayInputStream str(registerCmd);
    // PC-register depending on 64/32bit.
    str << "r " << (startParameters().toolChainAbi.wordWidth() == 64 ? "rip" : "eip") << '=';
    str.setHexPrefix(true);
    str.setIntegerBase(16);
    str << address;
    postCommand(registerCmd, 0);
}

void CdbEngine::handleJumpToLineAddressResolution(const CdbBuiltinCommandPtr &cmd)
{
    if (cmd->reply.isEmpty())
        return;
    // Evaluate expression: 5365511549 = 00000001`3fcf357d
    // Set register 'rip' to hex address and goto lcoation
    QByteArray answer = cmd->reply.front().trimmed();
    const int equalPos = answer.indexOf(" = ");
    if (equalPos == -1)
        return;
    answer.remove(0, equalPos + 3);
    const int apPos = answer.indexOf('`');
    if (apPos != -1)
        answer.remove(apPos, 1);
    bool ok;
    const quint64 address = answer.toLongLong(&ok, 16);
    if (ok && address) {
        QTC_ASSERT(cmd->cookie.canConvert<ContextData>(), return);
        const ContextData cookie = qvariant_cast<ContextData>(cmd->cookie);
        jumpToAddress(address);
        gotoLocation(Location(cookie.fileName, cookie.lineNumber));
    }
}

static inline bool isAsciiWord(const QString &s)
{
    foreach (const QChar &c, s) {
        if (!c.isLetterOrNumber() || c.toLatin1() == 0)
            return false;
    }
    return true;
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
    switch (value.type()) {
    case QVariant::String: {
        // Convert qstring to Utf16 data not considering endianness for Windows.
        const QString s = value.toString();
        if (isAsciiWord(s)) {
            str << m_extensionCommandPrefixBA << "assign \"" << w->iname << '='
                << s.toLatin1() << '"';
        } else {
            const QByteArray utf16(reinterpret_cast<const char *>(s.utf16()), 2 * s.size());
            str << m_extensionCommandPrefixBA << "assign -u " << w->iname << '='
                << utf16.toHex();
        }
    }
        break;
    default:
        str << m_extensionCommandPrefixBA << "assign " << w->iname << '='
            << value.toString();
        break;
    }

    postCommand(cmd, 0);
    // Update all locals in case we change a union or something pointed to
    // that affects other variables, too.
    updateLocals();
}

void CdbEngine::handleThreads(const CdbExtensionCommandPtr &reply)
{
    if (debug)
        qDebug("CdbEngine::handleThreads success=%d", reply->success);
    if (reply->success) {
        GdbMi data;
        data.fromString(reply->reply);
        threadsHandler()->updateThreads(data);
        // Continue sequence
        postCommandSequence(reply->commandSequence);
    } else {
        showMessage(QString::fromLatin1(reply->errorMessage), LogError);
    }
}

void CdbEngine::executeDebuggerCommand(const QString &command, DebuggerLanguages languages)
{
    if (languages & CppLanguage)
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
                .arg(QString::fromLocal8Bit(cmd), QString::fromLatin1(stateName(state())));
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
                .arg(QString::fromLocal8Bit(cmd), QString::fromLatin1(stateName(state())));
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
    if (index >= frames.size()) {
        reloadFullStack(); // Clicked on "More...".
        return;
    }

    const StackFrame frame = frames.at(index);
    if (debug || debugLocals)
        qDebug("activateFrame idx=%d '%s' %d", index,
               qPrintable(frame.file), frame.line);
    stackHandler()->setCurrentIndex(index);
    const bool showAssembler = !frames.at(index).isUsable();
    if (showAssembler) { // Assembly code: Clean out model and force instruction mode.
        watchHandler()->removeAllData();
        QAction *assemblerAction = theAssemblerAction();
        if (assemblerAction->isChecked())
            gotoLocation(frame);
        else
            assemblerAction->trigger(); // Seems to trigger update
    } else {
        gotoLocation(frame);
        updateLocals(true);
    }
}

void CdbEngine::updateLocals(bool forNewStackFrame)
{
    typedef QHash<QByteArray, int> WatcherHash;

    const int frameIndex = stackHandler()->currentIndex();
    if (frameIndex < 0) {
        watchHandler()->removeAllData();
        return;
    }
    const StackFrame frame = stackHandler()->currentFrame();
    if (!frame.isUsable()) {
        watchHandler()->removeAllData();
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
        foreach (const QByteArray &e, expanded) {
            if (i++)
                str << ',';
            str << e;
        }
    }
    addLocalsOptions(str);
    // Uninitialized variables if desired. Quote as safeguard against shadowed
    // variables in case of errors in uninitializedVariables().
    if (debuggerCore()->boolSetting(UseCodeModel)) {
        QStringList uninitializedVariables;
        getUninitializedVariables(debuggerCore()->cppCodeModelSnapshot(),
                                  frame.function, frame.file, frame.line, &uninitializedVariables);
        if (!uninitializedVariables.isEmpty()) {
            str << blankSeparator << "-u \"";
            int i = 0;
            foreach (const QString &u, uninitializedVariables) {
                if (i++)
                    str << ',';
                str << localsPrefixC << u;
            }
            str << '"';
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
    const int flags = forNewStackFrame ? LocalsUpdateForNewFrame : 0;
    str << blankSeparator << frameIndex;
    postExtensionCommand("locals", arguments, 0,
                         &CdbEngine::handleLocals, 0,
                         QVariant(flags));
}

void CdbEngine::selectThread(ThreadId threadId)
{
    if (!threadId.isValid() || threadId == threadsHandler()->currentThread())
        return;

    threadsHandler()->setCurrentThread(threadId);

    const QByteArray cmd = '~' + QByteArray::number(threadId.raw()) + " s";
    postBuiltinCommand(cmd, 0, &CdbEngine::dummyHandler, CommandListStack);
}

// Default address range for showing disassembly.
enum { DisassemblerRange = 512 };

/* Called with a stack frame (address and function) or just a function
 * name from the context menu. When address and function are
 * passed, try to emulate gdb's behaviour to display the whole function.
 * CDB's 'u' (disassemble) command takes a symbol,
 * but displays only 10 lines per default. So, to ensure the agent's
 * address is in that range, resolve the function symbol, cache it and
 * request the disassembly for a range that contains the agent's address. */

void CdbEngine::fetchDisassembler(DisassemblerAgent *agent)
{
    QTC_ASSERT(m_accessible, return);
    const QVariant cookie = qVariantFromValue<DisassemblerAgent*>(agent);
    const Location location = agent->location();
    if (debug)
        qDebug() << "CdbEngine::fetchDisassembler 0x"
                 << QString::number(location.address(), 16)
                 << location.from() << '!' << location.functionName();
    if (!location.functionName().isEmpty()) {
        // Resolve function (from stack frame with function and address
        // or just function from editor).
        postResolveSymbol(location.from(), location.functionName(), cookie);
    } else if (location.address()) {
        // No function, display a default range.
        postDisassemblerCommand(location.address(), cookie);
    } else {
        QTC_ASSERT(false, return);
    }
}

void CdbEngine::postDisassemblerCommand(quint64 address, const QVariant &cookie)
{
    postDisassemblerCommand(address - DisassemblerRange / 2,
                            address + DisassemblerRange / 2, cookie);
}

void CdbEngine::postDisassemblerCommand(quint64 address, quint64 endAddress,
                                        const QVariant &cookie)
{
    QByteArray cmd;
    ByteArrayInputStream str(cmd);
    str <<  "u " << hex <<hexPrefixOn << address << ' ' << endAddress;
    postBuiltinCommand(cmd, 0, &CdbEngine::handleDisassembler, 0, cookie);
}

void CdbEngine::postResolveSymbol(const QString &module, const QString &function,
                                  const QVariant &cookie)
{
    QString symbol = module.isEmpty() ? QString(QLatin1Char('*')) : module;
    symbol += QLatin1Char('!');
    symbol += function;
    const QList<quint64> addresses = m_symbolAddressCache.values(symbol);
    if (addresses.isEmpty()) {
        QVariantList cookieList;
        cookieList << QVariant(symbol) << cookie;
        showMessage(QLatin1String("Resolving symbol: ") + symbol + QLatin1String("..."), LogMisc);
        postBuiltinCommand(QByteArray("x ") + symbol.toLatin1(), 0,
                           &CdbEngine::handleResolveSymbol, 0,
                           QVariant(cookieList));
    } else {
        showMessage(QString::fromLatin1("Using cached addresses for %1.").
                    arg(symbol), LogMisc);
        handleResolveSymbol(addresses, cookie);
    }
}

// Parse address from 'x' response.
// "00000001`3f7ebe80 module!foo (void)"
static inline quint64 resolvedAddress(const QByteArray &line)
{
    const int blankPos = line.indexOf(' ');
    if (blankPos >= 0) {
        QByteArray addressBA = line.left(blankPos);
        if (addressBA.size() > 9 && addressBA.at(8) == '`')
            addressBA.remove(8, 1);
        bool ok;
        const quint64 address = addressBA.toULongLong(&ok, 16);
        if (ok)
            return address;
    }
    return 0;
}

void CdbEngine::handleResolveSymbol(const CdbBuiltinCommandPtr &command)
{
    QTC_ASSERT(command->cookie.type() == QVariant::List, return; );
    const QVariantList cookieList = command->cookie.toList();
    const QString symbol = cookieList.front().toString();
    // Insert all matches of (potentially) ambiguous symbols
    if (const int size = command->reply.size()) {
        for (int i = 0; i < size; i++) {
            if (const quint64 address = resolvedAddress(command->reply.at(i))) {
                m_symbolAddressCache.insert(symbol, address);
                showMessage(QString::fromLatin1("Obtained 0x%1 for %2 (#%3)").
                            arg(address, 0, 16).arg(symbol).arg(i + 1), LogMisc);
            }
        }
    } else {
        showMessage(QLatin1String("Symbol resolution failed: ")
                    + QString::fromLatin1(command->joinedReply()),
                    LogError);
    }
    handleResolveSymbol(m_symbolAddressCache.values(symbol), cookieList.back());
}

// Find the function address matching needle in a list of function
// addresses obtained from the 'x' command. Check for the
// mimimum POSITIVE offset (needle >= function address.)
static inline quint64 findClosestFunctionAddress(const QList<quint64> &addresses,
                                                 quint64 needle)
{
    const int size = addresses.size();
    if (!size)
        return 0;
    if (size == 1)
       return addresses.front();
    int closestIndex = 0;
    quint64 closestOffset = 0xFFFFFFFF;
    for (int i = 0; i < size; i++) {
        if (addresses.at(i) <= needle) {
            const quint64 offset = needle - addresses.at(i);
            if (offset < closestOffset) {
                closestOffset = offset;
                closestIndex = i;
            }
        }
    }
    return addresses.at(closestIndex);
}

static inline QString msgAmbiguousFunction(const QString &functionName,
                                           quint64 address,
                                           const QList<quint64> &addresses)
{
    QString result;
    QTextStream str(&result);
    str.setIntegerBase(16);
    str.setNumberFlags(str.numberFlags() | QTextStream::ShowBase);
    str << "Several overloads of function '" << functionName
        << "()' were found (";
    for (int i = 0; i < addresses.size(); ++i) {
        if (i)
            str << ", ";
        str << addresses.at(i);

    }
    str << "), using " << address << '.';
    return result;
}

void CdbEngine::handleResolveSymbol(const QList<quint64> &addresses, const QVariant &cookie)
{
    // Disassembly mode: Determine suitable range containing the
    // agent's address within the function to display.
    if (cookie.canConvert<DisassemblerAgent*>()) {
        DisassemblerAgent *agent = cookie.value<DisassemblerAgent *>();
        const quint64 agentAddress = agent->address();
        quint64 functionAddress = 0;
        quint64 endAddress = 0;
        if (agentAddress) {
            // We have an address from the agent, find closest.
            if (const quint64 closest = findClosestFunctionAddress(addresses, agentAddress)) {
                if (closest <= agentAddress) {
                    functionAddress = closest;
                    endAddress = agentAddress + DisassemblerRange / 2;
                }
            }
        } else {
            // No agent address, disassembly was started with a function name only.
            if (!addresses.isEmpty()) {
                functionAddress = addresses.first();
                endAddress = functionAddress + DisassemblerRange / 2;
                if (addresses.size() > 1)
                    showMessage(msgAmbiguousFunction(agent->location().functionName(), functionAddress, addresses), LogMisc);
            }
        }
        // Disassemble a function, else use default range around agent address
        if (functionAddress) {
            if (const quint64 remainder = endAddress % 8)
                endAddress += 8 - remainder;
            postDisassemblerCommand(functionAddress, endAddress, cookie);
        } else if (agentAddress) {
            postDisassemblerCommand(agentAddress, cookie);
        } else {
            QTC_ASSERT(false, return);
        }
        return;
    } // DisassemblerAgent
}

// Parse: "00000000`77606060 cc              int     3"
void CdbEngine::handleDisassembler(const CdbBuiltinCommandPtr &command)
{
    QTC_ASSERT(command->cookie.canConvert<DisassemblerAgent*>(), return);
    DisassemblerAgent *agent = qvariant_cast<DisassemblerAgent*>(command->cookie);
    agent->setContents(parseCdbDisassembler(command->reply));
}

void CdbEngine::fetchMemory(MemoryAgent *agent, QObject *editor, quint64 addr, quint64 length)
{
    if (debug)
        qDebug("CdbEngine::fetchMemory %llu bytes from 0x%llx", length, addr);
    const MemoryViewCookie cookie(agent, editor, addr, length);
    if (m_accessible)
        postFetchMemory(cookie);
    else
        doInterruptInferiorCustomSpecialStop(qVariantFromValue(cookie));
}

void CdbEngine::postFetchMemory(const MemoryViewCookie &cookie)
{
    QByteArray args;
    ByteArrayInputStream str(args);
    str << cookie.address << ' ' << cookie.length;
    postExtensionCommand("memory", args, 0, &CdbEngine::handleMemory, 0,
                         qVariantFromValue(cookie));
}

void CdbEngine::changeMemory(Internal::MemoryAgent *, QObject *, quint64 addr, const QByteArray &data)
{
    QTC_ASSERT(!data.isEmpty(), return);
    if (!m_accessible) {
        const MemoryChangeCookie cookie(addr, data);
        doInterruptInferiorCustomSpecialStop(qVariantFromValue(cookie));
    } else {
        postCommand(cdbWriteMemoryCommand(addr, data), 0);
    }
}

void CdbEngine::handleMemory(const CdbExtensionCommandPtr &command)
{
    QTC_ASSERT(command->cookie.canConvert<MemoryViewCookie>(), return);
    const MemoryViewCookie memViewCookie = qvariant_cast<MemoryViewCookie>(command->cookie);
    if (command->success) {
        const QByteArray data = QByteArray::fromBase64(command->reply);
        if (unsigned(data.size()) == memViewCookie.length)
            memViewCookie.agent->addLazyData(memViewCookie.editorToken,
                                             memViewCookie.address, data);
    } else {
        showMessage(QString::fromLocal8Bit(command->errorMessage), LogWarning);
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
    // Fails for core dumps.
    if (reply->success)
        notifyInferiorPid(reply->reply.toULongLong());
    if (reply->success || startParameters().startMode == AttachCore) {
        STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyInferiorSetupOk")
        notifyInferiorSetupOk();
    }  else {
        showMessage(QString::fromLatin1("Failed to determine inferior pid: %1").
                    arg(QLatin1String(reply->errorMessage)), LogError);
        STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyInferiorSetupFailed")
        notifyInferiorSetupFailed();
    }
}

// Parse CDB gdbmi register syntax.
static Register parseRegister(const GdbMi &gdbmiReg)
{
    Register reg;
    reg.name = gdbmiReg.findChild("name").data();
    const GdbMi description = gdbmiReg.findChild("description");
    if (description.type() != GdbMi::Invalid) {
        reg.name += " (";
        reg.name += description.data();
        reg.name += ')';
    }
    reg.value = gdbmiReg.findChild("value").data();
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
                module.moduleName = QString::fromLatin1(gdbmiModule.findChild("name").data());
                module.modulePath = QString::fromLatin1(gdbmiModule.findChild("image").data());
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
            registerHandler()->setAndMarkRegisters(registers);
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
    const int flags = reply->cookie.toInt();
    if (!(flags & PartialLocalsUpdate))
        watchHandler()->removeAllData();
    if (reply->success) {
        QList<WatchData> watchData;
        GdbMi root;
        root.fromString(reply->reply);
        QTC_ASSERT(root.isList(), return);
        if (debugLocals)
            qDebug() << root.toString(true, 4);
        // Courtesy of GDB engine
        foreach (const GdbMi &child, root.children()) {
            WatchData dummy;
            dummy.iname = child.findChild("iname").data();
            dummy.name = QLatin1String(child.findChild("name").data());
            parseWatchData(watchHandler()->expandedINames(), dummy, child, &watchData);
        }
        // Fix the names of watch data.
        for (int i =0; i < watchData.size(); ++i) {
            if (watchData.at(i).iname.startsWith('w')) {
                const QHash<QByteArray, QString>::const_iterator it
                    = m_watchInameToName.find(watchData.at(i).iname);
                if (it != m_watchInameToName.constEnd())
                    watchData[i].name = it.value();
            }
        }
        watchHandler()->insertData(watchData);
        if (debugLocals) {
            QDebug nsp = qDebug().nospace();
            nsp << "Obtained " << watchData.size() << " items:\n";
            foreach (const WatchData &wd, watchData)
                nsp << wd.toString() <<'\n';
        }
        if (flags & LocalsUpdateForNewFrame)
            emit stackFrameCompleted();
    } else {
        showMessage(QString::fromLatin1(reply->errorMessage), LogWarning);
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
    StopInArtificialThread = 0x40,
    StopShutdownInProgress = 0x80 // Shutdown in progress
};

static inline QString msgTracePointTriggered(BreakpointModelId id, const int number,
                                             const QString &threadId)
{
    return CdbEngine::tr("Trace point %1 (%2) in thread %3 triggered.")
            .arg(id.toString()).arg(number).arg(threadId);
}

static inline QString msgCheckingConditionalBreakPoint(BreakpointModelId id, const int number,
                                                       const QByteArray &condition,
                                                       const QString &threadId)
{
    return CdbEngine::tr("Conditional breakpoint %1 (%2) in thread %3 triggered, examining expression '%4'.")
            .arg(id.toString()).arg(number).arg(threadId, QString::fromLatin1(condition));
}

unsigned CdbEngine::examineStopReason(const GdbMi &stopReason,
                                      QString *message,
                                      QString *exceptionBoxMessage,
                                      bool conditionalBreakPointTriggered)
{
    // Report stop reason (GDBMI)
    unsigned rc  = 0;
    if (targetState() == DebuggerFinished)
        rc |= StopShutdownInProgress;
    if (debug)
        qDebug("%s", stopReason.toString(true, 4).constData());
    const QByteArray reason = stopReason.findChild("reason").data();
    if (reason.isEmpty()) {
        *message = tr("Malformed stop response received.");
        rc |= StopReportParseError|StopNotifyStop;
        return rc;
    }
    // Additional stop messages occurring for debuggee function calls (widgetAt, etc). Just log.
    if (state() == InferiorStopOk) {
        *message = QString::fromLatin1("Ignored stop notification from function call (%1).").
                    arg(QString::fromLatin1(reason));
        rc |= StopReportLog;
        return rc;
    }
    const int threadId = stopReason.findChild("threadId").data().toInt();
    if (reason == "breakpoint") {
        // Note: Internal breakpoints (run to line) are reported with id=0.
        // Step out creates temporary breakpoints with id 10000.
        int number = 0;
        BreakpointModelId id = cdbIdToBreakpointModelId(stopReason.findChild("breakpointId"));
        if (id.isValid()) {
            if (breakHandler()->engineBreakpointIds(this).contains(id)) {
                const BreakpointResponse parameters =  breakHandler()->response(id);
                if (!parameters.message.isEmpty()) {
                    showMessage(parameters.message + QLatin1Char('\n'), AppOutput);
                    showMessage(parameters.message, LogMisc);
                }
                // Trace point? Just report.
                number = parameters.id.majorPart();
                if (parameters.tracepoint) {
                    *message = msgTracePointTriggered(id, number, QString::number(threadId));
                    return StopReportLog|StopIgnoreContinue;
                }
                // Trigger evaluation of BP expression unless we are already in the response.
                if (!conditionalBreakPointTriggered && !parameters.condition.isEmpty()) {
                    *message = msgCheckingConditionalBreakPoint(id, number, parameters.condition,
                                                                QString::number(threadId));
                    ConditionalBreakPointCookie cookie(id);
                    cookie.stopReason = stopReason;
                    evaluateExpression(parameters.condition, qVariantFromValue(cookie));
                    return StopReportLog;
                }
            } else {
                id = BreakpointModelId();
            }
        }
        QString tid = QString::number(threadId);
        if (id && breakHandler()->type(id) == WatchpointAtAddress)
            *message = msgWatchpointByAddressTriggered(id, number, breakHandler()->address(id), tid);
        else if (id && breakHandler()->type(id) == WatchpointAtExpression)
            *message = msgWatchpointByExpressionTriggered(id, number, breakHandler()->expression(id), tid);
        else
            *message = msgBreakpointTriggered(id, number, tid);
        rc |= StopReportStatusMessage|StopNotifyStop;
        return rc;
    }
    if (reason == "exception") {
        WinException exception;
        exception.fromGdbMI(stopReason);
        QString description = exception.toString();
        // It is possible to hit on a startup trap or WOW86 exception while stepping (if something
        // pulls DLLs. Avoid showing a 'stopped' Message box.
        if (exception.exceptionCode == winExceptionStartupCompleteTrap
            || exception.exceptionCode == winExceptionWX86Breakpoint)
            return StopNotifyStop;
        if (exception.exceptionCode == winExceptionCtrlPressed) {
            // Detect interruption by pressing Ctrl in a console and force a switch to thread 0.
            *message = msgInterrupted();
            rc |= StopReportStatusMessage|StopNotifyStop|StopInArtificialThread;
            return rc;
        }
        if (isDebuggerWinException(exception.exceptionCode)) {
            rc |= StopReportStatusMessage|StopNotifyStop;
            // Detect interruption by DebugBreak() and force a switch to thread 0.
            if (exception.function == "ntdll!DbgBreakPoint")
                rc |= StopInArtificialThread;
            *message = msgInterrupted();
            return rc;
        }
        *exceptionBoxMessage = msgStoppedByException(description, QString::number(threadId));
        *message = description;
        rc |= StopShowExceptionMessageBox|StopReportStatusMessage|StopNotifyStop;
        return rc;
    }
    *message = msgStopped(QLatin1String(reason));
    rc |= StopReportStatusMessage|StopNotifyStop;
    return rc;
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

    switch (specialStopMode) {
    case SpecialStopSynchronizeBreakpoints:
        if (debug)
            qDebug("attemptBreakpointSynchronization in special stop");
        attemptBreakpointSynchronization();
        doContinueInferior();
        return;
    case SpecialStopGetWidgetAt:
        postWidgetAtCommand();
        return;
    case CustomSpecialStop:
        foreach (const QVariant &data, m_customSpecialStopData)
            handleCustomSpecialStop(data);
        m_customSpecialStopData.clear();
        doContinueInferior();
        return;
    case NoSpecialStop:
        break;
    }

    if (state() == EngineSetupRequested) { // Temporary stop at beginning
        STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyEngineSetupOk")
        notifyEngineSetupOk();
        // Store stop reason to be handled in runEngine().
        if (startParameters().startMode == AttachCore) {
            m_coreStopReason.reset(new GdbMi);
            m_coreStopReason->fromString(messageBA);
        }
        return;
    }

    GdbMi stopReason;
    stopReason.fromString(messageBA);
    processStop(stopReason, false);
}

void CdbEngine::processStop(const GdbMi &stopReason, bool conditionalBreakPointTriggered)
{
    // Further examine stop and report to user
    QString message;
    QString exceptionBoxMessage;
    ThreadId forcedThreadId;
    const unsigned stopFlags = examineStopReason(stopReason, &message, &exceptionBoxMessage,
                                                 conditionalBreakPointTriggered);
    // Do the non-blocking log reporting
    if (stopFlags & StopReportLog)
        showMessage(message, LogMisc);
    if (stopFlags & StopReportStatusMessage)
        showStatusMessage(message);
    if (stopFlags & StopReportParseError)
        showMessage(message, LogError);
    // Ignore things like WOW64, report tracepoints.
    if (stopFlags & StopIgnoreContinue) {
        postCommand("g", 0);
        return;
    }
    // Notify about state and send off command sequence to get stack, etc.
    if (stopFlags & StopNotifyStop) {
        if (startParameters().startMode != AttachCore) {
            if (state() == InferiorStopRequested) {
                STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyInferiorStopOk")
                        notifyInferiorStopOk();
            } else {
                STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyInferiorSpontaneousStop")
                        notifyInferiorSpontaneousStop();
            }
        }
        // Prevent further commands from being sent if shutdown is in progress
        if (stopFlags & StopShutdownInProgress) {
            showMessage(QString::fromLatin1("Shutdown request detected..."));
            return;
        }
        const bool sourceStepInto = m_sourceStepInto;
        m_sourceStepInto = false;
        // Start sequence to get all relevant data.
        if (stopFlags & StopInArtificialThread) {
            showMessage(tr("Switching to main thread..."), LogMisc);
            postCommand("~0 s", 0);
            forcedThreadId = ThreadId(0);
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
                showMessage(QString::fromLatin1(stopReason.findChild("stackerror").data()), LogError);
            }
        }
        const GdbMi threads = stopReason.findChild("threads");
        if (threads.isValid()) {
            threadsHandler()->updateThreads(threads);
            if (forcedThreadId.isValid())
                threadsHandler()->setCurrentThread(forcedThreadId);
        } else {
            showMessage(QString::fromLatin1(stopReason.findChild("threaderror").data()), LogError);
        }
        // Fire off remaining commands asynchronously
        if (!m_pendingBreakpointMap.isEmpty())
            postCommandSequence(CommandListBreakPoints);
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

    switch (s) {
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
        if (t == 'N' || debug > 1)
            nospace << ' ' << message;
        else
            nospace << ' ' << message.size() << " bytes";
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
        showStatusMessage(QString::fromLatin1(message),  5000);
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
        // Report C++ exception in application output as well.
        if (exception.exceptionCode == winExceptionCppException)
            showMessage(message + QLatin1Char('\n'), AppOutput);
        if (!isDebuggerWinException(exception.exceptionCode)) {
            const Task::TaskType type =
                    isFatalWinException(exception.exceptionCode) ? Task::Error : Task::Warning;
            const Utils::FileName fileName = exception.file.isEmpty() ?
                        Utils::FileName() :
                        Utils::FileName::fromUserInput(QString::fromLocal8Bit(exception.file));
            const Task task(type, exception.toString(false),
                            fileName, exception.lineNumber,
                            Core::Id(Debugger::Constants::TASK_CATEGORY_DEBUGGER_RUNTIME));
            taskHub()->addTask(task);
        }
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
        QTC_ASSERT(tokenEndPos != -1, return);
        const int token = line.mid(tokenPos, tokenEndPos - tokenPos).toInt();
        // remainingChunks
        const int remainingChunksPos = tokenEndPos + 1;
        const int remainingChunksEndPos = line.indexOf('|', remainingChunksPos);
        QTC_ASSERT(remainingChunksEndPos != -1, return);
        const int remainingChunks = line.mid(remainingChunksPos, remainingChunksEndPos - remainingChunksPos).toInt();
        // const char 'serviceName'
        const int whatPos = remainingChunksEndPos + 1;
        const int whatEndPos = line.indexOf('|', whatPos);
        QTC_ASSERT(whatEndPos != -1, return);
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
    if (m_ignoreCdbOutput)
        return;
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
    foreach (const BreakpointData *bp, bps)
        str << ' ' << bp->bpNumber;
    return cmd;
}
#endif

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

bool CdbEngine::acceptsBreakpoint(BreakpointModelId id) const
{
    const BreakpointParameters &data = breakHandler()->breakpointData(id);
    if (!data.isCppBreakpoint())
        return false;
    switch (data.type) {
    case UnknownType:
    case BreakpointAtFork:
    case WatchpointAtExpression:
    case BreakpointAtSysCall:
    case BreakpointOnQmlSignalEmit:
    case BreakpointAtJavaScriptThrow:
        return false;
    case WatchpointAtAddress:
    case BreakpointByFileAndLine:
    case BreakpointByFunction:
    case BreakpointByAddress:
    case BreakpointAtThrow:
    case BreakpointAtCatch:
    case BreakpointAtMain:
    case BreakpointAtExec:
        break;
    }
    return true;
}

// Context for fixing file/line-type breakpoints, for delayed creation.
class BreakpointCorrectionContext
{
public:
    explicit BreakpointCorrectionContext(const CPlusPlus::Snapshot &s,
                                         const CPlusPlus::CppModelManagerInterface::WorkingCopy &workingCopy) :
        m_snapshot(s), m_workingCopy(workingCopy) {}

    unsigned fixLineNumber(const QString &fileName, unsigned lineNumber) const;

private:
    const CPlusPlus::Snapshot m_snapshot;
    CPlusPlus::CppModelManagerInterface::WorkingCopy m_workingCopy;
};

static CPlusPlus::Document::Ptr getParsedDocument(const QString &fileName,
                                                  const CPlusPlus::CppModelManagerInterface::WorkingCopy &workingCopy,
                                                  const CPlusPlus::Snapshot &snapshot)
{
    QString src;
    if (workingCopy.contains(fileName)) {
        src = workingCopy.source(fileName);
    } else {
        Utils::FileReader reader;
        if (reader.fetch(fileName)) // ### FIXME error reporting
            src = QString::fromLocal8Bit(reader.data()); // ### FIXME encoding
    }

    CPlusPlus::Document::Ptr doc = snapshot.preprocessedDocument(src, fileName);
    doc->parse();
    return doc;
}

unsigned BreakpointCorrectionContext::fixLineNumber(const QString &fileName,
                                                    unsigned lineNumber) const
{
    CPlusPlus::Document::Ptr doc = m_snapshot.document(fileName);
    if (!doc || !doc->translationUnit()->ast())
        doc = getParsedDocument(fileName, m_workingCopy, m_snapshot);

    CPlusPlus::FindCdbBreakpoint findVisitor(doc->translationUnit());
    const unsigned correctedLine = findVisitor(lineNumber);
    if (!correctedLine) {
        qWarning("Unable to find breakpoint location for %s:%d",
                 qPrintable(QDir::toNativeSeparators(fileName)), lineNumber);
        return lineNumber;
    }
    if (debug)
        qDebug("Code model: Breakpoint line %u -> %u in %s",
               lineNumber, correctedLine, qPrintable(fileName));
    return correctedLine;
}

void CdbEngine::attemptBreakpointSynchronization()
{


    if (debug)
        qDebug("attemptBreakpointSynchronization in %s", stateName(state()));
    // Check if there is anything to be done at all.
    BreakHandler *handler = breakHandler();
    // Take ownership of the breakpoint. Requests insertion. TODO: Cpp only?
    foreach (BreakpointModelId id, handler->unclaimedBreakpointIds())
        if (acceptsBreakpoint(id))
            handler->setEngine(id, this);

    // Quick check: is there a need to change something? - Populate module cache
    bool changed = false;
    const BreakpointModelIds ids = handler->engineBreakpointIds(this);
    foreach (BreakpointModelId id, ids) {
        switch (handler->state(id)) {
        case BreakpointInsertRequested:
        case BreakpointRemoveRequested:
        case BreakpointChangeRequested:
            changed = true;
            break;
        case BreakpointInserted: {
            // Collect the new modules matching the files.
            // In the future, that information should be obtained from the build system.
            const BreakpointParameters &data = handler->breakpointData(id);
            if (data.type == BreakpointByFileAndLine && !data.module.isEmpty())
                m_fileNameModuleHash.insert(data.fileName, data.module);
        }
        break;
        default:
            break;
        }
    }

    if (debugBreakpoints)
        qDebug("attemptBreakpointSynchronizationI %dms accessible=%d, %s %d breakpoints, changed=%d",
               elapsedLogTime(), m_accessible, stateName(state()), ids.size(), changed);
    if (!changed)
        return;

    if (!m_accessible) {
        // No nested calls.
        if (m_specialStopMode != SpecialStopSynchronizeBreakpoints)
            doInterruptInferior(SpecialStopSynchronizeBreakpoints);
        return;
    }
    // Add/Change breakpoints and store pending ones in map, since
    // Breakhandler::setResponse() on pending breakpoints clears the pending flag.
    // handleBreakPoints will the complete that information and set it on the break handler.
    bool addedChanged = false;
    QScopedPointer<BreakpointCorrectionContext> lineCorrection;
    foreach (BreakpointModelId id, ids) {
        BreakpointParameters parameters = handler->breakpointData(id);
        BreakpointResponse response;
        response.fromParameters(parameters);
        response.id = BreakpointResponseId(id.majorPart(), id.minorPart());
        // If we encountered that file and have a module for it: Add it.
        if (parameters.type == BreakpointByFileAndLine && parameters.module.isEmpty()) {
            const QHash<QString, QString>::const_iterator it = m_fileNameModuleHash.constFind(parameters.fileName);
            if (it != m_fileNameModuleHash.constEnd())
                parameters.module = it.value();
        }
        switch (handler->state(id)) {
        case BreakpointInsertRequested:
            if (parameters.type == BreakpointByFileAndLine
                && m_options->breakpointCorrection) {
                if (lineCorrection.isNull())
                    lineCorrection.reset(new BreakpointCorrectionContext(debuggerCore()->cppCodeModelSnapshot(),
                                                                         CPlusPlus::CppModelManagerInterface::instance()->workingCopy()));
                response.lineNumber = lineCorrection->fixLineNumber(parameters.fileName, parameters.lineNumber);
                postCommand(cdbAddBreakpointCommand(response, m_sourcePathMappings, id, false), 0);
            } else {
                postCommand(cdbAddBreakpointCommand(parameters, m_sourcePathMappings, id, false), 0);
            }
            if (!parameters.enabled)
                postCommand("bd " + QByteArray::number(id.majorPart()), 0);
            handler->notifyBreakpointInsertProceeding(id);
            handler->notifyBreakpointInsertOk(id);
            m_pendingBreakpointMap.insert(id, response);
            addedChanged = true;
            // Ensure enabled/disabled is correct in handler and line number is there.
            handler->setResponse(id, response);
            if (debugBreakpoints)
                qDebug("Adding %d %s\n", id.toInternalId(),
                    qPrintable(response.toString()));
            break;
        case BreakpointChangeRequested:
            handler->notifyBreakpointChangeProceeding(id);
            if (debugBreakpoints)
                qDebug("Changing %d:\n    %s\nTo %s\n", id.toInternalId(),
                    qPrintable(handler->response(id).toString()),
                    qPrintable(parameters.toString()));
            if (parameters.enabled != handler->response(id).enabled) {
                // Change enabled/disabled breakpoints without triggering update.
                postCommand((parameters.enabled ? "be " : "bd ")
                    + QByteArray::number(breakPointIdToCdbId(id)), 0);
                response.pending = false;
                response.enabled = parameters.enabled;
                handler->setResponse(id, response);
            } else {
                // Delete and re-add, triggering update
                addedChanged = true;
                postCommand("bc " + QByteArray::number(breakPointIdToCdbId(id)), 0);
                postCommand(cdbAddBreakpointCommand(parameters, m_sourcePathMappings, id, false), 0);
                m_pendingBreakpointMap.insert(id, response);
            }
            handler->notifyBreakpointChangeOk(id);
            break;
        case BreakpointRemoveRequested:
            postCommand("bc " + QByteArray::number(breakPointIdToCdbId(id)), 0);
            handler->notifyBreakpointRemoveProceeding(id);
            handler->notifyBreakpointRemoveOk(id);
            m_pendingBreakpointMap.remove(id);
            break;
        default:
            break;
        }
    }
    // List breakpoints and send responses
    if (addedChanged)
        postCommandSequence(CommandListBreakPoints);
}

// Pass a file name through source mapping and normalize upper/lower case (for the editor
// manager to correctly process it) and convert to clean path.
CdbEngine::NormalizedSourceFileName CdbEngine::sourceMapNormalizeFileNameFromDebugger(const QString &f)
{
    // 1) Check cache.
    QMap<QString, NormalizedSourceFileName>::const_iterator it = m_normalizedFileCache.constFind(f);
    if (it != m_normalizedFileCache.constEnd())
        return it.value();
    if (debugSourceMapping)
        qDebug(">sourceMapNormalizeFileNameFromDebugger %s", qPrintable(f));
    // Do we have source path mappings? ->Apply.
    const QString fileName = cdbSourcePathMapping(QDir::toNativeSeparators(f), m_sourcePathMappings,
                                                  DebuggerToSource);
    // Up/lower case normalization according to Windows.
#ifdef Q_OS_WIN
    QString normalized = Utils::normalizePathName(fileName);
#else
    QString normalized = fileName;
#endif
    if (debugSourceMapping)
        qDebug(" sourceMapNormalizeFileNameFromDebugger %s->%s", qPrintable(fileName), qPrintable(normalized));
    // Check if it really exists, that is normalize worked and QFileInfo confirms it.
    const bool exists = !normalized.isEmpty() && QFileInfo(normalized).isFile();
    NormalizedSourceFileName result(QDir::cleanPath(normalized.isEmpty() ? fileName : normalized), exists);
    if (!exists) {
        // At least upper case drive letter if failed.
        if (result.fileName.size() > 2 && result.fileName.at(1) == QLatin1Char(':'))
            result.fileName[0] = result.fileName.at(0).toUpper();
    }
    m_normalizedFileCache.insert(f, result);
    if (debugSourceMapping)
        qDebug("<sourceMapNormalizeFileNameFromDebugger %s %d", qPrintable(result.fileName), result.exists);
    return result;
}

// Parse frame from GDBMI. Duplicate of the gdb code, but that
// has more processing.
static StackFrames parseFrames(const GdbMi &gdbmi, bool *incomplete = 0)
{
    if (incomplete)
        *incomplete = false;
    StackFrames rc;
    const int count = gdbmi.childCount();
    rc.reserve(count);
    for (int i = 0; i  < count; i++) {
        const GdbMi &frameMi = gdbmi.childAt(i);
        if (!frameMi.childCount()) { // Empty item indicates "More...".
            if (incomplete)
                *incomplete = true;
            break;
        }
        StackFrame frame;
        frame.level = i;
        const GdbMi fullName = frameMi.findChild("fullname");
        if (fullName.isValid()) {
            frame.file = QFile::decodeName(fullName.data());
            frame.line = frameMi.findChild("line").data().toInt();
            frame.usable = false; // To be decided after source path mapping.
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
    bool incomplete;
    StackFrames frames = parseFrames(data, &incomplete);
    const int count = frames.size();
    for (int i = 0; i < count; i++) {
        const bool hasFile = !frames.at(i).file.isEmpty();
        // jmp-frame hit by step into, do another 't' and abort sequence.
        if (!hasFile && i == 0 && sourceStepInto && frames.at(i).function.contains(QLatin1String("ILT+"))) {
            showMessage(QString::fromLatin1("Step into: Call instruction hit, performing additional step..."), LogMisc);
            return ParseStackStepInto;
        }
        if (hasFile) {
            const NormalizedSourceFileName fileName = sourceMapNormalizeFileNameFromDebugger(frames.at(i).file);
            frames[i].file = fileName.fileName;
            frames[i].usable = fileName.exists;
            if (current == -1 && frames[i].usable)
                current = i;
        }
    }
    if (count && current == -1) // No usable frame, use assembly.
        current = 0;
    // Set
    stackHandler()->setFrames(frames, incomplete);
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
        showMessage(QString::fromLocal8Bit(command->errorMessage), LogError);
    }
}

void CdbEngine::handleExpression(const CdbExtensionCommandPtr &command)
{
    int value = 0;
    if (command->success)
        value = command->reply.toInt();
    else
        showMessage(QString::fromLocal8Bit(command->errorMessage), LogError);
    // Is this a conditional breakpoint?
    if (command->cookie.isValid() && command->cookie.canConvert<ConditionalBreakPointCookie>()) {
        const ConditionalBreakPointCookie cookie = qvariant_cast<ConditionalBreakPointCookie>(command->cookie);
        const QString message = value ?
            tr("Value %1 obtained from evaluating the condition of breakpoint %2, stopping.").
            arg(value).arg(cookie.id.toString()) :
            tr("Value 0 obtained from evaluating the condition of breakpoint %1, continuing.").
            arg(cookie.id.toString());
        showMessage(message, LogMisc);
        // Stop if evaluation is true, else continue
        if (value)
            processStop(cookie.stopReason, true);
        else
            postCommand("g", 0);
    }
}

void CdbEngine::evaluateExpression(QByteArray exp, const QVariant &cookie)
{
    if (exp.contains(' ') && !exp.startsWith('"')) {
        exp.prepend('"');
        exp.append('"');
    }
    postExtensionCommand("expression", exp, 0, &CdbEngine::handleExpression, 0, cookie);
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
        postExtensionCommand("stack", "unlimited", 0, &CdbEngine::handleStackTrace, mask & ~CommandListStack);
        return;
    }
    if (mask & CommandListRegisters) {
        QTC_ASSERT(threadsHandler()->currentThreadIndex() >= 0,  return);
        postExtensionCommand("registers", QByteArray(), 0, &CdbEngine::handleRegisters, mask & ~CommandListRegisters);
        return;
    }
    if (mask & CommandListModules) {
        postExtensionCommand("modules", QByteArray(), 0, &CdbEngine::handleModules, mask & ~CommandListModules);
        return;
    }
    if (mask & CommandListBreakPoints) {
        postExtensionCommand("breakpoints", QByteArray("-v"), 0,
                             &CdbEngine::handleBreakPoints, mask & ~CommandListBreakPoints);
        return;
    }
}

void CdbEngine::handleWidgetAt(const CdbExtensionCommandPtr &reply)
{
    bool success = false;
    QString message;
    do {
        if (!reply->success) {
            message = QString::fromLatin1(reply->errorMessage);
            break;
        }
        // Should be "namespace::QWidget:0x555"
        QString watchExp = QString::fromLatin1(reply->reply);
        const int sepPos = watchExp.lastIndexOf(QLatin1Char(':'));
        if (sepPos == -1) {
            message = QString::fromLatin1("Invalid output: %1").arg(watchExp);
            break;
        }
        // 0x000 -> nothing found
        if (!watchExp.mid(sepPos + 1).toULongLong(0, 0)) {
            message = QString::fromLatin1("No widget could be found at %1, %2.").arg(m_watchPointX).arg(m_watchPointY);
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

static inline void formatCdbBreakPointResponse(BreakpointModelId id, const BreakpointResponse &r,
                                                  QTextStream &str)
{
    str << "Obtained breakpoint " << id << " (#" << r.id.majorPart() << ')';
    if (r.pending) {
        str << ", pending";
    } else {
        str.setIntegerBase(16);
        str << ", at 0x" << r.address;
        str.setIntegerBase(10);
    }
    if (!r.enabled)
        str << ", disabled";
    if (!r.module.isEmpty())
        str << ", module: '" << r.module << '\'';
    str << '\n';
}

void CdbEngine::handleBreakPoints(const CdbExtensionCommandPtr &reply)
{
    if (debugBreakpoints)
        qDebug("CdbEngine::handleBreakPoints: success=%d: %s", reply->success, reply->reply.constData());
    if (!reply->success) {
        showMessage(QString::fromLatin1(reply->errorMessage), LogError);
        return;
    }
    GdbMi value;
    value.fromString(reply->reply);
    if (value.type() != GdbMi::List) {
        showMessage(QString::fromLatin1("Unabled to parse breakpoints reply"), LogError);
        return;
    }
    handleBreakPoints(value);
}

void CdbEngine::handleBreakPoints(const GdbMi &value)
{
    // Report all obtained parameters back. Note that not all parameters are reported
    // back, so, match by id and complete
    if (debugBreakpoints)
        qDebug("\nCdbEngine::handleBreakPoints with %d", value.childCount());
    QString message;
    QTextStream str(&message);
    BreakHandler *handler = breakHandler();
    foreach (const GdbMi &breakPointG, value.children()) {
        BreakpointResponse reportedResponse;
        parseBreakPoint(breakPointG, &reportedResponse);
        if (debugBreakpoints)
            qDebug("  Parsed %d: pending=%d %s\n", reportedResponse.id.majorPart(),
                reportedResponse.pending,
                qPrintable(reportedResponse.toString()));
        if (reportedResponse.id.isValid() && !reportedResponse.pending) {
            const BreakpointModelId mid = handler->findBreakpointByResponseId(reportedResponse.id);
            if (!mid.isValid() && reportedResponse.type == BreakpointByFunction)
                continue; // Breakpoints from options, CrtDbgReport() and others.
            QTC_ASSERT(mid.isValid(), continue);
            const PendingBreakPointMap::iterator it = m_pendingBreakpointMap.find(mid);
            if (it != m_pendingBreakpointMap.end()) {
                // Complete the response and set on handler.
                BreakpointResponse &currentResponse = it.value();
                currentResponse.id = reportedResponse.id;
                currentResponse.address = reportedResponse.address;
                currentResponse.module = reportedResponse.module;
                currentResponse.pending = reportedResponse.pending;
                currentResponse.enabled = reportedResponse.enabled;
                formatCdbBreakPointResponse(mid, currentResponse, str);
                if (debugBreakpoints)
                    qDebug("  Setting for %d: %s\n", currentResponse.id.majorPart(),
                        qPrintable(currentResponse.toString()));
                handler->setResponse(mid, currentResponse);
                m_pendingBreakpointMap.erase(it);
            }
        } // not pending reported
    } // foreach
    if (m_pendingBreakpointMap.empty())
        str << QLatin1String("All breakpoints have been resolved.\n");
    else
        str << QString::fromLatin1("%1 breakpoint(s) pending...\n").arg(m_pendingBreakpointMap.size());
    showMessage(message, LogMisc);
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
                    arg(QString::fromLatin1(stateName(state()))), LogWarning);
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

void CdbEngine::handleCustomSpecialStop(const QVariant &v)
{
    if (v.canConvert<MemoryChangeCookie>()) {
        const MemoryChangeCookie changeData = qvariant_cast<MemoryChangeCookie>(v);
        postCommand(cdbWriteMemoryCommand(changeData.address, changeData.data), 0);
        return;
    }
    if (v.canConvert<MemoryViewCookie>()) {
        postFetchMemory(qvariant_cast<MemoryViewCookie>(v));
        return;
    }
}

} // namespace Internal
} // namespace Debugger
