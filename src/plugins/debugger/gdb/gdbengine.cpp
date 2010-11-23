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
#include "debuggeruiswitcher.h"
#include "debuggermainwindow.h"
#include "debuggercore.h"
#include "debuggerplugin.h"
#include "debuggerrunner.h"

#include "attachgdbadapter.h"
#include "coregdbadapter.h"
#include "localplaingdbadapter.h"
#include "termgdbadapter.h"
#include "remotegdbserveradapter.h"
#include "remoteplaingdbadapter.h"
#include "trkgdbadapter.h"
#include "tcftrkgdbadapter.h"

#include "watchutils.h"
#include "debuggeractions.h"
#include "debuggeragents.h"
#include "debuggerconstants.h"
#include "debuggertooltip.h"
#include "debuggerstringutils.h"
#include "gdbmi.h"

#include "breakhandler.h"
#include "moduleshandler.h"
#include "registerhandler.h"
#include "snapshothandler.h"
#include "sourcefileshandler.h"
#include "stackhandler.h"
#include "threadshandler.h"
#include "watchhandler.h"

#include "sourcefileswindow.h"

#include "debuggerdialogs.h"
#include "logwindow.h"

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

QByteArray GdbEngine::tooltipIName(const QString &exp)
{
    return "tooltip." + exp.toLatin1().toHex();
}

