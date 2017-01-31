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

#include "cdbengine.h"

#include "stringinputstream.h"
#include "cdboptionspage.h"
#include "cdbparsehelpers.h"

#include <debugger/breakhandler.h>
#include <debugger/debuggeractions.h>
#include <debugger/debuggercore.h>
#include <debugger/debuggerinternalconstants.h>
#include <debugger/debuggerprotocol.h>
#include <debugger/debuggermainwindow.h>
#include <debugger/debuggerstartparameters.h>
#include <debugger/debuggertooltipmanager.h>
#include <debugger/disassembleragent.h>
#include <debugger/disassemblerlines.h>
#include <debugger/memoryagent.h>
#include <debugger/moduleshandler.h>
#include <debugger/registerhandler.h>
#include <debugger/stackhandler.h>
#include <debugger/threadshandler.h>
#include <debugger/watchhandler.h>
#include <debugger/procinterrupt.h>
#include <debugger/sourceutils.h>
#include <debugger/shared/cdbsymbolpathlisteditor.h>
#include <debugger/shared/hostutils.h>

#include <coreplugin/icore.h>
#include <coreplugin/messagebox.h>
#include <projectexplorer/taskhub.h>
#include <texteditor/texteditor.h>

#include <utils/synchronousprocess.h>
#include <utils/qtcprocess.h>
#include <utils/winutils.h>
#include <utils/qtcassert.h>
#include <utils/savedaction.h>
#include <utils/consoleprocess.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>

#include <cplusplus/findcdbbreakpoint.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/cppworkingcopy.h>

#include <QDir>

#include <cctype>

enum { debug = 0 };
enum { debugLocals = 0 };
enum { debugSourceMapping = 0 };
enum { debugWatches = 0 };
enum { debugBreakpoints = 0 };

#define CB(callback) [this](const DebuggerResponse &r) { callback(r); }

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
    \li Notify the engine about the state of the debugging session:
        \list
        \li idle: (hooked up with .idle_cmd) debuggee stopped
        \li accessible: Debuggee stopped, cdb.exe accepts commands
        \li inaccessible: Debuggee runs, no way to post commands
        \li session active/inactive: Lost debuggee, terminating.
        \endlist
    \li Hook up with output/event callbacks and produce formatted output to be able
       to catch application output and exceptions.
    \li Provide some extension commands that produce output in a standardized (GDBMI)
      format that ends up in handleExtensionMessage(), for example:
      \list
      \li pid     Return debuggee pid for interrupting.
      \li locals  Print locals from SymbolGroup
      \li expandLocals Expand locals in symbol group
      \li registers, modules, threads
      \endlist
   \endlist

   Debugger commands can be posted by calling:

   \list

    \li postCommand(): Does not expect a response
    \li postBuiltinCommand(): Run a builtin-command producing free-format, multiline output
       that is captured by enclosing it in special tokens using the 'echo' command and
       then invokes a callback with a CdbBuiltinCommand structure.
    \li postExtensionCommand(): Run a command provided by the extension producing
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
using namespace Utils;

namespace Debugger {
namespace Internal {

static const char localsPrefixC[] = "local.";

struct MemoryViewCookie
{
    explicit MemoryViewCookie(MemoryAgent *a = 0, quint64 addr = 0, quint64 l = 0)
        : agent(a), address(addr), length(l)
    {}

    MemoryAgent *agent;
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

} // namespace Internal
} // namespace Debugger

Q_DECLARE_METATYPE(Debugger::Internal::MemoryViewCookie)
Q_DECLARE_METATYPE(Debugger::Internal::MemoryChangeCookie)

namespace Debugger {
namespace Internal {

static inline bool isCreatorConsole(const DebuggerRunParameters &sp)
{
    return !boolSetting(UseCdbConsole) && sp.useTerminal
           && (sp.startMode == StartInternal || sp.startMode == StartExternal);
}

// Base data structure for command queue entries with callback
class CdbCommand
{
public:
    CdbCommand() {}
    CdbCommand(CdbEngine::CommandHandler h) : handler(h) {}

