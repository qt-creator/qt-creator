// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gdbengine.h"

#include <debugger/breakhandler.h>
#include <debugger/debuggeractions.h>
#include <debugger/debuggercore.h>
#include <debugger/debuggerinternalconstants.h>
#include <debugger/debuggermainwindow.h>
#include <debugger/debuggerplugin.h>
#include <debugger/debuggerprotocol.h>
#include <debugger/debuggersourcepathmappingwidget.h>
#include <debugger/debuggertooltipmanager.h>
#include <debugger/debuggertr.h>
#include <debugger/disassembleragent.h>
#include <debugger/disassemblerlines.h>
#include <debugger/logwindow.h>
#include <debugger/memoryagent.h>
#include <debugger/moduleshandler.h>
#include <debugger/procinterrupt.h>
#include <debugger/registerhandler.h>
#include <debugger/shared/hostutils.h>
#include <debugger/sourcefileshandler.h>
#include <debugger/sourceutils.h>
#include <debugger/stackhandler.h>
#include <debugger/terminal.h>
#include <debugger/threadshandler.h>

#include <coreplugin/icore.h>
#include <coreplugin/messagebox.h>

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/taskhub.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/process.h>
#include <utils/processinterface.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/temporaryfile.h>

#include <QDirIterator>
#include <QGuiApplication>
#include <QJsonArray>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QRegularExpression>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace Debugger::Internal {

enum { debugPending = 0 };

#define PENDING_DEBUG(s) do { if (debugPending) qDebug() << s; } while (0)

#define CB(callback) [this](const DebuggerResponse &r) { callback(r); }

#define CHECK_STATE(s) do { checkState(s, __FILE__, __LINE__); } while (0)

static int &currentToken()
{
    static int token = 0;
    return token;
}

static bool isMostlyHarmlessMessage(const QStringView msg)
{
    return msg == u"warning: GDB: Failed to set controlling terminal: "
                   "Inappropriate ioctl for device\\n"
        || msg == u"warning: GDB: Failed to set controlling terminal: "
                   "Invalid argument\\n";
}

static QMessageBox *showMessageBox(QMessageBox::Icon icon,
                                   const QString &title, const QString &text,
                                   QMessageBox::StandardButtons buttons)
{
    auto mb = new QMessageBox(icon, title, text, buttons, ICore::dialogParent());
    mb->setAttribute(Qt::WA_DeleteOnClose);
    mb->setTextInteractionFlags(Qt::TextSelectableByMouse);
    mb->show();
    return mb;
}

enum class TracepointCaptureType
{
    Address,
    Caller,
    Callstack,
    FilePos,
    Function,
    Pid,
    ProcessName,
    Tick,
    Tid,
    ThreadName,
    Expression
};

struct TracepointCaptureData
{
    TracepointCaptureType type;
    QVariant expression;
    int start;
    int end;
};

const char tracepointCapturePropertyName[] = "GDB.TracepointCapture";
const char notCompatibleMessage[] = "is not compatible with target architecture";

///////////////////////////////////////////////////////////////////////
//
// GdbEngine
//
///////////////////////////////////////////////////////////////////////

GdbEngine::GdbEngine()
{
    m_gdbProc.setProcessMode(ProcessMode::Writer);

    setObjectName("GdbEngine");
    setDebuggerName("GDB");

    m_gdbOutputCodec = QTextCodec::codecForLocale();
    m_inferiorOutputCodec = QTextCodec::codecForLocale();

    m_commandTimer.setSingleShot(true);
    connect(&m_commandTimer, &QTimer::timeout,
            this, &GdbEngine::commandTimeout);

    DebuggerSettings &s = settings();
    connect(&s.autoDerefPointers, &BaseAspect::changed,
            this, &GdbEngine::reloadLocals);
    connect(s.createFullBacktrace.action(), &QAction::triggered,
            this, &GdbEngine::createFullBacktrace);
    connect(&s.useDebuggingHelpers, &BaseAspect::changed,
            this, &GdbEngine::reloadLocals);
    connect(&s.useDynamicType, &BaseAspect::changed,
            this, &GdbEngine::reloadLocals);

    connect(&m_gdbProc, &Process::started,
            this, &GdbEngine::handleGdbStarted);
    connect(&m_gdbProc, &Process::done,
            this, &GdbEngine::handleGdbDone);
    connect(&m_gdbProc, &Process::readyReadStandardOutput,
            this, &GdbEngine::readGdbStandardOutput);
    connect(&m_gdbProc, &Process::readyReadStandardError,
            this, &GdbEngine::readGdbStandardError);

    // Output
    connect(&m_outputCollector, &OutputCollector::byteDelivery,
            this, &GdbEngine::readDebuggeeOutput);
}

GdbEngine::~GdbEngine()
{
    // Prevent sending error messages afterwards.
    disconnect();
}

QString GdbEngine::failedToStartMessage()
{
    return Tr::tr("The gdb process failed to start.");
}

// Parse "~:gdb: unknown target exception 0xc0000139 at 0x77bef04e\n"
// and return an exception message
static QString msgWinException(const QString &data, unsigned *exCodeIn = nullptr)
{
    if (exCodeIn)
        *exCodeIn = 0;
    const int exCodePos = data.indexOf("0x");
    const int blankPos = exCodePos != -1 ? data.indexOf(' ', exCodePos + 1) : -1;
    const int addressPos = blankPos != -1 ? data.indexOf("0x", blankPos + 1) : -1;
    if (addressPos < 0)
        return Tr::tr("An exception was triggered.");
    const unsigned exCode = data.mid(exCodePos, blankPos - exCodePos).toUInt(nullptr, 0);
    if (exCodeIn)
        *exCodeIn = exCode;
    const quint64 address = data.mid(addressPos).trimmed().toULongLong(nullptr, 0);
    QString rc;
    QTextStream str(&rc);
    str << Tr::tr("An exception was triggered:") << ' ';
    formatWindowsException(exCode, address, 0, 0, 0, str);
    str << '.';
    return rc;
}

static bool isNameChar(int c)
{
    // could be 'stopped' or 'shlibs-added'
    return (c >= 'a' && c <= 'z') || c == '-';
}

static bool contains(const QString &message, const QString &pattern, int size)
{
    const int s = message.size();
    if (s < size)
        return false;
    const int pos = message.indexOf(pattern);
    if (pos == -1)
        return false;
    const bool beginFits = pos == 0 || message.at(pos - 1) == '\n';
    const bool endFits = pos + size == s || message.at(pos + size) == '\n';
    return beginFits && endFits;
}

static bool isGdbConnectionError(const QString &message)
{
    // Handle messages gdb client produces when the target exits (gdbserver)
    //
    // we get this as response either to a specific command, e.g.
    //    31^error,msg="Remote connection closed"
    // or as informative output:
    //    &Remote connection closed

    const char msg1[] = "Remote connection closed";
    const char msg2[] = "Remote communication error.  Target disconnected.: No error.";
    const char msg3[] = "Quit";

    return contains(message, msg1, sizeof(msg1) - 1)
        || contains(message, msg2, sizeof(msg2) - 1)
        || contains(message, msg3, sizeof(msg3) - 1);
}

void GdbEngine::handleResponse(const QString &buff)
{
    showMessage(buff, LogOutput);

    if (buff.isEmpty() || buff == "(gdb) ")
        return;

    DebuggerOutputParser parser(buff);

    const int token = parser.readInt();

    // Next char decides kind of response.
    switch (parser.readChar().unicode()) {
        case '*':
        case '+':
        case '=': {
            const QStringView asyncClass = parser.readString(isNameChar);
            GdbMi result;
            while (!parser.isAtEnd()) {
                GdbMi data;
                if (!parser.isCurrent(',')) {
                    // happens on archer where we get
                    // 23^running <NL> *running,thread-id="all" <NL> (gdb)
                    result.m_type = GdbMi::Tuple;
                    break;
                }
                parser.advance(); // skip ','
                data.parseResultOrValue(parser);
                if (data.isValid()) {
                    //qDebug() << "parsed result:" << data.toString();
                    result.addChild(data);
                    result.m_type = GdbMi::Tuple;
                }
            }
            handleAsyncOutput(asyncClass, result);
            break;
        }

        case '~': {
            QString data = parser.readCString();
            if (data.startsWith("bridgemessage={")) {
                // It's already logged.
                break;
            }
            if (data.startsWith("interpreterresult={")) {
                GdbMi allData;
                allData.fromStringMultiple(data);
                DebuggerResponse response;
                response.resultClass = ResultDone;
                response.data = allData["interpreterresult"];
                response.token = allData["token"].toInt();
                handleResultRecord(&response);
                break;
            }
            if (data.startsWith("interpreterasync={")) {
                GdbMi allData;
                allData.fromStringMultiple(data);
                QString asyncClass = allData["asyncclass"].data();
                if (asyncClass == "breakpointmodified")
                    handleInterpreterBreakpointModified(allData["interpreterasync"]);
                break;
            }
            if (data.startsWith("tracepointhit={")) {
                GdbMi allData;
                allData.fromStringMultiple(data);
                handleTracepointHit(allData["tracepointhit"]);
                break;
            }
            if (data.startsWith("tracepointmodified=")) {
                GdbMi allData;
                allData.fromStringMultiple(data);
                handleTracepointModified(allData["tracepointmodified"]);
                break;
            }
            m_pendingConsoleStreamOutput += data;

            // Fragile, but it's all we have.
            if (data.contains("\nNo more reverse-execution history.\n"))
                handleBeginOfRecordingReached();

            // Show some messages to give the impression something happens.
            if (data.startsWith("Reading symbols from ")) {
                showStatusMessage(Tr::tr("Reading %1...").arg(data.mid(21)), 1000);
                progressPing();
            } else if (data.startsWith("[New ") || data.startsWith("[Thread ")) {
                if (data.endsWith('\n'))
                    data.chop(1);
                progressPing();
                showStatusMessage(data, 1000);
            } else if (data.startsWith("gdb: unknown target exception 0x")) {
                // [Windows, most likely some DLL/Entry point not found]:
                // "gdb: unknown target exception 0xc0000139 at 0x77bef04e"
                // This may be fatal and cause the target to exit later
                unsigned exCode;
                m_lastWinException = msgWinException(data, &exCode);
                showMessage(m_lastWinException, LogMisc);
                const Task::TaskType type = isFatalWinException(exCode) ? Task::Error : Task::Warning;
                TaskHub::addTask(Task(type, m_lastWinException, {}, -1,
                                      Constants::TASK_CATEGORY_DEBUGGER_RUNTIME));
            }
            break;
        }

        case '@': {
            QString data = parser.readCString();
            QString msg = data.left(data.size() - 1);
            showMessage(msg, AppOutput);
            break;
        }

        case '&': {
            QString data = parser.readCString();
            // On Windows, the contents seem to depend on the debugger
            // version and/or OS version used.
            if (data.startsWith("warning:")) {
                showMessage(data.mid(9), AppStuff); // Cut "warning: "
                if (data.contains(notCompatibleMessage))
                    m_ignoreNextTrap = true;
            } else if (data.startsWith("Error while mapping")) {
                m_detectTargetIncompat = true;
            } else if (m_detectTargetIncompat && data.contains(notCompatibleMessage)) {
                m_detectTargetIncompat = false;
                m_ignoreNextTrap = true;
            }

            m_pendingLogStreamOutput += data;

            if (isGdbConnectionError(data)) {
                notifyInferiorExited();
                break;
            }

            break;
        }

        case '^': {
            DebuggerResponse response;

            response.token = token;

            const QStringView resultClass = parser.readString(isNameChar);

            if (resultClass == u"done")
                response.resultClass = ResultDone;
            else if (resultClass == u"running")
                response.resultClass = ResultRunning;
            else if (resultClass == u"connected")
                response.resultClass = ResultConnected;
            else if (resultClass == u"error")
                response.resultClass = ResultError;
            else if (resultClass == u"exit")
                response.resultClass = ResultExit;
            else
                response.resultClass = ResultUnknown;

            if (!parser.isAtEnd()) {
                if (parser.isCurrent(',')) {
                    parser.advance();
                    response.data.parseTuple_helper(parser);
                    response.data.m_type = GdbMi::Tuple;
                    response.data.m_name = "data";
                } else {
                    // Archer has this.
                    response.data.m_type = GdbMi::Tuple;
                    response.data.m_name = "data";
                }
            }

            response.logStreamOutput = m_pendingLogStreamOutput;
            response.consoleStreamOutput =  m_pendingConsoleStreamOutput;
            m_pendingLogStreamOutput.clear();
            m_pendingConsoleStreamOutput.clear();

            if (response.data.data().isEmpty())
                response.data.fromString(response.consoleStreamOutput);

            handleResultRecord(&response);
            break;
        }
        default: {
            qDebug() << "UNKNOWN RESPONSE TYPE '" << parser.current() << "'. BUFFER: "
                     << parser.buffer();
            break;
        }
    }

    if (settings().logTimeStamps())
        showMessage(QString("Output handled"));
}

void GdbEngine::handleAsyncOutput(const QStringView asyncClass, const GdbMi &result)
{
    if (asyncClass == u"stopped") {
        if (m_inUpdateLocals) {
            showMessage("UNEXPECTED *stopped NOTIFICATION IGNORED", LogWarning);
        } else {
            handleStopResponse(result);
            m_pendingLogStreamOutput.clear();
            m_pendingConsoleStreamOutput.clear();
        }
    } else if (asyncClass == u"running") {
        if (m_inUpdateLocals) {
            showMessage("UNEXPECTED *running NOTIFICATION IGNORED", LogWarning);
        } else {
            GdbMi threads = result["thread-id"];
            threadsHandler()->notifyRunning(threads.data());
            if (runParameters().toolChainAbi.os() == Abi::WindowsOS) {
                // NOTE: Each created thread spits out a *running message. We completely ignore them
                // on Windows, and handle only numbered responses

                // FIXME: Breakpoints on Windows are exceptions which are thrown in newly
                // created threads so we have to filter out the running threads messages when
                // we request a stop.
            } else if (state() == InferiorRunOk || state() == EngineSetupRequested) {
                // We get multiple *running after thread creation and in Windows terminals.
                showMessage(QString("NOTE: INFERIOR STILL RUNNING IN STATE %1.").
                            arg(DebuggerEngine::stateName(state())));
            } else {
                notifyInferiorRunOk();
            }
        }
    } else if (asyncClass == u"library-loaded") {
        // Archer has 'id="/usr/lib/libdrm.so.2",
        // target-name="/usr/lib/libdrm.so.2",
        // host-name="/usr/lib/libdrm.so.2",
        // symbols-loaded="0"

        // id="/lib/i386-linux-gnu/libc.so.6"
        // target-name="/lib/i386-linux-gnu/libc.so.6"
        // host-name="/lib/i386-linux-gnu/libc.so.6"
        // symbols-loaded="0",thread-group="i1"
        QString id = result["id"].data();
        if (!id.isEmpty())
            showStatusMessage(Tr::tr("Library %1 loaded.").arg(id), 1000);
        progressPing();
        Module module;
        module.startAddress = 0;
        module.endAddress = 0;
        module.hostPath = result["host-name"].data();
        const QString target = result["target-name"].data();
        module.modulePath = runParameters().inferior.command.executable().withNewPath(target);
        module.moduleName = QFileInfo(module.hostPath).baseName();
        modulesHandler()->updateModule(module);
    } else if (asyncClass == u"library-unloaded") {
        // Archer has 'id="/usr/lib/libdrm.so.2",
        // target-name="/usr/lib/libdrm.so.2",
        // host-name="/usr/lib/libdrm.so.2"
        QString id = result["id"].data();
        const QString target = result["target-name"].data();
        modulesHandler()->removeModule(runParameters().inferior.command.executable().withNewPath(target));
        progressPing();
        showStatusMessage(Tr::tr("Library %1 unloaded.").arg(id), 1000);
    } else if (asyncClass == u"thread-group-added") {
        // 7.1-symbianelf has "{id="i1"}"
    } else if (asyncClass == u"thread-group-started") {
        // Archer had only "{id="28902"}" at some point of 6.8.x.
        // *-started seems to be standard in 7.1, but in early
        // 7.0.x, there was a *-created instead.
        progressPing();
        // 7.1.50 has thread-group-started,id="i1",pid="3529"
        QString id = result["id"].data();
        showStatusMessage(Tr::tr("Thread group %1 created.").arg(id), 1000);
        notifyInferiorPid(result["pid"].toProcessHandle());
        handleThreadGroupCreated(result);
    } else if (asyncClass == u"thread-created") {
        //"{id="1",group-id="28902"}"
        QString id = result["id"].data();
        showStatusMessage(Tr::tr("Thread %1 created.").arg(id), 1000);
        ThreadData thread;
        thread.id = id;
        thread.groupId = result["group-id"].data();
        threadsHandler()->updateThread(thread);
    } else if (asyncClass == u"thread-group-exited") {
        // Archer has "{id="28902"}"
        QString id = result["id"].data();
        showStatusMessage(Tr::tr("Thread group %1 exited.").arg(id), 1000);
        handleThreadGroupExited(result);
    } else if (asyncClass == u"thread-exited") {
        //"{id="1",group-id="28902"}"
        QString id = result["id"].data();
        QString groupid = result["group-id"].data();
        showStatusMessage(Tr::tr("Thread %1 in group %2 exited.")
                          .arg(id).arg(groupid), 1000);
        threadsHandler()->removeThread(id);
    } else if (asyncClass == u"thread-selected") {
        QString id = result["id"].data();
        showStatusMessage(Tr::tr("Thread %1 selected.").arg(id), 1000);
        //"{id="2"}"
    } else if (asyncClass == u"breakpoint-modified") {
        // New in FSF gdb since 2011-04-27.
        // "{bkpt={number="3",type="breakpoint",disp="keep",
        // enabled="y",addr="<MULTIPLE>",times="1",
        // original-location="\\",simple_gdbtest_app.cpp\\":135"},
        // {number="3.1",enabled="y",addr="0x0805ff68",
        // func="Vector<int>::Vector(int)",
        // file="simple_gdbtest_app.cpp",
        // fullname="/data/...line="135"},{number="3.2"...}}.."

        // Note the leading comma in original-location. Filter it out.
        // We don't need the field anyway.
        QString ba = result.toString();
        ba = '[' + ba.mid(6, ba.size() - 7) + ']';
        const int pos1 = ba.indexOf(",original-location");
        const int pos2 = ba.indexOf("\":", pos1 + 2);
        const int pos3 = ba.indexOf('"', pos2 + 2);
        ba.remove(pos1, pos3 - pos1 + 1);
        GdbMi res;
        res.fromString(ba);
        const FilePath &fileRoot = runParameters().projectSourceDirectory;
        BreakHandler *handler = breakHandler();
        Breakpoint bp;
        for (const GdbMi &bkpt : res) {
            const QString nr = bkpt["number"].data();
            if (nr.contains('.')) {
                // A sub-breakpoint.
                QTC_ASSERT(bp, continue);
                SubBreakpoint loc = bp->findOrCreateSubBreakpoint(nr);
                loc->params.updateFromGdbOutput(bkpt, fileRoot);
                loc->params.type = bp->type();
            } else {
                // A primary breakpoint.
                bp = handler->findBreakpointByResponseId(nr);
                if (bp)
                    bp->updateFromGdbOutput(bkpt, fileRoot);
            }
        }
        if (bp)
            bp->adjustMarker();
    } else if (asyncClass == u"breakpoint-created") {
        // "{bkpt={number="1",type="breakpoint",disp="del",enabled="y",
        //  addr="<PENDING>",pending="main",times="0",
        //  original-location="main"}}" -- or --
        // {bkpt={number="2",type="hw watchpoint",disp="keep",enabled="y",
        // what="*0xbfffed48",times="0",original-location="*0xbfffed48"}}
        BreakHandler *handler = breakHandler();
        for (const GdbMi &bkpt : result) {
            const QString nr = bkpt["number"].data();
            BreakpointParameters br;
            br.type = BreakpointByFileAndLine;
            br.updateFromGdbOutput(bkpt, runParameters().projectSourceDirectory);
            handler->handleAlienBreakpoint(nr, br);
        }
    } else if (asyncClass == u"breakpoint-deleted") {
        // "breakpoint-deleted" "{id="1"}"
        // New in FSF gdb since 2011-04-27.
        const QString nr = result["id"].data();
        // This also triggers when a temporary breakpoint is hit.
        // We do not really want that, as this loses all information.
        // FIXME: Use a special marker for this case?
        // if (!bp.isOneShot()) ... is not sufficient.
        // It keeps temporary "Jump" breakpoints alive.
        breakHandler()->removeAlienBreakpoint(nr);
    } else if (asyncClass == u"cmd-param-changed") {
        // New since 2012-08-09
        //  "{param="debug remote",value="1"}"
    } else if (asyncClass == u"memory-changed") {
        // New since 2013
        //   "{thread-group="i1",addr="0x0918a7a8",len="0x10"}"
    } else if (asyncClass == u"tsv-created") {
        // New since 2013-02-06
    } else if (asyncClass == u"tsv-modified") {
        // New since 2013-02-06
    } else {
        qDebug() << "IGNORED ASYNC OUTPUT"
                 << asyncClass << result.toString();
    }
}

void GdbEngine::readGdbStandardError()
{
    QString err = QString::fromUtf8(m_gdbProc.readAllRawStandardError());
    showMessage("UNEXPECTED GDB STDERR: " + err);
    if (err == "Undefined command: \"bb\".  Try \"help\".\n")
        return;
    if (err.startsWith("BFD: reopening"))
        return;
    qWarning() << "Unexpected GDB stderr:" << err;
}

void GdbEngine::readDebuggeeOutput(const QByteArray &ba)
{
    const QString msg = m_inferiorOutputCodec->toUnicode(ba.constData(), ba.size(),
                                                         &m_inferiorOutputCodecState);

    if (msg.startsWith("&\"") && isMostlyHarmlessMessage(QStringView{msg}.mid(2, msg.size() - 4)))
        showMessage("Mostly harmless terminal warning suppressed.", LogWarning);
    else
        showMessage(msg, AppStuff);
}

void GdbEngine::readGdbStandardOutput()
{
    m_commandTimer.start(); // Restart timer.

    int newstart = 0;
    int scan = m_inbuffer.size();

    QByteArray out = m_gdbProc.readAllRawStandardOutput();
    m_inbuffer.append(out);

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
        if (m_inbuffer.at(end - 1) == '\r') {
            --end;
            if (end == start)
                continue;
        }
        m_busy = true;

        QString msg = m_gdbOutputCodec->toUnicode(m_inbuffer.constData() + start, end - start,
                                                  &m_gdbOutputCodecState);

        handleResponse(msg);
        m_busy = false;
    }
    m_inbuffer.clear();
}

void GdbEngine::interruptInferior()
{
    // A core never runs, so this cannot be called.
    QTC_ASSERT(!isCoreEngine(), return);

    CHECK_STATE(InferiorStopRequested);

    if (usesExecInterrupt()) {
        runCommand({"-exec-interrupt"});
    } else {
        showStatusMessage(Tr::tr("Stop requested..."), 5000);
        showMessage("TRYING TO INTERRUPT INFERIOR");
        if (HostOsInfo::isWindowsHost() && !m_isQnxGdb) {
            IDevice::ConstPtr dev = device();
            QTC_ASSERT(dev, notifyInferiorStopFailed(); return);
            DeviceProcessSignalOperation::Ptr signalOperation = dev->signalOperation();
            QTC_ASSERT(signalOperation, notifyInferiorStopFailed(); return);
            connect(signalOperation.data(), &DeviceProcessSignalOperation::finished,
                    this, [this, signalOperation](const QString &error) {
                        if (error.isEmpty()) {
                            showMessage("Interrupted " + QString::number(inferiorPid()));
                            notifyInferiorStopOk();
                        } else {
                            showMessage(error, LogError);
                            notifyInferiorStopFailed();
                        }
                    });
            signalOperation->setDebuggerCommand(runParameters().debugger.command.executable());
            signalOperation->interruptProcess(inferiorPid());
        } else {
            interruptInferior2();
        }
    }
}