static bool stateAcceptsGdbCommands(DebuggerState state)
{
    switch (state) {
    case EngineSetupRequested:
    case EngineSetupOk:
    case EngineSetupFailed:
    case InferiorUnrunnable:
    case InferiorSetupRequested:
    case InferiorSetupFailed:
    case EngineRunRequested:
    case InferiorRunRequested:
    case InferiorRunOk:
    case InferiorStopRequested:
    case InferiorStopOk:
    case InferiorShutdownRequested:
    case EngineShutdownRequested:
    case InferiorShutdownOk:
    case InferiorShutdownFailed:
        return true;
    case DebuggerNotReady:
    case InferiorStopFailed:
    case EngineRunFailed:
    case InferiorRunFailed:
    case EngineShutdownOk:
    case EngineShutdownFailed:
    case DebuggerFinished:
        return false;
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

GdbEngine::GdbEngine(const DebuggerStartParameters &startParameters)
  : DebuggerEngine(startParameters)
{
    setObjectName(QLatin1String("GdbEngine"));
    qRegisterMetaType<WatchData>("WatchData");

    m_busy = false;
    m_gdbAdapter = 0;
    m_debuggingHelperState = DebuggingHelperUninitialized;
    m_gdbVersion = 100;
    m_gdbBuildVersion = -1;
    m_isMacGdb = false;
    m_hasPython = false;
    m_registerNamesListed = false;
    m_hasInferiorThreadList = false;
    m_sourcesListUpdating = false;
    m_oldestAcceptableToken = -1;
    m_outputCodec = QTextCodec::codecForLocale();
    m_pendingWatchRequests = 0;
    m_pendingBreakpointRequests = 0;
    m_commandsDoneCallback = 0;
    invalidateSourcesList();

    m_gdbAdapter = createAdapter();

    m_commandTimer.setSingleShot(true);
    connect(&m_commandTimer, SIGNAL(timeout()), SLOT(commandTimeout()));

    connect(debuggerCore()->action(AutoDerefPointers), SIGNAL(valueChanged(QVariant)),
            SLOT(reloadLocals()));
    connect(debuggerCore()->action(SortStructMembers), SIGNAL(valueChanged(QVariant)),
            SLOT(reloadLocals()));
    connect(debuggerCore()->action(CreateFullBacktrace), SIGNAL(triggered()),
            SLOT(createFullBacktrace()));
}

DebuggerStartMode GdbEngine::startMode() const
{
    return startParameters().startMode;
}

AbstractGdbProcess *GdbEngine::gdbProc() const
{
    return m_gdbAdapter->gdbProc();
}

GdbEngine::~GdbEngine()
{
    // Prevent sending error messages afterwards.
    if (m_gdbAdapter)
        disconnect(gdbProc(), 0, this, 0);
    delete m_gdbAdapter;
    m_gdbAdapter = 0;
}

QString GdbEngine::errorMessage(QProcess::ProcessError error)
{
    switch (error) {
        case QProcess::FailedToStart:
            return tr("The Gdb process failed to start. Either the "
                "invoked program '%1' is missing, or you may have insufficient "
                "permissions to invoke the program.\n%2")
                .arg(m_gdb, gdbProc()->errorString());
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
    QString msg = m_outputCodec->toUnicode(data.constData(), data.length(),
        &m_outputCodecState);
    showMessage(msg, AppStuff);
}

void GdbEngine::handleResponse(const QByteArray &buff)
{
    showMessage(QString::fromLocal8Bit(buff, buff.length()), LogOutput);

#if 0
    static QTime lastTime;
    qDebug() // << "#### start response handling #### "
        << lastTime.msecsTo(QTime::currentTime()) << "ms,"
        << "buf:" << buff.left(1500) << "..."
        //<< "buf:" << buff
        << "size:" << buff.size();
    lastTime = QTime::currentTime();
#else
    //qDebug() << "buf:" << buff;
#endif
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
                    showStatusMessage(tr("Library %1 loaded").arg(_(id)), 1000);
                progressPing();
                invalidateSourcesList();
            } else if (asyncClass == "library-unloaded") {
                // Archer has 'id="/usr/lib/libdrm.so.2",
                // target-name="/usr/lib/libdrm.so.2",
                // host-name="/usr/lib/libdrm.so.2"
                QByteArray id = result.findChild("id").data();
                progressPing();
                showStatusMessage(tr("Library %1 unloaded").arg(_(id)), 1000);
                invalidateSourcesList();
            } else if (asyncClass == "thread-group-added") {
                // 7.1-symbianelf has "{id="i1"}"
            } else if (asyncClass == "thread-group-created"
                    || asyncClass == "thread-group-started") {
                // Archer had only "{id="28902"}" at some point of 6.8.x.
                // *-started seems to be standard in 7.1, but in early
                // 7.0.x, there was a *-created instead.
                progressPing();
                // 7.1.50 has thread-group-started,id="i1",pid="3529"
                QByteArray id = result.findChild("id").data();
                showStatusMessage(tr("Thread group %1 created").arg(_(id)), 1000);
                int pid = id.toInt();
                if (!pid) {
                    id = result.findChild("pid").data();
                    pid = id.toInt();
                }
                if (pid)
                    notifyInferiorPid(pid);
            } else if (asyncClass == "thread-created") {
                //"{id="1",group-id="28902"}"
                QByteArray id = result.findChild("id").data();
                showStatusMessage(tr("Thread %1 created").arg(_(id)), 1000);
            } else if (asyncClass == "thread-group-exited") {
                // Archer has "{id="28902"}"
                QByteArray id = result.findChild("id").data();
                showStatusMessage(tr("Thread group %1 exited").arg(_(id)), 1000);
            } else if (asyncClass == "thread-exited") {
                //"{id="1",group-id="28902"}"
                QByteArray id = result.findChild("id").data();
                QByteArray groupid = result.findChild("group-id").data();
                showStatusMessage(tr("Thread %1 in group %2 exited")
                    .arg(_(id)).arg(_(groupid)), 1000);
            } else if (asyncClass == "thread-selected") {
                QByteArray id = result.findChild("id").data();
                showStatusMessage(tr("Thread %1 selected").arg(_(id)), 1000);
                //"{id="2"}"
            } else if (m_isMacGdb && asyncClass == "shlibs-updated") {
                // Apple's gdb announces updated libs.
                invalidateSourcesList();
            } else if (m_isMacGdb && asyncClass == "shlibs-added") {
                // Apple's gdb announces added libs.
                // {shlib-info={num="2", name="libmathCommon.A_debug.dylib",
                // kind="-", dyld-addr="0x7f000", reason="dyld", requested-state="Y",
                // state="Y", path="/usr/lib/system/libmathCommon.A_debug.dylib",
                // description="/usr/lib/system/libmathCommon.A_debug.dylib",
                // loaded_addr="0x7f000", slide="0x7f000", prefix=""}}
                invalidateSourcesList();
            } else if (m_isMacGdb && asyncClass == "resolve-pending-breakpoint") {
                // Apple's gdb announces resolved breakpoints.
                // new_bp="1",pended_bp="1",new_expr="\"gdbengine.cpp\":1584",
                // bkpt={number="1",type="breakpoint",disp="keep",enabled="y",
                // addr="0x0000000115cc3ddf",func="foo()",file="../foo.cpp",
                // line="1584",shlib="/../libFoo_debug.dylib",times="0"}
                const GdbMi bkpt = result.findChild("bkpt");
                const int number = bkpt.findChild("number").data().toInt();
                BreakpointId id = breakHandler()->findBreakpointByNumber(number);
                updateBreakpointDataFromOutput(id, bkpt);
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
                progressPing();
                invalidateSourcesList();
            } else if (data.startsWith("[New ") || data.startsWith("[Thread ")) {
                if (data.endsWith('\n'))
                    data.chop(1);
                progressPing();
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
                showMessage(_(data.mid(9)), AppStuff); // cut "warning: "
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
                // FIXME: Handle this in the individual result handlers.
                //if (state() == InferiorStopOk) { // Result of manual command.
                //    resetLocation();
                //    setTokenBarrier();
                //    notifyInferiorRunRequested();
                //}
                //notifyInferiorRunOk();
                //showStatusMessage(tr("Running..."));
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
    QByteArray err = gdbProc()->readAllStandardError();
    showMessage(_("UNEXPECTED GDB STDERR: " + err));
    if (err == "Undefined command: \"bb\".  Try \"help\".\n")
        return;
    if (err.startsWith("BFD: reopening"))
        return;
    qWarning() << "Unexpected gdb stderr:" << err;
}

void GdbEngine::readGdbStandardOutput()
{
    m_commandTimer.start(); // Restart timer.

    int newstart = 0;
    int scan = m_inbuffer.size();

    m_inbuffer.append(gdbProc()->readAllStandardOutput());

    // This can trigger when a dialog starts a nested event loop.
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
#        if defined(Q_OS_WIN)
        if (m_inbuffer.at(end - 1) == '\r') {
            --end;
            if (end == start)
                continue;
        }
#        endif
        m_busy = true;
        handleResponse(QByteArray::fromRawData(m_inbuffer.constData() + start, end - start));
        m_busy = false;
    }
    m_inbuffer.clear();
}

void GdbEngine::interruptInferior()
{
    QTC_ASSERT(state() == InferiorStopRequested, qDebug() << state(); return);
    showStatusMessage(tr("Stop requested..."), 5000);
    showMessage(_("TRYING TO INTERRUPT INFERIOR"));
    m_gdbAdapter->interruptInferior();
}

void GdbEngine::interruptInferiorTemporarily()
{
    foreach (const GdbCommand &cmd, m_commandsToRunOnTemporaryBreak) {
        if (cmd.flags & LosesChild) {
            notifyInferiorIll();
            return;
        }
    }
    requestInterruptInferior();
}

void GdbEngine::maybeHandleInferiorPidChanged(const QString &pid0)
{
    const qint64 pid = pid0.toLongLong();
    if (pid == 0) {
        showMessage(_("Cannot parse PID from %1").arg(pid0));
        return;
    }
    if (pid == inferiorPid())
        return;

    showMessage(_("FOUND PID %1").arg(pid));
    notifyInferiorPid(pid);
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
        showMessage(_("NO GDB PROCESS RUNNING, CMD IGNORED: %1 %2")
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

    // FIXME: clean up logic below
    if (cmd.flags & Immediate) {
        // This should always be sent.
        flushCommand(cmd);
    } else if ((cmd.flags & NeedsStop)
            || !m_commandsToRunOnTemporaryBreak.isEmpty()) {
        if (state() == InferiorStopOk || state() == InferiorUnrunnable
            || state() == InferiorSetupRequested || state() == EngineSetupOk
            || state() == InferiorShutdownRequested) {
            // Can be safely sent now.
            flushCommand(cmd);
        } else {
            // Queue the commands that we cannot send at once.
            showMessage(_("QUEUING COMMAND " + cmd.command));
            m_commandsToRunOnTemporaryBreak.append(cmd);
            if (state() == InferiorStopRequested) {
                if (cmd.flags & LosesChild) {
                    notifyInferiorIll();
                }
                showMessage(_("CHILD ALREADY BEING INTERRUPTED. STILL HOPING."));
                // Calling shutdown() here breaks all situations where two
                // NeedsStop commands are issued in quick succession.
            } else if (state() == InferiorRunOk) {
                showStatusMessage(tr("Stopping temporarily"), 1000);
                interruptInferiorTemporarily();
            } else {
                qDebug() << "ATTEMPTING TO QUEUE COMMAND "
                    << cmd.command << "IN INAPPROPRIATE STATE" << state();
            }
        }
    } else if (!cmd.command.isEmpty()) {
        flushCommand(cmd);
    }
}

void GdbEngine::flushQueuedCommands()
{
    showStatusMessage(tr("Processing queued commands"), 1000);
    while (!m_commandsToRunOnTemporaryBreak.isEmpty()) {
        GdbCommand cmd = m_commandsToRunOnTemporaryBreak.takeFirst();
        showMessage(_("RUNNING QUEUED COMMAND " + cmd.command + ' '
            + cmd.callbackName));
        flushCommand(cmd);
    }
}

void GdbEngine::flushCommand(const GdbCommand &cmd0)
{
    if (!stateAcceptsGdbCommands(state())) {
        showMessage(_(cmd0.command), LogInput);
        showMessage(_("GDB PROCESS ACCEPTS NO CMD IN STATE %1 ").arg(state()));
        return;
    }

    QTC_ASSERT(gdbProc()->state() == QProcess::Running, return;)

    ++currentToken();
    GdbCommand cmd = cmd0;
    cmd.postTime = QTime::currentTime();
    m_cookieForToken[currentToken()] = cmd;
    if (cmd.flags & ConsoleCommand)
        cmd.command = "-interpreter-exec console \"" + cmd.command + '"';
    cmd.command = QByteArray::number(currentToken()) + cmd.command;
    showMessage(_(cmd.command), LogInput);

    m_gdbAdapter->write(cmd.command + "\r\n");

    // Start Watchdog.
    if (m_commandTimer.interval() <= 20000)
        m_commandTimer.setInterval(commandTimeoutTime());
    // The process can die for external reason between the "-gdb-exit" was
    // sent and a response could be retrieved. We don't want the watchdog
    // to bark in that case since the only possible outcome is a dead
    // process anyway.
    if (!cmd.command.endsWith("-gdb-exit"))
        m_commandTimer.start();

    //if (cmd.flags & LosesChild)
    //    notifyInferiorIll();
}

int GdbEngine::commandTimeoutTime() const
{
    int time = debuggerCore()->action(GdbWatchdogTimeout)->value().toInt();
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
        msg += cmd.callbackName ? cmd.callbackName : "<unnamed callback>";
        showMessage(_(msg));
    }
    if (killIt) {
        showMessage(_("TIMED OUT WAITING FOR GDB REPLY. COMMANDS STILL IN PROGRESS:"));
        int timeOut = m_commandTimer.interval();
        //m_commandTimer.stop();
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
            showMessage(_("KILLING DEBUGGER AS REQUESTED BY USER"));
            // This is an undefined state, so we just pull the emergency brake.
            watchHandler()->endCycle();
            gdbProc()->kill();
        } else {
            showMessage(_("CONTINUE DEBUGGER AS REQUESTED BY USER"));
        }
    } else {
        showMessage(_("\nNON-CRITICAL TIMEOUT\n"));
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
        showMessage(_("COOKIE FOR TOKEN %1 ALREADY EATEN. "
            "TWO RESPONSES FOR ONE COMMAND?").arg(token));
        if (response->resultClass == GdbResultError) {
            QByteArray msg = response->data.findChild("msg").data();
            if (msg == "Cannot find new threads: generic error") {
                // Handle a case known to occur on Linux/gdb 6.8 when debugging moc
                // with helpers enabled. In this case we get a second response with
                // msg="Cannot find new threads: generic error"
                showMessage(_("APPLYING WORKAROUND #1"));
                showMessageBox(QMessageBox::Critical,
                    tr("Executable failed"), QString::fromLocal8Bit(msg));
                showStatusMessage(tr("Process failed to start"));
                //shutdown();
                notifyInferiorIll();
            } else if (msg == "\"finish\" not meaningful in the outermost frame.") {
                // Handle a case known to appear on gdb 6.4 symbianelf when
                // the stack is cut due to access to protected memory.
                //showMessage(_("APPLYING WORKAROUND #2"));
                notifyInferiorStopOk();
            } else if (msg.startsWith("Cannot find bounds of current function")) {
                // Happens when running "-exec-next" in a function for which
                // there is no debug information. Divert to "-exec-next-step"
                showMessage(_("APPLYING WORKAROUND #3"));
                notifyInferiorStopOk();
                executeNextI();
            } else if (msg.startsWith("Couldn't get registers: No such process.")) {
                // Happens on archer-tromey-python 6.8.50.20090910-cvs
                // There might to be a race between a process shutting down
                // and library load messages.
                showMessage(_("APPLYING WORKAROUND #4"));
                notifyInferiorStopOk();
                //notifyInferiorIll();
                //showStatusMessage(tr("Executable failed: %1")
                //    .arg(QString::fromLocal8Bit(msg)));
                //shutdown();
                //showMessageBox(QMessageBox::Critical,
                //    tr("Executable failed"), QString::fromLocal8Bit(msg));
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
    if (debuggerCore()->boolSetting(LogTimeStamps)) {
        showMessage(_("Response time: %1: %2 s")
            .arg(_(cmd.command))
            .arg(cmd.postTime.msecsTo(QTime::currentTime()) / 1000.),
            LogTime);
    }

    if (response->token < m_oldestAcceptableToken && (cmd.flags & Discardable)) {
        //showMessage(_("### SKIPPING OLD RESULT") + response.toString());
        return;
    }

    response->cookie = cmd.cookie;

    bool isExpectedResult =
           (response->resultClass == GdbResultError) // Can always happen.
        || (response->resultClass == GdbResultRunning && (cmd.flags & RunRequest))
        || (response->resultClass == GdbResultExit && (cmd.flags & ExitRequest))
        || (response->resultClass == GdbResultDone);
        // GdbResultDone can almost "always" happen. Known examples are:
        //  (response->resultClass == GdbResultDone && cmd.command == "continue")
        // Happens with some incarnations of gdb 6.8 for "jump to line"
        //  (response->resultClass == GdbResultDone && cmd.command.startsWith("jump"))
        //  (response->resultClass == GdbResultDone && cmd.command.startsWith("detach"))
        // Happens when stepping finishes very quickly and issues *stopped/^done
        // instead of ^running/*stopped
        //  (response->resultClass == GdbResultDone && (cmd.flags & RunRequest));

    if (!isExpectedResult) {
#ifdef Q_OS_WIN
        // Ignore spurious 'running' responses to 'attach'
        const bool warning = !((startParameters().startMode == AttachExternal
                               || startParameters().useTerminal)
                               && cmd.command.startsWith("attach"));
#else
        const bool warning = true;
#endif
        if (warning) {
            QByteArray rsp = GdbResponse::stringFromResultClass(response->resultClass);
            rsp = "UNEXPECTED RESPONSE '" + rsp + "' TO COMMAND '" + cmd.command + "'";
            qWarning() << rsp << " AT " __FILE__ ":" STRINGIFY(__LINE__);
            showMessage(_(rsp));
        }
    }

    if (cmd.callback)
        (this->*cmd.callback)(*response);
    else if (cmd.adapterCallback)
        (m_gdbAdapter->*cmd.adapterCallback)(*response);

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
    if (state() == InferiorRunOk && !m_commandsToRunOnTemporaryBreak.isEmpty())
        interruptInferiorTemporarily();

    // Continue only if there are no commands wire anymore, so this will
    // be fully synchronous.
    // This is somewhat inefficient, as it makes the last command synchronous.
    // An optimization would be requesting the continue immediately when the
    // event loop is entered, and let individual commands have a flag to suppress
    // that behavior.
    if (m_commandsDoneCallback && m_cookieForToken.isEmpty()) {
        showMessage(_("ALL COMMANDS DONE; INVOKING CALLBACK"));
        CommandsDoneCallback cont = m_commandsDoneCallback;
        m_commandsDoneCallback = 0;
        (this->*cont)();
    } else {
        PENDING_DEBUG("MISSING TOKENS: " << m_cookieForToken.keys());
    }

    if (m_cookieForToken.isEmpty())
        m_commandTimer.stop();
}

void GdbEngine::executeDebuggerCommand(const QString &command)
{
    if (state() == DebuggerNotReady) {
        showMessage(_("GDB PROCESS NOT RUNNING, PLAIN CMD IGNORED: ") + command);
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
            sourceFilesHandler()->setSourceFiles(m_shortToFullName);
    }
}

void GdbEngine::handleExecuteJumpToLine(const GdbResponse &response)
{
    if (response.resultClass == GdbResultRunning) {
        notifyInferiorRunOk();
        // All is fine. Waiting for the temporary breakpoint to be hit.
    } else if (response.resultClass == GdbResultDone) {
        // This happens on old gdb. Trigger the effect of a '*stopped'.
        showStatusMessage(tr("Jumped. Stopped"));
        notifyInferiorSpontaneousStop();
        handleStop1(response);
    }
}

void GdbEngine::handleExecuteRunToLine(const GdbResponse &response)
{
    if (response.resultClass == GdbResultRunning) {
        notifyInferiorRunOk();
        // All is fine. Waiting for the temporary breakpoint to be hit.
    } else if (response.resultClass == GdbResultDone) {
        // This happens on old gdb. Trigger the effect of a '*stopped'.
        // >&"continue\n"
        // >~"Continuing.\n"
        //>~"testArray () at ../simple/app.cpp:241\n"
        //>~"241\t    s[1] = \"b\";\n"
        //>122^done
        gotoLocation(m_targetFrame, true);
        showStatusMessage(tr("Target line hit. Stopped"));
        notifyInferiorSpontaneousStop();
        handleStop1(response);
    }
}

/*
void GdbEngine::handleExecuteRunToFunction(const GdbResponse &response)
{
    // FIXME: remove this special case as soon as there's a real
    // reason given when the temporary breakpoint is hit.
    // reight now we get:
    // 14*stopped,thread-id="1",frame={addr="0x0000000000403ce4",
    // func="foo",args=[{name="str",value="@0x7fff0f450460"}],
    // file="main.cpp",fullname="/tmp/g/main.cpp",line="37"}
    QTC_ASSERT(state() == InferiorStopRequested, qDebug() << state())
    notifyInferiorStopOk();
    showStatusMessage(tr("Function reached. Stopped"));
    GdbMi frame = response.data.findChild("frame");
    StackFrame f = parseStackFrame(frame, 0);
    gotoLocation(f, true);
}
*/

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
    //if (debuggerCore()->boolSetting(EnableReverseDebugging))
    //    postCommand("target response");

    tryLoadDebuggingHelpers();

#    ifndef Q_OS_MAC
    // intentionally after tryLoadDebuggingHelpers(),
    // otherwise we'd interrupt solib loading.
    if (debuggerCore()->boolSetting(AllPluginBreakpoints)) {
        postCommand("set auto-solib-add on");
        postCommand("set stop-on-solib-events 0");
        postCommand("sharedlibrary .*");
    } else if (debuggerCore()->boolSetting(SelectedPluginBreakpoints)) {
        postCommand("set auto-solib-add on");
        postCommand("set stop-on-solib-events 1");
        postCommand("sharedlibrary "
          + theDebuggerStringSetting(SelectedPluginBreakpointsPattern));
    } else if (debuggerCore()->boolSetting(NoPluginBreakpoints)) {
        // should be like that already
        postCommand("set auto-solib-add off");
        postCommand("set stop-on-solib-events 0");
    }
#    endif

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
    if (state() == InferiorSetupRequested)
        return;

    if (isDying()) {
        notifyInferiorStopOk();
        return;
    }

    const QByteArray reason = data.findChild("reason").data();

    if (isExitedReason(reason)) {
        //   // The user triggered a stop, but meanwhile the app simply exited ...
        //    QTC_ASSERT(state() == InferiorStopRequested
        //            /*|| state() == InferiorStopRequested_Kill*/,
        //               qDebug() << state());
        QString msg;
        if (reason == "exited") {
            msg = tr("Application exited with exit code %1")
                .arg(_(data.findChild("exit-code").toString()));
        } else if (reason == "exited-signalled" || reason == "signal-received") {
            msg = tr("Application exited after receiving signal %1")
                .arg(_(data.findChild("signal-name").toString()));
        } else {
            msg = tr("Application exited normally");
        }
        showStatusMessage(msg);
        notifyInferiorExited();
        return;
    }

    const int bkptno = data.findChild("bkptno").data().toInt();
    const GdbMi frame = data.findChild("frame");

    const int lineNumber = frame.findChild("line").data().toInt();
    QString fullName = QString::fromUtf8(frame.findChild("fullname").data());
    if (fullName.isEmpty())
        fullName = QString::fromUtf8(frame.findChild("file").data());

    if (bkptno && frame.isValid()) {
        // Use opportunity to update the breakpoint marker position.
        BreakHandler *handler = breakHandler();
        BreakpointId id = handler->findBreakpointByNumber(bkptno);
        const BreakpointResponse &response = handler->response(id);
        QString fileName = response.fileName;
        if (fileName.isEmpty())
            fileName = handler->fileName(id);
        if (fileName.isEmpty())
            fileName = fullName;
        if (!fileName.isEmpty())
            handler->setMarkerFileAndLine(id, fileName, lineNumber);
    }

    // Quickly set the location marker.
    if (lineNumber && QFileInfo(fullName).exists())
        debuggerCore()->gotoLocation(fullName, lineNumber, true);

    if (!m_commandsToRunOnTemporaryBreak.isEmpty()) {
        QTC_ASSERT(state() == InferiorStopRequested, qDebug() << state())
        notifyInferiorStopOk();
        flushQueuedCommands();
        if (state() == InferiorStopOk) {
            QTC_ASSERT(m_commandsDoneCallback == 0, /**/);
            m_commandsDoneCallback = &GdbEngine::autoContinueInferior;
        } else {
            QTC_ASSERT(state() == InferiorShutdownRequested, qDebug() << state())
        }
        return;
    }

    if (state() == InferiorRunOk) {
        // Stop triggered by a breakpoint or otherwise not directly
        // initiated by the user.
        notifyInferiorSpontaneousStop();
    } else if (state() == InferiorRunRequested) {
        // Stop triggered by something like "-exec-step\n"
        // "&"Cannot access memory at address 0xbfffedd4\n"
        // In this case a proper response 94^error,msg="" will follow and
        // be handled in the result handler.
        // -- or --
        // *stopped arriving earlier than ^done response to an -exec-step
        notifyInferiorRunOk();
        notifyInferiorSpontaneousStop();
    } else {
        QTC_ASSERT(state() == InferiorStopRequested, qDebug() << state());
        notifyInferiorStopOk();
    }

    // FIXME: Replace the #ifdef by the "target" architecture.
    // FIXME: Is this needed at all anymore?
#ifdef Q_OS_LINUX
    if (!m_entryPoint.isEmpty()) {
        if (frame.findChild("addr").data() == m_entryPoint) {
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

    handleStop0(data);
}

void GdbEngine::handleStop0(const GdbMi &data)
{
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
            if (debuggerCore()->boolSetting(SelectedPluginBreakpoints)) {
                QString dataStr = _(data.toString());
                showMessage(_("SHARED LIBRARY EVENT: ") + dataStr);
                QString pat = theDebuggerStringSetting(SelectedPluginBreakpointsPattern);
                showMessage(_("PATTERN: ") + pat);
                postCommand("sharedlibrary " + pat.toLocal8Bit());
                showStatusMessage(tr("Loading %1...").arg(dataStr));
            }
#endif
            continueInferiorInternal();
            return;
        }
    }
#endif

    const GdbMi frame = data.findChild("frame");
    const QByteArray reason = data.findChild("reason").data();

    // This was seen on XP after removing a breakpoint while running
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

    // Jump over well-known frames.
    static int stepCounter = 0;
    if (debuggerCore()->boolSetting(SkipKnownFrames)) {
        if (reason == "end-stepping-range" || reason == "function-finished") {
            //showMessage(frame.toString());
            QString funcName = _(frame.findChild("func").data());
            QString fileName = QString::fromLocal8Bit(frame.findChild("file").data());
            if (isLeavableFunction(funcName, fileName)) {
                //showMessage(_("LEAVING ") + funcName);
                ++stepCounter;
                executeStepOut();
                return;
            }
            if (isSkippableFunction(funcName, fileName)) {
                //showMessage(_("SKIPPING ") + funcName);
                ++stepCounter;
                executeStep();
                return;
            }
            //if (stepCounter)
            //    qDebug() << "STEPCOUNTER:" << stepCounter;
            stepCounter = 0;
        }
    }

    // Show return value if possible, usually with reason "function-finished".
    // *stopped,reason="function-finished",frame={addr="0x080556da",
    // func="testReturnValue",args=[],file="/../app.cpp",
    // fullname="/../app.cpp",line="1611"},gdb-result-var="$1",
    // return-value="{d = 0x808d998}",thread-id="1",stopped-threads="all",
    // core="1"
    GdbMi resultVar = data.findChild("gdb-result-var");
    if (resultVar.isValid())
        m_resultVarName = resultVar.data();
    else
        m_resultVarName.clear();

    bool initHelpers = m_debuggingHelperState == DebuggingHelperUninitialized
                       || m_debuggingHelperState == DebuggingHelperLoadTried;
    // Don't load helpers on stops triggered by signals unless it's
    // an intentional trap.
    if (initHelpers
            && m_gdbAdapter->dumperHandling() != AbstractGdbAdapter::DumperLoadedByGdbPreload
            && reason == "signal-received") {
        QByteArray name = data.findChild("signal-name").data();
        if (name != STOP_SIGNAL
            && (startParameters().startMode != AttachToRemote
                || name != CROSS_STOP_SIGNAL))
            initHelpers = false;
    }
    if (isSynchronous())
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
    if (isDying()) {
        qDebug() << "HANDLING STOP WHILE DYING";
        notifyInferiorStopOk();
        return;
    }

    QByteArray reason = data.findChild("reason").data();

#ifdef Q_OS_WIN
    if (startParameters().useTerminal && reason == "signal-received"
        && data.findChild("signal-name").data() == "SIGTRAP") {
        // Command line start up trap
        showMessage(_("INTERNAL CONTINUE"), LogMisc);
        continueInferiorInternal();
        return;
    }
#endif

    // This is for display only.
    if (m_modulesListOutdated)
        reloadModulesInternal();

    if (m_breakListOutdated) {
        reloadBreakListInternal();
    } else {
        // Older gdb versions do not produce "library loaded" messages
        // so the breakpoint update is not triggered.
        if (m_gdbVersion < 70000 && !m_isMacGdb && breakHandler()->size() > 0)
            reloadBreakListInternal();
    }

    if (reason == "watchpoint-trigger") {
        // *stopped,reason="watchpoint-trigger",wpt={number="2",exp="*0xbfffed40"},
        // value={old="1",new="0"},frame={addr="0x00451e1b",
        // func="QScopedPointer",args=[{name="this",value="0xbfffed40"},
        // {name="p",value="0x0"}],file="x.h",fullname="/home/.../x.h",line="95"},
        // thread-id="1",stopped-threads="all",core="2"
        GdbMi wpt = data.findChild("wpt");
        const int bpNumber = wpt.findChild("number").data().toInt();
        const BreakpointId id = breakHandler()->findBreakpointByNumber(bpNumber);
        const quint64 bpAddress = wpt.findChild("exp").data().toULongLong(0, 0);
        showStatusMessage(msgWatchpointTriggered(id, bpNumber, bpAddress));
    } else if (reason == "breakpoint-hit") {
        GdbMi gNumber = data.findChild("bkptno"); // 'number' or 'bkptno'?
        if (!gNumber.isValid())
            gNumber = data.findChild("number");
        const int bpNumber = gNumber.data().toInt();
        const QByteArray threadId = data.findChild("thread-id").data();
        const BreakpointId id = breakHandler()->findBreakpointByNumber(bpNumber);
        showStatusMessage(msgBreakpointTriggered(id, bpNumber, QString::fromAscii(threadId)));
    } else {
        QString reasontr = msgStopped(_(reason));
        if (reason == "signal-received"
            && debuggerCore()->boolSetting(UseMessageBoxForSignals)) {
            QByteArray name = data.findChild("signal-name").data();
            QByteArray meaning = data.findChild("signal-meaning").data();
            // Ignore these as they are showing up regularly when
            // stopping debugging.
            if (name != STOP_SIGNAL
                && (startParameters().startMode != AttachToRemote
                    || name != CROSS_STOP_SIGNAL)) {
                showStoppedBySignalMessageBox(_(meaning), _(name));
                if (!name.isEmpty() && !meaning.isEmpty())
                    reasontr = msgStoppedBySignal(_(meaning), _(name));
            }
        }

        if (reason.isEmpty())
            showStatusMessage(msgStopped());
        else
            showStatusMessage(reasontr);
    }

    //
    // Stack
    //
    reloadStack(false);

    if (supportsThreads()) {
        int currentId = data.findChild("thread-id").data().toInt();
        if (m_gdbAdapter->isTrkAdapter()) {
            m_gdbAdapter->trkReloadThreads();
        } else if (m_isMacGdb) {
            postCommand("-thread-list-ids", Discardable,
                CB(handleThreadListIds), currentId);
        } else {
            // This is only available in gdb 7.1+.
            postCommand("-thread-info", Discardable,
                CB(handleThreadInfo), currentId);
        }
    }

    //
    // Registers
    //
    reloadRegisters();
}

void GdbEngine::handleInfoProc(const GdbResponse &response)
{
    if (response.resultClass == GdbResultDone) {
        static QRegExp re(_("\\bprocess ([0-9]+)\n"));
        QTC_ASSERT(re.isValid(), return);
        if (re.indexIn(_(response.data.findChild("consolestreamoutput").data())) != -1)
            maybeHandleInferiorPidChanged(re.cap(1));
    }
}

void GdbEngine::handleShowVersion(const GdbResponse &response)
{
    showMessage(_("PARSING VERSION: " + response.toString()));
    if (response.resultClass == GdbResultDone) {
        m_gdbVersion = 100;
        m_gdbBuildVersion = -1;
        m_isMacGdb = false;
        GdbMi version = response.data.findChild("consolestreamoutput");
        QString msg = QString::fromLocal8Bit(version.data());
        extractGdbVersion(msg,
              &m_gdbVersion, &m_gdbBuildVersion, &m_isMacGdb);
        if (m_gdbVersion > 60500 && m_gdbVersion < 200000)
            showMessage(_("SUPPORTED GDB VERSION ") + msg);
        else
            showMessage(_("UNSUPPORTED GDB VERSION ") + msg);

        showMessage(_("USING GDB VERSION: %1, BUILD: %2%3").arg(m_gdbVersion)
            .arg(m_gdbBuildVersion).arg(_(m_isMacGdb ? " (APPLE)" : "")));
    }
}

void GdbEngine::handleHasPython(const GdbResponse &response)
{
    if (response.resultClass == GdbResultDone) {
        m_hasPython = true;
        GdbMi contents = response.data.findChild("consolestreamoutput");
        GdbMi data;
        data.fromStringMultiple(contents.data());
        const GdbMi dumpers = data.findChild("dumpers");
        foreach (const GdbMi &dumper, dumpers.children()) {
            QByteArray type = dumper.findChild("type").data();
            QStringList formats(tr("Raw structure"));
            QString reported = _(dumper.findChild("formats").data());
            formats.append(reported.split(_(","), QString::SkipEmptyParts));
            watchHandler()->addTypeFormats(type, formats);
        }
        const GdbMi hasInferiorThreadList = data.findChild("hasInferiorThreadList");
        m_hasInferiorThreadList = (hasInferiorThreadList.data().toInt() != 0);
    } else {
        pythonDumpersFailed();
    }
}

void GdbEngine::pythonDumpersFailed()
{
    m_hasPython = false;
    if (m_gdbAdapter->dumperHandling()
                == AbstractGdbAdapter::DumperLoadedByGdbPreload
            && checkDebuggingHelpersClassic()) {
#ifdef Q_OS_MAC
        const char * const LD_PRELOAD_ENV_VAR = "DYLD_INSERT_LIBRARIES";
#else
        const char * const LD_PRELOAD_ENV_VAR = "LD_PRELOAD";
#endif
        QByteArray cmd = "set environment ";
        cmd += LD_PRELOAD_ENV_VAR;
        cmd += ' ';
        cmd += startParameters().startMode == StartRemoteGdb
           ? startParameters().remoteDumperLib
           : qtDumperLibraryName().toLocal8Bit();
        postCommand(cmd);
        m_debuggingHelperState = DebuggingHelperLoadTried;
    }
}

void GdbEngine::showExecutionError(const QString &message)
{
    showMessageBox(QMessageBox::Critical, tr("Execution Error"),
       tr("Cannot continue debugged process:\n") + message);
}

void GdbEngine::handleExecuteContinue(const GdbResponse &response)
{
    QTC_ASSERT(state() == InferiorRunRequested, qDebug() << state());
    if (response.resultClass == GdbResultRunning) {
        notifyInferiorRunOk();
        return;
    }
    QByteArray msg = response.data.findChild("msg").data();
    if (msg.startsWith("Cannot find bounds of current function")
        || msg.startsWith("\"finish\" not meaningful in the outermost frame")) {
        notifyInferiorRunFailed();
        if (isDying())
            return;
        if (!m_commandsToRunOnTemporaryBreak.isEmpty())
            flushQueuedCommands();
        QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
        showStatusMessage(tr("Stopped."), 5000);
        reloadStack(true);
        //showStatusMessage(tr("No debug information available. "
        //  "Leaving function..."));
        //executeStepOut();
    } else {
        showExecutionError(QString::fromLocal8Bit(msg));
        notifyInferiorIll();
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

QString GdbEngine::cleanupFullName(const QString &fileName)
{
    QString cleanFilePath = fileName;
#ifdef Q_OS_WIN
    QTC_ASSERT(!fileName.isEmpty(), return QString())
    // Gdb on windows often delivers "fullnames" which
    // a) have no drive letter and b) are not normalized.
    QFileInfo fi(fileName);
    if (fi.isReadable())
        cleanFilePath = QDir::cleanPath(fi.absoluteFilePath());
#endif
    if (startMode() == StartRemoteGdb) {
        cleanFilePath.replace(0, startParameters().remoteMountPoint.length(),
            startParameters().localMountDir);
    }
    return cleanFilePath;
}

void GdbEngine::shutdownInferior()
{
    QTC_ASSERT(state() == InferiorShutdownRequested, qDebug() << state());
    m_commandsToRunOnTemporaryBreak.clear();
    m_gdbAdapter->shutdownInferior();
}

void GdbEngine::defaultInferiorShutdown(const char *cmd)
{
    QTC_ASSERT(state() == InferiorShutdownRequested, qDebug() << state());
    postCommand(cmd, NeedsStop | LosesChild, CB(handleInferiorShutdown));
}

void GdbEngine::handleInferiorShutdown(const GdbResponse &response)
{
    QTC_ASSERT(state() == InferiorShutdownRequested, qDebug() << state());
    if (response.resultClass == GdbResultDone) {
        notifyInferiorShutdownOk();
        return;
    }
    QByteArray ba = response.data.findChild("msg").data();
    if (ba.contains(": No such file or directory.")) {
        // This happens when someone removed the binary behind our back.
        // It is not really an error from a user's point of view.
        showMessage(_("NOTE: " + ba));
        notifyInferiorShutdownOk();
        return;
    }
    showMessageBox(QMessageBox::Critical,
        tr("Failed to shut down application"),
        AbstractGdbAdapter::msgInferiorStopFailed(QString::fromLocal8Bit(ba)));
    notifyInferiorShutdownFailed();
}

void GdbEngine::shutdownEngine()
{
    m_gdbAdapter->shutdownAdapter();
}

void GdbEngine::notifyAdapterShutdownFailed()
{
    showMessage(_("ADAPTER SHUTDOWN FAILED"));
    QTC_ASSERT(state() == EngineShutdownRequested, qDebug() << state());
    notifyEngineShutdownFailed();
}

void GdbEngine::notifyAdapterShutdownOk()
{
    QTC_ASSERT(state() == EngineShutdownRequested, qDebug() << state());
    showMessage(_("INITIATE GDBENGINE SHUTDOWN IN STATE %1, PROC: %2")
        .arg(lastGoodState()).arg(gdbProc()->state()));
    m_commandsDoneCallback = 0;
    switch (gdbProc()->state()) {
    case QProcess::Running:
        postCommand("-gdb-exit", GdbEngine::ExitRequest, CB(handleGdbExit));
        break;
    case QProcess::NotRunning: // Cannot find executable
        notifyEngineShutdownOk();
        break;
    case QProcess::Starting:
        showMessage(_("GDB NOT REALLY RUNNING; KILLING IT"));
        gdbProc()->kill();
        notifyEngineShutdownFailed();
        break;
    }
}

void GdbEngine::handleGdbExit(const GdbResponse &response)
{
    if (response.resultClass == GdbResultExit) {
        showMessage(_("GDB CLAIMS EXIT; WAITING"));
        // Don't set state here, this will be handled in handleGdbFinished()
        //notifyEngineShutdownOk();
    } else {
        QString msg = m_gdbAdapter->msgGdbStopFailed(
            QString::fromLocal8Bit(response.data.findChild("msg").data()));
        qDebug() << (_("GDB WON'T EXIT (%1); KILLING IT").arg(msg));
        showMessage(_("GDB WON'T EXIT (%1); KILLING IT").arg(msg));
        gdbProc()->kill();
    }
}

void GdbEngine::detachDebugger()
{
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
    QTC_ASSERT(startMode() != AttachCore, qDebug() << startMode());
    postCommand("detach", GdbEngine::ExitRequest, CB(handleDetach));
}

void GdbEngine::handleDetach(const GdbResponse &response)
{
    Q_UNUSED(response);
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
    notifyInferiorExited();
}

int GdbEngine::currentFrame() const
{
    return stackHandler()->currentIndex();
}

QString msgNoBinaryForToolChain(int tc)
{
    using namespace ProjectExplorer;
    return GdbEngine::tr("There is no gdb binary available for '%1'")
        .arg(ToolChain::toolChainName(ToolChainType(tc)));
}

AbstractGdbAdapter *GdbEngine::createAdapter()
{
    const DebuggerStartParameters &sp = startParameters();
    //qDebug() << "CREATE ADAPTER: " << sp.toolChainType;
    switch (sp.toolChainType) {
        case ProjectExplorer::ToolChain_WINSCW: // S60
        case ProjectExplorer::ToolChain_GCCE:
        case ProjectExplorer::ToolChain_RVCT_ARMV5:
        case ProjectExplorer::ToolChain_RVCT_ARMV6:
        case ProjectExplorer::ToolChain_RVCT_ARMV5_GNUPOC:
        case ProjectExplorer::ToolChain_GCCE_GNUPOC:
            // fixme: 1 of 3 testing hacks
            if (sp.processArgs.startsWith(__("@tcf@ ")))
                return new TcfTrkGdbAdapter(this);
            return new TrkGdbAdapter(this);
        default:
            break;
    }

    switch (sp.startMode) {
        case AttachCore:
            return new CoreGdbAdapter(this);
        case AttachToRemote:
            return new RemoteGdbServerAdapter(this, sp.toolChainType);
        case StartRemoteGdb:
            return new RemotePlainGdbAdapter(this);
        case AttachExternal:
            return new AttachGdbAdapter(this);
        default:
            if (sp.useTerminal)
                return new TermGdbAdapter(this);
            return new LocalPlainGdbAdapter(this);
    }
}

void GdbEngine::setupEngine()
{
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());
    QTC_ASSERT(m_debuggingHelperState == DebuggingHelperUninitialized, /**/);

    if (m_gdbAdapter->dumperHandling() != AbstractGdbAdapter::DumperNotAvailable) {
        connect(debuggerCore()->action(UseDebuggingHelpers),
            SIGNAL(valueChanged(QVariant)),
            SLOT(setUseDebuggingHelpers(QVariant)));
        connect(debuggerCore()->action(DebugDebuggingHelpers),
            SIGNAL(valueChanged(QVariant)),
            SLOT(setDebugDebuggingHelpersClassic(QVariant)));
    }

    QTC_ASSERT(state() == EngineSetupRequested, /**/);
    m_gdbAdapter->startAdapter();
}

unsigned GdbEngine::debuggerCapabilities() const
{
    unsigned caps = ReverseSteppingCapability
        | AutoDerefPointersCapability | DisassemblerCapability
        | RegisterCapability | ShowMemoryCapability
        | JumpToLineCapability | ReloadModuleCapability
        | ReloadModuleSymbolsCapability | BreakOnThrowAndCatchCapability
        | ReturnFromFunctionCapability
        | CreateFullBacktraceCapability
        | WatchpointCapability
        | AddWatcherCapability;

    if (startParameters().startMode == AttachCore)
        return caps;

    return caps | SnapshotCapability;
}

void GdbEngine::continueInferiorInternal()
{
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
    notifyInferiorRunRequested();
    showStatusMessage(tr("Running requested..."), 5000);
    QTC_ASSERT(state() == InferiorRunRequested, qDebug() << state());
    postCommand("-exec-continue", RunRequest, CB(handleExecuteContinue));
}

void GdbEngine::autoContinueInferior()
{
    continueInferiorInternal();
    showStatusMessage(tr("Continuing after temporary stop..."), 1000);
}

void GdbEngine::continueInferior()
{
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
    setTokenBarrier();
    continueInferiorInternal();
}

void GdbEngine::executeStep()
{
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
    setTokenBarrier();
    notifyInferiorRunRequested();
    showStatusMessage(tr("Step requested..."), 5000);
    if (m_gdbAdapter->isTrkAdapter() && stackHandler()->stackSize() > 0)
        postCommand("sal step,0x" + QByteArray::number(stackHandler()->topAddress(), 16));
    if (isReverseDebugging())
        postCommand("reverse-step", RunRequest, CB(handleExecuteStep));
    else
        postCommand("-exec-step", RunRequest, CB(handleExecuteStep));
}

void GdbEngine::handleExecuteStep(const GdbResponse &response)
{
    if (response.resultClass == GdbResultDone) {
        // Step was finishing too quick, and a '*stopped' messages should
        // have preceeded it, so just ignore this result.
        QTC_ASSERT(state() == InferiorStopOk, /**/);
        return;
    }
    QTC_ASSERT(state() == InferiorRunRequested, qDebug() << state());
    if (response.resultClass == GdbResultRunning) {
        notifyInferiorRunOk();
        return;
    }
    QByteArray msg = response.data.findChild("msg").data();
    if (msg.startsWith("Cannot find bounds of current function")) {
        notifyInferiorRunFailed();
        if (isDying())
            return;
        if (!m_commandsToRunOnTemporaryBreak.isEmpty())
            flushQueuedCommands();
        executeStepI(); // Fall back to instruction-wise stepping.
    } else {
        showExecutionError(QString::fromLocal8Bit(msg));
        notifyInferiorIll();
    }
}

void GdbEngine::executeStepI()
{
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
    setTokenBarrier();
    notifyInferiorRunRequested();
    showStatusMessage(tr("Step by instruction requested..."), 5000);
    if (isReverseDebugging())
        postCommand("reverse-stepi", RunRequest, CB(handleExecuteContinue));
    else
        postCommand("-exec-step-instruction", RunRequest, CB(handleExecuteContinue));
}

void GdbEngine::executeStepOut()
{
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
    postCommand("-stack-select-frame 0");
    setTokenBarrier();
    notifyInferiorRunRequested();
    showStatusMessage(tr("Finish function requested..."), 5000);
    postCommand("-exec-finish", RunRequest, CB(handleExecuteContinue));
}

void GdbEngine::executeNext()
{
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
    setTokenBarrier();
    notifyInferiorRunRequested();
    showStatusMessage(tr("Step next requested..."), 5000);
    if (m_gdbAdapter->isTrkAdapter() && stackHandler()->stackSize() > 0)
        postCommand("sal next,0x" + QByteArray::number(stackHandler()->topAddress(), 16));
    if (isReverseDebugging())
        postCommand("reverse-next", RunRequest, CB(handleExecuteNext));
    else
        postCommand("-exec-next", RunRequest, CB(handleExecuteNext));
}

void GdbEngine::handleExecuteNext(const GdbResponse &response)
{
    if (response.resultClass == GdbResultDone) {
        // Step was finishing too quick, and a '*stopped' messages should
        // have preceeded it, so just ignore this result.
        QTC_ASSERT(state() == InferiorStopOk, /**/);
        return;
    }
    QTC_ASSERT(state() == InferiorRunRequested, qDebug() << state());
    if (response.resultClass == GdbResultRunning) {
        notifyInferiorRunOk();
        return;
    }
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
    QByteArray msg = response.data.findChild("msg").data();
    if (msg.startsWith("Cannot find bounds of current function")) {
        if (!m_commandsToRunOnTemporaryBreak.isEmpty())
            flushQueuedCommands();
        notifyInferiorRunFailed();
        if (!isDying())
            executeNextI(); // Fall back to instruction-wise stepping.
    } else {
        showMessageBox(QMessageBox::Critical, tr("Execution Error"),
           tr("Cannot continue debugged process:\n") + QString::fromLocal8Bit(msg));
        notifyInferiorIll();
    }
}

void GdbEngine::executeNextI()
{
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
    setTokenBarrier();
    notifyInferiorRunRequested();
    showStatusMessage(tr("Step next instruction requested..."), 5000);
    if (isReverseDebugging())
        postCommand("reverse-nexti", RunRequest, CB(handleExecuteContinue));
    else
        postCommand("-exec-next-instruction", RunRequest, CB(handleExecuteContinue));
}

void GdbEngine::executeRunToLine(const QString &fileName, int lineNumber)
{
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
    setTokenBarrier();
    notifyInferiorRunRequested();
    showStatusMessage(tr("Run to line %1 requested...").arg(lineNumber), 5000);
#if 1
    m_targetFrame.file = fileName;
    m_targetFrame.line = lineNumber;
    QByteArray loc = '"' + breakLocation(fileName).toLocal8Bit() + '"' + ':'
        + QByteArray::number(lineNumber);
    postCommand("tbreak " + loc);
    postCommand("continue", RunRequest, CB(handleExecuteRunToLine));
#else
    // Seems to jump to unpredicatable places. Observed in the manual
    // tests in the Foo::Foo() constructor with both gdb 6.8 and 7.1.
    QByteArray args = '"' + breakLocation(fileName).toLocal8Bit() + '"' + ':'
        + QByteArray::number(lineNumber);
    postCommand("-exec-until " + args, RunRequest, CB(handleExecuteContinue));
#endif
}

void GdbEngine::executeRunToFunction(const QString &functionName)
{
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
    setTokenBarrier();
    postCommand("-break-insert -t " + functionName.toLatin1());
    showStatusMessage(tr("Run to function %1 requested...").arg(functionName), 5000);
    continueInferiorInternal();
    //postCommand("-exec-continue", handleExecuteRunToFunction);
}

void GdbEngine::executeJumpToLine(const QString &fileName, int lineNumber)
{
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
    StackFrame frame;
    frame.file = fileName;
    frame.line = lineNumber;
#if 1
    QByteArray loc = '"' + breakLocation(fileName).toLocal8Bit() + '"' + ':'
        + QByteArray::number(lineNumber);
    postCommand("tbreak " + loc);
    notifyInferiorRunRequested();
    postCommand("jump " + loc, RunRequest, CB(handleExecuteJumpToLine));
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
    gotoLocation(frame, true);
    setBreakpoint(fileName, lineNumber);
    notifyInferiorRunRequested();
    postCommand("jump " + loc, RunRequest);
#endif
}

void GdbEngine::executeReturn()
{
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
    setTokenBarrier();
    notifyInferiorRunRequested();
    showStatusMessage(tr("Immediate return from function requested..."), 5000);
    postCommand("-exec-finish", RunRequest, CB(handleExecuteReturn));
}

void GdbEngine::handleExecuteReturn(const GdbResponse &response)
{
    if (response.resultClass == GdbResultDone) {
        notifyInferiorStopOk();
        updateAll();
        return;
    }
    notifyInferiorRunFailed();
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
            qDebug() << "CMD:" << cookie.command
                << " FLAGS:" << cookie.flags
                << " CALLBACK:" << cookie.callbackName;
            return
        );
    }
    PENDING_DEBUG("\n--- token barrier ---\n");
    showMessage(_("--- token barrier ---"), LogMiscInput);
    if (debuggerCore()->boolSetting(LogTimeStamps))
        showMessage(LogWindow::logTimeStamp(), LogMiscInput);
    m_oldestAcceptableToken = currentToken();
}


//////////////////////////////////////////////////////////////////////
//
// Breakpoint specific stuff
//
//////////////////////////////////////////////////////////////////////

void GdbEngine::updateBreakpointDataFromOutput(BreakpointId id, const GdbMi &bkpt)
{
    QTC_ASSERT(bkpt.isValid(), return);
    BreakpointResponse response = breakHandler()->response(id);

    response.multiple = false;
    response.enabled = true;
    response.pending = false;
    response.condition.clear();
    QByteArray file, fullName;
    foreach (const GdbMi &child, bkpt.children()) {
        if (child.hasName("number")) {
            response.number = child.data().toInt();
        } else if (child.hasName("func")) {
            response.functionName = _(child.data());
        } else if (child.hasName("addr")) {
            // <MULTIPLE> happens in constructors. In this case there are
            // _two_ fields named "addr" in the response. On Linux that is...
            if (child.data().startsWith("0x")) {
                response.address = child.data().mid(2).toULongLong(0, 16);
            } else {
                response.extra = child.data();
                if (child.data() == "<MULTIPLE>")
                    response.multiple = true;
            }
        } else if (child.hasName("file")) {
            file = child.data();
        } else if (child.hasName("fullname")) {
            fullName = child.data();
        } else if (child.hasName("line")) {
            bool ok;
            const int lineNumber = child.data().toInt(&ok);
            response.lineNumber = lineNumber;
            //if (ok && response.bpCorrectedLineNumber <= 0)
            //    data->setMarkerLineNumber(lineNumber);
        } else if (child.hasName("cond")) {
            response.condition = child.data();
            // gdb 6.3 likes to "rewrite" conditions. Just accept that fact.
            //if (response.bpCondition != data->condition()
            //        && data->conditionsMatch(response.bpCondition))
            //    data->setCondition(response.bpCondition);
        } else if (child.hasName("enabled")) {
            response.enabled = (child.data() == "y");
        } else if (child.hasName("pending")) {
            response.pending = true;
            // Any content here would be interesting only if we did accept
            // spontaneously appearing breakpoints (user using gdb commands).
        } else if (child.hasName("at")) {
            // Happens with (e.g.?) gdb 6.4 symbianelf
            QByteArray ba = child.data();
            if (ba.startsWith('<') && ba.endsWith('>'))
                ba = ba.mid(1, ba.size() - 2);
            response.functionName = _(ba);
        } else if (child.hasName("thread")) {
            response.threadSpec = child.data();
        } else if (child.hasName("type")) {
            // FIXME: This should not change the type.
            //if (child.data().contains("reakpoint")) // "breakpoint", "hw breakpoint"
            //    ; // data->type = BreakpointData::BreakpointType;
            //else // FIXME: Incomplete list of cases.
            //    data->type = Watchpoint;
        }
        // This field is not present.  Contents needs to be parsed from
        // the plain "ignore" response.
        //else if (child.hasName("ignore"))
        //    data->bpIgnoreCount = child.data();
    }

    QString name;
    if (!fullName.isEmpty()) {
        name = cleanupFullName(QFile::decodeName(fullName));
        response.fileName = name;
        //if (data->markerFileName().isEmpty())
        //    data->setMarkerFileName(name);
    } else {
        name = QFile::decodeName(file);
        // Use fullName() once we have a mapping which is more complete than
        // gdb's own. No point in assigning markerFileName for now.
    }
    if (!name.isEmpty())
        response.fileName = name;

    breakHandler()->setResponse(id, response);
}

QString GdbEngine::breakLocation(const QString &file) const
{
    //QTC_ASSERT(!m_breakListOutdated, /* */)
    QString where = m_fullToShortName.value(file);
    if (where.isEmpty())
        return QFileInfo(file).fileName();
    return where;
}

static QByteArray addressSpec(quint64 address)
{
    return "*0x" + QByteArray::number(address, 16);
}

QByteArray GdbEngine::breakpointLocation(BreakpointId id)
{
    BreakHandler *handler = breakHandler();
    const BreakpointParameters &data = handler->breakpointData(id);
    QTC_ASSERT(data.type != UnknownType, return QByteArray());
    // FIXME: Non-GCC-runtime
    if (data.type == BreakpointAtThrow)
        return "__cxa_throw";
    if (data.type == BreakpointAtCatch)
        return "__cxa_begin_catch";
    if (data.type == BreakpointAtMain)
#ifdef Q_OS_WIN
        return "qMain";
#else
        return "main";
#endif
    const QByteArray functionName = data.functionName.toUtf8();
    if (!functionName.isEmpty())
        return functionName;
    if (const quint64 address = data.address)
        return addressSpec(address);
    // In this case, data->funcName is something like '*0xdeadbeef'
    const int lineNumber = data.lineNumber;
    if (lineNumber == 0)
        return functionName;
    const QString fileName = data.useFullPath
        ? data.fileName : breakLocation(data.fileName);
    // The argument is simply a C-quoted version of the argument to the
    // non-MI "break" command, including the "original" quoting it wants.
    return "\"\\\"" + GdbMi::escapeCString(fileName).toLocal8Bit() + "\\\":"
        + QByteArray::number(lineNumber) + '"';
}

void GdbEngine::handleWatchInsert(const GdbResponse &response)
{
    const int id = response.cookie.toInt();
    if (response.resultClass == GdbResultDone) {
        // "Hardware watchpoint 2: *0xbfffed40\n"
        QByteArray ba = response.data.findChild("consolestreamoutput").data();
        if (ba.startsWith("Hardware watchpoint ")) {
            const int pos = ba.indexOf(':', 20);
            BreakpointResponse response = breakHandler()->response(id);
            response.number = ba.mid(20, pos - 20).toInt();
            breakHandler()->setResponse(id, response);
        } else {
            showMessage(_("CANNOT PARSE WATCHPOINT FROM" + ba));
        }
    }
}

void GdbEngine::attemptAdjustBreakpointLocation(BreakpointId id)
{
    if (!debuggerCore()->boolSetting(AdjustBreakpointLocations))
        return;
    BreakpointResponse response = breakHandler()->response(id);
    if (response.address == 0 || response.correctedLineNumber != 0)
        return;
    // Prevent endless loop.
    response.correctedLineNumber = -1;
    breakHandler()->setResponse(id, response);
    postCommand("info line *0x" + QByteArray::number(response.address, 16),
        NeedsStop | RebuildBreakpointModel,
        CB(handleInfoLine), id);
}

void GdbEngine::handleBreakInsert1(const GdbResponse &response)
{
    const int id = response.cookie.toInt();
    if (response.resultClass == GdbResultDone) {
        // Interesting only on Mac?
        GdbMi bkpt = response.data.findChild("bkpt");
        updateBreakpointDataFromOutput(id, bkpt);
        attemptAdjustBreakpointLocation(id);
    } else {
        // Some versions of gdb like "GNU gdb (GDB) SUSE (6.8.91.20090930-2.4)"
        // know how to do pending breakpoints using CLI but not MI. So try
        // again with MI.
        QByteArray cmd = "break " + breakpointLocation(id);
        postCommand(cmd, NeedsStop | RebuildBreakpointModel,
            CB(handleBreakInsert2), id);
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
/*
    postCommand("-break-list",
        NeedsStop | RebuildBreakpointModel,
        CB(handleBreakList));
*/
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
    const GdbMi body = table.findChild("body");
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

    foreach (const GdbMi &bkpt, bkpts) {
        BreakpointResponse needle;
        needle.fileName = _("xx");
        BreakpointId id = breakHandler()->findSimilarBreakpoint(needle);
        //qDebug() << "\n\nGOT: " << bkpt.toString() << '\n' << temp.toString();
        // FIXME: use updateBreakpointDataFromOutput()
        if (id != BreakpointId(-1)) {
            //qDebug() << "  FROM: " << data->toString();
            updateBreakpointDataFromOutput(id, bkpt);
            //qDebug() << "  TO: " << data->toString();
        } else {
            qDebug() << "  NOTHING SUITABLE FOUND";
            showMessage(_("CANNOT FIND BP: " + bkpt.toString()));
        }
    }

    m_breakListOutdated = false;
}

void GdbEngine::handleBreakDisable(const GdbResponse &response)
{
    QTC_ASSERT(response.resultClass == GdbResultDone, /**/)
    const BreakpointId id = response.cookie.toInt();
    // This should only be the requested state.
    QTC_ASSERT(!breakHandler()->isEnabled(id), /* Prevent later recursion */);
    BreakpointResponse br = breakHandler()->response(id);
    br.enabled = false;
    breakHandler()->setResponse(id, br);
}

void GdbEngine::handleBreakEnable(const GdbResponse &response)
{
    QTC_ASSERT(response.resultClass == GdbResultDone, /**/)
    const BreakpointId id = response.cookie.toInt();
    // This should only be the requested state.
    QTC_ASSERT(breakHandler()->isEnabled(id), /* Prevent later recursion */);
    BreakpointResponse br = breakHandler()->response(id);
    br.enabled = true;
    breakHandler()->setResponse(id, br);
}

void GdbEngine::handleBreakIgnore(const GdbResponse &response)
{
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
    QTC_ASSERT(response.resultClass == GdbResultDone, /**/)
    QString msg = _(response.data.findChild("consolestreamoutput").data());
    BreakpointId id(response.cookie.toInt());
    BreakpointResponse br = breakHandler()->response(id);
    //if (msg.contains(__("Will stop next time breakpoint")))
    //    response.ignoreCount = _("0");
    //else if (msg.contains(__("Will ignore next")))
    //    response.ignoreCount = data->ignoreCount;
    // FIXME: this assumes it is doing the right thing...
    br.ignoreCount = breakHandler()->ignoreCount(id);
    breakHandler()->setResponse(id, br);
}

void GdbEngine::handleBreakCondition(const GdbResponse &response)
{
    QTC_ASSERT(response.resultClass == GdbResultDone, /**/)
    const BreakpointId id = response.cookie.toInt();
    // We just assume it was successful. Otherwise we had to parse
    // the output stream data.
    // The following happens on Mac:
    //   QByteArray msg = response.data.findChild("msg").data();
    //   if (1 || msg.startsWith("Error parsing breakpoint condition. "
    //            " Will try again when we hit the breakpoint.")) {
    BreakpointResponse br = breakHandler()->response(id);
    br.condition = breakHandler()->condition(id);
    breakHandler()->setResponse(id, br);
}

void GdbEngine::extractDataFromInfoBreak(const QString &output, BreakpointId id)
{
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

    BreakpointResponse response;
    response.fileName = _("<MULTIPLE>");

    QString requestedFileName = breakHandler()->fileName(id);

    if (re.indexIn(output) != -1) {
        response.address = re.cap(1).toULongLong(0, 16);
        response.functionName = re.cap(2).trimmed();
        response.lineNumber = re.cap(4).toInt();
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
        if (requestedFileName.endsWith(full))
            full = requestedFileName;
        //response.bpCorrectedLineNumber = breakHandler()->lineNumber();
        //if (data->markerFileName().isEmpty()) {
        //    qDebug() << "111";
        //    data->setMarkerFileName(full);
        //}
        response.fileName = full;
    } else {
        qDebug() << "COULD NOT MATCH " << re.pattern() << " AND " << output;
        response.number = -1; // <unavailable>
    }
    breakHandler()->setResponse(id, response);
}

void GdbEngine::handleBreakInfo(const GdbResponse &response)
{
    if (response.resultClass == GdbResultDone) {
        // Old-style output for multiple breakpoints, presumably in a
        // constructor.
        const BreakpointId id(response.cookie.toInt());
        const QString str = QString::fromLocal8Bit(
            response.data.findChild("consolestreamoutput").data());
        extractDataFromInfoBreak(str, id);
    }
}

void GdbEngine::handleInfoLine(const GdbResponse &response)
{
    if (response.resultClass == GdbResultDone) {
        // Old-style output: "Line 1102 of \"simple/app.cpp\" starts
        // at address 0x80526aa <_Z10...+131> and ends at 0x80526b5
        // <_Z10testQStackv+142>.\n"
        QByteArray ba = response.data.findChild("consolestreamoutput").data();
        const int pos = ba.indexOf(' ', 5);
        if (ba.startsWith("Line ") && pos != -1) {
            const int line = ba.mid(5, pos - 5).toInt();
            const BreakpointId id = response.cookie.toInt();
            BreakpointResponse br = breakHandler()->response(id);
            br.lineNumber = line;
            breakHandler()->setResponse(id, br);
        }
    }
}


bool GdbEngine::stateAcceptsBreakpointChanges() const
{
    switch (state()) {
    case InferiorSetupRequested:
    case InferiorRunRequested:
    case InferiorRunOk:
    case InferiorStopRequested:
    case InferiorStopOk:
        return true;
    default:
        return false;
    }
}

/*
void GdbEngine::attemptBreakpointSynchronization()
{
    QTC_ASSERT(!m_sourcesListUpdating,
        qDebug() << "SOURCES LIST CURRENTLY UPDATING"; return);
    showMessage(_("ATTEMPT BREAKPOINT SYNC"));

    // We don't have breakpoints in core files.
    if (startParameters().startMode == AttachCore)
        return;

    switch (state()) {
    case InferiorSetupRequested:
    case InferiorRunRequested:
    case InferiorRunOk:
    case InferiorStopRequested:
    case InferiorStopOk:
        break;
    default:
        //qDebug() << "attempted breakpoint sync in state" << state();
        showMessage(_("... NOT POSSIBLE IN CURRENT STATE"));
        return;
    }

    if (m_breakListOutdated) {
        reloadBreakListInternal();
        return;
    }

    BreakHandler *handler = breakHandler();

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

    for (int index = 0; index != handler->size(); ++index) {
        BreakpointData *data = handler->at(index);
        if (data->bpNumber.isEmpty()) { // Unset breakpoint?
            data->bpNumber = " "; // Sent, but no feedback yet.
            sendInsertBreakpoint(index);
        } else if (data->bpNumber.toInt()) {
            if (data->bpMultiple && data->bpFileName.isEmpty()) {
                postCommand("info break " + data->bpNumber,
                    NeedsStop | RebuildBreakpointModel,
                    CB(handleBreakInfo), id);
                continue;
            }
            if (data->condition != data->bpCondition && !data->conditionsMatch()) {
                // Update conditions if needed.
                postCommand("condition " + data->bpNumber + ' '  + data->condition,
                    NeedsStop | RebuildBreakpointModel,
                    CB(handleBreakCondition), id);
            }
            else // Because gdb won't do both changes at a time anyway.
            if (data->ignoreCount != data->bpIgnoreCount) {
                // Update ignorecount if needed.
                QByteArray ic = QByteArray::number(data->ignoreCount);
                postCommand("ignore " + data->bpNumber + ' ' + ic,
                    NeedsStop | RebuildBreakpointModel,
                    CB(handleBreakIgnore), id);
                continue;
            }
            if (!data->enabled && data->bpEnabled) {
                postCommand("-break-disable " + data->bpNumber,
                    NeedsStop | RebuildBreakpointModel,
                    CB(handleBreakDisable), id);
                data->bpEnabled = false;
                continue;
            }
            if (data->threadSpec != data->bpThreadSpec && !data->bpNumber.isEmpty()) {
                // The only way to change this seems to be to re-set the bp completely.
                //qDebug() << "FIXME: THREAD: " << data->threadSpec << data->bpThreadSpec;
                data->bpThreadSpec.clear();
                postCommand("-break-delete " + data->bpNumber,
                    NeedsStop | RebuildBreakpointModel);
                sendInsertBreakpoint(index);
                continue;
            }
            if (data->bpAddress && data->bpCorrectedLineNumber == 0) {
                // Prevent endless loop.
                data->bpCorrectedLineNumber = -1;
                if (debuggerCore()->boolSetting(AdjustBreakpointLocations)) {
                    postCommand(
                        "info line *0x" + QByteArray::number(data->bpAddress, 16),
                        NeedsStop | RebuildBreakpointModel,
                        CB(handleInfoLine), id)
                }
            }
        }
    }

    handler->updateMarkers();
}
*/


bool GdbEngine::acceptsBreakpoint(BreakpointId id) const
{
    const QString fileName = breakHandler()->fileName(id);
    return !fileName.endsWith(QLatin1String("js"))
        && !fileName.endsWith(QLatin1String("qml"));
}

void GdbEngine::insertBreakpoint(BreakpointId id)
{
    // Set up fallback in case of pending breakpoints which aren't handled
    // by the MI interface.
    BreakHandler *handler = breakHandler();
    QTC_ASSERT(handler->state(id) == BreakpointInsertRequested, /**/);
    handler->notifyBreakpointInsertProceeding(id);
    if (handler->type(id) == Watchpoint) {
        postCommand("watch " + addressSpec(handler->address(id)),
            NeedsStop | RebuildBreakpointModel,
            CB(handleWatchInsert), id);
        return;
    }

    QByteArray cmd;
    if (m_isMacGdb) {
        cmd = "-break-insert -l -1 -f ";
    } else if (m_gdbAdapter->isTrkAdapter()) {
        cmd = "-break-insert -h -f ";
    } else if (m_gdbVersion >= 70000) {
        QByteArray spec = handler->threadSpec(id);
        cmd = "-break-insert ";
        if (!spec.isEmpty())
            cmd += "-p " + spec;
        cmd += " -f ";
    } else if (m_gdbVersion >= 60800) {
        // Probably some earlier version would work as well.
        cmd = "-break-insert -f ";
    } else {
        cmd = "-break-insert ";
    }
    //if (!data->condition.isEmpty())
    //    cmd += "-c " + data->condition + ' ';
    cmd += breakpointLocation(id);
    postCommand(cmd, NeedsStop | RebuildBreakpointModel,
        CB(handleBreakInsert1), id);
}

void GdbEngine::changeBreakpoint(BreakpointId id)
{
    BreakHandler *handler = breakHandler();
    const BreakpointParameters &data = handler->breakpointData(id);
    QTC_ASSERT(data.type != UnknownType, return);
    const BreakpointResponse &response = handler->response(id);
    QTC_ASSERT(response.number > 0, return);
    const QByteArray bpnr = QByteArray::number(response.number);
    QTC_ASSERT(response.number > 0, return);

    bool finished = true;
    if (data.condition != response.condition
        && !data.conditionsMatch(response.condition)) {
        // Update conditions if needed.
        handler->notifyBreakpointChangeProceeding(id);
        finished = false;
        postCommand("condition " + bpnr + ' '  + data.condition,
            NeedsStop | RebuildBreakpointModel,
            CB(handleBreakCondition), id);
    }
    if (data.ignoreCount != response.ignoreCount) {
        // Update ignorecount if needed.
        handler->notifyBreakpointChangeProceeding(id);
        finished = false;
        postCommand("ignore " + bpnr + ' ' + QByteArray::number(data.ignoreCount),
            NeedsStop | RebuildBreakpointModel,
            CB(handleBreakIgnore), id);
    }
    if (!data.enabled && response.enabled) {
        handler->notifyBreakpointChangeProceeding(id);
        finished = false;
        postCommand("-break-disable " + bpnr,
            NeedsStop | RebuildBreakpointModel,
            CB(handleBreakDisable), id);
    }
    if (data.enabled && !response.enabled) {
        handler->notifyBreakpointChangeProceeding(id);
        finished = false;
        postCommand("-break-enable " + bpnr,
            NeedsStop | RebuildBreakpointModel,
            CB(handleBreakEnable), id);
    }

    if (data.threadSpec != response.threadSpec) {
        // The only way to change this seems to be to re-set the bp completely.
        qDebug() << "FIXME: THREAD: " << data.threadSpec << response.threadSpec;
        //response.threadSpec.clear();
        //postCommand("-break-delete " + bpnr,
        //    NeedsStop | RebuildBreakpointModel);
        //sendInsertBreakpoint(index);
        //continue;
    }

    attemptAdjustBreakpointLocation(id);

    if (finished)
        handler->notifyBreakpointChangeOk(id);
}

void GdbEngine::removeBreakpoint(BreakpointId id)
{
    BreakHandler *handler = breakHandler();
    QTC_ASSERT(handler->state(id) == BreakpointRemoveRequested, /**/);
    handler->notifyBreakpointRemoveProceeding(id);
    BreakpointResponse br = handler->response(id);
    showMessage(_("DELETING BP %1 IN ").arg(br.number)
        + handler->fileName(id));
    postCommand("-break-delete " + QByteArray::number(br.number),
        NeedsStop | RebuildBreakpointModel);
    // Pretend it succeeds without waiting for response. Feels better.
    // FIXME: Really?
    handler->notifyBreakpointRemoveOk(id);
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

void GdbEngine::requestModuleSymbols(const QString &moduleName)
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
    showModuleSymbols(moduleName, rc);
}

void GdbEngine::reloadModules()
{
    if (state() == InferiorRunOk || state() == InferiorStopOk)
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
        QString data = QString::fromLocal8Bit(
            response.data.findChild("consolestreamoutput").data());
        QTextStream ts(&data, QIODevice::ReadOnly);
        while (!ts.atEnd()) {
            QString line = ts.readLine();
            Module module;
            QString symbolsRead;
            QTextStream ts(&line, QIODevice::ReadOnly);
            if (line.startsWith(__("0x"))) {
                ts >> module.startAddress >> module.endAddress >> symbolsRead;
                module.moduleName = ts.readLine().trimmed();
                module.symbolsRead =
                    (symbolsRead == __("Yes") ? Module::ReadOk : Module::ReadFailed);
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
                module.symbolsRead = (item.findChild("state").data() == "Y")
                        ? Module::ReadOk : Module::ReadFailed;
                module.startAddress = _(item.findChild("loaded_addr").data());
                //: End address of loaded module
                module.endAddress = tr("<unknown>", "address");
                modules.append(module);
            }
        }
    }
    modulesHandler()->setModules(modules);
}


void GdbEngine::examineModules()
{
    foreach (Module module, modulesHandler()->modules()) {
        if (module.symbolsType == Module::UnknownType) {
            QProcess proc;
            qDebug() << _("objdump -h \"%1\"").arg(module.moduleName);
            proc.start(_("objdump -h \"%1\"").arg(module.moduleName));
            if (!proc.waitForStarted())
                continue;
            if (!proc.waitForFinished())
                continue;
            QByteArray ba = proc.readAllStandardOutput();
            if (ba.contains(".gdb_index"))
                module.symbolsType = Module::FastSymbols;
            else
                module.symbolsType = Module::PlainSymbols;
            modulesHandler()->updateModule(module.moduleName, module);
        }
    }
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
    if ((state() == InferiorRunOk || state() == InferiorStopOk)
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
    threadsHandler()->setCurrentThread(index);
    Threads threads = threadsHandler()->threads();
    QTC_ASSERT(index < threads.size(), return);
    const int id = threads.at(index).id;
    showStatusMessage(tr("Retrieving data for stack view thread 0x%1...")
        .arg(id, 0, 16), 10000);
    postCommand("-thread-select " + QByteArray::number(id), Discardable,
        CB(handleStackSelectThread));
}

void GdbEngine::handleStackSelectThread(const GdbResponse &)
{
    QTC_ASSERT(state() == InferiorUnrunnable || state() == InferiorStopOk, /**/);
    showStatusMessage(tr("Retrieving data for stack view..."), 3000);
    reloadRegisters();
    reloadStack(true);
    updateLocals();
}

void GdbEngine::reloadFullStack()
{
    PENDING_DEBUG("RELOAD FULL STACK");
    postCommand("-stack-list-frames", Discardable, CB(handleStackListFrames),
        QVariant::fromValue<StackCookie>(StackCookie(true, true)));
}

void GdbEngine::reloadStack(bool forceGotoLocation)
{
    PENDING_DEBUG("RELOAD STACK");
    QByteArray cmd = "-stack-list-frames";
    int stackDepth = debuggerCore()->action(MaximalStackDepth)->value().toInt();
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
        postCommand(cmd);
    postCommand(cmd, Discardable, CB(handleStackListFrames),
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
    frame.address = frameMi.findChild("addr").data().toULongLong(0, 16);
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

#        if defined(Q_OS_WIN)
        const bool isBogus =
            // Assume this is wrong and points to some strange stl_algobase
            // implementation. Happens on Karsten's XP system with Gdb 5.50
            (frame.file.endsWith(__("/bits/stl_algobase.h")) && frame.line == 150)
            // Also wrong. Happens on Vista with Gdb 5.50
               || (frame.function == __("operator new") && frame.line == 151);

        // Immediately leave bogus frames.
        if (targetFrame == -1 && isBogus) {
            setTokenBarrier();
            notifyInferiorRunRequested();
            postCommand("-exec-finish", RunRequest, CB(handleExecuteContinue));
            showStatusMessage(tr("Jumping out of bogus frame..."), 1000);
            return;
        }
#        endif

        // Initialize top frame to the first valid frame.
        const bool isValid = frame.isUsable() && !frame.function.isEmpty();
        if (isValid && targetFrame == -1)
            targetFrame = i;
    }

    bool canExpand = !cookie.isFull
        && (n >= debuggerCore()->action(MaximalStackDepth)->value().toInt());
    debuggerCore()->action(ExpandStack)->setEnabled(canExpand);
    stackHandler()->setFrames(stackFrames, canExpand);

    // We can't jump to any file if we don't have any frames.
    if (stackFrames.isEmpty())
        return;

    // targetFrame contains the top most frame for which we have source
    // information. That's typically the frame we'd like to jump to, with
    // a few exceptions:

    // Always jump to frame #0 when stepping by instruction.
    if (debuggerCore()->boolSetting(OperateByInstruction))
        targetFrame = 0;

    // If there is no frame with source, jump to frame #0.
    if (targetFrame == -1)
        targetFrame = 0;

    stackHandler()->setCurrentIndex(targetFrame);
    activateFrame(targetFrame);
}

void GdbEngine::activateFrame(int frameIndex)
{
    if (state() != InferiorStopOk && state() != InferiorUnrunnable)
        return;

    StackHandler *handler = stackHandler();

    if (frameIndex == handler->stackSize()) {
        reloadFullStack();
        return;
    }

    QTC_ASSERT(frameIndex < handler->stackSize(), return);

    // Assuming the command always succeeds this saves a roundtrip.
    // Otherwise the lines below would need to get triggered
    // after a response to this -stack-select-frame here.
    handler->setCurrentIndex(frameIndex);
    postCommand("-stack-select-frame " + QByteArray::number(frameIndex),
        Discardable, CB(handleStackSelectFrame));
    gotoLocation(stackHandler()->currentFrame(), true);
    updateLocals();
    reloadRegisters();
}

void GdbEngine::handleStackSelectFrame(const GdbResponse &response)
{
    Q_UNUSED(response);
}

void GdbEngine::handleThreadInfo(const GdbResponse &response)
{
    const int id = response.cookie.toInt();
    if (response.resultClass == GdbResultDone) {
        int currentThreadId;
        const Threads threads =
            ThreadsHandler::parseGdbmiThreads(response.data, &currentThreadId);
        threadsHandler()->setThreads(threads);
        threadsHandler()->setCurrentThreadId(currentThreadId);
        updateViews(); // Adjust Threads combobox.
        if (m_hasInferiorThreadList) {
            postCommand("threadnames " +
                debuggerCore()->action(MaximalStackDepth)->value().toByteArray(),
                Discardable, CB(handleThreadNames), id);
        }
    } else {
        // Fall back for older versions: Try to get at least a list
        // of running threads.
        postCommand("-thread-list-ids", Discardable, CB(handleThreadListIds), id);
    }
}

void GdbEngine::handleThreadListIds(const GdbResponse &response)
{
    const int id = response.cookie.toInt();
    // "72^done,{thread-ids={thread-id="2",thread-id="1"},number-of-threads="2"}
    // In gdb 7.1+ additionally: current-thread-id="1"
    const QList<GdbMi> items = response.data.findChild("thread-ids").children();
    Threads threads;
    for (int index = 0, n = items.size(); index != n; ++index) {
        ThreadData thread;
        thread.id = items.at(index).data().toInt();
        threads.append(thread);
    }
    threadsHandler()->setThreads(threads);
    threadsHandler()->setCurrentThreadId(id);
}

void GdbEngine::handleThreadNames(const GdbResponse &response)
{
    if (response.resultClass == GdbResultDone) {
        GdbMi contents = response.data.findChild("consolestreamoutput");
        GdbMi names;
        names.fromString(contents.data());

        Threads threads = threadsHandler()->threads();

        foreach (const GdbMi &name, names.children()) {
            int id = name.findChild("id").data().toInt();
            for (int index = 0, n = threads.size(); index != n; ++index) {
                ThreadData & thread = threads[index];
                if (thread.id == (quint64)id) {
                    thread.name = decodeData(name.findChild("value").data(),
                        name.findChild("valueencoded").data().toInt());
                    break;
                }
            }
        }
        threadsHandler()->setThreads(threads);
        updateViews();
    }
}


//////////////////////////////////////////////////////////////////////
//
// Snapshot specific stuff
//
//////////////////////////////////////////////////////////////////////

void GdbEngine::createSnapshot()
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
        DebuggerStartParameters sp = startParameters();
        sp.startMode = AttachCore;
        sp.coreFile = response.cookie.toString();
        //snapshot.setDate(QDateTime::currentDateTime());
        StackFrames frames = stackHandler()->frames();
        QString function = _("<unknown>");
        if (!frames.isEmpty()) {
            const StackFrame &frame = frames.at(0);
            function = frame.function + _(":") + QString::number(frame.line);
        }
        sp.displayName = function + _(": ") + QDateTime::currentDateTime().toString();
        sp.isSnapshot = true;
        DebuggerRunControl *rc = DebuggerPlugin::createDebugger(sp);
        DebuggerPlugin::startDebugger(rc);
    } else {
        QByteArray msg = response.data.findChild("msg").data();
        showMessageBox(QMessageBox::Critical, tr("Snapshot Creation Error"),
            tr("Cannot create snapshot:\n") + QString::fromLocal8Bit(msg));
    }
}


//////////////////////////////////////////////////////////////////////
//
// Register specific stuff
//
//////////////////////////////////////////////////////////////////////

void GdbEngine::reloadRegisters()
{
    if (!debuggerCore()->isRegisterViewVisible())
        return;

    if (state() != InferiorStopOk && state() != InferiorUnrunnable)
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
    Register reg = registerHandler()->registers().at(nr);
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

    registerHandler()->setRegisters(registers);

    if (m_gdbAdapter->isTrkAdapter())
        m_gdbAdapter->trkReloadRegisters();
}

void GdbEngine::handleRegisterListValues(const GdbResponse &response)
{
    if (response.resultClass != GdbResultDone)
        return;

    Registers registers = registerHandler()->registers();

    // 24^done,register-values=[{number="0",value="0xf423f"},...]
    const GdbMi values = response.data.findChild("register-values");
    foreach (const GdbMi &item, values.children()) {
        const int index = item.findChild("number").data().toInt();
        if (index < registers.size()) {
            Register &reg = registers[index];
            GdbMi val = item.findChild("value");
            QByteArray ba;
            bool handled = false;
            if (val.data().startsWith("{")) {
                int pos1 = val.data().indexOf("v2_int32");
                if (pos1 == -1)
                    pos1 = val.data().indexOf("v4_int32");
                if (pos1 != -1) {
                    // FIXME: This block wastes cycles.
                    pos1 = val.data().indexOf("{", pos1 + 1) + 1;
                    int pos2 = val.data().indexOf("}", pos1);
                    QByteArray ba2 = val.data().mid(pos1, pos2 - pos1);
                    foreach (QByteArray ba3, ba2.split(',')) {
                        ba3 = ba3.trimmed();
                        QTC_ASSERT(ba3.size() >= 3, continue);
                        QTC_ASSERT(ba3.size() <= 10, continue);
                        ba.prepend(QByteArray(10 - ba3.size(), '0'));
                        ba.prepend(ba3.mid(2));
                    }
                    ba.prepend("0x");
                    handled = true;
                }
            }
            reg.value = _(handled ? ba : val.data());
        }
    }
    registerHandler()->setAndMarkRegisters(registers);
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

bool GdbEngine::showToolTip()
{
    const QByteArray iname = tooltipIName(m_toolTipExpression);

    if (!debuggerCore()->boolSetting(UseToolTipsInMainEditor)) {
        watchHandler()->removeData(iname);
        return true;
    }

    const QModelIndex index = watchHandler()->itemIndex(iname);
    if (!index.isValid()) {
        watchHandler()->removeData(iname);
        hideDebuggerToolTip();
        return false;
    }
    showDebuggerToolTip(m_toolTipPos, watchHandler()->model(TooltipsWatch), index, m_toolTipExpression);
    return true;
}

void GdbEngine::setToolTipExpression(const QPoint &mousePos,
    TextEditor::ITextEditor *editor, int cursorPos)
{
    if (state() != InferiorStopOk || !isCppEditor(editor)) {
        //qDebug() << "SUPPRESSING DEBUGGER TOOLTIP, INFERIOR NOT STOPPED/Non Cpp editor";
        return;
    }

    if (debuggerCore()->boolSetting(DebugDebuggingHelpers)) {
        // minimize interference
        return;
    }

    m_toolTipPos = mousePos;
    int line, column;
    QString exp = cppExpressionAt(editor, cursorPos, &line, &column);

    // Extract the first identifier, everything else is considered
    // too dangerous.
    int pos1 = 0, pos2 = exp.size();
    bool inId = false;
    for (int i = 0; i != exp.size(); ++i) {
        const QChar c = exp.at(i);
        const bool isIdChar = c.isLetterOrNumber() || c.unicode() == '_';
        if (inId && !isIdChar) {
            pos2 = i;
            break;
        }
        if (!inId && isIdChar) {
            inId = true;
            pos1 = i;
        }
    }
    exp = exp.mid(pos1, pos2 - pos1);

    if (!exp.isEmpty() && exp == m_toolTipExpression) {
        showToolTip();
        return;
    }

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

    if (isSynchronous()) {
        updateLocals(QVariant());
        return;
    }

    WatchData toolTip;
    toolTip.exp = exp.toLatin1();
    toolTip.name = exp;
    toolTip.iname = tooltipIName(exp);
    watchHandler()->removeData(toolTip.iname);
    watchHandler()->insertData(toolTip);
}


//////////////////////////////////////////////////////////////////////
//
// Watch specific stuff
//
//////////////////////////////////////////////////////////////////////

void GdbEngine::reloadLocals()
{
    setTokenBarrier();
    updateLocals();
}

bool GdbEngine::hasDebuggingHelperForType(const QByteArray &type) const
{
    if (!debuggerCore()->boolSetting(UseDebuggingHelpers))
        return false;

    if (m_gdbAdapter->dumperHandling() == AbstractGdbAdapter::DumperNotAvailable) {
        // "call" is not possible in gdb when looking at core files
        return type == "QString" || type.endsWith("::QString")
            || type == "QStringList" || type.endsWith("::QStringList");
    }

    if (debuggerCore()->boolSetting(DebugDebuggingHelpers)
            && stackHandler()->isDebuggingDebuggingHelpers())
        return false;

    if (m_debuggingHelperState != DebuggingHelperAvailable)
        return false;

    // simple types
    return m_dumperHelper.type(type) != QtDumperHelper::UnknownType;
}


void GdbEngine::updateWatchData(const WatchData &data, const WatchUpdateFlags &flags)
{
    if (isSynchronous()) {
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
            showMessage(_("<Breaking endless loop for " + data.iname + '>'),
                LogMiscInput);
            data1.setAllUnneeded();
            data1.setValue(_("<unavailable>"));
            data1.setHasChildren(false);
            insertData(data1);
            return;
        }
        m_processedNames.insert(processedName);

        // FIXME: Is this sufficient when "external" changes are
        // triggered e.g. by manually entered command in the gdb console?
        //qDebug() << "TRY PARTIAL: " << flags.tryIncremental
        //        << hasPython()
        //        << (m_pendingWatchRequests == 0)
        //        << (m_pendingBreakpointRequests == 0);

        bool tryPartial = flags.tryIncremental
                && hasPython()
                && m_pendingWatchRequests == 0
                && m_pendingBreakpointRequests == 0;

        if (tryPartial)
            updateLocalsPython(true, data.iname);
        else
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
#    if DEBUG_PENDING
    //qDebug() << "##############################################";
    qDebug() << "UPDATE MODEL, FOUND INCOMPLETE:";
    //qDebug() << data.toString();
#    endif

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
    if (!isSynchronous())
        m_processedNames.clear();
    PENDING_DEBUG("REBUILDING MODEL" << count);
    if (debuggerCore()->boolSetting(LogTimeStamps))
        showMessage(LogWindow::logTimeStamp(), LogMiscInput);
    showMessage(_("<Rebuild Watchmodel %1>").arg(count), LogMiscInput);
    showStatusMessage(tr("Finished retrieving data"), 400);
    watchHandler()->endCycle();
    showToolTip();
}

static QByteArray arrayFillCommand(const char *array, const QByteArray &params)
{
    QString buf;
    buf.sprintf("set {char[%d]} &%s = {", params.size(), array);
    QByteArray encoded;
    encoded.append(buf.toLocal8Bit());
    const int size = params.size();
    for (int i = 0; i != size; ++i) {
        buf.sprintf("%d,", int(params[i]));
        encoded.append(buf.toLocal8Bit());
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
    showMessage(QString::fromUtf8(params), LogMiscInput);

    params.clear();
    params.append('\0');
    const QByteArray outBufferCmd = arrayFillCommand("qDumpOutBuffer", params);

    postCommand(inBufferCmd);
    postCommand(outBufferCmd);
}

void GdbEngine::handleVarAssign(const GdbResponse &)
{
    // Everything might have changed, force re-evaluation.
    setTokenBarrier();
    updateLocals();
}

void GdbEngine::handleVarCreate(const GdbResponse &response)
{
    WatchData data = response.cookie.value<WatchData>();
    // Happens e.g. when we already issued a var-evaluate command.
    if (!data.isValid())
        return;
    //qDebug() << "HANDLE VARIABLE CREATION:" << data.toString();
    if (response.resultClass == GdbResultDone) {
        data.variable = data.iname;
        setWatchDataType(data, response.data.findChild("type"));
        if (watchHandler()->isExpandedIName(data.iname)
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
            data.type = " ";
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
    if (response.resultClass == GdbResultDone) {
    } else {
        QString msg = QString::fromLocal8Bit(response.data.findChild("msg").data());
        showStatusMessage(tr("Custom dumper setup: %1").arg(msg), 10000);
    }
}

void GdbEngine::updateLocals(const QVariant &cookie)
{
    m_pendingWatchRequests = 0;
    m_pendingBreakpointRequests = 0;
    if (hasPython())
        updateLocalsPython(false, QByteArray());
    else
        updateLocalsClassic(cookie);
}


// Parse a local variable from GdbMi.
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
        data.name = WatchData::shadowedName(nam, n);
        if (uninitializedVariables.contains(data.name)) {
            data.setError(WatchData::msgNotInScope());
            return data;
        }
        setWatchDataValue(data, item);
        //: Type of local variable or parameter shadowed by another
        //: variable of the same name in a nested block.
        data.setType(GdbEngine::tr("<shadowed>").toUtf8());
        data.setHasChildren(false);
        return data;
    }
    seen->insert(name, 1);
    WatchData data;
    QString nam = _(name);
    data.iname = "local." + name;
    data.name = nam;
    data.exp = name;
    setWatchDataType(data, item.findChild("type"));
    if (uninitializedVariables.contains(data.name)) {
        data.setError(WatchData::msgNotInScope());
        return data;
    }
    if (isSynchronous()) {
        setWatchDataValue(data, item);
        // We know that the complete list of children is
        // somewhere in the response.
        data.setChildrenUnneeded();
    } else {
        // Set value only directly if it is simple enough, otherwise
        // pass through the insertData() machinery.
        if (isIntOrFloatType(data.type) || isPointerType(data.type))
            setWatchDataValue(data, item);
        if (isSymbianIntType(data.type)) {
            setWatchDataValue(data, item);
            data.setHasChildren(false);
        }
    }

    if (!watchHandler()->isExpandedIName(data.iname))
        data.setChildrenUnneeded();

    GdbMi t = item.findChild("numchild");
    if (t.isValid())
        data.setHasChildren(t.data().toInt() > 0);
    else if (isPointerType(data.type) || data.name == __("this"))
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
    watchHandler()->insertData(data);
}

void GdbEngine::assignValueInDebugger(const WatchData *,
    const QString &expression, const QVariant &value)
{
    postCommand("-var-delete assign");
    postCommand("-var-create assign * " + expression.toLatin1());
    postCommand("-var-assign assign " + GdbMi::escapeCString(value.toString().toLatin1()),
        Discardable, CB(handleVarAssign));
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
        QByteArray ns = m_dumperHelper.qtNamespace();
        QByteArray type = ns.isEmpty() ? "QWidget*" : ("'" + ns + "QWidget'*");
        QString exp = _("(*(%1)%2)").arg(_(type)).arg(addr);
        watchHandler()->watchExpression(exp);
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

class DisassemblerAgentCookie
{
public:
    DisassemblerAgentCookie() : agent(0), attempts(0) {}
    DisassemblerAgentCookie(DisassemblerViewAgent *agent_)
        : agent(agent_), attempts(0)
    {}

public:
    QPointer<DisassemblerViewAgent> agent;
    int attempts;
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

void GdbEngine::fetchDisassemblerByAddress(const DisassemblerAgentCookie &ac0,
    bool useMixedMode)
{
    DisassemblerAgentCookie ac = ac0;
    QTC_ASSERT(ac.agent, return);
    const quint64 address = ac.agent->address();
    QByteArray start = QByteArray::number(address - 20, 16);
    QByteArray end = QByteArray::number(address + 100, 16);
    // -data-disassemble [ -s start-addr -e end-addr ]
    //  | [ -f filename -l linenum [ -n lines ] ] -- mode
    ++ac.attempts;
    if (useMixedMode)
        postCommand("-data-disassemble -s 0x" + start + " -e 0x" + end + " -- 1",
            Discardable, CB(handleFetchDisassemblerByAddress1),
            QVariant::fromValue(ac));
    else
        postCommand("-data-disassemble -s 0x" + start + " -e 0x" + end + " -- 0",
            Discardable, CB(handleFetchDisassemblerByAddress0),
            QVariant::fromValue(ac));
}

void GdbEngine::fetchDisassemblerByCli(const DisassemblerAgentCookie &ac0,
    bool useMixedMode)
{
    DisassemblerAgentCookie ac = ac0;
    QTC_ASSERT(ac.agent, return);
    const quint64 address = ac.agent->address();
    QByteArray cmd = "disassemble ";
    if (useMixedMode && m_gdbVersion >= 60850)
        cmd += "/m ";
    cmd += " 0x";
    cmd += QByteArray::number(address, 16);
    ++ac.attempts;
    postCommand(cmd, Discardable, CB(handleFetchDisassemblerByCli),
        QVariant::fromValue(ac));
}

void GdbEngine::fetchDisassemblerByAddressCli(const DisassemblerAgentCookie &ac0)
{
    DisassemblerAgentCookie ac = ac0;
    QTC_ASSERT(ac.agent, return);
    const quint64 address = ac.agent->address();
    QByteArray start = QByteArray::number(address - 20, 16);
    QByteArray end = QByteArray::number(address + 100, 16);
    // There have been changes to the syntax sometime between 7.0 and 7.1.
    // As it is unclear how a given incarnation of gdb behaves without
    // actually calling the command, try both.
    const char sep = (ac.attempts % 2) ? ',' : ' ';
    QByteArray cmd = "disassemble 0x" + start + sep + "0x" + end;
    ++ac.attempts;
    postCommand(cmd, Discardable, CB(handleFetchDisassemblerByCli),
        QVariant::fromValue(ac));
}

static QByteArray parseLine(const GdbMi &line)
{
    QByteArray ba;
    ba.reserve(200);
    QByteArray address = line.findChild("address").data();
    //QByteArray funcName = line.findChild("func-name").data();
    //QByteArray offset = line.findChild("offset").data();
    QByteArray inst = line.findChild("inst").data();
    ba += address;
    ba += QByteArray(15 - address.size(), ' ');
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
            // Mixed mode.
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
            // The non-mixed version.
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
            fetchDisassemblerByAddress(ac, true);
        else if (lines.children().size() == 1
                    && lines.childAt(0).findChild("line").data() == "0")
            fetchDisassemblerByAddress(ac, true);
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
                fetchDisassemblerByCli(ac, true);
            }
        }
    } else {
        // 536^error,msg="mi_cmd_disassemble: Invalid line number"
        QByteArray msg = response.data.findChild("msg").data();
        if (msg == "mi_cmd_disassemble: Invalid line number"
                || msg.startsWith("Cannot access memory at address"))
            fetchDisassemblerByAddress(ac, true);
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
            fetchDisassemblerByAddress(ac, false);
        else {
            QString contents = parseDisassembler(lines);
            if (ac.agent->contentsCoversAddress(contents)) {
                ac.agent->setContents(parseDisassembler(lines));
            } else {
                showMessage(_("FALL BACK TO NON-MIXED"));
                fetchDisassemblerByAddress(ac, false);
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
        // Apple's gdb produces MI output even for CLI commands.
        // FIXME: Check whether wrapping this into -interpreter-exec console
        // (i.e. usgind the 'ConsoleCommand' GdbCommandFlag makes a
        // difference.
        GdbMi lines = response.data.findChild("asm_insns");
        if (lines.isValid()) {
            ac.agent->setContents(parseDisassembler(lines));
        } else {
            const QByteArray someSpace = "        ";
            // First line is something like
            // "Dump of assembler code from 0xb7ff598f to 0xb7ff5a07:"
            GdbMi output = response.data.findChild("consolestreamoutput");
            QByteArray res;
            foreach (const QByteArray &line0, output.data().split('\n')) {
                QByteArray line = line0.trimmed();
                if (line.startsWith("=> "))
                    line = line.mid(3);
                if (line.startsWith("Current language:"))
                    continue;
                if (line.startsWith("The current source"))
                    continue;
                if (line.startsWith("End of assembler"))
                    continue;
                if (line.startsWith("0x")) {
                    res.append(line);
                    res.append('\n');
                    continue;
                }
                res.append(someSpace);
                res.append(line);
                res.append('\n');
            }
            if (res.size() > 1)
                ac.agent->setContents(_(res));
            else
                fetchDisassemblerByAddressCli(ac);
        }
    } else {
        QByteArray msg = response.data.findChild("msg").data();
        //76^error,msg="No function contains program counter for selected..."
        //76^error,msg="No function contains specified address."
        //>568^error,msg="Line number 0 out of range;
        if (ac.attempts < 4 // Try once more.
              && (msg.startsWith("No function ") || msg.startsWith("Line number ")))
            fetchDisassemblerByAddressCli(ac);
        else
            showStatusMessage(tr("Disassembler failed: %1")
                .arg(QString::fromLocal8Bit(msg)), 5000);
    }
}

//
// Starting up & shutting down
//

bool GdbEngine::startGdb(const QStringList &args, const QString &gdb, const QString &settingsIdHint)
{
    gdbProc()->disconnect(); // From any previous runs

    m_gdb = QString::fromLocal8Bit(qgetenv("QTC_DEBUGGER_PATH"));
    if (m_gdb.isEmpty() && startParameters().startMode != StartRemoteGdb)
        m_gdb = debuggerCore()->gdbBinaryForToolChain(startParameters().toolChainType);
    if (m_gdb.isEmpty())
        m_gdb = gdb;
    if (m_gdb.isEmpty()) {
        handleAdapterStartFailed(
            msgNoBinaryForToolChain(startParameters().toolChainType),
            GdbOptionsPage::settingsId());
        return false;
    }
    showMessage(_("STARTING GDB ") + m_gdb);
    QStringList gdbArgs;
    gdbArgs << _("-i");
    gdbArgs << _("mi");
    gdbArgs += args;
#ifdef Q_OS_WIN
    // Set python path. By convention, python is located below gdb executable.
    // Extend the environment set on the process in startAdapter().
    const QFileInfo fi(m_gdb);
    bool foundPython = false;
    if (fi.isAbsolute()) {
        const QString winPythonVersion = QLatin1String(winPythonVersionC);
        const QDir dir = fi.absoluteDir();
        if (dir.exists(winPythonVersion)) {
            QProcessEnvironment environment = gdbProc()->processEnvironment();
            const QString pythonPathVariable = QLatin1String("PYTHONPATH");
            // Check for existing values.
            if (environment.contains(pythonPathVariable)) {
                const QString oldPythonPath = environment.value(pythonPathVariable);
                showMessage(_("Using existing python path: %1")
                    .arg(oldPythonPath), LogMisc);
            } else {
                const QString pythonPath =
                    QDir::toNativeSeparators(dir.absoluteFilePath(winPythonVersion));
                environment.insert(pythonPathVariable, pythonPath);
                showMessage(_("Python path: %1").arg(pythonPath), LogMisc);
                gdbProc()->setProcessEnvironment(environment);
            }
            foundPython = true;
        }
    }
    if (!foundPython) {
        showMessage(_("UNSUPPORTED GDB %1 DOES NOT HAVE PYTHON.").arg(m_gdb));
        showStatusMessage(_("Gdb at %1 does not have python").arg(m_gdb));
    }
#endif

    connect(gdbProc(), SIGNAL(error(QProcess::ProcessError)),
        SLOT(handleGdbError(QProcess::ProcessError)));
    connect(gdbProc(), SIGNAL(finished(int, QProcess::ExitStatus)),
        SLOT(handleGdbFinished(int, QProcess::ExitStatus)));
    connect(gdbProc(), SIGNAL(readyReadStandardOutput()),
        SLOT(readGdbStandardOutput()));
    connect(gdbProc(), SIGNAL(readyReadStandardError()),
        SLOT(readGdbStandardError()));

    gdbProc()->start(m_gdb, gdbArgs);

    if (!gdbProc()->waitForStarted()) {
        const QString msg = errorMessage(QProcess::FailedToStart);
        handleAdapterStartFailed(msg, settingsIdHint);
        return false;
    }

    showMessage(_("GDB STARTED, INITIALIZING IT"));
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

    if (false && m_isMacGdb) {
        // FIXME: m_isMacGdb is only known after handleShowVersion!
        // also, setting load-rules seems to be only possible after 'file'
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
    } else {
        // Work around http://bugreports.qt.nokia.com/browse/QTCREATORBUG-2004
        postCommand("maintenance set internal-warning quit no", ConsoleCommand);
        postCommand("maintenance set internal-error quit no", ConsoleCommand);

        // We know that we don't have Python on Mac.
        loadPythonDumpers();
    }

    QString scriptFileName = debuggerCore()->stringSetting(GdbScriptFile);
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

    return true;
}

void GdbEngine::loadPythonDumpers()
{
    const QByteArray dumperSourcePath =
        Core::ICore::instance()->resourcePath().toLocal8Bit() + "/gdbmacros/";

    postCommand("python execfile('" + dumperSourcePath + "dumper.py')",
        ConsoleCommand|NonCriticalResponse);
    postCommand("python execfile('" + dumperSourcePath + "gdbmacros.py')",
        ConsoleCommand|NonCriticalResponse);
    postCommand("bbsetup",
        ConsoleCommand, CB(handleHasPython));
}

void GdbEngine::handleGdbError(QProcess::ProcessError error)
{
    const QString msg = errorMessage(error);
    showMessage(_("HANDLE GDB ERROR: ") + msg);
    // Show a message box for asynchronously reported issues.
    switch (error) {
    case QProcess::FailedToStart:
        // This should be handled by the code trying to start the process.
        break;
    case QProcess::Crashed:
        // This will get a processExited() as well.
        break;
    case QProcess::ReadError:
    case QProcess::WriteError:
    case QProcess::Timedout:
    default:
        //gdbProc()->kill();
        //notifyEngineIll();
        showMessageBox(QMessageBox::Critical, tr("Gdb I/O Error"), msg);
        break;
    }
}

void GdbEngine::handleGdbFinished(int code, QProcess::ExitStatus type)
{
    if (m_commandTimer.isActive())
        m_commandTimer.stop();

    showMessage(_("GDB PROCESS FINISHED, status %1, code %2").arg(type).arg(code));

    switch (state()) {
    case EngineShutdownRequested:
        notifyEngineShutdownOk();
        break;
    case InferiorRunOk:
        // This could either be a real gdb crash or a quickly exited inferior
        // in the terminal adapter. In this case the stub proc will die soon,
        // too, so there's no need to act here.
        showMessage(_("The gdb process exited somewhat unexpectedly."));
        notifyEngineSpontaneousShutdown();
        break;
    default: {
        notifyEngineIll(); // Initiate shutdown sequence
        const QString msg = type == QProcess::CrashExit ?
                    tr("The gdb process crashed.") :
                    tr("The gdb process exited unexpectedly (code %1)").arg(code);
        showMessageBox(QMessageBox::Critical, tr("Unexpected Gdb Exit"), msg);
        break;
    }
    }
}

void GdbEngine::handleAdapterStartFailed(const QString &msg, const QString &settingsIdHint)
{
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());
    showMessage(_("ADAPTER START FAILED"));
    if (!msg.isEmpty()) {
        const QString title = tr("Adapter start failed");
        if (settingsIdHint.isEmpty()) {
            Core::ICore::instance()->showWarningWithOptions(title, msg);
        } else {
            Core::ICore::instance()->showWarningWithOptions(title, msg, QString(),
                _(Debugger::Constants::DEBUGGER_SETTINGS_CATEGORY), settingsIdHint);
        }
    }
    notifyEngineSetupFailed();
}

void GdbEngine::handleAdapterStarted()
{
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());
    showMessage(_("ADAPTER SUCCESSFULLY STARTED"));
    notifyEngineSetupOk();
}

void GdbEngine::setupInferior()
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    showStatusMessage(tr("Setting up inferior..."));
    m_gdbAdapter->setupInferior();
}

void GdbEngine::notifyInferiorSetupFailed()
{
    // FIXME: that's not enough to stop gdb from getting confused
    // by a timeout of the adapter.
    //resetCommandQueue();
    DebuggerEngine::notifyInferiorSetupFailed();
}

void GdbEngine::handleInferiorPrepared()
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    const QByteArray qtInstallPath = startParameters().qtInstallPath.toLocal8Bit();
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
    showMessage(tr("Setting breakpoints..."));
    attemptBreakpointSynchronization();

    if (m_cookieForToken.isEmpty()) {
        finishInferiorSetup();
    } else {
        QTC_ASSERT(m_commandsDoneCallback == 0, /**/);
        m_commandsDoneCallback = &GdbEngine::finishInferiorSetup;
    }
}