    CdbEngine::CommandHandler handler;
};

static inline bool validMode(DebuggerStartMode sm)
{
    return sm != NoStartMode;
}

// Accessed by RunControlFactory
DebuggerEngine *createCdbEngine(const DebuggerRunParameters &rp, QStringList *errors)
{
    if (HostOsInfo::isWindowsHost()) {
        if (validMode(rp.startMode))
            return new CdbEngine(rp);
        errors->append(CdbEngine::tr("Internal error: Invalid start parameters passed for the CDB engine."));
    } else {
        errors->append(CdbEngine::tr("Unsupported CDB host system."));
    }
    return 0;
}

void addCdbOptionPages(QList<Core::IOptionsPage *> *opts)
{
    if (HostOsInfo::isWindowsHost()) {
        opts->push_back(new CdbOptionsPage);
        opts->push_back(new CdbPathsPage);
    }
}

#define QT_CREATOR_CDB_EXT "qtcreatorcdbext"

CdbEngine::CdbEngine(const DebuggerRunParameters &sp) :
    DebuggerEngine(sp),
    m_tokenPrefix("<token>"),
    m_effectiveStartMode(NoStartMode),
    m_accessible(false),
    m_specialStopMode(NoSpecialStop),
    m_nextCommandToken(0),
    m_currentBuiltinResponseToken(-1),
    m_extensionCommandPrefix("!" QT_CREATOR_CDB_EXT "."),
    m_operateByInstructionPending(true),
    m_operateByInstruction(true), // Default CDB setting
    m_hasDebuggee(false),
    m_wow64State(wow64Uninitialized),
    m_elapsedLogTime(0),
    m_sourceStepInto(false),
    m_watchPointX(0),
    m_watchPointY(0),
    m_ignoreCdbOutput(false)
{
    setObjectName("CdbEngine");

    connect(action(OperateByInstruction), &QAction::triggered,
            this, &CdbEngine::operateByInstructionTriggered);
    connect(action(CreateFullBacktrace), &QAction::triggered,
            this, &CdbEngine::createFullBacktrace);
    connect(&m_process, static_cast<void(QProcess::*)(int)>(&QProcess::finished),
            this, &CdbEngine::processFinished);
    connect(&m_process, &QProcess::errorOccurred, this, &CdbEngine::processError);
    connect(&m_process, &QProcess::readyReadStandardOutput,
            this, &CdbEngine::readyReadStandardOut);
    connect(&m_process, &QProcess::readyReadStandardError,
            this, &CdbEngine::readyReadStandardOut);
    connect(action(UseDebuggingHelpers), &SavedAction::valueChanged,
            this, &CdbEngine::updateLocals);
}

void CdbEngine::init()
{
    m_effectiveStartMode = NoStartMode;
    notifyInferiorPid(0);
    m_accessible = false;
    m_specialStopMode = NoSpecialStop;
    m_nextCommandToken  = 0;
    m_currentBuiltinResponseToken = -1;
    m_operateByInstructionPending = action(OperateByInstruction)->isChecked();
    m_operateByInstruction = true; // Default CDB setting
    m_hasDebuggee = false;
    m_sourceStepInto = false;
    m_watchPointX = m_watchPointY = 0;
    m_ignoreCdbOutput = false;
    m_autoBreakPointCorrection = false;
    m_wow64State = wow64Uninitialized;

    m_outputBuffer.clear();
    m_commandForToken.clear();
    m_currentBuiltinResponse.clear();
    m_extensionMessageBuffer.clear();
    m_pendingBreakpointMap.clear();
    m_insertSubBreakpointMap.clear();
    m_pendingSubBreakpointMap.clear();
    m_customSpecialStopData.clear();
    m_symbolAddressCache.clear();
    m_coreStopReason.reset();

    // Create local list of mappings in native separators
    m_sourcePathMappings.clear();
    const QSharedPointer<GlobalDebuggerOptions> globalOptions = Internal::globalDebuggerOptions();
    SourcePathMap sourcePathMap = globalOptions->sourcePathMap;
    if (!sourcePathMap.isEmpty()) {
        m_sourcePathMappings.reserve(sourcePathMap.size());
        for (auto it = sourcePathMap.constBegin(), cend = sourcePathMap.constEnd(); it != cend; ++it) {
            m_sourcePathMappings.push_back(SourcePathMapping(QDir::toNativeSeparators(it.key()),
                                                             QDir::toNativeSeparators(it.value())));
        }
    }
    // update source path maps from debugger start params
    mergeStartParametersSourcePathMap();
    QTC_ASSERT(m_process.state() != QProcess::Running, SynchronousProcess::stopProcess(m_process));
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
    runCommand({QLatin1String(m_operateByInstruction ? "l-t" : "l+t"), NoFlags});
    runCommand({QLatin1String(m_operateByInstruction ? "l-s" : "l+s"), NoFlags});
}

bool CdbEngine::canHandleToolTip(const DebuggerToolTipContext &context) const
{
    Q_UNUSED(context);
    // Tooltips matching local variables are already handled in the
    // base class. We don't handle anything else here in CDB
    // as it can slow debugging down.
    return false;
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
        cdbExtensionPath.append(';');
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
bool CdbEngine::startConsole(const DebuggerRunParameters &sp, QString *errorMessage)
{
    if (debug)
        qDebug("startConsole %s", qPrintable(sp.inferior.executable));
    m_consoleStub.reset(new ConsoleProcess);
    m_consoleStub->setMode(ConsoleProcess::Suspend);
    connect(m_consoleStub.data(), &ConsoleProcess::processError,
            this, &CdbEngine::consoleStubError);
    connect(m_consoleStub.data(), &ConsoleProcess::processStarted,
            this, &CdbEngine::consoleStubProcessStarted);
    connect(m_consoleStub.data(), &ConsoleProcess::stubStopped,
            this, &CdbEngine::consoleStubExited);
    m_consoleStub->setWorkingDirectory(sp.inferior.workingDirectory);
    if (sp.stubEnvironment.size())
        m_consoleStub->setEnvironment(sp.stubEnvironment);
    if (!m_consoleStub->start(sp.inferior.executable, sp.inferior.commandLineArguments)) {
        *errorMessage = tr("The console process \"%1\" could not be started.").arg(sp.inferior.executable);
        return false;
    }
    return true;
}

void CdbEngine::consoleStubError(const QString &msg)
{
    if (debug)
        qDebug("consoleStubProcessMessage() in %s %s", qPrintable(stateName(state())), qPrintable(msg));
    if (state() == EngineSetupRequested) {
        STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyEngineSetupFailed")
        notifyEngineSetupFailed();
    } else {
        STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyEngineIll")
        notifyEngineIll();
    }
    Core::AsynchronousMessageBox::critical(tr("Debugger Error"), msg);
}

void CdbEngine::consoleStubProcessStarted()
{
    if (debug)
        qDebug("consoleStubProcessStarted() PID=%lld", m_consoleStub->applicationPID());
    // Attach to console process.
    DebuggerRunParameters attachParameters = runParameters();
    attachParameters.inferior.executable.clear();
    attachParameters.inferior.commandLineArguments.clear();
    attachParameters.attachPID = m_consoleStub->applicationPID();
    attachParameters.startMode = AttachExternal;
    attachParameters.useTerminal = false;
    showMessage(QString("Attaching to %1...").arg(attachParameters.attachPID), LogMisc);
    QString errorMessage;
    if (!launchCDB(attachParameters, &errorMessage)) {
        showMessage(errorMessage, LogError);
        Core::AsynchronousMessageBox::critical(tr("Failed to Start the Debugger"), errorMessage);
        STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyEngineSetupFailed")
        notifyEngineSetupFailed();
    }
}

void CdbEngine::consoleStubExited()
{
}

void CdbEngine::createFullBacktrace()
{
    runCommand({"~*kp", BuiltinCommand, [this](const DebuggerResponse &response) {
        Internal::openTextEditor("Backtrace $", response.data.data());
    }});
}

void CdbEngine::setupEngine()
{
    if (debug)
        qDebug(">setupEngine");

    init();
    if (!m_logTime.elapsed())
        m_logTime.start();
    QString errorMessage;
    // Console: Launch the stub with the suspended application and attach to it
    // CDB in theory has a command line option '-2' that launches a
    // console, too, but that immediately closes when the debuggee quits.
    // Use the Creator stub instead.
    const DebuggerRunParameters &rp = runParameters();
    const bool launchConsole = isCreatorConsole(rp);
    m_effectiveStartMode = launchConsole ? AttachExternal : rp.startMode;
    const bool ok = launchConsole ?
                startConsole(runParameters(), &errorMessage) :
                launchCDB(runParameters(), &errorMessage);
    if (debug)
        qDebug("<setupEngine ok=%d", ok);
    if (!ok) {
        showMessage(errorMessage, LogError);
        Core::AsynchronousMessageBox::critical(tr("Failed to Start the Debugger"), errorMessage);
        STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyEngineSetupFailed")
        notifyEngineSetupFailed();
    }

    DisplayFormats stringFormats;
    stringFormats.append(SimpleFormat);
    stringFormats.append(SeparateFormat);

    WatchHandler *wh = watchHandler();
    wh->addTypeFormats("QString", stringFormats);
    wh->addTypeFormats("QString *", stringFormats);
    wh->addTypeFormats("QByteArray", stringFormats);
    wh->addTypeFormats("QByteArray *", stringFormats);
    wh->addTypeFormats("std__basic_string", stringFormats);  // Python dumper naming convention for std::[w]string

    DisplayFormats imageFormats;
    imageFormats.append(SimpleFormat);
    imageFormats.append(EnhancedFormat);
    wh->addTypeFormats("QImage", imageFormats);
    wh->addTypeFormats("QImage *", imageFormats);
}

bool CdbEngine::launchCDB(const DebuggerRunParameters &sp, QString *errorMessage)
{
    if (debug)
        qDebug("launchCDB startMode=%d", sp.startMode);
    const QChar blank(' ');
    // Start engine which will run until initial breakpoint:
    // Determine binary (force MSVC), extension lib name and path to use
    // The extension is passed as relative name with the path variable set
    //(does not work with absolute path names)
    const QString executable = sp.debugger.executable;
    if (executable.isEmpty()) {
        *errorMessage = tr("There is no CDB executable specified.");
        return false;
    }

    bool cdbIs64Bit = Utils::is64BitWindowsBinary(executable);
    if (!cdbIs64Bit)
        m_wow64State = noWow64Stack;
    const QFileInfo extensionFi(CdbEngine::extensionLibraryName(cdbIs64Bit));
    if (!extensionFi.isFile()) {
        *errorMessage = QString("Internal error: The extension %1 cannot be found.\n"
                                "If you build Qt Creator from sources, check out "
                                "https://code.qt.io/cgit/qt-creator/binary-artifacts.git/.").
                arg(QDir::toNativeSeparators(extensionFi.absoluteFilePath()));
        return false;
    }
    const QString extensionFileName = extensionFi.fileName();
    // Prepare arguments
    QStringList arguments;
    const bool isRemote = sp.startMode == AttachToRemoteServer;
    if (isRemote) { // Must be first
        arguments << "-remote" << sp.remoteChannel;
    } else {
        arguments << ("-a" + extensionFileName);
    }
    // Source line info/No terminal breakpoint / Pull extension
    arguments << "-lines" << "-G"
    // register idle (debuggee stop) notification
              << "-c"
              << ".idle_cmd " + m_extensionCommandPrefix + "idle";
    if (sp.useTerminal) // Separate console
        arguments << "-2";
    if (boolSetting(IgnoreFirstChanceAccessViolation))
        arguments << "-x";

    const QStringList &sourcePaths = stringListSetting(CdbSourcePaths);
    if (!sourcePaths.isEmpty())
        arguments << "-srcpath" << sourcePaths.join(';');

    QStringList symbolPaths = stringListSetting(CdbSymbolPaths);
    QString symbolPath = sp.inferior.environment.value("_NT_ALT_SYMBOL_PATH");
    if (!symbolPath.isEmpty())
        symbolPaths += symbolPath;
    symbolPath = sp.inferior.environment.value("_NT_SYMBOL_PATH");
    if (!symbolPath.isEmpty())
        symbolPaths += symbolPath;
    arguments << "-y" << (symbolPaths.isEmpty() ? "\"\"" : symbolPaths.join(';'));

    // Compile argument string preserving quotes
    QString nativeArguments = expand(stringSetting(CdbAdditionalArguments));
    switch (sp.startMode) {
    case StartInternal:
    case StartExternal:
        if (!nativeArguments.isEmpty())
            nativeArguments.push_back(blank);
        QtcProcess::addArgs(&nativeArguments,
                            QStringList(QDir::toNativeSeparators(sp.inferior.executable)));
        break;
    case AttachToRemoteServer:
        break;
    case AttachExternal:
    case AttachCrashedExternal:
        arguments << "-p" << QString::number(sp.attachPID);
        if (sp.startMode == AttachCrashedExternal) {
            arguments << "-e" << sp.crashParameter << "-g";
        } else {
            if (isCreatorConsole(runParameters()))
                arguments << "-pr" << "-pb";
        }
        break;
    case AttachCore:
        arguments << "-z" << sp.coreFile;
        break;
    default:
        *errorMessage = QString("Internal error: Unsupported start mode %1.").arg(sp.startMode);
        return false;
    }
    if (!sp.inferior.commandLineArguments.isEmpty()) { // Complete native argument string.
        if (!nativeArguments.isEmpty())
            nativeArguments.push_back(blank);
        nativeArguments += sp.inferior.commandLineArguments;
    }

    const QString msg = QString("Launching %1 %2\nusing %3 of %4.").
            arg(QDir::toNativeSeparators(executable),
                arguments.join(blank) + blank + nativeArguments,
                QDir::toNativeSeparators(extensionFi.absoluteFilePath()),
                extensionFi.lastModified().toString(Qt::SystemLocaleShortDate));
    showMessage(msg, LogMisc);

    m_outputBuffer.clear();
    m_autoBreakPointCorrection = false;
    const QStringList inferiorEnvironment = sp.inferior.environment.size() == 0 ?
                                    QProcessEnvironment::systemEnvironment().toStringList() :
                                    sp.inferior.environment.toStringList();
    m_process.setEnvironment(mergeEnvironment(inferiorEnvironment, extensionFi.absolutePath()));
    if (!sp.inferior.workingDirectory.isEmpty())
        m_process.setWorkingDirectory(sp.inferior.workingDirectory);

#ifdef Q_OS_WIN
    if (!nativeArguments.isEmpty()) // Appends
        m_process.setNativeArguments(nativeArguments);
#endif
    m_process.start(executable, arguments);
    if (!m_process.waitForStarted()) {
        *errorMessage = QString("Internal error: Cannot start process %1: %2").
                arg(QDir::toNativeSeparators(executable), m_process.errorString());
        return false;
    }

    const qint64 pid = m_process.processId();
    showMessage(QString("%1 running as %2").
                arg(QDir::toNativeSeparators(executable)).arg(pid), LogMisc);
    m_hasDebuggee = true;
    if (isRemote) { // We do not get an 'idle' in a remote session, but are accessible
        m_accessible = true;
        runCommand({".load " + extensionFileName, NoFlags});
        STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyEngineSetupOk")
        notifyEngineSetupOk();
    }
    return true;
}

void CdbEngine::setupInferior()
{
    if (debug)
        qDebug("setupInferior");
    const DebuggerRunParameters &rp = runParameters();
    if (!rp.commandsAfterConnect.isEmpty())
        runCommand({rp.commandsAfterConnect, NoFlags});
    // QmlCppEngine expects the QML engine to be connected before any breakpoints are hit
    // (attemptBreakpointSynchronization() will be directly called then)
    attemptBreakpointSynchronization();
    if (rp.breakOnMain) {
        const BreakpointParameters bp(BreakpointAtMain);
        BreakpointModelId id(quint16(-1));
        QString function = cdbAddBreakpointCommand(bp, m_sourcePathMappings, id, true);
        runCommand({function, BuiltinCommand,
                    [this, id](const DebuggerResponse &r) { handleBreakInsert(r, id); }});
    }

    runCommand({"sxn 0x4000001f", NoFlags}); // Do not break on WowX86 exceptions.
    runCommand({"sxn ibp", NoFlags}); // Do not break on initial breakpoints.
    runCommand({".asm source_line", NoFlags}); // Source line in assembly
    runCommand({m_extensionCommandPrefix + "setparameter maxStringLength="
                + action(MaximalStringLength)->value().toString()
                + " maxStackDepth="
                + action(MaximalStackDepth)->value().toString(), NoFlags});

    if (boolSetting(CdbUsePythonDumper))
        runCommand({"print(sys.version)", ScriptCommand, CB(setupScripting)});

    runCommand({"pid", ExtensionCommand, [this](const DebuggerResponse &response) {
        // Fails for core dumps.
        if (response.resultClass == ResultDone)
            notifyInferiorPid(response.data.data().toULongLong());
        if (response.resultClass == ResultDone || runParameters().startMode == AttachCore) {
            STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyInferiorSetupOk")
                    notifyInferiorSetupOk();
        }  else {
            showMessage(QString("Failed to determine inferior pid: %1").
                        arg(response.data["msg"].data()), LogError);
            STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyInferiorSetupFailed")
                    notifyInferiorSetupFailed();
        }
    }});
}

static QString msvcRunTime(const Abi::OSFlavor flavour)
{
    switch (flavour)  {
    case Abi::WindowsMsvc2005Flavor:
        return QLatin1String("MSVCR80");
    case Abi::WindowsMsvc2008Flavor:
        return QLatin1String("MSVCR90");
    case Abi::WindowsMsvc2010Flavor:
        return QLatin1String("MSVCR100");
    case Abi::WindowsMsvc2012Flavor:
        return QLatin1String("MSVCR110");
    case Abi::WindowsMsvc2013Flavor:
        return QLatin1String("MSVCR120");
    case Abi::WindowsMsvc2015Flavor:
        return QLatin1String("MSVCR140");
    default:
        break;
    }
    return QLatin1String("MSVCRT"); // MinGW, others.
}

static QString breakAtFunctionCommand(const QString &function,
                                      const QString &module = QString())
{
     QString result = "bu ";
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

    const QStringList breakEvents = stringListSetting(CdbBreakEvents);
    foreach (const QString &breakEvent, breakEvents)
        runCommand({"sxe " + breakEvent, NoFlags});
    // Break functions: each function must be fully qualified,
    // else the debugger will slow down considerably.
    const auto cb = [this](const DebuggerResponse &r) { handleBreakInsert(r, BreakpointModelId()); };
    if (boolSetting(CdbBreakOnCrtDbgReport)) {
        const QString module = msvcRunTime(runParameters().toolChainAbi.osFlavor());
        const QString debugModule = module + 'D';
        const QString wideFunc = QString::fromLatin1(CdbOptionsPage::crtDbgReport).append('W');
        runCommand({breakAtFunctionCommand(QLatin1String(CdbOptionsPage::crtDbgReport), module), BuiltinCommand, cb});
        runCommand({breakAtFunctionCommand(wideFunc, module), BuiltinCommand, cb});
        runCommand({breakAtFunctionCommand(QLatin1String(CdbOptionsPage::crtDbgReport), debugModule), BuiltinCommand, cb});
    }
    if (boolSetting(BreakOnWarning)) {
        runCommand({"bm /( QtCored4!qWarning", BuiltinCommand}); // 'bm': All overloads.
        runCommand({"bm /( Qt5Cored!QMessageLogger::warning", BuiltinCommand});
    }
    if (boolSetting(BreakOnFatal)) {
        runCommand({"bm /( QtCored4!qFatal", BuiltinCommand}); // 'bm': All overloads.
        runCommand({"bm /( Qt5Cored!QMessageLogger::fatal", BuiltinCommand});
    }
    if (runParameters().startMode == AttachCore) {
        QTC_ASSERT(!m_coreStopReason.isNull(), return; );
        notifyEngineRunOkAndInferiorUnrunnable();
        processStop(*m_coreStopReason, false);
    } else {
        doContinueInferior();
    }
}

bool CdbEngine::commandsPending() const
{
    return !m_commandForToken.isEmpty();
}

void CdbEngine::shutdownInferior()
{
    if (debug)
        qDebug("CdbEngine::shutdownInferior in state '%s', process running %d",
               qPrintable(stateName(state())), isCdbProcessRunning());

    if (!isCdbProcessRunning()) { // Direct launch: Terminated with process.
        if (debug)
            qDebug("notifyInferiorShutdownOk");
        STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyInferiorShutdownOk")
        notifyInferiorShutdownOk();
        return;
    }

    if (m_accessible) { // except console.
        if (runParameters().startMode == AttachExternal || runParameters().startMode == AttachCrashedExternal)
            detachDebugger();
        STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyInferiorShutdownOk")
        notifyInferiorShutdownOk();
    } else {
        // A command got stuck.
        if (commandsPending()) {
            showMessage("Cannot shut down inferior due to pending commands.", LogWarning);
            STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyInferiorShutdownFailed")
            notifyInferiorShutdownFailed();
            return;
        }
        if (!canInterruptInferior()) {
            showMessage("Cannot interrupt the inferior.", LogWarning);
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
               qPrintable(stateName(state())), isCdbProcessRunning(), m_accessible,
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
        if (runParameters().startMode == AttachExternal || runParameters().startMode == AttachCrashedExternal)
            detachDebugger();
        // Remote requires a bit more force to quit.
        if (m_effectiveStartMode == AttachToRemoteServer) {
            runCommand({m_extensionCommandPrefix + "shutdownex", NoFlags});
            runCommand({"qq", NoFlags});
        } else {
            runCommand({"q", NoFlags});
        }
    } else {
        // Remote process. No can do, currently
        SynchronousProcess::stopProcess(m_process);
    }
}

void CdbEngine::abortDebugger()
{
    if (targetState() == DebuggerFinished) {
        // We already tried. Try harder.
        showMessage("ABORTING DEBUGGER. SECOND TIME.");
        m_process.kill();
    } else {
        // Be friendly the first time. This will change targetState().
        showMessage("ABORTING DEBUGGER. FIRST TIME.");
        quitDebugger();
    }
}

void CdbEngine::processFinished()
{
    if (debug)
        qDebug("CdbEngine::processFinished %dms '%s' (exit state=%d, ex=%d)",
               elapsedLogTime(), qPrintable(stateName(state())),
               m_process.exitStatus(), m_process.exitCode());

    notifyDebuggerProcessFinished(m_process.exitCode(), m_process.exitStatus(), "CDB");
}

void CdbEngine::detachDebugger()
{
    runCommand({".detach", NoFlags});
}

static inline bool isWatchIName(const QString &iname)
{
    return iname.startsWith("watch");
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
           |CreateFullBacktraceCapability
           |OperateByInstructionCapability
           |RunToLineCapability
           |MemoryAddressCapability
           |AdditionalQmlStackCapability);
}

void CdbEngine::executeStep()
{
    if (!m_operateByInstruction)
        m_sourceStepInto = true; // See explanation at handleStackTrace().
    runCommand({"t", NoFlags}); // Step into-> t (trace)
    STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyInferiorRunRequested")
    notifyInferiorRunRequested();
}

void CdbEngine::executeStepOut()
{
    runCommand({"gu", NoFlags}); // Step out-> gu (go up)
    STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyInferiorRunRequested")
    notifyInferiorRunRequested();
}

void CdbEngine::executeNext()
{
    runCommand({"p", NoFlags}); // Step over -> p
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
    runCommand({"g", NoFlags});
}

bool CdbEngine::canInterruptInferior() const
{
    return m_effectiveStartMode != AttachToRemoteServer && inferiorPid();
}

void CdbEngine::interruptInferior()
{
    if (debug)
        qDebug() << "CdbEngine::interruptInferior()" << stateName(state());

    if (!canInterruptInferior()) {
        // Restore running state if inferior can't be stoped.
        showMessage(tr("Interrupting is not possible in remote sessions."), LogError);
        STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyInferiorStopOk")
        notifyInferiorStopOk();
        STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyInferiorRunRequested")
        notifyInferiorRunRequested();
        STATE_DEBUG(state(), Q_FUNC_INFO, __LINE__, "notifyInferiorRunOk")
        notifyInferiorRunOk();
        return;
    }
    doInterruptInferior(NoSpecialStop);
}

void CdbEngine::doInterruptInferiorCustomSpecialStop(const QVariant &v)
{
    if (m_specialStopMode == NoSpecialStop)
        doInterruptInferior(CustomSpecialStop);
    m_customSpecialStopData.push_back(v);
}

void CdbEngine::handleDoInterruptInferior(const QString &errorMessage)
{
    if (errorMessage.isEmpty()) {
        showMessage("Interrupted " + QString::number(inferiorPid()));
    } else {
        showMessage(errorMessage, LogError);
        notifyInferiorStopFailed();
    }
    m_signalOperation->disconnect(this);
    m_signalOperation.clear();
}

void CdbEngine::doInterruptInferior(SpecialStopMode sm)
{
    showMessage(QString("Interrupting process %1...").arg(inferiorPid()), LogMisc);

    QTC_ASSERT(!m_signalOperation, notifyInferiorStopFailed();  return;);
    m_signalOperation = runParameters().device->signalOperation();
    m_specialStopMode = sm;
    QTC_ASSERT(m_signalOperation, notifyInferiorStopFailed(); return;);
    connect(m_signalOperation.data(), &DeviceProcessSignalOperation::finished,
            this, &CdbEngine::handleDoInterruptInferior);

    m_signalOperation->setDebuggerCommand(runParameters().debugger.executable);
    m_signalOperation->interruptProcess(inferiorPid());
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

    runCommand({cdbAddBreakpointCommand(bp, m_sourcePathMappings, BreakpointModelId(), true), BuiltinCommand,
               [this](const DebuggerResponse &r) { handleBreakInsert(r, BreakpointModelId()); }});
    continueInferior();
}

void CdbEngine::executeRunToFunction(const QString &functionName)
{
    // Add one-shot breakpoint
    BreakpointParameters bp(BreakpointByFunction);
    bp.functionName = functionName;
    runCommand({cdbAddBreakpointCommand(bp, m_sourcePathMappings, BreakpointModelId(), true), BuiltinCommand,
               [this](const DebuggerResponse &r) { handleBreakInsert(r, BreakpointModelId()); }});
    continueInferior();
}

void CdbEngine::setRegisterValue(const QString &name, const QString &value)
{
    // Value is decimal or 0x-hex-prefixed
    runCommand({"r " + name + '=' + value, NoFlags});
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
        QString cmd;
        StringInputStream str(cmd);
        str << "? `" << QDir::toNativeSeparators(data.fileName) << ':' << data.lineNumber << '`';
        runCommand({cmd, BuiltinCommand, [this, data](const DebuggerResponse &r) {
                        handleJumpToLineAddressResolution(r, data); }});
    }
}

void CdbEngine::jumpToAddress(quint64 address)
{
    // Fake a jump to address by setting the PC register.
    QString cmd;
    StringInputStream str(cmd);
    // PC-register depending on 64/32bit.
    str << "r " << (runParameters().toolChainAbi.wordWidth() == 64 ? "rip" : "eip") << '=';
    str.setHexPrefix(true);
    str.setIntegerBase(16);
    str << address;
    runCommand({cmd, NoFlags});
}

void CdbEngine::handleJumpToLineAddressResolution(const DebuggerResponse &response, const ContextData &context)
{
    if (response.data.data().isEmpty())
        return;
    // Evaluate expression: 5365511549 = 00000001`3fcf357d
    // Set register 'rip' to hex address and goto lcoation
    QString answer = response.data.data().trimmed();
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
        jumpToAddress(address);
        gotoLocation(Location(context.fileName, context.lineNumber));
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

void CdbEngine::assignValueInDebugger(WatchItem *w, const QString &expr, const QVariant &value)
{
    if (debug)
        qDebug() << "CdbEngine::assignValueInDebugger" << w->iname << expr << value;

    if (state() != InferiorStopOk || stackHandler()->currentIndex() < 0) {
        qWarning("Internal error: assignValueInDebugger: Invalid state or no stack frame.");
        return;
    }
    QString cmd;
    StringInputStream str(cmd);
    switch (value.type()) {
    case QVariant::String: {
        // Convert qstring to Utf16 data not considering endianness for Windows.
        const QString s = value.toString();
        if (isAsciiWord(s)) {
            str << m_extensionCommandPrefix << "assign \"" << w->iname << '=' << s << '"';
        } else {
            const QByteArray utf16(reinterpret_cast<const char *>(s.utf16()), 2 * s.size());
            str << m_extensionCommandPrefix << "assign -u " << w->iname << '='
                << QString::fromLatin1(utf16.toHex());
        }
    }
        break;
    default:
        str << m_extensionCommandPrefix << "assign " << w->iname << '='
            << value.toString();
        break;
    }

    runCommand({cmd, NoFlags});
    // Update all locals in case we change a union or something pointed to
    // that affects other variables, too.
    updateLocals();
}

void CdbEngine::handleThreads(const DebuggerResponse &response)
{
    if (debug) {
        qDebug("CdbEngine::handleThreads %s",
               qPrintable(DebuggerResponse::stringFromResultClass(response.resultClass)));
    }
    if (response.resultClass == ResultDone) {
        threadsHandler()->updateThreads(response.data);
        // Continue sequence
        reloadFullStack();
    } else {
        showMessage(response.data["msg"].data(), LogError);
    }
}

void CdbEngine::executeDebuggerCommand(const QString &command, DebuggerLanguages languages)
{
    if (languages & CppLanguage)
        runCommand({command, NoFlags});
}

// Post command to the cdb process
void CdbEngine::runCommand(const DebuggerCommand &dbgCmd)
{
    QString cmd = dbgCmd.function + dbgCmd.argsToString();
    if (!m_accessible) {
        const QString msg = QString("Attempt to issue command \"%1\" to non-accessible session (%2)")
                .arg(cmd, stateName(state()));
        showMessage(msg, LogError);
        return;
    }

    QString fullCmd;
    if (dbgCmd.flags == NoFlags) {
        fullCmd = cmd;
    } else {
        const int token = m_nextCommandToken++;
        StringInputStream str(fullCmd);
        if (dbgCmd.flags == BuiltinCommand) {
            // Post a built-in-command producing free-format output with a callback.
            // In order to catch the output, it is enclosed in 'echo' commands
            // printing a specially formatted token to be identifiable in the output.
            str << ".echo \"" << m_tokenPrefix << token << "<\"\n"
                << cmd << "\n"
                << ".echo \"" << m_tokenPrefix << token << ">\"";
        } else if (dbgCmd.flags == ExtensionCommand) {
            // Post an extension command producing one-line output with a callback,
            // pass along token for identification in hash.
            str << m_extensionCommandPrefix << dbgCmd.function << "%1%2";
            if (dbgCmd.args.isString())
                str <<  ' ' << dbgCmd.argsToString();
            cmd = fullCmd.arg("", "");
            fullCmd = fullCmd.arg(" -t ").arg(token);
        } else if (dbgCmd.flags == ScriptCommand) {
            // Add extension prefix and quotes the script command
            // pass along token for identification in hash.
            str << m_extensionCommandPrefix + "script %1%2 " << dbgCmd.function;
            if (!dbgCmd.args.isNull())
                str << '(' << dbgCmd.argsToPython() << ')';
            cmd = fullCmd.arg("", "");
            fullCmd = fullCmd.arg(" -t ").arg(token);
        }
        m_commandForToken.insert(token, dbgCmd);
    }
    if (debug) {
        qDebug("CdbEngine::postCommand %dms '%s' %s, pending=%d",
               elapsedLogTime(), qPrintable(dbgCmd.function), qPrintable(stateName(state())),
               m_commandForToken.size());
    }
    if (debug > 1) {
        qDebug("CdbEngine::postCommand: resulting command '%s'\n", qPrintable(fullCmd));
    }
    showMessage(cmd, LogInput);
    m_process.write(fullCmd.toLocal8Bit() + '\n');
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
    if (frame.language != CppLanguage) {
        gotoLocation(frame);
        return;
    }

    if (debug || debugLocals)
        qDebug("activateFrame idx=%d '%s' %d", index,
               qPrintable(frame.file), frame.line);
    stackHandler()->setCurrentIndex(index);
    gotoLocation(frame);
    if (m_pythonVersion > 0x030000)
        runCommand({".frame " + QString::number(index), NoFlags});
    updateLocals();
}

void CdbEngine::doUpdateLocals(const UpdateParameters &updateParameters)
{
    if (m_pythonVersion > 0x030000) {
        watchHandler()->notifyUpdateStarted(updateParameters);

        DebuggerCommand cmd("theDumper.fetchVariables", ScriptCommand);
        watchHandler()->appendFormatRequests(&cmd);
        watchHandler()->appendWatchersAndTooltipRequests(&cmd);

        const static bool alwaysVerbose = !qgetenv("QTC_DEBUGGER_PYTHON_VERBOSE").isEmpty();
        cmd.arg("passexceptions", alwaysVerbose);
        cmd.arg("fancy", boolSetting(UseDebuggingHelpers));
        cmd.arg("autoderef", boolSetting(AutoDerefPointers));
        cmd.arg("dyntype", boolSetting(UseDynamicType));
        cmd.arg("partialvar", updateParameters.partialVariable);
        cmd.arg("qobjectnames", boolSetting(ShowQObjectNames));

        StackFrame frame = stackHandler()->currentFrame();
        cmd.arg("context", frame.context);
        cmd.arg("nativemixed", isNativeMixedActive());

        cmd.arg("stringcutoff", action(MaximalStringLength)->value().toString());
        cmd.arg("displaystringlimit", action(DisplayStringLimit)->value().toString());

        cmd.callback = [this](const DebuggerResponse &response) {
            if (response.resultClass == ResultDone) {
                showMessage(response.data.toString(), LogMisc);
                updateLocalsView(response.data);
            } else {
                showMessage(response.data["msg"].data(), LogError);
            }
            watchHandler()->notifyUpdateFinished();
        };

        runCommand(cmd);
    } else {

        const bool partialUpdate = !updateParameters.partialVariable.isEmpty();
        const bool isWatch = isWatchIName(updateParameters.partialVariable);

        const int frameIndex = stackHandler()->currentIndex();
        if (frameIndex < 0 && !isWatch) {
            watchHandler()->removeAllData();
            return;
        }
        const StackFrame frame = stackHandler()->currentFrame();
        if (!frame.isUsable()) {
            watchHandler()->removeAllData();
            return;
        }

        watchHandler()->notifyUpdateStarted(updateParameters);

        /* Watchers: Forcibly discard old symbol group as switching from
     * thread 0/frame 0 -> thread 1/assembly -> thread 0/frame 0 will otherwise re-use it
     * and cause errors as it seems to go 'stale' when switching threads.
     * Initial expand, get uninitialized and query */
        QString arguments;
        StringInputStream str(arguments);

        if (!partialUpdate) {
            str << "-D";
            // Pre-expand
            const QSet<QString> expanded = watchHandler()->expandedINames();
            if (!expanded.isEmpty()) {
                str << blankSeparator << "-e ";
                int i = 0;
                foreach (const QString &e, expanded) {
                    if (i++)
                        str << ',';
                    str << e;
                }
            }
        }
        str << blankSeparator << "-v";
        if (boolSetting(UseDebuggingHelpers))
            str << blankSeparator << "-c";
        if (boolSetting(SortStructMembers))
            str << blankSeparator << "-a";
        const QString typeFormats = watchHandler()->typeFormatRequests();
        if (!typeFormats.isEmpty())
            str << blankSeparator << "-T " << typeFormats;
        const QString individualFormats = watchHandler()->individualFormatRequests();
        if (!individualFormats.isEmpty())
            str << blankSeparator << "-I " << individualFormats;
        // Uninitialized variables if desired. Quote as safeguard against shadowed
        // variables in case of errors in uninitializedVariables().
        if (boolSetting(UseCodeModel)) {
            QStringList uninitializedVariables;
            getUninitializedVariables(Internal::cppCodeModelSnapshot(),
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
        // Perform watches synchronization only for full updates
        if (!partialUpdate)
            str << blankSeparator << "-W";
        if (!partialUpdate || isWatch) {
            const QMap<QString, int> watchers = WatchHandler::watcherNames();
            if (!watchers.isEmpty()) {
                for (auto it = watchers.constBegin(), cend = watchers.constEnd(); it != cend; ++it) {
                    str << blankSeparator << "-w " << "watch." + QString::number(it.value())
                        << " \"" << it.key() << '"';
                }
            }
        }

        // Required arguments: frame
        str << blankSeparator << frameIndex;

        if (partialUpdate)
            str << blankSeparator << updateParameters.partialVariable;

        DebuggerCommand cmd("locals", ExtensionCommand);
        cmd.args = arguments;
        cmd.callback = [this, partialUpdate](const DebuggerResponse &r) { handleLocals(r, partialUpdate); };
        runCommand(cmd);
    }
}

void CdbEngine::updateAll()
{
    updateLocals();
}

void CdbEngine::selectThread(ThreadId threadId)
{
    if (!threadId.isValid() || threadId == threadsHandler()->currentThread())
        return;

    threadsHandler()->setCurrentThread(threadId);

    runCommand({'~' + QString::number(threadId.raw()) + " s", BuiltinCommand,
               [this](const DebuggerResponse &) { reloadFullStack(); }});
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
    const Location location = agent->location();
    if (debug)
        qDebug() << "CdbEngine::fetchDisassembler 0x"
                 << QString::number(location.address(), 16)
                 << location.from() << '!' << location.functionName();
    if (!location.functionName().isEmpty()) {
        // Resolve function (from stack frame with function and address
        // or just function from editor).
        postResolveSymbol(location.from(), location.functionName(), agent);
    } else if (location.address()) {
        // No function, display a default range.
        postDisassemblerCommand(location.address(), agent);
    } else {
        QTC_ASSERT(false, return);
    }
}

void CdbEngine::postDisassemblerCommand(quint64 address, DisassemblerAgent *agent)
{
    postDisassemblerCommand(address - DisassemblerRange / 2,
                            address + DisassemblerRange / 2, agent);
}

void CdbEngine::postDisassemblerCommand(quint64 address, quint64 endAddress,
                                        DisassemblerAgent *agent)
{
    QString ba;
    StringInputStream str(ba);
    str <<  "u " << hex <<hexPrefixOn << address << ' ' << endAddress;
    DebuggerCommand cmd;
    cmd.function = ba;
    cmd.callback = [this, agent](const DebuggerResponse &response) {
        // Parse: "00000000`77606060 cc              int     3"
        agent->setContents(parseCdbDisassembler(response.data.data()));
    };
    cmd.flags = BuiltinCommand;
    runCommand(cmd);
}

void CdbEngine::postResolveSymbol(const QString &module, const QString &function,
                                  DisassemblerAgent *agent)
{
    QString symbol = module.isEmpty() ? QString('*') : module;
    symbol += '!';
    symbol += function;
    const QList<quint64> addresses = m_symbolAddressCache.values(symbol);
    if (addresses.isEmpty()) {
        showMessage("Resolving symbol: " + symbol + "...", LogMisc);
        runCommand({"x " + symbol, BuiltinCommand,
                    [this, symbol, agent](const DebuggerResponse &r) { handleResolveSymbol(r, symbol, agent); }});
    } else {
        showMessage(QString("Using cached addresses for %1.").arg(symbol), LogMisc);
        handleResolveSymbolHelper(addresses, agent);
    }
}

// Parse address from 'x' response.
// "00000001`3f7ebe80 module!foo (void)"
static inline quint64 resolvedAddress(const QString &line)
{
    const int blankPos = line.indexOf(' ');
    if (blankPos >= 0) {
        QString addressBA = line.left(blankPos);
        if (addressBA.size() > 9 && addressBA.at(8) == '`')
            addressBA.remove(8, 1);
        bool ok;
        const quint64 address = addressBA.toULongLong(&ok, 16);
        if (ok)
            return address;
    }
    return 0;
}

void CdbEngine::handleResolveSymbol(const DebuggerResponse &response, const QString &symbol,
                                    DisassemblerAgent *agent)
{
    // Insert all matches of (potentially) ambiguous symbols
    if (!response.data.data().isEmpty()) {
        foreach (const QString &line, response.data.data().split('\n')) {
            if (const quint64 address = resolvedAddress(line)) {
                m_symbolAddressCache.insert(symbol, address);
                showMessage(QString("Obtained 0x%1 for %2").
                            arg(address, 0, 16).arg(symbol), LogMisc);
            }
        }
    } else {
        showMessage("Symbol resolution failed: " + response.data["msg"].data(), LogError);
    }
    handleResolveSymbolHelper(m_symbolAddressCache.values(symbol), agent);
}

// Find the function address matching needle in a list of function
// addresses obtained from the 'x' command. Check for the
// minimum POSITIVE offset (needle >= function address.)
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

void CdbEngine::handleResolveSymbolHelper(const QList<quint64> &addresses, DisassemblerAgent *agent)
{
    // Disassembly mode: Determine suitable range containing the
    // agent's address within the function to display.
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
        postDisassemblerCommand(functionAddress, endAddress, agent);
    } else if (agentAddress) {
        postDisassemblerCommand(agentAddress, agent);
    } else {
        QTC_CHECK(false);
    }
}

void CdbEngine::fetchMemory(MemoryAgent *agent, quint64 addr, quint64 length)
{
    if (debug)
        qDebug("CdbEngine::fetchMemory %llu bytes from 0x%llx", length, addr);
    const MemoryViewCookie cookie(agent, addr, length);
    if (m_accessible)
        postFetchMemory(cookie);
    else
        doInterruptInferiorCustomSpecialStop(qVariantFromValue(cookie));
}

void CdbEngine::postFetchMemory(const MemoryViewCookie &cookie)
{
    DebuggerCommand cmd("memory", ExtensionCommand);
    QString args;
    StringInputStream str(args);
    str << cookie.address << ' ' << cookie.length;
    cmd.args = args;
    cmd.callback = [this, cookie](const DebuggerResponse &response) {
        if (!cookie.agent)
            return;
        if (response.resultClass == ResultDone) {
            const QByteArray data = QByteArray::fromHex(response.data.data().toUtf8());
            if (unsigned(data.size()) == cookie.length)
                cookie.agent->addData(cookie.address, data);
        } else {
            showMessage(response.data["msg"].data(), LogWarning);
            cookie.agent->addData(cookie.address, QByteArray(int(cookie.length), char()));
        }
    };
    runCommand(cmd);
}

void CdbEngine::changeMemory(MemoryAgent *, quint64 addr, const QByteArray &data)
{
    QTC_ASSERT(!data.isEmpty(), return);
    if (!m_accessible) {
        const MemoryChangeCookie cookie(addr, data);
        doInterruptInferiorCustomSpecialStop(qVariantFromValue(cookie));
    } else {
        runCommand({cdbWriteMemoryCommand(addr, data), NoFlags});
    }
}

void CdbEngine::reloadModules()
{
    runCommand({"modules", ExtensionCommand, CB(handleModules)});
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
    QTC_ASSERT(threadsHandler()->currentThreadIndex() >= 0,  return);
    runCommand({"registers", ExtensionCommand, CB(handleRegistersExt)});
}

void CdbEngine::reloadSourceFiles()
{
}

void CdbEngine::reloadFullStack()
{
    if (debug)
        qDebug("%s", Q_FUNC_INFO);
    DebuggerCommand cmd("stack", ExtensionCommand);
    cmd.args = QStringLiteral("unlimited");
    cmd.callback = CB(handleStackTrace);
    runCommand(cmd);
}

void CdbEngine::listBreakpoints()
{
    DebuggerCommand cmd("breakpoints", ExtensionCommand);
    cmd.args = QStringLiteral("-v");
    cmd.callback = CB(handleBreakPoints);
    runCommand(cmd);
}

void CdbEngine::handleModules(const DebuggerResponse &response)
{
    if (response.resultClass == ResultDone) {
        if (response.data.type() == GdbMi::List) {
            ModulesHandler *handler = modulesHandler();
            handler->beginUpdateAll();
            foreach (const GdbMi &gdbmiModule, response.data.children()) {
                Module module;
                module.moduleName = gdbmiModule["name"].data();
                module.modulePath = gdbmiModule["image"].data();
                module.startAddress = gdbmiModule["start"].data().toULongLong(0, 0);
                module.endAddress = gdbmiModule["end"].data().toULongLong(0, 0);
                if (gdbmiModule["deferred"].type() == GdbMi::Invalid)
                    module.symbolsRead = Module::ReadOk;
                handler->updateModule(module);
            }
            handler->endUpdateAll();
        } else {
            showMessage("Parse error in modules response.", LogError);
            qWarning("Parse error in modules response:\n%s", qPrintable(response.data.data()));
        }
    }  else {
        showMessage(QString("Failed to determine modules: %1").
                    arg(response.data["msg"].data()), LogError);
    }
}

void CdbEngine::handleRegistersExt(const DebuggerResponse &response)
{
    if (response.resultClass == ResultDone) {
        if (response.data.type() == GdbMi::List) {
            RegisterHandler *handler = registerHandler();
            foreach (const GdbMi &item, response.data.children()) {
                Register reg;
                reg.name = item["name"].data();
                reg.description = item["description"].data();
                reg.reportedType = item["type"].data();
                if (reg.reportedType.startsWith('I'))
                    reg.kind = IntegerRegister;
                else if (reg.reportedType.startsWith('F'))
                    reg.kind = FloatRegister;
                else if (reg.reportedType.startsWith('V'))
                    reg.kind = VectorRegister;
                else
                    reg.kind = OtherRegister;
                reg.value.fromString(item["value"].data(), HexadecimalFormat);
                reg.size = item["size"].data().toInt();
                handler->updateRegister(reg);
            }
            handler->commitUpdates();
        } else {
            showMessage("Parse error in registers response.", LogError);
            qWarning("Parse error in registers response:\n%s", qPrintable(response.data.data()));
        }
    }  else {
        showMessage(QString("Failed to determine registers: %1").
                    arg(response.data["msg"].data()), LogError);
    }
}

void CdbEngine::handleLocals(const DebuggerResponse &response, bool partialUpdate)
{
    if (response.resultClass == ResultDone) {
        showMessage(response.data.toString(), LogDebug);

        GdbMi partial;
        partial.m_name = "partial";
        partial.m_data = QString::number(partialUpdate ? 1 : 0);

        GdbMi all;
        all.m_children.push_back(response.data);
        all.m_children.push_back(partial);
        updateLocalsView(all);
    } else {
        showMessage(response.data["msg"].data(), LogWarning);
    }
    watchHandler()->notifyUpdateFinished();
}

void CdbEngine::handleExpandLocals(const DebuggerResponse &response)
{
    if (response.resultClass != ResultDone)
        showMessage(response.data["msg"].data(), LogError);
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
                                                       const QString &condition,
                                                       const QString &threadId)
{
    return CdbEngine::tr("Conditional breakpoint %1 (%2) in thread %3 triggered, examining expression \"%4\".")
            .arg(id.toString()).arg(number).arg(threadId, condition);
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
        qDebug("%s", qPrintable(stopReason.toString(true, 4)));
    const QString reason = stopReason["reason"].data();
    if (reason.isEmpty()) {
        *message = tr("Malformed stop response received.");
        rc |= StopReportParseError|StopNotifyStop;
        return rc;
    }
    // Additional stop messages occurring for debuggee function calls (widgetAt, etc). Just log.
    if (state() == InferiorStopOk) {
        *message = QString("Ignored stop notification from function call (%1).").arg(reason);
        rc |= StopReportLog;
        return rc;
    }
    const int threadId = stopReason["threadId"].toInt();
    if (reason == "breakpoint") {
        // Note: Internal breakpoints (run to line) are reported with id=0.
        // Step out creates temporary breakpoints with id 10000.
        int number = 0;
        BreakpointModelId id = cdbIdToBreakpointModelId(stopReason["breakpointId"]);
        Breakpoint bp = breakHandler()->breakpointById(id);
        if (bp) {
            if (bp.engine() == this) {
                const BreakpointResponse parameters =  bp.response();
                if (!parameters.message.isEmpty()) {
                    showMessage(parameters.message + '\n', AppOutput);
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
                    QString args = parameters.condition;
                    if (args.contains(' ') && !args.startsWith('"')) {
                        args.prepend('"');
                        args.append('"');
                    }
                    DebuggerCommand cmd("expression", ExtensionCommand);
                    cmd.args = args;
                    cmd.callback = [this, id, stopReason](const DebuggerResponse &response) {
                        handleExpression(response, id, stopReason);
                    };
                    runCommand(cmd);

                    return StopReportLog;
                }
            } else {
                bp = Breakpoint();
            }
        }
        QString tid = QString::number(threadId);
        if (bp.type() == WatchpointAtAddress)
            *message = bp.msgWatchpointByAddressTriggered(number, bp.address(), tid);
        else if (bp.type() == WatchpointAtExpression)
            *message = bp.msgWatchpointByExpressionTriggered(number, bp.expression(), tid);
        else
            *message = bp.msgBreakpointTriggered(number, tid);
        rc |= StopReportStatusMessage|StopNotifyStop;
        return rc;
    }
    if (reason == "exception") {
        WinException exception;
        exception.fromGdbMI(stopReason);
        QString description = exception.toString();
        // It is possible to hit on a set thread name or WOW86 exception while stepping (if something
        // pulls DLLs. Avoid showing a 'stopped' Message box.
        if (exception.exceptionCode == winExceptionSetThreadName
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
    *message = msgStopped(reason);
    rc |= StopReportStatusMessage|StopNotifyStop;
    return rc;
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
        doContinueInferior();
        return;
    }
    // Notify about state and send off command sequence to get stack, etc.
    if (stopFlags & StopNotifyStop) {
        if (runParameters().startMode != AttachCore) {
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
            showMessage("Shutdown request detected...");
            return;
        }
        const bool sourceStepInto = m_sourceStepInto;
        m_sourceStepInto = false;
        // Start sequence to get all relevant data.
        if (stopFlags & StopInArtificialThread) {
            showMessage(tr("Switching to main thread..."), LogMisc);
            runCommand({"~0 s", NoFlags});
            forcedThreadId = ThreadId(0);
            // Re-fetch stack again.
            reloadFullStack();
        } else {
            const GdbMi stack = stopReason["stack"];
            if (stack.isValid()) {
                switch (parseStackTrace(stack, sourceStepInto)) {
                case ParseStackStepInto: // Hit on a frame while step into, see parseStackTrace().
                    executeStep();
                    return;
                case ParseStackStepOut: // Hit on a frame with no source while step into.
                    executeStepOut();
                    return;
                case ParseStackWow64:
                    runCommand({"lm m wow64", BuiltinCommand,
                               [this, stack](const DebuggerResponse &r) { handleCheckWow64(r, stack); }});
                    break;
                }
            } else {
                showMessage(stopReason["stackerror"].data(), LogError);
            }
        }
        const GdbMi threads = stopReason["threads"];
        if (threads.isValid()) {
            threadsHandler()->updateThreads(threads);
            if (forcedThreadId.isValid())
                threadsHandler()->setCurrentThread(forcedThreadId);
        } else {
            showMessage(stopReason["threaderror"].data(), LogError);
        }
        // Fire off remaining commands asynchronously
        if (!m_pendingBreakpointMap.isEmpty() && !m_pendingSubBreakpointMap.isEmpty())
            listBreakpoints();
        if (Internal::isRegistersWindowVisible())
            reloadRegisters();
        if (Internal::isModulesWindowVisible())
            reloadModules();
    }
    // After the sequence has been sent off and CDB is pondering the commands,
    // pop up a message box for exceptions.
    if (stopFlags & StopShowExceptionMessageBox)
        showStoppedByExceptionMessageBox(exceptionBoxMessage);
}

void CdbEngine::handleBreakInsert(const DebuggerResponse &response, const BreakpointModelId &bpId)
{
    const QStringList reply = response.data.data().split('\n');
    if (reply.isEmpty())
        return;
    foreach (const QString &line, reply)
        showMessage(line);
    if (!reply.last().startsWith("Ambiguous symbol error") &&
            (reply.length() < 2 || !reply.at(reply.length() - 2).startsWith("Ambiguous symbol error"))) {
        return;
    }
    // *** WARNING: Unable to verify checksum for C:\dev\builds\qt5\qtbase\lib\Qt5Cored.dll
    // *** WARNING: Unable to verify checksum for untitled2.exe", "Matched: untitled2!main+0xc (000007f6`a103241c)
    // Matched: untitled123!main+0x1b6 (000007f6`be2f25c6)
    // Matched: untitled123!<lambda_4956dbaf7bce78acbc6759af75f3884a>::operator() (000007f6`be2f26b0)
    // Matched: untitled123!<lambda_4956dbaf7bce78acbc6759af75f3884a>::<helper_func_cdecl> (000007f6`be2f2730)
    // Matched: untitled123!<lambda_4956dbaf7bce78acbc6759af75f3884a>::operator QString (__cdecl*)(void) (000007f6`be2f27b0)
    // Matched: untitled123!<lambda_4956dbaf7bce78acbc6759af75f3884a>::<helper_func_vectorcall> (000007f6`be2f27d0)
    // Matched: untitled123!<lambda_4956dbaf7bce78acbc6759af75f3884a>::operator QString (__vectorcall*)(void) (000007f6`be2f2850)
    // Ambiguous symbol error at '`untitled2!C:\dev\src\tmp\untitled2\main.cpp:18`'
    //               ^ Extra character error in 'bu1004 `untitled2!C:\dev\src\tmp\untitled2\main.cpp:18`'

    if (!bpId.isValid())
        return;
    Breakpoint bp = breakHandler()->breakpointById(bpId);
    // add break point for every match
    int subBreakPointID = 0;
    for (auto line = reply.constBegin(), end = reply.constEnd(); line != end; ++line) {
        if (!line->startsWith("Matched: "))
            continue;
        const int addressStartPos = line->lastIndexOf('(') + 1;
        const int addressEndPos = line->indexOf(')', addressStartPos);
        if (addressStartPos == 0 || addressEndPos == -1)
            continue;

        QString addressString = line->mid(addressStartPos, addressEndPos - addressStartPos);
        addressString.replace("`", "");
        bool ok = true;
        quint64 address = addressString.toULongLong(&ok, 16);
        if (!ok)
            continue;

        BreakpointModelId id(bpId.majorPart(), ++subBreakPointID);
        BreakpointResponse res = bp.response();
        res.type = BreakpointByAddress;
        res.address = address;
        m_insertSubBreakpointMap.insert(id, res);
    }
    if (subBreakPointID == 0)
        return;

    attemptBreakpointSynchronization();
}

void CdbEngine::handleCheckWow64(const DebuggerResponse &response, const GdbMi &stack)
{
    // Using the lm (list modules) command to check if there is a 32 bit subsystem in this debuggee.
    // expected reply if there is a 32 bit stack:
    // start             end                 module name
    // 00000000`77490000 00000000`774d5000   wow64      (deferred)
    if (response.data.data().contains("wow64")) {
        runCommand({"k", BuiltinCommand,
                    [this, stack](const DebuggerResponse &r) { ensureUsing32BitStackInWow64(r, stack); }});
        return;
    }
    m_wow64State = noWow64Stack;
    parseStackTrace(stack, false);
}

void CdbEngine::ensureUsing32BitStackInWow64(const DebuggerResponse &response, const GdbMi &stack)
{
    // Parsing the header of the stack output to check which bitness
    // the cdb is currently using.
    foreach (const QStringRef &line, response.data.data().splitRef(QLatin1Char('\n'))) {
        if (!line.startsWith("Child"))
            continue;
        if (line.startsWith("ChildEBP")) {
            m_wow64State = wow64Stack32Bit;
            parseStackTrace(stack, false);
            return;
        } else if (line.startsWith("Child-SP")) {
            m_wow64State = wow64Stack64Bit;
            runCommand({"!wow64exts.sw", BuiltinCommand, CB(handleSwitchWow64Stack)});
            return;
        }
    }
    m_wow64State = noWow64Stack;
    parseStackTrace(stack, false);
}

void CdbEngine::handleSwitchWow64Stack(const DebuggerResponse &response)
{
    if (response.data.data() == "Switched to 32bit mode")
        m_wow64State = wow64Stack32Bit;
    else if (response.data.data() == "Switched to 64bit mode")
        m_wow64State = wow64Stack64Bit;
    else
        m_wow64State = noWow64Stack;
    // reload threads and the stack after switching the mode
    runCommand({"threads", ExtensionCommand, CB(handleThreads)});
}

void CdbEngine::handleSessionAccessible(unsigned long cdbExState)
{
    const DebuggerState s = state();
    if (!m_hasDebuggee || s == InferiorRunOk) // suppress reports
        return;

    if (debug)
        qDebug("CdbEngine::handleSessionAccessible %dms in state '%s'/'%s', special mode %d",
               elapsedLogTime(), cdbStatusName(cdbExState),
               qPrintable(stateName(state())), m_specialStopMode);

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
               elapsedLogTime(), cdbStatusName(cdbExState),
               qPrintable(stateName(state())), m_specialStopMode);

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

void CdbEngine::handleSessionIdle(const QString &message)
{
    if (!m_hasDebuggee)
        return;

    if (debug)
        qDebug("CdbEngine::handleSessionIdle %dms '%s' in state '%s', special mode %d",
               elapsedLogTime(), qPrintable(message),
               qPrintable(stateName(state())), m_specialStopMode);

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
        if (runParameters().startMode == AttachCore) {
            m_coreStopReason.reset(new GdbMi);
            m_coreStopReason->fromString(message);
        }
        return;
    }

    GdbMi stopReason;
    stopReason.fromString(message);
    processStop(stopReason, false);
}

void CdbEngine::handleExtensionMessage(char t, int token, const QString &what, const QString &message)
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
            showMessage(message, LogMisc);
            return;
        }
        // Did the command finish? Take off queue and complete, invoke CB
        const DebuggerCommand command = m_commandForToken.take(token);
        if (debug)
            qDebug("### Completed extension command '%s' for token=%d, pending=%d",
                   qPrintable(command.function), token, m_commandForToken.size());

        if (!command.callback) {
            if (!message.isEmpty()) // log unhandled output
                showMessage(message, LogMisc);
            return;
        }
        DebuggerResponse response;
        response.data.m_name = "data";
        if (t == 'R') {
            response.resultClass = ResultDone;
            response.data.fromString(message);
            if (!response.data.isValid()) {
                response.data.m_data = message;
                response.data.m_type = GdbMi::Tuple;
            }
        } else {
            response.resultClass = ResultError;
            GdbMi msg;
            msg.m_name = "msg";
            msg.m_data = message;
            msg.m_type = GdbMi::Tuple;
            response.data.m_type = GdbMi::Tuple;
            response.data.m_children.push_back(msg);
        }
        command.callback(response);
        return;
    }

