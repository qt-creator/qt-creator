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

#define QT_NO_CAST_FROM_ASCII

#include "gdbengine.h"
#include "gdboptionspage.h"
#include "trkoptions.h"
#include "trkoptionspage.h"

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
#include "stackhandler.h"
#include "watchhandler.h"
#include "sourcefileswindow.h"

#include "debuggerdialogs.h"

#include <utils/qtcassert.h>
#include <utils/fancymainwindow.h>
#include <texteditor/itexteditor.h>
#include <projectexplorer/toolchain.h>
#include <coreplugin/icore.h>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QMetaObject>
#include <QtCore/QTime>
#include <QtCore/QTimer>
#include <QtCore/QTextStream>

#include <QtGui/QAction>
#include <QtCore/QCoreApplication>
#include <QtGui/QLabel>
#include <QtGui/QMainWindow>
#include <QtGui/QMessageBox>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QPushButton>
#ifdef Q_OS_WIN
#    include "shared/sharedlibraryinjector.h"
#endif

#ifdef Q_OS_UNIX
#include <unistd.h>
#include <dlfcn.h>
#endif
#include <ctype.h>

namespace Debugger {
namespace Internal {

//#define DEBUG_PENDING  1
//#define DEBUG_SUBITEM  1

#if DEBUG_PENDING
#   define PENDING_DEBUG(s) qDebug() << s
#else
#   define PENDING_DEBUG(s)
#endif
#define PENDING_DEBUGX(s) qDebug() << s

#define CB(callback) &GdbEngine::callback, STRINGIFY(callback)

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

// reads a MI-encoded item frome the consolestream
static bool parseConsoleStream(const GdbResponse &response, GdbMi *contents)
{
    GdbMi output = response.data.findChild("consolestreamoutput");
    QByteArray out = output.data();

    int markerPos = out.indexOf('"') + 1; // position of 'success marker'
    if (markerPos == 0 || out.at(markerPos) == 'f') {  // 't' or 'f'
        // custom dumper produced no output
        return false;
    }

    out = out.mid(markerPos +  1);
    out = out.left(out.lastIndexOf('"'));
    // optimization: dumper output never needs real C unquoting
    out.replace('\\', "");
    out = "dummy={" + out + "}";

    contents->fromString(out);
    //qDebug() << "CONTENTS" << contents->toString(true);
    return contents->isValid();
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

GdbEngine::GdbEngine(DebuggerManager *manager) :
    IDebuggerEngine(manager),
#ifdef Q_OS_WIN // Do injection loading with MinGW (call loading does not work with 64bit)
    m_dumperInjectionLoad(true)
#else
    m_dumperInjectionLoad(false)
#endif
{
    m_trkOptions = QSharedPointer<TrkOptions>(new TrkOptions);
    m_trkOptions->fromSettings(Core::ICore::instance()->settings());
    m_gdbAdapter = 0;

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
            this, SLOT(setDebugDebuggingHelpers(QVariant)));
    connect(theDebuggerAction(RecheckDebuggingHelpers), SIGNAL(triggered()),
            this, SLOT(recheckDebuggingHelperAvailability()));
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
    return m_manager->mainWindow();
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
    m_isSynchroneous = false;
    m_registerNamesListed = false;

    m_fullToShortName.clear();
    m_shortToFullName.clear();
    m_varToType.clear();

    m_modulesListOutdated = m_sourcesListOutdated = true;
    m_sourcesListUpdating = false;
    m_oldestAcceptableToken = -1;
    m_outputCodec = QTextCodec::codecForLocale();
    m_pendingRequests = 0;
    m_commandsDoneCallback = 0;
    m_commandsToRunOnTemporaryBreak.clear();
    m_cookieForToken.clear();

    m_pendingConsoleStreamOutput.clear();
    m_pendingLogStreamOutput.clear();

    m_inbuffer.clear();

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
    gdbOutputAvailable(LogDebug, msg);
}

void GdbEngine::handleResponse(const QByteArray &buff)
{
    static QTime lastTime;

    if (theDebuggerBoolSetting(LogTimeStamps))
        gdbOutputAvailable(LogTime, currentTime());
    gdbOutputAvailable(LogOutput, QString::fromLocal8Bit(buff, buff.length()));

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
            } else if (asyncClass == "running") {
                // Archer has 'thread-id="all"' here
            } else if (asyncClass == "library-loaded") {
                // Archer has 'id="/usr/lib/libdrm.so.2",
                // target-name="/usr/lib/libdrm.so.2",
                // host-name="/usr/lib/libdrm.so.2",
                // symbols-loaded="0"
                QByteArray id = result.findChild("id").data();
                if (!id.isEmpty())
                    showStatusMessage(tr("Library %1 loaded.").arg(_(id)));
                m_modulesListOutdated = m_sourcesListOutdated = true;
            } else if (asyncClass == "library-unloaded") {
                // Archer has 'id="/usr/lib/libdrm.so.2",
                // target-name="/usr/lib/libdrm.so.2",
                // host-name="/usr/lib/libdrm.so.2"
                QByteArray id = result.findChild("id").data();
                showStatusMessage(tr("Library %1 unloaded.").arg(_(id)));
                m_modulesListOutdated = m_sourcesListOutdated = true;
            } else if (asyncClass == "thread-group-created") {
                // Archer has "{id="28902"}" 
                QByteArray id = result.findChild("id").data();
                showStatusMessage(tr("Thread group %1 created.").arg(_(id)));
                int pid = id.toInt();
                if (pid != inferiorPid())
                    handleInferiorPidChanged(pid);
            } else if (asyncClass == "thread-created") {
                //"{id="1",group-id="28902"}" 
                QByteArray id = result.findChild("id").data();
                showStatusMessage(tr("Thread %1 created.").arg(_(id)));
            } else if (asyncClass == "thread-group-exited") {
                // Archer has "{id="28902"}" 
                QByteArray id = result.findChild("id").data();
                showStatusMessage(tr("Thread group %1 exited.").arg(_(id)));
            } else if (asyncClass == "thread-exited") {
                //"{id="1",group-id="28902"}" 
                QByteArray id = result.findChild("id").data();
                QByteArray groupid = result.findChild("group-id").data();
                showStatusMessage(tr("Thread %1 in group %2 exited.")
                    .arg(_(id)).arg(_(groupid)));
            } else if (asyncClass == "thread-selected") {
                QByteArray id = result.findChild("id").data();
                showStatusMessage(tr("Thread %1 selected.").arg(_(id)));
                //"{id="2"}" 
            #if defined(Q_OS_MAC)
            } else if (asyncClass == "shlibs-updated") {
                // MAC announces updated libs
                m_modulesListOutdated = m_sourcesListOutdated = true;
            } else if (asyncClass == "shlibs-added") {
                // MAC announces added libs
                // {shlib-info={num="2", name="libmathCommon.A_debug.dylib",
                // kind="-", dyld-addr="0x7f000", reason="dyld", requested-state="Y",
                // state="Y", path="/usr/lib/system/libmathCommon.A_debug.dylib",
                // description="/usr/lib/system/libmathCommon.A_debug.dylib",
                // loaded_addr="0x7f000", slide="0x7f000", prefix=""}}
                m_modulesListOutdated = m_sourcesListOutdated = true;
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
                m_modulesListOutdated = m_sourcesListOutdated = true;
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
    if (m_dumperInjectionLoad)
        tryLoadDebuggingHelpers();
}

void GdbEngine::postCommand(const QString &command, AdapterCallback callback,
                            const char *callbackName, const QVariant &cookie)
{
    postCommand(command, NoFlags, callback, callbackName, cookie);
}

void GdbEngine::postCommand(const QString &command, GdbCommandFlags flags,
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

void GdbEngine::postCommand(const QString &command, GdbCommandCallback callback,
                            const char *callbackName, const QVariant &cookie)
{
    postCommand(command, NoFlags, callback, callbackName, cookie);
}

void GdbEngine::postCommand(const QString &command, GdbCommandFlags flags,
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
        PENDING_DEBUG(_("NO GDB PROCESS RUNNING, CMD IGNORED: ") + cmd.command);
        debugMessage(_("NO GDB PROCESS RUNNING, CMD IGNORED: %1 %2")
            .arg(cmd.command).arg(state()));
        return;
    }

    if (cmd.flags & RebuildModel) {
        ++m_pendingRequests;
        PENDING_DEBUG("   MODEL:" << cmd.command << "=>" << cmd.callbackName
                      << "INCREMENTS PENDING TO" << m_pendingRequests);
    } else {
        PENDING_DEBUG("   OTHER (IN):" << cmd.command << "=>" << cmd.callbackName
                      << "LEAVES PENDING AT" << m_pendingRequests);
    }

    if ((cmd.flags & NeedsStop) || !m_commandsToRunOnTemporaryBreak.isEmpty()) {
        if (state() == InferiorStopped
            || state() == InferiorStarting || state() == AdapterStarted) {
            // Can be safely sent now.
            flushCommand(cmd);
        } else {
            // Queue the commands that we cannot send at once.
            debugMessage(_("QUEUING COMMAND ") + cmd.command);
            m_commandsToRunOnTemporaryBreak.append(cmd);
            if (state() == InferiorStopping) {
                if (cmd.flags & LosesChild)
                    setState(InferiorStopping_Kill);
                debugMessage(_("CHILD ALREADY BEING INTERRUPTED"));
            } else if (state() == InferiorStopping_Kill) {
                debugMessage(_("CHILD ALREADY BEING INTERRUPTED (KILL PENDING)"));
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
        debugMessage(_("RUNNING QUEUED COMMAND %1 %2")
            .arg(cmd.command).arg(_(cmd.callbackName)));
        flushCommand(cmd);
    }
}

void GdbEngine::flushCommand(const GdbCommand &cmd0)
{
    GdbCommand cmd = cmd0;
    if (state() == DebuggerNotReady) {
        gdbInputAvailable(LogInput, cmd.command);
        debugMessage(_("GDB PROCESS NOT RUNNING, PLAIN CMD IGNORED: ") + cmd.command);
        return;
    }

    ++currentToken();
    cmd.postTime = QTime::currentTime();
    m_cookieForToken[currentToken()] = cmd;
    cmd.command = QString::number(currentToken()) + cmd.command;
    if (cmd.flags & EmbedToken)
        cmd.command = cmd.command.arg(currentToken());
    gdbInputAvailable(LogInput, cmd.command);

    m_gdbAdapter->write(cmd.command.toLatin1() + "\r\n");

    if (cmd.flags & LosesChild)
        setState(InferiorShuttingDown);
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
                nextIExec();
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
        gdbOutputAvailable(LogTime, _("Response time: %1: %2 s")
            .arg(cmd.command)
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
        QString rsp = _(GdbResponse::stringFromResultClass(response->resultClass));
        qWarning() << "UNEXPECTED RESPONSE " << rsp << " TO COMMAND" << cmd.command << " AT " __FILE__ ":" STRINGIFY(__LINE__);
        debugMessage(_("UNEXPECTED RESPONSE %1 TO COMMAND %2").arg(rsp).arg(cmd.command));
    } else {
        if (cmd.callback)
            (this->*cmd.callback)(*response);
        else if (cmd.adapterCallback)
            (m_gdbAdapter->*cmd.adapterCallback)(*response);
    }

    if (cmd.flags & RebuildModel) {
        --m_pendingRequests;
        PENDING_DEBUG("   WATCH" << cmd.command << "=>" << cmd.callbackName
                      << "DECREMENTS PENDING TO" << m_pendingRequests);
        if (m_pendingRequests <= 0) {
            PENDING_DEBUG("\n\n ... AND TRIGGERS MODEL UPDATE\n");
            rebuildModel();
        }
    } else {
        PENDING_DEBUG("   OTHER (OUT):" << cmd.command << "=>" << cmd.callbackName
                      << "LEAVES PENDING AT" << m_pendingRequests);
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
    QTC_ASSERT(state() == InferiorUnrunnable || state() == InferiorStopped, /**/);
    tryLoadDebuggingHelpers();
    reloadModulesInternal();
    postCommand(_("-stack-list-frames"), WatchUpdate, CB(handleStackListFrames),
        QVariant::fromValue<StackCookie>(StackCookie(false, true)));
    manager()->stackHandler()->setCurrentIndex(0);
    if (supportsThreads())
        postCommand(_("-thread-list-ids"), WatchUpdate, CB(handleStackListThreads), 0);
    manager()->reloadRegisters();
    updateLocals(); 
}

void GdbEngine::handleQuerySources(const GdbResponse &response)
{
    if (response.resultClass == GdbResultDone) {
        QMap<QString, QString> oldShortToFull = m_shortToFullName;
        m_shortToFullName.clear();
        m_fullToShortName.clear();
        // "^done,files=[{file="../../../../bin/gdbmacros/gdbmacros.cpp",
        // fullname="/data5/dev/ide/main/bin/gdbmacros/gdbmacros.cpp"},
        GdbMi files = response.data.findChild("files");
        foreach (const GdbMi &item, files.children()) {
            GdbMi fullName = item.findChild("fullname");
            if (fullName.isValid()) {
                QString full = cleanupFullName(QString::fromLocal8Bit(fullName.data()));
                QString fileName = QString::fromLocal8Bit(item.findChild("file").data());
                m_shortToFullName[fileName] = full;
                m_fullToShortName[full] = fileName;
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
    //    postCommand(_("target response"));

    tryLoadDebuggingHelpers();

    #ifndef Q_OS_MAC
    // intentionally after tryLoadDebuggingHelpers(),
    // otherwise we'd interupt solib loading.
    if (theDebuggerBoolSetting(AllPluginBreakpoints)) {
        postCommand(_("set auto-solib-add on"));
        postCommand(_("set stop-on-solib-events 0"));
        postCommand(_("sharedlibrary .*"));
    } else if (theDebuggerBoolSetting(SelectedPluginBreakpoints)) {
        postCommand(_("set auto-solib-add on"));
        postCommand(_("set stop-on-solib-events 1"));
        postCommand(_("sharedlibrary ")
          + theDebuggerStringSetting(SelectedPluginBreakpointsPattern));
    } else if (theDebuggerBoolSetting(NoPluginBreakpoints)) {
        // should be like that already
        if (!m_dumperInjectionLoad)
            postCommand(_("set auto-solib-add off"));
        postCommand(_("set stop-on-solib-events 0"));
    }
    #endif

    // It's nicer to see a bit of the world we live in.
    reloadModulesInternal();
    attemptBreakpointSynchronization();
}
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
            QTC_ASSERT(state() == InferiorStopping, qDebug() << state());
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

#ifdef Q_OS_LINUX
    // For some reason, attaching to a stopped process causes *two* stops
    // when trying to continue (kernel i386 2.6.24-23-ubuntu, gdb 6.8).
    // Interestingly enough, on MacOSX no signal is delivered at all.
    if (!m_entryPoint.isEmpty()) {
        if (reason == "signal-received"
            && data.findChild("signal-name").data() == "SIGSTOP") {
            GdbMi frameData = data.findChild("frame");
            if (frameData.findChild("addr").data() == m_entryPoint) {
                continueInferiorInternal();
                return;
            }
        }
        // We are past the initial stops. No need to waste time on further checks.
        m_entryPoint.clear();
    }
#endif

#if 0
    // The related code (handleAqcuiredInferior()) is disabled as well.
    // When re-enabling, try something to avoid spurious source list updates
    // due to unrelated no-reason stops.
    const QByteArray &msg = data.findChild("consolestreamoutput").data();
    if (msg.contains("Stopped due to shared library event") || reason.isEmpty()) {
        m_modulesListOutdated = m_sourcesListOutdated = true;
        if (theDebuggerBoolSetting(SelectedPluginBreakpoints)) {
            QString dataStr = _(data.toString());
            debugMessage(_("SHARED LIBRARY EVENT: ") + dataStr);
            QString pat = theDebuggerStringSetting(SelectedPluginBreakpointsPattern);
            debugMessage(_("PATTERN: ") + pat);
            postCommand(_("sharedlibrary ") + pat);
            continueInferiorInternal();
            showStatusMessage(tr("Loading %1...").arg(dataStr));
            return;
        }
        // fall through
    }
#endif

    // seen on XP after removing a breakpoint while running
    //  >945*stopped,reason="signal-received",signal-name="SIGTRAP",
    //  signal-meaning="Trace/breakpoint trap",thread-id="2",
    //  frame={addr="0x7c91120f",func="ntdll!DbgUiConnectToDbg",
    //  args=[],from="C:\\WINDOWS\\system32\\ntdll.dll"}
    //if (reason == "signal-received"
    //      && data.findChild("signal-name").data() == "SIGTRAP") {
    //    continueInferiorInternal();
    //    return;
    //}

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
                m_manager->stepOutExec();
                //stepExec();
                return;
            }
            if (isSkippableFunction(funcName, fileName)) {
                //debugMessage(_("SKIPPING ") + funcName);
                ++stepCounter;
                m_manager->stepExec();
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
    if (initHelpers && reason == "signal-received"
            && data.findChild("signal-name").data() != "SIGTRAP")
        initHelpers = false;
    if (isSynchroneous())
        initHelpers = false;
    if (initHelpers) {
        tryLoadDebuggingHelpers();
        QVariant var = QVariant::fromValue<GdbMi>(data);
        postCommand(_("p 4"), CB(handleStop1), var);  // dummy
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
    if (m_modulesListOutdated)
        reloadModulesInternal(); // This is for display only
    if (m_sourcesListOutdated)
        reloadSourceFilesInternal(); // This needs to be done before fullName() may need it

    QByteArray reason = data.findChild("reason").data();
    if (reason == "breakpoint-hit") {
        showStatusMessage(tr("Stopped at breakpoint."));
    } else {
        if (reason == "signal-received"
            && theDebuggerBoolSetting(UseMessageBoxForSignals)) {
            QByteArray name = data.findChild("signal-name").data();
            // Ignore SIGTRAP as they are showing up regularily when
            // stopping debugging.
            if (name != "SIGTRAP") {
                QByteArray meaning = data.findChild("signal-meaning").data();
                QString msg = tr("<p>The inferior stopped because it received a "
                    "signal from the Operating System.<p>"
                    "<table><tr><td>Signal name : </td><td>%1</td></tr>"
                    "<tr><td>Signal meaning : </td><td>%2</td></tr></table>")
                    .arg(name.isEmpty() ? tr(" <Unknown> ") : _(name))
                    .arg(meaning.isEmpty() ? tr(" <Unknown> ") : _(meaning));
                showMessageBox(QMessageBox::Information,
                    tr("Signal received"), msg);
            }
        }

        if (reason.isEmpty())
            showStatusMessage(tr("Stopped."));
        else
            showStatusMessage(tr("Stopped: \"%1\"").arg(_(reason)));
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
        postCommand(_("-thread-list-ids"), WatchUpdate,
            CB(handleStackListThreads), currentId);
    }

    //
    // Registers
    //
    manager()->reloadRegisters();
}

void GdbEngine::handleShowVersion(const GdbResponse &response)
{
    //qDebug () << "VERSION 2:" << response.data.findChild("consolestreamoutput").data();
    //qDebug () << "VERSION:" << response.toString();
    debugMessage(_("VERSION: " + response.toString()));
    if (response.resultClass == GdbResultDone) {
        m_gdbVersion = 100;
        m_gdbBuildVersion = -1;
        m_isMacGdb = false;
        QString msg = QString::fromLocal8Bit(response.data.findChild("consolestreamoutput").data());
        QRegExp supported(_("GNU gdb(.*) (\\d+)\\.(\\d+)(\\.(\\d+))?(-(\\d+))?"));
        if (supported.indexIn(msg) == -1) {
            debugMessage(_("UNSUPPORTED GDB VERSION ") + msg);
#if 0
            QStringList list = msg.split(_c('\n'));
            while (list.size() > 2)
                list.removeLast();
            msg = tr("The debugger you are using identifies itself as:")
                + _("<p><p>") + list.join(_("<br>")) + _("<p><p>")
                + tr("This version is not officially supported by Qt Creator.\n"
                     "Debugging will most likely not work well.\n"
                     "Using gdb 6.7 or later is strongly recommended.");
#if 0
            // ugly, but 'Show again' check box...
            static QErrorMessage *err = new QErrorMessage(mainWindow());
            err->setMinimumSize(400, 300);
            err->showMessage(msg);
#else
            //showMessageBox(QMessageBox::Information, tr("Warning"), msg);
#endif
#endif
        } else {
            m_gdbVersion = 10000 * supported.cap(2).toInt()
                         +   100 * supported.cap(3).toInt()
                         +     1 * supported.cap(5).toInt();
            m_gdbBuildVersion = supported.cap(7).toInt();
            m_isMacGdb = msg.contains(__("Apple version"));
            debugMessage(_("GDB VERSION: %1, BUILD: %2%3").arg(m_gdbVersion)
                .arg(m_gdbBuildVersion).arg(_(m_isMacGdb ? " (APPLE)" : "")));
        }
        //qDebug () << "VERSION 3:" << m_gdbVersion << m_gdbBuildVersion;
    }
}

void GdbEngine::handleIsSynchroneous(const GdbResponse &response)
{
    Q_UNUSED(response);
    if (response.resultClass == GdbResultDone) {
        m_isSynchroneous = true;
    } else {
        m_isSynchroneous = false;
    }
}

void GdbEngine::handleExecContinue(const GdbResponse &response)
{
    if (response.resultClass == GdbResultRunning) {
        // The "running" state is picked up in handleResponse()
        QTC_ASSERT(state() == InferiorRunning, /**/);
    } else {
        if (state() == InferiorRunningRequested_Kill) {
            setState(InferiorStopped);
            m_commandsToRunOnTemporaryBreak.clear();
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
            //stepOutExec();
        } else {
            showMessageBox(QMessageBox::Critical, tr("Execution Error"),
                           tr("Cannot continue debugged process:\n") + QString::fromLocal8Bit(msg));
            m_commandsToRunOnTemporaryBreak.clear();
            shutdown();
        }
    }
}

QString GdbEngine::fullName(const QString &fileName)
{
    if (fileName.isEmpty())
        return QString();
    QTC_ASSERT(!m_sourcesListOutdated, /* */)
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
        // FIXME set some timeout?
        if (m_gdbProc.state() == QProcess::Running) {
            postCommand(_("-gdb-exit"), GdbEngine::ExitRequest, CB(handleGdbExit));
        } else {
            setState(DebuggerNotReady);
        }
        break;
    case InferiorRunningRequested:
    case InferiorRunning:
    case InferiorStopping:
    case InferiorStopped:
        // FIXME set some timeout?
        postCommand(_(m_gdbAdapter->inferiorShutdownCommand()),
                    NeedsStop | LosesChild, CB(handleInferiorShutdown));
        break;
    case AdapterStarted: // We can't get here, really
    case InferiorStartFailed:
    case InferiorShutDown:
    case InferiorShutdownFailed: // Whatever
    case InferiorUnrunnable:
        // FIXME set some timeout?
        postCommand(_("-gdb-exit"), GdbEngine::ExitRequest, CB(handleGdbExit));
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
        QString msg = m_gdbAdapter->msgInferiorStopFailed(_(response.data.findChild("msg").data()));
        showMessageBox(QMessageBox::Critical, tr("Inferior shutdown failed"), msg);
    }
    shutdown(); // re-iterate...
}

void GdbEngine::handleGdbExit(const GdbResponse &response)
{
    if (response.resultClass == GdbResultExit) {
        debugMessage(_("GDB CLAIMS EXIT; WAITING"));
        // don't set state here, this will be handled in handleGdbFinished()
    } else {
        QString msg = m_gdbAdapter->msgGdbStopFailed(_(response.data.findChild("msg").data()));
        debugMessage(_("GDB WON'T EXIT (%1); KILLING IT").arg(msg));
        m_gdbProc.kill();
    }
}

void GdbEngine::detachDebugger()
{
    QTC_ASSERT(state() == InferiorStopped, /**/);
    QTC_ASSERT(startMode() != AttachCore, /**/);
    postCommand(_("detach")); 
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

void GdbEngine::continueInferiorInternal()
{
    QTC_ASSERT(state() == InferiorStopped || state() == InferiorStarting,
               qDebug() << state());
    setState(InferiorRunningRequested);
    postCommand(_("-exec-continue"), RunRequest, CB(handleExecContinue));
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

void GdbEngine::stepExec()
{
    QTC_ASSERT(state() == InferiorStopped, qDebug() << state());
    setTokenBarrier();
    setState(InferiorRunningRequested);
    showStatusMessage(tr("Step requested..."), 5000);
    if (manager()->isReverseDebugging())
        postCommand(_("-reverse-step"), RunRequest, CB(handleExecContinue));
    else
        postCommand(_("-exec-step"), RunRequest, CB(handleExecContinue));
}

void GdbEngine::stepIExec()
{
    QTC_ASSERT(state() == InferiorStopped, qDebug() << state());
    setTokenBarrier();
    setState(InferiorRunningRequested);
    showStatusMessage(tr("Step by instruction requested..."), 5000);
    if (manager()->isReverseDebugging())
        postCommand(_("-reverse-stepi"), RunRequest, CB(handleExecContinue));
    else
        postCommand(_("-exec-step-instruction"), RunRequest, CB(handleExecContinue));
}

void GdbEngine::stepOutExec()
{
    QTC_ASSERT(state() == InferiorStopped, qDebug() << state());
    setTokenBarrier();
    setState(InferiorRunningRequested);
    showStatusMessage(tr("Finish function requested..."), 5000);
    postCommand(_("-exec-finish"), RunRequest, CB(handleExecContinue));
}

void GdbEngine::nextExec()
{
    QTC_ASSERT(state() == InferiorStopped, qDebug() << state());
    setTokenBarrier();
    setState(InferiorRunningRequested);
    showStatusMessage(tr("Step next requested..."), 5000);
    if (manager()->isReverseDebugging())
        postCommand(_("-reverse-next"), RunRequest, CB(handleExecContinue));
    else {
#if 1
        postCommand(_("-exec-next"), RunRequest, CB(handleExecContinue));
#else
        postCommand(_("tbreak \"%2\":%1").arg(lastLine + 1).arg(breakLocation(lastFile)));
        postCommand(_("-exec-continue"), RunRequest, CB(handleExecContinue));
#endif
    }
}

void GdbEngine::nextIExec()
{
    QTC_ASSERT(state() == InferiorStopped, qDebug() << state());
    setTokenBarrier();
    setState(InferiorRunningRequested);
    showStatusMessage(tr("Step next instruction requested..."), 5000);
    if (manager()->isReverseDebugging())
        postCommand(_("-reverse-nexti"), RunRequest, CB(handleExecContinue));
    else
        postCommand(_("-exec-next-instruction"), RunRequest, CB(handleExecContinue));
}

void GdbEngine::runToLineExec(const QString &fileName, int lineNumber)
{
    QTC_ASSERT(state() == InferiorStopped, qDebug() << state());
    setTokenBarrier();
    setState(InferiorRunningRequested);
    showStatusMessage(tr("Run to line %1 requested...").arg(lineNumber), 5000);
    postCommand(_("-exec-until \"%2\":%1").arg(lineNumber).arg(breakLocation(fileName)),
                RunRequest, CB(handleExecContinue));
}

void GdbEngine::runToFunctionExec(const QString &functionName)
{
    QTC_ASSERT(state() == InferiorStopped, qDebug() << state());
    setTokenBarrier();
    postCommand(_("-break-insert -t ") + functionName);
    continueInferiorInternal();
    //setState(InferiorRunningRequested);
    //postCommand(_("-exec-continue"), handleExecRunToFunction);
    showStatusMessage(tr("Run to function %1 requested...").arg(functionName), 5000);
}

void GdbEngine::jumpToLineExec(const QString &fileName, int lineNumber)
{
    QTC_ASSERT(state() == InferiorStopped, qDebug() << state());
    StackFrame frame;
    frame.file = fileName;
    frame.line = lineNumber;
#if 1
    QString loc = breakLocation(fileName);
    postCommand(_("tbreak \"%2\":%1").arg(lineNumber).arg(loc));
    setState(InferiorRunningRequested);
    postCommand(_("jump \"%2\":%1").arg(lineNumber).arg(loc), RunRequest);
    // will produce something like
    //  &"jump \"/home/apoenitz/dev/work/test1/test1.cpp\":242"
    //  ~"Continuing at 0x4058f3."
    //  ~"run1 (argc=1, argv=0x7fffbf1f5538) at test1.cpp:242"
    //  ~"242\t x *= 2;"
    //  23^done"
    gotoLocation(frame, true);
    //setBreakpoint();
    //postCommand(_("jump ") + fileName + ':' + QString::number(lineNumber));
#else
    gotoLocation(frame,  true);
    setBreakpoint(fileName, lineNumber);
    setState(InferiorRunningRequested);
    postCommand(_("jump ") + fileName + ':' + QString::number(lineNumber), RunRequest);
#endif
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
    gdbInputAvailable(LogMisc, _("--- token barrier ---"));
    m_oldestAcceptableToken = currentToken();
}

void GdbEngine::setDebugDebuggingHelpers(const QVariant &on)
{
    if (on.toBool()) {
        debugMessage(_("SWITCHING ON DUMPER DEBUGGING"));
        postCommand(_("set unwindonsignal off"));
        m_manager->breakByFunction(_("qDumpObjectData440"));
        //updateLocals();
    } else {
        debugMessage(_("SWITCHING OFF DUMPER DEBUGGING"));
        postCommand(_("set unwindonsignal on"));
    }
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
            data->bpNumber = _(child.data());
        } else if (child.hasName("func")) {
            data->bpFuncName = _(child.data());
        } else if (child.hasName("addr")) {
            // <MULTIPLE> happens in constructors. In this case there are
            // _two_ fields named "addr" in the response. On Linux that is...
            if (child.data() == "<MULTIPLE>")
                data->bpMultiple = true;
            else
                data->bpAddress = _(child.data());
        } else if (child.hasName("file")) {
            file = child.data();
        } else if (child.hasName("fullname")) {
            fullName = child.data();
        } else if (child.hasName("line")) {
            data->bpLineNumber = _(child.data());
            if (child.data().toInt())
                data->markerLineNumber = child.data().toInt();
        } else if (child.hasName("cond")) {
            data->bpCondition = _(child.data());
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
        // Use fullName() once we have a mapping which is more complete than gdb's own ...
        // No point in assigning markerFileName for now.
    }
    data->bpFileName = name;
}

QString GdbEngine::breakLocation(const QString &file) const
{
    QTC_ASSERT(!m_sourcesListOutdated, /* */)
    QTC_ASSERT(!m_sourcesListUpdating, /* */)
    QString where = m_fullToShortName.value(file);
    if (where.isEmpty())
        return QFileInfo(file).fileName();
    return where;
}

void GdbEngine::sendInsertBreakpoint(int index)
{
    const BreakpointData *data = manager()->breakHandler()->at(index);
    QString where;
    if (data->funcName.isEmpty()) {
        where = data->useFullPath ? data->fileName : breakLocation(data->fileName);
        // The argument is simply a C-quoted version of the argument to the
        // non-MI "break" command, including the "original" quoting it wants.
        where = _("\"\\\"%2\\\":%1\"").arg(data->lineNumber).arg(GdbMi::escapeCString(where));
    } else {
        where = data->funcName;
    }

    // set up fallback in case of pending breakpoints which aren't handled
    // by the MI interface
    QString cmd;
    if (m_isMacGdb)
        cmd = _("-break-insert -l -1 -f ");
    else if (m_gdbVersion >= 60800) // Probably some earlier version would work as well ...
        cmd = _("-break-insert -f ");
    else
        cmd = _("-break-insert ");
    //if (!data->condition.isEmpty())
    //    cmd += _("-c ") + data->condition + _c(' ');
    cmd += where;
    postCommand(cmd, NeedsStop, CB(handleBreakInsert), index);
}

void GdbEngine::handleBreakList(const GdbResponse &response)
{
    m_sourcesListUpdating = false;

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

    attemptBreakpointSynchronization();
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

void GdbEngine::handleBreakInsert(const GdbResponse &response)
{
    int index = response.cookie.toInt();
    BreakHandler *handler = manager()->breakHandler();
    if (response.resultClass == GdbResultDone) {
//#if defined(Q_OS_MAC)
        // Interesting only on Mac?
        BreakpointData *data = handler->at(index);
        GdbMi bkpt = response.data.findChild("bkpt");
        breakpointDataFromOutput(data, bkpt);
//#endif
        attemptBreakpointSynchronization();
    } else {
        if (m_gdbVersion < 60800 && !m_isMacGdb) {
            // This gdb version doesn't "do" pending breakpoints.
        } else {
            QTC_ASSERT(false, /**/);
        }
    }
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
        data->bpAddress = re.cap(1);
        data->bpFuncName = re.cap(2).trimmed();
        data->bpLineNumber = re.cap(4);
        QString full = fullName(re.cap(3));
        if (full.isEmpty()) {
            qDebug() << "NO FULL NAME KNOWN FOR" << re.cap(3);
            full = cleanupFullName(re.cap(3));
            if (full.isEmpty()) {
                qDebug() << "FILE IS NOT RESOLVABLE" << re.cap(3);
                full = re.cap(3); // FIXME: wrong, but prevents recursion
            }
        }
        data->markerLineNumber = data->bpLineNumber.toInt();
        data->markerFileName = full;
        data->bpFileName = full;
    } else {
        qDebug() << "COULD NOT MATCH " << re.pattern() << " AND " << output;
        data->bpNumber = _("<unavailable>");
    }
}

void GdbEngine::handleBreakInfo(const GdbResponse &response)
{
    int bpNumber = response.cookie.toInt();
    BreakHandler *handler = manager()->breakHandler();
    if (response.resultClass == GdbResultDone) {
        // Old-style output for multiple breakpoints, presumably in a
        // constructor
        int found = handler->findBreakpoint(bpNumber);
        if (found != -1) {
            QString str = QString::fromLocal8Bit(response.data.findChild("consolestreamoutput").data());
            extractDataFromInfoBreak(str, handler->at(found));
            attemptBreakpointSynchronization(); // trigger "ready"
        }
    }
}

void GdbEngine::handleBreakInsert1(const GdbResponse &response)
{
    int index = response.cookie.toInt();
    BreakHandler *handler = manager()->breakHandler();
    if (response.resultClass == GdbResultDone) {
        // Pending breakpoints in dylibs on Mac only?
        BreakpointData *data = handler->at(index);
        GdbMi bkpt = response.data.findChild("bkpt");
        breakpointDataFromOutput(data, bkpt);
    } else {
        qDebug() << "INSERTING BREAKPOINT WITH BASE NAME FAILED. GIVING UP";
        BreakpointData *data = handler->at(index);
        data->bpNumber = _("<unavailable>");
    }
    attemptBreakpointSynchronization(); // trigger "ready"
}

void GdbEngine::attemptBreakpointSynchronization()
{
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
    if (m_sourcesListOutdated) {
        reloadSourceFilesInternal();
        return;
    } else if (m_sourcesListUpdating) {
        return;
    }

    BreakHandler *handler = manager()->breakHandler();

    foreach (BreakpointData *data, handler->takeDisabledBreakpoints()) {
        QString bpNumber = data->bpNumber;
        if (!bpNumber.trimmed().isEmpty()) {
            postCommand(_("-break-disable ") + bpNumber, NeedsStop);
            data->bpEnabled = false;
        }
    }

    foreach (BreakpointData *data, handler->takeEnabledBreakpoints()) {
        QString bpNumber = data->bpNumber;
        if (!bpNumber.trimmed().isEmpty()) {
            postCommand(_("-break-enable ") + bpNumber, NeedsStop);
            data->bpEnabled = true;
        }
    }

    foreach (BreakpointData *data, handler->takeRemovedBreakpoints()) {
        QString bpNumber = data->bpNumber;
        debugMessage(_("DELETING BP %1 IN %2").arg(bpNumber)
            .arg(data->markerFileName));
        if (!bpNumber.trimmed().isEmpty())
            postCommand(_("-break-delete ") + bpNumber, NeedsStop);
        delete data;
    }

    for (int index = 0; index != handler->size(); ++index) {
        BreakpointData *data = handler->at(index);
        if (data->bpNumber.isEmpty()) { // unset breakpoint?
            data->bpNumber = _(" "); // Sent, but no feedback yet
            sendInsertBreakpoint(index);
        } else if (data->bpNumber.toInt()) {
            if (data->bpMultiple && data->bpFileName.isEmpty()) {
                postCommand(_("info break %1").arg(data->bpNumber),
                    CB(handleBreakInfo), data->bpNumber.toInt());
                continue;
            }
            // update conditions if needed
            if (data->condition != data->bpCondition && !data->conditionsMatch())
                postCommand(_("condition %1 %2").arg(data->bpNumber).arg(data->condition),
                            CB(handleBreakCondition), index);
            // update ignorecount if needed
            if (data->ignoreCount != data->bpIgnoreCount)
                postCommand(_("ignore %1 %2").arg(data->bpNumber).arg(data->ignoreCount),
                            CB(handleBreakIgnore), index);
            if (!data->enabled && data->bpEnabled) {
                postCommand(_("-break-disable ") + data->bpNumber, NeedsStop);
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
    postCommand(_("sharedlibrary ") + dotEscape(moduleName));
    reloadModulesInternal();
}

void GdbEngine::loadAllSymbols()
{
    postCommand(_("sharedlibrary .*"));
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
    postCommand(_("info shared"), NeedsStop, CB(handleModulesList));
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
                module.endAddress = tr("<unknown>");
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

void GdbEngine::reloadSourceFiles()
{
    if ((state() == InferiorRunning || state() == InferiorStopped)
        && !m_sourcesListUpdating)
        reloadSourceFilesInternal();
}

void GdbEngine::reloadSourceFilesInternal()
{
    m_sourcesListUpdating = true;
    m_sourcesListOutdated = false;
    postCommand(_("-file-list-exec-source-files"), NeedsStop, CB(handleQuerySources));
    postCommand(_("-break-list"), CB(handleBreakList));
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
    postCommand(_("-thread-select %1").arg(id), CB(handleStackSelectThread));
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
    QString cmd = _("-stack-list-frames");
    postCommand(cmd, WatchUpdate, CB(handleStackListFrames),
        QVariant::fromValue<StackCookie>(StackCookie(true, true)));
}

void GdbEngine::reloadStack(bool forceGotoLocation)
{
    QString cmd = _("-stack-list-frames");
    int stackDepth = theDebuggerAction(MaximalStackDepth)->value().toInt();
    if (stackDepth && !m_gdbAdapter->isTrkAdapter())
        cmd += _(" 0 ") + QString::number(stackDepth);
    // FIXME: gdb 6.4 symbianelf likes to be asked twice. The first time it
    // returns with "^error,msg="Previous frame identical to this frame
    // (corrupt stack?)". Might be related to the fact that we can't
    // access the memory belonging to the lower frames. But as we know
    // this sometimes happens, ask the second time immediately instead
    // of waiting for the first request to fail.
    if (m_gdbAdapter->isTrkAdapter())
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
            postCommand(_("-exec-finish"), RunRequest, CB(handleExecContinue));
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
        postCommand(_("-stack-select-frame ") + QString::number(frameIndex));

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
// Register specific stuff
//
//////////////////////////////////////////////////////////////////////

void GdbEngine::reloadRegisters()
{
    if (!m_registerNamesListed) {
        postCommand(_("-data-list-register-names"), CB(handleRegisterListNames));
        m_registerNamesListed = true;
    }

    if (m_gdbAdapter->isTrkAdapter()) {
        // FIXME: remove that special case. This is only to prevent
        // gdb from asking for the values of the fixed point registers
        postCommand(_("-data-list-register-values x 0 1 2 3 4 5 6 7 8 9 "
                      "10 11 12 13 14 15 25"),
                    Discardable, CB(handleRegisterListValues));
    } else {
        postCommand(_("-data-list-register-values x"),
                    Discardable, CB(handleRegisterListValues));
    }
}

void GdbEngine::setRegisterValue(int nr, const QString &value)
{
    Register reg = manager()->registerHandler()->registers().at(nr);
    //qDebug() << "NOT IMPLEMENTED: CHANGE REGISTER " << nr << reg.name << ":"
    //    << value;
    postCommand(_("-var-delete \"R@\""));
    postCommand(_("-var-create \"R@\" * $%1").arg(reg.name));
    postCommand(_("-var-assign \"R@\" %1").arg(value));
    postCommand(_("-var-delete \"R@\""));
    //postCommand(_("-data-list-register-values d"),
    //            Discardable, CB(handleRegisterListValues));
    reloadRegisters();
}

void GdbEngine::handleRegisterListNames(const GdbResponse &response)
{
    if (response.resultClass != GdbResultDone) {
        m_registerNamesListed = false;
        return;
    }

    QList<Register> registers;
    foreach (const GdbMi &item, response.data.findChild("register-names").children())
        registers.append(Register(_(item.data())));

    manager()->registerHandler()->setRegisters(registers);
}

void GdbEngine::handleRegisterListValues(const GdbResponse &response)
{
    if (response.resultClass != GdbResultDone)
        return;

    QList<Register> registers = manager()->registerHandler()->registers();

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

static QString m_toolTipExpression;
static QPoint m_toolTipPos;

static QString tooltipINameForExpression(const QString &exp)
{
    // FIXME: 'exp' can contain illegal characters
    //return QLatin1String("tooltip.") + exp;
    Q_UNUSED(exp)
    return QLatin1String("tooltip.x");
}

bool GdbEngine::showToolTip()
{
    WatchHandler *handler = manager()->watchHandler();
    WatchModel *model = handler->model(TooltipsWatch);
    QString iname = tooltipINameForExpression(m_toolTipExpression);
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

    WatchData toolTip;
    toolTip.exp = exp;
    toolTip.name = exp;
    toolTip.iname = tooltipINameForExpression(exp);
    manager()->watchHandler()->removeData(toolTip.iname);
    manager()->watchHandler()->insertData(toolTip);
}


//////////////////////////////////////////////////////////////////////
//
// Watch specific stuff
//
//////////////////////////////////////////////////////////////////////

static void setWatchDataValue(WatchData &data, const GdbMi &mi,
    int encoding = 0)
{
    if (mi.isValid())
        data.setValue(decodeData(mi.data(), encoding));
    else
        data.setValueNeeded();
}

static void setWatchDataEditValue(WatchData &data, const GdbMi &mi)
{
    if (mi.isValid())
        data.editvalue = mi.data();
}

static void setWatchDataValueToolTip(WatchData &data, const GdbMi &mi,
        int encoding = 0)
{
    if (mi.isValid())
        data.setValueToolTip(decodeData(mi.data(), encoding));
}

static void setWatchDataChildCount(WatchData &data, const GdbMi &mi)
{
    if (mi.isValid())
        data.setHasChildren(mi.data().toInt() > 0);
}

static void setWatchDataValueEnabled(WatchData &data, const GdbMi &mi)
{
    if (mi.data() == "true")
        data.valueEnabled = true;
    else if (mi.data() == "false")
        data.valueEnabled = false;
}

static void setWatchDataValueEditable(WatchData &data, const GdbMi &mi)
{
    if (mi.data() == "true")
        data.valueEditable = true;
    else if (mi.data() == "false")
        data.valueEditable = false;
}

static void setWatchDataExpression(WatchData &data, const GdbMi &mi)
{
    if (mi.isValid())
        data.exp = _('(' + mi.data() + ')');
}

static void setWatchDataAddress(WatchData &data, const GdbMi &mi)
{
    if (mi.isValid()) {
        data.addr = _(mi.data());
        if (data.exp.isEmpty() && !data.addr.startsWith(_("$")))
            data.exp = _("(*(") + gdbQuoteTypes(data.type) + _("*)") + data.addr + _c(')');
    }
}

static void setWatchDataSAddress(WatchData &data, const GdbMi &mi)
{
    if (mi.isValid())
        data.saddr = _(mi.data());
}

void GdbEngine::setUseDebuggingHelpers(const QVariant &on)
{
    //qDebug() << "SWITCHING ON/OFF DUMPER DEBUGGING:" << on;
    Q_UNUSED(on)
    setTokenBarrier();
    updateLocals();
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

static inline QString msgRetrievingWatchData(int pending)
{
    return GdbEngine::tr("Retrieving data for watch view (%n requests pending)...", 0, pending);
}

void GdbEngine::runDirectDebuggingHelper(const WatchData &data, bool dumpChildren)
{
    Q_UNUSED(dumpChildren)
    QString type = data.type;
    QString cmd;

    if (type == __("QString") || type.endsWith(__("::QString")))
        cmd = _("qdumpqstring (&") + data.exp + _c(')');
    else if (type == __("QStringList") || type.endsWith(__("::QStringList")))
        cmd = _("qdumpqstringlist (&") + data.exp + _c(')');

    QVariant var;
    var.setValue(data);
    postCommand(cmd, WatchUpdate, CB(handleDebuggingHelperValue3), var);

    showStatusMessage(msgRetrievingWatchData(m_pendingRequests + 1), 10000);
}

void GdbEngine::runDebuggingHelper(const WatchData &data0, bool dumpChildren)
{
    if (m_debuggingHelperState != DebuggingHelperAvailable) {
        runDirectDebuggingHelper(data0, dumpChildren);
        return;
    }
    WatchData data = data0;

    // Avoid endless loops created by faulty dumpers.
    QString processedName = QString(_("%1-%2").arg(dumpChildren).arg(data.iname));
    if (m_processedNames.contains(processedName)) {
        gdbInputAvailable(LogStatus,
            _("<Breaking endless loop for %1>").arg(data.iname));
        data.setAllUnneeded();
        data.setValue(_("<unavailable>"));
        data.setHasChildren(false);
        insertData(data);
        return; 
    }
    m_processedNames.insert(processedName);

    QByteArray params;
    QStringList extraArgs;
    const QtDumperHelper::TypeData td = m_dumperHelper.typeData(data0.type);
    m_dumperHelper.evaluationParameters(data, td, QtDumperHelper::GdbDebugger, &params, &extraArgs);

    //int protocol = (data.iname.startsWith("watch") && data.type == "QImage") ? 3 : 2;
    //int protocol = data.iname.startsWith("watch") ? 3 : 2;
    const int protocol = 2;
    //int protocol = isDisplayedIName(data.iname) ? 3 : 2;

    QString addr;
    if (data.addr.startsWith(__("0x")))
        addr = _("(void*)") + data.addr;
    else if (data.exp.isEmpty()) // happens e.g. for QAbstractItem
        addr = _("0");
    else
        addr = _("&(") + data.exp + _c(')');

    sendWatchParameters(params);

    QString cmd;
    QTextStream(&cmd) << "call " << "(void*)qDumpObjectData440(" <<
            protocol << ',' << "%1+1"                // placeholder for token
            <<',' <<  addr << ',' << (dumpChildren ? "1" : "0")
            << ',' << extraArgs.join(QString(_c(','))) <<  ')';

    postCommand(cmd, WatchUpdate | EmbedToken);

    showStatusMessage(msgRetrievingWatchData(m_pendingRequests + 1), 10000);

    // retrieve response
    postCommand(_("p (char*)&qDumpOutBuffer"), WatchUpdate,
        CB(handleDebuggingHelperValue2), qVariantFromValue(data));
}

void GdbEngine::createGdbVariable(const WatchData &data)
{
    if (data.iname == _("local.flist.0")) {
        int i = 1;
        Q_UNUSED(i);
    }
    postCommand(_("-var-delete \"%1\"").arg(data.iname), WatchUpdate);
    QString exp = data.exp;
    if (exp.isEmpty() && data.addr.startsWith(__("0x")))
        exp = _("*(") + gdbQuoteTypes(data.type) + _("*)") + data.addr;
    QVariant val = QVariant::fromValue<WatchData>(data);
    postCommand(_("-var-create \"%1\" * \"%2\"").arg(data.iname).arg(exp),
        WatchUpdate, CB(handleVarCreate), val);
}

void GdbEngine::updateSubItem(const WatchData &data0)
{
    WatchData data = data0;
    #if DEBUG_SUBITEM
    qDebug() << "UPDATE SUBITEM:" << data.toString();
    #endif
    QTC_ASSERT(data.isValid(), return);

    // in any case we need the type first
    if (data.isTypeNeeded()) {
        // This should only happen if we don't have a variable yet.
        // Let's play safe, though.
        if (!data.variable.isEmpty()) {
            // Update: It does so for out-of-scope watchers.
            #if 1
            qDebug() << "FIXME: GdbEngine::updateSubItem:"
                 << data.toString() << "should not happen";
            #else
            data.setType(WatchData::msgNotInScope());
            data.setValue(WatchData::msgNotInScope());
            data.setHasChildren(false);
            insertData(data);
            return;
            #endif
        }
        // The WatchVarCreate handler will receive type information
        // and re-insert a WatchData item with correct type, so
        // we will not re-enter this bit.
        // FIXME: Concurrency issues?
        createGdbVariable(data);
        return;
    }

    // we should have a type now. this is relied upon further below
    QTC_ASSERT(!data.type.isEmpty(), return);

    // a common case that can be easily solved
    if (data.isChildrenNeeded() && isPointerType(data.type)
        && !hasDebuggingHelperForType(data.type)) {
        // We sometimes know what kind of children pointers have
        #if DEBUG_SUBITEM
        qDebug() << "IT'S A POINTER";
        #endif
    
        if (theDebuggerBoolSetting(AutoDerefPointers)) {
            // Try automatic dereferentiation
            data.exp = _("(*(") + data.exp + _("))");
            data.type = data.type + _("."); // FIXME: fragile HACK to avoid recursion
            insertData(data);
        } else {
            data.setChildrenUnneeded();
            insertData(data);
            WatchData data1;
            data1.iname = data.iname + QLatin1String(".*");
            data1.name = QLatin1Char('*') + data.name;
            data1.exp = QLatin1String("(*(") + data.exp + QLatin1String("))");
            data1.type = stripPointerType(data.type);
            data1.setValueNeeded();
            data1.setChildrenUnneeded();
            insertData(data1);
        }
        return;
    }

    if (data.isValueNeeded() && hasDebuggingHelperForType(data.type)) {
        #if DEBUG_SUBITEM
        qDebug() << "UPDATE SUBITEM: CUSTOMVALUE";
        #endif
        runDebuggingHelper(data, manager()->watchHandler()->isExpandedIName(data.iname));
        return;
    }

/*
    if (data.isValueNeeded() && data.exp.isEmpty()) {
        #if DEBUG_SUBITEM
        qDebug() << "UPDATE SUBITEM: NO EXPRESSION?";
        #endif
        data.setError("<no expression given>");
        insertData(data);
        return;
    }
*/

    if (data.isValueNeeded() && data.variable.isEmpty()) {
        #if DEBUG_SUBITEM
        qDebug() << "UPDATE SUBITEM: VARIABLE NEEDED FOR VALUE";
        #endif
        createGdbVariable(data);
        // the WatchVarCreate handler will re-insert a WatchData
        // item, with valueNeeded() set.
        return;
    }

    if (data.isValueNeeded()) {
        QTC_ASSERT(!data.variable.isEmpty(), return); // tested above
        #if DEBUG_SUBITEM
        qDebug() << "UPDATE SUBITEM: VALUE";
        #endif
        QString cmd = _("-var-evaluate-expression \"") + data.iname + _c('"');
        postCommand(cmd, WatchUpdate, CB(handleEvaluateExpression),
            QVariant::fromValue(data));
        return;
    }

    if (data.isChildrenNeeded() && hasDebuggingHelperForType(data.type)) {
        #if DEBUG_SUBITEM
        qDebug() << "UPDATE SUBITEM: CUSTOMVALUE WITH CHILDREN";
        #endif
        runDebuggingHelper(data, true);
        return;
    }

    if (data.isChildrenNeeded() && data.variable.isEmpty()) {
        #if DEBUG_SUBITEM
        qDebug() << "UPDATE SUBITEM: VARIABLE NEEDED FOR CHILDREN";
        #endif
        createGdbVariable(data);
        // the WatchVarCreate handler will re-insert a WatchData
        // item, with childrenNeeded() set.
        return;
    }

    if (data.isChildrenNeeded()) {
        QTC_ASSERT(!data.variable.isEmpty(), return); // tested above
        QString cmd = _("-var-list-children --all-values \"") + data.variable + _c('"');
        postCommand(cmd, WatchUpdate, CB(handleVarListChildren), QVariant::fromValue(data));
        return;
    }

    if (data.isHasChildrenNeeded() && hasDebuggingHelperForType(data.type)) {
        #if DEBUG_SUBITEM
        qDebug() << "UPDATE SUBITEM: CUSTOMVALUE WITH CHILDREN";
        #endif
        runDebuggingHelper(data, manager()->watchHandler()->isExpandedIName(data.iname));
        return;
    }

//#if !X
    if (data.isHasChildrenNeeded() && data.variable.isEmpty()) {
        #if DEBUG_SUBITEM
        qDebug() << "UPDATE SUBITEM: VARIABLE NEEDED FOR CHILDCOUNT";
        #endif
        createGdbVariable(data);
        // the WatchVarCreate handler will re-insert a WatchData
        // item, with childrenNeeded() set.
        return;
    }
//#endif

    if (data.isHasChildrenNeeded()) {
        QTC_ASSERT(!data.variable.isEmpty(), return); // tested above
        QString cmd = _("-var-list-children --all-values \"") + data.variable + _c('"');
        postCommand(cmd, Discardable, CB(handleVarListChildren), QVariant::fromValue(data));
        return;
    }

    qDebug() << "FIXME: UPDATE SUBITEM:" << data.toString();
    QTC_ASSERT(false, return);
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
        if (data.iname.endsWith(_(".")))
            return;

        // Avoid endless loops created by faulty dumpers.
        QString processedName = QString(_("%1-%2").arg(1).arg(data.iname));
        //qDebug() << "PROCESSED NAMES: " << processedName << m_processedNames;
        if (m_processedNames.contains(processedName)) {
            WatchData data1 = data;
            gdbInputAvailable(LogStatus,
                _("<Breaking endless loop for %1>").arg(data1.iname));
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
        ++m_pendingRequests;
        PENDING_DEBUG("UPDATE WATCH BUMPS PENDING UP TO " << m_pendingRequests);
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

    updateSubItem(data);
    //PENDING_DEBUG("INTERNAL TRIGGERING UPDATE WATCH MODEL");
    --m_pendingRequests;
    PENDING_DEBUG("UPDATE WATCH DONE BUMPS PENDING DOWN TO " << m_pendingRequests);
    if (m_pendingRequests <= 0)
        rebuildModel();
}

void GdbEngine::rebuildModel()
{
    static int count = 0;
    ++count;
    if (!isSynchroneous())
        m_processedNames.clear();
    PENDING_DEBUG("REBUILDING MODEL" << count);
    gdbInputAvailable(LogStatus, _("<Rebuild Watchmodel %1>").arg(count));
    showStatusMessage(tr("Finished retrieving data."), 400);
    manager()->watchHandler()->endCycle();
    showToolTip();
}

static inline double getDumperVersion(const GdbMi &contents)
{
    const GdbMi dumperVersionG = contents.findChild("dumperversion");
    if (dumperVersionG.type() != GdbMi::Invalid) {
        bool ok;
        const double v = QString::fromAscii(dumperVersionG.data()).toDouble(&ok);
        if (ok)
            return v;
    }
    return 1.0;
}

void GdbEngine::handleQueryDebuggingHelper(const GdbResponse &response)
{
    const double dumperVersionRequired = 1.0;
    //qDebug() << "DATA DUMPER TRIAL:" << response.toString();

    GdbMi contents;
    QTC_ASSERT(parseConsoleStream(response, &contents), qDebug() << response.toString());
    const bool ok = m_dumperHelper.parseQuery(contents, QtDumperHelper::GdbDebugger)
        && m_dumperHelper.typeCount();
    if (ok) {
        // Get version and sizes from dumpers. Expression cache
        // currently causes errors.
        const double dumperVersion = getDumperVersion(contents);
        if (dumperVersion < dumperVersionRequired) {
            manager()->showQtDumperLibraryWarning(
                QtDumperHelper::msgDumperOutdated(dumperVersionRequired, dumperVersion));
            m_debuggingHelperState = DebuggingHelperUnavailable;
            return;
        }
        m_debuggingHelperState = DebuggingHelperAvailable;
        const QString successMsg = tr("Dumper version %1, %n custom dumpers found.",
            0, m_dumperHelper.typeCount()).arg(dumperVersion);
        showStatusMessage(successMsg);
    } else {
        if (!m_dumperInjectionLoad) // Retry if thread has not terminated yet.
            m_debuggingHelperState = DebuggingHelperUnavailable;
        showStatusMessage(tr("Debugging helpers not found."));
    }
    //qDebug() << m_dumperHelper.toString(true);
    //qDebug() << m_availableSimpleDebuggingHelpers << "DATA DUMPERS AVAILABLE";
}

static inline QString arrayFillCommand(const char *array, const QByteArray &params)
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
    return _(encoded);
}

void GdbEngine::sendWatchParameters(const QByteArray &params0)
{
    QByteArray params = params0;
    params.append('\0');
    const QString inBufferCmd = arrayFillCommand("qDumpInBuffer", params);

    params.replace('\0','!');
    gdbInputAvailable(LogMisc, QString::fromUtf8(params));

    params.clear();
    params.append('\0');
    const QString outBufferCmd = arrayFillCommand("qDumpOutBuffer", params);

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
        if (hasDebuggingHelperForType(data.type)) {
            // we do not trust gdb if we have a custom dumper
            if (response.data.findChild("children").isValid())
                data.setChildrenUnneeded();
            else if (manager()->watchHandler()->isExpandedIName(data.iname))
                data.setChildrenNeeded();
            insertData(data);
        } else {
            if (response.data.findChild("children").isValid())
                data.setChildrenUnneeded();
            else if (manager()->watchHandler()->isExpandedIName(data.iname))
                data.setChildrenNeeded();
            setWatchDataChildCount(data, response.data.findChild("numchild"));
            //if (data.isValueNeeded() && data.childCount > 0)
            //    data.setValue(QString());
            insertData(data);
        }
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

void GdbEngine::handleEvaluateExpression(const GdbResponse &response)
{
    WatchData data = response.cookie.value<WatchData>();
    QTC_ASSERT(data.isValid(), qDebug() << "HUH?");
    if (response.resultClass == GdbResultDone) {
        //if (col == 0)
        //    data.name = response.data.findChild("value").data();
        //else
            setWatchDataValue(data, response.data.findChild("value"));
    } else {
        data.setError(QString::fromLocal8Bit(response.data.findChild("msg").data()));
    }
    //qDebug() << "HANDLE EVALUATE EXPRESSION:" << data.toString();
    insertData(data);
    //updateWatchModel2();
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

void GdbEngine::handleDebuggingHelperValue2(const GdbResponse &response)
{
    WatchData data = response.cookie.value<WatchData>();
    QTC_ASSERT(data.isValid(), return);

    //qDebug() << "CUSTOM VALUE RESULT:" << response.toString();
    //qDebug() << "FOR DATA:" << data.toString() << response.resultClass;
    if (response.resultClass != GdbResultDone) {
        qDebug() << "STRANGE CUSTOM DUMPER RESULT DATA:" << data.toString();
        return;
    }

    GdbMi contents;
    if (!parseConsoleStream(response, &contents)) {
        data.setError(WatchData::msgNotInScope());
        insertData(data);
        return;
    }

    setWatchDataType(data, response.data.findChild("type"));
    setWatchDataDisplayedType(data, response.data.findChild("displaytype"));
    QList<WatchData> list;
    handleChildren(data, contents, &list);
    //for (int i = 0; i != list.size(); ++i)
    //    qDebug() << "READ: " << list.at(i).toString();
    manager()->watchHandler()->insertBulkData(list);
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
    setWatchDataExpression(data, item.findChild("exp"));
    setWatchDataChildCount(data, item.findChild("numchild"));
    setWatchDataValue(data, item.findChild("value"),
        item.findChild("valueencoded").data().toInt());
    setWatchDataAddress(data, item.findChild("addr"));
    setWatchDataSAddress(data, item.findChild("saddr"));
    setWatchDataValueToolTip(data, item.findChild("valuetooltip"),
        item.findChild("valuetooltipencoded").data().toInt());
    setWatchDataValueEnabled(data, item.findChild("valueenabled"));
    setWatchDataValueEditable(data, item.findChild("valueeditable"));
    //qDebug() << "\nAPPEND TO LIST: " << data.toString() << "\n";
    list->append(data);

    // try not to repeat data too often
    WatchData childtemplate;
    setWatchDataType(childtemplate, item.findChild("childtype"));
    setWatchDataChildCount(childtemplate, item.findChild("childnumchild"));
    //qDebug() << "CHILD TEMPLATE:" << childtemplate.toString();

    int i = 0;
    foreach (GdbMi child, children.children()) {
        WatchData data1 = childtemplate;
        GdbMi name = child.findChild("name");
        if (name.isValid())
            data1.name = _(name.data());
        else
            data1.name = QString::number(i);
        data1.iname = data.iname + _c('.') + data1.name;
        if (!data1.name.isEmpty() && data1.name.at(0).isDigit())
            data1.name = _c('[') + data1.name + _c(']');
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

void GdbEngine::handleDebuggingHelperValue3(const GdbResponse &response)
{
    if (response.resultClass == GdbResultDone) {
        WatchData data = response.cookie.value<WatchData>();
        QByteArray out = response.data.findChild("consolestreamoutput").data();
        while (out.endsWith(' ') || out.endsWith('\n'))
            out.chop(1);
        QList<QByteArray> list = out.split(' ');
        //qDebug() << "RECEIVED" << response.toString() << "FOR" << data0.toString()
        //    <<  " STREAM:" << out;
        if (list.isEmpty()) {
            //: Value for variable
            data.setError(WatchData::msgNotInScope());
            data.setAllUnneeded();
            insertData(data);
        } else if (data.type == __("QString")
                || data.type.endsWith(__("::QString"))) {
            QList<QByteArray> list = out.split(' ');
            QString str;
            int l = out.isEmpty() ? 0 : list.size();
            for (int i = 0; i < l; ++i)
                 str.append(list.at(i).toInt());
            data.setValue(_c('"') + str + _c('"'));
            data.setHasChildren(false);
            data.setAllUnneeded();
            insertData(data);
        } else if (data.type == __("QStringList")
                || data.type.endsWith(__("::QStringList"))) {
            if (out.isEmpty()) {
                data.setValue(tr("<0 items>"));
                data.setHasChildren(false);
                data.setAllUnneeded();
                insertData(data);
            } else {
                int l = list.size();
                //: In string list
                data.setValue(tr("<%n items>", 0, l));
                data.setHasChildren(!list.empty());
                data.setAllUnneeded();
                insertData(data);
                for (int i = 0; i < l; ++i) {
                    WatchData data1;
                    data1.name = _("[%1]").arg(i);
                    data1.type = data.type.left(data.type.size() - 4);
                    data1.iname = data.iname + _(".%1").arg(i);
                    data1.addr = _(list.at(i));
                    data1.exp = _("((") + gdbQuoteTypes(data1.type) + _("*)") + data1.addr + _c(')');
                    data1.setHasChildren(false);
                    data1.setValueNeeded();
                    QString cmd = _("qdumpqstring (") + data1.exp + _c(')');
                    QVariant var;
                    var.setValue(data1);
                    postCommand(cmd, WatchUpdate, CB(handleDebuggingHelperValue3), var);
                }
            }
        } else {
            //: Value for variable
            data.setError(WatchData::msgNotInScope());
            data.setAllUnneeded();
            insertData(data);
        }
    } else {
        WatchData data = response.cookie.value<WatchData>();
        data.setError(WatchData::msgNotInScope());
        data.setAllUnneeded();
        insertData(data);
    }
}

void GdbEngine::updateLocals(const QVariant &cookie)
{
    m_pendingRequests = 0;
    if (isSynchroneous()) {
        m_processedNames.clear();
        manager()->watchHandler()->beginCycle();
        m_toolTipExpression.clear();
        QStringList expanded = m_manager->watchHandler()->expandedINames().toList();
        postCommand(_("-interpreter-exec console \"bb %1 %2\"")
                .arg(int(theDebuggerBoolSetting(UseDebuggingHelpers)))
                .arg(expanded.join(_(","))),
            CB(handleStackFrame));
    } else {
        m_processedNames.clear();

        PENDING_DEBUG("\nRESET PENDING");
        //m_toolTipCache.clear();
        m_toolTipExpression.clear();
        manager()->watchHandler()->beginCycle();

        // Asynchronous load of injected library, initialize in first stop
        if (m_dumperInjectionLoad && m_debuggingHelperState == DebuggingHelperLoadTried
                && m_dumperHelper.typeCount() == 0
                && inferiorPid() > 0)
            tryQueryDebuggingHelpers();

        QString level = QString::number(currentFrame());
        // '2' is 'list with type and value'
        QString cmd = _("-stack-list-arguments 2 ") + level + _c(' ') + level;
        postCommand(cmd, WatchUpdate, CB(handleStackListArguments));
        // '2' is 'list with type and value'
        postCommand(_("-stack-list-locals 2"), WatchUpdate,
            CB(handleStackListLocals), cookie); // stage 2/2
    }
}

void GdbEngine::handleStackFrame(const GdbResponse &response)
{
    if (response.resultClass == GdbResultDone) {
        QByteArray out = response.data.findChild("consolestreamoutput").data();
        while (out.endsWith(' ') || out.endsWith('\n'))
            out.chop(1);
        //qDebug() << "SECOND CHUNK: " << out;
        int pos = out.indexOf("locals=");
        if (pos != 0) {
            qDebug() << "DISCARDING JUNK AT BEGIN OF RESPONSE: "
                << out.left(pos);
            out = out.mid(pos);
        }
        GdbMi all("[" + out + "]");
        //GdbMi all(out);
        
        //qDebug() << "\n\n\nALL: " << all.toString() << "\n";
        GdbMi locals = all.findChild("locals");
        //qDebug() << "\n\n\nLOCALS: " << locals.toString() << "\n";
        WatchData *data = manager()->watchHandler()->findItem(_("local"));
        QTC_ASSERT(data, return);

        QList<WatchData> list;
        //foreach (const GdbMi &local, locals.children)
        //   handleChildren(*data, local, &list);
        handleChildren(*data, locals, &list);
        //for (int i = 0; i != list.size(); ++i)
        //    qDebug() << "READ: " << list.at(i).toString();
        manager()->watchHandler()->insertBulkData(list);

        // FIXME:
        //manager()->watchHandler()->updateWatchers();
        PENDING_DEBUG("AFTER handleStackFrame()");
        // FIXME: This should only be used when updateLocals() was
        // triggered by expanding an item in the view.
        if (m_pendingRequests <= 0) {
            PENDING_DEBUG("\n\n ....  AND TRIGGERS MODEL UPDATE\n");
            rebuildModel();
        }
    } else {
        QTC_ASSERT(false, /**/);
    }
}

void GdbEngine::handleStackListArguments(const GdbResponse &response)
{
    // stage 1/2

    // Linux:
    // 12^done,stack-args=
    //   [frame={level="0",args=[
    //     {name="argc",type="int",value="1"},
    //     {name="argv",type="char **",value="(char **) 0x7..."}]}]
    // Mac:
    // 78^done,stack-args=
    //    {frame={level="0",args={
    //      varobj=
    //        {exp="this",value="0x38a2fab0",name="var21",numchild="3",
    //             type="CurrentDocumentFind *  const",typecode="PTR",
    //             dynamic_type="",in_scope="true",block_start_addr="0x3938e946",
    //             block_end_addr="0x3938eb2d"},
    //      varobj=
    //         {exp="before",value="@0xbfffb9f8: {d = 0x3a7f2a70}",
    //              name="var22",numchild="1",type="const QString  ...} }}}
    //
    // In both cases, iterating over the children of stack-args/frame/args
    // is ok.
    m_currentFunctionArgs.clear();
    if (response.resultClass == GdbResultDone) {
        const GdbMi list = response.data.findChild("stack-args");
        const GdbMi frame = list.findChild("frame");
        const GdbMi args = frame.findChild("args");
        m_currentFunctionArgs = args.children();
    } else {
        qDebug() << "FIXME: GdbEngine::handleStackListArguments: should not happen"
            << response.toString();
    }
}

void GdbEngine::handleStackListLocals(const GdbResponse &response)
{
    // stage 2/2

    // There could be shadowed variables
    QList<GdbMi> locals = response.data.findChild("locals").children();
    locals += m_currentFunctionArgs;
    QMap<QByteArray, int> seen;
    // If desired, retrieve list of uninitialized variables looking at
    // the current frame. This is invoked first time after a stop from
    // handleStop1, which passes on the frame as cookie. The whole stack
    // is not known at this point.
    QStringList uninitializedVariables;
    if (theDebuggerAction(UseCodeModel)->isChecked()) {
        const StackFrame frame = qVariantCanConvert<Debugger::Internal::StackFrame>(response.cookie) ?
                                 qVariantValue<Debugger::Internal::StackFrame>(response.cookie) :
                                 m_manager->stackHandler()->currentFrame();
        if (frame.isUsable())
            getUninitializedVariables(m_manager->cppCodeModelSnapshot(),
                                      frame.function, frame.file, frame.line,
                                      &uninitializedVariables);
    }
    QList<WatchData> list;
    foreach (const GdbMi &item, locals) {
        const WatchData data = localVariable(item, uninitializedVariables, &seen);
        if (data.isValid())
            list.push_back(data);
    }
    manager()->watchHandler()->insertBulkData(list);
    manager()->watchHandler()->updateWatchers();
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
        data.iname = _("local.") + nam + QString::number(n + 1);
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
    data.iname = _("local.") + nam;
    data.name = nam;
    data.exp = nam;
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

void GdbEngine::handleVarListChildrenHelper(const GdbMi &item,
    const WatchData &parent)
{
    //qDebug() <<  "VAR_LIST_CHILDREN: APPENDEE" << data.toString();
    QByteArray exp = item.findChild("exp").data();
    QByteArray name = item.findChild("name").data();
    if (isAccessSpecifier(_(exp))) {
        // suppress 'private'/'protected'/'public' level
        WatchData data;
        data.variable = _(name);
        data.iname = parent.iname;
        //data.iname = data.variable;
        data.exp = parent.exp;
        data.setTypeUnneeded();
        data.setValueUnneeded();
        data.setHasChildrenUnneeded();
        data.setChildrenUnneeded();
        //qDebug() << "DATA" << data.toString();
        QString cmd = _("-var-list-children --all-values \"") + data.variable + _c('"');
        //iname += '.' + exp;
        postCommand(cmd, WatchUpdate,
            CB(handleVarListChildren), QVariant::fromValue(data));
    } else if (item.findChild("numchild").data() == "0") {
        // happens for structs without data, e.g. interfaces.
        WatchData data;
        data.name = _(exp);
        data.iname = parent.iname + _c('.') + data.name;
        data.variable = _(name);
        setWatchDataType(data, item.findChild("type"));
        setWatchDataValue(data, item.findChild("value"));
        setWatchDataAddress(data, item.findChild("addr"));
        setWatchDataSAddress(data, item.findChild("saddr"));
        data.setHasChildren(false);
        insertData(data);
    } else if (parent.iname.endsWith(_c('.'))) {
        // Happens with anonymous unions
        WatchData data;
        data.iname = _(name);
        QString cmd = _("-var-list-children --all-values \"") + data.variable + _c('"');
        postCommand(cmd, WatchUpdate,
            CB(handleVarListChildren), QVariant::fromValue(data));
    } else if (exp == "staticMetaObject") {
        //    && item.findChild("type").data() == "const QMetaObject")
        // FIXME: Namespaces?
        // { do nothing }    FIXME: make configurable?
        // special "clever" hack to avoid clutter in the GUI.
        // I am not sure this is a good idea...
    } else {
        WatchData data;
        data.iname = parent.iname + _c('.') + __(exp);
        data.variable = _(name);
        setWatchDataType(data, item.findChild("type"));
        setWatchDataValue(data, item.findChild("value"));
        setWatchDataAddress(data, item.findChild("addr"));
        setWatchDataSAddress(data, item.findChild("saddr"));
        setWatchDataChildCount(data, item.findChild("numchild"));
        if (!manager()->watchHandler()->isExpandedIName(data.iname))
            data.setChildrenUnneeded();

        data.name = _(exp);
        if (data.type == data.name) {
            if (isPointerType(parent.type)) {
                data.exp = _("*(") + parent.exp + _c(')');
                data.name = _("*") + parent.name;
            } else {
                // A type we derive from? gdb crashes when creating variables here
                data.exp = parent.exp;
            }
        } else if (exp.startsWith("*")) {
            // A pointer
            data.exp = _("*(") + parent.exp + _c(')');
        } else if (startsWithDigit(data.name)) {
            // An array. No variables needed?
            data.name = _c('[') + data.name + _c(']');
            data.exp = parent.exp + _('[' + exp + ']');
        } else if (0 && parent.name.endsWith(_c('.'))) {
            // Happens with anonymous unions
            data.exp = parent.exp + data.name;
            //data.name = "<anonymous union>";
        } else if (exp.isEmpty()) {
            // Happens with anonymous unions
            data.exp = parent.exp;
            data.name = tr("<n/a>");
            data.iname = parent.iname + _(".@");
            data.type = tr("<anonymous union>");
        } else {
            // A structure. Hope there's nothing else...
            data.exp = parent.exp + _c('.') + data.name;
        }

        if (hasDebuggingHelperForType(data.type)) {
            // we do not trust gdb if we have a custom dumper
            data.setValueNeeded();
            data.setHasChildrenNeeded();
        }

        //qDebug() <<  "VAR_LIST_CHILDREN: PARENT 3" << parent.toString();
        //qDebug() <<  "VAR_LIST_CHILDREN: APPENDEE" << data.toString();
        insertData(data);
    }
}

void GdbEngine::handleVarListChildren(const GdbResponse &response)
{
    //WatchResultCounter dummy(this, WatchVarListChildren);
    WatchData data = response.cookie.value<WatchData>();
    if (!data.isValid())
        return;
    if (response.resultClass == GdbResultDone) {
        //qDebug() <<  "VAR_LIST_CHILDREN: PARENT" << data.toString();
        GdbMi children = response.data.findChild("children");

        foreach (const GdbMi &child, children.children())
            handleVarListChildrenHelper(child, data);

        if (children.children().isEmpty()) {
            // happens e.g. if no debug information is present or
            // if the class really has no children
            WatchData data1;
            data1.iname = data.iname + _(".child");
            //: About variable's value
            data1.value = tr("<no information>");
            data1.hasChildren = false;
            data1.setAllUnneeded();
            insertData(data1);
            data.setAllUnneeded();
            insertData(data);
        } else if (!isAccessSpecifier(data.variable.split(_c('.')).last())) {
            data.setChildrenUnneeded();
            insertData(data);
        } else {
            // this skips the spurious "public", "private" etc levels
            // gdb produces
        }
    } else {
        data.setError(QString::fromLocal8Bit(response.data.findChild("msg").data()));
    }
}

#if 0
void GdbEngine::handleChangedItem(QStandardItem *item)
{
    // HACK: Just store the item for the slot
    //  handleChangedItem(QWidget *widget) below.
    QModelIndex index = item->index().sibling(item->index().row(), 0);
    //WatchData data = m_currentSet.takeData(iname);
    //m_editedData = inameFromItem(m_model.itemFromIndex(index)).exp;
    //qDebug() << "HANDLE CHANGED EXPRESSION:" << m_editedData;
}
#endif

void GdbEngine::assignValueInDebugger(const QString &expression, const QString &value)
{
    postCommand(_("-var-delete assign"));
    postCommand(_("-var-create assign * ") + expression);
    postCommand(_("-var-assign assign ") + value, Discardable, CB(handleVarAssign));
}

QString GdbEngine::qtDumperLibraryName() const
{
    return m_manager->qtDumperLibraryName();
}

bool GdbEngine::checkDebuggingHelpers()
{
    if (!manager()->qtDumperLibraryEnabled())
        return false;
    const QString lib = qtDumperLibraryName();
    //qDebug() << "DUMPERLIB:" << lib;
    const QFileInfo fi(lib);
    if (!fi.exists()) {
        const QStringList &locations = manager()->qtDumperLibraryLocations();
        const QString loc = locations.join(QLatin1String(", "));
        const QString msg = tr("The debugging helper library was not found at %1.").arg(loc);
        debugMessage(msg);
        manager()->showQtDumperLibraryWarning(msg);
        return false;
    }
    return true;
}

void GdbEngine::setDebuggingHelperState(DebuggingHelperState s)
{
    m_debuggingHelperState = s;
}

void GdbEngine::tryLoadDebuggingHelpers()
{
    if (isSynchroneous())
        return;
    switch (m_debuggingHelperState) {
    case DebuggingHelperUninitialized:
        break;
    case DebuggingHelperLoadTried:
        tryQueryDebuggingHelpers();
        return;
    case DebuggingHelperAvailable:
    case DebuggingHelperUnavailable:
        return;
    }

    if (m_gdbAdapter->dumperHandling() == AbstractGdbAdapter::DumperNotAvailable) {
        // Load at least gdb macro based dumpers.
        QFile file(_(":/gdb/gdbmacros.txt"));
        file.open(QIODevice::ReadOnly);
        QByteArray contents = file.readAll(); 
        m_debuggingHelperState = DebuggingHelperLoadTried;
        postCommand(_(contents));
        return;
    }
    if (m_dumperInjectionLoad && inferiorPid() <= 0) // Need PID to inject
        return;

    PENDING_DEBUG("TRY LOAD CUSTOM DUMPERS");
    m_debuggingHelperState = DebuggingHelperUnavailable;
    if (!checkDebuggingHelpers())
        return;

    m_debuggingHelperState = DebuggingHelperLoadTried;
    const QString lib = manager()->qtDumperLibraryName();
#if defined(Q_OS_WIN)
    if (m_dumperInjectionLoad) {
        /// Launch asynchronous remote thread to load.
        SharedLibraryInjector injector(inferiorPid());
        QString errorMessage;
        if (injector.remoteInject(lib, false, &errorMessage)) {
            debugMessage(_("Dumper injection loading triggered (%1)...").arg(lib));
        } else {
            debugMessage(_("Dumper loading (%1) failed: %2").arg(lib, errorMessage));
            manager()->showQtDumperLibraryWarning(errorMessage);
            m_debuggingHelperState = DebuggingHelperUnavailable;
            return;
        }
    } else {
        debugMessage(_("Loading dumpers via debugger call (%1)...").arg(lib));
        postCommand(_("sharedlibrary .*")); // for LoadLibraryA
        //postCommand(_("handle SIGSEGV pass stop print"));
        //postCommand(_("set unwindonsignal off"));
        postCommand(_("call LoadLibraryA(\"") + GdbMi::escapeCString(lib) + _("\")"),
                    CB(handleDebuggingHelperSetup));
        postCommand(_("sharedlibrary ") + dotEscape(lib));
    }
#elif defined(Q_OS_MAC)
    //postCommand(_("sharedlibrary libc")); // for malloc
    //postCommand(_("sharedlibrary libdl")); // for dlopen
    postCommand(_("call (void)dlopen(\"") + GdbMi::escapeCString(lib) + _("\", " STRINGIFY(RTLD_NOW) ")"),
        CB(handleDebuggingHelperSetup));
    //postCommand(_("sharedlibrary ") + dotEscape(lib));
#else
    //postCommand(_("p dlopen"));
    postCommand(_("sharedlibrary libc")); // for malloc
    postCommand(_("sharedlibrary libdl")); // for dlopen
    postCommand(_("call (void*)dlopen(\"") + GdbMi::escapeCString(lib) + _("\", " STRINGIFY(RTLD_NOW) ")"),
        CB(handleDebuggingHelperSetup));
    // some older systems like CentOS 4.6 prefer this:
    postCommand(_("call (void*)__dlopen(\"") + GdbMi::escapeCString(lib) + _("\", " STRINGIFY(RTLD_NOW) ")"),
        CB(handleDebuggingHelperSetup));
    postCommand(_("sharedlibrary ") + dotEscape(lib));
#endif
    if (!m_dumperInjectionLoad)
        tryQueryDebuggingHelpers();
}

void GdbEngine::tryQueryDebuggingHelpers()
{
    // retrieve list of dumpable classes
    postCommand(_("call (void*)qDumpObjectData440(1,%1+1,0,0,0,0,0,0)"), EmbedToken);
    postCommand(_("p (char*)&qDumpOutBuffer"), CB(handleQueryDebuggingHelper));
}

void GdbEngine::recheckDebuggingHelperAvailability()
{
    if (m_gdbAdapter->dumperHandling() != AbstractGdbAdapter::DumperNotAvailable) {
        // retreive list of dumpable classes
        postCommand(_("call (void*)qDumpObjectData440(1,%1+1,0,0,0,0,0,0)"), EmbedToken);
        postCommand(_("p (char*)&qDumpOutBuffer"), CB(handleQueryDebuggingHelper));
    }
}

void GdbEngine::watchPoint(const QPoint &pnt)
{
    //qDebug() << "WATCH " << pnt;
    postCommand(_("call (void*)watchPoint(%1,%2)").arg(pnt.x()).arg(pnt.y()),
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
    MemoryAgentCookie() : agent(0), address(0) {}
    MemoryAgentCookie(MemoryViewAgent *agent_, quint64 address_)
        : agent(agent_), address(address_)
    {}
    QPointer<MemoryViewAgent> agent;
    quint64 address;
};

void GdbEngine::fetchMemory(MemoryViewAgent *agent, quint64 addr, quint64 length)
{
    //qDebug() << "GDB MEMORY FETCH" << agent << addr << length;
    postCommand(_("-data-read-memory %1 x 1 1 %2").arg(addr).arg(length),
        NeedsStop, CB(handleFetchMemory),
        QVariant::fromValue(MemoryAgentCookie(agent, addr)));
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
    ac.agent->addLazyData(ac.address, ba);
}


struct DisassemblerAgentCookie
{
    DisassemblerAgentCookie() : agent(0) {}
    DisassemblerAgentCookie(DisassemblerViewAgent *agent_)
        : agent(agent_)
    {}
    QPointer<DisassemblerViewAgent> agent;
};

void GdbEngine::fetchDisassembler(DisassemblerViewAgent *agent,
    const StackFrame &frame)
{
    if (frame.file.isEmpty()) {
        fetchDisassemblerByAddress(agent, true);
    } else {
        // Disassemble full function:
        QString cmd = _("-data-disassemble -f %1 -l %2 -n -1 -- 1");
        postCommand(cmd.arg(frame.file).arg(frame.line),
            Discardable, CB(handleFetchDisassemblerByLine),
            QVariant::fromValue(DisassemblerAgentCookie(agent)));
    }
}

void GdbEngine::fetchDisassemblerByAddress(DisassemblerViewAgent *agent,
    bool useMixedMode)
{
    QTC_ASSERT(agent, return);
    bool ok = true;
    quint64 address = agent->address().toULongLong(&ok, 0);
    QTC_ASSERT(ok, qDebug() << "ADDRESS: " << agent->address() << address; return);
    QString start = QString::number(address - 20, 16);
    QString end = QString::number(address + 100, 16);
    // -data-disassemble [ -s start-addr -e end-addr ]
    //  | [ -f filename -l linenum [ -n lines ] ] -- mode
    if (useMixedMode) 
        postCommand(_("-data-disassemble -s 0x%1 -e 0x%2 -- 1").arg(start).arg(end),
            Discardable, CB(handleFetchDisassemblerByAddress1),
            QVariant::fromValue(DisassemblerAgentCookie(agent)));
    else
        postCommand(_("-data-disassemble -s 0x%1 -e 0x%2 -- 0").arg(start).arg(end),
            Discardable, CB(handleFetchDisassemblerByAddress0),
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
                QFile file(fullName(fileName));
                file.open(QIODevice::ReadOnly);
                fileContents = file.readAll().split('\n');
                fileLoaded = true;
            }
            int line = child.findChild("line").data().toInt();
            if (line >= 0 && line < fileContents.size())
                ba += "    " + fileContents.at(line) + '\n';
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
        else
            ac.agent->setContents(parseDisassembler(lines));
    } else {
        // 536^error,msg="mi_cmd_disassemble: Invalid line number"
        QByteArray msg = response.data.findChild("msg").data();
        if (msg == "mi_cmd_disassemble: Invalid line number")
            fetchDisassemblerByAddress(ac.agent, true);
        else
            showStatusMessage(tr("Disassembler failed: %1").arg(_(msg)), 5000);
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
        showStatusMessage(tr("Disassembler failed: %1").arg(_(msg)), 5000);
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
        showStatusMessage(tr("Disassembler failed: %1").arg(_(msg)), 5000);
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
    if (location.isEmpty())
        location = theDebuggerStringSetting(GdbLocation);
    QStringList gdbArgs;
    gdbArgs << _("-i");
    gdbArgs << _("mi");
    gdbArgs += args;
    m_gdbProc.start(location, gdbArgs);

    if (!m_gdbProc.waitForStarted()) {
        const QString msg = tr("Unable to start gdb '%1': %2").arg(location, m_gdbProc.errorString());
        handleAdapterStartFailed(msg, settingsIdHint);
        return false;
    }

    // Do this only after the process is running, so we get no needless error notifications
    connect(&m_gdbProc, SIGNAL(error(QProcess::ProcessError)),
        SLOT(handleGdbError(QProcess::ProcessError)));
    connect(&m_gdbProc, SIGNAL(finished(int, QProcess::ExitStatus)),
        SLOT(handleGdbFinished(int, QProcess::ExitStatus)));
    connect(&m_gdbProc, SIGNAL(readyReadStandardOutput()),
        SLOT(readGdbStandardOutput()));
    connect(&m_gdbProc, SIGNAL(readyReadStandardError()),
        SLOT(readGdbStandardError()));

    debugMessage(_("GDB STARTED, INITIALIZING IT"));

    postCommand(_("show version"), CB(handleShowVersion));
    postCommand(_("-interpreter-exec console \"help bb\""),
        CB(handleIsSynchroneous));
    //postCommand(_("-enable-timings");
    postCommand(_("set print static-members off")); // Seemingly doesn't work.
    //postCommand(_("set debug infrun 1"));
    //postCommand(_("define hook-stop\n-thread-list-ids\n-stack-list-frames\nend"));
    //postCommand(_("define hook-stop\nprint 4\nend"));
    //postCommand(_("define hookpost-stop\nprint 5\nend"));
    //postCommand(_("define hook-call\nprint 6\nend"));
    //postCommand(_("define hookpost-call\nprint 7\nend"));
    //postCommand(_("set print object on")); // works with CLI, but not MI
    //postCommand(_("set step-mode on"));  // we can't work with that yes
    //postCommand(_("set exec-done-display on"));
    //postCommand(_("set print pretty on"));
    //postCommand(_("set confirm off"));
    //postCommand(_("set pagination off"));

    // The following does not work with 6.3.50-20050815 (Apple version gdb-1344)
    // (Mac OS 10.6), but does so for gdb-966 (10.5):
    //postCommand(_("set print inferior-events 1"));

    postCommand(_("set breakpoint pending on"));
    postCommand(_("set print elements 10000"));

    //postCommand(_("set substitute-path /var/tmp/qt-x11-src-4.5.0 "
    //    "/home/sandbox/qtsdk-2009.01/qt"));

    // one of the following is needed to prevent crashes in gdb on code like:
    //  template <class T> T foo() { return T(0); }
    //  int main() { return foo<int>(); }
    //  (gdb) call 'int foo<int>'()
    //  /build/buildd/gdb-6.8/gdb/valops.c:2069: internal-error:
    postCommand(_("set overload-resolution off"));
    //postCommand(_("set demangle-style none"));
    // From the docs:
    //  Stop means reenter debugger if this signal happens (implies print).
    //  Print means print a message if this signal happens.
    //  Pass means let program see this signal;
    //  otherwise program doesn't know.
    //  Pass and Stop may be combined.
    // We need "print" as otherwise we would get no feedback whatsoever
    // Custom DebuggingHelper crashs which happen regularily for when accessing
    // uninitialized variables.
    postCommand(_("handle SIGSEGV nopass stop print"));

    // This is useful to kill the inferior whenever gdb dies.
    //postCommand(_("handle SIGTERM pass nostop print"));

    postCommand(_("set unwindonsignal on"));
    //postCommand(_("pwd"));
    postCommand(_("set width 0"));
    postCommand(_("set height 0"));

    if (m_isMacGdb) {
        postCommand(_("-gdb-set inferior-auto-start-cfm off"));
        postCommand(_("-gdb-set sharedLibrary load-rules "
            "dyld \".*libSystem.*\" all "
            "dyld \".*libauto.*\" all "
            "dyld \".*AppKit.*\" all "
            "dyld \".*PBGDBIntrospectionSupport.*\" all "
            "dyld \".*Foundation.*\" all "
            "dyld \".*CFDataFormatters.*\" all "
            "dyld \".*libobjc.*\" all "
            "dyld \".*CarbonDataFormatters.*\" all"));
    }

    QString scriptFileName = theDebuggerStringSetting(GdbScriptFile);
    if (!scriptFileName.isEmpty()) {
        if (QFileInfo(scriptFileName).isReadable()) {
            postCommand(_("source ") + scriptFileName);
        } else {
            showMessageBox(QMessageBox::Warning,
            tr("Cannot find debugger initialization script"),
            tr("The debugger settings point to a script file at '%1' "
               "which is not accessible. If a script file is not needed, "
               "consider clearing that entry to avoid this warning. "
              ).arg(scriptFileName));
        }
    }
    if (m_gdbAdapter->dumperHandling() == AbstractGdbAdapter::DumperLoadedByGdbPreload
        && checkDebuggingHelpers()) {        
        const QString cmd = QLatin1String("set environment LD_PRELOAD ") + manager()->qtDumperLibraryName();
        postCommand(cmd);
        m_debuggingHelperState = DebuggingHelperLoadTried;
    }
    return true;
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
        showMessageBox(QMessageBox::Critical, tr("Unexpected Gdb Exit"),
                       tr("The gdb process exited unexpectedly (%1).")
                       .arg((type == QProcess::CrashExit)
                            ? tr("crashed") : tr("code %1").arg(code)));
        m_gdbAdapter->shutdown();
    }
    initializeVariables();
    setState(DebuggerNotReady, true);
}

void GdbEngine::handleAdapterStartFailed(const QString &msg, const QString &settingsIdHint)
{
    setState(AdapterStartFailed);
    debugMessage(_("ADAPTER START FAILED"));
    const QString title = tr("Adapter start failed");
    if (settingsIdHint.isEmpty()) {
        Core::ICore::instance()->showWarningWithOptions(title, msg);
    } else {
        Core::ICore::instance()->showWarningWithOptions(title, msg, QString(),
                    _(Debugger::Constants::DEBUGGER_SETTINGS_CATEGORY),
                    settingsIdHint);
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
    // Initial attempt to set breakpoints
    showStatusMessage(tr("Setting breakpoints..."));
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
    if (state() == AdapterStartFailed)
        return; // Adapter crashed meanwhile, so this notification is meaningless.
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

void GdbEngine::showMessageBox(int icon, const QString &title, const QString &text)
{
    m_manager->showMessageBox(icon, title, text);
}

bool GdbEngine::isSynchroneous() const
{
    return m_isSynchroneous;
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
