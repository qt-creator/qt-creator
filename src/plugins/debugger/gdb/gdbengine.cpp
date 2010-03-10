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

#define QT_NO_CAST_FROM_ASCII

#include "gdbengine.h"
#include "gdboptionspage.h"
#include "trkoptions.h"
#include "trkoptionspage.h"
#include "debugger/debuggeruiswitcher.h"
#include "debugger/debuggermainwindow.h"

#include "attachgdbadapter.h"
#include "coregdbadapter.h"
#include "plaingdbadapter.h"
#include "termgdbadapter.h"
#include "remotegdbadapter.h"
#include "trkgdbadapter.h"

#include "watchutils.h"
#include "debuggeractions.h"
#include "debuggeragents.h"
#include "debuggerconstants.h"
#include "debuggermanager.h"
#include "debuggertooltip.h"
#include "debuggerstringutils.h"
#include "gdbmi.h"

#include "breakhandler.h"
#include "moduleshandler.h"
#include "registerhandler.h"
#include "snapshothandler.h"
#include "stackhandler.h"
#include "watchhandler.h"

#include "sourcefileswindow.h"

#include "debuggerdialogs.h"

#include <utils/qtcassert.h>
#include <utils/fancymainwindow.h>
#include <texteditor/itexteditor.h>
#include <projectexplorer/toolchain.h>
#include <coreplugin/icore.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QMetaObject>
#include <QtCore/QTime>
#include <QtCore/QTimer>
#include <QtCore/QTemporaryFile>
#include <QtCore/QTextStream>

#include <QtGui/QAction>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QLabel>
#include <QtGui/QMainWindow>
#include <QtGui/QMessageBox>
#include <QtGui/QPushButton>

#ifdef Q_OS_UNIX
#include <unistd.h>
#include <dlfcn.h>
#endif
#include <ctype.h>