void GdbEngine::runCommand(const DebuggerCommand &command)
{
    const int token = ++currentToken();

    DebuggerCommand cmd = command;

    if (cmd.function.isEmpty()) {
        showMessage(QString("EMPTY FUNCTION RUN, TOKEN: %1 ARGS: %2")
                        .arg(token).arg(cmd.args.toString()));
        QTC_ASSERT(false, return);
    }

    if (!m_gdbProc.isRunning()) {
        showMessage(QString("NO GDB PROCESS RUNNING, CMD IGNORED: %1 %2")
            .arg(cmd.function).arg(state()));
        if (cmd.callback) {
            DebuggerResponse response;
            response.resultClass = ResultError;
            cmd.callback(response);
        }
        return;
    }

    if (cmd.flags & (NeedsTemporaryStop|NeedsFullStop)) {
        showMessage("RUNNING NEEDS-STOP COMMAND " + cmd.function);
        const bool wantContinue = bool(cmd.flags & NeedsTemporaryStop);
        cmd.flags &= ~(NeedsTemporaryStop|NeedsFullStop);
        if (state() == InferiorStopRequested) {
            if (cmd.flags & LosesChild) {
                notifyInferiorIll();
                return;
            }
            showMessage("CHILD ALREADY BEING INTERRUPTED. STILL HOPING.");
            // Calling shutdown() here breaks all situations where two
            // NeedsStop commands are issued in quick succession.
            m_onStop.append(cmd, wantContinue);
            return;
        }
        if (state() == InferiorRunOk) {
            showStatusMessage(Tr::tr("Stopping temporarily."), 1000);
            m_onStop.append(cmd, wantContinue);
            setState(InferiorStopRequested);
            interruptInferior();
            return;
        }
        if (state() != InferiorStopOk)
            showMessage("UNSAFE STATE FOR QUEUED COMMAND. EXECUTING IMMEDIATELY");
    }

    if (!(cmd.flags & Discardable))
        ++m_nonDiscardableCount;

    bool isPythonCommand = true;
    if ((cmd.flags & NativeCommand) || cmd.function.contains('-') || cmd.function.contains(' '))
        isPythonCommand = false;
    if (isPythonCommand) {
        cmd.arg("token", token);
        cmd.function = "python theDumper." + cmd.function + "(" + cmd.argsToPython() + ")";
    }

    QTC_ASSERT(m_gdbProc.isRunning(), return);

    cmd.postTime = QTime::currentTime().msecsSinceStartOfDay();
    m_commandForToken[token] = cmd;
    m_flagsForToken[token] = cmd.flags;
    if (cmd.flags & ConsoleCommand)
        cmd.function = "-interpreter-exec console \"" + cmd.function + '"';
    cmd.function = QString::number(token) + cmd.function;

    showMessage(cmd.function.left(100), LogInput);

    if (m_scheduledTestResponses.contains(token)) {
        // Fake response for test cases.
        QString buffer = m_scheduledTestResponses.value(token);
        buffer.replace("@TOKEN@", QString::number(token));
        m_scheduledTestResponses.remove(token);
        showMessage(QString("FAKING TEST RESPONSE (TOKEN: %2, RESPONSE: %3)")
                    .arg(token).arg(buffer));
        QMetaObject::invokeMethod(this, [this, buffer] { handleResponse(buffer); });
    } else {
        m_gdbProc.write(cmd.function + "\r\n");
        if (command.flags & NeedsFlush) {
            // We don't need the response or result here, just want to flush
            // anything that's still on the gdb side.
            m_gdbProc.write({"p 0\n"});
        }

        // Start Watchdog.
        if (m_commandTimer.interval() <= 20000)
            m_commandTimer.setInterval(commandTimeoutTime());
        // The process can die for external reason between the "-gdb-exit" was
        // sent and a response could be retrieved. We don't want the watchdog
        // to bark in that case since the only possible outcome is a dead
        // process anyway.
        if (!cmd.function.endsWith("-gdb-exit"))
            m_commandTimer.start();

        //if (cmd.flags & LosesChild)
        //    notifyInferiorIll();
    }
}

int GdbEngine::commandTimeoutTime() const
{
    const int time = settings().gdbWatchdogTimeout();
    return 1000 * qMax(20, time);
}

void GdbEngine::commandTimeout()
{
    const QList<int> keys = Utils::sorted(m_commandForToken.keys());
    bool killIt = false;
    for (int key : keys) {
        const DebuggerCommand &cmd = m_commandForToken.value(key);
        killIt = true;
        showMessage(QString::number(key) + ": " + cmd.function);
    }
    QStringList commands;
    for (const DebuggerCommand &cmd : std::as_const(m_commandForToken))
        commands << QString("\"%1\"").arg(cmd.function);
    if (killIt) {
        showMessage(QString("TIMED OUT WAITING FOR GDB REPLY. "
                      "COMMANDS STILL IN PROGRESS: ") + commands.join(", "));
        int timeOut = m_commandTimer.interval();
        //m_commandTimer.stop();
        const QString msg = Tr::tr("The gdb process has not responded "
            "to a command within %n seconds. This could mean it is stuck "
            "in an endless loop or taking longer than expected to perform "
            "the operation.\nYou can choose between waiting "
            "longer or aborting debugging.", nullptr, timeOut / 1000);
        QMessageBox *mb = showMessageBox(QMessageBox::Critical,
            Tr::tr("GDB Not Responding"), msg,
            QMessageBox::Ok | QMessageBox::Cancel);
        mb->button(QMessageBox::Cancel)->setText(Tr::tr("Give GDB More Time"));
        mb->button(QMessageBox::Ok)->setText(Tr::tr("Stop Debugging"));
        if (mb->exec() == QMessageBox::Ok) {
            showMessage("KILLING DEBUGGER AS REQUESTED BY USER");
            // This is an undefined state, so we just pull the emergency brake.
            m_gdbProc.kill();
            notifyEngineShutdownFinished();
        } else {
            showMessage("CONTINUE DEBUGGER AS REQUESTED BY USER");
        }
    } else {
        showMessage(QString("\nNON-CRITICAL TIMEOUT\nCOMMANDS STILL IN PROGRESS: ")
                    + commands.join(", "));
    }
}

void GdbEngine::handleResultRecord(DebuggerResponse *response)
{
    //qDebug() << "TOKEN:" << response->token
    //    << " ACCEPTABLE:" << m_oldestAcceptableToken;
    //qDebug() << "\nRESULT" << response->token << response->toString();

    int token = response->token;
    if (token <= 0)
        return;

    if (!m_commandForToken.contains(token)) {
        // In theory this should not happen (rather the error should be
        // reported in the "first" response to the command) in practice it
        // does. We try to handle a few situations we are aware of gracefully.
        // Ideally, this code should not be present at all.
        showMessage(QString("COOKIE FOR TOKEN %1 ALREADY EATEN (%2). "
                            "TWO RESPONSES FOR ONE COMMAND?").arg(token).
                    arg(stateName(state())));
        if (response->resultClass == ResultError) {
            QString msg = response->data["msg"].data();
            if (msg == "Cannot find new threads: generic error") {
                // Handle a case known to occur on Linux/gdb 6.8 when debugging moc
                // with helpers enabled. In this case we get a second response with
                // msg="Cannot find new threads: generic error"
                showMessage("APPLYING WORKAROUND #1");
                AsynchronousMessageBox::critical(Tr::tr("Executable Failed"), msg);
                showStatusMessage(Tr::tr("Process failed to start."));
                //shutdown();
                notifyInferiorIll();
            } else if (msg == "\"finish\" not meaningful in the outermost frame.") {
                // Handle a case known to appear on GDB 6.4 symbianelf when
                // the stack is cut due to access to protected memory.
                //showMessage("APPLYING WORKAROUND #2");
                notifyInferiorStopOk();
            } else if (msg.startsWith("Cannot find bounds of current function")) {
                // Happens when running "-exec-next" in a function for which
                // there is no debug information. Divert to "-exec-next-step"
                showMessage("APPLYING WORKAROUND #3");
                notifyInferiorStopOk();
                executeStepOver(true);
            } else if (msg.startsWith("Couldn't get registers: No such process.")) {
                // Happens on archer-tromey-python 6.8.50.20090910-cvs
                // There might to be a race between a process shutting down
                // and library load messages.
                showMessage("APPLYING WORKAROUND #4");
                notifyInferiorStopOk();
                //notifyInferiorIll();
                //showStatusMessage(Tr::tr("Executable failed: %1").arg(msg));
                //shutdown();
                //AsynchronousMessageBox::critical(Tr::tr("Executable Failed"), msg);
            } else if (msg.contains("Cannot insert breakpoint")) {
                // For breakpoints set by address to non-existent addresses we
                // might get something like "6^error,msg="Warning:\nCannot insert
                // breakpoint 3.\nError accessing memory address 0x34592327:
                // Input/output error.\nCannot insert breakpoint 4.\nError
                // accessing memory address 0x34592335: Input/output error.\n".
                // This should not stop us from proceeding.
                // Most notably, that happens after a "6^running" and "*running"
                // We are probably sitting at _start and can't proceed as
                // long as the breakpoints are enabled.
                // FIXME: Should we silently disable the offending breakpoints?
                showMessage("APPLYING WORKAROUND #5");
                AsynchronousMessageBox::critical(Tr::tr("Setting Breakpoints Failed"), msg);
                QTC_CHECK(state() == InferiorRunOk);
                notifyInferiorSpontaneousStop();
                notifyEngineIll();
            } else if (msg.startsWith("Process record: failed to record execution log.")) {
                // Reverse execution recording failed. Full message is something like
                // ~"Process record does not support instruction 0xfae64 at address 0x7ffff7dec6f8.\n"
                notifyInferiorSpontaneousStop();
                handleRecordingFailed();
            } else if (msg.startsWith("Target multi-thread does not support this command.")) {
                notifyInferiorSpontaneousStop();
                handleRecordingFailed();
            } else if (isGdbConnectionError(msg)) {
                notifyInferiorExited();
            } else {
                // Windows: Some DLL or some function not found. Report
                // the exception now in a box.
                if (msg.startsWith("During startup program exited with"))
                    notifyInferiorExited();
                else if (msg.contains("Command aborted."))
                    notifyInferiorSpontaneousStop();
                QString logMsg;
                if (!m_lastWinException.isEmpty())
                    logMsg = m_lastWinException + '\n';
                logMsg += msg;
                AsynchronousMessageBox::critical(Tr::tr("Executable Failed"), logMsg);
                showStatusMessage(Tr::tr("Executable failed: %1").arg(logMsg));
            }
        }
        return;
    }

    DebuggerCommand cmd = m_commandForToken.take(token);
    const int flags = m_flagsForToken.take(token);
    if (settings().logTimeStamps()) {
        showMessage(QString("Response time: %1: %2 s")
            .arg(cmd.function)
            .arg(QTime::fromMSecsSinceStartOfDay(cmd.postTime).msecsTo(QTime::currentTime()) / 1000.),
            LogTime);
    }

    if (response->token < m_oldestAcceptableToken && (flags & Discardable)) {
        //showMessage(_("### SKIPPING OLD RESULT") + response.toString());
        return;
    }

    bool isExpectedResult =
           (response->resultClass == ResultError) // Can always happen.
        || (response->resultClass == ResultRunning && (flags & RunRequest))
        || (response->resultClass == ResultExit && (flags & ExitRequest))
        || (response->resultClass == ResultDone);
        // ResultDone can almost "always" happen. Known examples are:
        //  (response->resultClass == ResultDone && cmd.function == "continue")
        // Happens with some incarnations of gdb 6.8 for "jump to line"
        //  (response->resultClass == ResultDone && cmd.function.startsWith("jump"))
        //  (response->resultClass == ResultDone && cmd.function.startsWith("detach"))
        // Happens when stepping finishes very quickly and issues *stopped and ^done
        // instead of ^running and *stopped
        //  (response->resultClass == ResultDone && (cmd.flags & RunRequest));

    if (!isExpectedResult) {
        const DebuggerRunParameters &rp = runParameters();
        Abi abi = rp.toolChainAbi;
        if (abi.os() == Abi::WindowsOS
            && cmd.function.startsWith("attach")
            && (rp.startMode == AttachToLocalProcess || terminal()))
        {
            // Ignore spurious 'running' responses to 'attach'.
        } else {
            QString rsp = DebuggerResponse::stringFromResultClass(response->resultClass);
            rsp = "UNEXPECTED RESPONSE '" + rsp + "' TO COMMAND '" + cmd.function + "'";
            qWarning("%s", qPrintable(rsp));
            showMessage(rsp);
        }
    }

    if (!(flags & Discardable))
        --m_nonDiscardableCount;

    m_inUpdateLocals = (flags & InUpdateLocals);

    if (cmd.callback)
        cmd.callback(*response);

    PENDING_DEBUG("MISSING TOKENS: " << m_commandForToken.keys());

    if (m_commandForToken.isEmpty())
        m_commandTimer.stop();
}

bool GdbEngine::acceptsDebuggerCommands() const
{
    return true;
//    return state() == InferiorStopOk
//        || state() == InferiorUnrunnable;
}

void GdbEngine::executeDebuggerCommand(const QString &command)
{
    QTC_CHECK(acceptsDebuggerCommands());
    runCommand({command, NativeCommand});
}

// This is triggered when switching snapshots.
void GdbEngine::updateAll()
{
    //PENDING_DEBUG("UPDATING ALL\n");
    QTC_CHECK(state() == InferiorUnrunnable || state() == InferiorStopOk);
    DebuggerCommand cmd(stackCommand(settings().maximalStackDepth()));
    cmd.callback = [this](const DebuggerResponse &r) { handleStackListFrames(r, false); };
    runCommand(cmd);
    stackHandler()->setCurrentIndex(0);
    runCommand({"-thread-info", CB(handleThreadInfo)});
    reloadRegisters();
    reloadPeripheralRegisters();
    updateLocals();
}

void GdbEngine::handleQuerySources(const DebuggerResponse &response)
{
    m_sourcesListUpdating = false;
    if (response.resultClass == ResultDone) {
        QMap<QString, FilePath> oldShortToFull = m_shortToFullName;
        m_shortToFullName.clear();
        m_fullToShortName.clear();
        // "^done,files=[{file="../../../../bin/dumper/dumper.cpp",
        // fullname="/data5/dev/ide/main/bin/dumper/dumper.cpp"},
        for (const GdbMi &item : response.data["files"]) {
            GdbMi fileName = item["file"];
            if (fileName.data().endsWith("<built-in>"))
                continue;
            GdbMi fullName = item["fullname"];
            QString file = fileName.data();
            FilePath full;
            if (fullName.isValid()) {
                full = cleanupFullName(fullName.data());
                m_fullToShortName[full] = file;
            }
            m_shortToFullName[file] = full;
        }
        if (m_shortToFullName != oldShortToFull)
            sourceFilesHandler()->setSourceFiles(m_shortToFullName);
    }
}

void GdbEngine::handleExecuteJumpToLine(const DebuggerResponse &response)
{
    if (response.resultClass == ResultRunning) {
        // All is fine. Waiting for a *running
        // and the temporary breakpoint to be hit.
        notifyInferiorRunOk(); // Only needed for gdb < 7.0.
    } else if (response.resultClass == ResultError) {
        // Could be "Unreasonable jump request" or similar.
        QString out = Tr::tr("Cannot jump. Stopped.");
        QString msg = response.data["msg"].data();
        if (!msg.isEmpty())
            out += ". " + msg;
        showStatusMessage(out);
        notifyInferiorRunFailed();
    } else if (response.resultClass == ResultDone) {
        // This happens on old gdb. Trigger the effect of a '*stopped'.
        showStatusMessage(Tr::tr("Jumped. Stopped."));
        notifyInferiorSpontaneousStop();
        handleStop2(response.data);
    }
}

void GdbEngine::handleExecuteRunToLine(const DebuggerResponse &response)
{
    if (response.resultClass == ResultRunning) {
        // All is fine. Waiting for a *running
        // and the temporary breakpoint to be hit.
    } else if (response.resultClass == ResultDone) {
        // This happens on old gdb (Mac). gdb is not stopped yet,
        // but merely accepted the continue.
        // >&"continue\n"
        // >~"Continuing.\n"
        //>~"testArray () at ../simple/app.cpp:241\n"
        //>~"241\t    s[1] = \"b\";\n"
        //>122^done
        showStatusMessage(Tr::tr("Target line hit, and therefore stopped."));
        notifyInferiorRunOk();
    }
}

static bool isExitedReason(const QString &reason)
{
    return reason == "exited-normally"   // inferior exited normally
        || reason == "exited-signalled"  // inferior exited because of a signal
        //|| reason == "signal-received" // inferior received signal
        || reason == "exited";           // inferior exited
}

void GdbEngine::handleStopResponse(const GdbMi &data)
{
    // Ignore trap on Windows terminals, which results in
    // spurious "* stopped" message.
    if (m_expectTerminalTrap) {
        if ((!data.isValid() || !data["reason"].isValid())
                && Abi::hostAbi().os() == Abi::WindowsOS) {
            m_expectTerminalTrap = false;
            showMessage("IGNORING TERMINAL SIGTRAP", LogMisc);
            return;
        }
    }

    if (isDying()) {
        notifyInferiorStopOk();
        return;
    }

    const QString reason = data["reason"].data();
    const GdbMi frame = data["frame"];

    // Jump over well-known frames.
    //static int stepCounter = 0;
    if (settings().skipKnownFrames()) {
        if (reason == "end-stepping-range" || reason == "function-finished") {
            //showMessage(frame.toString());
            QString funcName = frame["function"].data();
            QString fileName = frame["file"].data();
            if (isLeavableFunction(funcName, fileName)) {
                //showMessage(_("LEAVING ") + funcName);
                //++stepCounter;
                executeStepOut();
                return;
            }
            if (isSkippableFunction(funcName, fileName)) {
                //showMessage(_("SKIPPING ") + funcName);
                //++stepCounter;
                executeStepIn(false);
                return;
            }
            //if (stepCounter)
            //    qDebug() << "STEPCOUNTER:" << stepCounter;
            //stepCounter = 0;
        }
    }

    GdbMi threads = data["stopped-thread"];
    threadsHandler()->notifyStopped(threads.data());

    if (isExitedReason(reason)) {
        //   // The user triggered a stop, but meanwhile the app simply exited ...
        //    QTC_ASSERT(state() == InferiorStopRequested
        //            /*|| state() == InferiorStopRequested_Kill*/,
        //               qDebug() << state());
        QString msg;
        if (reason == "exited") {
            const int exitCode = data["exit-code"].toInt();
            notifyExitCode(exitCode);
            msg = Tr::tr("Application exited with exit code %1").arg(exitCode);
        } else if (reason == "exited-signalled" || reason == "signal-received") {
            msg = Tr::tr("Application exited after receiving signal %1")
                .arg(data["signal-name"].toString());
        } else {
            msg = Tr::tr("Application exited normally.");
        }
        // Only show the message. Ramp-down will be triggered by -thread-group-exited.
        showStatusMessage(msg);
        return;
    }

    if (!m_onStop.isEmpty()) {
        notifyInferiorStopOk();
        showMessage("HANDLING QUEUED COMMANDS AFTER TEMPORARY STOP", LogMisc);
        DebuggerCommandSequence seq = m_onStop;
        m_onStop = DebuggerCommandSequence();
        for (const DebuggerCommand &cmd : seq.commands())
            runCommand(cmd);
        if (seq.wantContinue())
            continueInferiorInternal();
        return;
    }

    const QString nr = data["bkptno"].data();
    int lineNumber = 0;
    FilePath fullName;
    QString function;
    QString language;
    if (frame.isValid()) {
        const GdbMi lineNumberG = frame["line"];
        function = frame["function"].data(); // V4 protocol
        if (function.isEmpty())
            function = frame["func"].data(); // GDB's *stopped messages
        language = frame["language"].data();
        if (lineNumberG.isValid()) {
            lineNumber = lineNumberG.toInt();
            fullName = cleanupFullName(frame["fullname"].data());
            if (fullName.isEmpty())
                fullName = runParameters().projectSourceDirectory.withNewPath(frame["file"].data());
        } // found line number
    } else {
        showMessage("INVALID STOPPED REASON", LogWarning);
    }

    const FilePath fileName = fullName.localSource().value_or(fullName);

    if (!nr.isEmpty() && frame.isValid()) {
        // Use opportunity to update the breakpoint marker position.
        if (Breakpoint bp = breakHandler()->findBreakpointByResponseId(nr)) {
            const FilePath &bpFileName = bp->fileName();
            if (!bpFileName.isEmpty())
                bp->setMarkerFileAndPosition(bpFileName, {lineNumber, -1});
            else if (!fileName.isEmpty())
                bp->setMarkerFileAndPosition(fileName, {lineNumber, -1});
        }
    }

    //qDebug() << "BP " << rid << data.toString();
    // Quickly set the location marker.
    if (lineNumber
            && !operatesByInstruction()
            && fileName.exists()
            && function != "qt_v4TriggeredBreakpointHook"
            && function != "qt_qmlDebugMessageAvailable"
            && language != "js")
        gotoLocation(Location(fileName, lineNumber));

    if (state() == InferiorRunOk) {
        // Stop triggered by a breakpoint or otherwise not directly
        // initiated by the user.
        notifyInferiorSpontaneousStop();
    } else if (state() == InferiorRunRequested) {
        // Stop triggered by something like "-exec-step\n"
        //  "&"Cannot access memory at address 0xbfffedd4\n"
        // or, on S40,
        //  "*running,thread-id="30""
        //  "&"Warning:\n""
        //  "&"Cannot insert breakpoint -33.\n"
        //  "&"Error accessing memory address 0x11673fc: Input/output error.\n""
        // In this case a proper response 94^error,msg="" will follow and
        // be handled in the result handler.
        // -- or --
        // *stopped arriving earlier than ^done response to an -exec-step
        notifyInferiorRunOk();
        notifyInferiorSpontaneousStop();
    } else if (state() == InferiorStopOk) {
        // That's expected.
    } else if (state() == InferiorStopRequested) {
        notifyInferiorStopOk();
    } else if (state() == EngineRunRequested) {
        // This is gdb 7+'s initial *stopped in response to attach that
        // appears before the ^done is seen for local setups.
        notifyEngineRunAndInferiorStopOk();
        if (terminal()) {
            continueInferiorInternal();
            return;
        }
    } else {
        QTC_CHECK(false);
    }

    CHECK_STATE(InferiorStopOk);

    handleStop1(data);
}

static QString stopSignal(const Abi &abi)
{
    return QLatin1String(abi.os() == Abi::WindowsOS ? "SIGTRAP" : "SIGINT");
}

void GdbEngine::handleStop1(const GdbMi &data)
{
    CHECK_STATE(InferiorStopOk);
    QTC_ASSERT(!isDying(), return);
    const GdbMi frame = data["frame"];
    const QString reason = data["reason"].data();

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
     if (m_ignoreNextTrap && reason == "signal-received"
             && data["signal-name"].data() == "SIGTRAP") {
         m_ignoreNextTrap = false;
         continueInferiorInternal();
         return;
     }

    // Show return value if possible, usually with reason "function-finished".
    // *stopped,reason="function-finished",frame={addr="0x080556da",
    // func="testReturnValue",args=[],file="/../app.cpp",
    // fullname="/../app.cpp",line="1611"},gdb-result-var="$1",
    // return-value="{d = 0x808d998}",thread-id="1",stopped-threads="all",
    // core="1"
    GdbMi resultVar = data["gdb-result-var"];
    if (resultVar.isValid())
        m_resultVarName = resultVar.data();
    else
        m_resultVarName.clear();

    if (!m_systemDumpersLoaded) {
        m_systemDumpersLoaded = true;
        if (m_gdbVersion >= 70400 && settings().loadGdbDumpers())
            runCommand({"importPlainDumpers on"});
        else
            runCommand({"importPlainDumpers off"});
    }

    handleStop2(data);
}