void GdbEngine::finishInferiorSetup()
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    notifyInferiorSetupOk();
}

void GdbEngine::runEngine()
{
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state());
    m_gdbAdapter->runEngine();
}

void GdbEngine::notifyInferiorSetupFailed(const QString &msg)
{
    showStatusMessage(tr("Failed to start application: ") + msg);
    if (state() == EngineSetupFailed) {
        showMessage(_("INFERIOR START FAILED, BUT ADAPTER DIED ALREADY"));
        return; // Adapter crashed meanwhile, so this notification is meaningless.
    }
    showMessage(_("INFERIOR START FAILED"));
    showMessageBox(QMessageBox::Critical, tr("Failed to start application"), msg);
    DebuggerEngine::notifyInferiorSetupFailed();
}

void GdbEngine::handleAdapterCrashed(const QString &msg)
{
    showMessage(_("ADAPTER CRASHED"));

    // The adapter is expected to have cleaned up after itself when we get here,
    // so the effect is about the same as AdapterStartFailed => use it.
    // Don't bother with state transitions - this can happen in any state and
    // the end result is always the same, so it makes little sense to find a
    // "path" which does not assert.
    notifyEngineSetupFailed();

    // No point in being friendly here ...
    gdbProc()->kill();

    if (!msg.isEmpty())
        showMessageBox(QMessageBox::Critical, tr("Adapter crashed"), msg);
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

void GdbEngine::createFullBacktrace()
{
    postCommand("thread apply all bt full",
        NeedsStop|ConsoleCommand, CB(handleCreateFullBacktrace));
}

void GdbEngine::handleCreateFullBacktrace(const GdbResponse &response)
{
    if (response.resultClass == GdbResultDone) {
        debuggerCore()->openTextEditor(_("Backtrace $"),
            _(response.data.findChild("consolestreamoutput").data()));
    }
}

void GdbEngine::resetCommandQueue()
{
    m_commandTimer.stop();
    if (!m_cookieForToken.isEmpty()) {
        QString msg;
        QTextStream ts(&msg);
        ts << "RESETING COMMAND QUEUE. LEFT OVER TOKENS: ";
        foreach (const GdbCommand &cookie, m_cookieForToken)
            ts << "CMD:" << cookie.command << cookie.callbackName;
        m_cookieForToken.clear();
        showMessage(msg);
    }
}

void GdbEngine::removeTooltip()
{
    m_toolTipExpression.clear();
    m_toolTipPos = QPoint();
    DebuggerEngine::removeTooltip();
}

//
// Factory
//

DebuggerEngine *createGdbEngine(const DebuggerStartParameters &startParameters)
{
    return new GdbEngine(startParameters);
}

void addGdbOptionPages(QList<Core::IOptionsPage *> *opts)
{
    opts->push_back(new GdbOptionsPage());
}

} // namespace Internal
} // namespace Debugger

Q_DECLARE_METATYPE(Debugger::Internal::MemoryAgentCookie);
Q_DECLARE_METATYPE(Debugger::Internal::DisassemblerAgentCookie);
Q_DECLARE_METATYPE(Debugger::Internal::GdbMi);