namespace Debugger {
namespace Internal {

//#define DEBUG_PENDING  1

static const char winPythonVersionC[] = "python2.5";

#if DEBUG_PENDING
#   define PENDING_DEBUG(s) qDebug() << s
#else
#   define PENDING_DEBUG(s)
#endif
#define PENDING_DEBUGX(s) qDebug() << s

#define CB(callback) &GdbEngine::callback, STRINGIFY(callback)

QByteArray GdbEngine::tooltipINameForExpression(const QByteArray &exp)
{
    // FIXME: 'exp' can contain illegal characters
    //return "tooltip." + exp;
    Q_UNUSED(exp)
    return "tooltip.x";
}

static bool stateAcceptsGdbCommands(DebuggerState state)
{
    switch (state) {
    case AdapterStarting:
    case AdapterStarted:
    case AdapterStartFailed:
    case InferiorUnrunnable:
    case InferiorStarting:
    case InferiorStartFailed:
    case InferiorRunningRequested:
    case InferiorRunningRequested_Kill:
    case InferiorRunning:
    case InferiorStopping:
    case InferiorStopping_Kill:
    case InferiorStopped:
    case InferiorShuttingDown:
    case InferiorShutDown:
    case InferiorShutdownFailed:
        return true;
    case DebuggerNotReady:
    case EngineStarting:
    case InferiorStopFailed:
    case EngineShuttingDown:
        break;
    }
    return false;
}

static int &currentToken()
{
    static int token = 0;
    return token;
}

static QByteArray parsePlainConsoleStream(const GdbResponse &response)
{
    GdbMi output = response.data.findChild("consolestreamoutput");
    QByteArray out = output.data();
    // FIXME: proper decoding needed
    if (out.endsWith("\\n"))
        out.chop(2);
    while (out.endsWith('\n') || out.endsWith(' '))
        out.chop(1);
    int pos = out.indexOf(" = ");
    return out.mid(pos + 3);
}

///////////////////////////////////////////////////////////////////////
//
// GdbEngine
//
///////////////////////////////////////////////////////////////////////

GdbEngine::GdbEngine(DebuggerManager *manager) : IDebuggerEngine(manager)
{
    m_trkOptions = QSharedPointer<TrkOptions>(new TrkOptions);
    m_trkOptions->fromSettings(Core::ICore::instance()->settings());
    m_gdbAdapter = 0;

    m_commandTimer = new QTimer(this);
    m_commandTimer->setSingleShot(true);
    connect(m_commandTimer, SIGNAL(timeout()), SLOT(commandTimeout()));

    // Needs no resetting in initializeVariables()
    m_busy = false;

    connect(theDebuggerAction(AutoDerefPointers), SIGNAL(valueChanged(QVariant)),
            this, SLOT(setAutoDerefPointers(QVariant)));
}

void GdbEngine::connectDebuggingHelperActions()
{
    connect(theDebuggerAction(UseDebuggingHelpers), SIGNAL(valueChanged(QVariant)),
            this, SLOT(setUseDebuggingHelpers(QVariant)));
    connect(theDebuggerAction(DebugDebuggingHelpers), SIGNAL(valueChanged(QVariant)),
            this, SLOT(setDebugDebuggingHelpersClassic(QVariant)));
    connect(theDebuggerAction(RecheckDebuggingHelpers), SIGNAL(triggered()),
            this, SLOT(recheckDebuggingHelperAvailabilityClassic()));
}

void GdbEngine::disconnectDebuggingHelperActions()
{
    disconnect(theDebuggerAction(UseDebuggingHelpers), 0, this, 0);
    disconnect(theDebuggerAction(DebugDebuggingHelpers), 0, this, 0);
    disconnect(theDebuggerAction(RecheckDebuggingHelpers), 0, this, 0);
}

DebuggerStartMode GdbEngine::startMode() const
{
    QTC_ASSERT(!m_startParameters.isNull(), return NoStartMode);
    return m_startParameters->startMode;
}

QMainWindow *GdbEngine::mainWindow() const
{
    return DebuggerUISwitcher::instance()->mainWindow();
}

GdbEngine::~GdbEngine()
{
    // prevent sending error messages afterwards
    disconnect(&m_gdbProc, 0, this, 0);
    delete m_gdbAdapter;
    m_gdbAdapter = 0;
}

void GdbEngine::connectAdapter()
{
    connect(m_gdbAdapter, SIGNAL(adapterStarted()),
        this, SLOT(handleAdapterStarted()));
    connect(m_gdbAdapter, SIGNAL(adapterStartFailed(QString,QString)),
        this, SLOT(handleAdapterStartFailed(QString,QString)));

    connect(m_gdbAdapter, SIGNAL(inferiorPrepared()),
        this, SLOT(handleInferiorPrepared()));

    connect(m_gdbAdapter, SIGNAL(inferiorStartFailed(QString)),
        this, SLOT(handleInferiorStartFailed(QString)));

    connect(m_gdbAdapter, SIGNAL(adapterCrashed(QString)),
        this, SLOT(handleAdapterCrashed(QString)));
}

void GdbEngine::initializeVariables()
{
    m_debuggingHelperState = DebuggingHelperUninitialized;
    m_gdbVersion = 100;
    m_gdbBuildVersion = -1;
    m_isMacGdb = false;
    m_hasPython = false;
    m_registerNamesListed = false;

    m_fullToShortName.clear();
    m_shortToFullName.clear();
    m_varToType.clear();

    invalidateSourcesList();
    m_sourcesListUpdating = false;
    m_oldestAcceptableToken = -1;
    m_outputCodec = QTextCodec::codecForLocale();
    m_pendingWatchRequests = 0;
    m_pendingBreakpointRequests = 0;
    m_commandsDoneCallback = 0;
    m_commandsToRunOnTemporaryBreak.clear();
    m_cookieForToken.clear();

    m_pendingConsoleStreamOutput.clear();
    m_pendingLogStreamOutput.clear();

    m_inbuffer.clear();

    m_commandTimer->stop();

    // ConverterState has no reset() function.
    m_outputCodecState.~ConverterState();
    new (&m_outputCodecState) QTextCodec::ConverterState();

    m_currentFunctionArgs.clear();
    m_currentFrame.clear();
    m_dumperHelper.clear();
#ifdef Q_OS_LINUX
    m_entryPoint.clear();
#endif
}

QString GdbEngine::errorMessage(QProcess::ProcessError error)
{
    switch (error) {
        case QProcess::FailedToStart:
            return tr("The Gdb process failed to start. Either the "
                "invoked program '%1' is missing, or you may have insufficient "
                "permissions to invoke the program.")
                .arg(theDebuggerStringSetting(GdbLocation));
        case QProcess::Crashed:
            return tr("The Gdb process crashed some time after starting "
                "successfully.");
        case QProcess::Timedout:
            return tr("The last waitFor...() function timed out. "
                "The state of QProcess is unchanged, and you can try calling "
                "waitFor...() again.");
        case QProcess::WriteError:
            return tr("An error occurred when attempting to write "
                "to the Gdb process. For example, the process may not be running, "
                "or it may have closed its input channel.");
        case QProcess::ReadError:
            return tr("An error occurred when attempting to read from "
                "the Gdb process. For example, the process may not be running.");
        default:
            return tr("An unknown error in the Gdb process occurred. ");
    }
}

#if 0
static void dump(const char *first, const char *middle, const QString & to)
{
    QByteArray ba(first, middle - first);
    Q_UNUSED(to)
    // note that qDebug cuts off output after a certain size... (bug?)
    qDebug("\n>>>>> %s\n%s\n====\n%s\n<<<<<\n",
        qPrintable(currentTime()),
        qPrintable(QString(ba).trimmed()),
        qPrintable(to.trimmed()));
    //qDebug() << "";
    //qDebug() << qPrintable(currentTime())
    //    << " Reading response:  " << QString(ba).trimmed() << "\n";
}
#endif

void GdbEngine::readDebugeeOutput(const QByteArray &data)
{
    m_manager->showApplicationOutput(m_outputCodec->toUnicode(
            data.constData(), data.length(), &m_outputCodecState));
}

void GdbEngine::debugMessage(const QString &msg)
{
    showDebuggerOutput(LogDebug, msg);
}

void GdbEngine::handleResponse(const QByteArray &buff)
{
    static QTime lastTime;

    if (theDebuggerBoolSetting(LogTimeStamps))
        showDebuggerOutput(LogTime, currentTime());
    showDebuggerOutput(LogOutput, QString::fromLocal8Bit(buff, buff.length()));

#if 0
    qDebug() // << "#### start response handling #### "
        << currentTime()
        << lastTime.msecsTo(QTime::currentTime()) << "ms,"
        << "buf:" << buff.left(1500) << "..."
        //<< "buf:" << buff
        << "size:" << buff.size();
#else
    //qDebug() << "buf:" << buff;
#endif

    lastTime = QTime::currentTime();

    if (buff.isEmpty() || buff == "(gdb) ")
        return;

    const char *from = buff.constData();
    const char *to = from + buff.size();
    const char *inner;

    int token = -1;
    // token is a sequence of numbers
    for (inner = from; inner != to; ++inner)
        if (*inner < '0' || *inner > '9')
            break;
    if (from != inner) {
        token = QByteArray(from, inner - from).toInt();
        from = inner;
        //qDebug() << "found token" << token;
    }

    // next char decides kind of response
    const char c = *from++;
    //qDebug() << "CODE:" << c;
    switch (c) {
        case '*':
        case '+':
        case '=': {
            QByteArray asyncClass;
            for (; from != to; ++from) {
                const char c = *from;
                if (!isNameChar(c))
                    break;
                asyncClass += *from;
            }
            //qDebug() << "ASYNCCLASS" << asyncClass;

            GdbMi result;
            while (from != to) {
                GdbMi data;
                if (*from != ',') {
                    // happens on archer where we get
                    // 23^running <NL> *running,thread-id="all" <NL> (gdb)
                    result.m_type = GdbMi::Tuple;
                    break;
                }
                ++from; // skip ','
                data.parseResultOrValue(from, to);
                if (data.isValid()) {
                    //qDebug() << "parsed result:" << data.toString();
                    result.m_children += data;
                    result.m_type = GdbMi::Tuple;
                }
            }
            if (asyncClass == "stopped") {
                handleStopResponse(result);
                m_pendingLogStreamOutput.clear();
                m_pendingConsoleStreamOutput.clear();
            } else if (asyncClass == "running") {
                // Archer has 'thread-id="all"' here
            } else if (asyncClass == "library-loaded") {
                // Archer has 'id="/usr/lib/libdrm.so.2",
                // target-name="/usr/lib/libdrm.so.2",
                // host-name="/usr/lib/libdrm.so.2",
                // symbols-loaded="0"
                QByteArray id = result.findChild("id").data();
                if (!id.isEmpty())
                    showStatusMessage(tr("Library %1 loaded.").arg(_(id)), 1000);
                invalidateSourcesList();
            } else if (asyncClass == "library-unloaded") {
                // Archer has 'id="/usr/lib/libdrm.so.2",
                // target-name="/usr/lib/libdrm.so.2",
                // host-name="/usr/lib/libdrm.so.2"
                QByteArray id = result.findChild("id").data();
                showStatusMessage(tr("Library %1 unloaded.").arg(_(id)), 1000);
                invalidateSourcesList();
            } else if (asyncClass == "thread-group-created") {
                // Archer has "{id="28902"}"
                QByteArray id = result.findChild("id").data();
                showStatusMessage(tr("Thread group %1 created.").arg(_(id)), 1000);
                int pid = id.toInt();
                if (pid != inferiorPid())
                    handleInferiorPidChanged(pid);
            } else if (asyncClass == "thread-created") {
                //"{id="1",group-id="28902"}"
                QByteArray id = result.findChild("id").data();
                showStatusMessage(tr("Thread %1 created.").arg(_(id)), 1000);
            } else if (asyncClass == "thread-group-exited") {
                // Archer has "{id="28902"}"
                QByteArray id = result.findChild("id").data();
                showStatusMessage(tr("Thread group %1 exited.").arg(_(id)), 1000);
            } else if (asyncClass == "thread-exited") {
                //"{id="1",group-id="28902"}"
                QByteArray id = result.findChild("id").data();
                QByteArray groupid = result.findChild("group-id").data();
                showStatusMessage(tr("Thread %1 in group %2 exited.")
                    .arg(_(id)).arg(_(groupid)), 1000);
            } else if (asyncClass == "thread-selected") {
                QByteArray id = result.findChild("id").data();
                showStatusMessage(tr("Thread %1 selected.").arg(_(id)), 1000);
                //"{id="2"}"
            #if defined(Q_OS_MAC)
            } else if (asyncClass == "shlibs-updated") {
                // MAC announces updated libs
                invalidateSourcesList();
            } else if (asyncClass == "shlibs-added") {
                // MAC announces added libs
                // {shlib-info={num="2", name="libmathCommon.A_debug.dylib",
                // kind="-", dyld-addr="0x7f000", reason="dyld", requested-state="Y",
                // state="Y", path="/usr/lib/system/libmathCommon.A_debug.dylib",
                // description="/usr/lib/system/libmathCommon.A_debug.dylib",
                // loaded_addr="0x7f000", slide="0x7f000", prefix=""}}
                invalidateSourcesList();
            #endif
            } else {
                qDebug() << "IGNORED ASYNC OUTPUT"
                    << asyncClass << result.toString();
            }
            break;
        }

        case '~': {
            QByteArray data = GdbMi::parseCString(from, to);
            m_pendingConsoleStreamOutput += data;

            // Parse pid from noise.
            if (!inferiorPid()) {
                // Linux/Mac gdb: [New [Tt]hread 0x545 (LWP 4554)]
                static QRegExp re1(_("New .hread 0x[0-9a-f]+ \\(LWP ([0-9]*)\\)"));
                // MinGW 6.8: [New thread 2437.0x435345]
                static QRegExp re2(_("New .hread ([0-9]+)\\.0x[0-9a-f]*"));
                // Mac: [Switching to process 9294 local thread 0x2e03] or
                // [Switching to process 31773]
                static QRegExp re3(_("Switching to process ([0-9]+)"));
                QTC_ASSERT(re1.isValid() && re2.isValid(), return);
                if (re1.indexIn(_(data)) != -1)
                    maybeHandleInferiorPidChanged(re1.cap(1));
                else if (re2.indexIn(_(data)) != -1)
                    maybeHandleInferiorPidChanged(re2.cap(1));
                else if (re3.indexIn(_(data)) != -1)
                    maybeHandleInferiorPidChanged(re3.cap(1));
            }

            // Show some messages to give the impression something happens.
            if (data.startsWith("Reading symbols from ")) {
                showStatusMessage(tr("Reading %1...").arg(_(data.mid(21))), 1000);
                invalidateSourcesList();
            } else if (data.startsWith("[New ") || data.startsWith("[Thread ")) {
                if (data.endsWith('\n'))
                    data.chop(1);
                showStatusMessage(_(data), 1000);
            }
            break;
        }

        case '@': {
            readDebugeeOutput(GdbMi::parseCString(from, to));
            break;
        }

        case '&': {
            QByteArray data = GdbMi::parseCString(from, to);
            m_pendingLogStreamOutput += data;
            // On Windows, the contents seem to depend on the debugger
            // version and/or OS version used.
            if (data.startsWith("warning:"))
                manager()->showApplicationOutput(_(data.mid(9))); // cut "warning: "
            break;
        }

        case '^': {
            GdbResponse response;

            response.token = token;

            for (inner = from; inner != to; ++inner)
                if (*inner < 'a' || *inner > 'z')
                    break;

            QByteArray resultClass = QByteArray::fromRawData(from, inner - from);
            if (resultClass == "done") {
                response.resultClass = GdbResultDone;
            } else if (resultClass == "running") {
                if (state() == InferiorStopped) { // Result of manual command.
                    m_manager->resetLocation();
                    setTokenBarrier();
                    setState(InferiorRunningRequested);
                }
                setState(InferiorRunning);
                showStatusMessage(tr("Running..."));
                response.resultClass = GdbResultRunning;
            } else if (resultClass == "connected") {
                response.resultClass = GdbResultConnected;
            } else if (resultClass == "error") {
                response.resultClass = GdbResultError;
            } else if (resultClass == "exit") {
                response.resultClass = GdbResultExit;
            } else {
                response.resultClass = GdbResultUnknown;
            }

            from = inner;
            if (from != to) {
                if (*from == ',') {
                    ++from;
                    response.data.parseTuple_helper(from, to);
                    response.data.m_type = GdbMi::Tuple;
                    response.data.m_name = "data";
                } else {
                    // Archer has this
                    response.data.m_type = GdbMi::Tuple;
                    response.data.m_name = "data";
                }
            }

            //qDebug() << "\nLOG STREAM:" + m_pendingLogStreamOutput;
            //qDebug() << "\nCONSOLE STREAM:" + m_pendingConsoleStreamOutput;
            response.data.setStreamOutput("logstreamoutput",
                m_pendingLogStreamOutput);
            response.data.setStreamOutput("consolestreamoutput",
                m_pendingConsoleStreamOutput);
            m_pendingLogStreamOutput.clear();
            m_pendingConsoleStreamOutput.clear();

            handleResultRecord(&response);
            break;
        }
        default: {
            qDebug() << "UNKNOWN RESPONSE TYPE" << c;
            break;
        }
    }
}

void GdbEngine::readGdbStandardError()
{
    QByteArray err = m_gdbProc.readAllStandardError();
    if (err == "Undefined command: \"bb\".  Try \"help\".\n")
        return;
    qWarning() << "Unexpected gdb stderr:" << err;
}

void GdbEngine::readGdbStandardOutput()
{
    if (m_commandTimer->isActive())
        m_commandTimer->start(); // Retrigger

    int newstart = 0;
    int scan = m_inbuffer.size();

    m_inbuffer.append(m_gdbProc.readAllStandardOutput());

    // This can trigger when a dialog starts a nested event loop
    if (m_busy)
        return;

    while (newstart < m_inbuffer.size()) {
        int start = newstart;
        int end = m_inbuffer.indexOf('\n', scan);
        if (end < 0) {
            m_inbuffer.remove(0, start);
            return;
        }
        newstart = end + 1;
        scan = newstart;
        if (end == start)
            continue;
        #if defined(Q_OS_WIN)
        if (m_inbuffer.at(end - 1) == '\r') {
            --end;
            if (end == start)
                continue;
        }
        #endif
        m_busy = true;
        handleResponse(QByteArray::fromRawData(m_inbuffer.constData() + start, end - start));
        m_busy = false;
    }
    m_inbuffer.clear();
}

void GdbEngine::interruptInferior()
{
    QTC_ASSERT(state() == InferiorRunning, qDebug() << state(); return);

    setState(InferiorStopping);
    showStatusMessage(tr("Stop requested..."), 5000);

    debugMessage(_("TRYING TO INTERRUPT INFERIOR"));
    m_gdbAdapter->interruptInferior();
}

void GdbEngine::interruptInferiorTemporarily()
{
    interruptInferior();
    foreach (const GdbCommand &cmd, m_commandsToRunOnTemporaryBreak)
        if (cmd.flags & LosesChild) {
            setState(InferiorStopping_Kill);
            break;
        }
}

void GdbEngine::maybeHandleInferiorPidChanged(const QString &pid0)
{
    const qint64 pid = pid0.toLongLong();
    if (pid == 0) {
        debugMessage(_("Cannot parse PID from %1").arg(pid0));
        return;
    }
    if (pid == inferiorPid())
        return;
    debugMessage(_("FOUND PID %1").arg(pid));

    handleInferiorPidChanged(pid);
}

void GdbEngine::postCommand(const QByteArray &command, AdapterCallback callback,
                            const char *callbackName, const QVariant &cookie)
{
    postCommand(command, NoFlags, callback, callbackName, cookie);
}

void GdbEngine::postCommand(const QByteArray &command, GdbCommandFlags flags,
                            AdapterCallback callback,
                            const char *callbackName, const QVariant &cookie)
{
    GdbCommand cmd;
    cmd.command = command;
    cmd.flags = flags;
    cmd.adapterCallback = callback;
    cmd.callbackName = callbackName;
    cmd.cookie = cookie;
    postCommandHelper(cmd);
}

void GdbEngine::postCommand(const QByteArray &command, GdbCommandCallback callback,
                            const char *callbackName, const QVariant &cookie)
{
    postCommand(command, NoFlags, callback, callbackName, cookie);
}

void GdbEngine::postCommand(const QByteArray &command, GdbCommandFlags flags,
                            GdbCommandCallback callback, const char *callbackName,
                            const QVariant &cookie)
{
    GdbCommand cmd;
    cmd.command = command;
    cmd.flags = flags;
    cmd.callback = callback;
    cmd.callbackName = callbackName;
    cmd.cookie = cookie;
    postCommandHelper(cmd);
}

void GdbEngine::postCommandHelper(const GdbCommand &cmd)
{
    if (!stateAcceptsGdbCommands(state())) {
        PENDING_DEBUG(_("NO GDB PROCESS RUNNING, CMD IGNORED: " + cmd.command));
        debugMessage(_("NO GDB PROCESS RUNNING, CMD IGNORED: %1 %2")
            .arg(_(cmd.command)).arg(state()));
        return;
    }

    if (cmd.flags & RebuildWatchModel) {
        ++m_pendingWatchRequests;
        PENDING_DEBUG("   WATCH MODEL:" << cmd.command << "=>" << cmd.callbackName
                      << "INCREMENTS PENDING TO" << m_pendingWatchRequests);
    } else if (cmd.flags & RebuildBreakpointModel) {
        ++m_pendingBreakpointRequests;
        PENDING_DEBUG("   BRWAKPOINT MODEL:" << cmd.command << "=>" << cmd.callbackName
                      << "INCREMENTS PENDING TO" << m_pendingBreakpointRequests);
    } else {
        PENDING_DEBUG("   OTHER (IN):" << cmd.command << "=>" << cmd.callbackName
                      << "LEAVES PENDING WATCH AT" << m_pendingWatchRequests
                      << "LEAVES PENDING BREAKPOINT AT" << m_pendingBreakpointRequests);
    }

    if ((cmd.flags & NeedsStop) || !m_commandsToRunOnTemporaryBreak.isEmpty()) {
        if (state() == InferiorStopped || state() == InferiorUnrunnable
            || state() == InferiorStarting || state() == AdapterStarted) {
            // Can be safely sent now.
            flushCommand(cmd);
        } else {
            // Queue the commands that we cannot send at once.
            debugMessage(_("QUEUING COMMAND " + cmd.command));
            m_commandsToRunOnTemporaryBreak.append(cmd);
            if (state() == InferiorStopping) {
                if (cmd.flags & LosesChild)
                    setState(InferiorStopping_Kill);
                debugMessage(_("CHILD ALREADY BEING INTERRUPTED"));
                // FIXME
                shutdown();
            } else if (state() == InferiorStopping_Kill) {
                debugMessage(_("CHILD ALREADY BEING INTERRUPTED (KILL PENDING)"));
                // FIXME
                shutdown();
            } else if (state() == InferiorRunningRequested) {
                if (cmd.flags & LosesChild)
                    setState(InferiorRunningRequested_Kill);
                debugMessage(_("RUNNING REQUESTED; POSTPONING INTERRUPT"));
            } else if (state() == InferiorRunningRequested_Kill) {
                debugMessage(_("RUNNING REQUESTED; POSTPONING INTERRUPT (KILL PENDING)"));
            } else if (state() == InferiorRunning) {
                showStatusMessage(tr("Stopping temporarily."), 1000);
                interruptInferiorTemporarily();
            } else {
                qDebug() << "ATTEMPTING TO QUEUE COMMAND IN INAPPROPRIATE STATE" << state();
            }
        }
    } else if (!cmd.command.isEmpty()) {
        flushCommand(cmd);
    }
}

void GdbEngine::flushQueuedCommands()
{
    showStatusMessage(tr("Processing queued commands."), 1000);
    while (!m_commandsToRunOnTemporaryBreak.isEmpty()) {
        GdbCommand cmd = m_commandsToRunOnTemporaryBreak.takeFirst();
        debugMessage(_("RUNNING QUEUED COMMAND " + cmd.command + ' '
            + cmd.callbackName));
        flushCommand(cmd);
    }
}

void GdbEngine::flushCommand(const GdbCommand &cmd0)
{
    GdbCommand cmd = cmd0;
    if (state() == DebuggerNotReady) {
        showDebuggerInput(LogInput, _(cmd.command));
        debugMessage(_("GDB PROCESS NOT RUNNING, PLAIN CMD IGNORED: " + cmd.command));
        return;
    }

    ++currentToken();
    cmd.postTime = QTime::currentTime();
    m_cookieForToken[currentToken()] = cmd;
    cmd.command = QByteArray::number(currentToken()) + cmd.command;
    showDebuggerInput(LogInput, _(cmd.command));

    m_gdbAdapter->write(cmd.command + "\r\n");

    m_commandTimer->start();

    if (cmd.flags & LosesChild)
        setState(InferiorShuttingDown);
}

int GdbEngine::commandTimeoutTime() const
{
    int time = theDebuggerAction(GdbWatchdogTimeout)->value().toInt();
    return 1000 * qMax(20, time);
}

void GdbEngine::commandTimeout()
{
    QList<int> keys = m_cookieForToken.keys();
    qSort(keys);
    bool killIt = false;
    foreach (int key, keys) {
        const GdbCommand &cmd = m_cookieForToken.value(key);
        if (!(cmd.flags & NonCriticalResponse))
            killIt = true;
        QByteArray msg = QByteArray::number(key);
        msg += ": " + cmd.command + " => ";
        QTC_ASSERT(cmd.callbackName, /**/);
        msg += cmd.callbackName;
        debugMessage(_(msg));
    }
    if (killIt) {
        debugMessage(_("TIMED OUT WAITING FOR GDB REPLY. COMMANDS STILL IN PROGRESS:"));
        int timeOut = m_commandTimer->interval();
        //m_commandTimer->stop();
        const QString msg = tr("The gdb process has not responded "
            "to a command within %1 seconds. This could mean it is stuck "
            "in an endless loop or taking longer than expected to perform "
            "the operation.\nYou can choose between waiting "
            "longer or abort debugging.").arg(timeOut / 1000);
        QMessageBox *mb = showMessageBox(QMessageBox::Critical,
            tr("Gdb not responding"), msg,
            QMessageBox::Ok | QMessageBox::Cancel);
        mb->button(QMessageBox::Cancel)->setText(tr("Give gdb more time"));
        mb->button(QMessageBox::Ok)->setText(tr("Stop debugging"));
        if (mb->exec() == QMessageBox::Ok) {
            debugMessage(_("KILLING DEBUGGER AS REQUESTED BY USER"));
            // This is an undefined state, so we just pull the emergency brake.
            manager()->watchHandler()->endCycle();
            m_gdbProc.kill();
        } else {
            debugMessage(_("CONTINUE DEBUGGER AS REQUESTED BY USER"));
        }
    } else {
        debugMessage(_("\nNON-CRITICAL TIMEOUT\n"));
    }
}

void GdbEngine::handleResultRecord(GdbResponse *response)
{
    //qDebug() << "TOKEN:" << response.token
    //    << " ACCEPTABLE:" << m_oldestAcceptableToken;
    //qDebug() << "\nRESULT" << response.token << response.toString();

    int token = response->token;
    if (token == -1)
        return;

    if (!m_cookieForToken.contains(token)) {
        // In theory this should not happen (rather the error should be
        // reported in the "first" response to the command) in practice it
        // does. We try to handle a few situations we are aware of gracefully.
        // Ideally, this code should not be present at all.
        debugMessage(_("COOKIE FOR TOKEN %1 ALREADY EATEN. "
            "TWO RESPONSES FOR ONE COMMAND?").arg(token));
        if (response->resultClass == GdbResultError) {
            QByteArray msg = response->data.findChild("msg").data();
            if (msg == "Cannot find new threads: generic error") {
                // Handle a case known to occur on Linux/gdb 6.8 when debugging moc
                // with helpers enabled. In this case we get a second response with
                // msg="Cannot find new threads: generic error"
                debugMessage(_("APPLYING WORKAROUND #1"));
                showMessageBox(QMessageBox::Critical,
                    tr("Executable failed"), QString::fromLocal8Bit(msg));
                showStatusMessage(tr("Process failed to start."));
                shutdown();
            } else if (msg == "\"finish\" not meaningful in the outermost frame.") {
                // Handle a case known to appear on gdb 6.4 symbianelf when
                // the stack is cut due to access to protected memory.
                debugMessage(_("APPLYING WORKAROUND #2"));
                setState(InferiorStopping);
                setState(InferiorStopped);
            } else if (msg.startsWith("Cannot find bounds of current function")) {
                // Happens when running "-exec-next" in a function for which
                // there is no debug information. Divert to "-exec-next-step"
                debugMessage(_("APPLYING WORKAROUND #3"));
                setState(InferiorStopping);
                setState(InferiorStopped);
                executeNextI();
            } else if (msg.startsWith("Couldn't get registers: No such process.")) {
                // Happens on archer-tromey-python 6.8.50.20090910-cvs
                // There might to be a race between a process shutting down
                // and library load messages.
                debugMessage(_("APPLYING WORKAROUND #4"));
                setState(InferiorStopping);
                setState(InferiorStopped);
                setState(InferiorShuttingDown);
                setState(InferiorShutDown);
                showStatusMessage(tr("Executable failed: %1")
                    .arg(QString::fromLocal8Bit(msg)));
                shutdown();
                showMessageBox(QMessageBox::Critical,
                    tr("Executable failed"), QString::fromLocal8Bit(msg));
            } else {
                showMessageBox(QMessageBox::Critical,
                    tr("Executable failed"), QString::fromLocal8Bit(msg));
                showStatusMessage(tr("Executable failed: %1")
                    .arg(QString::fromLocal8Bit(msg)));
            }
        }
        return;
    }

    GdbCommand cmd = m_cookieForToken.take(token);
    if (theDebuggerBoolSetting(LogTimeStamps)) {
        showDebuggerOutput(LogTime, _("Response time: %1: %2 s")
            .arg(_(cmd.command))
            .arg(cmd.postTime.msecsTo(QTime::currentTime()) / 1000.));
    }

    if (response->token < m_oldestAcceptableToken && (cmd.flags & Discardable)) {
        //debugMessage(_("### SKIPPING OLD RESULT") + response.toString());
        return;
    }

    response->cookie = cmd.cookie;

    if (response->resultClass != GdbResultError &&
        response->resultClass != ((cmd.flags & RunRequest) ? GdbResultRunning :
                                  (cmd.flags & ExitRequest) ? GdbResultExit :
                                  GdbResultDone)) {
        QByteArray rsp = GdbResponse::stringFromResultClass(response->resultClass);
        rsp = "UNEXPECTED RESPONSE " + rsp + " TO COMMAND" + cmd.command;
        qWarning() << rsp << " AT " __FILE__ ":" STRINGIFY(__LINE__);
        debugMessage(_(rsp));
    } else {
        if (cmd.callback)
            (this->*cmd.callback)(*response);
        else if (cmd.adapterCallback)
            (m_gdbAdapter->*cmd.adapterCallback)(*response);
    }

    if (cmd.flags & RebuildWatchModel) {
        --m_pendingWatchRequests;
        PENDING_DEBUG("   WATCH" << cmd.command << "=>" << cmd.callbackName
                      << "DECREMENTS PENDING WATCH TO" << m_pendingWatchRequests);
        if (m_pendingWatchRequests <= 0) {
            PENDING_DEBUG("\n\n ... AND TRIGGERS WATCH MODEL UPDATE\n");
            rebuildWatchModel();
        }
    } else if (cmd.flags & RebuildBreakpointModel) {
        --m_pendingBreakpointRequests;
        PENDING_DEBUG("   BREAKPOINT" << cmd.command << "=>" << cmd.callbackName
                      << "DECREMENTS PENDING TO" << m_pendingWatchRequests);
        if (m_pendingBreakpointRequests <= 0) {
            PENDING_DEBUG("\n\n ... AND TRIGGERS BREAKPOINT MODEL UPDATE\n");
            attemptBreakpointSynchronization();
        }
    } else {
        PENDING_DEBUG("   OTHER (OUT):" << cmd.command << "=>" << cmd.callbackName
                      << "LEAVES PENDING WATCH AT" << m_pendingWatchRequests
                      << "LEAVES PENDING BREAKPOINT AT" << m_pendingBreakpointRequests);
    }

    // Commands were queued, but we were in RunningRequested state, so the interrupt
    // was postponed.
    // This is done after the command callbacks so the running-requesting commands
    // can assert on the right state.
    if (state() == InferiorRunning && !m_commandsToRunOnTemporaryBreak.isEmpty())
        interruptInferiorTemporarily();

    // Continue only if there are no commands wire anymore, so this will
    // be fully synchroneous.
    // This is somewhat inefficient, as it makes the last command synchronous.
    // An optimization would be requesting the continue immediately when the
    // event loop is entered, and let individual commands have a flag to suppress
    // that behavior.
    if (m_commandsDoneCallback && m_cookieForToken.isEmpty()) {
        debugMessage(_("ALL COMMANDS DONE; INVOKING CALLBACK"));
        CommandsDoneCallback cont = m_commandsDoneCallback;
        m_commandsDoneCallback = 0;
        (this->*cont)();
    } else {
        PENDING_DEBUG("MISSING TOKENS: " << m_cookieForToken.keys());
    }

    if (m_cookieForToken.isEmpty())
        m_commandTimer->stop();
}

void GdbEngine::executeDebuggerCommand(const QString &command)
{
    if (state() == DebuggerNotReady) {
        debugMessage(_("GDB PROCESS NOT RUNNING, PLAIN CMD IGNORED: ") + command);
        return;
    }

    m_gdbAdapter->write(command.toLatin1() + "\r\n");
}

// Called from CoreAdapter and AttachAdapter
void GdbEngine::updateAll()
{
    if (hasPython())
        updateAllPython();
    else
        updateAllClassic();
}

void GdbEngine::handleQuerySources(const GdbResponse &response)
{
    m_sourcesListUpdating = false;
    m_sourcesListOutdated = false;
    if (response.resultClass == GdbResultDone) {
        QMap<QString, QString> oldShortToFull = m_shortToFullName;
        m_shortToFullName.clear();
        m_fullToShortName.clear();
        // "^done,files=[{file="../../../../bin/gdbmacros/gdbmacros.cpp",
        // fullname="/data5/dev/ide/main/bin/gdbmacros/gdbmacros.cpp"},
        GdbMi files = response.data.findChild("files");
        foreach (const GdbMi &item, files.children()) {
            GdbMi fullName = item.findChild("fullname");
            GdbMi fileName = item.findChild("file");
            QString file = QString::fromLocal8Bit(fileName.data());
            if (fullName.isValid()) {
                QString full = cleanupFullName(QString::fromLocal8Bit(fullName.data()));
                m_shortToFullName[file] = full;
                m_fullToShortName[full] = file;
            } else if (fileName.isValid()) {
                m_shortToFullName[file] = tr("<unknown>");
            }
        }
        if (m_shortToFullName != oldShortToFull)
            manager()->sourceFileWindow()->setSourceFiles(m_shortToFullName);
    }
}

#if 0
void GdbEngine::handleExecJumpToLine(const GdbResponse &response)
{
    // FIXME: remove this special case as soon as 'jump'
    // is supported by MI
    // "&"jump /home/apoenitz/dev/work/test1/test1.cpp:242"
    // ~"Continuing at 0x4058f3."
    // ~"run1 (argc=1, argv=0x7fffb213a478) at test1.cpp:242"
    // ~"242\t x *= 2;"
    //109^done"
    setState(InferiorStopped);
    showStatusMessage(tr("Jumped. Stopped."));
    QByteArray output = response.data.findChild("logstreamoutput").data();
    if (output.isEmpty())
        return;
    int idx1 = output.indexOf(' ') + 1;
    if (idx1 > 0) {
        int idx2 = output.indexOf(':', idx1);
        if (idx2 > 0) {
            QString file = QString::fromLocal8Bit(output.mid(idx1, idx2 - idx1));
            int line = output.mid(idx2 + 1).toInt();
            gotoLocation(file, line, true);
        }
    }
}
#endif

//void GdbEngine::handleExecRunToFunction(const GdbResponse &response)
//{
//    // FIXME: remove this special case as soon as there's a real
//    // reason given when the temporary breakpoint is hit.
//    // reight now we get:
//    // 14*stopped,thread-id="1",frame={addr="0x0000000000403ce4",
//    // func="foo",args=[{name="str",value="@0x7fff0f450460"}],
//    // file="main.cpp",fullname="/tmp/g/main.cpp",line="37"}
//    QTC_ASSERT(state() == InferiorStopping, qDebug() << state())
//    setState(InferiorStopped);
//    showStatusMessage(tr("Function reached. Stopped."));
//    GdbMi frame = response.data.findChild("frame");
//    StackFrame f = parseStackFrame(frame, 0);
//    gotoLocation(f, true);
//}

static bool isExitedReason(const QByteArray &reason)
{
    return reason == "exited-normally"   // inferior exited normally
        || reason == "exited-signalled"  // inferior exited because of a signal
        //|| reason == "signal-received" // inferior received signal
        || reason == "exited";           // inferior exited
}

#if 0
void GdbEngine::handleAqcuiredInferior()
{
    // Reverse debugging. FIXME: Should only be used when available.
    //if (theDebuggerBoolSetting(EnableReverseDebugging))
    //    postCommand("target response");

    tryLoadDebuggingHelpers();

    #ifndef Q_OS_MAC
    // intentionally after tryLoadDebuggingHelpers(),
    // otherwise we'd interrupt solib loading.
    if (theDebuggerBoolSetting(AllPluginBreakpoints)) {
        postCommand("set auto-solib-add on");
        postCommand("set stop-on-solib-events 0");
        postCommand("sharedlibrary .*");
    } else if (theDebuggerBoolSetting(SelectedPluginBreakpoints)) {
        postCommand("set auto-solib-add on");
        postCommand("set stop-on-solib-events 1");
        postCommand("sharedlibrary "
          + theDebuggerStringSetting(SelectedPluginBreakpointsPattern));
    } else if (theDebuggerBoolSetting(NoPluginBreakpoints)) {
        // should be like that already
        postCommand("set auto-solib-add off");
        postCommand("set stop-on-solib-events 0");
    }
    #endif

    // It's nicer to see a bit of the world we live in.
    reloadModulesInternal();
}
#endif

#ifdef Q_OS_UNIX
# define STOP_SIGNAL "SIGINT"
# define CROSS_STOP_SIGNAL "SIGTRAP"
#else
# define STOP_SIGNAL "SIGTRAP"
# define CROSS_STOP_SIGNAL "SIGINT"
#endif

void GdbEngine::handleStopResponse(const GdbMi &data)
{
    // This is gdb 7+'s initial *stopped in response to attach.
    // For consistency, we just discard it.
    if (state() == InferiorStarting)
        return;

    const QByteArray reason = data.findChild("reason").data();

    if (isExitedReason(reason)) {
        if (state() == InferiorRunning) {
            setState(InferiorStopping);
        } else {
            // The user triggered a stop, but meanwhile the app simply exited ...
            QTC_ASSERT(state() == InferiorStopping || state() == InferiorStopping_Kill,
                       qDebug() << state());
        }
        setState(InferiorStopped);
        QString msg;
        if (reason == "exited") {
            msg = tr("Program exited with exit code %1.")
                .arg(_(data.findChild("exit-code").toString()));
        } else if (reason == "exited-signalled" || reason == "signal-received") {
            msg = tr("Program exited after receiving signal %1.")
                .arg(_(data.findChild("signal-name").toString()));
        } else {
            msg = tr("Program exited normally.");
        }
        showStatusMessage(msg);
        setState(InferiorShuttingDown);
        setState(InferiorShutDown);
        shutdown();
        return;
    }

    if (!m_commandsToRunOnTemporaryBreak.isEmpty()) {
        QTC_ASSERT(state() == InferiorStopping || state() == InferiorStopping_Kill,
                   qDebug() << state())
        setState(InferiorStopped);
        flushQueuedCommands();
        if (state() == InferiorStopped) {
            QTC_ASSERT(m_commandsDoneCallback == 0, /**/);
            m_commandsDoneCallback = &GdbEngine::autoContinueInferior;
        } else {
            QTC_ASSERT(state() == InferiorShuttingDown, qDebug() << state())
        }
        return;
    }

    if (state() == InferiorRunning) {
        // Stop triggered by a breakpoint or otherwise not directly
        // initiated by the user.
        setState(InferiorStopping);
    } else {
        QTC_ASSERT(state() == InferiorStopping, qDebug() << state());
    }
    setState(InferiorStopped);

#if 0 // See http://vladimir_prus.blogspot.com/2007/12/debugger-stories-pending-breakpoints.html
    // Due to LD_PRELOADing the dumpers, these events can occur even before
    // reaching the entry point. So handle it before the entry point hacks below.
    if (reason.isEmpty() && m_gdbVersion < 70000 && !m_isMacGdb) {
        // On Linux it reports "Stopped due to shared library event\n", but
        // on Windows it simply forgets about it. Thus, we identify the response
        // based on it having no frame information.
        if (!data.findChild("frame").isValid()) {
            invalidateSourcesList();
            // Each stop causes a roundtrip and button flicker, so prevent
            // a flood of useless stops. Will be automatically re-enabled.
            postCommand("set stop-on-solib-events 0");
#if 0
            // The related code (handleAqcuiredInferior()) is disabled as well.
            if (theDebuggerBoolSetting(SelectedPluginBreakpoints)) {
                QString dataStr = _(data.toString());
                debugMessage(_("SHARED LIBRARY EVENT: ") + dataStr);
                QString pat = theDebuggerStringSetting(SelectedPluginBreakpointsPattern);
                debugMessage(_("PATTERN: ") + pat);
                postCommand("sharedlibrary " + pat.toLocal8Bit());
                showStatusMessage(tr("Loading %1...").arg(dataStr));
            }
#endif
            continueInferiorInternal();
            return;
        }
    }
#endif

#ifdef Q_OS_LINUX
    if (!m_entryPoint.isEmpty()) {
        GdbMi frameData = data.findChild("frame");
        if (frameData.findChild("addr").data() == m_entryPoint) {
            // There are two expected reasons for getting here:
            // 1) For some reason, attaching to a stopped process causes *two* SIGSTOPs
            //    when trying to continue (kernel i386 2.6.24-23-ubuntu, gdb 6.8).
            //    Interestingly enough, on MacOSX no signal is delivered at all.
            // 2) The explicit tbreak at the entry point we set to query the PID.
            //    Gdb <= 6.8 reports a frame but no reason, 6.8.50+ reports everything.
            // The case of the user really setting a breakpoint at _start is simply
            // unsupported.
            if (!inferiorPid()) // For programs without -pthread under gdb <= 6.8.
                postCommand("info proc", CB(handleInfoProc));
            continueInferiorInternal();
            return;
        }
        // We are past the initial stop(s). No need to waste time on further checks.
        m_entryPoint.clear();
    }
#endif

    // seen on XP after removing a breakpoint while running
    //  >945*stopped,reason="signal-received",signal-name="SIGTRAP",
    //  signal-meaning="Trace/breakpoint trap",thread-id="2",
    //  frame={addr="0x7c91120f",func="ntdll!DbgUiConnectToDbg",
    //  args=[],from="C:\\WINDOWS\\system32\\ntdll.dll"}
    // also seen on gdb 6.8-symbianelf without qXfer:libraries:read+;
    // FIXME: remote.c parses "loaded" reply. It should be turning
    // that into a TARGET_WAITKIND_LOADED. Does it?
    // The bandaid here has the problem that it breaks for 'next' over a
    // statement that indirectly loads shared libraries
    // 6.1.2010: Breaks interrupting inferiors, disabled:
    // if (reason == "signal-received"
    //      && data.findChild("signal-name").data() == "SIGTRAP") {
    //    continueInferiorInternal();
    //    return;
    // }

    // jump over well-known frames
    static int stepCounter = 0;
    if (theDebuggerBoolSetting(SkipKnownFrames)) {
        if (reason == "end-stepping-range" || reason == "function-finished") {
            GdbMi frame = data.findChild("frame");
            //debugMessage(frame.toString());
            QString funcName = _(frame.findChild("func").data());
            QString fileName = QString::fromLocal8Bit(frame.findChild("file").data());
            if (isLeavableFunction(funcName, fileName)) {
                //debugMessage(_("LEAVING ") + funcName);
                ++stepCounter;
                m_manager->executeStepOut();
                return;
            }
            if (isSkippableFunction(funcName, fileName)) {
                //debugMessage(_("SKIPPING ") + funcName);
                ++stepCounter;
                m_manager->executeStep();
                return;
            }
            //if (stepCounter)
            //    qDebug() << "STEPCOUNTER:" << stepCounter;
            stepCounter = 0;
        }
    }

    bool initHelpers = m_debuggingHelperState == DebuggingHelperUninitialized
                       || m_debuggingHelperState == DebuggingHelperLoadTried;
    // Don't load helpers on stops triggered by signals unless it's
    // an intentional trap.
    if (initHelpers
            && m_gdbAdapter->dumperHandling() != AbstractGdbAdapter::DumperLoadedByGdbPreload
            && reason == "signal-received") {
        QByteArray name = data.findChild("signal-name").data();
        if (name != STOP_SIGNAL
            && (startParameters().startMode != StartRemote
                || name != CROSS_STOP_SIGNAL))
            initHelpers = false;
    }
    if (isSynchroneous())
        initHelpers = false;
    if (initHelpers) {
        tryLoadDebuggingHelpersClassic();
        QVariant var = QVariant::fromValue<GdbMi>(data);
        postCommand("p 4", CB(handleStop1), var);  // dummy
    } else {
        handleStop1(data);
    }
    // Dumper loading is sequenced, as otherwise the display functions
    // will start requesting data without knowing that dumpers are available.
}

void GdbEngine::handleStop1(const GdbResponse &response)
{
    handleStop1(response.cookie.value<GdbMi>());
}

void GdbEngine::handleStop1(const GdbMi &data)
{
    QByteArray reason = data.findChild("reason").data();

    if (0 && m_gdbAdapter->isTrkAdapter()
            && reason == "signal-received"
            && data.findChild("signal-name").data() == "SIGTRAP") {
        // Caused by "library load" message.
        debugMessage(_("INTERNAL CONTINUE"));
        continueInferiorInternal();
        return;
    }

    // This is for display only.
    if (m_modulesListOutdated)
        reloadModulesInternal();

    // This needs to be done before fullName() may need it.
    if (m_sourcesListOutdated && theDebuggerBoolSetting(UsePreciseBreakpoints))
        reloadSourceFilesInternal();

    if (m_breakListOutdated) {
        reloadBreakListInternal();
    } else {
        // Older gdb versions do not produce "library loaded" messages
        // so the breakpoint update is not triggered.
        if (m_gdbVersion < 70000 && !m_isMacGdb
            && manager()->breakHandler()->size() > 0)
            reloadBreakListInternal();
    }

    if (reason == "breakpoint-hit") {
        QByteArray bpNumber = data.findChild("bkptno").data();
        QByteArray threadId = data.findChild("thread-id").data();
        showStatusMessage(tr("Stopped at breakpoint %1 in thread %2.")
            .arg(_(bpNumber), _(threadId)));
    } else {
        QString reasontr = tr("Stopped: \"%1\"").arg(_(reason));
        if (reason == "signal-received"
            && theDebuggerBoolSetting(UseMessageBoxForSignals)) {
            QByteArray name = data.findChild("signal-name").data();
            QByteArray meaning = data.findChild("signal-meaning").data();
            // Ignore these as they are showing up regularly when
            // stopping debugging.
            if (name != STOP_SIGNAL
                && (startParameters().startMode != StartRemote
                    || name != CROSS_STOP_SIGNAL)) {
                QString msg = tr("<p>The inferior stopped because it received a "
                    "signal from the Operating System.<p>"
                    "<table><tr><td>Signal name : </td><td>%1</td></tr>"
                    "<tr><td>Signal meaning : </td><td>%2</td></tr></table>")
                    .arg(name.isEmpty() ? tr(" <Unknown> ", "name") : _(name))
                    .arg(meaning.isEmpty() ? tr(" <Unknown> ", "meaning") : _(meaning));
                showMessageBox(QMessageBox::Information,
                    tr("Signal received"), msg);
                if (!name.isEmpty() && !meaning.isEmpty())
                    reasontr = tr("Stopped: %1 by signal %2.")
                        .arg(_(meaning)).arg(_(name));
            }
        }

        if (reason.isEmpty())
            showStatusMessage(tr("Stopped."));
        else
            showStatusMessage(reasontr);
    }

    const GdbMi gdbmiFrame = data.findChild("frame");

    m_currentFrame = _(gdbmiFrame.findChild("addr").data() + '%' +
         gdbmiFrame.findChild("func").data() + '%');

    // Quick shot: Jump to stack frame #0.
    StackFrame frame;
    if (gdbmiFrame.isValid()) {
        frame = parseStackFrame(gdbmiFrame, 0);
        gotoLocation(frame, true);
    }

    //
    // Stack
    //
    manager()->stackHandler()->setCurrentIndex(0);
    updateLocals(qVariantFromValue(frame)); // Quick shot

    reloadStack(false);

    if (supportsThreads()) {
        int currentId = data.findChild("thread-id").data().toInt();
        if (m_gdbAdapter->isTrkAdapter())
            m_gdbAdapter->trkReloadThreads();
        else
            postCommand("-thread-list-ids", WatchUpdate,
                CB(handleStackListThreads), currentId);
    }

    //
    // Registers
    //
    manager()->reloadRegisters();
}

#ifdef Q_OS_LINUX
void GdbEngine::handleInfoProc(const GdbResponse &response)
{
    if (response.resultClass == GdbResultDone) {
        static QRegExp re(_("\\bprocess ([0-9]+)\n"));
        QTC_ASSERT(re.isValid(), return);
        if (re.indexIn(_(response.data.findChild("consolestreamoutput").data())) != -1)
            maybeHandleInferiorPidChanged(re.cap(1));
    }
}
#endif

void GdbEngine::handleShowVersion(const GdbResponse &response)
{
    //qDebug () << "VERSION 2:" << response.data.findChild("consolestreamoutput").data();
    //qDebug () << "VERSION:" << response.toString();
    debugMessage(_("VERSION: " + response.toString()));
    if (response.resultClass == GdbResultDone) {
        m_gdbVersion = 100;
        m_gdbBuildVersion = -1;
        m_isMacGdb = false;
        GdbMi version = response.data.findChild("consolestreamoutput");
        QString msg = QString::fromLocal8Bit(version.data());

        bool foundIt = false;

        QRegExp supported(_("GNU gdb(.*) (\\d+)\\.(\\d+)(\\.(\\d+))?(-(\\d+))?"));
        if (supported.indexIn(msg) >= 0) {
            debugMessage(_("SUPPORTED GDB VERSION ") + msg);
            m_gdbVersion = 10000 * supported.cap(2).toInt()
                         +   100 * supported.cap(3).toInt()
                         +     1 * supported.cap(5).toInt();
            m_gdbBuildVersion = supported.cap(7).toInt();
            m_isMacGdb = msg.contains(__("Apple version"));
            foundIt = true;
        }

        // OpenSUSE managed to ship "GNU gdb (GDB) SUSE (6.8.91.20090930-2.4).
        if (!foundIt && msg.startsWith(_("GNU gdb (GDB) SUSE "))) {
            supported.setPattern(_("[^\\d]*(\\d+).(\\d+).(\\d+).*"));
            if (supported.indexIn(msg) >= 0) {
                debugMessage(_("SUSE PATCHED GDB VERSION ") + msg);
                m_gdbVersion = 10000 * supported.cap(1).toInt()
                             +   100 * supported.cap(2).toInt()
                             +     1 * supported.cap(3).toInt();
                m_gdbBuildVersion = -1;
                m_isMacGdb = false;
                foundIt = true;
            } else {
                debugMessage(_("UNPARSABLE SUSE PATCHED GDB VERSION ") + msg);
            }
        }

        if (!foundIt) {
            debugMessage(_("UNSUPPORTED GDB VERSION ") + msg);
#if 0
            QStringList list = msg.split(_c('\n'));
            while (list.size() > 2)
                list.removeLast();
            msg = tr("The debugger you are using identifies itself as:")
                + _("<p><p>") + list.join(_("<br>")) + _("<p><p>")
                + tr("This version is not officially supported by Qt Creator.\n"
                     "Debugging will most likely not work well.\n"
                     "Using gdb 7.1 or later is strongly recommended.");
#if 0
            // ugly, but 'Show again' check box...
            static QErrorMessage *err = new QErrorMessage(mainWindow());
            err->setMinimumSize(400, 300);
            err->showMessage(msg);
#else
            //showMessageBox(QMessageBox::Information, tr("Warning"), msg);
#endif
#endif
        }

        debugMessage(_("USING GDB VERSION: %1, BUILD: %2%3").arg(m_gdbVersion)
            .arg(m_gdbBuildVersion).arg(_(m_isMacGdb ? " (APPLE)" : "")));
        //qDebug () << "VERSION 3:" << m_gdbVersion << m_gdbBuildVersion;
    }
}

void GdbEngine::handleHasPython(const GdbResponse &response)
{
    if (response.resultClass == GdbResultDone) {
        m_hasPython = true;
    } else {
        m_hasPython = false;
        if (m_gdbAdapter->dumperHandling() == AbstractGdbAdapter::DumperLoadedByGdbPreload
                && checkDebuggingHelpersClassic()) {
            QByteArray cmd = "set environment ";
            cmd += Debugger::Constants::Internal::LD_PRELOAD_ENV_VAR;
            cmd += ' ';
            cmd += manager()->qtDumperLibraryName().toLocal8Bit();
            postCommand(cmd);
            m_debuggingHelperState = DebuggingHelperLoadTried;
        }
    }
}

void GdbEngine::handleExecuteContinue(const GdbResponse &response)
{
    if (response.resultClass == GdbResultRunning) {
        // The "running" state is picked up in handleResponse()
        QTC_ASSERT(state() == InferiorRunning, /**/);
    } else {
        if (state() == InferiorRunningRequested_Kill) {
            setState(InferiorStopped);
            shutdown();
            return;
        }
        QTC_ASSERT(state() == InferiorRunningRequested, /**/);
        setState(InferiorStopped);
        QByteArray msg = response.data.findChild("msg").data();
        if (msg.startsWith("Cannot find bounds of current function")) {
            if (!m_commandsToRunOnTemporaryBreak.isEmpty())
                flushQueuedCommands();
            showStatusMessage(tr("Stopped."), 5000);
            //showStatusMessage(tr("No debug information available. "
            //  "Leaving function..."));
            //executeStepOut();
        } else {
            showMessageBox(QMessageBox::Critical, tr("Execution Error"),
                           tr("Cannot continue debugged process:\n") + QString::fromLocal8Bit(msg));
            shutdown();
        }
    }
}

QString GdbEngine::fullName(const QString &fileName)
{
    if (fileName.isEmpty())
        return QString();
    //QTC_ASSERT(!m_sourcesListOutdated, /* */)
    QTC_ASSERT(!m_sourcesListUpdating, /* */)
    return m_shortToFullName.value(fileName, QString());
}

#ifdef Q_OS_WIN
QString GdbEngine::cleanupFullName(const QString &fileName)
{
    QTC_ASSERT(!fileName.isEmpty(), return QString())
    // Gdb on windows often delivers "fullnames" which
    // a) have no drive letter and b) are not normalized.
    QFileInfo fi(fileName);
    if (!fi.isReadable())
        return QString();
    return QDir::cleanPath(fi.absoluteFilePath());
}
#endif

void GdbEngine::shutdown()
{
    debugMessage(_("INITIATE GDBENGINE SHUTDOWN"));
    switch (state()) {
    case DebuggerNotReady: // Nothing to do! :)
    case EngineStarting: // We can't get here, really
    case InferiorShuttingDown: // Will auto-trigger further shutdown steps
    case EngineShuttingDown: // Do not disturb! :)
    case InferiorRunningRequested_Kill:
    case InferiorStopping_Kill:
        break;
    case AdapterStarting: // GDB is up, adapter is "doing something"
        setState(AdapterStartFailed);
        m_gdbAdapter->shutdown();
        // fall-through
    case AdapterStartFailed: // Adapter "did something", but it did not help
        if (m_gdbProc.state() == QProcess::Running) {
            m_commandsToRunOnTemporaryBreak.clear();
            postCommand("-gdb-exit", GdbEngine::ExitRequest, CB(handleGdbExit));
        } else {
            setState(DebuggerNotReady);
        }
        break;
    case InferiorRunningRequested:
    case InferiorRunning:
    case InferiorStopping:
    case InferiorStopped:
        m_commandsToRunOnTemporaryBreak.clear();
        postCommand(m_gdbAdapter->inferiorShutdownCommand(),
                    NeedsStop | LosesChild, CB(handleInferiorShutdown));
        break;
    case AdapterStarted: // We can't get here, really
    case InferiorStartFailed:
    case InferiorShutDown:
    case InferiorShutdownFailed: // Whatever
    case InferiorUnrunnable:
        m_commandsToRunOnTemporaryBreak.clear();
        postCommand("-gdb-exit", GdbEngine::ExitRequest, CB(handleGdbExit));
        setState(EngineShuttingDown); // Do it after posting the command!
        break;
    case InferiorStarting: // This may take some time, so just short-circuit it
        setState(InferiorStartFailed);
        // fall-through
    case InferiorStopFailed: // Tough luck, I guess. But unreachable as of now anyway.
        setState(EngineShuttingDown);
        m_gdbProc.kill();
        break;
    }
}

void GdbEngine::handleInferiorShutdown(const GdbResponse &response)
{
    QTC_ASSERT(state() == InferiorShuttingDown, qDebug() << state());
    if (response.resultClass == GdbResultDone) {
        debugMessage(_("INFERIOR SUCCESSFULLY SHUT DOWN"));
        setState(InferiorShutDown);
    } else {
        debugMessage(_("INFERIOR SHUTDOWN FAILED"));
        setState(InferiorShutdownFailed);
        QString msg = m_gdbAdapter->msgInferiorStopFailed(
            QString::fromLocal8Bit(response.data.findChild("msg").data()));
        showMessageBox(QMessageBox::Critical, tr("Inferior shutdown failed"), msg);
    }
    shutdown(); // re-iterate...
}

void GdbEngine::handleGdbExit(const GdbResponse &response)
{
    if (response.resultClass == GdbResultExit) {
        debugMessage(_("GDB CLAIMS EXIT; WAITING"));
        m_commandsDoneCallback = 0;
        // don't set state here, this will be handled in handleGdbFinished()
    } else {
        QString msg = m_gdbAdapter->msgGdbStopFailed(
            QString::fromLocal8Bit(response.data.findChild("msg").data()));
        debugMessage(_("GDB WON'T EXIT (%1); KILLING IT").arg(msg));
        m_gdbProc.kill();
    }
}

void GdbEngine::detachDebugger()
{
    QTC_ASSERT(state() == InferiorStopped, /**/);
    QTC_ASSERT(startMode() != AttachCore, /**/);
    postCommand("detach");
    setState(InferiorShuttingDown);
    setState(InferiorShutDown);
    shutdown();
}

void GdbEngine::exitDebugger() // called from the manager
{
    disconnectDebuggingHelperActions();
    shutdown();
}

int GdbEngine::currentFrame() const
{
    return manager()->stackHandler()->currentIndex();
}

bool GdbEngine::checkConfiguration(int toolChain, QString *errorMessage, QString *settingsPage) const
{
    switch (toolChain) {
    case ProjectExplorer::ToolChain::WINSCW: // S60
    case ProjectExplorer::ToolChain::GCCE:
    case ProjectExplorer::ToolChain::RVCT_ARMV5:
    case ProjectExplorer::ToolChain::RVCT_ARMV6:
    case ProjectExplorer::ToolChain::RVCT_ARMV5_GNUPOC:
    case ProjectExplorer::ToolChain::GCCE_GNUPOC:
        if (!m_trkOptions->check(errorMessage)) {
            if (settingsPage)
                *settingsPage = TrkOptionsPage::settingsId();
            return false;
        }
    default:
        break;
    }
    return true;
}

AbstractGdbAdapter *GdbEngine::createAdapter(const DebuggerStartParametersPtr &sp)
{
    switch (sp->toolChainType) {
    case ProjectExplorer::ToolChain::WINSCW: // S60
    case ProjectExplorer::ToolChain::GCCE:
    case ProjectExplorer::ToolChain::RVCT_ARMV5:
    case ProjectExplorer::ToolChain::RVCT_ARMV6:
    case ProjectExplorer::ToolChain::RVCT_ARMV5_GNUPOC:
    case ProjectExplorer::ToolChain::GCCE_GNUPOC:
        return new TrkGdbAdapter(this, m_trkOptions);
    default:
        break;
    }
    // @todo: remove testing hack
    if (sp->processArgs.size() == 3 && sp->processArgs.at(0) == _("@sym@"))
        return new TrkGdbAdapter(this, m_trkOptions);
    switch (sp->startMode) {
    case AttachCore:
        return new CoreGdbAdapter(this);
    case StartRemote:
        return new RemoteGdbAdapter(this, sp->toolChainType);
    case AttachExternal:
        return new AttachGdbAdapter(this);
    default:
        if (sp->useTerminal)
            return new TermGdbAdapter(this);
        return new PlainGdbAdapter(this);
    }
}

void GdbEngine::startDebugger(const DebuggerStartParametersPtr &sp)
{
    QTC_ASSERT(state() == EngineStarting, qDebug() << state());
    // This should be set by the constructor or in exitDebugger()
    // via initializeVariables()
    //QTC_ASSERT(m_debuggingHelperState == DebuggingHelperUninitialized,
    //    initializeVariables());
    //QTC_ASSERT(m_gdbAdapter == 0, delete m_gdbAdapter; m_gdbAdapter = 0);

    initializeVariables();

    m_startParameters = sp;

    delete m_gdbAdapter;
    m_gdbAdapter = createAdapter(sp);
    connectAdapter();

    if (m_gdbAdapter->dumperHandling() != AbstractGdbAdapter::DumperNotAvailable)
        connectDebuggingHelperActions();

    m_gdbAdapter->startAdapter();
}

unsigned GdbEngine::debuggerCapabilities() const
{
    return ReverseSteppingCapability | SnapshotCapability
        | AutoDerefPointersCapability | DisassemblerCapability
        | RegisterCapability | ShowMemoryCapability
        | JumpToLineCapability | ReloadModuleCapability
        | ReloadModuleSymbolsCapability | BreakOnThrowAndCatchCapability
        | ReturnFromFunctionCapability;
}

void GdbEngine::continueInferiorInternal()
{
    QTC_ASSERT(state() == InferiorStopped || state() == InferiorStarting,
               qDebug() << state());
    setState(InferiorRunningRequested);
    postCommand("-exec-continue", RunRequest, CB(handleExecuteContinue));
}

void GdbEngine::autoContinueInferior()
{
    continueInferiorInternal();
    showStatusMessage(tr("Continuing after temporary stop..."), 1000);
}

void GdbEngine::continueInferior()
{
    m_manager->resetLocation();
    setTokenBarrier();
    continueInferiorInternal();
    showStatusMessage(tr("Running requested..."), 5000);
}

void GdbEngine::executeStep()
{
    QTC_ASSERT(state() == InferiorStopped, qDebug() << state());
    setTokenBarrier();
    setState(InferiorRunningRequested);
    showStatusMessage(tr("Step requested..."), 5000);
    StackHandler *stackHandler = manager()->stackHandler();
    if (m_gdbAdapter->isTrkAdapter() && stackHandler->stackSize() > 0)
        postCommand("sal step," + stackHandler->topAddress().toLatin1());
    if (manager()->isReverseDebugging())
        postCommand("-reverse-step", RunRequest, CB(handleExecuteStep));
    else
        postCommand("-exec-step", RunRequest, CB(handleExecuteStep));
}

void GdbEngine::handleExecuteStep(const GdbResponse &response)
{
    if (response.resultClass == GdbResultRunning) {
        // The "running" state is picked up in handleResponse()
        QTC_ASSERT(state() == InferiorRunning, /**/);
    } else {
        if (state() == InferiorRunningRequested_Kill) {
            setState(InferiorStopped);
            shutdown();
            return;
        }
        QTC_ASSERT(state() == InferiorRunningRequested, /**/);
        setState(InferiorStopped);
        QByteArray msg = response.data.findChild("msg").data();
        if (msg.startsWith("Cannot find bounds of current function")) {
            if (!m_commandsToRunOnTemporaryBreak.isEmpty())
                flushQueuedCommands();
            executeStepI(); // Fall back to instruction-wise stepping.
        } else {
            showMessageBox(QMessageBox::Critical, tr("Execution Error"),
                tr("Cannot continue debugged process:\n") + QString::fromLocal8Bit(msg));
            shutdown();
        }
    }
}

void GdbEngine::executeStepI()
{
    QTC_ASSERT(state() == InferiorStopped, qDebug() << state());
    setTokenBarrier();
    setState(InferiorRunningRequested);
    showStatusMessage(tr("Step by instruction requested..."), 5000);
    if (manager()->isReverseDebugging())
        postCommand("-reverse-stepi", RunRequest, CB(handleExecuteContinue));
    else
        postCommand("-exec-step-instruction", RunRequest, CB(handleExecuteContinue));
}

void GdbEngine::executeStepOut()
{
    QTC_ASSERT(state() == InferiorStopped, qDebug() << state());
    setTokenBarrier();
    setState(InferiorRunningRequested);
    showStatusMessage(tr("Finish function requested..."), 5000);
    postCommand("-exec-finish", RunRequest, CB(handleExecuteContinue));
}

void GdbEngine::executeNext()
{
    QTC_ASSERT(state() == InferiorStopped, qDebug() << state());
    setTokenBarrier();
    setState(InferiorRunningRequested);
    showStatusMessage(tr("Step next requested..."), 5000);
    StackHandler *stackHandler = manager()->stackHandler();
    if (m_gdbAdapter->isTrkAdapter() && stackHandler->stackSize() > 0)
        postCommand("sal next," + stackHandler->topAddress().toLatin1());
    if (manager()->isReverseDebugging())
        postCommand("-reverse-next", RunRequest, CB(handleExecuteNext));
    else
        postCommand("-exec-next", RunRequest, CB(handleExecuteNext));
}

void GdbEngine::handleExecuteNext(const GdbResponse &response)
{
    if (response.resultClass == GdbResultRunning) {
        // The "running" state is picked up in handleResponse()
        QTC_ASSERT(state() == InferiorRunning, /**/);
    } else {
        if (state() == InferiorRunningRequested_Kill) {
            setState(InferiorStopped);
            shutdown();
            return;
        }
        QTC_ASSERT(state() == InferiorRunningRequested, /**/);
        setState(InferiorStopped);
        QByteArray msg = response.data.findChild("msg").data();
        if (msg.startsWith("Cannot find bounds of current function")) {
            if (!m_commandsToRunOnTemporaryBreak.isEmpty())
                flushQueuedCommands();
            executeNextI(); // Fall back to instruction-wise stepping.
        } else {
            showMessageBox(QMessageBox::Critical, tr("Execution Error"),
                tr("Cannot continue debugged process:\n") + QString::fromLocal8Bit(msg));
            shutdown();
        }
    }
}

void GdbEngine::executeNextI()
{
    QTC_ASSERT(state() == InferiorStopped, qDebug() << state());
    setTokenBarrier();
    setState(InferiorRunningRequested);
    showStatusMessage(tr("Step next instruction requested..."), 5000);
    if (manager()->isReverseDebugging())
        postCommand("-reverse-nexti", RunRequest, CB(handleExecuteContinue));
    else
        postCommand("-exec-next-instruction", RunRequest, CB(handleExecuteContinue));
}

void GdbEngine::executeRunToLine(const QString &fileName, int lineNumber)
{
    QTC_ASSERT(state() == InferiorStopped, qDebug() << state());
    setTokenBarrier();
    setState(InferiorRunningRequested);
    showStatusMessage(tr("Run to line %1 requested...").arg(lineNumber), 5000);
    QByteArray args = '"' + breakLocation(fileName).toLocal8Bit() + '"' + ':'
        + QByteArray::number(lineNumber);
    postCommand("-exec-until " + args, RunRequest, CB(handleExecuteContinue));
}

void GdbEngine::executeRunToFunction(const QString &functionName)
{
    QTC_ASSERT(state() == InferiorStopped, qDebug() << state());
    setTokenBarrier();
    postCommand("-break-insert -t " + functionName.toLatin1());
    continueInferiorInternal();
    //setState(InferiorRunningRequested);
    //postCommand("-exec-continue", handleExecRunToFunction);
    showStatusMessage(tr("Run to function %1 requested...").arg(functionName), 5000);
}

void GdbEngine::executeJumpToLine(const QString &fileName, int lineNumber)
{
    QTC_ASSERT(state() == InferiorStopped, qDebug() << state());
    StackFrame frame;
    frame.file = fileName;
    frame.line = lineNumber;
#if 1
    QByteArray loc = '"' + breakLocation(fileName).toLocal8Bit() + '"' + ':'
        + QByteArray::number(lineNumber);
    postCommand("tbreak " + loc);
    setState(InferiorRunningRequested);
    postCommand("jump " + loc, RunRequest);
    // will produce something like
    //  &"jump \"/home/apoenitz/dev/work/test1/test1.cpp\":242"
    //  ~"Continuing at 0x4058f3."
    //  ~"run1 (argc=1, argv=0x7fffbf1f5538) at test1.cpp:242"
    //  ~"242\t x *= 2;"
    //  23^done"
    gotoLocation(frame, true);
    //setBreakpoint();
    //postCommand("jump " + loc);
#else
    gotoLocation(frame,  true);
    setBreakpoint(fileName, lineNumber);
    setState(InferiorRunningRequested);
    postCommand("jump " + loc, RunRequest);
#endif
}

void GdbEngine::executeReturn()
{
    QTC_ASSERT(state() == InferiorStopped, qDebug() << state());
    setTokenBarrier();
    setState(InferiorRunningRequested);
    showStatusMessage(tr("Immediate return from function requested..."), 5000);
    postCommand("-exec-finish", RunRequest, CB(handleExecuteReturn));
}

void GdbEngine::handleExecuteReturn(const GdbResponse &response)
{
    if (response.resultClass == GdbResultDone) {
        updateAll();
    }
}
/*!
    \fn void GdbEngine::setTokenBarrier()
    \brief Discard the results of all pending watch-updating commands.

    This method is called at the beginning of all step/next/finish etc.
    debugger functions.
    If non-watch-updating commands with call-backs are still in the pipe,
    it will complain.
*/

void GdbEngine::setTokenBarrier()
{
    foreach (const GdbCommand &cookie, m_cookieForToken) {
        QTC_ASSERT(!cookie.callback || (cookie.flags & Discardable),
            qDebug() << "CMD:" << cookie.command << " CALLBACK:" << cookie.callbackName;
            return
        );
    }
    PENDING_DEBUG("\n--- token barrier ---\n");
    showDebuggerInput(LogMisc, _("--- token barrier ---"));
    if (theDebuggerBoolSetting(LogTimeStamps))
        showDebuggerInput(LogMisc, currentTime());
    m_oldestAcceptableToken = currentToken();
}


//////////////////////////////////////////////////////////////////////
//
// Breakpoint specific stuff
//
//////////////////////////////////////////////////////////////////////

void GdbEngine::breakpointDataFromOutput(BreakpointData *data, const GdbMi &bkpt)
{
    if (!bkpt.isValid())
        return;
    if (!data)
        return;
    data->pending = false;
    data->bpMultiple = false;
    data->bpEnabled = true;
    data->bpCondition.clear();
    QByteArray file, fullName;
    foreach (const GdbMi &child, bkpt.children()) {
        if (child.hasName("number")) {
            data->bpNumber = child.data();
        } else if (child.hasName("func")) {
            data->bpFuncName = _(child.data());
        } else if (child.hasName("addr")) {
            // <MULTIPLE> happens in constructors. In this case there are
            // _two_ fields named "addr" in the response. On Linux that is...
            if (child.data() == "<MULTIPLE>")
                data->bpMultiple = true;
            else
                data->bpAddress = child.data();
        } else if (child.hasName("file")) {
            file = child.data();
        } else if (child.hasName("fullname")) {
            fullName = child.data();
        } else if (child.hasName("line")) {
            data->bpLineNumber = child.data();
            if (child.data().toInt())
                data->markerLineNumber = child.data().toInt();
        } else if (child.hasName("cond")) {
            data->bpCondition = child.data();
            // gdb 6.3 likes to "rewrite" conditions. Just accept that fact.
            if (data->bpCondition != data->condition && data->conditionsMatch())
                data->condition = data->bpCondition;
        } else if (child.hasName("enabled")) {
            data->bpEnabled = (child.data() == "y");
        } else if (child.hasName("pending")) {
            data->pending = true;
            // Any content here would be interesting only if we did accept
            // spontaneously appearing breakpoints (user using gdb commands).
        } else if (child.hasName("at")) {
            // Happens with (e.g.?) gdb 6.4 symbianelf
            QByteArray ba = child.data();
            if (ba.startsWith('<') && ba.endsWith('>'))
                ba = ba.mid(1, ba.size() - 2);
            data->bpFuncName = _(ba);
        }
    }
    // This field is not present.  Contents needs to be parsed from
    // the plain "ignore" response.
    //else if (child.hasName("ignore"))
    //    data->bpIgnoreCount = child.data();

    QString name;
    if (!fullName.isEmpty()) {
        name = cleanupFullName(QFile::decodeName(fullName));
        if (data->markerFileName.isEmpty())
            data->markerFileName = name;
    } else {
        name = QFile::decodeName(file);
        // Use fullName() once we have a mapping which is more complete than
        // gdb's own. No point in assigning markerFileName for now.
    }
    if (!name.isEmpty())
        data->bpFileName = name;
}

QString GdbEngine::breakLocation(const QString &file) const
{
    //QTC_ASSERT(!m_breakListOutdated, /* */)
    QString where = m_fullToShortName.value(file);
    if (where.isEmpty())
        return QFileInfo(file).fileName();
    return where;
}

QByteArray GdbEngine::breakpointLocation(int index)
{
    const BreakpointData *data = manager()->breakHandler()->at(index);
    if (!data->funcName.isEmpty())
        return data->funcName.toLatin1();
    QString loc = data->useFullPath ? data->fileName : breakLocation(data->fileName);
    // The argument is simply a C-quoted version of the argument to the
    // non-MI "break" command, including the "original" quoting it wants.
    return "\"\\\"" + GdbMi::escapeCString(loc).toLocal8Bit() + "\\\":"
        + data->lineNumber + '"';
}

void GdbEngine::sendInsertBreakpoint(int index)
{
    // Set up fallback in case of pending breakpoints which aren't handled
    // by the MI interface.
    QByteArray cmd;
    if (m_isMacGdb)
        cmd = "-break-insert -l -1 -f ";
    else if (m_gdbAdapter->isTrkAdapter())
        cmd = "-break-insert -h -f ";
    else if (m_gdbVersion >= 60800)
        // Probably some earlier version would work as well.
        cmd = "-break-insert -f ";
    else
        cmd = "-break-insert ";
    //if (!data->condition.isEmpty())
    //    cmd += "-c " + data->condition + ' ';
    cmd += breakpointLocation(index);
    postCommand(cmd, NeedsStop | RebuildBreakpointModel,
        CB(handleBreakInsert1), index);
}

void GdbEngine::handleBreakInsert1(const GdbResponse &response)
{
    int index = response.cookie.toInt();
    BreakHandler *handler = manager()->breakHandler();
    if (response.resultClass == GdbResultDone) {
        // Interesting only on Mac?
        BreakpointData *data = handler->at(index);
        GdbMi bkpt = response.data.findChild("bkpt");
        breakpointDataFromOutput(data, bkpt);
    } else {
        // Some versions of gdb like "GNU gdb (GDB) SUSE (6.8.91.20090930-2.4)"
        // know how to do pending breakpoints using CLI but not MI. So try 
        // again with MI.
        QByteArray cmd = "break " + breakpointLocation(index);
        postCommand(cmd, NeedsStop | RebuildBreakpointModel,
            CB(handleBreakInsert2), index);
    }
}

void GdbEngine::handleBreakInsert2(const GdbResponse &response)
{
    if (response.resultClass == GdbResultError) {
        if (m_gdbVersion < 60800 && !m_isMacGdb) {
            // This gdb version doesn't "do" pending breakpoints.
            // Not much we can do about it except implementing the
            // logic on top of shared library events, and that's not
            // worth the effort.
        } else {
            QTC_ASSERT(false, /**/);
        }
    }
}

void GdbEngine::reloadBreakListInternal()
{
    postCommand("-break-list",
        NeedsStop | RebuildBreakpointModel,
        CB(handleBreakList));
}

void GdbEngine::handleBreakList(const GdbResponse &response)
{
    // 45^done,BreakpointTable={nr_rows="2",nr_cols="6",hdr=[
    // {width="3",alignment="-1",col_name="number",colhdr="Num"}, ...
    // body=[bkpt={number="1",type="breakpoint",disp="keep",enabled="y",
    //  addr="0x000000000040109e",func="main",file="app.cpp",
    //  fullname="/home/apoenitz/dev/work/plugintest/app/app.cpp",
    //  line="11",times="1"},
    //  bkpt={number="2",type="breakpoint",disp="keep",enabled="y",
    //  addr="<PENDING>",pending="plugin.cpp:7",times="0"}] ... }

    if (response.resultClass == GdbResultDone) {
        GdbMi table = response.data.findChild("BreakpointTable");
        if (table.isValid())
            handleBreakList(table);
    }
}

void GdbEngine::handleBreakList(const GdbMi &table)
{
    GdbMi body = table.findChild("body");
    QList<GdbMi> bkpts;
    if (body.isValid()) {
        // Non-Mac
        bkpts = body.children();
    } else {
        // Mac
        bkpts = table.children();
        // Remove the 'hdr' and artificial items.
        for (int i = bkpts.size(); --i >= 0; ) {
            int num = bkpts.at(i).findChild("number").data().toInt();
            if (num <= 0)
                bkpts.removeAt(i);
        }
        //qDebug() << "LEFT" << bkpts.size() << "BREAKPOINTS";
    }

    BreakHandler *handler = manager()->breakHandler();
    for (int index = 0; index != bkpts.size(); ++index) {
        BreakpointData temp(handler);
        breakpointDataFromOutput(&temp, bkpts.at(index));
        int found = handler->findBreakpoint(temp);
        if (found != -1)
            breakpointDataFromOutput(handler->at(found), bkpts.at(index));
        //else
            //qDebug() << "CANNOT HANDLE RESPONSE" << bkpts.at(index).toString();
    }

    m_breakListOutdated = false;
}

void GdbEngine::handleBreakIgnore(const GdbResponse &response)
{
    int index = response.cookie.toInt();
    // gdb 6.8:
    // ignore 2 0:
    // ~"Will stop next time breakpoint 2 is reached.\n"
    // 28^done
    // ignore 2 12:
    // &"ignore 2 12\n"
    // ~"Will ignore next 12 crossings of breakpoint 2.\n"
    // 29^done
    //
    // gdb 6.3 does not produce any console output
    BreakHandler *handler = manager()->breakHandler();
    if (response.resultClass == GdbResultDone && index < handler->size()) {
        QString msg = _(response.data.findChild("consolestreamoutput").data());
        BreakpointData *data = handler->at(index);
        //if (msg.contains(__("Will stop next time breakpoint"))) {
        //    data->bpIgnoreCount = _("0");
        //} else if (msg.contains(__("Will ignore next"))) {
        //    data->bpIgnoreCount = data->ignoreCount;
        //}
        // FIXME: this assumes it is doing the right thing...
        data->bpIgnoreCount = data->ignoreCount;
        handler->updateMarkers();
    }
}

void GdbEngine::handleBreakCondition(const GdbResponse &response)
{
    int index = response.cookie.toInt();
    BreakHandler *handler = manager()->breakHandler();
    if (response.resultClass == GdbResultDone) {
        // We just assume it was successful. Otherwise we had to parse
        // the output stream data.
        BreakpointData *data = handler->at(index);
        //qDebug() << "HANDLE BREAK CONDITION" << index << data->condition;
        data->bpCondition = data->condition;
    } else {
        QByteArray msg = response.data.findChild("msg").data();
        // happens on Mac
        if (1 || msg.startsWith("Error parsing breakpoint condition. "
                " Will try again when we hit the breakpoint.")) {
            BreakpointData *data = handler->at(index);
            //qDebug() << "ERROR BREAK CONDITION" << index << data->condition;
            data->bpCondition = data->condition;
        }
    }
    handler->updateMarkers();
}

void GdbEngine::extractDataFromInfoBreak(const QString &output, BreakpointData *data)
{
    data->bpFileName = _("<MULTIPLE>");

    //qDebug() << output;
    if (output.isEmpty())
        return;
    // "Num     Type           Disp Enb Address            What
    // 4       breakpoint     keep y   <MULTIPLE>         0x00000000004066ad
    // 4.1                         y     0x00000000004066ad in CTorTester
    //  at /data5/dev/ide/main/tests/manual/gdbdebugger/simple/app.cpp:124
    // - or -
    // everything on a single line on Windows for constructors of classes
    // within namespaces.
    // Sometimes the path is relative too.

    // 2    breakpoint     keep y   <MULTIPLE> 0x0040168e
    // 2.1    y     0x0040168e in MainWindow::MainWindow(QWidget*) at mainwindow.cpp:7
    // 2.2    y     0x00401792 in MainWindow::MainWindow(QWidget*) at mainwindow.cpp:7

    // tested in ../../../tests/auto/debugger/
    QRegExp re(_("MULTIPLE.*(0x[0-9a-f]+) in (.*)\\s+at (.*):([\\d]+)([^\\d]|$)"));
    re.setMinimal(true);

    if (re.indexIn(output) != -1) {
        data->bpAddress = re.cap(1).toLatin1();
        data->bpFuncName = re.cap(2).trimmed();
        data->bpLineNumber = re.cap(4).toLatin1();
        QString full = fullName(re.cap(3));
        if (full.isEmpty()) {
            // FIXME: This happens without UsePreciseBreakpoints regularly.
            // We need to revive that "fill full name mapping bit by bit"
            // approach of 1.2.x
            //qDebug() << "NO FULL NAME KNOWN FOR" << re.cap(3);
            full = cleanupFullName(re.cap(3));
            if (full.isEmpty()) {
                qDebug() << "FILE IS NOT RESOLVABLE" << re.cap(3);
                full = re.cap(3); // FIXME: wrong, but prevents recursion
            }
        }
        // The variable full could still contain, say "foo.cpp" when we asked
        // for "/full/path/to/foo.cpp". In this case, using the requested
        // instead of the acknowledged name makes sense as it will allow setting
        // the marker in more cases.
        if (data->fileName.endsWith(full))
            full = data->fileName;
        data->markerLineNumber = data->bpLineNumber.toInt();
        data->markerFileName = full;
        data->bpFileName = full;
    } else {
        qDebug() << "COULD NOT MATCH " << re.pattern() << " AND " << output;
        data->bpNumber = "<unavailable>";
    }
}

void GdbEngine::handleBreakInfo(const GdbResponse &response)
{
    int bpNumber = response.cookie.toInt();
    BreakHandler *handler = manager()->breakHandler();
    if (response.resultClass == GdbResultDone) {
        // Old-style output for multiple breakpoints, presumably in a
        // constructor.
        int found = handler->findBreakpoint(bpNumber);
        if (found != -1) {
            QString str = QString::fromLocal8Bit(
                response.data.findChild("consolestreamoutput").data());
            extractDataFromInfoBreak(str, handler->at(found));
        }
    }
}

void GdbEngine::attemptBreakpointSynchronization()
{
    QTC_ASSERT(!m_sourcesListUpdating,
        qDebug() << "SOURCES LIST CURRENTLY UPDATING"; return);
    debugMessage(tr("ATTEMPT BREAKPOINT SYNC"));

    switch (state()) {
    case InferiorStarting:
    case InferiorRunningRequested:
    case InferiorRunning:
    case InferiorStopping:
    case InferiorStopped:
        break;
    default:
        //qDebug() << "attempted breakpoint sync in state" << state();
        return;
    }

    // For best results, we rely on an up-to-date fullname mapping.
    // The listing completion will retrigger us, so no futher action is needed.
    if (m_sourcesListOutdated && theDebuggerBoolSetting(UsePreciseBreakpoints)) {
        if (state() == InferiorRunning) {
            // FIXME: this is a hack
            // The hack solves the problem that we want both commands
            // (reloadSourceFiles and reloadBreakList) to be executed
            // within the same stop-executecommand-continue cycle.
            // Just calling reloadSourceFiles and reloadBreakList doesn't work
            // in this case, because a) stopping the executable is asynchronous,
            // b) we wouldn't want to stop-exec-continue twice
            m_sourcesListUpdating = true;
            GdbCommand cmd;
            cmd.command = "-file-list-exec-source-files";
            cmd.flags = NoFlags;
            cmd.callback = &GdbEngine::handleQuerySources;
            cmd.callbackName = "";
            m_commandsToRunOnTemporaryBreak.append(cmd);
        } else {
            reloadSourceFilesInternal();
        }
        reloadBreakListInternal();
        return;
    }

    if (m_breakListOutdated) {
        reloadBreakListInternal();
        return;
    }

    BreakHandler *handler = manager()->breakHandler();

    foreach (BreakpointData *data, handler->takeDisabledBreakpoints()) {
        QByteArray bpNumber = data->bpNumber;
        if (!bpNumber.trimmed().isEmpty()) {
            postCommand("-break-disable " + bpNumber,
                NeedsStop | RebuildBreakpointModel);
            data->bpEnabled = false;
        }
    }

    foreach (BreakpointData *data, handler->takeEnabledBreakpoints()) {
        QByteArray bpNumber = data->bpNumber;
        if (!bpNumber.trimmed().isEmpty()) {
            postCommand("-break-enable " + bpNumber,
                NeedsStop | RebuildBreakpointModel);
            data->bpEnabled = true;
        }
    }

    foreach (BreakpointData *data, handler->takeRemovedBreakpoints()) {
        QByteArray bpNumber = data->bpNumber;
        debugMessage(_("DELETING BP " + bpNumber + " IN "
            + data->markerFileName.toLocal8Bit()));
        if (!bpNumber.trimmed().isEmpty())
            postCommand("-break-delete " + bpNumber,
                NeedsStop | RebuildBreakpointModel);
        delete data;
    }

    for (int index = 0; index != handler->size(); ++index) {
        BreakpointData *data = handler->at(index);
        if (data->bpNumber.isEmpty()) { // Unset breakpoint?
            data->bpNumber = " "; // Sent, but no feedback yet.
            sendInsertBreakpoint(index);
        } else if (data->bpNumber.toInt()) {
            if (data->bpMultiple && data->bpFileName.isEmpty()) {
                postCommand("info break " + data->bpNumber,
                    RebuildBreakpointModel,
                    CB(handleBreakInfo), data->bpNumber.toInt());
                continue;
            }
            // update conditions if needed
            if (data->condition != data->bpCondition && !data->conditionsMatch())
                postCommand("condition " + data->bpNumber + ' '  + data->condition,
                    RebuildBreakpointModel,
                    CB(handleBreakCondition), index);
            // update ignorecount if needed
            if (data->ignoreCount != data->bpIgnoreCount)
                postCommand("ignore " + data->bpNumber + ' ' + data->ignoreCount,
                    RebuildBreakpointModel,
                    CB(handleBreakIgnore), index);
            if (!data->enabled && data->bpEnabled) {
                postCommand("-break-disable " + data->bpNumber,
                    NeedsStop | RebuildBreakpointModel);
                data->bpEnabled = false;
            }
        }
    }

    handler->updateMarkers();
}


//////////////////////////////////////////////////////////////////////
//
// Modules specific stuff
//
//////////////////////////////////////////////////////////////////////

void GdbEngine::loadSymbols(const QString &moduleName)
{
    // FIXME: gdb does not understand quoted names here (tested with 6.8)
    postCommand("sharedlibrary " + dotEscape(moduleName.toLocal8Bit()));
    reloadModulesInternal();
}

void GdbEngine::loadAllSymbols()
{
    postCommand("sharedlibrary .*");
    reloadModulesInternal();
}

QList<Symbol> GdbEngine::moduleSymbols(const QString &moduleName)
{
    QList<Symbol> rc;
    bool success = false;
    QString errorMessage;
    do {
        const QString nmBinary = _("nm");
        QProcess proc;
        proc.start(nmBinary, QStringList() << _("-D") << moduleName);
        if (!proc.waitForFinished()) {
            errorMessage = tr("Unable to run '%1': %2").arg(nmBinary, proc.errorString());
            break;
        }
        const QString contents = QString::fromLocal8Bit(proc.readAllStandardOutput());
        const QRegExp re(_("([0-9a-f]+)?\\s+([^\\s]+)\\s+([^\\s]+)"));
        Q_ASSERT(re.isValid());
        foreach (const QString &line, contents.split(_c('\n'))) {
            if (re.indexIn(line) != -1) {
                Symbol symbol;
                symbol.address = re.cap(1);
                symbol.state = re.cap(2);
                symbol.name = re.cap(3);
                rc.push_back(symbol);
            } else {
                qWarning("moduleSymbols: unhandled: %s", qPrintable(line));
            }
        }
        success = true;
    } while (false);
    if (!success)
        qWarning("moduleSymbols: %s\n", qPrintable(errorMessage));
    return rc;
}

void GdbEngine::reloadModules()
{
    if (state() == InferiorRunning || state() == InferiorStopped)
        reloadModulesInternal();
}

void GdbEngine::reloadModulesInternal()
{
    m_modulesListOutdated = false;
    postCommand("info shared", NeedsStop, CB(handleModulesList));
#if 0
    if (m_gdbVersion < 70000 && !m_isMacGdb)
        postCommand("set stop-on-solib-events 1");
#endif
}

void GdbEngine::handleModulesList(const GdbResponse &response)
{
    QList<Module> modules;
    if (response.resultClass == GdbResultDone) {
        // That's console-based output, likely Linux or Windows,
        // but we can avoid the #ifdef here.
        QString data = QString::fromLocal8Bit(response.data.findChild("consolestreamoutput").data());
        QTextStream ts(&data, QIODevice::ReadOnly);
        while (!ts.atEnd()) {
            QString line = ts.readLine();
            Module module;
            QString symbolsRead;
            QTextStream ts(&line, QIODevice::ReadOnly);
            if (line.startsWith(__("0x"))) {
                ts >> module.startAddress >> module.endAddress >> symbolsRead;
                module.moduleName = ts.readLine().trimmed();
                module.symbolsRead = (symbolsRead == __("Yes"));
                modules.append(module);
            } else if (line.trimmed().startsWith(__("No"))) {
                // gdb 6.4 symbianelf
                ts >> symbolsRead;
                QTC_ASSERT(symbolsRead == __("No"), continue);
                module.moduleName = ts.readLine().trimmed();
                modules.append(module);
            }
        }
        if (modules.isEmpty()) {
            // Mac has^done,shlib-info={num="1",name="dyld",kind="-",
            // dyld-addr="0x8fe00000",reason="dyld",requested-state="Y",
            // state="Y",path="/usr/lib/dyld",description="/usr/lib/dyld",
            // loaded_addr="0x8fe00000",slide="0x0",prefix="__dyld_"},
            // shlib-info={...}...
            foreach (const GdbMi &item, response.data.children()) {
                Module module;
                module.moduleName = QString::fromLocal8Bit(item.findChild("path").data());
                module.symbolsRead = (item.findChild("state").data() == "Y");
                module.startAddress = _(item.findChild("loaded_addr").data());
                //: End address of loaded module
                module.endAddress = tr("<unknown>", "address");
                modules.append(module);
            }
        }
    }
    manager()->modulesHandler()->setModules(modules);
}


//////////////////////////////////////////////////////////////////////
//
// Source files specific stuff
//
//////////////////////////////////////////////////////////////////////

void GdbEngine::invalidateSourcesList()
{
    m_modulesListOutdated = true;
    m_sourcesListOutdated = true;
    m_breakListOutdated = true;
}

void GdbEngine::reloadSourceFiles()
{
    if ((state() == InferiorRunning || state() == InferiorStopped)
        && !m_sourcesListUpdating)
        reloadSourceFilesInternal();
}

void GdbEngine::reloadSourceFilesInternal()
{
    QTC_ASSERT(!m_sourcesListUpdating, /**/);
    m_sourcesListUpdating = true;
    postCommand("-file-list-exec-source-files", NeedsStop, CB(handleQuerySources));
#if 0
    if (m_gdbVersion < 70000 && !m_isMacGdb)
        postCommand("set stop-on-solib-events 1");
#endif
}


//////////////////////////////////////////////////////////////////////
//
// Stack specific stuff
//
//////////////////////////////////////////////////////////////////////

void GdbEngine::selectThread(int index)
{
    ThreadsHandler *threadsHandler = manager()->threadsHandler();
    threadsHandler->setCurrentThread(index);

    QList<ThreadData> threads = threadsHandler->threads();
    QTC_ASSERT(index < threads.size(), return);
    int id = threads.at(index).id;
    showStatusMessage(tr("Retrieving data for stack view..."), 10000);
    postCommand("-thread-select " + QByteArray::number(id), CB(handleStackSelectThread));
}

void GdbEngine::handleStackSelectThread(const GdbResponse &)
{
    QTC_ASSERT(state() == InferiorUnrunnable || state() == InferiorStopped, /**/);
    //qDebug("FIXME: StackHandler::handleOutput: SelectThread");
    showStatusMessage(tr("Retrieving data for stack view..."), 3000);
    manager()->reloadRegisters();
    reloadStack(true);
    updateLocals();
}

void GdbEngine::reloadFullStack()
{
    PENDING_DEBUG("RELOAD FULL STACK");
    postCommand("-stack-list-frames", WatchUpdate, CB(handleStackListFrames),
        QVariant::fromValue<StackCookie>(StackCookie(true, true)));
}

void GdbEngine::reloadStack(bool forceGotoLocation)
{
    PENDING_DEBUG("RELOAD STACK");
    QByteArray cmd = "-stack-list-frames";
    int stackDepth = theDebuggerAction(MaximalStackDepth)->value().toInt();
    if (stackDepth && !m_gdbAdapter->isTrkAdapter())
        cmd += " 0 " + QByteArray::number(stackDepth);
    // FIXME: gdb 6.4 symbianelf likes to be asked twice. The first time it
    // returns with "^error,msg="Previous frame identical to this frame
    // (corrupt stack?)". Might be related to the fact that we can't
    // access the memory belonging to the lower frames. But as we know
    // this sometimes happens, ask the second time immediately instead
    // of waiting for the first request to fail.
    // FIXME: Seems to work with 6.8.
    if (m_gdbAdapter->isTrkAdapter() && m_gdbVersion < 6.8)
        postCommand(cmd, WatchUpdate);
    postCommand(cmd, WatchUpdate, CB(handleStackListFrames),
        QVariant::fromValue<StackCookie>(StackCookie(false, forceGotoLocation)));
}

StackFrame GdbEngine::parseStackFrame(const GdbMi &frameMi, int level)
{
    //qDebug() << "HANDLING FRAME:" << frameMi.toString();
    StackFrame frame;
    frame.level = level;
    GdbMi fullName = frameMi.findChild("fullname");
    if (fullName.isValid())
        frame.file = cleanupFullName(QFile::decodeName(fullName.data()));
    else
        frame.file = QFile::decodeName(frameMi.findChild("file").data());
    frame.function = _(frameMi.findChild("func").data());
    frame.from = _(frameMi.findChild("from").data());
    frame.line = frameMi.findChild("line").data().toInt();
    frame.address = _(frameMi.findChild("addr").data());
    return frame;
}

void GdbEngine::handleStackListFrames(const GdbResponse &response)
{
    bool handleIt = (m_isMacGdb || response.resultClass == GdbResultDone);
    if (!handleIt) {
        // That always happens on symbian gdb with
        // ^error,data={msg="Previous frame identical to this frame (corrupt stack?)"
        // logstreamoutput="Previous frame identical to this frame (corrupt stack?)\n"
        //qDebug() << "LISTING STACK FAILED: " << response.toString();
        return;
    }

    StackCookie cookie = response.cookie.value<StackCookie>();
    QList<StackFrame> stackFrames;

    GdbMi stack = response.data.findChild("stack");
    if (!stack.isValid()) {
        qDebug() << "FIXME: stack:" << stack.toString();
        return;
    }

    int targetFrame = -1;

    int n = stack.childCount();
    for (int i = 0; i != n; ++i) {
        stackFrames.append(parseStackFrame(stack.childAt(i), i));
        const StackFrame &frame = stackFrames.back();

        #if defined(Q_OS_WIN)
        const bool isBogus =
            // Assume this is wrong and points to some strange stl_algobase
            // implementation. Happens on Karsten's XP system with Gdb 5.50
            (frame.file.endsWith(__("/bits/stl_algobase.h")) && frame.line == 150)
            // Also wrong. Happens on Vista with Gdb 5.50
               || (frame.function == __("operator new") && frame.line == 151);

        // Immediately leave bogus frames.
        if (targetFrame == -1 && isBogus) {
            setTokenBarrier();
            setState(InferiorRunningRequested);
            postCommand("-exec-finish", RunRequest, CB(handleExecuteContinue));
            showStatusMessage(tr("Jumping out of bogus frame..."), 1000);
            return;
        }
        #endif

        // Initialize top frame to the first valid frame.
        const bool isValid = frame.isUsable() && !frame.function.isEmpty();
        if (isValid && targetFrame == -1)
            targetFrame = i;
    }

    bool canExpand = !cookie.isFull
        && (n >= theDebuggerAction(MaximalStackDepth)->value().toInt());
    theDebuggerAction(ExpandStack)->setEnabled(canExpand);
    manager()->stackHandler()->setFrames(stackFrames, canExpand);

    // We can't jump to any file if we don't have any frames.
    if (stackFrames.isEmpty())
        return;

    // targetFrame contains the top most frame for which we have source
    // information. That's typically the frame we'd like to jump to, with
    // a few exceptions:

    // Always jump to frame #0 when stepping by instruction.
    if (theDebuggerBoolSetting(OperateByInstruction))
        targetFrame = 0;

    // If there is no frame with source, jump to frame #0.
    if (targetFrame == -1)
        targetFrame = 0;

    // Mac gdb does not add the location to the "stopped" message,
    // so the early gotoLocation() was not triggered. Force it here.
    // For targetFrame == 0 we already issued a 'gotoLocation'
    // when reading the *stopped message.
    bool jump = (m_isMacGdb || targetFrame != 0);

    manager()->stackHandler()->setCurrentIndex(targetFrame);
    if (jump || cookie.gotoLocation) {
        const StackFrame &frame = manager()->stackHandler()->currentFrame();
        //qDebug() << "GOTO, 2ND ATTEMPT: " << frame.toString() << targetFrame;
        gotoLocation(frame, true);
    }
}

void GdbEngine::activateFrame(int frameIndex)
{
    m_manager->resetLocation();
    if (state() != InferiorStopped && state() != InferiorUnrunnable)
        return;

    StackHandler *stackHandler = manager()->stackHandler();
    int oldIndex = stackHandler->currentIndex();

    if (frameIndex == stackHandler->stackSize()) {
        reloadFullStack();
        return;
    }

    QTC_ASSERT(frameIndex < stackHandler->stackSize(), return);

    if (oldIndex != frameIndex) {
        setTokenBarrier();

        // Assuming the command always succeeds this saves a roundtrip.
        // Otherwise the lines below would need to get triggered
        // after a response to this -stack-select-frame here.
        postCommand("-stack-select-frame " + QByteArray::number(frameIndex));

        stackHandler->setCurrentIndex(frameIndex);
        updateLocals();
        reloadRegisters();
    }

    gotoLocation(stackHandler->currentFrame(), true);
}

void GdbEngine::handleStackListThreads(const GdbResponse &response)
{
    int id = response.cookie.toInt();
    // "72^done,{thread-ids={thread-id="2",thread-id="1"},number-of-threads="2"}
    const QList<GdbMi> items = response.data.findChild("thread-ids").children();
    QList<ThreadData> threads;
    int currentIndex = -1;
    for (int index = 0, n = items.size(); index != n; ++index) {
        ThreadData thread;
        thread.id = items.at(index).data().toInt();
        threads.append(thread);
        if (thread.id == id) {
            //qDebug() << "SETTING INDEX TO:" << index << " ID:"
            // << id << " RECOD:" << response.toString();
            currentIndex = index;
        }
    }
    ThreadsHandler *threadsHandler = manager()->threadsHandler();
    threadsHandler->setThreads(threads);
    threadsHandler->setCurrentThread(currentIndex);
}


//////////////////////////////////////////////////////////////////////
//
// Snapshot specific stuff
//
//////////////////////////////////////////////////////////////////////

void GdbEngine::makeSnapshot()
{
    QString fileName;
    QTemporaryFile tf(QDir::tempPath() + _("/gdbsnapshot"));
    if (tf.open()) {
        fileName = tf.fileName();
        tf.close();
        postCommand("gcore " + fileName.toLocal8Bit(),
            NeedsStop, CB(handleMakeSnapshot), fileName);
    } else {
        showMessageBox(QMessageBox::Critical, tr("Snapshot Creation Error"),
            tr("Cannot create snapshot file."));
    }
}

void GdbEngine::handleMakeSnapshot(const GdbResponse &response)
{
    if (response.resultClass == GdbResultDone) {
        SnapshotData snapshot;
        snapshot.setDate(QDateTime::currentDateTime());
        snapshot.setLocation(response.cookie.toString());
        snapshot.setFrames(manager()->stackHandler()->frames());
        manager()->snapshotHandler()->appendSnapshot(snapshot);
    } else {
        QByteArray msg = response.data.findChild("msg").data();
        showMessageBox(QMessageBox::Critical, tr("Snapshot Creation Error"),
            tr("Cannot create snapshot:\n") + QString::fromLocal8Bit(msg));
    }
}

void GdbEngine::activateSnapshot(int index)
{
    SnapshotData snapshot = m_manager->snapshotHandler()->setCurrentIndex(index);
    m_startParameters->startMode = AttachCore;
    m_startParameters->coreFile = snapshot.location();

    if (state() == InferiorUnrunnable) {
        // All is well. We are looking at another core file.
        setState(EngineShuttingDown);
        setState(DebuggerNotReady);
        activateSnapshot2();
    } else if (state() != DebuggerNotReady) {
        QMessageBox *mb = showMessageBox(QMessageBox::Critical,
            tr("Snapshot Reloading"),
            tr("In order to load snapshots the debugged process needs "
             "to be stopped. Continuation will not be possible afterwards.\n"
             "Do you want to stop the debugged process and load the selected "
             "snapshot?"), QMessageBox::Ok | QMessageBox::Cancel);
        if (mb->exec() == QMessageBox::Cancel)
            return;
        debugMessage(_("KILLING DEBUGGER AS REQUESTED BY USER"));
        delete m_gdbAdapter;
        m_gdbAdapter = createAdapter(m_startParameters);
        postCommand("kill", NeedsStop, CB(handleActivateSnapshot));
    } else {
        activateSnapshot2();
    }
}

void GdbEngine::handleActivateSnapshot(const GdbResponse &response)
{
    Q_UNUSED(response);
    setState(InferiorShuttingDown);
    setState(InferiorShutDown);
    setState(EngineShuttingDown);
    setState(DebuggerNotReady);
    activateSnapshot2();
}

void GdbEngine::activateSnapshot2()
{
    // Otherwise the stack data might be stale.
    // See http://sourceware.org/bugzilla/show_bug.cgi?id=1124.
    setState(EngineStarting);
    setState(AdapterStarting);
    postCommand("set stack-cache off");
    handleAdapterStarted();
}


//////////////////////////////////////////////////////////////////////
//
// Register specific stuff
//
//////////////////////////////////////////////////////////////////////

void GdbEngine::reloadRegisters()
{
    if (state() != InferiorStopped && state() != InferiorUnrunnable)
        return;
    if (!m_registerNamesListed) {
        postCommand("-data-list-register-names", CB(handleRegisterListNames));
        m_registerNamesListed = true;
        // FIXME: Maybe better completely re-do this logic in TRK adapter.
        if (m_gdbAdapter->isTrkAdapter())
            return;
    }

    if (m_gdbAdapter->isTrkAdapter()) {
        m_gdbAdapter->trkReloadRegisters();
    } else {
        postCommand("-data-list-register-values x",
                    Discardable, CB(handleRegisterListValues));
    }
}

void GdbEngine::setRegisterValue(int nr, const QString &value)
{
    Register reg = manager()->registerHandler()->registers().at(nr);
    //qDebug() << "NOT IMPLEMENTED: CHANGE REGISTER " << nr << reg.name << ":"
    //    << value;
    postCommand("-var-delete \"R@\"");
    postCommand("-var-create \"R@\" * $" + reg.name);
    postCommand("-var-assign \"R@\" " + value.toLatin1());
    postCommand("-var-delete \"R@\"");
    //postCommand("-data-list-register-values d",
    //            Discardable, CB(handleRegisterListValues));
    reloadRegisters();
}

void GdbEngine::handleRegisterListNames(const GdbResponse &response)
{
    if (response.resultClass != GdbResultDone) {
        m_registerNamesListed = false;
        return;
    }

    Registers registers;
    foreach (const GdbMi &item, response.data.findChild("register-names").children())
        registers.append(Register(item.data()));

    manager()->registerHandler()->setRegisters(registers);

    if (m_gdbAdapter->isTrkAdapter())
        m_gdbAdapter->trkReloadRegisters();
}

void GdbEngine::handleRegisterListValues(const GdbResponse &response)
{
    if (response.resultClass != GdbResultDone)
        return;

    Registers registers = manager()->registerHandler()->registers();

    // 24^done,register-values=[{number="0",value="0xf423f"},...]
    foreach (const GdbMi &item, response.data.findChild("register-values").children()) {
        int index = item.findChild("number").data().toInt();
        if (index < registers.size()) {
            Register &reg = registers[index];
            QString value = _(item.findChild("value").data());
            reg.changed = (value != reg.value);
            if (reg.changed)
                reg.value = value;
        }
    }
    manager()->registerHandler()->setRegisters(registers);
}


//////////////////////////////////////////////////////////////////////
//
// Thread specific stuff
//
//////////////////////////////////////////////////////////////////////

bool GdbEngine::supportsThreads() const
{
    // FSF gdb 6.3 crashes happily on -thread-list-ids. So don't use it.
    // The test below is a semi-random pick, 6.8 works fine
    return m_isMacGdb || m_gdbVersion > 60500;
}


//////////////////////////////////////////////////////////////////////
//
// Tooltip specific stuff
//
//////////////////////////////////////////////////////////////////////

QString GdbEngine::m_toolTipExpression;
QPoint GdbEngine::m_toolTipPos;

bool GdbEngine::showToolTip()
{
    WatchHandler *handler = manager()->watchHandler();
    WatchModel *model = handler->model(TooltipsWatch);
    QByteArray iname = tooltipINameForExpression(m_toolTipExpression.toLatin1());
    WatchItem *item = model->findItem(iname, model->rootItem());
    if (!item) {
        hideDebuggerToolTip();
        return false;
    }
    QModelIndex index = model->watchIndex(item);
    showDebuggerToolTip(m_toolTipPos, model, index, m_toolTipExpression);
    return true;
}

void GdbEngine::setToolTipExpression(const QPoint &mousePos,
    TextEditor::ITextEditor *editor, int cursorPos)
{
    if (state() != InferiorStopped || !isCppEditor(editor)) {
        //qDebug() << "SUPPRESSING DEBUGGER TOOLTIP, INFERIOR NOT STOPPED/Non Cpp editor";
        return;
    }

    if (theDebuggerBoolSetting(DebugDebuggingHelpers)) {
        // minimize interference
        return;
    }

    m_toolTipPos = mousePos;
    int line, column;
    QString exp = cppExpressionAt(editor, cursorPos, &line, &column);
    if (exp == m_toolTipExpression)
        return;

    m_toolTipExpression = exp;

    // FIXME: enable caching
    //if (showToolTip())
    //    return;

    if (exp.isEmpty() || exp.startsWith(_c('#')))  {
        //QToolTip::hideText();
        return;
    }

    if (!hasLetterOrNumber(exp)) {
        //QToolTip::showText(m_toolTipPos,
        //    tr("'%1' contains no identifier").arg(exp));
        return;
    }

    if (isKeyWord(exp))
        return;

    if (exp.startsWith(_c('"')) && exp.endsWith(_c('"')))  {
        //QToolTip::showText(m_toolTipPos, tr("String literal %1").arg(exp));
        return;
    }

    if (exp.startsWith(__("++")) || exp.startsWith(__("--")))
        exp = exp.mid(2);

    if (exp.endsWith(__("++")) || exp.endsWith(__("--")))
        exp = exp.mid(2);

    if (exp.startsWith(_c('<')) || exp.startsWith(_c('[')))
        return;

    if (hasSideEffects(exp)) {
        //QToolTip::showText(m_toolTipPos,
        //    tr("Cowardly refusing to evaluate expression '%1' "
        //       "with potential side effects").arg(exp));
        return;
    }

    // Gdb crashes when creating a variable object with the name
    // of the type of 'this'
/*
    for (int i = 0; i != m_currentLocals.childCount(); ++i) {
        if (m_currentLocals.childAt(i).exp == "this") {
            qDebug() << "THIS IN ROW" << i;
            if (m_currentLocals.childAt(i).type.startsWith(exp)) {
                QToolTip::showText(m_toolTipPos,
                    tr("%1: type of current 'this'").arg(exp));
                qDebug() << " TOOLTIP CRASH SUPPRESSED";
                return;
            }
            break;
        }
    }
*/

    if (isSynchroneous()) {
        updateLocals(QVariant());
        return;
    }

    WatchData toolTip;
    toolTip.exp = exp.toLatin1();
    toolTip.name = exp;
    toolTip.iname = tooltipINameForExpression(toolTip.exp);
    manager()->watchHandler()->removeData(toolTip.iname);
    manager()->watchHandler()->insertData(toolTip);
}


//////////////////////////////////////////////////////////////////////
//
// Watch specific stuff
//
//////////////////////////////////////////////////////////////////////

void GdbEngine::setWatchDataValue(WatchData &data, const GdbMi &mi,
    int encoding)
{
    if (mi.isValid())
        data.setValue(decodeData(mi.data(), encoding));
    else
        data.setValueNeeded();
}

void GdbEngine::setWatchDataEditValue(WatchData &data, const GdbMi &mi)
{
    if (mi.isValid())
        data.editvalue = mi.data();
}

void GdbEngine::setWatchDataValueToolTip(WatchData &data, const GdbMi &mi,
    int encoding)
{
    if (mi.isValid())
        data.setValueToolTip(decodeData(mi.data(), encoding));
}

void GdbEngine::setWatchDataChildCount(WatchData &data, const GdbMi &mi)
{
    if (mi.isValid())
        data.setHasChildren(mi.data().toInt() > 0);
}

void GdbEngine::setWatchDataValueEnabled(WatchData &data, const GdbMi &mi)
{
    if (mi.data() == "true")
        data.valueEnabled = true;
    else if (mi.data() == "false")
        data.valueEnabled = false;
}

void GdbEngine::setWatchDataValueEditable(WatchData &data, const GdbMi &mi)
{
    if (mi.data() == "true")
        data.valueEditable = true;
    else if (mi.data() == "false")
        data.valueEditable = false;
}

void GdbEngine::setWatchDataExpression(WatchData &data, const GdbMi &mi)
{
    if (mi.isValid())
        data.exp = mi.data();
}

void GdbEngine::setWatchDataAddress(WatchData &data, const GdbMi &mi)
{
    if (mi.isValid())
        setWatchDataAddressHelper(data, mi.data());
}

void GdbEngine::setWatchDataAddressHelper(WatchData &data, const QByteArray &addr)
{
    data.addr = addr;
    if (data.exp.isEmpty() && !data.addr.startsWith("$"))
        data.exp = "*(" + gdbQuoteTypes(data.type).toLatin1() + "*)" + data.addr;
}

void GdbEngine::setWatchDataSAddress(WatchData &data, const GdbMi &mi)
{
    if (mi.isValid())
        data.saddr = mi.data();
}

void GdbEngine::setAutoDerefPointers(const QVariant &on)
{
    Q_UNUSED(on)
    setTokenBarrier();
    updateLocals();
}

bool GdbEngine::hasDebuggingHelperForType(const QString &type) const
{
    if (!theDebuggerBoolSetting(UseDebuggingHelpers))
        return false;

    if (m_gdbAdapter->dumperHandling() == AbstractGdbAdapter::DumperNotAvailable) {
        // "call" is not possible in gdb when looking at core files
        return type == __("QString") || type.endsWith(__("::QString"))
            || type == __("QStringList") || type.endsWith(__("::QStringList"));
    }

    if (theDebuggerBoolSetting(DebugDebuggingHelpers)
            && manager()->stackHandler()->isDebuggingDebuggingHelpers())
        return false;

    if (m_debuggingHelperState != DebuggingHelperAvailable)
        return false;

    // simple types
    return m_dumperHelper.type(type) != QtDumperHelper::UnknownType;
}


void GdbEngine::updateWatchData(const WatchData &data)
{
    if (isSynchroneous()) {
        // This should only be called for fresh expanded items, not for
        // items that had their children retrieved earlier.
        //qDebug() << "\nUPDATE WATCH DATA: " << data.toString() << "\n";
#if 0
        WatchData data1 = data;
        data1.setAllUnneeded();
        insertData(data1);
        rebuildModel();
#else
        if (data.iname.endsWith("."))
            return;

        // Avoid endless loops created by faulty dumpers.
        QByteArray processedName = "1-" + data.iname;
        //qDebug() << "PROCESSED NAMES: " << processedName << m_processedNames;
        if (m_processedNames.contains(processedName)) {
            WatchData data1 = data;
            showDebuggerInput(LogStatus,
                _("<Breaking endless loop for " + data.iname + '>'));
            data1.setAllUnneeded();
            data1.setValue(_("<unavailable>"));
            data1.setHasChildren(false);
            insertData(data1);
            return;
        }
        m_processedNames.insert(processedName);

        updateLocals();
#endif
    } else {
        // Bump requests to avoid model rebuilding during the nested
        // updateWatchModel runs.
        ++m_pendingWatchRequests;
        PENDING_DEBUG("UPDATE WATCH BUMPS PENDING UP TO " << m_pendingWatchRequests);
#if 1
        QMetaObject::invokeMethod(this, "updateWatchDataHelper",
            Qt::QueuedConnection, Q_ARG(WatchData, data));
#else
        updateWatchDataHelper(data);
#endif
    }
}

void GdbEngine::updateWatchDataHelper(const WatchData &data)
{
    //m_pendingRequests = 0;
    PENDING_DEBUG("UPDATE WATCH DATA");
    #if DEBUG_PENDING
    //qDebug() << "##############################################";
    qDebug() << "UPDATE MODEL, FOUND INCOMPLETE:";
    //qDebug() << data.toString();
    #endif

    updateSubItemClassic(data);
    //PENDING_DEBUG("INTERNAL TRIGGERING UPDATE WATCH MODEL");
    --m_pendingWatchRequests;
    PENDING_DEBUG("UPDATE WATCH DONE BUMPS PENDING DOWN TO " << m_pendingWatchRequests);
    if (m_pendingWatchRequests <= 0)
        rebuildWatchModel();
}

void GdbEngine::rebuildWatchModel()
{
    static int count = 0;
    ++count;
    if (!isSynchroneous())
        m_processedNames.clear();
    PENDING_DEBUG("REBUILDING MODEL" << count);
    if (theDebuggerBoolSetting(LogTimeStamps))
        showDebuggerInput(LogMisc, currentTime());
    showDebuggerInput(LogStatus, _("<Rebuild Watchmodel %1>").arg(count));
    showStatusMessage(tr("Finished retrieving data."), 400);
    manager()->watchHandler()->endCycle();
    showToolTip();
}

static QByteArray arrayFillCommand(const char *array, const QByteArray &params)
{
    char buf[50];
    sprintf(buf, "set {char[%d]} &%s = {", params.size(), array);
    QByteArray encoded;
    encoded.append(buf);
    const int size = params.size();
    for (int i = 0; i != size; ++i) {
        sprintf(buf, "%d,", int(params[i]));
        encoded.append(buf);
    }
    encoded[encoded.size() - 1] = '}';
    return encoded;
}

void GdbEngine::sendWatchParameters(const QByteArray &params0)
{
    QByteArray params = params0;
    params.append('\0');
    const QByteArray inBufferCmd = arrayFillCommand("qDumpInBuffer", params);

    params.replace('\0','!');
    showDebuggerInput(LogMisc, QString::fromUtf8(params));

    params.clear();
    params.append('\0');
    const QByteArray outBufferCmd = arrayFillCommand("qDumpOutBuffer", params);

    postCommand(inBufferCmd);
    postCommand(outBufferCmd);
}

void GdbEngine::handleVarAssign(const GdbResponse &)
{
    // Everything might have changed, force re-evaluation.
    // FIXME: Speed this up by re-using variables and only
    // marking values as 'unknown'
    setTokenBarrier();
    updateLocals();
}

// Find the "type" and "displayedtype" children of root and set up type.
void GdbEngine::setWatchDataType(WatchData &data, const GdbMi &item)
{
    if (item.isValid()) {
        const QString miData = _(item.data());
        if (!data.framekey.isEmpty())
            m_varToType[data.framekey] = miData;
        data.setType(miData);
    } else if (data.type.isEmpty()) {
        data.setTypeNeeded();
    }
}

void GdbEngine::setWatchDataDisplayedType(WatchData &data, const GdbMi &item)
{
    if (item.isValid())
        data.displayedType = _(item.data());
}

void GdbEngine::handleVarCreate(const GdbResponse &response)
{
    WatchData data = response.cookie.value<WatchData>();
    // happens e.g. when we already issued a var-evaluate command
    if (!data.isValid())
        return;
    //qDebug() << "HANDLE VARIABLE CREATION:" << data.toString();
    if (response.resultClass == GdbResultDone) {
        data.variable = data.iname;
        setWatchDataType(data, response.data.findChild("type"));
        if (manager()->watchHandler()->isExpandedIName(data.iname)
                && !response.data.findChild("children").isValid())
            data.setChildrenNeeded();
        else
            data.setChildrenUnneeded();
        setWatchDataChildCount(data, response.data.findChild("numchild"));
        insertData(data);
    } else {
        data.setError(QString::fromLocal8Bit(response.data.findChild("msg").data()));
        if (data.isWatcher()) {
            data.value = WatchData::msgNotInScope();
            data.type = _(" ");
            data.setAllUnneeded();
            data.setHasChildren(false);
            data.valueEnabled = false;
            data.valueEditable = false;
            insertData(data);
        }
    }
}

void GdbEngine::handleDebuggingHelperSetup(const GdbResponse &response)
{
    //qDebug() << "CUSTOM SETUP RESULT:" << response.toString();
    if (response.resultClass == GdbResultDone) {
    } else {
        QString msg = QString::fromLocal8Bit(response.data.findChild("msg").data());
        //qDebug() << "CUSTOM DUMPER SETUP ERROR MESSAGE:" << msg;
        showStatusMessage(tr("Custom dumper setup: %1").arg(msg), 10000);
    }
}

void GdbEngine::handleChildren(const WatchData &data0, const GdbMi &item,
    QList<WatchData> *list)
{
    //qDebug() << "HANDLE CHILDREN: " << data0.toString() << item.toString();
    WatchData data = data0;
    if (!manager()->watchHandler()->isExpandedIName(data.iname))
        data.setChildrenUnneeded();

    GdbMi children = item.findChild("children");
    if (children.isValid() || !manager()->watchHandler()->isExpandedIName(data.iname))
        data.setChildrenUnneeded();

    if (manager()->watchHandler()->isDisplayedIName(data.iname)) {
        GdbMi editvalue = item.findChild("editvalue");
        if (editvalue.isValid()) {
            setWatchDataEditValue(data, editvalue);
            manager()->watchHandler()->showEditValue(data);
        }
    }
    setWatchDataType(data, item.findChild("type"));
    setWatchDataEditValue(data, item.findChild("editvalue"));
    setWatchDataChildCount(data, item.findChild("numchild"));
    setWatchDataValue(data, item.findChild("value"),
        item.findChild("valueencoded").data().toInt());
    setWatchDataAddress(data, item.findChild("addr"));
    setWatchDataExpression(data, item.findChild("exp"));
    setWatchDataSAddress(data, item.findChild("saddr"));
    setWatchDataValueToolTip(data, item.findChild("valuetooltip"),
        item.findChild("valuetooltipencoded").data().toInt());
    setWatchDataValueEnabled(data, item.findChild("valueenabled"));
    setWatchDataValueEditable(data, item.findChild("valueeditable"));
    //qDebug() << "\nAPPEND TO LIST: " << data.toString() << "\n";
    list->append(data);

    bool ok = false;
    qulonglong addressBase = item.findChild("addrbase").data().toULongLong(&ok, 0);
    qulonglong addressStep = item.findChild("addrstep").data().toULongLong();

    // Try not to repeat data too often.
    WatchData childtemplate;
    setWatchDataType(childtemplate, item.findChild("childtype"));
    setWatchDataChildCount(childtemplate, item.findChild("childnumchild"));
    //qDebug() << "CHILD TEMPLATE:" << childtemplate.toString();

    int i = 0;
    foreach (const GdbMi &child, children.children()) {
        WatchData data1 = childtemplate;
        GdbMi name = child.findChild("name");
        if (name.isValid())
            data1.name = _(name.data());
        else
            data1.name = QString::number(i);
        GdbMi iname = child.findChild("iname");
        if (iname.isValid())
            data1.iname = iname.data();
        else
            data1.iname = data.iname + '.' + data1.name.toLatin1();
        if (!data1.name.isEmpty() && data1.name.at(0).isDigit())
            data1.name = _c('[') + data1.name + _c(']');
        if (addressStep) {
            const QByteArray addr = "0x" + QByteArray::number(addressBase, 16);
            setWatchDataAddressHelper(data1, addr);
            addressBase += addressStep;
        }
        QByteArray key = child.findChild("key").data();
        if (!key.isEmpty()) {
            int encoding = child.findChild("keyencoded").data().toInt();
            QString skey = decodeData(key, encoding);
            if (skey.size() > 13) {
                skey = skey.left(12);
                skey += _("...");
            }
            //data1.name += " (" + skey + ")";
            data1.name = skey;
        }
        handleChildren(data1, child, list);
        ++i;
    }
}

void GdbEngine::updateLocals(const QVariant &cookie)
{
    m_pendingWatchRequests = 0;
    m_pendingBreakpointRequests = 0;
    if (hasPython())
        updateLocalsPython(QByteArray());
    else
        updateLocalsClassic(cookie);
}


// Parse a local variable from GdbMi
WatchData GdbEngine::localVariable(const GdbMi &item,
                                   const QStringList &uninitializedVariables,
                                   QMap<QByteArray, int> *seen)
{
    // Local variables of inlined code are reported as
    // 26^done,locals={varobj={exp="this",value="",name="var4",exp="this",
    // numchild="1",type="const QtSharedPointer::Basic<CPlusPlus::..."}}
    // We do not want these at all. Current hypotheses is that those
    // "spurious" locals have _two_ "exp" field. Try to filter them:
    QByteArray name;
    if (m_isMacGdb) {
        int numExps = 0;
        foreach (const GdbMi &child, item.children())
            numExps += int(child.name() == "exp");
        if (numExps > 1)
            return WatchData();
        name = item.findChild("exp").data();
    } else {
        name = item.findChild("name").data();
    }
    const QMap<QByteArray, int>::iterator it  = seen->find(name);
    if (it != seen->end()) {
        const int n = it.value();
        ++(it.value());
        WatchData data;
        QString nam = _(name);
        data.iname = "local." + name + QByteArray::number(n + 1);
        //: Variable %1 is the variable name, %2 is a simple count
        data.name = WatchData::shadowedName(nam, n);
        if (uninitializedVariables.contains(data.name)) {
            data.setError(WatchData::msgNotInScope());
            return data;
        }
        //: Type of local variable or parameter shadowed by another
        //: variable of the same name in a nested block.
        setWatchDataValue(data, item.findChild("value"));
        data.setType(GdbEngine::tr("<shadowed>"));
        data.setHasChildren(false);
        return data;
    }
    seen->insert(name, 1);
    WatchData data;
    QString nam = _(name);
    data.iname = "local." + name;
    data.name = nam;
    data.exp = name;
    data.framekey = m_currentFrame + data.name;
    setWatchDataType(data, item.findChild("type"));
    if (uninitializedVariables.contains(data.name)) {
        data.setError(WatchData::msgNotInScope());
        return data;
    }
    if (isSynchroneous()) {
        setWatchDataValue(data, item.findChild("value"),
                          item.findChild("valueencoded").data().toInt());
        // We know that the complete list of children is
        // somewhere in the response.
        data.setChildrenUnneeded();
    } else {
        // set value only directly if it is simple enough, otherwise
        // pass through the insertData() machinery
        if (isIntOrFloatType(data.type) || isPointerType(data.type))
            setWatchDataValue(data, item.findChild("value"));
        if (isSymbianIntType(data.type)) {
            setWatchDataValue(data, item.findChild("value"));
            data.setHasChildren(false);
        }
    }

    if (!m_manager->watchHandler()->isExpandedIName(data.iname))
        data.setChildrenUnneeded();
    if (isPointerType(data.type) || data.name == __("this"))
        data.setHasChildren(true);
    return data;
}

void GdbEngine::insertData(const WatchData &data0)
{
    PENDING_DEBUG("INSERT DATA" << data0.toString());
    WatchData data = data0;
    if (data.value.startsWith(__("mi_cmd_var_create:"))) {
        qDebug() << "BOGUS VALUE:" << data.toString();
        return;
    }
    manager()->watchHandler()->insertData(data);
}

void GdbEngine::assignValueInDebugger(const QString &expression, const QString &value)
{
    postCommand("-var-delete assign");
    postCommand("-var-create assign * " + expression.toLatin1());
    postCommand("-var-assign assign " + value.toLatin1(),
        Discardable, CB(handleVarAssign));
}

QString GdbEngine::qtDumperLibraryName() const
{
    return m_manager->qtDumperLibraryName();
}

void GdbEngine::watchPoint(const QPoint &pnt)
{
    //qDebug() << "WATCH " << pnt;
    QByteArray x = QByteArray::number(pnt.x());
    QByteArray y = QByteArray::number(pnt.y());
    postCommand("call (void*)watchPoint(" + x + ',' + y + ')',
        NeedsStop, CB(handleWatchPoint));
}

void GdbEngine::handleWatchPoint(const GdbResponse &response)
{
    //qDebug() << "HANDLE WATCH POINT:" << response.toString();
    if (response.resultClass == GdbResultDone) {
        GdbMi contents = response.data.findChild("consolestreamoutput");
        // "$5 = (void *) 0xbfa7ebfc\n"
        QString str = _(parsePlainConsoleStream(response));
        // "(void *) 0xbfa7ebfc"
        QString addr = str.mid(9);
        QString ns = m_dumperHelper.qtNamespace();
        QString type = ns.isEmpty() ? _("QWidget*") : _("'%1QWidget'*").arg(ns);
        QString exp = _("(*(%1)%2)").arg(type).arg(addr);
        theDebuggerAction(WatchExpression)->trigger(exp);
    }
}


struct MemoryAgentCookie
{
    MemoryAgentCookie() : agent(0), token(0), address(0) {}
    MemoryAgentCookie(MemoryViewAgent *agent_, QObject *token_, quint64 address_)
        : agent(agent_), token(token_), address(address_)
    {}
    QPointer<MemoryViewAgent> agent;
    QPointer<QObject> token;
    quint64 address;
};

void GdbEngine::fetchMemory(MemoryViewAgent *agent, QObject *token, quint64 addr,
                            quint64 length)
{
    //qDebug() << "GDB MEMORY FETCH" << agent << addr << length;
    postCommand("-data-read-memory " + QByteArray::number(addr) + " x 1 1 "
            + QByteArray::number(length),
        NeedsStop, CB(handleFetchMemory),
        QVariant::fromValue(MemoryAgentCookie(agent, token, addr)));
}

void GdbEngine::handleFetchMemory(const GdbResponse &response)
{
    // ^done,addr="0x08910c88",nr-bytes="16",total-bytes="16",
    // next-row="0x08910c98",prev-row="0x08910c78",next-page="0x08910c98",
    // prev-page="0x08910c78",memory=[{addr="0x08910c88",
    // data=["1","0","0","0","5","0","0","0","0","0","0","0","0","0","0","0"]}]
    MemoryAgentCookie ac = response.cookie.value<MemoryAgentCookie>();
    QTC_ASSERT(ac.agent, return);
    QByteArray ba;
    GdbMi memory = response.data.findChild("memory");
    QTC_ASSERT(memory.children().size() <= 1, return);
    if (memory.children().isEmpty())
        return;
    GdbMi memory0 = memory.children().at(0); // we asked for only one 'row'
    GdbMi data = memory0.findChild("data");
    foreach (const GdbMi &child, data.children()) {
        bool ok = true;
        unsigned char c = '?';
        c = child.data().toUInt(&ok, 0);
        QTC_ASSERT(ok, return);
        ba.append(c);
    }
    ac.agent->addLazyData(ac.token, ac.address, ba);
}


struct DisassemblerAgentCookie
{
    DisassemblerAgentCookie() : agent(0) {}
    DisassemblerAgentCookie(DisassemblerViewAgent *agent_)
        : agent(agent_)
    {}
    QPointer<DisassemblerViewAgent> agent;
};


// FIXME: add agent->frame() accessor and use that
void GdbEngine::fetchDisassembler(DisassemblerViewAgent *agent)
{
    fetchDisassemblerByCli(agent, agent->isMixed());
/*
    if (agent->isMixed()) {
        // Disassemble full function:
        const StackFrame &frame = agent->frame();
        QByteArray cmd = "-data-disassemble"
            " -f " + frame.file.toLocal8Bit() +
            " -l " + QByteArray::number(frame.line) + " -n -1 -- 1";
        postCommand(cmd, Discardable, CB(handleFetchDisassemblerByLine),
            QVariant::fromValue(DisassemblerAgentCookie(agent)));
    } else {
        fetchDisassemblerByAddress(agent, true);
    }
*/
}

void GdbEngine::fetchDisassemblerByAddress(DisassemblerViewAgent *agent,
    bool useMixedMode)
{
    QTC_ASSERT(agent, return);
    bool ok = true;
    quint64 address = agent->address().toULongLong(&ok, 0);
    QTC_ASSERT(ok, qDebug() << "ADDRESS: " << agent->address() << address; return);
    QByteArray start = QByteArray::number(address - 20, 16);
    QByteArray end = QByteArray::number(address + 100, 16);
    // -data-disassemble [ -s start-addr -e end-addr ]
    //  | [ -f filename -l linenum [ -n lines ] ] -- mode
    if (useMixedMode)
        postCommand("-data-disassemble -s 0x" + start + " -e 0x" + end + " -- 1",
            Discardable, CB(handleFetchDisassemblerByAddress1),
            QVariant::fromValue(DisassemblerAgentCookie(agent)));
    else
        postCommand("-data-disassemble -s 0x" + start + " -e 0x" + end + " -- 0",
            Discardable, CB(handleFetchDisassemblerByAddress0),
            QVariant::fromValue(DisassemblerAgentCookie(agent)));
}

void GdbEngine::fetchDisassemblerByCli(DisassemblerViewAgent *agent,
    bool useMixedMode)
{
    QTC_ASSERT(agent, return);
    bool ok = false;
    quint64 address = agent->address().toULongLong(&ok, 0);
    QByteArray cmd = "disassemble ";
    if (useMixedMode && m_gdbVersion >= 60850)
        cmd += "/m ";
    cmd += " 0x";
    cmd += QByteArray::number(address, 16);
    postCommand(cmd, Discardable, CB(handleFetchDisassemblerByCli),
        QVariant::fromValue(DisassemblerAgentCookie(agent)));
}

void GdbEngine::fetchDisassemblerByAddressCli(DisassemblerViewAgent *agent)
{
    QTC_ASSERT(agent, return);
    bool ok = false;
    quint64 address = agent->address().toULongLong(&ok, 0);
    QByteArray start = QByteArray::number(address - 20, 16);
    QByteArray end = QByteArray::number(address + 100, 16);
    QByteArray cmd = "disassemble 0x" + start + " 0x" + end;
    postCommand(cmd, Discardable, CB(handleFetchDisassemblerByCli),
        QVariant::fromValue(DisassemblerAgentCookie(agent)));
}

static QByteArray parseLine(const GdbMi &line)
{
    QByteArray ba;
    ba.reserve(200);
    QByteArray address = line.findChild("address").data();
    //QByteArray funcName = line.findChild("func-name").data();
    //QByteArray offset = line.findChild("offset").data();
    QByteArray inst = line.findChild("inst").data();
    ba += address + QByteArray(15 - address.size(), ' ');
    //ba += funcName + "+" + offset + "  ";
    //ba += QByteArray(30 - funcName.size() - offset.size(), ' ');
    ba += inst;
    ba += '\n';
    return ba;
}

QString GdbEngine::parseDisassembler(const GdbMi &lines)
{
    // ^done,data={asm_insns=[src_and_asm_line={line="1243",file=".../app.cpp",
    // line_asm_insn=[{address="0x08054857",func-name="main",offset="27",
    // inst="call 0x80545b0 <_Z13testQFileInfov>"}]},
    // src_and_asm_line={line="1244",file=".../app.cpp",
    // line_asm_insn=[{address="0x0805485c",func-name="main",offset="32",
    //inst="call 0x804cba1 <_Z11testObject1v>"}]}]}
    // - or -
    // ^done,asm_insns=[
    // {address="0x0805acf8",func-name="...",offset="25",inst="and $0xe8,%al"},
    // {address="0x0805acfa",func-name="...",offset="27",inst="pop %esp"},

    QList<QByteArray> fileContents;
    bool fileLoaded = false;
    QByteArray ba;
    ba.reserve(200 * lines.children().size());

    // FIXME: Performance?
    foreach (const GdbMi &child, lines.children()) {
        if (child.hasName("src_and_asm_line")) {
            // mixed mode
            if (!fileLoaded) {
                QString fileName = QFile::decodeName(child.findChild("file").data());
                fileName = cleanupFullName(fileName);
                QFile file(fileName);
                file.open(QIODevice::ReadOnly);
                fileContents = file.readAll().split('\n');
                fileLoaded = true;
            }
            int line = child.findChild("line").data().toInt();
            if (line >= 1 && line <= fileContents.size())
                ba += "    " + fileContents.at(line - 1) + '\n';
            GdbMi insn = child.findChild("line_asm_insn");
            foreach (const GdbMi &line, insn.children())
                ba += parseLine(line);
        } else {
            // the non-mixed version
            ba += parseLine(child);
        }
    }
    return _(ba);
}

void GdbEngine::handleFetchDisassemblerByLine(const GdbResponse &response)
{
    DisassemblerAgentCookie ac = response.cookie.value<DisassemblerAgentCookie>();
    QTC_ASSERT(ac.agent, return);

    if (response.resultClass == GdbResultDone) {
        GdbMi lines = response.data.findChild("asm_insns");
        if (lines.children().isEmpty())
            fetchDisassemblerByAddress(ac.agent, true);
        else if (lines.children().size() == 1
                    && lines.childAt(0).findChild("line").data() == "0")
            fetchDisassemblerByAddress(ac.agent, true);
        else {
            QString contents = parseDisassembler(lines);
            if (ac.agent->contentsCoversAddress(contents)) {
                // All is well.
                ac.agent->setContents(contents);
            } else {
                // Can happen e.g. for initializer list on symbian/rvct where
                // we get a file name and line number but where the 'fully
                // disassembled function' does not cover the code in the
                // initializer list. Fall back needed:
                //fetchDisassemblerByAddress(ac.agent, true);
                fetchDisassemblerByCli(ac.agent, true);
            }
        }
    } else {
        // 536^error,msg="mi_cmd_disassemble: Invalid line number"
        QByteArray msg = response.data.findChild("msg").data();
        if (msg == "mi_cmd_disassemble: Invalid line number"
                || msg.startsWith("Cannot access memory at address"))
            fetchDisassemblerByAddress(ac.agent, true);
        else
            showStatusMessage(tr("Disassembler failed: %1")
                .arg(QString::fromLocal8Bit(msg)), 5000);
    }
}

void GdbEngine::handleFetchDisassemblerByAddress1(const GdbResponse &response)
{
    DisassemblerAgentCookie ac = response.cookie.value<DisassemblerAgentCookie>();
    QTC_ASSERT(ac.agent, return);

    if (response.resultClass == GdbResultDone) {
        GdbMi lines = response.data.findChild("asm_insns");
        if (lines.children().isEmpty())
            fetchDisassemblerByAddress(ac.agent, false);
        else {
            QString contents = parseDisassembler(lines);
            if (ac.agent->contentsCoversAddress(contents)) {
                ac.agent->setContents(parseDisassembler(lines));
            } else {
                debugMessage(_("FALL BACK TO NON-MIXED"));
                fetchDisassemblerByAddress(ac.agent, false);
            }
        }
    } else {
        // 26^error,msg="Cannot access memory at address 0x801ca308"
        QByteArray msg = response.data.findChild("msg").data();
        showStatusMessage(tr("Disassembler failed: %1")
            .arg(QString::fromLocal8Bit(msg)), 5000);
    }
}

void GdbEngine::handleFetchDisassemblerByAddress0(const GdbResponse &response)
{
    DisassemblerAgentCookie ac = response.cookie.value<DisassemblerAgentCookie>();
    QTC_ASSERT(ac.agent, return);

    if (response.resultClass == GdbResultDone) {
        GdbMi lines = response.data.findChild("asm_insns");
        ac.agent->setContents(parseDisassembler(lines));
    } else {
        QByteArray msg = response.data.findChild("msg").data();
        showStatusMessage(tr("Disassembler failed: %1")
            .arg(QString::fromLocal8Bit(msg)), 5000);
    }
}

void GdbEngine::handleFetchDisassemblerByCli(const GdbResponse &response)
{
    DisassemblerAgentCookie ac = response.cookie.value<DisassemblerAgentCookie>();
    QTC_ASSERT(ac.agent, return);

    if (response.resultClass == GdbResultDone) {
        const QString someSpace = _("        ");
        // First line is something like
        // "Dump of assembler code from 0xb7ff598f to 0xb7ff5a07:"
        GdbMi output = response.data.findChild("consolestreamoutput");
        QStringList res;
        QString out = QString::fromLatin1(output.data()).trimmed();
        foreach (const QString &line, out.split(_c('\n'))) {
            if (line.startsWith(_("Current language:")))
                continue;
            if (line.startsWith(_("The current source")))
                continue;
            if (line.startsWith(_("0x"))) {
                int pos1 = line.indexOf(_c('<'));
                int pos2 = line.indexOf(_c('+'), pos1);
                int pos3 = line.indexOf(_c('>'), pos2);
                if (pos3 >= 0) {
                    QString ba = _("  <+") + line.mid(pos2 + 1, pos3 - pos2 - 1);
                    res.append(line.left(pos1 - 1) + ba.rightJustified(4)
                        + _(">:            ") + line.mid(pos3 + 2));
                } else {
                    res.append(line);
                }
                continue;
            }
            res.append(someSpace + line);
        }
        // Drop "End of assembler dump." line.
        res.takeLast();
        if (res.size() > 1)
            ac.agent->setContents(res.join(_("\n")));
        else
            fetchDisassemblerByAddressCli(ac.agent);
    } else {
        QByteArray msg = response.data.findChild("msg").data();
        //76^error,msg="No function contains program counter for selected..."
        //76^error,msg="No function contains specified address."
        //>568^error,msg="Line number 0 out of range;
        if (msg.startsWith("No function ") || msg.startsWith("Line number "))
            fetchDisassemblerByAddressCli(ac.agent);
        else
            showStatusMessage(tr("Disassembler failed: %1")
                .arg(QString::fromLocal8Bit(msg)), 5000);
    }
}

void GdbEngine::gotoLocation(const StackFrame &frame, bool setMarker)
{
    // qDebug() << "GOTO " << frame << setMarker;
    m_manager->gotoLocation(frame, setMarker);
}

//
// Starting up & shutting down
//

bool GdbEngine::startGdb(const QStringList &args, const QString &gdb, const QString &settingsIdHint)
{
    debugMessage(_("STARTING GDB ") + gdb);

    m_gdbProc.disconnect(); // From any previous runs

    QString location = gdb;
    const QByteArray env = qgetenv("QTC_DEBUGGER_PATH");
    if (!env.isEmpty())
        location = QString::fromLatin1(env);
    if (location.isEmpty())
        location = theDebuggerStringSetting(GdbLocation);
    QStringList gdbArgs;
    gdbArgs << _("-i");
    gdbArgs << _("mi");
    gdbArgs += args;
#ifdef Q_OS_WIN
    // Set python path. By convention, python is located below gdb executable.
    const QFileInfo fi(location);
    if (fi.isAbsolute()) {
        const QString winPythonVersion = QLatin1String(winPythonVersionC);
        const QDir dir = fi.absoluteDir();
        if (dir.exists(winPythonVersion)) {
            QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
            const QString pythonPathVariable = QLatin1String("PYTHONPATH");
            // Check for existing values.
            if (environment.contains(pythonPathVariable)) {
                const QString oldPythonPath = environment.value(pythonPathVariable);
                manager()->showDebuggerOutput(LogMisc,
                    _("Using existing python path: %1").arg(oldPythonPath));
            } else {
                const QString pythonPath =
                    QDir::toNativeSeparators(dir.absoluteFilePath(winPythonVersion));
                environment.insert(pythonPathVariable, pythonPath);
                manager()->showDebuggerOutput(LogMisc,
                    _("Python path: %1").arg(pythonPath));
                m_gdbProc.setProcessEnvironment(environment);
            }
        }
    }
#endif

    connect(&m_gdbProc, SIGNAL(error(QProcess::ProcessError)),
        SLOT(handleGdbError(QProcess::ProcessError)));
    connect(&m_gdbProc, SIGNAL(finished(int, QProcess::ExitStatus)),
        SLOT(handleGdbFinished(int, QProcess::ExitStatus)));
    connect(&m_gdbProc, SIGNAL(readyReadStandardOutput()),
        SLOT(readGdbStandardOutput()));
    connect(&m_gdbProc, SIGNAL(readyReadStandardError()),
        SLOT(readGdbStandardError()));

    m_gdbProc.start(location, gdbArgs);

    if (!m_gdbProc.waitForStarted()) {
        const QString msg = tr("Unable to start gdb '%1': %2")
            .arg(location, m_gdbProc.errorString());
        handleAdapterStartFailed(msg, settingsIdHint);
        return false;
    }

    const QByteArray dumperSourcePath =
        Core::ICore::instance()->resourcePath().toLocal8Bit() + "/gdbmacros/";

    debugMessage(_("GDB STARTED, INITIALIZING IT"));
    m_commandTimer->setInterval(commandTimeoutTime());

    postCommand("show version", CB(handleShowVersion));

    //postCommand("-enable-timings");
    postCommand("set print static-members off"); // Seemingly doesn't work.
    //postCommand("set debug infrun 1");
    //postCommand("define hook-stop\n-thread-list-ids\n-stack-list-frames\nend");
    //postCommand("define hook-stop\nprint 4\nend");
    //postCommand("define hookpost-stop\nprint 5\nend");
    //postCommand("define hook-call\nprint 6\nend");
    //postCommand("define hookpost-call\nprint 7\nend");
    //postCommand("set print object on"); // works with CLI, but not MI
    //postCommand("set step-mode on");  // we can't work with that yes
    //postCommand("set exec-done-display on");
    //postCommand("set print pretty on");
    //postCommand("set confirm off");
    //postCommand("set pagination off");

    // The following does not work with 6.3.50-20050815 (Apple version gdb-1344)
    // (Mac OS 10.6), but does so for gdb-966 (10.5):
    //postCommand("set print inferior-events 1");

    postCommand("set breakpoint pending on");
    postCommand("set print elements 10000");
    // Produces a few messages during symtab loading
    //postCommand("set verbose on");

    //postCommand("set substitute-path /var/tmp/qt-x11-src-4.5.0 "
    //    "/home/sandbox/qtsdk-2009.01/qt");

    // one of the following is needed to prevent crashes in gdb on code like:
    //  template <class T> T foo() { return T(0); }
    //  int main() { return foo<int>(); }
    //  (gdb) call 'int foo<int>'()
    //  /build/buildd/gdb-6.8/gdb/valops.c:2069: internal-error:
    postCommand("set overload-resolution off");
    //postCommand(_("set demangle-style none"));
    // From the docs:
    //  Stop means reenter debugger if this signal happens (implies print).
    //  Print means print a message if this signal happens.
    //  Pass means let program see this signal;
    //  otherwise program doesn't know.
    //  Pass and Stop may be combined.
    // We need "print" as otherwise we will get no feedback whatsoever
    // when Custom DebuggingHelper crash (which happen regularly when accessing
    // uninitialized variables).
    postCommand("handle SIGSEGV nopass stop print");

    // This is useful to kill the inferior whenever gdb dies.
    //postCommand(_("handle SIGTERM pass nostop print"));

    postCommand("set unwindonsignal on");
    //postCommand("pwd");
    postCommand("set width 0");
    postCommand("set height 0");

    if (m_isMacGdb) {
        postCommand("-gdb-set inferior-auto-start-cfm off");
        postCommand("-gdb-set sharedLibrary load-rules "
            "dyld \".*libSystem.*\" all "
            "dyld \".*libauto.*\" all "
            "dyld \".*AppKit.*\" all "
            "dyld \".*PBGDBIntrospectionSupport.*\" all "
            "dyld \".*Foundation.*\" all "
            "dyld \".*CFDataFormatters.*\" all "
            "dyld \".*libobjc.*\" all "
            "dyld \".*CarbonDataFormatters.*\" all");
    }

    QString scriptFileName = theDebuggerStringSetting(GdbScriptFile);
    if (!scriptFileName.isEmpty()) {
        if (QFileInfo(scriptFileName).isReadable()) {
            postCommand("source " + scriptFileName.toLocal8Bit());
        } else {
            showMessageBox(QMessageBox::Warning,
            tr("Cannot find debugger initialization script"),
            tr("The debugger settings point to a script file at '%1' "
               "which is not accessible. If a script file is not needed, "
               "consider clearing that entry to avoid this warning. "
              ).arg(scriptFileName));
        }
    }

    postCommand("-interpreter-exec console "
            "\"python execfile('" + dumperSourcePath + "dumper.py')\"",
        NonCriticalResponse);
    postCommand("-interpreter-exec console "
            "\"python execfile('" + dumperSourcePath + "gdbmacros.py')\"",
        NonCriticalResponse);

    postCommand("-interpreter-exec console \"help bb\"",
        CB(handleHasPython));

    return true;
}

bool GdbEngine::checkDebuggingHelpers()
{
    return !hasPython() && checkDebuggingHelpersClassic();
}

void GdbEngine::handleGdbError(QProcess::ProcessError error)
{
    debugMessage(_("HANDLE GDB ERROR"));
    switch (error) {
    case QProcess::Crashed:
        break; // will get a processExited() as well
    // impossible case QProcess::FailedToStart:
    case QProcess::ReadError:
    case QProcess::WriteError:
    case QProcess::Timedout:
    default:
        m_gdbProc.kill();
        setState(EngineShuttingDown, true);
        showMessageBox(QMessageBox::Critical, tr("Gdb I/O Error"),
                       errorMessage(error));
        break;
    }
}

void GdbEngine::handleGdbFinished(int code, QProcess::ExitStatus type)
{
    debugMessage(_("GDB PROCESS FINISHED, status %1, code %2").arg(type).arg(code));
    if (!m_gdbAdapter)
        return;
    if (state() == EngineShuttingDown) {
        m_gdbAdapter->shutdown();
    } else if (state() != AdapterStartFailed) {
        QString msg = tr("The gdb process exited unexpectedly (%1).")
          .arg((type == QProcess::CrashExit)
              ? tr("crashed") : tr("code %1").arg(code));
        showMessageBox(QMessageBox::Critical, tr("Unexpected Gdb Exit"), msg);
        showStatusMessage(msg);
        m_gdbAdapter->shutdown();
    }
    initializeVariables();
    setState(DebuggerNotReady, true);
}

void GdbEngine::handleAdapterStartFailed(const QString &msg, const QString &settingsIdHint)
{
    setState(AdapterStartFailed);
    debugMessage(_("ADAPTER START FAILED"));
    if (!msg.isEmpty()) {
        const QString title = tr("Adapter start failed");
        if (settingsIdHint.isEmpty()) {
            Core::ICore::instance()->showWarningWithOptions(title, msg);
        } else {
            Core::ICore::instance()->showWarningWithOptions(title, msg, QString(),
                _(Debugger::Constants::DEBUGGER_SETTINGS_CATEGORY), settingsIdHint);
        }
    }
    shutdown();
}

void GdbEngine::handleAdapterStarted()
{
    setState(AdapterStarted);
    debugMessage(_("ADAPTER SUCCESSFULLY STARTED"));

    showStatusMessage(tr("Starting inferior..."));
    setState(InferiorStarting);
    m_gdbAdapter->startInferior();
}

void GdbEngine::handleInferiorPrepared()
{
    const QByteArray qtInstallPath = m_startParameters->qtInstallPath.toLocal8Bit();
    if (!qtInstallPath.isEmpty()) {
        QByteArray qtBuildPath;
#if defined(Q_OS_WIN)
        qtBuildPath = "C:/qt-greenhouse/Trolltech/Code_less_create_more/Trolltech/Code_less_create_more/Troll/4.6/qt";
        postCommand("set substitute-path " + qtBuildPath + ' ' + qtInstallPath);
        qtBuildPath = "C:/iwmake/build_mingw_opensource";
        postCommand("set substitute-path " + qtBuildPath + ' ' + qtInstallPath);
#elif defined(Q_OS_UNIX) && !defined (Q_OS_MAC)
        qtBuildPath = "/var/tmp/qt-src";
        postCommand("set substitute-path " + qtBuildPath + ' ' + qtInstallPath);
#endif
    }

    // Initial attempt to set breakpoints
    showStatusMessage(tr("Setting breakpoints..."));
    debugMessage(tr("Setting breakpoints..."));
    attemptBreakpointSynchronization();

    if (m_cookieForToken.isEmpty()) {
        startInferiorPhase2();
    } else {
        QTC_ASSERT(m_commandsDoneCallback == 0, /**/);
        m_commandsDoneCallback = &GdbEngine::startInferiorPhase2;
    }
}

void GdbEngine::startInferiorPhase2()
{
    debugMessage(_("BREAKPOINTS SET, CONTINUING INFERIOR STARTUP"));
    m_gdbAdapter->startInferiorPhase2();
}

void GdbEngine::handleInferiorStartFailed(const QString &msg)
{
    if (state() == AdapterStartFailed) {
        debugMessage(_("INFERIOR START FAILED, BUT ADAPTER DIED ALREADY"));
        return; // Adapter crashed meanwhile, so this notification is meaningless.
    }
    debugMessage(_("INFERIOR START FAILED"));
    showMessageBox(QMessageBox::Critical, tr("Inferior start failed"), msg);
    setState(InferiorStartFailed);
    shutdown();
}

void GdbEngine::handleAdapterCrashed(const QString &msg)
{
    debugMessage(_("ADAPTER CRASHED"));

    // The adapter is expected to have cleaned up after itself when we get here,
    // so the effect is about the same as AdapterStartFailed => use it.
    // Don't bother with state transitions - this can happen in any state and
    // the end result is always the same, so it makes little sense to find a
    // "path" which does not assert.
    setState(AdapterStartFailed, true);

    // No point in being friendly here ...
    m_gdbProc.kill();

    if (!msg.isEmpty())
        showMessageBox(QMessageBox::Critical, tr("Adapter crashed"), msg);
}

void GdbEngine::addOptionPages(QList<Core::IOptionsPage*> *opts) const
{
    opts->push_back(new GdbOptionsPage);
    opts->push_back(new TrkOptionsPage(m_trkOptions));
}

QMessageBox * GdbEngine::showMessageBox(int icon, const QString &title,
    const QString &text, int buttons)
{
    return m_manager->showMessageBox(icon, title, text, buttons);
}

void GdbEngine::setUseDebuggingHelpers(const QVariant &on)
{
    //qDebug() << "SWITCHING ON/OFF DUMPER DEBUGGING:" << on;
    Q_UNUSED(on)
    setTokenBarrier();
    updateLocals();
}

bool GdbEngine::hasPython() const
{
    return m_hasPython;
}

//
// Factory
//

IDebuggerEngine *createGdbEngine(DebuggerManager *manager)
{
    return new GdbEngine(manager);
}

} // namespace Internal
} // namespace Debugger

Q_DECLARE_METATYPE(Debugger::Internal::MemoryAgentCookie);
Q_DECLARE_METATYPE(Debugger::Internal::DisassemblerAgentCookie);
Q_DECLARE_METATYPE(Debugger::Internal::GdbMi);