void GdbEngine::handleStop2(const GdbMi &data)
{
    CHECK_STATE(InferiorStopOk);
    QTC_ASSERT(!isDying(), return);

    // A user initiated stop looks like the following. Note that there is
    // this extra "stopper thread" created and "properly" reported by gdb.
    //
    // dNOTE: INFERIOR RUN OK
    // dState changed from InferiorRunRequested(10) to InferiorRunOk(11).
    // >*running,thread-id="all"
    // >=thread-exited,id="11",group-id="i1"
    // sThread 11 in group i1 exited
    // dState changed from InferiorRunOk(11) to InferiorStopRequested(13).
    // dCALL: INTERRUPT INFERIOR
    // sStop requested...
    // dTRYING TO INTERRUPT INFERIOR
    // >=thread-created,id="12",group-id="i1"
    // sThread 12 created
    // >~"[New Thread 8576.0x1154]\n"
    // s[New Thread 8576.0x1154]
    // >*running,thread-id="all"
    // >~"[Switching to Thread 8576.0x1154]\n"
    // >*stopped,reason="signal-received",signal-name="SIGTRAP",
    // signal-meaning="Trace/breakpointtrap",frame={addr="0x7c90120f",func=
    // "ntdll!DbgUiConnectToDbg",args=[],from="C:\\WINDOWS\\system32\\ntdll.dll"},
    // thread-id="12",stopped-threads="all"
    // dNOTE: INFERIOR STOP OK
    // dState changed from InferiorStopRequested(13) to InferiorStopOk(14).

    const QString reason = data["reason"].data();
    const DebuggerRunParameters &rp = runParameters();

    bool isStopperThread = false;

    if (rp.toolChainAbi.os() == Abi::WindowsOS
            && terminal()
            && reason == "signal-received"
            && data["signal-name"].data() == "SIGTRAP")
    {
        // This is the stopper thread. That also means that the
        // reported thread is not the one we'd like to expose
        // to the user.
        isStopperThread = true;
    }

    if (reason == "watchpoint-trigger") {
        // *stopped,reason="watchpoint-trigger",wpt={number="2",exp="*0xbfffed40"},
        // value={old="1",new="0"},frame={addr="0x00451e1b",
        // func="QScopedPointer",args=[{name="this",value="0xbfffed40"},
        // {name="p",value="0x0"}],file="x.h",fullname="/home/.../x.h",line="95"},
        // thread-id="1",stopped-threads="all",core="2"
        const GdbMi wpt = data["wpt"];
        const QString rid = wpt["number"].data();
        const Breakpoint bp = breakHandler()->findBreakpointByResponseId(rid);
        const quint64 bpAddress = wpt["exp"].data().mid(1).toULongLong(nullptr, 0);
        QString msg;
        if (bp) {
            if (bp->type() == WatchpointAtExpression)
                msg = bp->msgWatchpointByExpressionTriggered(bp->expression());
            if (bp->type() == WatchpointAtAddress)
                msg = bp->msgWatchpointByAddressTriggered(bpAddress);
        }
        GdbMi value = data["value"];
        GdbMi oldValue = value["old"];
        GdbMi newValue = value["new"];
        if (oldValue.isValid() && newValue.isValid()) {
            msg += ' ';
            msg += Tr::tr("Value changed from %1 to %2.")
                .arg(oldValue.data()).arg(newValue.data());
        }
        showStatusMessage(msg);
    } else if (reason == "breakpoint-hit") {
        GdbMi gNumber = data["bkptno"]; // 'number' or 'bkptno'?
        if (!gNumber.isValid())
            gNumber = data["number"];
        const QString rid = gNumber.data();
        const QString threadId = data["thread-id"].data();
        m_currentThread = threadId;
        if (const Breakpoint bp = breakHandler()->findBreakpointByResponseId(rid)) {
            showStatusMessage(bp->msgBreakpointTriggered(threadId));
            const QString commands = bp->command().trimmed();
            // Can be either c or cont[inue]
            const QRegularExpression contExp("(^|\\n)\\s*c(ont(i(n(ue?)?)?)?)?$");
            QTC_CHECK(contExp.isValid());
            if (contExp.match(commands).hasMatch()) {
                notifyInferiorRunRequested();
                notifyInferiorRunOk();
                return;
            }
        }
    } else {
        QString reasontr = msgStopped(reason);
        if (reason == "signal-received") {
            QString name = data["signal-name"].data();
            QString meaning = data["signal-meaning"].data();
            // Ignore these as they are showing up regularly when
            // stopping debugging.
            if (name == stopSignal(rp.toolChainAbi) || rp.expectedSignals.contains(name)) {
                showMessage(name + " CONSIDERED HARMLESS. CONTINUING.");
            } else if (m_isQnxGdb && name == "0" && meaning == "Signal 0") {
                showMessage("SIGNAL 0 CONSIDERED BOGUS.");
            } else {
                if (terminal() && name == "SIGCONT" && m_expectTerminalTrap) {
                    continueInferior();
                    m_expectTerminalTrap = false;
                } else {
                    showMessage("HANDLING SIGNAL " + name);
                    if (settings().useMessageBoxForSignals() && !isStopperThread)
                        if (!showStoppedBySignalMessageBox(meaning, name)) {
                            showMessage("SIGNAL RECEIVED WHILE SHOWING SIGNAL MESSAGE");
                            return;
                        }
                    if (!name.isEmpty() && !meaning.isEmpty())
                        reasontr = msgStoppedBySignal(meaning, name);
                }
            }
        }
        if (reason.isEmpty())
            showStatusMessage(msgStopped());
        else
            showStatusMessage(reasontr);
    }

    // Let the event loop run before deciding whether to update the stack.
    m_stackNeeded = true; // setTokenBarrier() might reset this.
    QTimer::singleShot(0, this, &GdbEngine::handleStop3);
}

void GdbEngine::handleStop3()
{
    if (!terminal() || state() != InferiorRunOk) {
        DebuggerCommand cmd("-thread-info", Discardable);
        cmd.callback = CB(handleThreadInfo);
        runCommand(cmd);
    }
}

void GdbEngine::handleShowVersion(const DebuggerResponse &response)
{
    showMessage("PARSING VERSION: " + response.toString());
    if (response.resultClass == ResultDone) {
        bool isMacGdb = false;
        int gdbBuildVersion = -1;
        m_gdbVersion = 100;
        m_isQnxGdb = false;
        QString msg = response.consoleStreamOutput;
        extractGdbVersion(msg,
              &m_gdbVersion, &gdbBuildVersion, &isMacGdb, &m_isQnxGdb);

        // On Mac, FSF GDB does not work sufficiently well,
        // and on Linux and Windows we require at least 7.4.1,
        // on Android 7.3.1.
        bool isSupported = m_gdbVersion >= 70300;
        if (isSupported)
            showMessage("SUPPORTED GDB VERSION " + msg);
        else
            showMessage("UNSUPPORTED GDB VERSION " + msg);

        showMessage(QString("USING GDB VERSION: %1, BUILD: %2%3").arg(m_gdbVersion)
            .arg(gdbBuildVersion).arg(QLatin1String(isMacGdb ? " (APPLE)" : "")));

        if (usesExecInterrupt())
            runCommand({"set target-async on", ConsoleCommand});
        else
            runCommand({"set target-async off", ConsoleCommand});

        //runCommand("set build-id-verbose 2", ConsoleCommand);
    }
}

void GdbEngine::handlePythonSetup(const DebuggerResponse &response)
{
    CHECK_STATE(EngineSetupRequested);
    if (response.resultClass == ResultDone) {
        GdbMi data = response.data;
        watchHandler()->addDumpers(data["dumpers"]);
        m_pythonVersion = data["python"].toInt();
        if (m_pythonVersion < 20700) {
            int pythonMajor = m_pythonVersion / 10000;
            int pythonMinor = (m_pythonVersion / 100) % 100;
            QString out = "<p>"
                          + Tr::tr("The selected build of GDB supports Python scripting, "
                                   "but the used version %1.%2 is not sufficient for "
                                   "%3. Supported versions are Python 2.7 and 3.x.")
                                .arg(pythonMajor)
                                .arg(pythonMinor)
                                .arg(QGuiApplication::applicationDisplayName());
            showStatusMessage(out);
            AsynchronousMessageBox::critical(Tr::tr("Execution Error"), out);
        }
        loadInitScript();
        CHECK_STATE(EngineSetupRequested);
        showMessage("ENGINE SUCCESSFULLY STARTED");
        setupInferior();
    } else {
        QString msg = response.data["msg"].data();
        if (msg.contains("Python scripting is not supported in this copy of GDB.")) {
            QString out1 = "The selected build of GDB does not support Python scripting.";
            QString out2 = QStringLiteral("It cannot be used in %1.")
                               .arg(QGuiApplication::applicationDisplayName());
            showStatusMessage(out1 + ' ' + out2);
            AsynchronousMessageBox::critical(Tr::tr("Execution Error"), out1 + "<br>" + out2);
        }
        notifyEngineSetupFailed();
    }
}

void GdbEngine::showExecutionError(const QString &message)
{
    AsynchronousMessageBox::critical(Tr::tr("Execution Error"),
       Tr::tr("Cannot continue debugged process:") + '\n' + message);
}

void GdbEngine::handleExecuteContinue(const DebuggerResponse &response)
{
    CHECK_STATE(InferiorRunRequested);
    if (response.resultClass == ResultRunning) {
        // All is fine. Waiting for a *running.
        notifyInferiorRunOk(); // Only needed for gdb < 7.0.
        return;
    }
    QString msg = response.data["msg"].data();
    if (msg.startsWith("Cannot find bounds of current function")) {
        notifyInferiorRunFailed();
        if (isDying())
            return;
        CHECK_STATE(InferiorStopOk);
        showStatusMessage(Tr::tr("Stopped."), 5000);
        reloadStack();
    } else if (msg.startsWith("Cannot access memory at address")) {
        // Happens on single step on ARM prolog and epilogs.
    } else if (msg.startsWith("\"finish\" not meaningful in the outermost frame")) {
        notifyInferiorRunFailed();
        if (isDying())
            return;
        CHECK_STATE(InferiorStopOk);
        // FIXME: Fix translation in master.
        showStatusMessage(msg, 5000);
        gotoCurrentLocation();
    } else if (msg.startsWith("Cannot execute this command while the selected thread is running.")) {
        showExecutionError(msg);
        notifyInferiorRunFailed() ;
    } else {
        showExecutionError(msg);
        notifyInferiorIll();
    }
}

FilePath GdbEngine::fullName(const QString &fileName)
{
    if (fileName.isEmpty())
        return {};
    QTC_CHECK(!m_sourcesListUpdating);
    return m_shortToFullName.value(fileName, {});
}

FilePath GdbEngine::cleanupFullName(const QString &fileName)
{
    FilePath cleanFilePath =
        runParameters().projectSourceDirectory.withNewPath(fileName).cleanPath();

    // Gdb running on windows often delivers "fullnames" which
    // (a) have no drive letter and (b) are not normalized.
    if (Abi::hostAbi().os() == Abi::WindowsOS) {
        if (fileName.isEmpty())
            return {};
    }

    if (!settings().autoEnrichParameters())
        return cleanFilePath;

    if (cleanFilePath.isReadableFile())
        return cleanFilePath;

    const FilePath sysroot = runParameters().sysRoot;
    if (!sysroot.isEmpty() && fileName.startsWith('/')) {
        cleanFilePath = sysroot.pathAppended(fileName.mid(1));
        if (cleanFilePath.isReadableFile())
            return cleanFilePath;
    }
    if (m_baseNameToFullName.isEmpty()) {
        FilePath filePath = sysroot.pathAppended("/usr/src/debug");

        if (filePath.isDir()) {
            filePath.iterateDirectory(
                [this](const FilePath &filePath) {
                    QString name = filePath.fileName();
                    if (!name.startsWith('.'))
                        m_baseNameToFullName.insert(name, filePath);
                    return IterationPolicy::Continue;
                },
                {{"*"}, QDir::NoFilter, QDirIterator::Subdirectories});
        }
    }

    cleanFilePath.clear();
    const QString base = FilePath::fromUserInput(fileName).fileName();

    auto jt = m_baseNameToFullName.constFind(base);
    while (jt != m_baseNameToFullName.constEnd() && jt.key() == base) {
        // FIXME: Use some heuristics to find the "best" match.
        return jt.value();
        //++jt;
    }

    return cleanFilePath;
}

void GdbEngine::shutdownInferior()
{
    CHECK_STATE(InferiorShutdownRequested);
    DebuggerCommand cmd;
    cmd.function = QLatin1String(runParameters().closeMode == DetachAtClose ? "detach " : "kill ");
    cmd.callback = CB(handleInferiorShutdown);
    cmd.flags = NeedsTemporaryStop|LosesChild;
    runCommand(cmd);
}

void GdbEngine::handleInferiorShutdown(const DebuggerResponse &response)
{
    if (response.resultClass == ResultDone) {
        // We'll get async thread-group-exited responses to which we react.
        // Nothing to do here.
        // notifyInferiorShutdownFinished();
        return;
    }
    // "kill" got stuck, gdb was kill -9'd, or similar.
    CHECK_STATE(InferiorShutdownRequested);
    QString msg = response.data["msg"].data();
    if (msg.contains(": No such file or directory.")) {
        // This happens when someone removed the binary behind our back.
        // It is not really an error from a user's point of view.
        showMessage("NOTE: " + msg);
    } else if (m_gdbProc.isRunning()) {
        AsynchronousMessageBox::critical(Tr::tr("Failed to Shut Down Application"),
                                         msgInferiorStopFailed(msg));
    }
    notifyInferiorShutdownFinished();
}

void GdbEngine::handleGdbExit(const DebuggerResponse &response)
{
    if (response.resultClass == ResultExit) {
        showMessage("GDB CLAIMS EXIT; WAITING");
        // Don't set state here, this will be handled in handleGdbFinished()
        //notifyEngineShutdownOk();
    } else {
        QString msg = msgGdbStopFailed(response.data["msg"].data());
        qDebug() << QString("GDB WON'T EXIT (%1); KILLING IT").arg(msg);
        showMessage(QString("GDB WON'T EXIT (%1); KILLING IT").arg(msg));
        m_gdbProc.kill();
        notifyEngineShutdownFinished();
    }
}

void GdbEngine::setLinuxOsAbi()
{
    // In case GDB has multiple supported targets, the default osabi can be Cygwin.
    if (!HostOsInfo::isWindowsHost())
        return;
    const DebuggerRunParameters &rp = runParameters();
    bool isElf = (rp.toolChainAbi.binaryFormat() == Abi::ElfFormat);
    if (!isElf && !rp.inferior.command.isEmpty()) {
        isElf = Utils::anyOf(Abi::abisOfBinary(rp.inferior.command.executable()), [](const Abi &abi) {
            return abi.binaryFormat() == Abi::ElfFormat;
        });
    }
    if (isElf)
        runCommand({"set osabi GNU/Linux"});
}

void GdbEngine::detachDebugger()
{
    CHECK_STATE(InferiorStopOk);
    QTC_CHECK(runParameters().startMode != AttachToCore);
    DebuggerCommand cmd("detach", NativeCommand | ExitRequest);
    cmd.callback = [this](const DebuggerResponse &) {
        CHECK_STATE(InferiorStopOk);
        notifyInferiorExited();
    };
    runCommand(cmd);
}

void GdbEngine::handleThreadGroupCreated(const GdbMi &result)
{
    QString groupId = result["id"].data();
    QString pid = result["pid"].data();
    threadsHandler()->notifyGroupCreated(groupId, pid);
}

void GdbEngine::handleThreadGroupExited(const GdbMi &result)
{
    QString groupId = result["id"].data();
    if (threadsHandler()->notifyGroupExited(groupId)) {
        const int exitCode = result["exit-code"].toInt();
        notifyExitCode(exitCode);
        if (m_rerunPending)
            m_rerunPending = false;
        else if (state() == EngineShutdownRequested)
            notifyEngineShutdownFinished();
        else
            notifyInferiorExited();
    }
}

static QString msgNoGdbBinaryForToolChain(const Abi &tc)
{
    return Tr::tr("There is no GDB binary available for binaries in format \"%1\".")
        .arg(tc.toString());
}

bool GdbEngine::hasCapability(unsigned cap) const
{
    if (cap & (AutoDerefPointersCapability
               | DisassemblerCapability
               | RegisterCapability
               | ShowMemoryCapability
               | CreateFullBacktraceCapability
               | AddWatcherCapability
               | ShowModuleSymbolsCapability
               | ShowModuleSectionsCapability
               | OperateByInstructionCapability
               | WatchComplexExpressionsCapability
               | MemoryAddressCapability
               | AdditionalQmlStackCapability)) {
        return true;
    }

    if (runParameters().startMode == AttachToCore)
        return false;

    return cap & (JumpToLineCapability
                  | ReloadModuleCapability
                  | ReloadModuleSymbolsCapability
                  | BreakOnThrowAndCatchCapability
                  | BreakConditionCapability
                  | BreakIndividualLocationsCapability
                  | TracePointCapability
                  | ReturnFromFunctionCapability
                  | WatchpointByAddressCapability
                  | WatchpointByExpressionCapability
                  | AddWatcherWhileRunningCapability
                  | WatchWidgetsCapability
                  | CatchCapability
                  | RunToLineCapability
                  | MemoryAddressCapability
                  | AdditionalQmlStackCapability
                  | ResetInferiorCapability
                  | SnapshotCapability
                  | ReverseSteppingCapability);
}


void GdbEngine::continueInferiorInternal()
{
    CHECK_STATE(InferiorStopOk);
    notifyInferiorRunRequested();
    showStatusMessage(Tr::tr("Running requested..."), 5000);
    CHECK_STATE(InferiorRunRequested);
    if (isNativeMixedActiveFrame()) {
        DebuggerCommand cmd("executeContinue", RunRequest);
        cmd.callback = CB(handleExecuteContinue);
        runCommand(cmd);
    } else {
        DebuggerCommand cmd("-exec-continue");
        cmd.flags = RunRequest | NeedsFlush;
        cmd.callback = CB(handleExecuteContinue);
        runCommand(cmd);
    }
}

void GdbEngine::continueInferior()
{
    CHECK_STATE(InferiorStopOk);
    setTokenBarrier();
    continueInferiorInternal();
}

void GdbEngine::executeStepIn(bool byInstruction)
{
    CHECK_STATE(InferiorStopOk);
    setTokenBarrier();
    notifyInferiorRunRequested();
    showStatusMessage(Tr::tr("Step requested..."), 5000);

    DebuggerCommand cmd;
    if (isNativeMixedActiveFrame()) {
        cmd.flags = RunRequest;
        cmd.function = "executeStep";
        cmd.callback = CB(handleExecuteStep);
    } else if (byInstruction) {
        cmd.flags = RunRequest|NeedsFlush;
        cmd.function = "-exec-step-instruction";
        if (isReverseDebugging())
            cmd.function += "--reverse";
        cmd.callback = CB(handleExecuteContinue);
    } else {
        cmd.flags = RunRequest|NeedsFlush;
        cmd.function = "-exec-step";
        if (isReverseDebugging())
            cmd.function += " --reverse";
        cmd.callback = CB(handleExecuteStep);
    }
    runCommand(cmd);
}

void GdbEngine::handleExecuteStep(const DebuggerResponse &response)
{
    if (response.resultClass == ResultDone) {
        // Step was finishing too quick, and a '*stopped' messages should
        // have preceded it, so just ignore this result.
        QTC_CHECK(state() == InferiorStopOk);
        return;
    }
    CHECK_STATE(InferiorRunRequested);
    if (response.resultClass == ResultRunning) {
        // All is fine. Waiting for a *running.
        notifyInferiorRunOk(); // Only needed for gdb < 7.0.
        return;
    }
    QString msg = response.data["msg"].data();
    if (msg.startsWith("Cannot find bounds of current function")
            || msg.contains("Error accessing memory address")
            || msg.startsWith("Cannot access memory at address")) {
        // On S40: "40^error,msg="Warning:\nCannot insert breakpoint -39.\n"
        //" Error accessing memory address 0x11673fc: Input/output error.\n"
        notifyInferiorRunFailed();
        if (isDying())
            return;
        executeStepIn(true); // Fall back to instruction-wise stepping.
    } else if (msg.startsWith("Cannot execute this command while the selected thread is running.")) {
        showExecutionError(msg);
        notifyInferiorRunFailed();
    } else if (msg.startsWith("warning: SuspendThread failed")) {
        // On Win: would lead to "PC register is not available" or "\312"
        continueInferiorInternal();
    } else {
        showExecutionError(msg);
        notifyInferiorIll();
    }
}

void GdbEngine::executeStepOut()
{
    CHECK_STATE(InferiorStopOk);
    runCommand({"-stack-select-frame 0", Discardable});
    setTokenBarrier();
    notifyInferiorRunRequested();
    showStatusMessage(Tr::tr("Finish function requested..."), 5000);
    if (isNativeMixedActiveFrame()) {
        runCommand({"executeStepOut", RunRequest});
    } else {
        // -exec-finish in 'main' results (correctly) in
        //  40^error,msg="\"finish\" not meaningful in the outermost frame."
        // However, this message does not seem to get flushed before
        // anything else happen - i.e. "never". Force some extra output.
        runCommand({"-exec-finish", RunRequest|NeedsFlush, CB(handleExecuteContinue)});
    }
}

void GdbEngine::executeStepOver(bool byInstruction)
{
    CHECK_STATE(InferiorStopOk);
    setTokenBarrier();
    notifyInferiorRunRequested();
    showStatusMessage(Tr::tr("Step next requested..."), 5000);

    DebuggerCommand cmd;
    cmd.flags = RunRequest;
    if (isNativeMixedActiveFrame()) {
        cmd.function = "executeNext";
    } else if (byInstruction) {
        cmd.function = "-exec-next-instruction";
        if (isReverseDebugging())
            cmd.function += " --reverse";
        cmd.callback = CB(handleExecuteContinue);
    } else {
        cmd.function = "-exec-next";
        if (isReverseDebugging())
            cmd.function += " --reverse";
        cmd.callback = CB(handleExecuteNext);
    }
    runCommand(cmd);
}

void GdbEngine::handleExecuteNext(const DebuggerResponse &response)
{
    if (response.resultClass == ResultDone) {
        // Step was finishing too quick, and a '*stopped' messages should
        // have preceded it, so just ignore this result.
        CHECK_STATE(InferiorStopOk);
        return;
    }
    CHECK_STATE(InferiorRunRequested);
    if (response.resultClass == ResultRunning) {
        // All is fine. Waiting for a *running.
        notifyInferiorRunOk(); // Only needed for gdb < 7.0.
        return;
    }
    CHECK_STATE(InferiorStopOk);
    QString msg = response.data["msg"].data();
    if (msg.startsWith("Cannot find bounds of current function")
            || msg.contains("Error accessing memory address ")) {
        notifyInferiorRunFailed();
        if (!isDying())
            executeStepOver(true); // Fall back to instruction-wise stepping.
    } else if (msg.startsWith("Cannot execute this command while the selected thread is running.")) {
        showExecutionError(msg);
        notifyInferiorRunFailed();
    } else if (msg.startsWith("Target multi-thread does not support this command.")) {
        notifyInferiorRunFailed();
        handleRecordingFailed();
    } else {
        AsynchronousMessageBox::warning(Tr::tr("Execution Error"),
           Tr::tr("Cannot continue debugged process:") + '\n' + msg);
        //notifyInferiorIll();
    }
}