    if (what == "debuggee_output") {
        const QByteArray decoded = QByteArray::fromHex(message.toUtf8());
        showMessage(QString::fromUtf16(reinterpret_cast<const ushort *>(decoded.data()), decoded.size() / 2),
                    AppOutput);
        return;
    }

    if (what == "event") {
        if (message.startsWith("Process exited"))
            notifyInferiorExited();
        showStatusMessage(message,  5000);
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
        // Don't show the Win32 x86 emulation subsystem breakpoint hit or the
        // set thread names exception.
        if (exception.exceptionCode == winExceptionWX86Breakpoint
                || exception.exceptionCode == winExceptionSetThreadName) {
            return;
        }
        const QString message = exception.toString(true);
        showStatusMessage(message);
        // Report C++ exception in application output as well.
        if (exception.exceptionCode == winExceptionCppException)
            showMessage(message + '\n', AppOutput);
        if (!isDebuggerWinException(exception.exceptionCode)) {
            const Task::TaskType type =
                    isFatalWinException(exception.exceptionCode) ? Task::Error : Task::Warning;
            const FileName fileName = exception.file.isEmpty()
                    ? FileName() : FileName::fromUserInput(exception.file);
            TaskHub::addTask(type, exception.toString(false).trimmed(),
                             Debugger::Constants::TASK_CATEGORY_DEBUGGER_RUNTIME,
                             fileName, exception.lineNumber);
        }
        return;
    }
}

// Check for a CDB prompt '0:000> ' ('process:thread> ')..no regexps for QByteArray...
enum { CdbPromptLength = 7 };

static inline bool isCdbPrompt(const QString &c)
{
    return c.size() >= CdbPromptLength && c.at(6) == ' ' && c.at(5) == '>' && c.at(1) == ':'
            && std::isdigit(c.at(0).unicode())
            && std::isdigit(c.at(2).unicode())
            && std::isdigit(c.at(3).unicode())
            && std::isdigit(c.at(4).unicode());
}

// Check for '<token>32>' or '<token>32<'
static inline bool checkCommandToken(const QString &tokenPrefix, const QString &c,
                                  int *token, bool *isStart)
{
    *token = 0;
    *isStart = false;
    const int tokenPrefixSize = tokenPrefix.size();
    const int size = c.size();
    if (size < tokenPrefixSize + 2 || !std::isdigit(c.at(tokenPrefixSize).unicode()))
        return false;
    switch (c.at(size - 1).unicode()) {
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

void CdbEngine::parseOutputLine(QString line)
{
    // The hooked output callback in the extension suppresses prompts,
    // it should happen only in initial and exit stages. Note however that
    // if the output is not hooked, sequences of prompts are possible which
    // can mix things up.
    while (isCdbPrompt(line))
        line.remove(0, CdbPromptLength);
    // An extension notification (potentially consisting of several chunks)
    static const QString creatorExtPrefix = "<qtcreatorcdbext>|";
    if (line.startsWith(creatorExtPrefix)) {
        // "<qtcreatorcdbext>|type_char|token|remainingChunks|serviceName|message"
        const char type = line.at(creatorExtPrefix.size()).unicode();
        // integer token
        const int tokenPos = creatorExtPrefix.size() + 2;
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
        const QString what = line.mid(whatPos, whatEndPos - whatPos);
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
        qDebug("Reading CDB stdout '%s',\n  isCommand=%d, token=%d, isStart=%d",
               qPrintable(line), isCommandToken, token, isStartToken);

    // If there is a current command, wait for end of output indicated by token,
    // command, trigger handler and finish, else append to its output.
    if (m_currentBuiltinResponseToken != -1) {
        QTC_ASSERT(!isStartToken, return);
        if (isCommandToken) {
            // Did the command finish? Invoke callback and remove from queue.
            const DebuggerCommand &command = m_commandForToken.take(token);
            if (debug)
                qDebug("### Completed builtin command '%s' for token=%d, %d lines, pending=%d",
                       qPrintable(command.function), m_currentBuiltinResponseToken,
                       m_currentBuiltinResponse.count('\n'), m_commandForToken.size() - 1);
            QTC_ASSERT(token == m_currentBuiltinResponseToken, return);
            showMessage(m_currentBuiltinResponse, LogMisc);
            if (command.callback) {
                DebuggerResponse response;
                response.token = token;
                response.data.m_name = "data";
                response.data.m_data = m_currentBuiltinResponse;
                response.data.m_type = GdbMi::Tuple;
                response.resultClass = ResultDone;
                command.callback(response);
            }
            m_currentBuiltinResponseToken = -1;
            m_currentBuiltinResponse.clear();
        } else {
            // Record output of current command
            if (!m_currentBuiltinResponse.isEmpty())
                m_currentBuiltinResponse.push_back('\n');
            m_currentBuiltinResponse.push_back(line);
        }
        return;
    }
    if (isCommandToken) {
        // Beginning command token encountered, start to record output.
        m_currentBuiltinResponseToken = token;
        if (debug)
            qDebug("### Gathering output for token %d", token);
        return;
    }
    const char versionString[] = "Microsoft (R) Windows Debugger Version";
    if (line.startsWith(versionString)) {
        QRegExp versionRegEx("(\\d+)\\.(\\d+)\\.\\d+\\.\\d+");
        if (versionRegEx.indexIn(line) > -1) {
            bool ok = true;
            int major = versionRegEx.cap(1).toInt(&ok);
            int minor = versionRegEx.cap(2).toInt(&ok);
            if (ok) {
                // for some incomprehensible reasons Microsoft cdb version 6.2 is newer than 6.12
                m_autoBreakPointCorrection = major > 6 || (major == 6 && minor >= 2 && minor < 10);
                showMessage(line, LogMisc);
                showMessage(QString::fromLatin1("Using ")
                            + QLatin1String(m_autoBreakPointCorrection ? "CDB " : "codemodel ")
                            + QString::fromLatin1("based breakpoint correction."), LogMisc);
            }
        }
    } else if (line.startsWith("ModLoad: ")) {
        // output(64): ModLoad: 00007ffb`842b0000 00007ffb`843ee000   C:\Windows\system32\KERNEL32.DLL
        // output(32): ModLoad: 00007ffb 00007ffb   C:\Windows\system32\KERNEL32.DLL
        QRegExp moduleRegExp("[0-9a-fA-F]+(`[0-9a-fA-F]+)? [0-9a-fA-F]+(`[0-9a-fA-F]+)? (.*)");
        if (moduleRegExp.indexIn(line) > -1)
            showStatusMessage(tr("Module loaded: ") + moduleRegExp.cap(3).trimmed(), 3000);
    } else {
        showMessage(line, LogMisc);
    }
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
            parseOutputLine(QString::fromLocal8Bit(line));
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

bool CdbEngine::acceptsBreakpoint(Breakpoint bp) const
{
    if (bp.parameters().isCppBreakpoint()) {
        switch (bp.type()) {
        case UnknownBreakpointType:
        case LastBreakpointType:
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
    return isNativeMixedEnabled();
}

// Context for fixing file/line-type breakpoints, for delayed creation.
class BreakpointCorrectionContext
{
public:
    explicit BreakpointCorrectionContext(const CPlusPlus::Snapshot &s,
                                         const CppTools::WorkingCopy &workingCopy) :
        m_snapshot(s), m_workingCopy(workingCopy) {}

    unsigned fixLineNumber(const QString &fileName, unsigned lineNumber) const;

private:
    const CPlusPlus::Snapshot m_snapshot;
    CppTools::WorkingCopy m_workingCopy;
};

static CPlusPlus::Document::Ptr getParsedDocument(const QString &fileName,
                                                  const CppTools::WorkingCopy &workingCopy,
                                                  const CPlusPlus::Snapshot &snapshot)
{
    QByteArray src;
    if (workingCopy.contains(fileName)) {
        src = workingCopy.source(fileName);
    } else {
        FileReader reader;
        if (reader.fetch(fileName)) // ### FIXME error reporting
            src = QString::fromLocal8Bit(reader.data()).toUtf8();
    }

    CPlusPlus::Document::Ptr doc = snapshot.preprocessedDocument(src, fileName);
    doc->parse();
    return doc;
}

unsigned BreakpointCorrectionContext::fixLineNumber(const QString &fileName,
                                                    unsigned lineNumber) const
{
    const CPlusPlus::Document::Ptr doc = getParsedDocument(fileName, m_workingCopy, m_snapshot);
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
        qDebug("attemptBreakpointSynchronization in %s", qPrintable(stateName(state())));
    // Check if there is anything to be done at all.
    BreakHandler *handler = breakHandler();
    // Take ownership of the breakpoint. Requests insertion. TODO: Cpp only?
    foreach (Breakpoint bp, handler->unclaimedBreakpoints())
        if (acceptsBreakpoint(bp))
            bp.setEngine(this);

    // Quick check: is there a need to change something? - Populate module cache
    bool changed = !m_insertSubBreakpointMap.isEmpty();
    const Breakpoints bps = handler->engineBreakpoints(this);
    if (!changed) {
        foreach (Breakpoint bp, bps) {
            switch (bp.state()) {
            case BreakpointInsertRequested:
            case BreakpointRemoveRequested:
            case BreakpointChangeRequested:
                changed = true;
                break;
            case BreakpointInserted: {
                // Collect the new modules matching the files.
                // In the future, that information should be obtained from the build system.
                const BreakpointParameters &data = bp.parameters();
                if (data.type == BreakpointByFileAndLine && !data.module.isEmpty())
                    m_fileNameModuleHash.insert(data.fileName, data.module);
            }
                break;
            default:
                break;
            }
        }
    }

    if (debugBreakpoints)
        qDebug("attemptBreakpointSynchronizationI %dms accessible=%d, %s %d breakpoints, changed=%d",
               elapsedLogTime(), m_accessible, qPrintable(stateName(state())), bps.size(), changed);
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
    foreach (Breakpoint bp, bps) {
        BreakpointParameters parameters = bp.parameters();
        BreakpointModelId id = bp.id();
        const auto handleBreakInsertCB = [this, id](const DebuggerResponse &r) { handleBreakInsert(r, id); };
        BreakpointResponse response;
        response.fromParameters(parameters);
        response.id = BreakpointResponseId(id.majorPart(), id.minorPart());
        // If we encountered that file and have a module for it: Add it.
        if (parameters.type == BreakpointByFileAndLine && parameters.module.isEmpty()) {
            const QHash<QString, QString>::const_iterator it = m_fileNameModuleHash.constFind(parameters.fileName);
            if (it != m_fileNameModuleHash.constEnd())
                parameters.module = it.value();
        }
        switch (bp.state()) {
        case BreakpointInsertRequested:
            if (!m_autoBreakPointCorrection
                    && parameters.type == BreakpointByFileAndLine
                    && boolSetting(CdbBreakPointCorrection)) {
                if (lineCorrection.isNull())
                    lineCorrection.reset(new BreakpointCorrectionContext(Internal::cppCodeModelSnapshot(),
                                                                         CppTools::CppModelManager::instance()->workingCopy()));
                response.lineNumber = lineCorrection->fixLineNumber(parameters.fileName, parameters.lineNumber);
                QString cmd = cdbAddBreakpointCommand(response, m_sourcePathMappings, id, false);
                runCommand({cmd, BuiltinCommand, handleBreakInsertCB});
            } else {
                QString cmd = cdbAddBreakpointCommand(parameters, m_sourcePathMappings, id, false);
                runCommand({cmd, BuiltinCommand, handleBreakInsertCB});
            }
            if (!parameters.enabled)
                runCommand({"bd " + QString::number(breakPointIdToCdbId(id)), NoFlags});
            bp.notifyBreakpointInsertProceeding();
            bp.notifyBreakpointInsertOk();
            m_pendingBreakpointMap.insert(id, response);
            addedChanged = true;
            // Ensure enabled/disabled is correct in handler and line number is there.
            bp.setResponse(response);
            if (debugBreakpoints)
                qDebug("Adding %d %s\n", id.toInternalId(),
                    qPrintable(response.toString()));
            break;
        case BreakpointChangeRequested:
            bp.notifyBreakpointChangeProceeding();
            if (debugBreakpoints)
                qDebug("Changing %d:\n    %s\nTo %s\n", id.toInternalId(),
                    qPrintable(bp.response().toString()),
                    qPrintable(parameters.toString()));
            if (parameters.enabled != bp.response().enabled) {
                // Change enabled/disabled breakpoints without triggering update.
                if (parameters.enabled)
                    runCommand({"be " + QString::number(breakPointIdToCdbId(id)), NoFlags});
                else
                    runCommand({"bd " + QString::number(breakPointIdToCdbId(id)), NoFlags});
                response.pending = false;
                response.enabled = parameters.enabled;
                bp.setResponse(response);
            } else {
                // Delete and re-add, triggering update
                addedChanged = true;
                runCommand({cdbClearBreakpointCommand(id), NoFlags});
                QString cmd(cdbAddBreakpointCommand(parameters, m_sourcePathMappings, id, false));
                runCommand({cmd, BuiltinCommand, handleBreakInsertCB});
                m_pendingBreakpointMap.insert(id, response);
            }
            bp.notifyBreakpointChangeOk();
            break;
        case BreakpointRemoveRequested:
            runCommand({cdbClearBreakpointCommand(id), NoFlags});
            bp.notifyBreakpointRemoveProceeding();
            bp.notifyBreakpointRemoveOk();
            m_pendingBreakpointMap.remove(id);
            break;
        default:
            break;
        }
    }
    foreach (BreakpointModelId id, m_insertSubBreakpointMap.keys()) {
        addedChanged = true;
        const BreakpointResponse &response = m_insertSubBreakpointMap.value(id);
        runCommand({cdbAddBreakpointCommand(response, m_sourcePathMappings, id, false), NoFlags});
        m_insertSubBreakpointMap.remove(id);
        m_pendingSubBreakpointMap.insert(id, response);
    }
    // List breakpoints and send responses
    if (addedChanged)
        listBreakpoints();
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
    const QString normalized = FileUtils::normalizePathName(fileName);
    if (debugSourceMapping)
        qDebug(" sourceMapNormalizeFileNameFromDebugger %s->%s", qPrintable(fileName), qPrintable(normalized));
    // Check if it really exists, that is normalize worked and QFileInfo confirms it.
    const bool exists = !normalized.isEmpty() && QFileInfo(normalized).isFile();
    NormalizedSourceFileName result(QDir::cleanPath(normalized.isEmpty() ? fileName : normalized), exists);
    if (!exists) {
        // At least upper case drive letter if failed.
        if (result.fileName.size() > 2 && result.fileName.at(1) == ':')
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
        frame.level = QString::number(i);
        const GdbMi fullName = frameMi["fullname"];
        if (fullName.isValid()) {
            frame.file = fullName.data();
            frame.line = frameMi["line"].data().toInt();
            frame.usable = false; // To be decided after source path mapping.
            const GdbMi languageMi = frameMi["language"];
            if (languageMi.isValid() && languageMi.data() == "js")
                frame.language = QmlLanguage;
        }
        frame.function = frameMi["function"].data();
        frame.module = frameMi["from"].data();
        frame.context = frameMi["context"].data();
        frame.address = frameMi["address"].data().toULongLong(0, 16);
        rc.push_back(frame);
    }
    return rc;
}

unsigned CdbEngine::parseStackTrace(const GdbMi &data, bool sourceStepInto)
{
    // Parse frames, find current. Special handling for step into:
    // When stepping into on an actual function (source mode) by executing 't', an assembler
    // frame pointing at the jmp instruction is hit (noticeable by top function being
    // 'ILT+'). If that is the case, execute another 't' to step into the actual function.
    // Note that executing 't 2' does not work since it steps 2 instructions on a non-call code line.
    // If the operate by instruction flag is set, always use the first frame.
    int current = -1;
    bool incomplete;
    StackFrames frames = parseFrames(data, &incomplete);
    const int count = frames.size();
    for (int i = 0; i < count; i++) {
        if (m_wow64State == wow64Uninitialized) {
            showMessage("Checking for wow64 subsystem...", LogMisc);
            return ParseStackWow64;
        }
        const bool hasFile = !frames.at(i).file.isEmpty();
        // jmp-frame hit by step into, do another 't' and abort sequence.
        if (!hasFile && i == 0 && sourceStepInto) {
            if (frames.at(i).function.contains("ILT+")) {
                showMessage("Step into: Call instruction hit, performing additional step...", LogMisc);
                return ParseStackStepInto;
            }
            showMessage("Step into: Hit frame with no source, step out...", LogMisc);
            return ParseStackStepOut;
        }
        if (hasFile) {
            const NormalizedSourceFileName fileName = sourceMapNormalizeFileNameFromDebugger(frames.at(i).file);
            if (!fileName.exists && i == 0 && sourceStepInto) {
                showMessage("Step into: Hit frame with no source, step out...", LogMisc);
                return ParseStackStepOut;
            }
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

void CdbEngine::loadAdditionalQmlStack()
{
    runCommand({"qmlstack", ExtensionCommand, CB(handleAdditionalQmlStack)});
}

void CdbEngine::handleAdditionalQmlStack(const DebuggerResponse &response)
{
    QString errorMessage;
    do {
        if (response.resultClass != ResultDone) {
            errorMessage = response.data["msg"].data();
            break;
        }
        if (!response.data.isValid()) {
            errorMessage = "GDBMI parser error";
            break;
        }
        StackFrames qmlFrames = parseFrames(response.data);
        const int qmlFrameCount = qmlFrames.size();
        if (!qmlFrameCount) {
            errorMessage = "Empty stack";
            break;
        }
        for (int i = 0; i < qmlFrameCount; ++i)
            qmlFrames[i].fixQrcFrame(runParameters());
        stackHandler()->prependFrames(qmlFrames);
    } while (false);
    if (!errorMessage.isEmpty())
        showMessage("Unable to obtain QML stack trace: " + errorMessage, LogError);
}

void CdbEngine::setupScripting(const DebuggerResponse &response)
{
    GdbMi data = response.data;
    if (response.resultClass != ResultDone) {
        showMessage(data["msg"].data(), LogMisc);
        return;
    }
    const QString &verOutput = data.data();
    const QString firstToken = verOutput.split(QLatin1Char(' ')).constFirst();
    const QVector<QStringRef> pythonVersion =firstToken.splitRef(QLatin1Char('.'));

    bool ok = false;
    if (pythonVersion.size() == 3) {
        m_pythonVersion |= pythonVersion[0].toInt(&ok);
        if (ok) {
            m_pythonVersion = m_pythonVersion << 8;
            m_pythonVersion |= pythonVersion[1].toInt(&ok);
            if (ok) {
                m_pythonVersion = m_pythonVersion << 8;
                m_pythonVersion |= pythonVersion[2].toInt(&ok);
            }
        }
    }
    if (!ok) {
        m_pythonVersion = 0;
        showMessage(QString("Can not parse sys.version:\n%1").arg(verOutput), LogWarning);
        return;
    }

    QString dumperPath = QDir::toNativeSeparators(Core::ICore::resourcePath() + "/debugger");
    dumperPath.replace('\\', "\\\\");
    runCommand({"sys.path.insert(1, '" + dumperPath + "')", ScriptCommand});
    runCommand({"from cdbbridge import Dumper", ScriptCommand});
    runCommand({"print(dir())", ScriptCommand});
    runCommand({"theDumper = Dumper()", ScriptCommand});
    runCommand({"theDumper.loadDumpers(None)", ScriptCommand,
                [this](const DebuggerResponse &response) {
                    watchHandler()->addDumpers(response.data["dumpers"]);
    }});
}

void CdbEngine::mergeStartParametersSourcePathMap()
{
    const DebuggerRunParameters &rp = runParameters();
    QMap<QString, QString>::const_iterator end = rp.sourcePathMap.end();
    for (QMap<QString, QString>::const_iterator it = rp.sourcePathMap.begin(); it != end; ++it) {
        SourcePathMapping spm(QDir::toNativeSeparators(it.key()), QDir::toNativeSeparators(it.value()));
        if (!m_sourcePathMappings.contains(spm))
            m_sourcePathMappings.push_back(spm);
    }
}

void CdbEngine::handleStackTrace(const DebuggerResponse &response)
{
    GdbMi stack = response.data;
    if (response.resultClass == ResultDone) {
        if (parseStackTrace(stack, false) == ParseStackWow64) {
            runCommand({"lm m wow64", BuiltinCommand,
                       [this, stack](const DebuggerResponse &r) { handleCheckWow64(r, stack); }});
        }
    } else {
        showMessage(stack["msg"].data(), LogError);
    }
}

void CdbEngine::handleExpression(const DebuggerResponse &response, BreakpointModelId id, const GdbMi &stopReason)
{
    int value = 0;
    if (response.resultClass == ResultDone)
        value = response.data.toInt();
    else
        showMessage(response.data["msg"].data(), LogError);
    // Is this a conditional breakpoint?
    const QString message = value ?
        tr("Value %1 obtained from evaluating the condition of breakpoint %2, stopping.").
        arg(value).arg(id.toString()) :
        tr("Value 0 obtained from evaluating the condition of breakpoint %1, continuing.").
        arg(id.toString());
    showMessage(message, LogMisc);
    // Stop if evaluation is true, else continue
    if (value)
        processStop(stopReason, true);
    else
        doContinueInferior();
}

void CdbEngine::handleWidgetAt(const DebuggerResponse &response)
{
    bool success = false;
    QString message;
    do {
        if (response.resultClass != ResultDone) {
            message = response.data["msg"].data();
            break;
        }
        // Should be "namespace::QWidget:0x555"
        QString watchExp = response.data.data();
        const int sepPos = watchExp.lastIndexOf(':');
        if (sepPos == -1) {
            message = QString("Invalid output: %1").arg(watchExp);
            break;
        }
        // 0x000 -> nothing found
        if (!watchExp.mid(sepPos + 1).toULongLong(0, 0)) {
            message = QString("No widget could be found at %1, %2.").arg(m_watchPointX).arg(m_watchPointY);
            break;
        }
        // Turn into watch expression: "*(namespace::QWidget*)0x555"
        watchExp.replace(sepPos, 1, "*)");
        watchExp.insert(0, "*(");
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

void CdbEngine::handleBreakPoints(const DebuggerResponse &response)
{
    if (debugBreakpoints) {
        qDebug("CdbEngine::handleBreakPoints: success=%d: %s",
               response.resultClass == ResultDone, qPrintable(response.data.toString()));
    }
    if (response.resultClass != ResultDone) {
        showMessage(response.data["msg"].data(), LogError);
        return;
    }
    if (response.data.type() != GdbMi::List) {
        showMessage("Unable to parse breakpoints reply", LogError);
        return;
    }

    // Report all obtained parameters back. Note that not all parameters are reported
    // back, so, match by id and complete
    if (debugBreakpoints)
        qDebug("\nCdbEngine::handleBreakPoints with %d", response.data.childCount());
    QString message;
    QTextStream str(&message);
    BreakHandler *handler = breakHandler();
    foreach (const GdbMi &breakPointG, response.data.children()) {
        BreakpointResponse reportedResponse;
        parseBreakPoint(breakPointG, &reportedResponse);
        if (debugBreakpoints)
            qDebug("  Parsed %d: pending=%d %s\n", reportedResponse.id.majorPart(),
                reportedResponse.pending,
                qPrintable(reportedResponse.toString()));
        if (reportedResponse.id.isValid() && !reportedResponse.pending) {
            Breakpoint bp = handler->findBreakpointByResponseId(reportedResponse.id);
            if (!bp && reportedResponse.type == BreakpointByFunction)
                continue; // Breakpoints from options, CrtDbgReport() and others.
            QTC_ASSERT(bp, continue);
            const auto it = m_pendingBreakpointMap.find(bp.id());
            const auto subIt = m_pendingSubBreakpointMap.find(
                        BreakpointModelId(reportedResponse.id.majorPart(),
                                          reportedResponse.id.minorPart()));
            if (it != m_pendingBreakpointMap.end() || subIt != m_pendingSubBreakpointMap.end()) {
                // Complete the response and set on handler.
                BreakpointResponse currentResponse = it != m_pendingBreakpointMap.end()
                        ? it.value()
                        : subIt.value();
                currentResponse.id = reportedResponse.id;
                currentResponse.address = reportedResponse.address;
                currentResponse.module = reportedResponse.module;
                currentResponse.pending = reportedResponse.pending;
                currentResponse.enabled = reportedResponse.enabled;
                currentResponse.fileName = reportedResponse.fileName;
                currentResponse.lineNumber = reportedResponse.lineNumber;
                formatCdbBreakPointResponse(bp.id(), currentResponse, str);
                if (debugBreakpoints)
                    qDebug("  Setting for %d: %s\n", currentResponse.id.majorPart(),
                           qPrintable(currentResponse.toString()));
                if (it != m_pendingBreakpointMap.end()) {
                    bp.setResponse(currentResponse);
                    m_pendingBreakpointMap.erase(it);
                }
                if (subIt != m_pendingSubBreakpointMap.end()) {
                    bp.insertSubBreakpoint(currentResponse);
                    m_pendingSubBreakpointMap.erase(subIt);
                }
            }
        } // not pending reported
    } // foreach
    if (m_pendingBreakpointMap.empty())
        str << "All breakpoints have been resolved.\n";
    else
        str << QString("%1 breakpoint(s) pending...\n").arg(m_pendingBreakpointMap.size());
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
        showMessage(tr("\"Select Widget to Watch\": Not supported in state \"%1\".").
                    arg(stateName(state())), LogWarning);
        break;
    }
}

void CdbEngine::postWidgetAtCommand()
{
    DebuggerCommand cmd("widgetat", ExtensionCommand);
    cmd.args = QString("%1 %2").arg(m_watchPointX, m_watchPointY);
    runCommand(cmd);
}

void CdbEngine::handleCustomSpecialStop(const QVariant &v)
{
    if (v.canConvert<MemoryChangeCookie>()) {
        const MemoryChangeCookie changeData = qvariant_cast<MemoryChangeCookie>(v);
        runCommand({cdbWriteMemoryCommand(changeData.address, changeData.data), NoFlags});
        return;
    }
    if (v.canConvert<MemoryViewCookie>()) {
        postFetchMemory(qvariant_cast<MemoryViewCookie>(v));
        return;
    }
}

} // namespace Internal
} // namespace Debugger

Q_DECLARE_METATYPE(Debugger::Internal::GdbMi)