static QString addressSpec(quint64 address)
{
    return "*0x" + QString::number(address, 16);
}

void GdbEngine::executeRunToLine(const ContextData &data)
{
    CHECK_STATE(InferiorStopOk);
    setTokenBarrier();
    notifyInferiorRunRequested();
    showStatusMessage(Tr::tr("Run to line %1 requested...").arg(data.textPosition.line), 5000);
#if 1
    QString loc;
    if (data.address)
        loc = addressSpec(data.address);
    else
        loc = '"' + breakLocation(data.fileName) + '"' + ':' + QString::number(data.textPosition.line);
    runCommand({"tbreak " + loc});

    runCommand({"continue", NativeCommand|RunRequest, CB(handleExecuteRunToLine)});
#else
    // Seems to jump to unpredicatable places. Observed in the manual
    // tests in the Foo::Foo() constructor with both gdb 6.8 and 7.1.
    QString args = '"' + breakLocation(fileName) + '"' + ':' + QString::number(lineNumber);
    runCommand("-exec-until " + args, RunRequest, CB(handleExecuteContinue));
#endif
}

void GdbEngine::executeRunToFunction(const QString &functionName)
{
    CHECK_STATE(InferiorStopOk);
    setTokenBarrier();
    runCommand({"-break-insert -t " + functionName});
    showStatusMessage(Tr::tr("Run to function %1 requested...").arg(functionName), 5000);
    continueInferiorInternal();
}

void GdbEngine::executeJumpToLine(const ContextData &data)
{
    CHECK_STATE(InferiorStopOk);
    QString loc;
    if (data.address)
        loc = addressSpec(data.address);
    else
        loc = '"' + breakLocation(data.fileName) + '"' + ':' + QString::number(data.textPosition.line);
    runCommand({"tbreak " + loc});
    notifyInferiorRunRequested();

    runCommand({"jump " + loc, RunRequest, CB(handleExecuteJumpToLine)});
    // will produce something like
    //  &"jump \"/home/apoenitz/dev/work/test1/test1.cpp\":242"
    //  ~"Continuing at 0x4058f3."
    //  ~"run1 (argc=1, argv=0x7fffbf1f5538) at test1.cpp:242"
    //  ~"242\t x *= 2;"
    //  23^done"
}

void GdbEngine::executeReturn()
{
    CHECK_STATE(InferiorStopOk);
    setTokenBarrier();
    notifyInferiorRunRequested();
    showStatusMessage(Tr::tr("Immediate return from function requested..."), 5000);
    runCommand({"-exec-return", RunRequest, CB(handleExecuteReturn)});
}

void GdbEngine::executeRecordReverse(bool record)
{
    if (record)
        runCommand({"record full"});
    else
        runCommand({"record stop"});
}

void GdbEngine::handleExecuteReturn(const DebuggerResponse &response)
{
    if (response.resultClass == ResultDone) {
        notifyInferiorStopOk();
        updateAll();
        return;
    }
    notifyInferiorRunFailed();
}

/*!
    Discards the results of all pending watch-updating commands.

    This function is called at the beginning of all step, next, finish, and so on,
    debugger functions.
    If non-watch-updating commands with call-backs are still in the pipe,
    it will complain.
*/
void GdbEngine::setTokenBarrier()
{
    //QTC_ASSERT(m_nonDiscardableCount == 0, /**/);
    bool good = true;
    for (auto it = m_commandForToken.cbegin(), end = m_commandForToken.cend(); it != end; ++it) {
        if (!(m_flagsForToken.value(it.key()) & Discardable)) {
            qDebug() << "TOKEN: " << it.key() << "CMD:" << it.value().function;
            good = false;
        }
    }
    QTC_ASSERT(good, return);
    PENDING_DEBUG("\n--- token barrier ---\n");
    showMessage("--- token barrier ---", LogMiscInput);
    if (settings().logTimeStamps())
        showMessage(LogWindow::logTimeStamp(), LogMiscInput);
    m_oldestAcceptableToken = currentToken();
    m_stackNeeded = false;
}


//////////////////////////////////////////////////////////////////////
//
// Breakpoint specific stuff
//
//////////////////////////////////////////////////////////////////////

QString GdbEngine::breakLocation(const FilePath &file) const
{
    QString where = m_fullToShortName.value(file);
    if (where.isEmpty())
        return file.fileName();
    return where;
}

QString GdbEngine::breakpointLocation(const BreakpointParameters &data)
{
    QTC_ASSERT(data.type != UnknownBreakpointType, return QString());
    // FIXME: Non-GCC-runtime
    if (data.type == BreakpointAtThrow)
        return QLatin1String("__cxa_throw");
    if (data.type == BreakpointAtCatch)
        return QLatin1String("__cxa_begin_catch");
    if (data.type == BreakpointAtMain)
        return mainFunction();
    if (data.type == BreakpointByFunction)
        return "--function \"" + data.functionName + '"';
    if (data.type == BreakpointByAddress)
        return addressSpec(data.address);

    BreakpointPathUsage usage = data.pathUsage;
    if (usage == BreakpointPathUsageEngineDefault)
        usage = BreakpointUseShortPath;

    const QString fileName = usage == BreakpointUseFullPath
        ? data.fileName.path() : breakLocation(data.fileName);
    // The argument is simply a C-quoted version of the argument to the
    // non-MI "break" command, including the "original" quoting it wants.
    return "\"\\\"" + GdbMi::escapeCString(fileName) + "\\\":"
        + QString::number(data.textPosition.line) + '"';
}

QString GdbEngine::breakpointLocation2(const BreakpointParameters &data)
{
    BreakpointPathUsage usage = data.pathUsage;
    if (usage == BreakpointPathUsageEngineDefault)
        usage = BreakpointUseShortPath;

    const QString fileName = usage == BreakpointUseFullPath
        ? data.fileName.path() : breakLocation(data.fileName);
    return GdbMi::escapeCString(fileName) + ':' + QString::number(data.textPosition.line);
}

void GdbEngine::handleInsertInterpreterBreakpoint(const DebuggerResponse &response,
                                                  const Breakpoint &bp)
{
    QTC_ASSERT(bp, return);
    const bool pending = response.data["pending"].toInt();
    if (pending) {
        notifyBreakpointInsertOk(bp);
    } else {
        bp->setResponseId(response.data["number"].data());
        bp->updateFromGdbOutput(response.data, runParameters().projectSourceDirectory);
        notifyBreakpointInsertOk(bp);
    }
}

void GdbEngine::handleInterpreterBreakpointModified(const GdbMi &data)
{
    int modelId = data["modelid"].toInt();
    Breakpoint bp = breakHandler()->findBreakpointByModelId(modelId);
    QTC_ASSERT(bp, return);
    bp->updateFromGdbOutput(data, runParameters().projectSourceDirectory);
}

void GdbEngine::handleWatchInsert(const DebuggerResponse &response, const Breakpoint &bp)
{
    if (bp && response.resultClass == ResultDone) {
        // "Hardware watchpoint 2: *0xbfffed40\n"
        QString ba = response.consoleStreamOutput;
        GdbMi wpt = response.data["wpt"];
        if (wpt.isValid()) {
            // Mac yields:
            //>32^done,wpt={number="4",exp="*4355182176"}
            bp->setResponseId(wpt["number"].data());
            QString exp = wpt["exp"].data();
            if (exp.startsWith('*'))
                bp->setAddress(exp.mid(1).toULongLong(nullptr, 0));
            QTC_CHECK(!bp->needsChange());
            notifyBreakpointInsertOk(bp);
        } else if (ba.startsWith("Hardware watchpoint ")
                || ba.startsWith("Watchpoint ")) {
            // Non-Mac: "Hardware watchpoint 2: *0xbfffed40\n"
            const int end = ba.indexOf(':');
            const int begin = ba.lastIndexOf(' ', end) + 1;
            const QString address = ba.mid(end + 2).trimmed();
            bp->setResponseId(ba.mid(begin, end - begin));
            if (address.startsWith('*'))
                bp->setAddress(address.mid(1).toULongLong(nullptr, 0));
            QTC_CHECK(!bp->needsChange());
            notifyBreakpointInsertOk(bp);
        } else {
            showMessage("CANNOT PARSE WATCHPOINT FROM " + ba);
        }
    }
}

void GdbEngine::handleCatchInsert(const DebuggerResponse &response, const Breakpoint &bp)
{
    if (response.resultClass == ResultDone)
        notifyBreakpointInsertOk(bp);
}

void GdbEngine::handleBkpt(const GdbMi &bkpt, const Breakpoint &bp)
{
    QTC_ASSERT(bp, return);
    const bool usePseudoTracepoints = settings().usePseudoTracepoints();
    const QString nr = bkpt["number"].data();
    if (nr.contains('.')) {
        // A sub-breakpoint.
        SubBreakpoint sub = bp->findOrCreateSubBreakpoint(nr);
        QTC_ASSERT(sub, return);
        sub->params.updateFromGdbOutput(bkpt, runParameters().projectSourceDirectory);
        sub->params.type = bp->type();
        if (usePseudoTracepoints && bp->isTracepoint()) {
            sub->params.tracepoint = true;
            sub->params.message = bp->message();
        }
        return;
    }

    // The MI output format might change, see
    // http://permalink.gmane.org/gmane.comp.gdb.patches/83936
    const GdbMi locations = bkpt["locations"];
    if (locations.isValid()) {
        for (const GdbMi &location : locations) {
            // A sub-breakpoint.
            const QString subnr = location["number"].data();
            SubBreakpoint sub = bp->findOrCreateSubBreakpoint(subnr);
            QTC_ASSERT(sub, return);
            sub->params.updateFromGdbOutput(location, runParameters().projectSourceDirectory);
            sub->params.type = bp->type();
            if (usePseudoTracepoints && bp->isTracepoint()) {
                sub->params.tracepoint = true;
                sub->params.message = bp->message();
            }
        }
    }

    // A (the?) primary breakpoint.
    bp->setResponseId(nr);
    bp->updateFromGdbOutput(bkpt, runParameters().projectSourceDirectory);
    if (usePseudoTracepoints && bp->isTracepoint())
        bp->setMessage(bp->requestedParameters().message);
}

void GdbEngine::handleBreakInsert1(const DebuggerResponse &response, const Breakpoint &bp)
{
    QTC_ASSERT(bp, return);
    if (bp->state() == BreakpointRemoveRequested) {
        if (response.resultClass == ResultDone) {
            // This delete was deferred. Act now.
            const GdbMi mainbkpt = response.data["bkpt"];
            notifyBreakpointRemoveProceeding(bp);
            DebuggerCommand cmd("-break-delete " + mainbkpt["number"].data());
            cmd.flags = NeedsTemporaryStop;
            runCommand(cmd);
            notifyBreakpointRemoveOk(bp);
            return;
        }
    }
    if (response.resultClass == ResultDone) {
        // The result is a list with the first entry marked "bkpt"
        // and "unmarked" rest. The "bkpt" one seems to always be
        // the "main" entry. Use the "main" entry to retrieve the
        // already known data from the BreakpointManager, and then
        // iterate over all items to update main- and sub-data.
        for (const GdbMi &bkpt : response.data)
            handleBkpt(bkpt, bp);
        if (bp->needsChange()) {
            bp->gotoState(BreakpointUpdateRequested, BreakpointInsertionProceeding);
            updateBreakpoint(bp);
        } else {
            notifyBreakpointInsertOk(bp);
        }
    } else if (response.data["msg"].data().contains("Unknown option")) {
        // Older version of gdb don't know the -a option to set tracepoints
        // ^error,msg="mi_cmd_break_insert: Unknown option ``a''"
        const QString fileName = bp->fileName().toString();
        const int lineNumber = bp->textPosition().line;
        DebuggerCommand cmd("trace \"" + GdbMi::escapeCString(fileName) + "\":"
                            + QString::number(lineNumber),
                            NeedsTemporaryStop);
        runCommand(cmd);
    } else {
        // Some versions of gdb like "GNU gdb (GDB) SUSE (6.8.91.20090930-2.4)"
        // know how to do pending breakpoints using CLI but not MI. So try
        // again with MI.
        DebuggerCommand cmd("break " + breakpointLocation2(bp->requestedParameters()),
                            NeedsTemporaryStop);
        cmd.callback = [this, bp](const DebuggerResponse &r) { handleBreakInsert2(r, bp); };
        runCommand(cmd);
    }
}

void GdbEngine::handleBreakInsert2(const DebuggerResponse &response, const Breakpoint &bp)
{
    if (response.resultClass == ResultDone) {
        notifyBreakpointInsertOk(bp);
    } else {
        // Note: gdb < 60800  doesn't "do" pending breakpoints.
        // Not much we can do about it except implementing the
        // logic on top of shared library events, and that's not
        // worth the effort.
    }
}

void GdbEngine::handleBreakDisable(const DebuggerResponse &response, const Breakpoint &bp)
{
    if (response.resultClass == ResultDone) {
        // This should only be the requested state.
        QTC_ASSERT(bp, return);
        bp->setEnabled(false);
        // GDB does *not* touch the subbreakpoints in that case
        // bp->forFirstLevelChildren([&](const SubBreakpoint &l) { l->params.enabled = false; });
        updateBreakpoint(bp); // Maybe there's more to do.
    }
}

void GdbEngine::handleBreakEnable(const DebuggerResponse &response, const Breakpoint &bp)
{
    if (response.resultClass == ResultDone) {
        QTC_ASSERT(bp, return);
        // This should only be the requested state.
        bp->setEnabled(true);
        // GDB does *not* touch the subbreakpoints in that case
        //bp->forFirstLevelChildren([&](const SubBreakpoint &l) { l->params.enabled = true; });
        updateBreakpoint(bp); // Maybe there's more to do.
    }
}

void GdbEngine::handleBreakThreadSpec(const DebuggerResponse &response, const Breakpoint &bp)
{
    QTC_CHECK(response.resultClass == ResultDone);
    QTC_ASSERT(bp, return);
    // Parsing is fragile. Assume we got what we asked for instead.
    bp->setThreadSpec(bp->requestedParameters().threadSpec);
    notifyBreakpointNeedsReinsertion(bp);
    insertBreakpoint(bp);
}

void GdbEngine::handleBreakLineNumber(const DebuggerResponse &response, const Breakpoint &bp)
{
    QTC_CHECK(response.resultClass == ResultDone);
    QTC_ASSERT(bp, return);
    notifyBreakpointNeedsReinsertion(bp);
    insertBreakpoint(bp);
}

void GdbEngine::handleBreakIgnore(const DebuggerResponse &response, const Breakpoint &bp)
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
    QTC_CHECK(response.resultClass == ResultDone);
    QTC_ASSERT(bp, return);
    //QString msg = _(response.consoleStreamOutput);
    //if (msg.contains(__("Will stop next time breakpoint")))
    //    response.ignoreCount = _("0");
    //else if (msg.contains(__("Will ignore next")))
    //    response.ignoreCount = data->ignoreCount;
    // FIXME: this assumes it is doing the right thing...
    bp->setIgnoreCount(bp->requestedParameters().ignoreCount);
    bp->setCommand(bp->requestedParameters().command);
    updateBreakpoint(bp); // Maybe there's more to do.
}

void GdbEngine::handleBreakCondition(const DebuggerResponse &, const Breakpoint &bp)
{
    QTC_ASSERT(bp, return);
    // Can happen at invalid condition strings.
    //QTC_CHECK(response.resultClass == ResultDone)
    // We just assume it was successful. Otherwise we had to parse
    // the output stream data.
    // The following happens on Mac:
    //   QByteArray msg = response.data.findChild("msg").data();
    //   if (msg.startsWith("Error parsing breakpoint condition. "
    //         " Will try again when we hit the breakpoint."))
    bp->setCondition(bp->requestedParameters().condition);
    updateBreakpoint(bp); // Maybe there's more to do.
}

void GdbEngine::updateTracepointCaptures(const Breakpoint &bp)
{
    static QRegularExpression capsRegExp(
        "(^|[^\\\\])(\\$(ADDRESS|CALLER|CALLSTACK|FILEPOS|FUNCTION|PID|PNAME|TICK|TID|TNAME)"
        "|{[^}]+})");
    QString message = bp->globalBreakpoint()->requestedParameters().message;
    if (message.isEmpty()) {
        bp->setProperty(tracepointCapturePropertyName, {});
        return;
    }
    QVariantList caps;
    QRegularExpressionMatch match = capsRegExp.match(message, 0);
    while (match.hasMatch()) {
        QString t = match.captured(2);
        if (t[0] == '$') {
            TracepointCaptureType type;
            if (t == "$ADDRESS")
                type = TracepointCaptureType::Address;
            else if (t == "$CALLER")
                type = TracepointCaptureType::Caller;
            else if (t == "$CALLSTACK")
                type = TracepointCaptureType::Callstack;
            else if (t == "$FILEPOS")
                type = TracepointCaptureType::FilePos;
            else if (t == "$FUNCTION")
                type = TracepointCaptureType::Function;
            else if (t == "$PID")
                type = TracepointCaptureType::Pid;
            else if (t == "$PNAME")
                type = TracepointCaptureType::ProcessName;
            else if (t == "$TICK")
                type = TracepointCaptureType::Tick;
            else if (t == "$TID")
                type = TracepointCaptureType::Tid;
            else if (t == "$TNAME")
                type = TracepointCaptureType::ThreadName;
            else
                QTC_ASSERT(false, continue);
            caps << QVariant::fromValue<TracepointCaptureData>(
                {type,
                 {},
                 static_cast<int>(match.capturedStart(2)),
                 static_cast<int>(match.capturedEnd(2))});
        } else {
            QString expression = t.mid(1, t.length() - 2);
            caps << QVariant::fromValue<TracepointCaptureData>(
                {TracepointCaptureType::Expression,
                 expression,
                 static_cast<int>(match.capturedStart(2)),
                 static_cast<int>(match.capturedEnd(2))});
        }
        match = capsRegExp.match(message, match.capturedEnd());
    }
    bp->setProperty(tracepointCapturePropertyName, caps);
}

void GdbEngine::handleTracepointInsert(const DebuggerResponse &response, const Breakpoint &bp)
{
    QTC_ASSERT(bp, return);
    if (bp->state() == BreakpointRemoveRequested) {
        if (response.resultClass == ResultDone) {
            // This delete was deferred. Act now.
            const GdbMi mainbkpt = response.data["tracepoint"][0];
            notifyBreakpointRemoveProceeding(bp);
            DebuggerCommand cmd("-break-delete " + mainbkpt["number"].data());
            cmd.flags = NeedsTemporaryStop;
            runCommand(cmd);
            notifyBreakpointRemoveOk(bp);
            return;
        }
    }
    if (response.resultClass == ResultDone) {
        for (const GdbMi &bkpt : response.data["tracepoint"])
            handleBkpt(bkpt, bp);
        if (bp->needsChange()) {
            bp->gotoState(BreakpointUpdateRequested, BreakpointInsertionProceeding);
            updateBreakpoint(bp);
        } else {
            notifyBreakpointInsertOk(bp);
        }
    }
}

void GdbEngine::handleTracepointHit(const GdbMi &data)
{
    const GdbMi &result = data["result"];
    const QString rid = result["number"].data();
    Breakpoint bp = breakHandler()->findBreakpointByResponseId(rid);
    QTC_ASSERT(bp, return);
    const GdbMi &warnings = data["warnings"];
    if (warnings.childCount() > 0) {
        for (const GdbMi &warning: warnings) {
            emit appendMessageRequested(warning.toString(), ErrorMessageFormat, true);
        }
    }
    QString message = bp->message();
    QVariant caps = bp->property(tracepointCapturePropertyName);
    if (caps.isValid()) {
        QList<QVariant> capsList = caps.toList();
        const GdbMi &miCaps = result["caps"];
        if (capsList.length() == miCaps.childCount()) {
            // reverse iterate to make start/end correct
            for (int i = capsList.length() - 1; i >= 0; --i) {
               TracepointCaptureData cap = capsList.at(i).value<TracepointCaptureData>();
               const GdbMi &miCap = miCaps.childAt(i);
               switch (cap.type) {
               case TracepointCaptureType::Callstack: {
                   QStringList frames;
                   for (const GdbMi &frame: miCap) {
                       frames.append(frame.data());
                   }
                   message.replace(cap.start, cap.end - cap.start, frames.join(" <- "));
                   break;
               }
               case TracepointCaptureType::Expression: {
                   QString key = miCap.data();
                   const GdbMi &expression = data["expressions"][key.toLatin1().data()];
                   if (expression.isValid()) {
                       QString s = expression.toString();
                       // remove '<key>='
                       s = s.right(s.length() - key.length() - 1);
                       message.replace(cap.start, cap.end - cap.start, s);
                   } else {
                       QTC_CHECK(false);
                   }
                   break;
               }
               default:
                   message.replace(cap.start, cap.end - cap.start, miCap.data());
               }
            }
        } else {
            QTC_CHECK(false);
        }
    }
    showMessage(message);
    emit appendMessageRequested(message, NormalMessageFormat, true);
}

void GdbEngine::handleTracepointModified(const GdbMi &data)
{
    QString ba = data.toString();
    // remove original-location
    const int pos1 = ba.indexOf("original-location=");
    const int pos2 = ba.indexOf(":", pos1 + 17);
    int pos3 = ba.indexOf('"', pos2 + 1);
    if (ba[pos3 + 1] == ',')
        ++pos3;
    ba.remove(pos1, pos3 - pos1 + 1);
    GdbMi res;
    res.fromString(ba);
    BreakHandler *handler = breakHandler();
    Breakpoint bp;
    for (const GdbMi &bkpt : res) {
        const QString nr = bkpt["number"].data();
        if (nr.contains('.')) {
            // A sub-breakpoint.
            QTC_ASSERT(bp, continue);
            SubBreakpoint loc = bp->findOrCreateSubBreakpoint(nr);
            loc->params.updateFromGdbOutput(bkpt, runParameters().projectSourceDirectory);
            loc->params.type = bp->type();
            if (bp->isTracepoint()) {
                loc->params.tracepoint = true;
                loc->params.message = bp->message();
            }
        } else {
            // A primary breakpoint.
            bp = handler->findBreakpointByResponseId(nr);
            if (bp)
                bp->updateFromGdbOutput(bkpt, runParameters().projectSourceDirectory);
        }
    }
    QTC_ASSERT(bp, return);
    bp->adjustMarker();
}

bool GdbEngine::acceptsBreakpoint(const BreakpointParameters &bp) const
{
    if (runParameters().startMode == AttachToCore)
        return false;
    if (bp.isCppBreakpoint())
        return true;
    return isNativeMixedEnabled();
}

void GdbEngine::insertBreakpoint(const Breakpoint &bp)
{
    // Set up fallback in case of pending breakpoints which aren't handled
    // by the MI interface.A
    QTC_ASSERT(bp, return);
    QTC_CHECK(bp->state() == BreakpointInsertionRequested);
    notifyBreakpointInsertProceeding(bp);

    const BreakpointParameters &requested = bp->requestedParameters();

    if (!requested.isCppBreakpoint()) {
        DebuggerCommand cmd("insertInterpreterBreakpoint", NeedsTemporaryStop);
        bp->addToCommand(&cmd);
        cmd.callback = [this, bp](const DebuggerResponse &r) { handleInsertInterpreterBreakpoint(r, bp); };
        runCommand(cmd);
        return;
    }

    const auto handleWatch = [this, bp](const DebuggerResponse &r) { handleWatchInsert(r, bp); };
    const auto handleCatch = [this, bp](const DebuggerResponse &r) { handleCatchInsert(r, bp); };

    BreakpointType type = requested.type;

    DebuggerCommand cmd;
    if (type == WatchpointAtAddress) {
        cmd.function = "watch " + addressSpec(requested.address);
        cmd.callback = handleWatch;
    } else if (type == WatchpointAtExpression) {
        cmd.function = "watch " + requested.expression;
        cmd.callback = handleWatch;
    } else if (type == BreakpointAtFork) {
        cmd.function = "catch fork";
        cmd.callback = handleCatch;
        cmd.flags = NeedsTemporaryStop;
        runCommand(cmd);
        // Another one...
        cmd.function = "catch vfork";
    } else if (type == BreakpointAtExec) {
        cmd.function = "catch exec";
        cmd.callback = handleCatch;
    } else if (type == BreakpointAtSysCall) {
        cmd.function = "catch syscall";
        cmd.callback = handleCatch;
    } else {
        int spec = requested.threadSpec;
        if (requested.isTracepoint()) {

            if (settings().usePseudoTracepoints()) {
                cmd.function = "createTracepoint";

                if (requested.oneShot)
                    cmd.arg("temporary", true);

                if (int ignoreCount = requested.ignoreCount)
                    cmd.arg("ignore_count", ignoreCount);

                QString condition = requested.condition;
                if (!condition.isEmpty())
                    cmd.arg("condition", condition.replace('"', "\\\""));

                if (spec >= 0)
                   cmd.arg("thread", spec);

                updateTracepointCaptures(bp);
                QVariant tpCaps = bp->property(tracepointCapturePropertyName);
                if (tpCaps.isValid()) {
                    QJsonArray caps;
                    for (const QVariant &tpCap : tpCaps.toList()) {
                        TracepointCaptureData data = tpCap.value<TracepointCaptureData>();
                        QJsonArray cap;
                        cap.append(static_cast<int>(data.type));
                        if (data.expression.isValid())
                            cap.append(data.expression.toString());
                        else
                            cap.append(QJsonValue::Null);
                        caps.append(cap);
                    }
                    cmd.arg("caps", caps);
                }

                // for dumping of expressions
                const bool alwaysVerbose = qtcEnvironmentVariableIsSet(
                    "QTC_DEBUGGER_PYTHON_VERBOSE");
                const DebuggerSettings &s = settings();
                cmd.arg("passexceptions", alwaysVerbose);
                cmd.arg("fancy", s.useDebuggingHelpers());
                cmd.arg("autoderef", s.autoDerefPointers());
                cmd.arg("dyntype", s.useDynamicType());
                cmd.arg("qobjectnames", s.showQObjectNames());
                cmd.arg("nativemixed", isNativeMixedActive());
                cmd.arg("stringcutoff", s.maximalStringLength());
                cmd.arg("displaystringlimit", s.displayStringLimit());

                cmd.arg("spec", breakpointLocation2(requested));
                cmd.callback = [this, bp](const DebuggerResponse &r) { handleTracepointInsert(r, bp); };

            } else {
                cmd.function = "-break-insert -a -f ";

                if (requested.oneShot)
                    cmd.function += "-t ";

                if (!requested.enabled)
                    cmd.function += "-d ";

                if (int ignoreCount = requested.ignoreCount)
                    cmd.function += "-i " + QString::number(ignoreCount) + ' ';

                QString condition = requested.condition;
                if (!condition.isEmpty())
                    cmd.function += " -c \"" + condition.replace('"', "\\\"") + "\" ";

                cmd.function += breakpointLocation(requested);
                cmd.callback = [this, bp](const DebuggerResponse &r) { handleBreakInsert1(r, bp); };

            }

        } else {
            cmd.function = "-break-insert ";
            if (spec >= 0)
                cmd.function += "-p " + QString::number(spec);
            cmd.function += " -f ";

           if (requested.oneShot)
               cmd.function += "-t ";

           if (!requested.enabled)
               cmd.function += "-d ";

           if (int ignoreCount = requested.ignoreCount)
               cmd.function += "-i " + QString::number(ignoreCount) + ' ';

           QString condition = requested.condition;
           if (!condition.isEmpty())
               cmd.function += " -c \"" + condition.replace('"', "\\\"") + "\" ";

           cmd.function += breakpointLocation(requested);
           cmd.callback = [this, bp](const DebuggerResponse &r) { handleBreakInsert1(r, bp); };

       }
    }
    cmd.flags = NeedsTemporaryStop;
    runCommand(cmd);
}

void GdbEngine::updateBreakpoint(const Breakpoint &bp)
{
    QTC_ASSERT(bp, return);
    const BreakpointParameters &requested = bp->requestedParameters();
    QTC_ASSERT(requested.type != UnknownBreakpointType, return);
    QTC_ASSERT(!bp->responseId().isEmpty(), return);
    const QString bpnr = bp->responseId();
    const BreakpointState state = bp->state();
    if (state == BreakpointUpdateRequested)
        notifyBreakpointChangeProceeding(bp);
    const BreakpointState state2 = bp->state();
    QTC_ASSERT(state2 == BreakpointUpdateProceeding, qDebug() << state2);

    DebuggerCommand cmd;
    // FIXME: See QTCREATORBUG-21611, QTCREATORBUG-21616
//    if (!bp->isPending() && requested.threadSpec != bp->threadSpec()) {
//        // The only way to change this seems to be to re-set the bp completely.
//        cmd.function = "-break-delete " + bpnr;
//        cmd.callback = [this, bp](const DebuggerResponse &r) { handleBreakThreadSpec(r, bp); };
//    } else if (!bp->isPending() && requested.lineNumber != bp->lineNumber()) {
//      // The only way to change this seems to be to re-set the bp completely.
//      cmd.function = "-break-delete " + bpnr;
//      cmd.callback = [this, bp](const DebuggerResponse &r) { handleBreakLineNumber(r, bp); };
//    } else if
    if (requested.command != bp->command()) {
        cmd.function = "-break-commands " + bpnr;
        for (QString command : requested.command.split('\n')) {
            if (!command.isEmpty()) {
                // escape backslashes and quotes
                command.replace('\\', "\\\\");
                command.replace('"', "\\\"");
                cmd.function +=  " \"" + command + '"';
            }
        }
        cmd.callback = [this, bp](const DebuggerResponse &r) { handleBreakIgnore(r, bp); };
    } else if (!requested.conditionsMatch(bp->condition())) {
        cmd.function = "condition " + bpnr + ' '  + requested.condition;
        cmd.callback = [this, bp](const DebuggerResponse &r) { handleBreakCondition(r, bp); };
    } else if (requested.ignoreCount != bp->ignoreCount()) {
        cmd.function = "ignore " + bpnr + ' ' + QString::number(requested.ignoreCount);
        cmd.callback = [this, bp](const DebuggerResponse &r) { handleBreakIgnore(r, bp); };
    } else if (!requested.enabled && bp->isEnabled()) {
        cmd.function = "-break-disable " + bpnr;
        cmd.callback = [this, bp](const DebuggerResponse &r) { handleBreakDisable(r, bp); };
    } else if (requested.enabled && !bp->isEnabled()) {
        cmd.function = "-break-enable " + bpnr;
        cmd.callback = [this, bp](const DebuggerResponse &r) { handleBreakEnable(r, bp); };
    } else {
        notifyBreakpointChangeOk(bp);
        return;
    }
    cmd.flags = NeedsTemporaryStop;
    runCommand(cmd);
}

void GdbEngine::enableSubBreakpoint(const SubBreakpoint &sbp, bool on)
{
    QTC_ASSERT(sbp, return);
    DebuggerCommand cmd((on ? "-break-enable " : "-break-disable ") + sbp->responseId);
    runCommand(cmd);
}

void GdbEngine::removeBreakpoint(const Breakpoint &bp)
{
    QTC_ASSERT(bp, return);
    QTC_CHECK(bp->state() == BreakpointRemoveRequested);

    const BreakpointParameters &requested = bp->requestedParameters();
    if (!requested.isCppBreakpoint()) {
        DebuggerCommand cmd("removeInterpreterBreakpoint");
        bp->addToCommand(&cmd);
        runCommand(cmd);
        notifyBreakpointRemoveOk(bp);
        return;
    }

    if (!bp->responseId().isEmpty()) {
        // We already have a fully inserted breakpoint.
        notifyBreakpointRemoveProceeding(bp);
        showMessage(
            QString("DELETING BP %1 IN %2").arg(bp->responseId()).arg(bp->fileName().toString()));
        DebuggerCommand cmd("-break-delete " + bp->responseId(), NeedsTemporaryStop);
        runCommand(cmd);

        // Pretend it succeeds without waiting for response. Feels better.
        // Otherwise, clicking in the gutter leaves the breakpoint visible
        // for quite some time, so the user assumes a mis-click and clicks
        // again, effectivly re-introducing the breakpoint.
        notifyBreakpointRemoveOk(bp);

    } else {
        // Breakpoint was scheduled to be inserted, but we haven't had
        // an answer so far. Postpone activity by doing nothing.
    }
}


//////////////////////////////////////////////////////////////////////
//
// Modules specific stuff
//
//////////////////////////////////////////////////////////////////////

static QString dotEscape(QString str)
{
    str.replace(' ', '.');
    str.replace('\\', '.');
    str.replace('/', '.');
    return str;
}

void GdbEngine::loadSymbols(const FilePath &modulePath)
{
    // FIXME: gdb does not understand quoted names here (tested with 6.8)
    runCommand({"sharedlibrary " + dotEscape(modulePath.path())});
    reloadModulesInternal();
    reloadStack();
    updateLocals();
}

void GdbEngine::loadAllSymbols()
{
    runCommand({"sharedlibrary .*"});
    reloadModulesInternal();
    reloadStack();
    updateLocals();
}

void GdbEngine::loadSymbolsForStack()
{
    bool needUpdate = false;
    const Modules &modules = modulesHandler()->modules();
    stackHandler()->forItemsAtLevel<2>([modules, &needUpdate, this](StackFrameItem *frameItem) {
        const StackFrame &frame = frameItem->frame;
        if (frame.function == "??") {
            //qDebug() << "LOAD FOR " << frame.address;
            for (const Module &module : modules) {
                if (module.startAddress <= frame.address
                        && frame.address < module.endAddress) {
                    runCommand({"sharedlibrary " + dotEscape(module.modulePath.path())});
                    needUpdate = true;
                }
            }
        }
    });
    if (needUpdate) {
        reloadStack();
        updateLocals();
    }
}

static void handleShowModuleSymbols(const DebuggerResponse &response,
                                    const FilePath &modulePath, const QString &fileName)
{
    if (response.resultClass == ResultDone) {
        Symbols symbols;
        QFile file(fileName);
        file.open(QIODevice::ReadOnly);
        // Object file /opt/dev/qt/lib/libQtNetworkMyns.so.4:
        // [ 0] A 0x16bd64 _DYNAMIC  moc_qudpsocket.cpp
        // [12] S 0xe94680 _ZN4myns5QFileC1Ev section .plt  myns::QFile::QFile()
        const QStringList lines = QString::fromLocal8Bit(file.readAll()).split('\n');
        for (const QString &line : lines) {
            if (line.isEmpty())
                continue;
            if (line.at(0) != '[')
                continue;
            int posCode = line.indexOf(']') + 2;
            int posAddress = line.indexOf("0x", posCode);
            if (posAddress == -1)
                continue;
            int posName = line.indexOf(" ", posAddress);
            int lenAddress = posName - posAddress;
            int posSection = line.indexOf(" section ");
            int lenName = 0;
            int lenSection = 0;
            int posDemangled = 0;
            if (posSection == -1) {
                lenName = line.size() - posName;
                posDemangled = posName;
            } else {
                lenName = posSection - posName;
                posSection += 10;
                posDemangled = line.indexOf(' ', posSection + 1);
                if (posDemangled == -1) {
                    lenSection = line.size() - posSection;
                } else {
                    lenSection = posDemangled - posSection;
                    posDemangled += 1;
                }
            }
            int lenDemangled = 0;
            if (posDemangled != -1)
                lenDemangled = line.size() - posDemangled;
            Symbol symbol;
            symbol.state = line.mid(posCode, 1);
            symbol.address = line.mid(posAddress, lenAddress);
            symbol.name = line.mid(posName, lenName);
            symbol.section = line.mid(posSection, lenSection);
            symbol.demangled = line.mid(posDemangled, lenDemangled);
            symbols.push_back(symbol);
        }
        file.close();
        file.remove();
        DebuggerEngine::showModuleSymbols(modulePath, symbols);
    } else {
        AsynchronousMessageBox::critical(Tr::tr("Cannot Read Symbols"),
            Tr::tr("Cannot read symbols for module \"%1\".").arg(fileName));
    }
}

void GdbEngine::requestModuleSymbols(const FilePath &modulePath)
{
    TemporaryFile tf("gdbsymbols");
    if (!tf.open())
        return;
    QString fileName = tf.fileName();
    tf.close();
    DebuggerCommand cmd("maint print msymbols \"" + fileName + "\" " + modulePath.path(), NeedsTemporaryStop);
    cmd.callback = [modulePath, fileName](const DebuggerResponse &r) {
        handleShowModuleSymbols(r, modulePath, fileName);
    };
    runCommand(cmd);
}

void GdbEngine::requestModuleSections(const FilePath &moduleName)
{
    // There seems to be no way to get the symbols from a single .so.
    DebuggerCommand cmd("maint info section ALLOBJ", NeedsTemporaryStop);
    cmd.callback = [this, moduleName](const DebuggerResponse &r) {
        handleShowModuleSections(r, moduleName);
    };
    runCommand(cmd);
}

void GdbEngine::handleShowModuleSections(const DebuggerResponse &response,
                                         const FilePath &moduleName)
{
    // ~"  Object file: /usr/lib/i386-linux-gnu/libffi.so.6\n"
    // ~"    0xb44a6114->0xb44a6138 at 0x00000114: .note.gnu.build-id ALLOC LOAD READONLY DATA HAS_CONTENTS\n"
    if (response.resultClass == ResultDone) {
        const QStringList lines = response.consoleStreamOutput.split('\n');
        const QString prefix = "  Object file: ";
        const QString needle = prefix + moduleName.path();
        Sections sections;
        bool active = false;
        for (const QString &line : std::as_const(lines)) {
            if (line.startsWith(prefix)) {
                if (active)
                    break;
                if (line == needle)
                    active = true;
            } else {
                if (active) {
                    QStringList items = line.split(' ', Qt::SkipEmptyParts);
                    QString fromTo = items.value(0, QString());
                    const int pos = fromTo.indexOf('-');
                    QTC_ASSERT(pos >= 0, continue);
                    Section section;
                    section.from = fromTo.left(pos);
                    section.to = fromTo.mid(pos + 2);
                    section.address = items.value(2, QString());
                    section.name = items.value(3, QString());
                    section.flags = items.value(4, QString());
                    sections.append(section);
                }
            }
        }
        if (!sections.isEmpty())
            DebuggerEngine::showModuleSections(moduleName, sections);
    }
}

void GdbEngine::reloadModules()
{
    if (state() == InferiorRunOk || state() == InferiorStopOk)
        reloadModulesInternal();
}

void GdbEngine::reloadModulesInternal()
{
    runCommand({"info shared", NeedsTemporaryStop, CB(handleModulesList)});
}

void GdbEngine::handleModulesList(const DebuggerResponse &response)
{
    if (response.resultClass == ResultDone) {
        ModulesHandler *handler = modulesHandler();
        Module module;
        // That's console-based output, likely Linux or Windows,
        // but we can avoid the target dependency here.
        QString data = response.consoleStreamOutput;
        QTextStream ts(&data, QIODevice::ReadOnly);
        bool found = false;
        const FilePath inferior = runParameters().inferior.command.executable();
        while (!ts.atEnd()) {
            QString line = ts.readLine();
            QString symbolsRead;
            QTextStream ts(&line, QIODevice::ReadOnly);
            if (line.startsWith("0x")) {
                ts >> module.startAddress >> module.endAddress >> symbolsRead;
                module.modulePath = inferior.withNewPath(ts.readLine().trimmed());
                module.moduleName = module.modulePath.baseName();
                module.symbolsRead =
                    (symbolsRead == "Yes" ? Module::ReadOk : Module::ReadFailed);
                handler->updateModule(module);
                found = true;
            } else if (line.trimmed().startsWith("No")) {
                // gdb 6.4 symbianelf
                ts >> symbolsRead;
                QTC_ASSERT(symbolsRead == "No", continue);
                module.startAddress = 0;
                module.endAddress = 0;
                module.modulePath = inferior.withNewPath(ts.readLine().trimmed());
                module.moduleName = module.modulePath.baseName();
                handler->updateModule(module);
                found = true;
            }
        }
        if (!found) {
            // Mac has^done,shlib-info={num="1",name="dyld",kind="-",
            // dyld-addr="0x8fe00000",reason="dyld",requested-state="Y",
            // state="Y",path="/usr/lib/dyld",description="/usr/lib/dyld",
            // loaded_addr="0x8fe00000",slide="0x0",prefix="__dyld_"},
            // shlib-info={...}...
            for (const GdbMi &item : response.data) {
                module.modulePath = inferior.withNewPath(item["path"].data());
                module.moduleName = module.modulePath.baseName();
                module.symbolsRead = (item["state"].data() == "Y")
                        ? Module::ReadOk : Module::ReadFailed;
                module.startAddress =
                    item["loaded_addr"].data().toULongLong(nullptr, 0);
                module.endAddress = 0; // FIXME: End address not easily available.
                handler->updateModule(module);
            }
        }
    }
}

void GdbEngine::examineModules()
{
    ModulesHandler *handler = modulesHandler();
    for (const Module &module : handler->modules()) {
        if (module.elfData.symbolsType == UnknownSymbols)
            handler->updateModule(module);
    }
}

//////////////////////////////////////////////////////////////////////
//
// Source files specific stuff
//
//////////////////////////////////////////////////////////////////////

void GdbEngine::reloadSourceFiles()
{
    if ((state() == InferiorRunOk || state() == InferiorStopOk) && !m_sourcesListUpdating) {
        m_sourcesListUpdating = true;
        DebuggerCommand cmd("-file-list-exec-source-files", NeedsTemporaryStop);
        cmd.callback = [this](const DebuggerResponse &response) {
            m_sourcesListUpdating = false;
            if (response.resultClass == ResultDone) {
                QMap<QString, FilePath> oldShortToFull = m_shortToFullName;
                m_shortToFullName.clear();
                m_fullToShortName.clear();
                // "^done,files=[{file="../../../../bin/dumper/dumper.cpp",
                // fullname="/data5/dev/ide/main/bin/dumper/dumper.cpp"},
                for (const GdbMi &item : response.data["files"]) {
                    GdbMi fileName = item["file"];
                    if (fileName.data().endsWith("<built-in>"))
                        continue;
                    GdbMi fullName = item["fullname"];
                    QString file = fileName.data();
                    FilePath full;
                    if (fullName.isValid()) {
                        full = cleanupFullName(fullName.data());
                        m_fullToShortName[full] = file;
                    }
                    m_shortToFullName[file] = full;
                }
                if (m_shortToFullName != oldShortToFull)
                    sourceFilesHandler()->setSourceFiles(m_shortToFullName);
            }
        };
        runCommand(cmd);
    }
}


//////////////////////////////////////////////////////////////////////
//
// Stack specific stuff
//
//////////////////////////////////////////////////////////////////////

void GdbEngine::selectThread(const Thread &thread)
{
    showStatusMessage(Tr::tr("Retrieving data for stack view thread %1...")
        .arg(thread->id()), 10000);
    DebuggerCommand cmd("-thread-select " + thread->id(), Discardable);
    cmd.callback = [this](const DebuggerResponse &) {
        QTC_CHECK(state() == InferiorUnrunnable || state() == InferiorStopOk);
        showStatusMessage(Tr::tr("Retrieving data for stack view..."), 3000);
        reloadStack(); // Will reload registers.
        updateLocals();
    };
    runCommand(cmd);
}

void GdbEngine::reloadFullStack()
{
    PENDING_DEBUG("RELOAD FULL STACK");
    resetLocation();
    DebuggerCommand cmd = stackCommand(-1);
    cmd.callback = [this](const DebuggerResponse &r) { handleStackListFrames(r, true); };
    cmd.flags = Discardable;
    runCommand(cmd);
}

void GdbEngine::loadAdditionalQmlStack()
{
    DebuggerCommand cmd = stackCommand(-1);
    cmd.arg("extraqml", "1");
    cmd.callback = [this](const DebuggerResponse &r) { handleStackListFrames(r, true); };
    cmd.flags = Discardable;
    runCommand(cmd);
}

DebuggerCommand GdbEngine::stackCommand(int depth)
{
    DebuggerCommand cmd("fetchStack");
    cmd.arg("limit", depth);
    cmd.arg("nativemixed", isNativeMixedActive());
    return cmd;
}

void GdbEngine::reloadStack()
{
    PENDING_DEBUG("RELOAD STACK");
    DebuggerCommand cmd = stackCommand(settings().maximalStackDepth());
    cmd.callback = [this](const DebuggerResponse &r) { handleStackListFrames(r, false); };
    cmd.flags = Discardable;
    runCommand(cmd);
}

void GdbEngine::handleStackListFrames(const DebuggerResponse &response, bool isFull)
{
    if (response.resultClass != ResultDone) {
        // That always happens on symbian gdb with
        // ^error,data={msg="Previous frame identical to this frame (corrupt stack?)"
        // logStreamOutput: "Previous frame identical to this frame (corrupt stack?)\n"
        //qDebug() << "LISTING STACK FAILED: " << response.toString();
        reloadRegisters();
        reloadPeripheralRegisters();
        return;
    }

    GdbMi stack = response.data["stack"]; // C++
    //if (!frames.isValid() || frames.childCount() == 0) // Mixed.
    GdbMi frames = stack["frames"];
    if (!frames.isValid())
        isFull = true;

    stackHandler()->setFramesAndCurrentIndex(frames, isFull);
    activateFrame(stackHandler()->currentIndex());
}

void GdbEngine::activateFrame(int frameIndex)
{
    if (state() != InferiorStopOk && state() != InferiorUnrunnable)
        return;

    StackHandler *handler = stackHandler();

    if (handler->isSpecialFrame(frameIndex)) {
        reloadFullStack();
        return;
    }

    QTC_ASSERT(frameIndex < handler->stackSize(), return);
    handler->setCurrentIndex(frameIndex);
    gotoCurrentLocation();

    if (handler->frameAt(frameIndex).language != QmlLanguage) {
        // Assuming the command always succeeds this saves a roundtrip.
        // Otherwise the lines below would need to get triggered
        // after a response to this -stack-select-frame here.
        //if (!m_currentThread.isEmpty())
        //    cmd += " --thread " + m_currentThread;
        runCommand({"-stack-select-frame " + QString::number(frameIndex), Discardable});
    }

    updateLocals();
    reloadRegisters();
    reloadPeripheralRegisters();
}

void GdbEngine::handleThreadInfo(const DebuggerResponse &response)
{
    if (response.resultClass == ResultDone) {
        ThreadsHandler *handler = threadsHandler();
        handler->setThreads(response.data);
        updateState(); // Adjust Threads combobox.
        if (settings().showThreadNames()) {
            runCommand({QString("threadnames %1").arg(settings().maximalStackDepth()),
                Discardable, CB(handleThreadNames)});
        }
        reloadStack(); // Will trigger register reload.
    } else {
        // Fall back for older versions: Try to get at least a list
        // of running threads.
        runCommand({"-thread-list-ids", Discardable, CB(handleThreadListIds)});
    }
}

void GdbEngine::handleThreadListIds(const DebuggerResponse &response)
{
    // "72^done,{thread-ids={thread-id="2",thread-id="1"},number-of-threads="2"}
    // In gdb 7.1+ additionally: current-thread-id="1"
    ThreadsHandler *handler = threadsHandler();
    for (const GdbMi &item : response.data["thread-ids"]) {
        ThreadData thread;
        thread.id = item.data();
        handler->updateThread(thread);
    }
    reloadStack(); // Will trigger register reload.
}

void GdbEngine::handleThreadNames(const DebuggerResponse &response)
{
    if (response.resultClass == ResultDone) {
        ThreadsHandler *handler = threadsHandler();
        GdbMi names;
        names.fromString(response.consoleStreamOutput);
        for (const GdbMi &name : names) {
            ThreadData thread;
            thread.id = name["id"].data();
            // Core is unavailable in core dump. Allow the user to provide it.
            thread.core = name["core"].data();
            thread.name = decodeData(name["value"].data(), name["valueencoded"].data());
            handler->updateThread(thread);
        }
        updateState();
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
    Utils::TemporaryFile tf("gdbsnapshot");
    if (tf.open()) {
        fileName = tf.fileName();
        tf.close();
        // This must not be quoted, it doesn't work otherwise.
        DebuggerCommand cmd("gcore " + fileName, NeedsTemporaryStop | ConsoleCommand);
        cmd.callback = [this, fileName](const DebuggerResponse &r) { handleMakeSnapshot(r, fileName); };
        runCommand(cmd);
    } else {
        AsynchronousMessageBox::critical(Tr::tr("Snapshot Creation Error"),
            Tr::tr("Cannot create snapshot file."));
    }
}

void GdbEngine::handleMakeSnapshot(const DebuggerResponse &response, const QString &coreFile)
{
    if (response.resultClass == ResultDone) {
        emit attachToCoreRequested(coreFile);
    } else {
        QString msg = response.data["msg"].data();
        AsynchronousMessageBox::critical(Tr::tr("Snapshot Creation Error"),
            Tr::tr("Cannot create snapshot:") + '\n' + msg);
    }
}


//////////////////////////////////////////////////////////////////////
//
// Register specific stuff
//
//////////////////////////////////////////////////////////////////////

void GdbEngine::reloadRegisters()
{
    if (!isRegistersWindowVisible())
        return;

    if (state() != InferiorStopOk && state() != InferiorUnrunnable)
        return;

    if (true) {
        if (!m_registerNamesListed) {
            // The MI version does not give register size.
            // runCommand("-data-list-register-names", CB(handleRegisterListNames));
            m_registerNamesListed = true;
            runCommand({"maintenance print register-groups",
                        CB(handleRegisterListing)});
        }
        // Can cause i386-linux-nat.c:571: internal-error: Got request
        // for bad register number 41.\nA problem internal to GDB has been detected.
        runCommand({"-data-list-register-values r", Discardable,
                    CB(handleRegisterListValues)});
    } else {
        runCommand({"maintenance print cooked-registers",
                    CB(handleMaintPrintRegisters)});
    }
}

void GdbEngine::reloadPeripheralRegisters()
{
    if (!isPeripheralRegistersWindowVisible())
        return;

    const QList<quint64> addresses = peripheralRegisterHandler()->activeRegisters();
    if (addresses.isEmpty())
        return; // Nothing to update.

    for (const quint64 address : addresses) {
        const QString fun = QStringLiteral("x/1u 0x%1")
                .arg(QString::number(address, 16));
        runCommand({fun, CB(handlePeripheralRegisterListValues)});
    }
}

static QString readWord(const QString &ba, int *pos)
{
    const int n = ba.size();
    while (*pos < n && ba.at(*pos) == ' ')
        ++*pos;
    const int start = *pos;
    while (*pos < n && ba.at(*pos) != ' ' && ba.at(*pos) != '\n')
        ++*pos;
    return ba.mid(start, *pos - start);
}

void GdbEngine::handleMaintPrintRegisters(const DebuggerResponse &response)
{
    if (response.resultClass != ResultDone)
        return;

    const QString &ba = response.consoleStreamOutput;
    RegisterHandler *handler = registerHandler();
    //0         1         2         3         4         5         6
    //0123456789012345678901234567890123456789012345678901234567890
    // Name         Nr  Rel Offset    Size  Type            Raw value
    // rax           0    0      0       8 int64_t         0x0000000000000000
    // rip          16   16    128       8 *1              0x0000000000400dc9
    // eflags       17   17    136       4 i386_eflags     0x00000246
    // cs           18   18    140       4 int32_t         0x00000033
    // xmm15        55   55    516      16 vec128          0x00000000000000000000000000000000
    // mxcsr        56   56    532       4 i386_mxcsr      0x00001fa0
    // ''
    // st6          30   30    224      10 _i387_ext       0x00000000000000000000
    // st7          31   31    234      10 _i387_ext       0x00000000000000000000
    // fctrl        32   32    244       4 int             0x0000037f

    const int n = ba.size();
    int pos = 0;
    while (true) {
        // Skip first line, and until '\n' after each line finished.
        while (pos < n && ba.at(pos) != '\n')
            ++pos;
        if (pos >= n)
            break;
        ++pos; // skip \n
        Register reg;
        reg.name = readWord(ba, &pos);
        if (reg.name == "''" || reg.name == "*1:" || reg.name.isEmpty())
            continue;
        readWord(ba, &pos); // Nr
        readWord(ba, &pos); // Rel
        readWord(ba, &pos); // Offset
        reg.size = readWord(ba, &pos).toInt();
        reg.reportedType = readWord(ba, &pos);
        reg.value.fromString(readWord(ba, &pos), HexadecimalFormat);
        handler->updateRegister(reg);
    }
    handler->commitUpdates();
}

void GdbEngine::setRegisterValue(const QString &name, const QString &value)
{
    QString fullName = name;
    if (name.startsWith("xmm"))
        fullName += ".uint128";
    runCommand({"set $" + fullName  + "=" + value});
    reloadRegisters();
}

void GdbEngine::setPeripheralRegisterValue(quint64 address, quint64 value)
{
    const QString fun = QStringLiteral("set {int}0x%1=%2")
            .arg(QString::number(address, 16))
            .arg(value);
    runCommand({fun});
    reloadPeripheralRegisters();
}

void GdbEngine::handleRegisterListNames(const DebuggerResponse &response)
{
    if (response.resultClass != ResultDone) {
        m_registerNamesListed = false;
        return;
    }

    m_registers.clear();
    int gdbRegisterNumber = 0;
    for (const GdbMi &item : response.data["register-names"]) {
        if (!item.data().isEmpty()) {
            Register reg;
            reg.name = item.data();
            m_registers[gdbRegisterNumber] = reg;
        }
        ++gdbRegisterNumber;
    }
}

void GdbEngine::handleRegisterListing(const DebuggerResponse &response)
{
    if (response.resultClass != ResultDone) {
        m_registerNamesListed = false;
        return;
    }

    // &"maintenance print raw-registers\n"
    // >~" Name         Nr  Rel Offset    Size  Type            Groups\n"
    // >~" rax           0    0      0       8 int64_t         general,all,save,restore\n"
    // >~" rip          16   16    128       8 *1              general,all,save,restore\n"
    // >~" fop          39   39    272       4 int             float,all,save,restore\n"
    // >~" xmm0         40   40    276      16 vec128          sse,all,save,restore,vector\n"
    // >~" ''          145  145    536       0 int0_t          general\n"
    m_registers.clear();
    QStringList lines = response.consoleStreamOutput.split('\n');
    for (int i = 1; i < lines.size(); ++i) {
        const QStringList parts = lines.at(i).split(' ', Qt::SkipEmptyParts);
        if (parts.size() < 7)
            continue;
        int gdbRegisterNumber = parts.at(1).toInt();
        Register reg;
        reg.name = parts.at(0);
        reg.size = parts.at(4).toInt();
        reg.reportedType = parts.at(5);
        reg.groups = Utils::toSet(parts.at(6).split(','));
        m_registers[gdbRegisterNumber] = reg;
    }
}

void GdbEngine::handleRegisterListValues(const DebuggerResponse &response)
{
    if (response.resultClass != ResultDone)
        return;

    RegisterHandler *handler = registerHandler();
    // 24^done,register-values=[{number="0",value="0xf423f"},...]
    for (const GdbMi &item : response.data["register-values"]) {
        const int number = item["number"].toInt();
        auto reg = m_registers.find(number);
        if (reg == m_registers.end())
            continue;
        QString data = item["value"].data();
        if (data.startsWith("0x")) {
            reg->value.fromString(data, HexadecimalFormat);
        } else if (data == "<error reading variable>") {
            // Nothing. See QTCREATORBUG-14029.
        } else {
            // This is what GDB considers machine readable output:
            // value="{v4_float = {0x00000000, 0x00000000, 0x00000000, 0x00000000},
            // v2_double = {0x0000000000000000, 0x0000000000000000},
            // v16_int8 = {0x00 <repeats 16 times>},
            // v8_int16 = {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
            // v4_int32 = {0x00000000, 0x00000000, 0x00000000, 0x00000000},
            // v2_int64 = {0x0000000000000000, 0x0000000000000000},
            // uint128 = <error reading variable>}"}
            // Try to make sense of it using the int32 chunks.
            // Android gdb 7.10 has u32 = {0x00000000, 0x40340000}.
            // Use that if available.
            QString result;
            int pos1 = data.indexOf("_int32");
            if (pos1 == -1)
                pos1 = data.indexOf("u32");
            const int pos2 = data.indexOf('{', pos1) + 1;
            const int pos3 = data.indexOf('}', pos2);
            QString inner = data.mid(pos2, pos3 - pos2);
            QStringList list = inner.split(',');
            for (int i = list.size(); --i >= 0; ) {
                QString chunk = list.at(i);
                if (chunk.startsWith(' '))
                    chunk.remove(0, 1);
                if (chunk.startsWith('<') || chunk.startsWith('{')) // <unavailable>, {v4_float=...
                    continue;
                if (chunk.startsWith("0x"))
                    chunk.remove(0, 2);
                QTC_ASSERT(chunk.size() == 8, continue);
                result.append(chunk);
            }
            reg->value.fromString(result, HexadecimalFormat);
        }
        handler->updateRegister(*reg);
    }
    handler->commitUpdates();
}

void GdbEngine::handlePeripheralRegisterListValues(
        const DebuggerResponse &response)
{
    if (response.resultClass != ResultDone)
        return;

    const QString output = response.consoleStreamOutput;
    // Regexp to match for '0x50060800:\t0\n'.
    const QRegularExpression re("^(0x[0-9A-Fa-f]+):\\t(\\d+)\\n$");
    const QRegularExpressionMatch m = re.match(output);
    if (!m.hasMatch())
        return;
    enum { AddressMatch = 1, ValueMatch = 2 };
    bool aok = false;
    bool vok = false;
    const quint64 address = m.captured(AddressMatch).toULongLong(&aok, 16);
    const quint64 value = m.captured(ValueMatch).toULongLong(&vok, 10);
    if (!aok || !vok)
        return;

    peripheralRegisterHandler()->updateRegister(address, value);
}

//////////////////////////////////////////////////////////////////////
//
// Watch specific stuff
//
//////////////////////////////////////////////////////////////////////

void GdbEngine::reloadLocals()
{
    // if the engine is not running - do nothing
    if (state() == DebuggerState::DebuggerFinished || state() == DebuggerState::DebuggerNotReady)
        return;

    setTokenBarrier();
    updateLocals();
}

void GdbEngine::handleVarAssign(const DebuggerResponse &)
{
    // Everything might have changed, force re-evaluation.
    setTokenBarrier();
    updateLocals();
}

void GdbEngine::assignValueInDebugger(WatchItem *item,
    const QString &expression, const QVariant &value)
{
    DebuggerCommand cmd("assignValue");
    cmd.arg("type", toHex(item->type));
    cmd.arg("expr", toHex(expression));
    cmd.arg("value", toHex(value.toString()));
    cmd.arg("simpleType", isIntOrFloatType(item->type));
    cmd.callback = CB(handleVarAssign);
    runCommand(cmd);
}

class MemoryAgentCookie
{
public:
    MemoryAgentCookie() = default;

    QByteArray *accumulator = nullptr; // Shared between split request. Last one cleans up.
    uint *pendingRequests = nullptr; // Shared between split request. Last one cleans up.

    QPointer<MemoryAgent> agent;
    quint64 base = 0; // base address.
    uint offset = 0; // offset to base, and in accumulator
    uint length = 0; //
};


void GdbEngine::changeMemory(MemoryAgent *agent, quint64 addr, const QByteArray &data)
{
    Q_UNUSED(agent)
    DebuggerCommand cmd("-data-write-memory 0x" + QString::number(addr, 16) + " d 1", NeedsTemporaryStop);
    for (unsigned char c : data)
        cmd.function += ' ' + QString::number(uint(c));
    cmd.callback = CB(handleVarAssign);
    runCommand(cmd);
}

void GdbEngine::fetchMemory(MemoryAgent *agent, quint64 addr, quint64 length)
{
    MemoryAgentCookie ac;
    ac.accumulator = new QByteArray(length, char());
    ac.pendingRequests = new uint(1);
    ac.agent = agent;
    ac.base = addr;
    ac.length = length;
    fetchMemoryHelper(ac);
}

void GdbEngine::fetchMemoryHelper(const MemoryAgentCookie &ac)
{
    DebuggerCommand cmd("-data-read-memory 0x"
                        + QString::number(ac.base + ac.offset, 16) + " x 1 1 "
                        + QString::number(ac.length),
                        NeedsTemporaryStop);
    cmd.callback = [this, ac](const DebuggerResponse &r) { handleFetchMemory(r, ac); };
    runCommand(cmd);
}

void GdbEngine::handleFetchMemory(const DebuggerResponse &response, MemoryAgentCookie ac)
{
    // ^done,addr="0x08910c88",nr-bytes="16",total-bytes="16",
    // next-row="0x08910c98",prev-row="0x08910c78",next-page="0x08910c98",
    // prev-page="0x08910c78",memory=[{addr="0x08910c88",
    // data=["1","0","0","0","5","0","0","0","0","0","0","0","0","0","0","0"]}]
    --*ac.pendingRequests;
    showMessage(QString("PENDING: %1").arg(*ac.pendingRequests));
    QTC_ASSERT(ac.agent, return);
    if (response.resultClass == ResultDone) {
        GdbMi memory = response.data["memory"];
        QTC_ASSERT(memory.childCount() <= 1, return);
        if (memory.childCount() == 0)
            return;
        GdbMi memory0 = memory.childAt(0); // we asked for only one 'row'
        GdbMi data = memory0["data"];
        int i = 0;
        for (const GdbMi &child : data) {
            bool ok = true;
            unsigned char c = '?';
            c = child.data().toUInt(&ok, 0);
            QTC_ASSERT(ok, return);
            (*ac.accumulator)[ac.offset + i++] = c;
        }
    } else {
        // We have an error
        if (ac.length > 1) {
            // ... and size > 1, split the load and re-try.
            *ac.pendingRequests += 2;
            uint hunk = ac.length / 2;
            MemoryAgentCookie ac1 = ac;
            ac1.length = hunk;
            ac1.offset = ac.offset;
            MemoryAgentCookie ac2 = ac;
            ac2.length = ac.length - hunk;
            ac2.offset = ac.offset + hunk;
            fetchMemoryHelper(ac1);
            fetchMemoryHelper(ac2);
        }
    }

    if (*ac.pendingRequests <= 0) {
        ac.agent->addData(ac.base, *ac.accumulator);
        delete ac.pendingRequests;
        delete ac.accumulator;
    }
}

class DisassemblerAgentCookie
{
public:
    DisassemblerAgentCookie() : agent(nullptr) {}
    DisassemblerAgentCookie(DisassemblerAgent *agent_) : agent(agent_) {}

public:
    QPointer<DisassemblerAgent> agent;
};

void GdbEngine::fetchDisassembler(DisassemblerAgent *agent)
{
    if (settings().intelFlavor())
        runCommand({"set disassembly-flavor intel"});
    else
        runCommand({"set disassembly-flavor att"});

    fetchDisassemblerByCliPointMixed(agent);
}

static inline QString disassemblerCommand(const Location &location, bool mixed, QChar mixedFlag)
{
    QString command = "disassemble /r";
    if (mixed)
        command += mixedFlag;
    command += ' ';
    if (const quint64 address = location.address()) {
        command += "0x";
        command += QString::number(address, 16);
    } else if (!location.functionName().isEmpty()) {
        command += location.functionName();
    } else {
        QTC_ASSERT(false, return QString(); );
    }
    return command;
}

void GdbEngine::fetchDisassemblerByCliPointMixed(const DisassemblerAgentCookie &ac)
{
    QTC_ASSERT(ac.agent, return);
    DebuggerCommand cmd(disassemblerCommand(ac.agent->location(), true, mixedDisasmFlag()),
                        Discardable | ConsoleCommand);
    cmd.callback = [this, ac](const DebuggerResponse &response) {
        if (response.resultClass == ResultDone)
            if (handleCliDisassemblerResult(response.consoleStreamOutput, ac.agent))
                return;
        // 'point, plain' can take far too long.
        // Skip this feature and immediately fall back to the 'range' version:
        fetchDisassemblerByCliRangeMixed(ac);
    };
    runCommand(cmd);
}

void GdbEngine::fetchDisassemblerByCliRangeMixed(const DisassemblerAgentCookie &ac)
{
    QTC_ASSERT(ac.agent, return);
    const quint64 address = ac.agent->address();
    QString start = QString::number(address - 20, 16);
    QString end = QString::number(address + 100, 16);
    DebuggerCommand cmd("disassemble /r" + mixedDisasmFlag() + " 0x" + start + ",0x" + end,
                        Discardable | ConsoleCommand);
    cmd.callback = [this, ac](const DebuggerResponse &response) {
        if (response.resultClass == ResultDone)
            if (handleCliDisassemblerResult(response.consoleStreamOutput, ac.agent))
                return;
        fetchDisassemblerByCliRangePlain(ac);
    };
    runCommand(cmd);
}

void GdbEngine::fetchDisassemblerByCliRangePlain(const DisassemblerAgentCookie &ac0)
{
    DisassemblerAgentCookie ac = ac0;
    QTC_ASSERT(ac.agent, return);
    const quint64 address = ac.agent->address();
    QString start = QString::number(address - 20, 16);
    QString end = QString::number(address + 100, 16);
    DebuggerCommand cmd("disassemble /r 0x" + start + ",0x" + end, Discardable);
    cmd.callback = [this, ac](const DebuggerResponse &response) {
        if (response.resultClass == ResultDone)
            if (handleCliDisassemblerResult(response.consoleStreamOutput, ac.agent))
                return;
        // Finally, give up.
        //76^error,msg="No function contains program counter for selected..."
        //76^error,msg="No function contains specified address."
        //>568^error,msg="Line number 0 out of range;
        QString msg = response.data["msg"].data();
        showStatusMessage(Tr::tr("Disassembler failed: %1").arg(msg), 5000);
    };
    runCommand(cmd);
}

struct LineData
{
    LineData() = default;
    LineData(int i, int f) : index(i), function(f) {}
    int index;
    int function;
};

bool GdbEngine::handleCliDisassemblerResult(const QString &output, DisassemblerAgent *agent)
{
    QTC_ASSERT(agent, return true);
    // First line is something like
    // "Dump of assembler code from 0xb7ff598f to 0xb7ff5a07:"
    DisassemblerLines dlines;
    const QStringList lineList = output.split('\n');
    for (const QString &line : lineList)
        dlines.appendUnparsed(line);

    QVector<DisassemblerLine> lines = dlines.data();

    using LineMap = QMap<quint64, LineData>;
    LineMap lineMap;
    int currentFunction = -1;
    for (int i = 0, n = lines.size(); i != n; ++i) {
        const DisassemblerLine &line = lines.at(i);
        if (line.address)
            lineMap.insert(line.address, LineData(i, currentFunction));
        else
            currentFunction = i;
    }

    currentFunction = -1;
    DisassemblerLines result;
    result.setBytesLength(dlines.bytesLength());
    for (LineMap::const_iterator it = lineMap.constBegin(), et = lineMap.constEnd(); it != et; ++it) {
        LineData d = *it;
        if (d.function != currentFunction) {
            if (d.function != -1) {
                DisassemblerLine &line = lines[d.function];
                ++line.hunk;
                result.appendLine(line);
                currentFunction = d.function;
            }
        }
        result.appendLine(lines.at(d.index));
    }

    if (result.coversAddress(agent->address())) {
        agent->setContents(result);
        return true;
    }

    return false;
}

static SourcePathMap mergeStartParametersSourcePathMap(const DebuggerRunParameters &sp,
                                                       const SourcePathMap &in)
{
    // Do not overwrite user settings.
    SourcePathMap rc = sp.sourcePathMap;
    for (auto it = in.constBegin(), end = in.constEnd(); it != end; ++it) {
        // Entries that start with parenthesis are handled in CppDebuggerEngine::validateRunParameters
        if (!it.key().startsWith('('))
            rc.insert(it.key(), sp.macroExpander->expand(it.value()));
    }
    return rc;
}

//
// Starting up & shutting down
//

void GdbEngine::setupEngine()
{
    CHECK_STATE(EngineSetupRequested);
    showMessage("TRYING TO START ADAPTER");

    if (isRemoteEngine())
        m_gdbProc.setUseCtrlCStub(runParameters().useCtrlCStub); // This is only set for QNX

    const DebuggerRunParameters &rp = runParameters();
    CommandLine gdbCommand = rp.debugger.command;

    if (usesOutputCollector()) {
        if (!m_outputCollector.listen()) {
            handleAdapterStartFailed(Tr::tr("Cannot set up communication with child process: %1")
                                     .arg(m_outputCollector.errorString()));
            return;
        }
        gdbCommand.addArg("--tty=" + m_outputCollector.serverName());
    }

    const QStringList testList = qtcEnvironmentVariable("QTC_DEBUGGER_TESTS").split(',');
    for (const QString &test : testList)
        m_testCases.insert(test.toInt());
    for (int test : std::as_const(m_testCases))
        showMessage("ENABLING TEST CASE: " + QString::number(test));

    m_expectTerminalTrap = terminal();

    if (rp.debugger.command.isEmpty()) {
        handleGdbStartFailed();
        handleAdapterStartFailed(
            msgNoGdbBinaryForToolChain(rp.toolChainAbi),
            Constants::DEBUGGER_COMMON_SETTINGS_ID);
        return;
    }

    gdbCommand.addArgs({"-i", "mi"});
    if (!settings().loadGdbInit())
        gdbCommand.addArg("-n");

    // This is filled in DebuggerKitAspect::runnable
    Environment gdbEnv = rp.debugger.environment;
    gdbEnv.setupEnglishOutput();
    if (rp.runAsRoot)
        RunControl::provideAskPassEntry(gdbEnv);
    m_gdbProc.setRunAsRoot(rp.runAsRoot);

    showMessage("STARTING " + gdbCommand.toUserOutput());

    m_gdbProc.setCommand(gdbCommand);
    if (rp.debugger.workingDirectory.isDir())
        m_gdbProc.setWorkingDirectory(rp.debugger.workingDirectory);
    m_gdbProc.setEnvironment(gdbEnv);
    m_gdbProc.start();
}

void GdbEngine::handleGdbStarted()
{
    showMessage("GDB STARTED, INITIALIZING IT");
    runCommand({"show version", CB(handleShowVersion)});
    runCommand({"show debug-file-directory", CB(handleDebugInfoLocation)});

    //runCommand("-enable-timings");
    //rurun print static-members off"); // Seemingly doesn't work.
    //runCommand("set debug infrun 1");
    //runCommand("define hook-stop\n-thread-list-ids\n-stack-list-frames\nend");
    //runCommand("define hook-stop\nprint 4\nend");
    //runCommand("define hookpost-stop\nprint 5\nend");
    //runCommand("define hook-call\nprint 6\nend");
    //runCommand("define hookpost-call\nprint 7\nend");
    //runCommand("set step-mode on");  // we can't work with that yes
    //runCommand("set exec-done-display on");
    //runCommand("set print pretty on");
    //runCommand("set confirm off");
    //runCommand("set pagination off");

    // The following does not work with 6.3.50-20050815 (Apple version gdb-1344)
    // (Mac OS 10.6), but does so for gdb-966 (10.5):
    //runCommand("set print inferior-events 1");

    runCommand({"set breakpoint pending on"});
    runCommand({"set print elements 10000"});

    if (settings().useIndexCache())
        runCommand({"set index-cache on"});

    // Produces a few messages during symtab loading
    //runCommand("set verbose on");

    // one of the following is needed to prevent crashes in gdb on code like:
    //  template <class T> T foo() { return T(0); }
    //  int main() { return foo<int>(); }
    //  (gdb) call 'int foo<int>'()
    //  /build/buildd/gdb-6.8/gdb/valops.c:2069: internal-error
    // This seems to be fixed, however, with 'on' it seems to _require_
    // explicit casting of function pointers:
    // GNU gdb (GDB) 7.5.91.20130417-cvs-ubuntu
    //  (gdb) p &Myns::QMetaType::typeName  -> $1 = (const char *(*)(int)) 0xb7cf73b0 <Myns::QMetaType::typeName(int)>
    //  (gdb) p Myns::QMetaType::typeName(1024)  -> 31^error,msg="Couldn't find method Myns::QMetaType::typeName"
    // But we can work around on the dumper side. So let's use the default (i.e. 'on')
    //runCommand("set overload-resolution off");

    //runCommand(_("set demangle-style none"));
    runCommand({"set unwindonsignal on"});
    runCommand({"set width 0"});
    runCommand({"set height 0"});

    // FIXME: Provide proper Gui settings for these:
    //runCommand("set breakpoint always-inserted on", ConsoleCommand);
    // displaced-stepping does not work in Thumb mode.
    //runCommand("set displaced-stepping on");
    //runCommand("set trust-readonly-sections on", ConsoleCommand);
    //runCommand("set remotecache on", ConsoleCommand);
    //runCommand("set non-stop on", ConsoleCommand);

    showStatusMessage(Tr::tr("Setting up inferior..."));

    const DebuggerRunParameters &rp = runParameters();
    // Addint executable to modules list.
    Module module;
    module.startAddress = 0;
    module.endAddress = 0;
    module.modulePath = rp.inferior.command.executable();
    module.moduleName = "<executable>";
    modulesHandler()->updateModule(module);

    // Apply source path mappings from global options.
    //showMessage(_("Assuming Qt is installed at %1").arg(qtInstallPath));
    const SourcePathMap sourcePathMap = mergePlatformQtPath(rp, settings().sourcePathMap());
    const SourcePathMap completeSourcePathMap =
            mergeStartParametersSourcePathMap(rp, sourcePathMap);
    for (auto it = completeSourcePathMap.constBegin(), cend = completeSourcePathMap.constEnd();
         it != cend;
         ++it) {
        runCommand({"set substitute-path " + it.key() + " " + expand(it.value())});
    }

    // Spaces just will not work.
    for (const QString &src : rp.debugSourceLocation) {
        if (QDir(src).exists())
            runCommand({"directory " + src});
        else
            showMessage("# directory does not exist: " + src, LogInput);
    }

    if (!rp.sysRoot.isEmpty()) {
        runCommand({"set sysroot " + rp.sysRoot.path()});
        // sysroot is not enough to correctly locate the sources, so explicitly
        // relocate the most likely place for the debug source
        runCommand({"set substitute-path /usr/src " + rp.sysRoot.path() + "/usr/src"});
    }

    //QByteArray ba = QFileInfo(sp.dumperLibrary).path().toLocal8Bit();
    //if (!ba.isEmpty())
    //    runCommand("set solib-search-path " + ba);

    if (settings().multiInferior() || runParameters().multiProcess) {
        //runCommand("set follow-exec-mode new");
        runCommand({"set detach-on-fork off"});
    }

    // Finally, set up Python.
    // We need to guarantee a roundtrip before the adapter proceeds.
    // Make sure this stays the last command in startGdb().
    // Don't use ConsoleCommand, otherwise Mac won't markup the output.

    //if (terminal()->isUsable())
    //    runCommand({"set inferior-tty " + QString::fromUtf8(terminal()->slaveDevice())});

    const FilePath dumperPath = ICore::resourcePath("debugger");
    if (rp.debugger.command.executable().needsDevice()) {
        // Gdb itself running remotely.
        const FilePath loadOrderFile = dumperPath / "loadorder.txt";
        const expected_str<QByteArray> toLoad = loadOrderFile.fileContents();
        if (!toLoad) {
            AsynchronousMessageBox::critical(Tr::tr("Cannot Find Debugger Initialization Script"),
                                             Tr::tr("Cannot read \"%1\": %2")
                                                 .arg(loadOrderFile.toUserOutput(), toLoad.error()));
            notifyEngineSetupFailed();
            return;
        }

        runCommand({"python import sys, types"});
        QStringList moduleList;
        for (const QByteArray &rawModuleName : toLoad->split('\n')) {
            QString module = QString::fromUtf8(rawModuleName).trimmed();
            if (module.startsWith('#') || module.isEmpty())
                continue;
            if (module == "***bridge***")
                module = "gdbbridge";

            const FilePath codeFile = dumperPath / (module + ".py");
            const expected_str<QByteArray> code = codeFile.fileContents();
            if (!code) {
                qDebug() << Tr::tr("Cannot read \"%1\": %2")
                                .arg(codeFile.toUserOutput(), code.error());
                continue;
            }

            showMessage("Reading " + codeFile.toUserOutput(), LogInput);
            runCommand({QString("python module = types.ModuleType('%1')").arg(module)});
            runCommand({QString("python code = bytes.fromhex('%1').decode('utf-8')")
                            .arg(QString::fromUtf8(code->toHex()))});
            runCommand({QString("python exec(code, module.__dict__)")});
            runCommand({QString("python sys.modules['%1'] = module").arg(module)});
            runCommand({QString("python import %1").arg(module)});

            if (module.endsWith("types"))
                moduleList.append('"' + module + '"');
        }

        runCommand({"python from gdbbridge import *"});
        runCommand(QString("python theDumper.dumpermodules = [%1]").arg(moduleList.join(',')));

    } else {
        // Gdb on local host
        // This is useful (only) in custom gdb builds that did not run 'make install'
        const FilePath uninstalledData = rp.debugger.command.executable().parentDir()
            / "data-directory/python";
        if (uninstalledData.exists())
            runCommand({"python sys.path.append('" + uninstalledData.path() + "')"});

        runCommand({"python sys.path.insert(1, '" + dumperPath.path() + "')"});
        runCommand({"python from gdbbridge import *"});
    }

    const FilePath path = settings().extraDumperFile();
    if (!path.isEmpty() && path.isReadableFile()) {
        DebuggerCommand cmd("addDumperModule");
        cmd.arg("path", path.path());
        runCommand(cmd);
    }

    const QString commands = expand(settings().extraDumperCommands());
    if (!commands.isEmpty())
        runCommand({commands});

    DebuggerCommand cmd1("setFallbackQtVersion");
    cmd1.arg("version", rp.fallbackQtVersion);
    runCommand(cmd1);

    runCommand({"loadDumpers", CB(handlePythonSetup)});

    // Reload peripheral register description.
    peripheralRegisterHandler()->updateRegisterGroups();
}

void GdbEngine::handleGdbStartFailed()
{
    if (usesOutputCollector())
        m_outputCollector.shutdown();
}

void GdbEngine::loadInitScript()
{
    const FilePath script = runParameters().overrideStartScript;
    if (!script.isEmpty()) {
        if (script.isReadableFile()) {
            runCommand({"source " + script.path()});
        } else {
            AsynchronousMessageBox::warning(
            Tr::tr("Cannot Find Debugger Initialization Script"),
            Tr::tr("The debugger settings point to a script file at \"%1\", "
               "which is not accessible. If a script file is not needed, "
               "consider clearing that entry to avoid this warning."
              ).arg(script.toUserOutput()));
        }
    } else {
        const QString commands = nativeStartupCommands().trimmed();
        if (!commands.isEmpty())
            runCommand({commands});
    }
}

void GdbEngine::setEnvironmentVariables()
{
    auto isWindowsPath = [this](const QString &str){
        return HostOsInfo::isWindowsHost()
                && !isRemoteEngine()
                && str.compare("path", Qt::CaseInsensitive) == 0;
    };

    Environment baseEnv = runParameters().debugger.environment;
    Environment runEnv = runParameters().inferior.environment;
    const NameValueItems items = baseEnv.diff(runEnv);
    for (const EnvironmentItem &item : items) {
        // imitate the weird windows gdb behavior of setting the case of the path environment
        // variable name to an all uppercase PATH
        const QString name = isWindowsPath(item.name) ? "PATH" : item.name;
        if (item.operation == EnvironmentItem::Unset
                || item.operation == EnvironmentItem::SetDisabled) {
            runCommand({"unset environment " + name});
        } else {
            if (name != item.name)
                runCommand({"unset environment " + item.name});
            runCommand({"-gdb-set environment " + name + '=' + item.value});
        }
    }
}

void GdbEngine::reloadDebuggingHelpers()
{
    runCommand({"reloadDumpers"});
    reloadLocals();
}

void GdbEngine::handleGdbDone()
{
    if (m_gdbProc.result() == ProcessResult::StartFailed) {
        handleGdbStartFailed();
        QString msg;
        const FilePath wd = m_gdbProc.workingDirectory();
        if (!wd.isReadableDir()) {
            msg = failedToStartMessage() + ' ' + Tr::tr("The working directory \"%1\" is not usable.")
                .arg(wd.toUserOutput());
        } else {
            msg = RunWorker::userMessageForProcessError(QProcess::FailedToStart,
                runParameters().debugger.command.executable());
        }
        handleAdapterStartFailed(msg);
        return;
    }

    const QProcess::ProcessError error = m_gdbProc.error();
    if (error != QProcess::UnknownError) {
        QString msg = RunWorker::userMessageForProcessError(error,
                      runParameters().debugger.command.executable());
        const QString errorString = m_gdbProc.errorString();
        if (!errorString.isEmpty())
            msg += '\n' + errorString;
        showMessage("HANDLE GDB ERROR: " + msg);

        if (error == QProcess::FailedToStart)
            return; // This should be handled by the code trying to start the process.

        AsynchronousMessageBox::critical(Tr::tr("GDB I/O Error"), msg);
    }
    if (m_commandTimer.isActive())
        m_commandTimer.stop();

    notifyDebuggerProcessFinished(m_gdbProc.resultData(), "GDB");
}

void GdbEngine::abortDebuggerProcess()
{
    m_gdbProc.kill();
}

void GdbEngine::resetInferior()
{
    if (!runParameters().commandsForReset.isEmpty()) {
        const QStringList commands = expand(runParameters().commandsForReset).split('\n');
        for (QString command : commands) {
            command = command.trimmed();
            if (!command.isEmpty())
                runCommand({command, ConsoleCommand | NeedsTemporaryStop | NativeCommand});
        }
    }
    m_rerunPending = true;
    requestInterruptInferior();
    runEngine();
}

void GdbEngine::handleAdapterStartFailed(const QString &msg, Id settingsIdHint)
{
    showMessage("ADAPTER START FAILED");
    if (!msg.isEmpty() && !Internal::isTestRun()) {
        const QString title = Tr::tr("Adapter Start Failed");
        ICore::showWarningWithOptions(title, msg, QString(), settingsIdHint);
    }
    notifyEngineSetupFailed();
}

void GdbEngine::prepareForRestart()
{
    m_rerunPending = false;
    m_commandForToken.clear();
    m_flagsForToken.clear();
}

void GdbEngine::handleInferiorPrepared()
{
    CHECK_STATE(EngineSetupRequested);

    notifyEngineSetupOk();
    runEngine();
}

void GdbEngine::handleDebugInfoLocation(const DebuggerResponse &response)
{
    if (response.resultClass == ResultDone) {
        const FilePath debugInfoLocation = runParameters().debugInfoLocation;
        if (!debugInfoLocation.isEmpty() && debugInfoLocation.exists()) {
            const QString curDebugInfoLocations = response.consoleStreamOutput.split('"').value(1);
            QString cmd = "set debug-file-directory " + debugInfoLocation.path();
            if (!curDebugInfoLocations.isEmpty())
                cmd += HostOsInfo::pathListSeparator() + curDebugInfoLocations;
            runCommand({cmd});
        }
    }
}

void GdbEngine::notifyInferiorSetupFailedHelper(const QString &msg)
{
    showStatusMessage(Tr::tr("Failed to start application:") + ' ' + msg);
    if (state() == EngineSetupFailed) {
        showMessage("INFERIOR START FAILED, BUT ADAPTER DIED ALREADY");
        return; // Adapter crashed meanwhile, so this notification is meaningless.
    }
    showMessage("INFERIOR START FAILED");
    AsynchronousMessageBox::critical(Tr::tr("Failed to Start Application"), msg);
    notifyEngineSetupFailed();
}

void GdbEngine::createFullBacktrace()
{
    DebuggerCommand cmd("thread apply all bt full", NeedsTemporaryStop | ConsoleCommand);
    cmd.callback = [](const DebuggerResponse &response) {
        if (response.resultClass == ResultDone) {
            Internal::openTextEditor("Backtrace $",
                response.consoleStreamOutput + response.logStreamOutput);
        }
    };
    runCommand(cmd);
}

void GdbEngine::resetCommandQueue()
{
    m_commandTimer.stop();
    if (!m_commandForToken.isEmpty()) {
        QString msg;
        QTextStream ts(&msg);
        ts << "RESETING COMMAND QUEUE. LEFT OVER TOKENS: ";
        for (const DebuggerCommand &cmd : std::as_const(m_commandForToken))
            ts << "CMD:" << cmd.function;
        m_commandForToken.clear();
        m_flagsForToken.clear();
        showMessage(msg);
    }
}

bool GdbEngine::usesExecInterrupt() const
{
    DebuggerStartMode mode = runParameters().startMode;
    return (mode == AttachToRemoteServer || mode == AttachToRemoteProcess)
            && usesTargetAsync();
}

bool GdbEngine::usesTargetAsync() const
{
    return runParameters().useTargetAsync || settings().targetAsync();
}

void GdbEngine::scheduleTestResponse(int testCase, const QString &response)
{
    if (!m_testCases.contains(testCase) && runParameters().testCase != testCase)
        return;

    int token = currentToken() + 1;
    showMessage(QString("SCHEDULING TEST RESPONSE (CASE: %1, TOKEN: %2, RESPONSE: %3)")
        .arg(testCase).arg(token).arg(response));
    m_scheduledTestResponses[token] = response;
}

QString GdbEngine::msgGdbStopFailed(const QString &why)
{
    return Tr::tr("The gdb process could not be stopped:\n%1").arg(why);
}

QString GdbEngine::msgInferiorStopFailed(const QString &why)
{
    return Tr::tr("Application process could not be stopped:\n%1").arg(why);
}

QString GdbEngine::msgInferiorSetupOk()
{
    return Tr::tr("Application started.");
}

QString GdbEngine::msgInferiorRunOk()
{
    return Tr::tr("Application running.");
}

QString GdbEngine::msgAttachedToStoppedInferior()
{
    return Tr::tr("Attached to stopped application.");
}

QString GdbEngine::msgConnectRemoteServerFailed(const QString &why)
{
    return Tr::tr("Connecting to remote server failed:\n%1").arg(why);
}

void GdbEngine::interruptLocalInferior(qint64 pid)
{
    CHECK_STATE(InferiorStopRequested);
    if (pid <= 0) {
        showMessage("TRYING TO INTERRUPT INFERIOR BEFORE PID WAS OBTAINED", LogError);
        return;
    }
    QString errorMessage;
    if (runParameters().runAsRoot) {
        Environment env = Environment::systemEnvironment();
        RunControl::provideAskPassEntry(env);
        Process proc;
        proc.setCommand(CommandLine{"sudo", {"-A", "kill", "-s", "SIGINT", QString::number(pid)}});
        proc.setEnvironment(env);
        proc.start();
        proc.waitForFinished();
    } else if (interruptProcess(pid, GdbEngineType, &errorMessage)) {
        showMessage("Interrupted " + QString::number(pid));
    } else {
        showMessage(errorMessage, LogError);
        notifyInferiorStopFailed();
    }
}

void GdbEngine::debugLastCommand()
{
    runCommand(m_lastDebuggableCommand);
}

bool GdbEngine::isLocalRunEngine() const
{
    return !isCoreEngine() && !isLocalAttachEngine() && !isRemoteEngine();
}

bool GdbEngine::isPlainEngine() const
{
    return isLocalRunEngine() && !terminal();
}

bool GdbEngine::isCoreEngine() const
{
    return runParameters().startMode == AttachToCore;
}

bool GdbEngine::isRemoteEngine() const
{
    DebuggerStartMode startMode = runParameters().startMode;
    return startMode == StartRemoteProcess || startMode == AttachToRemoteServer;
}

bool GdbEngine::isLocalAttachEngine() const
{
    return runParameters().startMode == AttachToLocalProcess;
}

bool GdbEngine::isTermEngine() const
{
    return isLocalRunEngine() && terminal();
}

bool GdbEngine::usesOutputCollector() const
{
    return isPlainEngine() && !runParameters().debugger.command.executable().needsDevice();
}

void GdbEngine::claimInitialBreakpoints()
{
    CHECK_STATE(EngineRunRequested);

    const DebuggerRunParameters &rp = runParameters();
    if (rp.startMode != AttachToCore) {
        showStatusMessage(Tr::tr("Setting breakpoints..."));
        showMessage(Tr::tr("Setting breakpoints..."));
        BreakpointManager::claimBreakpointsForEngine(this);

        const DebuggerSettings &s = settings();
        const bool onAbort = s.breakOnAbort();
        const bool onWarning = s.breakOnWarning();
        const bool onFatal = s.breakOnFatal();
        if (onAbort || onWarning || onFatal) {
            DebuggerCommand cmd("createSpecialBreakpoints");
            cmd.arg("breakonabort", onAbort);
            cmd.arg("breakonwarning", onWarning);
            cmd.arg("breakonfatal", onFatal);
            runCommand(cmd);
        }
    }

    // It is ok to cut corners here and not wait for createSpecialBreakpoints()'s
    // response, as the command is synchronous from Creator's point of view,
    // and even if it fails (e.g. due to stripped binaries), continuing with
    // the start up is the best we can do.

    if (!rp.commandsAfterConnect.isEmpty()) {
        const QString commands = expand(rp.commandsAfterConnect);
        for (const QString &command : commands.split('\n'))
            runCommand({command, NativeCommand});
    }
}

void GdbEngine::setupInferior()
{
    CHECK_STATE(EngineSetupRequested);

    const DebuggerRunParameters &rp = runParameters();

    //runCommand("set follow-exec-mode new");
    if (rp.breakOnMain)
        runCommand({"tbreak " + mainFunction()});

    if (!rp.solibSearchPath.isEmpty()) {
        DebuggerCommand cmd("appendSolibSearchPath");
        cmd.arg("path", transform(rp.solibSearchPath, &FilePath::path));
        cmd.arg("separator", HostOsInfo::pathListSeparator());
        runCommand(cmd);
    }

    if (rp.startMode == AttachToRemoteProcess) {

        handleInferiorPrepared();

    } else if (isLocalAttachEngine()) {
        // Task 254674 does not want to remove them
        //qq->breakHandler()->removeAllBreakpoints();
        handleInferiorPrepared();

    } else if (isRemoteEngine()) {

        setLinuxOsAbi();
        QString symbolFile;
        if (!rp.symbolFile.isEmpty())
            symbolFile = rp.symbolFile.absoluteFilePath().path();

        //const QByteArray sysroot = sp.sysroot.toLocal8Bit();
        //const QByteArray remoteArch = sp.remoteArchitecture.toLatin1();
        const QString args = runParameters().inferior.command.arguments();

    //    if (!remoteArch.isEmpty())
    //        postCommand("set architecture " + remoteArch);

        if (!args.isEmpty())
            runCommand({"-exec-arguments " + args});

        setEnvironmentVariables();

        // This has to be issued before 'target remote'. On pre-7.0 the
        // command is not present and will result in ' No symbol table is
        // loaded.  Use the "file" command.' as gdb tries to set the
        // value of a variable with name 'target-async'.
        //
        // Testing with -list-target-features which was introduced at
        // the same time would not work either, as this need an existing
        // target.
        //
        // Using it even without a target and having it fail might still
        // be better as:
        // Some external comment: '[but] "set target-async on" with a native
        // windows gdb will work, but then fail when you actually do
        // "run"/"attach", I think..


        // gdb/mi/mi-main.c:1958: internal-error:
        // mi_execute_async_cli_command: Assertion `is_running (inferior_ptid)'
        // failed.\nA problem internal to GDB has been detected,[...]
        if (usesTargetAsync())
            runCommand({"set target-async on", CB(handleSetTargetAsync)});

        if (symbolFile.isEmpty()) {
            showMessage(Tr::tr("No symbol file given."), StatusBar);
            callTargetRemote();
        } else {
            runCommand({"-file-exec-and-symbols \"" + symbolFile + '"',
                        CB(handleFileExecAndSymbols)});
        }

    } else if (isCoreEngine()) {

        setLinuxOsAbi();

        FilePath executable = rp.inferior.command.executable();

        if (executable.isEmpty()) {
            CoreInfo cinfo = CoreInfo::readExecutableNameFromCore(rp.debugger, rp.coreFile);

            if (!cinfo.isCore) {
                AsynchronousMessageBox::warning(Tr::tr("Error Loading Core File"),
                                                Tr::tr("The specified file does not appear to be a core file."));
                notifyEngineSetupFailed();
                return;
            }

            executable = cinfo.foundExecutableName;
            if (executable.isEmpty()) {
                AsynchronousMessageBox::warning(Tr::tr("Error Loading Symbols"),
                                                Tr::tr("No executable to load symbols from specified core."));
                notifyEngineSetupFailed();
                return;
            }
        }

        // Do that first, otherwise no symbols are loaded.
        const QString localExecutablePath = executable.absoluteFilePath().path();
        // This is *not* equivalent to -file-exec-and-symbols. If the file is not executable
        // (contains only debugging symbols), this should still work.
        runCommand({"-file-exec-file \"" + localExecutablePath + '"'});
        runCommand({"-file-symbol-file \"" + localExecutablePath + '"',
                    CB(handleFileExecAndSymbols)});

    } else if (isTermEngine()) {

        const qint64 attachedPID = terminal()->applicationPid();
        const qint64 attachedMainThreadID = terminal()->applicationMainThreadId();
        notifyInferiorPid(ProcessHandle(attachedPID));
        const QString msg = (attachedMainThreadID != -1)
                ? QString("Going to attach to %1 (%2)").arg(attachedPID).arg(attachedMainThreadID)
                : QString("Going to attach to %1").arg(attachedPID);
        showMessage(msg, LogMisc);
        // For some reason, this breaks GDB 9 on Linux. See QTCREATORBUG-26299.
        if (HostOsInfo::isWindowsHost() && m_gdbVersion >= 100000) {
            // Required for debugging MinGW32 apps with 64-bit GDB. See QTCREATORBUG-26208.
            const QString executable
                = runParameters().inferior.command.executable().toFileInfo().absoluteFilePath();
            runCommand({"-file-exec-and-symbols \"" + executable + '"',
                        CB(handleFileExecAndSymbols)});
        } else {
            handleInferiorPrepared();
        }
    } else if (isPlainEngine()) {

        setEnvironmentVariables();
        if (!rp.inferior.workingDirectory.isEmpty())
            runCommand({"cd " + rp.inferior.workingDirectory.path()});
        if (!rp.inferior.command.arguments().isEmpty()) {
            QString args = rp.inferior.command.arguments();
            runCommand({"-exec-arguments " + args});
        }

        QString executable = runParameters().inferior.command.executable().path();
        runCommand({"-file-exec-and-symbols \"" + executable + '"',
                    CB(handleFileExecAndSymbols)});
    }
}

void GdbEngine::runEngine()
{
    CHECK_STATE(EngineRunRequested);

    const DebuggerRunParameters &rp = runParameters();

    if (rp.startMode == AttachToRemoteProcess) {

        claimInitialBreakpoints();
        notifyEngineRunAndInferiorStopOk();

        QString channel = rp.remoteChannel;
        runCommand({"target remote " + channel});

    } else if (isLocalAttachEngine()) {

        const qint64 pid = rp.attachPID.pid();
        showStatusMessage(Tr::tr("Attaching to process %1.").arg(pid));
        runCommand({"attach " + QString::number(pid), [this](const DebuggerResponse &r) {
                        handleLocalAttach(r);
                    }});
        // In some cases we get only output like
        //   "Could not attach to process.  If your uid matches the uid of the target\n"
        //   "process, check the setting of /proc/sys/kernel/yama/ptrace_scope, or try\n"
        //   " again as the root user.  For more details, see /etc/sysctl.d/10-ptrace.conf\n"
        //   " ptrace: Operation not permitted.\n"
        // but no(!) ^ response. Use a second command to force *some* output
        runCommand({"print 24"});

    } else if (isRemoteEngine()) {

        claimInitialBreakpoints();
        if (runParameters().useContinueInsteadOfRun) {
            notifyEngineRunAndInferiorStopOk();
            continueInferiorInternal();
        } else {
            runCommand({"-exec-run", DebuggerCommand::RunRequest, CB(handleExecRun)});
        }

    } else if (isCoreEngine()) {

        claimInitialBreakpoints();
        runCommand({"target core " + runParameters().coreFile.path(), CB(handleTargetCore)});

    } else if (isTermEngine()) {

        const qint64 attachedPID = terminal()->applicationPid();
        const qint64 mainThreadId = terminal()->applicationMainThreadId();
        runCommand({"attach " + QString::number(attachedPID),
                    [this, mainThreadId](const DebuggerResponse &r) {
                        handleStubAttached(r, mainThreadId);
                    }});

    } else if (isPlainEngine()) {

        claimInitialBreakpoints();
        if (runParameters().useContinueInsteadOfRun)
            runCommand({"-exec-continue", DebuggerCommand::RunRequest, CB(handleExecuteContinue)});
        else
            runCommand({"-exec-run", DebuggerCommand::RunRequest, CB(handleExecRun)});

    }
}

void GdbEngine::handleLocalAttach(const DebuggerResponse &response)
{
    QTC_ASSERT(state() == EngineRunRequested || state() == InferiorStopOk, qDebug() << state());
    switch (response.resultClass) {
    case ResultDone:
    case ResultRunning:
    {
        showMessage("INFERIOR ATTACHED");

        QString commands = expand(settings().gdbPostAttachCommands());
        if (!commands.isEmpty())
            runCommand({commands, NativeCommand});

        if (state() == EngineRunRequested) {
            // Happens e.g. for "Attach to unstarted application"
            // We will get a '*stopped' later that we'll interpret as 'spontaneous'
            // So acknowledge the current state and put a delayed 'continue' in the pipe.
            showMessage(Tr::tr("Attached to running application."), StatusBar);
            claimInitialBreakpoints();
            notifyEngineRunAndInferiorRunOk();
        } else {
            // InferiorStopOk, e.g. for "Attach to running application".
            // The *stopped came in between sending the 'attach' and
            // receiving its '^done'.
            claimInitialBreakpoints();
            notifyEngineRunAndInferiorStopOk();
            if (runParameters().continueAfterAttach)
                continueInferiorInternal();
            else
                updateAll();
        }
        break;
    }
    case ResultError:
        if (response.data["msg"].data() == "ptrace: Operation not permitted.") {
            QString msg = msgPtraceError(runParameters().startMode);
            showStatusMessage(Tr::tr("Failed to attach to application: %1").arg(msg));
            AsynchronousMessageBox::warning(Tr::tr("Debugger Error"), msg);
            notifyEngineIll();
            break;
        }
        showStatusMessage(Tr::tr("Failed to attach to application: %1")
                          .arg(QString(response.data["msg"].data())));
        notifyEngineIll();
        break;
    default:
        showStatusMessage(Tr::tr("Failed to attach to application: %1")
                          .arg(QString(response.data["msg"].data())));
        notifyEngineIll();
        break;
    }
}

void GdbEngine::handleRemoteAttach(const DebuggerResponse &response)
{
    CHECK_STATE(EngineSetupRequested);
    switch (response.resultClass) {
    case ResultDone:
    case ResultRunning: {
        showMessage("INFERIOR ATTACHED");
        showMessage(msgAttachedToStoppedInferior(), StatusBar);
        handleInferiorPrepared();
        break;
    }
    case ResultError:
        if (response.data["msg"].data() == "ptrace: Operation not permitted.") {
            notifyInferiorSetupFailedHelper(msgPtraceError(runParameters().startMode));
            break;
        }
        notifyInferiorSetupFailedHelper(response.data["msg"].data());
        break;
    default:
        notifyInferiorSetupFailedHelper(response.data["msg"].data());
        break;
    }
}

void GdbEngine::interruptInferior2()
{
    if (isLocalAttachEngine()) {

        interruptLocalInferior(runParameters().attachPID.pid());

    } else if (isRemoteEngine() || runParameters().startMode == AttachToRemoteProcess
               || m_gdbProc.commandLine().executable().needsDevice()) {

        CHECK_STATE(InferiorStopRequested);
        if (usesTargetAsync()) {
            runCommand({"-exec-interrupt", CB(handleInterruptInferior)});
        } else {
            m_gdbProc.interrupt();
        }

    } else if (isPlainEngine()) {

        interruptLocalInferior(inferiorPid());

    } else if (isTermEngine()) {

        terminal()->interrupt();
    }
}

QChar GdbEngine::mixedDisasmFlag() const
{
    // /m is deprecated since 7.11, and was replaced by /s which works better with optimizations
    return m_gdbVersion >= 71100 ? 's' : 'm';
}

void GdbEngine::shutdownEngine()
{
    if (usesOutputCollector()) {
        showMessage(QString("PLAIN ADAPTER SHUTDOWN %1").arg(state()));
        m_outputCollector.shutdown();
    }

    CHECK_STATE(EngineShutdownRequested);
    showMessage(QString("INITIATE GDBENGINE SHUTDOWN, PROC STATE: %1").arg(m_gdbProc.state()));

    switch (m_gdbProc.state()) {
    case QProcess::Running: {
        if (runParameters().closeMode == KillAndExitMonitorAtClose)
            runCommand({"monitor exit"});
        runCommand({"exitGdb", ExitRequest, CB(handleGdbExit)});
        break;
    }
    case QProcess::NotRunning:
        // Cannot find executable.
        notifyEngineShutdownFinished();
        break;
    case QProcess::Starting:
        showMessage("GDB NOT REALLY RUNNING; KILLING IT");
        m_gdbProc.kill();
        notifyEngineShutdownFinished();
        break;
    }
}

void GdbEngine::handleFileExecAndSymbols(const DebuggerResponse &response)
{
    CHECK_STATE(EngineSetupRequested);

    if (isRemoteEngine()) {
        if (response.resultClass != ResultDone) {
            QString msg = response.data["msg"].data();
            if (!msg.isEmpty()) {
                showMessage(msg);
                showMessage(msg, StatusBar);
            }
        }
        callTargetRemote(); // Proceed nevertheless.

    } else  if (isCoreEngine()) {

        const FilePath core = runParameters().coreFile;
        if (response.resultClass == ResultDone) {
            showMessage(Tr::tr("Symbols found."), StatusBar);
            handleInferiorPrepared();
        } else {
            QString msg = Tr::tr("No symbols found in the core file \"%1\".").arg(core.toUserOutput())
                    + ' ' + Tr::tr("This can be caused by a path length limitation "
                               "in the core file.")
                    + ' ' + Tr::tr("Try to specify the binary in "
                               "Debug > Start Debugging > Load Core File.");
            notifyInferiorSetupFailedHelper(msg);
        }

    } else if (isLocalRunEngine()) {

        if (response.resultClass == ResultDone) {
            handleInferiorPrepared();
        } else {
            QString msg = response.data["msg"].data();
            // Extend the message a bit in unknown cases.
            if (!msg.endsWith("File format not recognized"))
                msg = Tr::tr("Starting executable failed:") + '\n' + msg;
            notifyInferiorSetupFailedHelper(msg);
        }

    }
}

void GdbEngine::handleExecRun(const DebuggerResponse &response)
{
    CHECK_STATE(EngineRunRequested);

    if (response.resultClass == ResultRunning) {

        if (isLocalRunEngine()) {
            QString commands = expand(settings().gdbPostAttachCommands());
            if (!commands.isEmpty())
                runCommand({commands, NativeCommand});
        }

        notifyEngineRunAndInferiorRunOk();
        showMessage("INFERIOR STARTED");
        showMessage(msgInferiorSetupOk(), StatusBar);
    } else {
        showMessage(response.data["msg"].data());
        notifyEngineRunFailed();
    }
}

void GdbEngine::handleSetTargetAsync(const DebuggerResponse &response)
{
    CHECK_STATE(EngineSetupRequested);
    if (response.resultClass == ResultError)
        qDebug() << "Adapter too old: does not support asynchronous mode.";
}

void GdbEngine::callTargetRemote()
{
    CHECK_STATE(EngineSetupRequested);
    QString channel = runParameters().remoteChannel;

    // Don't touch channels with explicitly set protocols.
    if (!channel.startsWith("tcp:") && !channel.startsWith("udp:")
            && !channel.startsWith("file:") && channel.contains(':')
            && !channel.startsWith('|'))
    {
        // "Fix" the IPv6 case with host names without '['...']'
        if (!channel.startsWith('[') && channel.count(':') >= 2) {
            channel.insert(0, '[');
            channel.insert(channel.lastIndexOf(':'), ']');
        }
        channel = "tcp:" + channel;
    }

    if (m_isQnxGdb)
        runCommand({"target qnx " + channel, CB(handleTargetQnx)});
    else if (runParameters().useExtendedRemote)
        runCommand({"target extended-remote " + channel, CB(handleTargetExtendedRemote)});
    else
        runCommand({"target remote " + channel, CB(handleTargetRemote)});
}

void GdbEngine::handleTargetRemote(const DebuggerResponse &response)
{
    CHECK_STATE(EngineSetupRequested);
    if (response.resultClass == ResultDone) {
        // gdb server will stop the remote application itself.
        showMessage("INFERIOR STARTED");
        showMessage(msgAttachedToStoppedInferior(), StatusBar);
        QString commands = expand(settings().gdbPostAttachCommands());
        if (!commands.isEmpty())
            runCommand({commands, NativeCommand});
        handleInferiorPrepared();
    } else {
        // 16^error,msg="hd:5555: Connection timed out."
        notifyInferiorSetupFailedHelper(msgConnectRemoteServerFailed(response.data["msg"].data()));
    }
}

void GdbEngine::handleTargetExtendedRemote(const DebuggerResponse &response)
{
    CHECK_STATE(EngineSetupRequested);
    if (response.resultClass == ResultDone) {
        showMessage("ATTACHED TO GDB SERVER STARTED");
        showMessage(msgAttachedToStoppedInferior(), StatusBar);
        QString commands = expand(settings().gdbPostAttachCommands());
        if (!commands.isEmpty())
            runCommand({commands, NativeCommand});
        if (runParameters().attachPID.isValid()) { // attach to pid if valid
            // gdb server will stop the remote application itself.
            runCommand({"attach " + QString::number(runParameters().attachPID.pid()),
                        CB(handleTargetExtendedAttach)});
        } else if (!runParameters().inferior.command.isEmpty()) {
            runCommand({"-gdb-set remote exec-file " + runParameters().inferior.command.executable().path(),
                        CB(handleTargetExtendedAttach)});
        } else {
            const QString title = Tr::tr("No Remote Executable or Process ID Specified");
            const QString msg = Tr::tr(
                "No remote executable could be determined from your build system files.<p>"
                "In case you use qmake, consider adding<p>"
                "&nbsp;&nbsp;&nbsp;&nbsp;target.path = /tmp/your_executable # path on device<br>"
                "&nbsp;&nbsp;&nbsp;&nbsp;INSTALLS += target</p>"
                "to your .pro file.");
            QMessageBox *mb = showMessageBox(QMessageBox::Critical, title, msg,
                QMessageBox::Ok | QMessageBox::Cancel);
            mb->button(QMessageBox::Cancel)->setText(Tr::tr("Continue Debugging"));
            mb->button(QMessageBox::Ok)->setText(Tr::tr("Stop Debugging"));
            if (mb->exec() == QMessageBox::Ok) {
                showMessage("KILLING DEBUGGER AS REQUESTED BY USER");
                notifyInferiorSetupFailedHelper(title);
            } else {
                showMessage("CONTINUE DEBUGGER AS REQUESTED BY USER");
                handleInferiorPrepared(); // This will likely fail.
            }
        }
    } else {
        notifyInferiorSetupFailedHelper(msgConnectRemoteServerFailed(response.data["msg"].data()));
    }
}

void GdbEngine::handleTargetExtendedAttach(const DebuggerResponse &response)
{
    CHECK_STATE(EngineSetupRequested);
    if (response.resultClass == ResultDone) {
        // gdb server will stop the remote application itself.
        handleInferiorPrepared();
    } else {
        notifyInferiorSetupFailedHelper(msgConnectRemoteServerFailed(response.data["msg"].data()));
    }
}

void GdbEngine::handleTargetQnx(const DebuggerResponse &response)
{
    CHECK_STATE(EngineSetupRequested);
    if (response.resultClass == ResultDone) {
        // gdb server will stop the remote application itself.
        showMessage("INFERIOR STARTED");
        showMessage(msgAttachedToStoppedInferior(), StatusBar);

        const DebuggerRunParameters &rp = runParameters();
        if (rp.attachPID.isValid())
            runCommand({"attach " + QString::number(rp.attachPID.pid()), CB(handleRemoteAttach)});
        else if (!rp.inferior.command.isEmpty())
            runCommand({"set nto-executable " + rp.inferior.command.executable().path(),
                        CB(handleSetNtoExecutable)});
        else
            handleInferiorPrepared();
    } else {
        // 16^error,msg="hd:5555: Connection timed out."
        notifyInferiorSetupFailedHelper(response.data["msg"].data());
    }
}

void GdbEngine::handleSetNtoExecutable(const DebuggerResponse &response)
{
    CHECK_STATE(EngineSetupRequested);
    switch (response.resultClass) {
    case ResultDone:
    case ResultRunning: {
        showMessage("EXECUTABLE SET");
        showMessage(msgAttachedToStoppedInferior(), StatusBar);
        handleInferiorPrepared();
        break;
    }
    case ResultError:
    default:
        notifyInferiorSetupFailedHelper(response.data["msg"].data());
    }
}

void GdbEngine::handleInterruptInferior(const DebuggerResponse &response)
{
    if (response.resultClass == ResultDone) {
        // The gdb server will trigger extra output that we will pick up
        // to do a proper state transition.
    } else {
        // FIXME: On some gdb versions like git 170ffa5d7dd this produces
        // >810^error,msg="mi_cmd_exec_interrupt: Inferior not executing."
        notifyInferiorStopOk();
    }
}

void GdbEngine::handleStubAttached(const DebuggerResponse &response, qint64 mainThreadId)
{
    // InferiorStopOk can happen if the "*stopped" in response to the
    // 'attach' comes in before its '^done'
    QTC_ASSERT(state() == EngineRunRequested || state() == InferiorStopOk, qDebug() << state());

    switch (response.resultClass) {
    case ResultDone:
    case ResultRunning:
        claimInitialBreakpoints();
        if (runParameters().toolChainAbi.os() == ProjectExplorer::Abi::WindowsOS) {
            QString errorMessage;
            // Resume thread that was suspended by console stub process (see stub code).
            if (winResumeThread(mainThreadId, &errorMessage)) {
                showMessage(QString("Inferior attached, thread %1 resumed").
                            arg(mainThreadId), LogMisc);
            } else {
                showMessage(QString("Inferior attached, unable to resume thread %1: %2").
                            arg(mainThreadId).arg(errorMessage),
                            LogWarning);
            }
            notifyEngineRunAndInferiorStopOk();
            continueInferiorInternal();
        } else {
            showMessage("INFERIOR ATTACHED");
            QTC_ASSERT(terminal(), return);
            terminal()->kickoffProcess();
            //notifyEngineRunAndInferiorRunOk();
            // Wait for the upcoming *stopped and handle it there.
        }
        break;
    case ResultError:
        if (response.data["msg"].data() == "ptrace: Operation not permitted.") {
            notifyInferiorSetupFailedHelper(msgPtraceError(runParameters().startMode));
            break;
        }
        showMessage(response.data["msg"].data());
        notifyEngineIll();
        break;
    default:
        showMessage(QString("Invalid response %1").arg(response.resultClass));
        notifyEngineIll();
        break;
    }
}

static FilePath findExecutableFromName(const QString &fileNameFromCore, const FilePath &coreFile)
{
    if (fileNameFromCore.isEmpty())
        return {};

    FilePath filePathFromCore = FilePath::fromUserInput(fileNameFromCore);
    if (filePathFromCore.isFile())
        return filePathFromCore;

    // turn the filename into an absolute path, using the location of the core as a hint
    const FilePath coreDir = coreFile.absoluteFilePath().parentDir();
    const FilePath absPath = coreDir.resolvePath(fileNameFromCore);

    if (absPath.isFile() || absPath.isEmpty())
        return absPath;

    // remove possible trailing arguments
    QStringList pathFragments = absPath.path().split(' ');
    while (pathFragments.size() > 0) {
        const FilePath joined_path = FilePath::fromString(pathFragments.join(' '));
        if (joined_path.isFile())
            return joined_path;
        pathFragments.pop_back();
    }

    return {};
}

CoreInfo CoreInfo::readExecutableNameFromCore(const ProcessRunData &debugger, const FilePath &coreFile)
{
    CoreInfo cinfo;
#if 0
    ElfReader reader(coreFile);
    cinfo.rawStringFromCore = QString::fromLocal8Bit(reader.readCoreName(&cinfo.isCore));
    cinfo.foundExecutableName = findExecutableFromName(cinfo.rawStringFromCore, coreFile);
#else
    QStringList args = {"-nx",  "-batch"};
    // Multiarch GDB on Windows crashes if osabi is cygwin (the default) when opening a core dump.
    if (HostOsInfo::isWindowsHost())
        args += {"-ex", "set osabi GNU/Linux"};
    args += {"-ex", "core " + coreFile.toUserOutput()};

    Process proc;
    Environment envLang(Environment::systemEnvironment());
    envLang.setupEnglishOutput();
    proc.setEnvironment(envLang);
    proc.setCommand({debugger.command.executable(), args});
    proc.runBlocking();

    if (proc.result() == ProcessResult::FinishedWithSuccess) {
        QString output = proc.cleanedStdOut();
        // Core was generated by `/data/dev/creator-2.6/bin/qtcreator'.
        // Program terminated with signal 11, Segmentation fault.
        int pos1 = output.indexOf("Core was generated by");
        if (pos1 != -1) {
            pos1 += 23;
            int pos2 = output.indexOf('\'', pos1);
            if (pos2 != -1) {
                cinfo.isCore = true;
                cinfo.rawStringFromCore = output.mid(pos1, pos2 - pos1);
                cinfo.foundExecutableName = findExecutableFromName(cinfo.rawStringFromCore, coreFile);
            }
        }
    }
#endif
    return cinfo;
}

void GdbEngine::handleTargetCore(const DebuggerResponse &response)
{
    CHECK_STATE(EngineRunRequested);
    notifyEngineRunOkAndInferiorUnrunnable();
    showMessage(Tr::tr("Attached to core."), StatusBar);
    if (response.resultClass == ResultError) {
        // We'll accept any kind of error e.g. &"Cannot access memory at address 0x2abc2a24\n"
        // Even without the stack, the user can find interesting stuff by exploring
        // the memory, globals etc.
        showStatusMessage(
            Tr::tr("Attach to core \"%1\" failed:").arg(runParameters().coreFile.toUserOutput())
            + '\n' + response.data["msg"].data() + '\n' + Tr::tr("Continuing nevertheless."));
    }
    // Due to the auto-solib-add off setting, we don't have any
    // symbols yet. Load them in order of importance.
    reloadStack();
    reloadModulesInternal();
    runCommand({"p 5", CB(handleCoreRoundTrip)});
}

void GdbEngine::handleCoreRoundTrip(const DebuggerResponse &response)
{
    CHECK_STATE(InferiorUnrunnable);
    Q_UNUSED(response)
    loadSymbolsForStack();
    handleStop3();
    QTimer::singleShot(1000, this, &GdbEngine::loadAllSymbols);
}

void GdbEngine::doUpdateLocals(const UpdateParameters &params)
{
    watchHandler()->notifyUpdateStarted(params);

    DebuggerCommand cmd("fetchVariables", Discardable|InUpdateLocals);
    watchHandler()->appendFormatRequests(&cmd);
    watchHandler()->appendWatchersAndTooltipRequests(&cmd);

    const bool alwaysVerbose = qtcEnvironmentVariableIsSet("QTC_DEBUGGER_PYTHON_VERBOSE");
    const DebuggerSettings &s = settings();
    cmd.arg("passexceptions", alwaysVerbose);
    cmd.arg("fancy", s.useDebuggingHelpers());
    cmd.arg("autoderef", s.autoDerefPointers());
    cmd.arg("dyntype", s.useDynamicType());
    cmd.arg("qobjectnames", s.showQObjectNames());
    cmd.arg("timestamps", s.logTimeStamps());

    StackFrame frame = stackHandler()->currentFrame();
    cmd.arg("context", frame.context);
    cmd.arg("nativemixed", isNativeMixedActive());

    cmd.arg("stringcutoff", s.maximalStringLength());
    cmd.arg("displaystringlimit", s.displayStringLimit());

    cmd.arg("resultvarname", m_resultVarName);
    cmd.arg("partialvar", params.partialVariable);

    m_lastDebuggableCommand = cmd;
    m_lastDebuggableCommand.arg("passexceptions", "1");

    cmd.callback = CB(handleFetchVariables);

    runCommand(cmd);
}

void GdbEngine::handleFetchVariables(const DebuggerResponse &response)
{
    m_inUpdateLocals = false;
    updateLocalsView(response.data);
    watchHandler()->notifyUpdateFinished();
    updateToolTips();
}

QString GdbEngine::msgPtraceError(DebuggerStartMode sm)
{
    if (sm == StartInternal) {
        return Tr::tr(
            "ptrace: Operation not permitted.\n\n"
            "Could not attach to the process. "
            "Make sure no other debugger traces this process.\n"
            "Check the settings of\n"
            "/proc/sys/kernel/yama/ptrace_scope\n"
            "For more details, see /etc/sysctl.d/10-ptrace.conf\n");
    }
    return Tr::tr(
        "ptrace: Operation not permitted.\n\n"
        "Could not attach to the process. "
        "Make sure no other debugger traces this process.\n"
        "If your uid matches the uid\n"
        "of the target process, check the settings of\n"
        "/proc/sys/kernel/yama/ptrace_scope\n"
        "For more details, see /etc/sysctl.d/10-ptrace.conf\n");
}

QString GdbEngine::mainFunction() const
{
    const DebuggerRunParameters &rp = runParameters();
    return QLatin1String(rp.toolChainAbi.os() == Abi::WindowsOS && !terminal() ? "qMain" : "main");
}

//
// Factory
//

DebuggerEngine *createGdbEngine()
{
    return new GdbEngine;
}

} // Debugger::Internal

Q_DECLARE_METATYPE(Debugger::Internal::GdbMi)
Q_DECLARE_METATYPE(Debugger::Internal::TracepointCaptureData)
