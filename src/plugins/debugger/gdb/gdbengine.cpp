/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "gdbengine.h"

#include "attachgdbadapter.h"
#include "coregdbadapter.h"
#include "gdbplainengine.h"
#include "termgdbadapter.h"
#include "remotegdbserveradapter.h"
#include "gdboptionspage.h"

#include <debugger/debuggerstartparameters.h>
#include <debugger/debuggerinternalconstants.h>
#include <debugger/debuggerruncontrol.h>
#include <debugger/disassemblerlines.h>

#include <debugger/debuggeractions.h>
#include <debugger/debuggercore.h>
#include <debugger/debuggermainwindow.h>
#include <debugger/debuggerplugin.h>
#include <debugger/debuggerprotocol.h>
#include <debugger/debuggerstringutils.h>
#include <debugger/debuggertooltipmanager.h>
#include <debugger/disassembleragent.h>
#include <debugger/memoryagent.h>
#include <debugger/sourceutils.h>
#include <debugger/terminal.h>

#include <debugger/breakhandler.h>
#include <debugger/moduleshandler.h>
#include <debugger/registerhandler.h>
#include <debugger/sourcefileshandler.h>
#include <debugger/stackhandler.h>
#include <debugger/threadshandler.h>
#include <debugger/debuggersourcepathmappingwidget.h>
#include <debugger/logwindow.h>
#include <debugger/procinterrupt.h>
#include <debugger/shared/hostutils.h>

#include <coreplugin/icore.h>
#include <coreplugin/messagebox.h>

#include <projectexplorer/devicesupport/deviceprocess.h>
#include <projectexplorer/itaskhandler.h>
#include <projectexplorer/taskhub.h>

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/macroexpander.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/savedaction.h>

#include <QBuffer>
#include <QDirIterator>
#include <QMessageBox>
#include <QPushButton>
#include <QTemporaryFile>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace Debugger {
namespace Internal {

enum { debugPending = 0 };

#define PENDING_DEBUG(s) do { if (debugPending) qDebug() << s; } while (0)

#define CB(callback) [this](const DebuggerResponse &r) { callback(r); }

#define CHECK_STATE(s) \
    do { \
        if (state() != s) { \
            showMessage(QString::fromLatin1("UNEXPECTED STATE: %1  WANTED: %2 IN %3:%4") \
                .arg(state()).arg(s).arg(QLatin1String(__FILE__)).arg(__LINE__), LogError); \
            QTC_ASSERT(false, qDebug() << state() << s); \
        } \
    } while (0)

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
    case InferiorSetupOk:
    case EngineRunFailed:
    case InferiorExitOk:
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

static QByteArray parsePlainConsoleStream(const DebuggerResponse &response)
{
    QByteArray out = response.consoleStreamOutput;
    // FIXME: proper decoding needed
    if (out.endsWith("\\n"))
        out.chop(2);
    while (out.endsWith('\n') || out.endsWith(' '))
        out.chop(1);
    int pos = out.indexOf(" = ");
    return out.mid(pos + 3);
}

static bool isMostlyHarmlessMessage(const QByteArray &msg)
{
    return msg == "warning: GDB: Failed to set controlling terminal: "
                  "Inappropriate ioctl for device\\n"
            || msg == "warning: GDB: Failed to set controlling terminal: Invalid argument\\n";
}

///////////////////////////////////////////////////////////////////////
//
// Debuginfo Taskhandler
//
///////////////////////////////////////////////////////////////////////

class DebugInfoTask
{
public:
    QString command;
};

class DebugInfoTaskHandler : public ITaskHandler
{
public:
    explicit DebugInfoTaskHandler(GdbEngine *engine)
        : m_engine(engine)
    {}

    bool canHandle(const Task &task) const
    {
        return m_debugInfoTasks.contains(task.taskId);
    }

    void handle(const Task &task)
    {
        m_engine->requestDebugInformation(m_debugInfoTasks.value(task.taskId));
    }

    void addTask(unsigned id, const DebugInfoTask &task)
    {
        m_debugInfoTasks[id] = task;
    }

    QAction *createAction(QObject *parent) const
    {
        QAction *action = new QAction(DebuggerPlugin::tr("Install &Debug Information"), parent);
        action->setToolTip(DebuggerPlugin::tr("Tries to install missing debug information."));
        return action;
    }

private:
    GdbEngine *m_engine;
    QHash<unsigned, DebugInfoTask> m_debugInfoTasks;
};

///////////////////////////////////////////////////////////////////////
//
// GdbEngine
//
///////////////////////////////////////////////////////////////////////

GdbEngine::GdbEngine(const DebuggerStartParameters &startParameters)
  : DebuggerEngine(startParameters)
{
    setObjectName(_("GdbEngine"));

    m_busy = false;
    m_gdbVersion = 100;
    m_isQnxGdb = false;
    m_registerNamesListed = false;
    m_sourcesListUpdating = false;
    m_oldestAcceptableToken = -1;
    m_nonDiscardableCount = 0;
    m_outputCodec = QTextCodec::codecForLocale();
    m_pendingBreakpointRequests = 0;
    m_commandsDoneCallback = 0;
    m_stackNeeded = false;
    m_terminalTrap = startParameters.useTerminal;
    m_fullStartDone = false;
    m_systemDumpersLoaded = false;
    m_gdbProc = new GdbProcess(this);

    m_debugInfoTaskHandler = new DebugInfoTaskHandler(this);
    //ExtensionSystem::PluginManager::addObject(m_debugInfoTaskHandler);

    m_commandTimer.setSingleShot(true);
    connect(&m_commandTimer, &QTimer::timeout,
            this, &GdbEngine::commandTimeout);

    connect(action(AutoDerefPointers), &SavedAction::valueChanged,
            this, &GdbEngine::reloadLocals);
    connect(action(CreateFullBacktrace), &QAction::triggered,
            this, &GdbEngine::createFullBacktrace);
    connect(action(UseDebuggingHelpers), &SavedAction::valueChanged,
            this, &GdbEngine::reloadLocals);
    connect(action(UseDynamicType), &SavedAction::valueChanged,
            this, &GdbEngine::reloadLocals);
}

GdbEngine::~GdbEngine()
{
    //ExtensionSystem::PluginManager::removeObject(m_debugInfoTaskHandler);
    delete m_debugInfoTaskHandler;
    m_debugInfoTaskHandler = 0;

    // Prevent sending error messages afterwards.
    disconnect();
}

DebuggerStartMode GdbEngine::startMode() const
{
    return startParameters().startMode;
}

QString GdbEngine::errorMessage(QProcess::ProcessError error)
{
    switch (error) {
        case QProcess::FailedToStart:
            return tr("The gdb process failed to start. Either the "
                "invoked program \"%1\" is missing, or you may have insufficient "
                "permissions to invoke the program.\n%2")
                .arg(m_gdb, m_gdbProc->errorString());
        case QProcess::Crashed:
            if (targetState() == DebuggerFinished)
                return tr("The gdb process crashed some time after starting "
                    "successfully.");
            else
                return tr("The gdb process was ended forcefully");
        case QProcess::Timedout:
            return tr("The last waitFor...() function timed out. "
                "The state of QProcess is unchanged, and you can try calling "
                "waitFor...() again.");
        case QProcess::WriteError:
            return tr("An error occurred when attempting to write "
                "to the gdb process. For example, the process may not be running, "
                "or it may have closed its input channel.");
        case QProcess::ReadError:
            return tr("An error occurred when attempting to read from "
                "the gdb process. For example, the process may not be running.");
        default:
            return tr("An unknown error in the gdb process occurred.");
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

// Parse "~:gdb: unknown target exception 0xc0000139 at 0x77bef04e\n"
// and return an exception message
static inline QString msgWinException(const QByteArray &data, unsigned *exCodeIn = 0)
{
    if (exCodeIn)
        *exCodeIn = 0;
    const int exCodePos = data.indexOf("0x");
    const int blankPos = exCodePos != -1 ? data.indexOf(' ', exCodePos + 1) : -1;
    const int addressPos = blankPos != -1 ? data.indexOf("0x", blankPos + 1) : -1;
    if (addressPos < 0)
        return GdbEngine::tr("An exception was triggered.");
    const unsigned exCode = data.mid(exCodePos, blankPos - exCodePos).toUInt(0, 0);
    if (exCodeIn)
        *exCodeIn = exCode;
    const quint64 address = data.mid(addressPos).trimmed().toULongLong(0, 0);
    QString rc;
    QTextStream str(&rc);
    str << GdbEngine::tr("An exception was triggered:") << ' ';
    formatWindowsException(exCode, address, 0, 0, 0, str);
    str << '.';
    return rc;
}

void GdbEngine::readDebugeeOutput(const QByteArray &data)
{
    if (isMostlyHarmlessMessage(data.mid(2, data.size() - 4)))
        return;
    QString msg = m_outputCodec->toUnicode(data.constData(), data.length(),
        &m_outputCodecState);
    showMessage(msg, AppOutput);
}

static bool isNameChar(char c)
{
    // could be 'stopped' or 'shlibs-added'
    return (c >= 'a' && c <= 'z') || c == '-';
}

static bool contains(const QByteArray &message, const char *pattern, int size)
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

static bool isGdbConnectionError(const QByteArray &message)
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

void GdbEngine::handleResponse(const QByteArray &buff)
{
    showMessage(QString::fromLocal8Bit(buff, buff.length()), LogOutput);

    if (buff.isEmpty() || buff == "(gdb) ")
        return;

    const char *from = buff.constData();
    const char *to = from + buff.size();
    const char *inner;

    int token = -1;
    // Token is a sequence of numbers.
    for (inner = from; inner != to; ++inner)
        if (*inner < '0' || *inner > '9')
            break;
    if (from != inner) {
        token = QByteArray(from, inner - from).toInt();
        from = inner;
    }

    // Next char decides kind of response.
    const char c = *from++;
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
                    result.m_children.push_back(data);
                    result.m_type = GdbMi::Tuple;
                }
            }
            if (asyncClass == "stopped") {
                handleStopResponse(result);
                m_pendingLogStreamOutput.clear();
                m_pendingConsoleStreamOutput.clear();
            } else if (asyncClass == "running") {
                GdbMi threads = result["thread-id"];
                threadsHandler()->notifyRunning(threads.data());
                if (state() == InferiorRunOk || state() == InferiorSetupRequested) {
                    // We get multiple *running after thread creation and in Windows terminals.
                    showMessage(QString::fromLatin1("NOTE: INFERIOR STILL RUNNING IN STATE %1.").
                                arg(QLatin1String(DebuggerEngine::stateName(state()))));
                } else if (HostOsInfo::isWindowsHost() && (state() == InferiorStopRequested
                               || state() == InferiorShutdownRequested)) {
                    // FIXME: Breakpoints on Windows are exceptions which are thrown in newly
                    // created threads so we have to filter out the running threads messages when
                    // we request a stop.
                } else {
                    notifyInferiorRunOk();
                }
            } else if (asyncClass == "library-loaded") {
                // Archer has 'id="/usr/lib/libdrm.so.2",
                // target-name="/usr/lib/libdrm.so.2",
                // host-name="/usr/lib/libdrm.so.2",
                // symbols-loaded="0"

                // id="/lib/i386-linux-gnu/libc.so.6"
                // target-name="/lib/i386-linux-gnu/libc.so.6"
                // host-name="/lib/i386-linux-gnu/libc.so.6"
                // symbols-loaded="0",thread-group="i1"
                QByteArray id = result["id"].data();
                if (!id.isEmpty())
                    showStatusMessage(tr("Library %1 loaded").arg(_(id)), 1000);
                progressPing();
                Module module;
                module.startAddress = 0;
                module.endAddress = 0;
                module.hostPath = _(result["host-name"].data());
                module.modulePath = _(result["target-name"].data());
                module.moduleName = QFileInfo(module.hostPath).baseName();
                modulesHandler()->updateModule(module);
            } else if (asyncClass == "library-unloaded") {
                // Archer has 'id="/usr/lib/libdrm.so.2",
                // target-name="/usr/lib/libdrm.so.2",
                // host-name="/usr/lib/libdrm.so.2"
                QByteArray id = result["id"].data();
                progressPing();
                showStatusMessage(tr("Library %1 unloaded").arg(_(id)), 1000);
            } else if (asyncClass == "thread-group-added") {
                // 7.1-symbianelf has "{id="i1"}"
            } else if (asyncClass == "thread-group-created"
                    || asyncClass == "thread-group-started") {
                // Archer had only "{id="28902"}" at some point of 6.8.x.
                // *-started seems to be standard in 7.1, but in early
                // 7.0.x, there was a *-created instead.
                progressPing();
                // 7.1.50 has thread-group-started,id="i1",pid="3529"
                QByteArray id = result["id"].data();
                showStatusMessage(tr("Thread group %1 created").arg(_(id)), 1000);
                int pid = id.toInt();
                if (!pid) {
                    id = result["pid"].data();
                    pid = id.toInt();
                }
                if (pid)
                    notifyInferiorPid(pid);
                handleThreadGroupCreated(result);
            } else if (asyncClass == "thread-created") {
                //"{id="1",group-id="28902"}"
                QByteArray id = result["id"].data();
                showStatusMessage(tr("Thread %1 created").arg(_(id)), 1000);
                ThreadData thread;
                thread.id = ThreadId(id.toLong());
                thread.groupId = result["group-id"].data();
                threadsHandler()->updateThread(thread);
            } else if (asyncClass == "thread-group-exited") {
                // Archer has "{id="28902"}"
                QByteArray id = result["id"].data();
                showStatusMessage(tr("Thread group %1 exited").arg(_(id)), 1000);
                handleThreadGroupExited(result);
            } else if (asyncClass == "thread-exited") {
                //"{id="1",group-id="28902"}"
                QByteArray id = result["id"].data();
                QByteArray groupid = result["group-id"].data();
                showStatusMessage(tr("Thread %1 in group %2 exited")
                    .arg(_(id)).arg(_(groupid)), 1000);
                threadsHandler()->removeThread(ThreadId(id.toLong()));
            } else if (asyncClass == "thread-selected") {
                QByteArray id = result["id"].data();
                showStatusMessage(tr("Thread %1 selected").arg(_(id)), 1000);
                //"{id="2"}"
            } else if (asyncClass == "breakpoint-modified") {
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
                QByteArray ba = result.toString();
                ba = '[' + ba.mid(6, ba.size() - 7) + ']';
                const int pos1 = ba.indexOf(",original-location");
                const int pos2 = ba.indexOf("\":", pos1 + 2);
                const int pos3 = ba.indexOf('"', pos2 + 2);
                ba.remove(pos1, pos3 - pos1 + 1);
                result = GdbMi();
                result.fromString(ba);
                BreakHandler *handler = breakHandler();
                Breakpoint bp;
                BreakpointResponse br;
                foreach (const GdbMi &bkpt, result.children()) {
                    const QByteArray nr = bkpt["number"].data();
                    BreakpointResponseId rid(nr);
                    if (!isHiddenBreakpoint(rid)) {
                        if (nr.contains('.')) {
                            // A sub-breakpoint.
                            BreakpointResponse sub;
                            updateResponse(sub, bkpt);
                            sub.id = rid;
                            sub.type = br.type;
                            bp.insertSubBreakpoint(sub);
                        } else {
                            // A primary breakpoint.
                            bp = handler->findBreakpointByResponseId(rid);
                            //qDebug() << "NR: " << nr << "RID: " << rid
                            //    << "ID: " << bp.id();
                            br = bp.response();
                            updateResponse(br, bkpt);
                            bp.setResponse(br);
                        }
                    }
                }
            } else if (asyncClass == "breakpoint-created") {
                // "{bkpt={number="1",type="breakpoint",disp="del",enabled="y",
                //  addr="<PENDING>",pending="main",times="0",
                //  original-location="main"}}" -- or --
                // {bkpt={number="2",type="hw watchpoint",disp="keep",enabled="y",
                // what="*0xbfffed48",times="0",original-location="*0xbfffed48"}}
                BreakHandler *handler = breakHandler();
                foreach (const GdbMi &bkpt, result.children()) {
                    BreakpointResponse br;
                    br.type = BreakpointByFileAndLine;
                    updateResponse(br, bkpt);
                    handler->handleAlienBreakpoint(br, this);
                }
            } else if (asyncClass == "breakpoint-deleted") {
                // "breakpoint-deleted" "{id="1"}"
                // New in FSF gdb since 2011-04-27.
                QByteArray nr = result["id"].data();
                BreakpointResponseId rid(nr);
                if (Breakpoint bp = breakHandler()->findBreakpointByResponseId(rid)) {
                    // This also triggers when a temporary breakpoint is hit.
                    // We do not really want that, as this loses all information.
                    // FIXME: Use a special marker for this case?
                    if (!bp.isOneShot())
                        bp.removeAlienBreakpoint();
                }
            } else if (asyncClass == "cmd-param-changed") {
                // New since 2012-08-09
                //  "{param="debug remote",value="1"}"
            } else if (asyncClass == "memory-changed") {
                // New since 2013
                //   "{thread-group="i1",addr="0x0918a7a8",len="0x10"}"
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
            } else if (data.startsWith("[New ") || data.startsWith("[Thread ")) {
                if (data.endsWith('\n'))
                    data.chop(1);
                progressPing();
                showStatusMessage(_(data), 1000);
            } else if (data.startsWith("gdb: unknown target exception 0x")) {
                // [Windows, most likely some DLL/Entry point not found]:
                // "gdb: unknown target exception 0xc0000139 at 0x77bef04e"
                // This may be fatal and cause the target to exit later
                unsigned exCode;
                m_lastWinException = msgWinException(data, &exCode);
                showMessage(m_lastWinException, LogMisc);
                const Task::TaskType type = isFatalWinException(exCode) ? Task::Error : Task::Warning;
                TaskHub::addTask(type, m_lastWinException, Constants::TASK_CATEGORY_DEBUGGER_RUNTIME);
            }
            break;
        }

        case '@': {
            readDebugeeOutput(GdbMi::parseCString(from, to));
            break;
        }

        case '&': {
            QByteArray data = GdbMi::parseCString(from, to);
            // On Windows, the contents seem to depend on the debugger
            // version and/or OS version used.
            if (data.startsWith("warning:")) {
                if (isMostlyHarmlessMessage(data)) {
                    showMessage(_("Mostly harmless terminal warning suppressed."), LogWarning);
                    break;
                }
                showMessage(_(data.mid(9)), AppStuff); // Cut "warning: "
            }

            m_pendingLogStreamOutput += data;

            if (isGdbConnectionError(data)) {
                notifyInferiorExited();
                break;
            }

            if (boolSetting(IdentifyDebugInfoPackages)) {
                // From SuSE's gdb: >&"Missing separate debuginfo for ...\n"
                // ">&"Try: zypper install -C \"debuginfo(build-id)=c084ee5876ed1ac12730181c9f07c3e027d8e943\"\n"
                if (data.startsWith("Missing separate debuginfo for ")) {
                    m_lastMissingDebugInfo = QString::fromLocal8Bit(data.mid(32));
                } else if (data.startsWith("Try: zypper")) {
                    QString cmd = QString::fromLocal8Bit(data.mid(4));

                    Task task(Task::Warning,
                        tr("Missing debug information for %1\nTry: %2")
                            .arg(m_lastMissingDebugInfo).arg(cmd),
                        FileName(), 0, Debugger::Constants::TASK_CATEGORY_DEBUGGER_DEBUGINFO);

                    TaskHub::addTask(task);

                    DebugInfoTask dit;
                    dit.command = cmd;
                    m_debugInfoTaskHandler->addTask(task.taskId, dit);
                }
            }

            break;
        }

        case '^': {
            DebuggerResponse response;

            response.token = token;

            for (inner = from; inner != to; ++inner)
                if (*inner < 'a' || *inner > 'z')
                    break;

            QByteArray resultClass = QByteArray::fromRawData(from, inner - from);
            if (resultClass == "done")
                response.resultClass = ResultDone;
            else if (resultClass == "running")
                response.resultClass = ResultRunning;
            else if (resultClass == "connected")
                response.resultClass = ResultConnected;
            else if (resultClass == "error")
                response.resultClass = ResultError;
            else if (resultClass == "exit")
                response.resultClass = ResultExit;
            else
                response.resultClass = ResultUnknown;

            from = inner;
            if (from != to) {
                if (*from == ',') {
                    ++from;
                    response.data.parseTuple_helper(from, to);
                    response.data.m_type = GdbMi::Tuple;
                    response.data.m_name = "data";
                } else {
                    // Archer has this.
                    response.data.m_type = GdbMi::Tuple;
                    response.data.m_name = "data";
                }
            }

            //qDebug() << "\nLOG STREAM:" + m_pendingLogStreamOutput;
            //qDebug() << "\nCONSOLE STREAM:" + m_pendingConsoleStreamOutput;
            response.logStreamOutput = m_pendingLogStreamOutput;
            response.consoleStreamOutput =  m_pendingConsoleStreamOutput;
            m_pendingLogStreamOutput.clear();
            m_pendingConsoleStreamOutput.clear();

            handleResultRecord(&response);
            break;
        }
        default: {
            qDebug() << "UNKNOWN RESPONSE TYPE '" << c << "'. REST: " << from;
            break;
        }
    }
}

void GdbEngine::readGdbStandardError()
{
    QByteArray err = m_gdbProc->readAllStandardError();
    showMessage(_("UNEXPECTED GDB STDERR: " + err));
    if (err == "Undefined command: \"bb\".  Try \"help\".\n")
        return;
    if (err.startsWith("BFD: reopening"))
        return;
    qWarning() << "Unexpected GDB stderr:" << err;
}

void GdbEngine::readGdbStandardOutput()
{
    m_commandTimer.start(); // Restart timer.

    int newstart = 0;
    int scan = m_inbuffer.size();

    QByteArray out = m_gdbProc->readAllStandardOutput();
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
        QByteArray ba = QByteArray::fromRawData(m_inbuffer.constData() + start, end - start);
        handleResponse(ba);
        m_busy = false;
    }
    m_inbuffer.clear();
}

void GdbEngine::interruptInferior()
{
    CHECK_STATE(InferiorStopRequested);

    if (terminal()->sendInterrupt())
        return;

    if (usesExecInterrupt()) {
        postCommand("-exec-interrupt", Immediate);
    } else {
        showStatusMessage(tr("Stop requested..."), 5000);
        showMessage(_("TRYING TO INTERRUPT INFERIOR"));
        if (HostOsInfo::isWindowsHost() && !m_isQnxGdb) {
            QTC_ASSERT(state() == InferiorStopRequested, qDebug() << state(); notifyInferiorStopFailed());
            QTC_ASSERT(!m_signalOperation, notifyInferiorStopFailed());
            m_signalOperation = startParameters().device->signalOperation();
            QTC_ASSERT(m_signalOperation, notifyInferiorStopFailed());
            connect(m_signalOperation.data(), SIGNAL(finished(QString)),
                    SLOT(handleInterruptDeviceInferior(QString)));

            m_signalOperation->setDebuggerCommand(startParameters().debuggerCommand);
            m_signalOperation->interruptProcess(inferiorPid());
        } else {
            interruptInferior2();
        }
    }
}

void GdbEngine::handleInterruptDeviceInferior(const QString &error)
{
    if (error.isEmpty()) {
        showMessage(QLatin1String("Interrupted ") + QString::number(inferiorPid()));
        notifyInferiorStopOk();
    } else {
        showMessage(error, LogError);
        notifyInferiorStopFailed();
    }
    m_signalOperation->disconnect(this);
    m_signalOperation.clear();
}

void GdbEngine::interruptInferiorTemporarily()
{
    foreach (const DebuggerCommand &cmd, m_commandsToRunOnTemporaryBreak) {
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

void GdbEngine::runCommand(const DebuggerCommand &command)
{
    QByteArray cmd  = command.function + "({" + command.args + "})";
    postCommand("python theDumper." + cmd, command.flags, command.callback);
}

void GdbEngine::postCommand(const QByteArray &command, int flags,
                            DebuggerCommand::Callback callback)
{
    DebuggerCommand cmd;
    cmd.function = command;
    cmd.flags = flags;
    cmd.callback = callback;

    if (!stateAcceptsGdbCommands(state())) {
        PENDING_DEBUG(_("NO GDB PROCESS RUNNING, CMD IGNORED: " + cmd.function));
        showMessage(_("NO GDB PROCESS RUNNING, CMD IGNORED: %1 %2")
            .arg(_(cmd.function)).arg(state()));
        return;
    }

    if (cmd.flags & RebuildBreakpointModel) {
        ++m_pendingBreakpointRequests;
        PENDING_DEBUG("   BRWAKPOINT MODEL:" << cmd.function
                      << "INCREMENTS PENDING TO" << m_pendingBreakpointRequests);
    } else {
        PENDING_DEBUG("   OTHER (IN):" << cmd.function
                      << "LEAVES PENDING BREAKPOINT AT" << m_pendingBreakpointRequests);
    }

    if (!(cmd.flags & Discardable))
        ++m_nonDiscardableCount;

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
            showMessage(_("QUEUING COMMAND " + cmd.function));
            m_commandsToRunOnTemporaryBreak.append(cmd);
            if (state() == InferiorStopRequested) {
                if (cmd.flags & LosesChild)
                    notifyInferiorIll();
                showMessage(_("CHILD ALREADY BEING INTERRUPTED. STILL HOPING."));
                // Calling shutdown() here breaks all situations where two
                // NeedsStop commands are issued in quick succession.
            } else if (state() == InferiorRunOk) {
                showStatusMessage(tr("Stopping temporarily"), 1000);
                interruptInferiorTemporarily();
            } else {
                qDebug() << "ATTEMPTING TO QUEUE COMMAND "
                    << cmd.function << "IN INAPPROPRIATE STATE" << state();
            }
        }
    } else if (!cmd.function.isEmpty()) {
        flushCommand(cmd);
    }
}

void GdbEngine::flushQueuedCommands()
{
    showStatusMessage(tr("Processing queued commands"), 1000);
    while (!m_commandsToRunOnTemporaryBreak.isEmpty()) {
        DebuggerCommand cmd = m_commandsToRunOnTemporaryBreak.takeFirst();
        showMessage(_("RUNNING QUEUED COMMAND " + cmd.function));
        flushCommand(cmd);
    }
}

void GdbEngine::flushCommand(const DebuggerCommand &cmd0)
{
    if (!stateAcceptsGdbCommands(state())) {
        showMessage(_(cmd0.function), LogInput);
        showMessage(_("GDB PROCESS ACCEPTS NO CMD IN STATE %1 ").arg(state()));
        return;
    }

    QTC_ASSERT(m_gdbProc->state() == QProcess::Running, return);

    const int token = ++currentToken();

    DebuggerCommand cmd = cmd0;
    cmd.postTime = QTime::currentTime().msecsSinceStartOfDay();
    m_commandForToken[token] = cmd;
    if (cmd.flags & ConsoleCommand)
        cmd.function = "-interpreter-exec console \"" + cmd.function + '"';
    cmd.function = QByteArray::number(token) + cmd.function;
    showMessage(_(cmd.function), LogInput);

    if (m_scheduledTestResponses.contains(token)) {
        // Fake response for test cases.
        QByteArray buffer = m_scheduledTestResponses.value(token);
        buffer.replace("@TOKEN@", QByteArray::number(token));
        m_scheduledTestResponses.remove(token);
        showMessage(_("FAKING TEST RESPONSE (TOKEN: %2, RESPONSE: %3)")
            .arg(token).arg(_(buffer)));
        QMetaObject::invokeMethod(this, "handleResponse",
            Q_ARG(QByteArray, buffer));
    } else {
        write(cmd.function + "\r\n");

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
    int time = action(GdbWatchdogTimeout)->value().toInt();
    return 1000 * qMax(40, time);
}

void GdbEngine::commandTimeout()
{
    QList<int> keys = m_commandForToken.keys();
    Utils::sort(keys);
    bool killIt = false;
    foreach (int key, keys) {
        const DebuggerCommand &cmd = m_commandForToken.value(key);
        if (!(cmd.flags & NonCriticalResponse))
            killIt = true;
        showMessage(_(QByteArray::number(key) + ": " + cmd.function));
    }
    if (killIt) {
        QStringList commands;
        foreach (const DebuggerCommand &cmd, m_commandForToken)
            commands << QString(_("\"%1\"")).arg(
                            QString::fromLatin1(cmd.function));
        showMessage(_("TIMED OUT WAITING FOR GDB REPLY. "
                      "COMMANDS STILL IN PROGRESS: ") + commands.join(_(", ")));
        int timeOut = m_commandTimer.interval();
        //m_commandTimer.stop();
        const QString msg = tr("The gdb process has not responded "
            "to a command within %n second(s). This could mean it is stuck "
            "in an endless loop or taking longer than expected to perform "
            "the operation.\nYou can choose between waiting "
            "longer or aborting debugging.", 0, timeOut / 1000);
        QMessageBox *mb = showMessageBox(QMessageBox::Critical,
            tr("GDB not responding"), msg,
            QMessageBox::Ok | QMessageBox::Cancel);
        mb->button(QMessageBox::Cancel)->setText(tr("Give GDB more time"));
        mb->button(QMessageBox::Ok)->setText(tr("Stop debugging"));
        if (mb->exec() == QMessageBox::Ok) {
            showMessage(_("KILLING DEBUGGER AS REQUESTED BY USER"));
            // This is an undefined state, so we just pull the emergency brake.
            m_gdbProc->kill();
            notifyEngineShutdownFailed();
        } else {
            showMessage(_("CONTINUE DEBUGGER AS REQUESTED BY USER"));
        }
    } else {
        showMessage(_("\nNON-CRITICAL TIMEOUT\n"));
    }
}

void GdbEngine::handleResultRecord(DebuggerResponse *response)
{
    //qDebug() << "TOKEN:" << response->token
    //    << " ACCEPTABLE:" << m_oldestAcceptableToken;
    //qDebug() << "\nRESULT" << response->token << response->toString();

    int token = response->token;
    if (token == -1)
        return;

    if (!m_commandForToken.contains(token)) {
        // In theory this should not happen (rather the error should be
        // reported in the "first" response to the command) in practice it
        // does. We try to handle a few situations we are aware of gracefully.
        // Ideally, this code should not be present at all.
        showMessage(_("COOKIE FOR TOKEN %1 ALREADY EATEN (%2). "
                      "TWO RESPONSES FOR ONE COMMAND?").arg(token).
                    arg(QString::fromLatin1(stateName(state()))));
        if (response->resultClass == ResultError) {
            QByteArray msg = response->data["msg"].data();
            if (msg == "Cannot find new threads: generic error") {
                // Handle a case known to occur on Linux/gdb 6.8 when debugging moc
                // with helpers enabled. In this case we get a second response with
                // msg="Cannot find new threads: generic error"
                showMessage(_("APPLYING WORKAROUND #1"));
                AsynchronousMessageBox::critical(
                    tr("Executable failed"), QString::fromLocal8Bit(msg));
                showStatusMessage(tr("Process failed to start"));
                //shutdown();
                notifyInferiorIll();
            } else if (msg == "\"finish\" not meaningful in the outermost frame.") {
                // Handle a case known to appear on GDB 6.4 symbianelf when
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
                //Core::AsynchronousMessageBox::critical(
                //    tr("Executable failed"), QString::fromLocal8Bit(msg));
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
                showMessage(_("APPLYING WORKAROUND #5"));
                AsynchronousMessageBox::critical(
                    tr("Setting breakpoints failed"), QString::fromLocal8Bit(msg));
                QTC_CHECK(state() == InferiorRunOk);
                notifyInferiorSpontaneousStop();
                notifyEngineIll();
            } else if (isGdbConnectionError(msg)) {
                notifyInferiorExited();
            } else {
                // Windows: Some DLL or some function not found. Report
                // the exception now in a box.
                if (msg.startsWith("During startup program exited with"))
                    notifyInferiorExited();
                QString logMsg;
                if (!m_lastWinException.isEmpty())
                    logMsg = m_lastWinException + QLatin1Char('\n');
                logMsg += QString::fromLocal8Bit(msg);
                AsynchronousMessageBox::critical(tr("Executable Failed"), logMsg);
                showStatusMessage(tr("Executable failed: %1").arg(logMsg));
            }
        }
        return;
    }

    DebuggerCommand cmd = m_commandForToken.take(token);
    if (boolSetting(LogTimeStamps)) {
        showMessage(_("Response time: %1: %2 s")
            .arg(_(cmd.function))
            .arg(QTime::fromMSecsSinceStartOfDay(cmd.postTime).msecsTo(QTime::currentTime()) / 1000.),
            LogTime);
    }

    if (response->token < m_oldestAcceptableToken && (cmd.flags & Discardable)) {
        //showMessage(_("### SKIPPING OLD RESULT") + response.toString());
        return;
    }

    bool isExpectedResult =
           (response->resultClass == ResultError) // Can always happen.
        || (response->resultClass == ResultRunning && (cmd.flags & RunRequest))
        || (response->resultClass == ResultExit && (cmd.flags & ExitRequest))
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
        const DebuggerStartParameters &sp = startParameters();
        Abi abi = sp.toolChainAbi;
        if (abi.os() == Abi::WindowsOS
            && cmd.function.startsWith("attach")
            && (sp.startMode == AttachExternal || sp.useTerminal))
        {
            // Ignore spurious 'running' responses to 'attach'.
        } else {
            QByteArray rsp = DebuggerResponse::stringFromResultClass(response->resultClass);
            rsp = "UNEXPECTED RESPONSE '" + rsp + "' TO COMMAND '" + cmd.function + "'";
            qWarning() << rsp << " AT " __FILE__ ":" STRINGIFY(__LINE__);
            showMessage(_(rsp));
        }
    }

    if (!(cmd.flags & Discardable))
        --m_nonDiscardableCount;

    if (cmd.callback)
        cmd.callback(*response);

    if (cmd.flags & RebuildBreakpointModel) {
        --m_pendingBreakpointRequests;
        PENDING_DEBUG("   BREAKPOINT" << cmd.function);
        if (m_pendingBreakpointRequests <= 0) {
            PENDING_DEBUG("\n\n ... AND TRIGGERS BREAKPOINT MODEL UPDATE\n");
            attemptBreakpointSynchronization();
        }
    } else {
        PENDING_DEBUG("   OTHER (OUT):" << cmd.function
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
    if (m_commandsDoneCallback && m_commandForToken.isEmpty()) {
        showMessage(_("ALL COMMANDS DONE; INVOKING CALLBACK"));
        CommandsDoneCallback cont = m_commandsDoneCallback;
        m_commandsDoneCallback = 0;
        if (response->resultClass != ResultRunning) //only start if the thing is not already running
            (this->*cont)();
    } else {
        PENDING_DEBUG("MISSING TOKENS: " << m_commandForToken.keys());
    }

    if (m_commandForToken.isEmpty())
        m_commandTimer.stop();
}

bool GdbEngine::acceptsDebuggerCommands() const
{
    return true;
    return state() == InferiorStopOk
        || state() == InferiorUnrunnable;
}

void GdbEngine::executeDebuggerCommand(const QString &command, DebuggerLanguages languages)
{
    if (!(languages & CppLanguage))
        return;
    QTC_CHECK(acceptsDebuggerCommands());
    DebuggerCommand cmd;
    cmd.function = command.toLatin1();
    flushCommand(cmd);
}

// This is triggered when switching snapshots.
void GdbEngine::updateAll()
{
    //PENDING_DEBUG("UPDATING ALL\n");
    QTC_CHECK(state() == InferiorUnrunnable || state() == InferiorStopOk);
    reloadModulesInternal();
    DebuggerCommand cmd = stackCommand(action(MaximalStackDepth)->value().toInt());
    cmd.flags = NoFlags;
    cmd.callback = [this](const DebuggerResponse &r) { handleStackListFrames(r, false); };
    runCommand(cmd);
    stackHandler()->setCurrentIndex(0);
    postCommand("-thread-info", NoFlags, CB(handleThreadInfo));
    reloadRegisters();
    updateLocals();
}

void GdbEngine::handleQuerySources(const DebuggerResponse &response)
{
    m_sourcesListUpdating = false;
    if (response.resultClass == ResultDone) {
        QMap<QString, QString> oldShortToFull = m_shortToFullName;
        m_shortToFullName.clear();
        m_fullToShortName.clear();
        // "^done,files=[{file="../../../../bin/dumper/dumper.cpp",
        // fullname="/data5/dev/ide/main/bin/dumper/dumper.cpp"},
        GdbMi files = response.data["files"];
        foreach (const GdbMi &item, files.children()) {
            GdbMi fileName = item["file"];
            if (fileName.data().endsWith("<built-in>"))
                continue;
            GdbMi fullName = item["fullname"];
            QString file = QString::fromLocal8Bit(fileName.data());
            QString full;
            if (fullName.isValid()) {
                full = cleanupFullName(QString::fromLocal8Bit(fullName.data()));
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
        QString out = tr("Cannot jump. Stopped");
        QByteArray msg = response.data["msg"].data();
        if (!msg.isEmpty())
            out += QString::fromLatin1(". " + msg);
        showStatusMessage(out);
        notifyInferiorRunFailed();
    } else if (response.resultClass == ResultDone) {
        // This happens on old gdb. Trigger the effect of a '*stopped'.
        showStatusMessage(tr("Jumped. Stopped"));
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
        showStatusMessage(tr("Target line hit. Stopped"));
        notifyInferiorRunOk();
    }
}

static bool isExitedReason(const QByteArray &reason)
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
    if (!data.isValid() && m_terminalTrap && Abi::hostAbi().os() == Abi::WindowsOS) {
        m_terminalTrap = false;
        showMessage(_("IGNORING TERMINAL SIGTRAP"), LogMisc);
        return;
    }

    if (isDying()) {
        notifyInferiorStopOk();
        return;
    }

    GdbMi threads = data["stopped-thread"];
    threadsHandler()->notifyStopped(threads.data());

    const QByteArray reason = data["reason"].data();
    const GdbMi frame = data["frame"];
    const QByteArray func = frame["from"].data();

    if (isExitedReason(reason)) {
        //   // The user triggered a stop, but meanwhile the app simply exited ...
        //    QTC_ASSERT(state() == InferiorStopRequested
        //            /*|| state() == InferiorStopRequested_Kill*/,
        //               qDebug() << state());
        QString msg;
        if (reason == "exited") {
            msg = tr("Application exited with exit code %1")
                .arg(_(data["exit-code"].toString()));
        } else if (reason == "exited-signalled" || reason == "signal-received") {
            msg = tr("Application exited after receiving signal %1")
                .arg(_(data["signal-name"].toString()));
        } else {
            msg = tr("Application exited normally");
        }
        showStatusMessage(msg);
        notifyInferiorExited();
        return;
    }

    // Ignore signals from the process stub.
    if (startParameters().useTerminal
            && data["reason"].data() == "signal-received"
            && data["signal-name"].data() == "SIGSTOP"
            && (func.endsWith("/ld-linux.so.2")
                || func.endsWith("/ld-linux-x86-64.so.2")))
    {
        showMessage(_("INTERNAL CONTINUE AFTER SIGSTOP FROM STUB"), LogMisc);
        notifyInferiorSpontaneousStop();
        continueInferiorInternal();
        return;
    }

    bool gotoHandleStop1 = true;
    if (!m_fullStartDone) {
        m_fullStartDone = true;
        postCommand("sharedlibrary .*");
        postCommand("p 3", NoFlags, [this, data](const DebuggerResponse &) { handleStop1(data); });
        gotoHandleStop1 = false;
    }

    BreakpointResponseId rid(data["bkptno"].data());
    int lineNumber = 0;
    QString fullName;
    QByteArray function;
    if (frame.isValid()) {
        const GdbMi lineNumberG = frame["line"];
        function = frame["func"].data();
        if (lineNumberG.isValid()) {
            lineNumber = lineNumberG.toInt();
            fullName = cleanupFullName(QString::fromLocal8Bit(frame["fullname"].data()));
            if (fullName.isEmpty())
                fullName = QString::fromLocal8Bit(frame["file"].data());
        } // found line number
    } else {
        showMessage(_("INVALID STOPPED REASON"), LogWarning);
    }

    if (rid.isValid() && frame.isValid() && !isQFatalBreakpoint(rid)) {
        // Use opportunity to update the breakpoint marker position.
        //qDebug() << " PROBLEM: " << m_qmlBreakpointNumbers << rid
        //    << isQmlStepBreakpoint1(rid)
        //    << isQmlStepBreakpoint2(rid)
        Breakpoint bp = breakHandler()->findBreakpointByResponseId(rid);
        const BreakpointResponse &response = bp.response();
        QString fileName = response.fileName;
        if (fileName.isEmpty())
            fileName = bp.fileName();
        if (fileName.isEmpty())
            fileName = fullName;
        if (!fileName.isEmpty())
            bp.setMarkerFileAndLine(fileName, lineNumber);
    }

    //qDebug() << "BP " << rid << data.toString();
    // Quickly set the location marker.
    if (lineNumber && !boolSetting(OperateByInstruction)
            && QFileInfo::exists(fullName)
            && !isQFatalBreakpoint(rid)
            && function != "qt_v4TriggeredBreakpointHook")
        gotoLocation(Location(fullName, lineNumber));

    if (!m_commandsToRunOnTemporaryBreak.isEmpty()) {
        CHECK_STATE(InferiorStopRequested);
        notifyInferiorStopOk();
        flushQueuedCommands();
        if (state() == InferiorStopOk) {
            QTC_CHECK(m_commandsDoneCallback == 0);
            m_commandsDoneCallback = &GdbEngine::autoContinueInferior;
        } else {
            CHECK_STATE(InferiorShutdownRequested);
        }
        return;
    }

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
        notifyInferiorSpontaneousStop();
    } else if (state() == InferiorStopOk) {
        // That's expected.
    } else if (state() == InferiorStopRequested) {
        notifyInferiorStopOk();
    } else if (state() == EngineRunRequested) {
        // This is gdb 7+'s initial *stopped in response to attach that
        // appears before the ^done is seen.
        notifyEngineRunAndInferiorStopOk();
        const DebuggerStartParameters &sp = startParameters();
        if (sp.useTerminal)
            continueInferiorInternal();
        return;
    } else {
        QTC_CHECK(false);
    }

    CHECK_STATE(InferiorStopOk);

    if (gotoHandleStop1)
        handleStop1(data);
}

static QByteArray stopSignal(const Abi &abi)
{
    return (abi.os() == Abi::WindowsOS) ? QByteArray("SIGTRAP") : QByteArray("SIGINT");
}

void GdbEngine::handleStop1(const GdbMi &data)
{
    CHECK_STATE(InferiorStopOk);
    QTC_ASSERT(!isDying(), return);
    const GdbMi frame = data["frame"];
    const QByteArray reason = data["reason"].data();

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
    if (boolSetting(SkipKnownFrames)) {
        if (reason == "end-stepping-range" || reason == "function-finished") {
            //showMessage(frame.toString());
            QString funcName = _(frame["func"].data());
            QString fileName = QString::fromLocal8Bit(frame["file"].data());
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
    GdbMi resultVar = data["gdb-result-var"];
    if (resultVar.isValid())
        m_resultVarName = resultVar.data();
    else
        m_resultVarName.clear();

    if (!m_systemDumpersLoaded) {
        m_systemDumpersLoaded = true;
        if (m_gdbVersion >= 70400 && boolSetting(LoadGdbDumpers))
            postCommand("importPlainDumpers on");
        else
            postCommand("importPlainDumpers off");
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

    const QByteArray reason = data["reason"].data();
    const DebuggerStartParameters &sp = startParameters();

    bool isStopperThread = false;

    if (sp.toolChainAbi.os() == Abi::WindowsOS
            && sp.useTerminal
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
        const BreakpointResponseId rid(wpt["number"].data());
        const Breakpoint bp = breakHandler()->findBreakpointByResponseId(rid);
        const quint64 bpAddress = wpt["exp"].data().mid(1).toULongLong(0, 0);
        QString msg;
        if (bp.type() == WatchpointAtExpression)
            msg = bp.msgWatchpointByExpressionTriggered(rid.majorPart(), bp.expression());
        if (bp.type() == WatchpointAtAddress)
            msg = bp.msgWatchpointByAddressTriggered(rid.majorPart(), bpAddress);
        GdbMi value = data["value"];
        GdbMi oldValue = value["old"];
        GdbMi newValue = value["new"];
        if (oldValue.isValid() && newValue.isValid()) {
            msg += QLatin1Char(' ');
            msg += tr("Value changed from %1 to %2.")
                .arg(_(oldValue.data())).arg(_(newValue.data()));
        }
        showStatusMessage(msg);
    } else if (reason == "breakpoint-hit") {
        GdbMi gNumber = data["bkptno"]; // 'number' or 'bkptno'?
        if (!gNumber.isValid())
            gNumber = data["number"];
        const BreakpointResponseId rid(gNumber.data());
        const QByteArray threadId = data["thread-id"].data();
        const Breakpoint bp = breakHandler()->findBreakpointByResponseId(rid);
        showStatusMessage(bp.msgBreakpointTriggered(rid.majorPart(), _(threadId)));
        m_currentThread = threadId;
    } else {
        QString reasontr = msgStopped(_(reason));
        if (reason == "signal-received") {
            QByteArray name = data["signal-name"].data();
            QByteArray meaning = data["signal-meaning"].data();
            // Ignore these as they are showing up regularly when
            // stopping debugging.
            if (name == stopSignal(sp.toolChainAbi) || sp.expectedSignals.contains(name)) {
                showMessage(_(name + " CONSIDERED HARMLESS. CONTINUING."));
            } else {
                showMessage(_("HANDLING SIGNAL " + name));
                if (boolSetting(UseMessageBoxForSignals) && !isStopperThread)
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

    // Let the event loop run before deciding whether to update the stack.
    m_stackNeeded = true; // setTokenBarrier() might reset this.
    QTimer::singleShot(0, this, SLOT(handleStop2()));
}

void GdbEngine::handleStop2()
{
    // We are already continuing.
    if (!m_stackNeeded)
        return;

    // This is only available in gdb 7.1+.
    postCommand("-thread-info", Discardable, CB(handleThreadInfo));
}

void GdbEngine::handleInfoProc(const DebuggerResponse &response)
{
    if (response.resultClass == ResultDone) {
        static QRegExp re(_("\\bprocess ([0-9]+)\n"));
        QTC_ASSERT(re.isValid(), return);
        if (re.indexIn(_(response.consoleStreamOutput)) != -1)
            maybeHandleInferiorPidChanged(re.cap(1));
    }
}

void GdbEngine::handleShowVersion(const DebuggerResponse &response)
{
    showMessage(_("PARSING VERSION: " + response.toString()));
    if (response.resultClass == ResultDone) {
        bool isMacGdb = false;
        int gdbBuildVersion = -1;
        m_gdbVersion = 100;
        m_isQnxGdb = false;
        QString msg = QString::fromLocal8Bit(response.consoleStreamOutput);
        extractGdbVersion(msg,
              &m_gdbVersion, &gdbBuildVersion, &isMacGdb, &m_isQnxGdb);

        // On Mac, FSF GDB does not work sufficiently well,
        // and on Linux and Windows we require at least 7.4.1,
        // on Android 7.3.1.
        bool isSupported = m_gdbVersion >= 70300;
        if (isSupported)
            showMessage(_("SUPPORTED GDB VERSION ") + msg);
        else
            showMessage(_("UNSUPPORTED GDB VERSION ") + msg);

        showMessage(_("USING GDB VERSION: %1, BUILD: %2%3").arg(m_gdbVersion)
            .arg(gdbBuildVersion).arg(_(isMacGdb ? " (APPLE)" : "")));

        if (usesExecInterrupt())
            postCommand("set target-async on", ConsoleCommand);
        else
            postCommand("set target-async off", ConsoleCommand);

        if (startParameters().multiProcess)
            postCommand("set detach-on-fork off", ConsoleCommand);
        //postCommand("set build-id-verbose 2", ConsoleCommand);
    }
}

void GdbEngine::handleListFeatures(const DebuggerResponse &response)
{
    showMessage(_("FEATURES: " + response.toString()));
}

void GdbEngine::handlePythonSetup(const DebuggerResponse &response)
{
    CHECK_STATE(EngineSetupRequested);
    if (response.resultClass == ResultDone) {
        GdbMi data;
        data.fromStringMultiple(response.consoleStreamOutput);
        watchHandler()->addDumpers(data["dumpers"]);
        loadInitScript();
        CHECK_STATE(EngineSetupRequested);
        showMessage(_("ENGINE SUCCESSFULLY STARTED"));
        notifyEngineSetupOk();
    } else {
        QByteArray msg = response.data["msg"].data();
        if (msg.contains("Python scripting is not supported in this copy of GDB.")) {
            QString out1 = _("The selected build of GDB does not support Python scripting.");
            QString out2 = _("It cannot be used in Qt Creator.");
            showStatusMessage(out1 + QLatin1Char(' ') + out2);
            AsynchronousMessageBox::critical(tr("Execution Error"), out1 + _("<br>") + out2);
        }
        notifyEngineSetupFailed();
    }
}

void GdbEngine::showExecutionError(const QString &message)
{
    AsynchronousMessageBox::critical(tr("Execution Error"),
       tr("Cannot continue debugged process:") + QLatin1Char('\n') + message);
}

void GdbEngine::handleExecuteContinue(const DebuggerResponse &response)
{
    CHECK_STATE(InferiorRunRequested);
    if (response.resultClass == ResultRunning) {
        // All is fine. Waiting for a *running.
        notifyInferiorRunOk(); // Only needed for gdb < 7.0.
        return;
    }
    QByteArray msg = response.data["msg"].data();
    if (msg.startsWith("Cannot find bounds of current function")) {
        notifyInferiorRunFailed();
        if (isDying())
            return;
        if (!m_commandsToRunOnTemporaryBreak.isEmpty())
            flushQueuedCommands();
        CHECK_STATE(InferiorStopOk);
        showStatusMessage(tr("Stopped."), 5000);
        reloadStack();
    } else if (msg.startsWith("Cannot access memory at address")) {
        // Happens on single step on ARM prolog and epilogs.
    } else if (msg.startsWith("\"finish\" not meaningful in the outermost frame")) {
        notifyInferiorRunFailed();
        if (isDying())
            return;
        CHECK_STATE(InferiorStopOk);
        // FIXME: Fix translation in master.
        showStatusMessage(QString::fromLocal8Bit(msg), 5000);
        gotoLocation(stackHandler()->currentFrame());
    } else if (msg.startsWith("Cannot execute this command while the selected thread is running.")) {
        showExecutionError(QString::fromLocal8Bit(msg));
        notifyInferiorRunFailed() ;
    } else {
        showExecutionError(QString::fromLocal8Bit(msg));
        notifyInferiorIll();
    }
}

QString GdbEngine::fullName(const QString &fileName)
{
    if (fileName.isEmpty())
        return QString();
    QTC_ASSERT(!m_sourcesListUpdating, /* */);
    return m_shortToFullName.value(fileName, QString());
}

QString GdbEngine::cleanupFullName(const QString &fileName)
{
    QString cleanFilePath = fileName;

    // Gdb running on windows often delivers "fullnames" which
    // (a) have no drive letter and (b) are not normalized.
    if (Abi::hostAbi().os() == Abi::WindowsOS) {
        QTC_ASSERT(!fileName.isEmpty(), return QString());
        QFileInfo fi(fileName);
        if (fi.isReadable())
            cleanFilePath = QDir::cleanPath(fi.absoluteFilePath());
    }

    if (!boolSetting(AutoEnrichParameters))
        return cleanFilePath;

    const QString sysroot = startParameters().sysRoot;
    if (QFileInfo(cleanFilePath).isReadable())
        return cleanFilePath;
    if (!sysroot.isEmpty() && fileName.startsWith(QLatin1Char('/'))) {
        cleanFilePath = sysroot + fileName;
        if (QFileInfo(cleanFilePath).isReadable())
            return cleanFilePath;
    }
    if (m_baseNameToFullName.isEmpty()) {
        QString debugSource = sysroot + QLatin1String("/usr/src/debug");
        if (QFileInfo(debugSource).isDir()) {
            QDirIterator it(debugSource, QDirIterator::Subdirectories);
            while (it.hasNext()) {
                it.next();
                QString name = it.fileName();
                if (!name.startsWith(QLatin1Char('.'))) {
                    QString path = it.filePath();
                    m_baseNameToFullName.insert(name, path);
                }
            }
        }
    }

    cleanFilePath.clear();
    const QString base = FileName::fromString(fileName).fileName();

    QMap<QString, QString>::const_iterator jt = m_baseNameToFullName.constFind(base);
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
    m_commandsToRunOnTemporaryBreak.clear();
    switch (startParameters().closeMode) {
        case KillAtClose:
        case KillAndExitMonitorAtClose:
            postCommand("kill", NeedsStop | LosesChild, CB(handleInferiorShutdown));
            return;
        case DetachAtClose:
            postCommand("detach", NeedsStop | LosesChild, CB(handleInferiorShutdown));
            return;
    }
    QTC_ASSERT(false, notifyInferiorShutdownFailed());
}

void GdbEngine::handleInferiorShutdown(const DebuggerResponse &response)
{
    CHECK_STATE(InferiorShutdownRequested);
    if (response.resultClass == ResultDone) {
        notifyInferiorShutdownOk();
        return;
    }
    QByteArray ba = response.data["msg"].data();
    if (ba.contains(": No such file or directory.")) {
        // This happens when someone removed the binary behind our back.
        // It is not really an error from a user's point of view.
        showMessage(_("NOTE: " + ba));
        notifyInferiorShutdownOk();
        return;
    }
    AsynchronousMessageBox::critical(
        tr("Failed to shut down application"),
        msgInferiorStopFailed(QString::fromLocal8Bit(ba)));
    notifyInferiorShutdownFailed();
}

void GdbEngine::notifyAdapterShutdownFailed()
{
    showMessage(_("ADAPTER SHUTDOWN FAILED"));
    CHECK_STATE(EngineShutdownRequested);
    notifyEngineShutdownFailed();
}

void GdbEngine::notifyAdapterShutdownOk()
{
    CHECK_STATE(EngineShutdownRequested);
    showMessage(_("INITIATE GDBENGINE SHUTDOWN IN STATE %1, PROC: %2")
        .arg(lastGoodState()).arg(m_gdbProc->state()));
    m_commandsDoneCallback = 0;
    switch (m_gdbProc->state()) {
    case QProcess::Running: {
        if (startParameters().closeMode == KillAndExitMonitorAtClose)
            postCommand("monitor exit");
        DebuggerCommand cmd("exitGdb");
        cmd.flags = GdbEngine::ExitRequest;
        cmd.callback = CB(handleGdbExit);
        runCommand(cmd);
        break;
    }
    case QProcess::NotRunning:
        // Cannot find executable.
        notifyEngineShutdownOk();
        break;
    case QProcess::Starting:
        showMessage(_("GDB NOT REALLY RUNNING; KILLING IT"));
        m_gdbProc->kill();
        notifyEngineShutdownFailed();
        break;
    }
}

void GdbEngine::handleGdbExit(const DebuggerResponse &response)
{
    if (response.resultClass == ResultExit) {
        showMessage(_("GDB CLAIMS EXIT; WAITING"));
        // Don't set state here, this will be handled in handleGdbFinished()
        //notifyEngineShutdownOk();
    } else {
        QString msg = msgGdbStopFailed(
            QString::fromLocal8Bit(response.data["msg"].data()));
        qDebug() << (_("GDB WON'T EXIT (%1); KILLING IT").arg(msg));
        showMessage(_("GDB WON'T EXIT (%1); KILLING IT").arg(msg));
        m_gdbProc->kill();
        notifyEngineShutdownFailed();
    }
}

void GdbEngine::detachDebugger()
{
    CHECK_STATE(InferiorStopOk);
    QTC_ASSERT(startMode() != AttachCore, qDebug() << startMode());
    postCommand("detach", GdbEngine::ExitRequest, CB(handleDetach));
}

void GdbEngine::handleDetach(const DebuggerResponse &response)
{
    Q_UNUSED(response);
    CHECK_STATE(InferiorStopOk);
    notifyInferiorExited();
}

void GdbEngine::handleThreadGroupCreated(const GdbMi &result)
{
    Q_UNUSED(result);
//    QByteArray id = result["id"].data();
//    QByteArray pid = result["pid"].data();
//    Q_UNUSED(id);
//    Q_UNUSED(pid);
}

void GdbEngine::handleThreadGroupExited(const GdbMi &result)
{
    Q_UNUSED(result);
//    QByteArray id = result["id"].data();
//    Q_UNUSED(id);
}

int GdbEngine::currentFrame() const
{
    return stackHandler()->currentIndex();
}

static QString msgNoGdbBinaryForToolChain(const Abi &tc)
{
    return GdbEngine::tr("There is no GDB binary available for binaries in format \"%1\"")
        .arg(tc.toString());
}

bool GdbEngine::hasCapability(unsigned cap) const
{
    if (cap & (ReverseSteppingCapability
        | AutoDerefPointersCapability
        | DisassemblerCapability
        | RegisterCapability
        | ShowMemoryCapability
        | JumpToLineCapability
        | ReloadModuleCapability
        | ReloadModuleSymbolsCapability
        | BreakOnThrowAndCatchCapability
        | BreakConditionCapability
        | TracePointCapability
        | ReturnFromFunctionCapability
        | CreateFullBacktraceCapability
        | WatchpointByAddressCapability
        | WatchpointByExpressionCapability
        | AddWatcherCapability
        | WatchWidgetsCapability
        | ShowModuleSymbolsCapability
        | ShowModuleSectionsCapability
        | CatchCapability
        | OperateByInstructionCapability
        | RunToLineCapability
        | WatchComplexExpressionsCapability
        | MemoryAddressCapability
        | AdditionalQmlStackCapability
        | NativeMixedCapability
        | ResetInferiorCapability))
        return true;

    if (startParameters().startMode == AttachCore)
        return false;

    // FIXME: Remove in case we have gdb 7.x on Mac.
    if (startParameters().toolChainAbi.os() == Abi::MacOS)
        return false;

    return cap == SnapshotCapability;
}


void GdbEngine::continueInferiorInternal()
{
    CHECK_STATE(InferiorStopOk);
    notifyInferiorRunRequested();
    showStatusMessage(tr("Running requested..."), 5000);
    CHECK_STATE(InferiorRunRequested);
    postCommand("-exec-continue", RunRequest, CB(handleExecuteContinue));
}

void GdbEngine::autoContinueInferior()
{
    resetLocation();
    continueInferiorInternal();
    showStatusMessage(tr("Continuing after temporary stop..."), 1000);
}

void GdbEngine::continueInferior()
{
    CHECK_STATE(InferiorStopOk);
    setTokenBarrier();
    continueInferiorInternal();
}

void GdbEngine::executeStep()
{
    CHECK_STATE(InferiorStopOk);
    setTokenBarrier();
    notifyInferiorRunRequested();
    showStatusMessage(tr("Step requested..."), 5000);
    if (isNativeMixedActive()) {
        runCommand("prepareQmlStep");
        postCommand("-exec-continue", RunRequest, CB(handleExecuteContinue));
        return;
    }
    if (isReverseDebugging())
        postCommand("reverse-step", RunRequest, CB(handleExecuteStep));
    else
        postCommand("-exec-step", RunRequest, CB(handleExecuteStep));
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
    QByteArray msg = response.data["msg"].data();
    if (msg.startsWith("Cannot find bounds of current function")
            || msg.contains("Error accessing memory address")
            || msg.startsWith("Cannot access memory at address")) {
        // On S40: "40^error,msg="Warning:\nCannot insert breakpoint -39.\n"
        //" Error accessing memory address 0x11673fc: Input/output error.\n"
        notifyInferiorRunFailed();
        if (isDying())
            return;
        if (!m_commandsToRunOnTemporaryBreak.isEmpty())
            flushQueuedCommands();
        executeStepI(); // Fall back to instruction-wise stepping.
    } else if (msg.startsWith("Cannot execute this command while the selected thread is running.")) {
        showExecutionError(QString::fromLocal8Bit(msg));
        notifyInferiorRunFailed();
    } else if (msg.startsWith("warning: SuspendThread failed")) {
        // On Win: would lead to "PC register is not available" or "\312"
        continueInferiorInternal();
    } else {
        showExecutionError(QString::fromLocal8Bit(msg));
        notifyInferiorIll();
    }
}

void GdbEngine::executeStepI()
{
    CHECK_STATE(InferiorStopOk);
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
    CHECK_STATE(InferiorStopOk);
    postCommand("-stack-select-frame 0", Discardable);
    setTokenBarrier();
    notifyInferiorRunRequested();
    showStatusMessage(tr("Finish function requested..."), 5000);
    postCommand("-exec-finish", RunRequest, CB(handleExecuteContinue));
}

void GdbEngine::executeNext()
{
    CHECK_STATE(InferiorStopOk);
    setTokenBarrier();
    notifyInferiorRunRequested();
    showStatusMessage(tr("Step next requested..."), 5000);
    if (isNativeMixedActive()) {
        runCommand("prepareQmlStep");
        postCommand("-exec-continue", RunRequest, CB(handleExecuteContinue));
        return;
    }
    if (isReverseDebugging()) {
        postCommand("reverse-next", RunRequest, CB(handleExecuteNext));
    } else {
        scheduleTestResponse(TestNoBoundsOfCurrentFunction,
            "@TOKEN@^error,msg=\"Warning:\\nCannot insert breakpoint -39.\\n"
            " Error accessing memory address 0x11673fc: Input/output error.\\n\"");
        postCommand("-exec-next", RunRequest, CB(handleExecuteNext));
    }
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
    QByteArray msg = response.data["msg"].data();
    if (msg.startsWith("Cannot find bounds of current function")
            || msg.contains("Error accessing memory address ")) {
        if (!m_commandsToRunOnTemporaryBreak.isEmpty())
            flushQueuedCommands();
        notifyInferiorRunFailed();
        if (!isDying())
            executeNextI(); // Fall back to instruction-wise stepping.
    } else if (msg.startsWith("Cannot execute this command while the selected thread is running.")) {
        showExecutionError(QString::fromLocal8Bit(msg));
        notifyInferiorRunFailed();
    } else {
        AsynchronousMessageBox::critical(tr("Execution Error"),
           tr("Cannot continue debugged process:") + QLatin1Char('\n') + QString::fromLocal8Bit(msg));
        notifyInferiorIll();
    }
}

void GdbEngine::executeNextI()
{
    CHECK_STATE(InferiorStopOk);
    setTokenBarrier();
    notifyInferiorRunRequested();
    showStatusMessage(tr("Step next instruction requested..."), 5000);
    if (isReverseDebugging())
        postCommand("reverse-nexti", RunRequest, CB(handleExecuteContinue));
    else
        postCommand("-exec-next-instruction", RunRequest, CB(handleExecuteContinue));
}

static QByteArray addressSpec(quint64 address)
{
    return "*0x" + QByteArray::number(address, 16);
}

void GdbEngine::executeRunToLine(const ContextData &data)
{
    CHECK_STATE(InferiorStopOk);
    setTokenBarrier();
    resetLocation();
    notifyInferiorRunRequested();
    showStatusMessage(tr("Run to line %1 requested...").arg(data.lineNumber), 5000);
#if 1
    QByteArray loc;
    if (data.address)
        loc = addressSpec(data.address);
    else
        loc = '"' + breakLocation(data.fileName).toLocal8Bit() + '"' + ':'
           + QByteArray::number(data.lineNumber);
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
    CHECK_STATE(InferiorStopOk);
    setTokenBarrier();
    resetLocation();
    postCommand("-break-insert -t " + functionName.toLatin1());
    showStatusMessage(tr("Run to function %1 requested...").arg(functionName), 5000);
    continueInferiorInternal();
}

void GdbEngine::executeJumpToLine(const ContextData &data)
{
    CHECK_STATE(InferiorStopOk);
    QByteArray loc;
    if (data.address)
        loc = addressSpec(data.address);
    else
        loc = '"' + breakLocation(data.fileName).toLocal8Bit() + '"' + ':'
        + QByteArray::number(data.lineNumber);
    postCommand("tbreak " + loc);
    notifyInferiorRunRequested();
    postCommand("jump " + loc, RunRequest, CB(handleExecuteJumpToLine));
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
    showStatusMessage(tr("Immediate return from function requested..."), 5000);
    postCommand("-exec-finish", RunRequest, CB(handleExecuteReturn));
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
    QHashIterator<int, DebuggerCommand> it(m_commandForToken);
    while (it.hasNext()) {
        it.next();
        if (!(it.value().flags & Discardable)) {
            qDebug() << "TOKEN: " << it.key() << "CMD:" << it.value().function
                << " FLAGS:" << it.value().flags;
            good = false;
        }
    }
    QTC_ASSERT(good, return);
    PENDING_DEBUG("\n--- token barrier ---\n");
    showMessage(_("--- token barrier ---"), LogMiscInput);
    if (boolSetting(LogTimeStamps))
        showMessage(LogWindow::logTimeStamp(), LogMiscInput);
    m_oldestAcceptableToken = currentToken();
    m_stackNeeded = false;
}


//////////////////////////////////////////////////////////////////////
//
// Breakpoint specific stuff
//
//////////////////////////////////////////////////////////////////////

void GdbEngine::updateResponse(BreakpointResponse &response, const GdbMi &bkpt)
{
    QTC_ASSERT(bkpt.isValid(), return);

    QByteArray originalLocation;

    response.multiple = false;
    response.enabled = true;
    response.pending = false;
    response.condition.clear();
    QByteArray file, fullName;
    foreach (const GdbMi &child, bkpt.children()) {
        if (child.hasName("number")) {
            response.id = BreakpointResponseId(child.data());
        } else if (child.hasName("func")) {
            response.functionName = _(child.data());
        } else if (child.hasName("addr")) {
            // <MULTIPLE> happens in constructors, inline functions, and
            // at other places like 'foreach' lines. In this case there are
            // fields named "addr" in the response and/or the address
            // is called <MULTIPLE>.
            //qDebug() << "ADDR: " << child.data() << (child.data() == "<MULTIPLE>");
            if (child.data() == "<MULTIPLE>")
                response.multiple = true;
            if (child.data().startsWith("0x"))
                response.address = child.toAddress();
        } else if (child.hasName("file")) {
            file = child.data();
        } else if (child.hasName("fullname")) {
            fullName = child.data();
        } else if (child.hasName("line")) {
            // The line numbers here are the uncorrected ones. So don't
            // change it if we know better already.
            if (response.correctedLineNumber == 0)
                response.lineNumber = child.toInt();
        } else if (child.hasName("cond")) {
            // gdb 6.3 likes to "rewrite" conditions. Just accept that fact.
            response.condition = child.data();
        } else if (child.hasName("enabled")) {
            response.enabled = (child.data() == "y");
        } else if (child.hasName("disp")) {
            response.oneShot = child.data() == "del";
        } else if (child.hasName("pending")) {
            // Any content here would be interesting only if we did accept
            // spontaneously appearing breakpoints (user using gdb commands).
            if (file.isEmpty())
                file = child.data();
            response.pending = true;
        } else if (child.hasName("at")) {
            // Happens with gdb 6.4 symbianelf.
            QByteArray ba = child.data();
            if (ba.startsWith('<') && ba.endsWith('>'))
                ba = ba.mid(1, ba.size() - 2);
            response.functionName = _(ba);
        } else if (child.hasName("thread")) {
            response.threadSpec = child.toInt();
        } else if (child.hasName("type")) {
            // "breakpoint", "hw breakpoint", "tracepoint", "hw watchpoint"
            // {bkpt={number="2",type="hw watchpoint",disp="keep",enabled="y",
            //  what="*0xbfffed48",times="0",original-location="*0xbfffed48"}}
            if (child.data().contains("tracepoint")) {
                response.tracepoint = true;
            } else if (child.data() == "hw watchpoint" || child.data() == "watchpoint") {
                QByteArray what = bkpt["what"].data();
                if (what.startsWith("*0x")) {
                    response.type = WatchpointAtAddress;
                    response.address = what.mid(1).toULongLong(0, 0);
                } else {
                    response.type = WatchpointAtExpression;
                    response.expression = QString::fromLocal8Bit(what);
                }
            } else if (child.data() == "breakpoint") {
                QByteArray catchType = bkpt["catch-type"].data();
                if (catchType == "throw")
                    response.type = BreakpointAtThrow;
                else if (catchType == "catch")
                    response.type = BreakpointAtCatch;
                else if (catchType == "fork")
                    response.type = BreakpointAtFork;
                else if (catchType == "exec")
                    response.type = BreakpointAtExec;
                else if (catchType == "syscall")
                    response.type = BreakpointAtSysCall;
            }
        } else if (child.hasName("original-location")) {
            originalLocation = child.data();
        }
        // This field is not present.  Contents needs to be parsed from
        // the plain "ignore" response.
        //else if (child.hasName("ignore"))
        //    response.ignoreCount = child.data();
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

    if (response.fileName.isEmpty())
        response.updateLocation(originalLocation);
}

QString GdbEngine::breakLocation(const QString &file) const
{
    QString where = m_fullToShortName.value(file);
    if (where.isEmpty())
        return FileName::fromString(file).fileName();
    return where;
}

QByteArray GdbEngine::breakpointLocation(const BreakpointParameters &data)
{
    QTC_ASSERT(data.type != UnknownBreakpointType, return QByteArray());
    // FIXME: Non-GCC-runtime
    if (data.type == BreakpointAtThrow)
        return "__cxa_throw";
    if (data.type == BreakpointAtCatch)
        return "__cxa_begin_catch";
    if (data.type == BreakpointAtMain) {
        const Abi abi = startParameters().toolChainAbi;
        return (abi.os() == Abi::WindowsOS) ? "qMain" : "main";
    }
    if (data.type == BreakpointByFunction)
        return '"' + data.functionName.toUtf8() + '"';
    if (data.type == BreakpointByAddress)
        return addressSpec(data.address);

    BreakpointPathUsage usage = data.pathUsage;
    if (usage == BreakpointPathUsageEngineDefault)
        usage = BreakpointUseShortPath;

    const QString fileName = usage == BreakpointUseFullPath
        ? data.fileName : breakLocation(data.fileName);
    // The argument is simply a C-quoted version of the argument to the
    // non-MI "break" command, including the "original" quoting it wants.
    return "\"\\\"" + GdbMi::escapeCString(fileName.toLocal8Bit()) + "\\\":"
        + QByteArray::number(data.lineNumber) + '"';
}

QByteArray GdbEngine::breakpointLocation2(const BreakpointParameters &data)
{
    BreakpointPathUsage usage = data.pathUsage;
    if (usage == BreakpointPathUsageEngineDefault)
        usage = BreakpointUseShortPath;

    const QString fileName = usage == BreakpointUseFullPath
        ? data.fileName : breakLocation(data.fileName);
    return  GdbMi::escapeCString(fileName.toLocal8Bit()) + ':'
        + QByteArray::number(data.lineNumber);
}

void GdbEngine::handleWatchInsert(const DebuggerResponse &response, Breakpoint bp)
{
    if (bp && response.resultClass == ResultDone) {
        BreakpointResponse br = bp.response();
        // "Hardware watchpoint 2: *0xbfffed40\n"
        QByteArray ba = response.consoleStreamOutput;
        GdbMi wpt = response.data["wpt"];
        if (wpt.isValid()) {
            // Mac yields:
            //>32^done,wpt={number="4",exp="*4355182176"}
            br.id = BreakpointResponseId(wpt["number"].data());
            QByteArray exp = wpt["exp"].data();
            if (exp.startsWith('*'))
                br.address = exp.mid(1).toULongLong(0, 0);
            bp.setResponse(br);
            QTC_CHECK(!bp.needsChange());
            bp.notifyBreakpointInsertOk();
        } else if (ba.startsWith("Hardware watchpoint ")
                || ba.startsWith("Watchpoint ")) {
            // Non-Mac: "Hardware watchpoint 2: *0xbfffed40\n"
            const int end = ba.indexOf(':');
            const int begin = ba.lastIndexOf(' ', end) + 1;
            const QByteArray address = ba.mid(end + 2).trimmed();
            br.id = BreakpointResponseId(ba.mid(begin, end - begin));
            if (address.startsWith('*'))
                br.address = address.mid(1).toULongLong(0, 0);
            bp.setResponse(br);
            QTC_CHECK(!bp.needsChange());
            bp.notifyBreakpointInsertOk();
        } else {
            showMessage(_("CANNOT PARSE WATCHPOINT FROM " + ba));
        }
    }
}

void GdbEngine::handleCatchInsert(const DebuggerResponse &response, Breakpoint bp)
{
    if (bp && response.resultClass == ResultDone)
        bp.notifyBreakpointInsertOk();
}

void GdbEngine::handleBkpt(const GdbMi &bkpt, Breakpoint bp)
{
    BreakpointResponse br = bp.response();
    QTC_ASSERT(bp, return);
    const QByteArray nr = bkpt["number"].data();
    const BreakpointResponseId rid(nr);
    QTC_ASSERT(rid.isValid(), return);
    if (nr.contains('.')) {
        // A sub-breakpoint.
        BreakpointResponse sub;
        updateResponse(sub, bkpt);
        sub.id = rid;
        sub.type = bp.type();
        bp.insertSubBreakpoint(sub);
        return;
    }

    // The MI output format might change, see
    // http://permalink.gmane.org/gmane.comp.gdb.patches/83936
    const GdbMi locations = bkpt["locations"];
    if (locations.isValid()) {
        foreach (const GdbMi &loc, locations.children()) {
            // A sub-breakpoint.
            const QByteArray subnr = loc["number"].data();
            const BreakpointResponseId subrid(subnr);
            BreakpointResponse sub;
            updateResponse(sub, loc);
            sub.id = subrid;
            sub.type = br.type;
            bp.insertSubBreakpoint(sub);
        }
    }

    // A (the?) primary breakpoint.
    updateResponse(br, bkpt);
    br.id = rid;
    bp.setResponse(br);
}

void GdbEngine::handleBreakInsert1(const DebuggerResponse &response, Breakpoint bp)
{
    if (bp.state() == BreakpointRemoveRequested) {
        if (response.resultClass == ResultDone) {
            // This delete was deferred. Act now.
            const GdbMi mainbkpt = response.data["bkpt"];
            bp.notifyBreakpointRemoveProceeding();
            QByteArray nr = mainbkpt["number"].data();
            postCommand("-break-delete " + nr,
                NeedsStop | RebuildBreakpointModel);
            bp.notifyBreakpointRemoveOk();
            return;
        }
    }
    if (response.resultClass == ResultDone) {
        // The result is a list with the first entry marked "bkpt"
        // and "unmarked" rest. The "bkpt" one seems to always be
        // the "main" entry. Use the "main" entry to retrieve the
        // already known data from the BreakpointManager, and then
        // iterate over all items to update main- and sub-data.
        const GdbMi mainbkpt = response.data["bkpt"];
        const QByteArray mainnr = mainbkpt["number"].data();
        const BreakpointResponseId mainrid(mainnr);
        if (!isHiddenBreakpoint(mainrid)) {
            foreach (const GdbMi &bkpt, response.data.children())
                handleBkpt(bkpt, bp);
            if (bp.needsChange()) {
                bp.notifyBreakpointChangeAfterInsertNeeded();
                changeBreakpoint(bp);
            } else {
                bp.notifyBreakpointInsertOk();
            }
        }
    } else if (response.data["msg"].data().contains("Unknown option")) {
        // Older version of gdb don't know the -a option to set tracepoints
        // ^error,msg="mi_cmd_break_insert: Unknown option ``a''"
        const QString fileName = bp.fileName();
        const int lineNumber = bp.lineNumber();
        QByteArray cmd = "trace "
            "\"" + GdbMi::escapeCString(fileName.toLocal8Bit()) + "\":"
            + QByteArray::number(lineNumber);
        postCommand(cmd, NeedsStop | RebuildBreakpointModel);
    } else {
        // Some versions of gdb like "GNU gdb (GDB) SUSE (6.8.91.20090930-2.4)"
        // know how to do pending breakpoints using CLI but not MI. So try
        // again with MI.
        QByteArray cmd = "break " + breakpointLocation2(bp.parameters());
        postCommand(cmd, NeedsStop | RebuildBreakpointModel,
            [this, bp](const DebuggerResponse &r) {  handleBreakInsert2(r, bp); });
    }
}

void GdbEngine::handleBreakInsert2(const DebuggerResponse &response, Breakpoint bp)
{
    if (response.resultClass == ResultDone) {
        QTC_ASSERT(bp, return);
        bp.notifyBreakpointInsertOk();
    } else {
        // Note: gdb < 60800  doesn't "do" pending breakpoints.
        // Not much we can do about it except implementing the
        // logic on top of shared library events, and that's not
        // worth the effort.
    }
}

void GdbEngine::handleBreakDisable(const DebuggerResponse &response, Breakpoint bp)
{
    QTC_CHECK(response.resultClass == ResultDone);
    // This should only be the requested state.
    QTC_ASSERT(!bp.isEnabled(), /* Prevent later recursion */);
    BreakpointResponse br = bp.response();
    br.enabled = false;
    bp.setResponse(br);
    changeBreakpoint(bp); // Maybe there's more to do.
}

void GdbEngine::handleBreakEnable(const DebuggerResponse &response, Breakpoint bp)
{
    QTC_CHECK(response.resultClass == ResultDone);
    // This should only be the requested state.
    QTC_ASSERT(bp.isEnabled(), /* Prevent later recursion */);
    BreakpointResponse br = bp.response();
    br.enabled = true;
    bp.setResponse(br);
    changeBreakpoint(bp); // Maybe there's more to do.
}

void GdbEngine::handleBreakThreadSpec(const DebuggerResponse &response, Breakpoint bp)
{
    QTC_CHECK(response.resultClass == ResultDone);
    BreakpointResponse br = bp.response();
    br.threadSpec = bp.threadSpec();
    bp.setResponse(br);
    bp.notifyBreakpointNeedsReinsertion();
    insertBreakpoint(bp);
}

void GdbEngine::handleBreakLineNumber(const DebuggerResponse &response, Breakpoint bp)
{
    QTC_CHECK(response.resultClass == ResultDone);
    BreakpointResponse br = bp.response();
    br.lineNumber = bp.lineNumber();
    bp.setResponse(br);
    bp.notifyBreakpointNeedsReinsertion();
    insertBreakpoint(bp);
}

void GdbEngine::handleBreakIgnore(const DebuggerResponse &response, Breakpoint bp)
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
    //QString msg = _(response.consoleStreamOutput);
    BreakpointResponse br = bp.response();
    //if (msg.contains(__("Will stop next time breakpoint")))
    //    response.ignoreCount = _("0");
    //else if (msg.contains(__("Will ignore next")))
    //    response.ignoreCount = data->ignoreCount;
    // FIXME: this assumes it is doing the right thing...
    const BreakpointParameters &parameters = bp.parameters();
    br.ignoreCount = parameters.ignoreCount;
    br.command = parameters.command;
    bp.setResponse(br);
    changeBreakpoint(bp); // Maybe there's more to do.
}

void GdbEngine::handleBreakCondition(const DebuggerResponse &, Breakpoint bp)
{
    // Can happen at invalid condition strings.
    //QTC_CHECK(response.resultClass == ResultDone)
    // We just assume it was successful. Otherwise we had to parse
    // the output stream data.
    // The following happens on Mac:
    //   QByteArray msg = response.data.findChild("msg").data();
    //   if (msg.startsWith("Error parsing breakpoint condition. "
    //         " Will try again when we hit the breakpoint."))
    BreakpointResponse br = bp.response();
    br.condition = bp.condition();
    bp.setResponse(br);
    changeBreakpoint(bp); // Maybe there's more to do.
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

bool GdbEngine::acceptsBreakpoint(Breakpoint bp) const
{
    if (startParameters().startMode == AttachCore)
        return false;
    // We handle QML breakpoint unless specifically
    if (isNativeMixedEnabled() && !(startParameters().languages & QmlLanguage))
        return true;
    return bp.parameters().isCppBreakpoint();
}

void GdbEngine::insertBreakpoint(Breakpoint bp)
{
    // Set up fallback in case of pending breakpoints which aren't handled
    // by the MI interface.
    QTC_CHECK(bp.state() == BreakpointInsertRequested);
    bp.notifyBreakpointInsertProceeding();

    const BreakpointParameters &data = bp.parameters();

    if (!data.isCppBreakpoint()) {
        DebuggerCommand cmd("insertQmlBreakpoint");
        bp.addToCommand(&cmd);
        runCommand(cmd);
        bp.notifyBreakpointInsertOk();
        return;
    }

    BreakpointType type = bp.type();
    if (type == WatchpointAtAddress) {
        postCommand("watch " + addressSpec(bp.address()),
            NeedsStop | RebuildBreakpointModel | ConsoleCommand,
            [this, bp](const DebuggerResponse &r) { handleWatchInsert(r, bp); });
        return;
    }
    if (type == WatchpointAtExpression) {
        postCommand("watch " + bp.expression().toLocal8Bit(),
            NeedsStop | RebuildBreakpointModel | ConsoleCommand,
            [this, bp](const DebuggerResponse &r) { handleWatchInsert(r, bp); });
        return;
    }
    if (type == BreakpointAtFork) {
        postCommand("catch fork",
            NeedsStop | RebuildBreakpointModel | ConsoleCommand,
            [this, bp](const DebuggerResponse &r) { handleCatchInsert(r, bp); });
        postCommand("catch vfork",
            NeedsStop | RebuildBreakpointModel | ConsoleCommand,
            [this, bp](const DebuggerResponse &r) { handleCatchInsert(r, bp); });
        return;
    }
    //if (type == BreakpointAtVFork) {
    //    postCommand("catch vfork", NeedsStop | RebuildBreakpointModel,
    //        CB(handleCatchInsert), vid);
    //    return;
    //}
    if (type == BreakpointAtExec) {
        postCommand("catch exec",
            NeedsStop | RebuildBreakpointModel | ConsoleCommand,
            [this, bp](const DebuggerResponse &r) { handleCatchInsert(r, bp); });
        return;
    }
    if (type == BreakpointAtSysCall) {
        postCommand("catch syscall",
            NeedsStop | RebuildBreakpointModel | ConsoleCommand,
            [this, bp](const DebuggerResponse &r) { handleCatchInsert(r, bp); });
        return;
    }

    QByteArray cmd;
    if (bp.isTracepoint()) {
        cmd = "-break-insert -a -f ";
    } else {
        int spec = bp.threadSpec();
        cmd = "-break-insert ";
        if (spec >= 0)
            cmd += "-p " + QByteArray::number(spec);
        cmd += " -f ";
    }

    if (bp.isOneShot())
        cmd += "-t ";

    if (!bp.isEnabled())
        cmd += "-d ";

    if (int ignoreCount = bp.ignoreCount())
        cmd += "-i " + QByteArray::number(ignoreCount) + ' ';

    QByteArray condition = bp.condition();
    if (!condition.isEmpty())
        cmd += " -c \"" + condition + "\" ";

    cmd += breakpointLocation(bp.parameters());
    postCommand(cmd, NeedsStop | RebuildBreakpointModel,
        [this, bp](const DebuggerResponse &r) { handleBreakInsert1(r, bp); });
}

void GdbEngine::changeBreakpoint(Breakpoint bp)
{
    const BreakpointParameters &data = bp.parameters();
    QTC_ASSERT(data.type != UnknownBreakpointType, return);
    const BreakpointResponse &response = bp.response();
    QTC_ASSERT(response.id.isValid(), return);
    const QByteArray bpnr = response.id.toByteArray();
    const BreakpointState state = bp.state();
    if (state == BreakpointChangeRequested)
        bp.notifyBreakpointChangeProceeding();
    const BreakpointState state2 = bp.state();
    QTC_ASSERT(state2 == BreakpointChangeProceeding, qDebug() << state2);

    if (!response.pending && data.threadSpec != response.threadSpec) {
        // The only way to change this seems to be to re-set the bp completely.
        postCommand("-break-delete " + bpnr,
            NeedsStop | RebuildBreakpointModel,
            [this, bp](const DebuggerResponse &r) { handleBreakThreadSpec(r, bp); });
        return;
    }
    if (!response.pending && data.lineNumber != response.lineNumber) {
        // The only way to change this seems to be to re-set the bp completely.
        postCommand("-break-delete " + bpnr,
            NeedsStop | RebuildBreakpointModel,
            [this, bp](const DebuggerResponse &r) { handleBreakLineNumber(r, bp); });
        return;
    }
    if (data.command != response.command) {
        QByteArray breakCommand = "-break-commands " + bpnr;
        foreach (const QString &command, data.command.split(QLatin1String("\n"))) {
            if (!command.isEmpty()) {
                breakCommand.append(" \"");
                breakCommand.append(command.toLatin1());
                breakCommand.append('"');
            }
        }
        postCommand(breakCommand, NeedsStop | RebuildBreakpointModel,
            [this, bp](const DebuggerResponse &r) { handleBreakIgnore(r, bp); });
        return;
    }
    if (!data.conditionsMatch(response.condition)) {
        postCommand("condition " + bpnr + ' '  + data.condition,
            NeedsStop | RebuildBreakpointModel,
            [this, bp](const DebuggerResponse &r) { handleBreakCondition(r, bp); });
        return;
    }
    if (data.ignoreCount != response.ignoreCount) {
        postCommand("ignore " + bpnr + ' ' + QByteArray::number(data.ignoreCount),
            NeedsStop | RebuildBreakpointModel,
            [this, bp](const DebuggerResponse &r) { handleBreakIgnore(r, bp); });
        return;
    }
    if (!data.enabled && response.enabled) {
        postCommand("-break-disable " + bpnr,
            NeedsStop | RebuildBreakpointModel,
            [this, bp](const DebuggerResponse &r) { handleBreakDisable(r, bp); });
        return;
    }
    if (data.enabled && !response.enabled) {
        postCommand("-break-enable " + bpnr,
            NeedsStop | RebuildBreakpointModel,
            [this, bp](const DebuggerResponse &r) { handleBreakEnable(r, bp); });
        return;
    }
    bp.notifyBreakpointChangeOk();
}

void GdbEngine::removeBreakpoint(Breakpoint bp)
{
    QTC_CHECK(bp.state() == BreakpointRemoveRequested);
    BreakpointResponse br = bp.response();

    const BreakpointParameters &data = bp.parameters();
    if (!data.isCppBreakpoint()) {
        DebuggerCommand cmd("removeQmlBreakpoint");
        bp.addToCommand(&cmd);
        runCommand(cmd);
        bp.notifyBreakpointRemoveOk();
        return;
    }

    if (br.id.isValid()) {
        // We already have a fully inserted breakpoint.
        bp.notifyBreakpointRemoveProceeding();
        showMessage(_("DELETING BP %1 IN %2").arg(br.id.toString()).arg(bp.fileName()));
        postCommand("-break-delete " + br.id.toByteArray(),
            NeedsStop | RebuildBreakpointModel);

        // Pretend it succeeds without waiting for response. Feels better.
        // Otherwise, clicking in the gutter leaves the breakpoint visible
        // for quite some time, so the user assumes a mis-click and clicks
        // again, effectivly re-introducing the breakpoint.
        bp.notifyBreakpointRemoveOk();

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

void GdbEngine::loadSymbols(const QString &modulePath)
{
    // FIXME: gdb does not understand quoted names here (tested with 6.8)
    postCommand("sharedlibrary " + dotEscape(modulePath.toLocal8Bit()));
    reloadModulesInternal();
    reloadStack();
    updateLocals();
}

void GdbEngine::loadAllSymbols()
{
    postCommand("sharedlibrary .*");
    reloadModulesInternal();
    reloadStack();
    updateLocals();
}

void GdbEngine::loadSymbolsForStack()
{
    bool needUpdate = false;
    const Modules &modules = modulesHandler()->modules();
    foreach (const StackFrame &frame, stackHandler()->frames()) {
        if (frame.function == _("??")) {
            //qDebug() << "LOAD FOR " << frame.address;
            foreach (const Module &module, modules) {
                if (module.startAddress <= frame.address
                        && frame.address < module.endAddress) {
                    postCommand("sharedlibrary "
                        + dotEscape(module.modulePath.toLocal8Bit()));
                    needUpdate = true;
                }
            }
        }
    }
    if (needUpdate) {
        //reloadModulesInternal();
        reloadStack();
        updateLocals();
    }
}

static void handleShowModuleSymbols(const DebuggerResponse &response,
    const QString &modulePath, const QString &fileName)
{
    if (response.resultClass == ResultDone) {
        Symbols symbols;
        QFile file(fileName);
        file.open(QIODevice::ReadOnly);
        // Object file /opt/dev/qt/lib/libQtNetworkMyns.so.4:
        // [ 0] A 0x16bd64 _DYNAMIC  moc_qudpsocket.cpp
        // [12] S 0xe94680 _ZN4myns5QFileC1Ev section .plt  myns::QFile::QFile()
        foreach (const QByteArray &line, file.readAll().split('\n')) {
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
            symbol.state = _(line.mid(posCode, 1));
            symbol.address = _(line.mid(posAddress, lenAddress));
            symbol.name = _(line.mid(posName, lenName));
            symbol.section = _(line.mid(posSection, lenSection));
            symbol.demangled = _(line.mid(posDemangled, lenDemangled));
            symbols.push_back(symbol);
        }
        file.close();
        file.remove();
        Internal::showModuleSymbols(modulePath, symbols);
    } else {
        AsynchronousMessageBox::critical(GdbEngine::tr("Cannot Read Symbols"),
            GdbEngine::tr("Cannot read symbols for module \"%1\".").arg(fileName));
    }
}

void GdbEngine::requestModuleSymbols(const QString &modulePath)
{
    QTemporaryFile tf(QDir::tempPath() + _("/gdbsymbols"));
    if (!tf.open())
        return;
    QString fileName = tf.fileName();
    tf.close();
    postCommand("maint print msymbols \"" + fileName.toLocal8Bit()
            + "\" " + modulePath.toLocal8Bit(), NeedsStop,
        [modulePath, fileName](const DebuggerResponse &r) {
            handleShowModuleSymbols(r, modulePath, fileName); });
}

void GdbEngine::requestModuleSections(const QString &moduleName)
{
    // There seems to be no way to get the symbols from a single .so.
    postCommand("maint info section ALLOBJ", NeedsStop,
        [this, moduleName](const DebuggerResponse &r) {
            handleShowModuleSections(r, moduleName); });
}

void GdbEngine::handleShowModuleSections(const DebuggerResponse &response,
                                         const QString &moduleName)
{
    // ~"  Object file: /usr/lib/i386-linux-gnu/libffi.so.6\n"
    // ~"    0xb44a6114->0xb44a6138 at 0x00000114: .note.gnu.build-id ALLOC LOAD READONLY DATA HAS_CONTENTS\n"
    if (response.resultClass == ResultDone) {
        const QStringList lines = QString::fromLocal8Bit(response.consoleStreamOutput).split(QLatin1Char('\n'));
        const QString prefix = QLatin1String("  Object file: ");
        const QString needle = prefix + moduleName;
        Sections sections;
        bool active = false;
        foreach (const QString &line, lines) {
            if (line.startsWith(prefix)) {
                if (active)
                    break;
                if (line == needle)
                    active = true;
            } else {
                if (active) {
                    QStringList items = line.split(QLatin1Char(' '), QString::SkipEmptyParts);
                    QString fromTo = items.value(0, QString());
                    const int pos = fromTo.indexOf(QLatin1Char('-'));
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
            Internal::showModuleSections(moduleName, sections);
    }
}

void GdbEngine::reloadModules()
{
    if (state() == InferiorRunOk || state() == InferiorStopOk)
        reloadModulesInternal();
}

void GdbEngine::reloadModulesInternal()
{
    postCommand("info shared", NeedsStop, CB(handleModulesList));
}

static QString nameFromPath(const QString &path)
{
    return QFileInfo(path).baseName();
}

void GdbEngine::handleModulesList(const DebuggerResponse &response)
{
    if (response.resultClass == ResultDone) {
        ModulesHandler *handler = modulesHandler();
        Module module;
        // That's console-based output, likely Linux or Windows,
        // but we can avoid the target dependency here.
        QString data = QString::fromLocal8Bit(response.consoleStreamOutput);
        QTextStream ts(&data, QIODevice::ReadOnly);
        bool found = false;
        while (!ts.atEnd()) {
            QString line = ts.readLine();
            QString symbolsRead;
            QTextStream ts(&line, QIODevice::ReadOnly);
            if (line.startsWith(QLatin1String("0x"))) {
                ts >> module.startAddress >> module.endAddress >> symbolsRead;
                module.modulePath = ts.readLine().trimmed();
                module.moduleName = nameFromPath(module.modulePath);
                module.symbolsRead =
                    (symbolsRead == QLatin1String("Yes") ? Module::ReadOk : Module::ReadFailed);
                handler->updateModule(module);
                found = true;
            } else if (line.trimmed().startsWith(QLatin1String("No"))) {
                // gdb 6.4 symbianelf
                ts >> symbolsRead;
                QTC_ASSERT(symbolsRead == QLatin1String("No"), continue);
                module.startAddress = 0;
                module.endAddress = 0;
                module.modulePath = ts.readLine().trimmed();
                module.moduleName = nameFromPath(module.modulePath);
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
            foreach (const GdbMi &item, response.data.children()) {
                module.modulePath =
                    QString::fromLocal8Bit(item["path"].data());
                module.moduleName = nameFromPath(module.modulePath);
                module.symbolsRead = (item["state"].data() == "Y")
                        ? Module::ReadOk : Module::ReadFailed;
                module.startAddress =
                    item["loaded_addr"].data().toULongLong(0, 0);
                module.endAddress = 0; // FIXME: End address not easily available.
                handler->updateModule(module);
            }
        }
    }
}

void GdbEngine::examineModules()
{
    ModulesHandler *handler = modulesHandler();
    foreach (const Module &module, handler->modules()) {
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
    if ((state() == InferiorRunOk || state() == InferiorStopOk)
        && !m_sourcesListUpdating)
        reloadSourceFilesInternal();
}

void GdbEngine::reloadSourceFilesInternal()
{
    QTC_CHECK(!m_sourcesListUpdating);
    m_sourcesListUpdating = true;
    postCommand("-file-list-exec-source-files", NeedsStop, CB(handleQuerySources));
}


//////////////////////////////////////////////////////////////////////
//
// Stack specific stuff
//
//////////////////////////////////////////////////////////////////////

void GdbEngine::selectThread(ThreadId threadId)
{
    threadsHandler()->setCurrentThread(threadId);
    showStatusMessage(tr("Retrieving data for stack view thread 0x%1...")
        .arg(threadId.raw(), 0, 16), 10000);
    postCommand("-thread-select " + QByteArray::number(threadId.raw()), Discardable,
        CB(handleStackSelectThread));
}

void GdbEngine::handleStackSelectThread(const DebuggerResponse &)
{
    QTC_CHECK(state() == InferiorUnrunnable || state() == InferiorStopOk);
    showStatusMessage(tr("Retrieving data for stack view..."), 3000);
    reloadStack(); // Will reload registers.
    updateLocals();
}

void GdbEngine::reloadFullStack()
{
    PENDING_DEBUG("RELOAD FULL STACK");
    resetLocation();
    DebuggerCommand cmd = stackCommand(-1);
    cmd.flags = Discardable;
    cmd.callback = [this](const DebuggerResponse &r) { handleStackListFrames(r, true); };
    runCommand(cmd);
}

void GdbEngine::loadAdditionalQmlStack()
{
    // Scan for QV4::ExecutionContext parameter in the parameter list of a V4 call.
    postCommand("-stack-list-arguments --simple-values", NeedsStop, CB(handleQmlStackFrameArguments));
}

// Scan the arguments of a stack list for the address of a QV4::ExecutionContext.
static quint64 findJsExecutionContextAddress(const GdbMi &stackArgsResponse, const QByteArray &qtNamespace)
{
    const GdbMi frameList = stackArgsResponse.childAt(0);
    if (!frameList.childCount())
        return 0;
    QByteArray jsExecutionContextType = qtNamespace;
    if (!jsExecutionContextType.isEmpty())
        jsExecutionContextType.append("::");
    jsExecutionContextType.append("QV4::ExecutionContext *");
    foreach (const GdbMi &frameNode, frameList.children()) {
        foreach (const GdbMi &argNode, frameNode["args"].children()) {
            if (argNode["type"].data() == jsExecutionContextType) {
                bool ok;
                const quint64 address = argNode["value"].data().toULongLong(&ok, 16);
                if (ok && address)
                    return address;
            }
        }
    }
    return 0;
}

static QString msgCannotLoadQmlStack(const QString &why)
{
    return _("Unable to load QML stack: ") + why;
}

void GdbEngine::handleQmlStackFrameArguments(const DebuggerResponse &response)
{
    if (!response.data.isValid()) {
        showMessage(msgCannotLoadQmlStack(_("No stack obtained.")), LogError);
        return;
    }
    const quint64 contextAddress = findJsExecutionContextAddress(response.data, qtNamespace());
    if (!contextAddress) {
        showMessage(msgCannotLoadQmlStack(_("The address of the JS execution context could not be found.")), LogError);
        return;
    }
    // Call the debug function of QML with the context address to obtain the QML stack trace.
    QByteArray command = "-data-evaluate-expression \"qt_v4StackTrace((QV4::ExecutionContext *)0x";
    command += QByteArray::number(contextAddress, 16);
    command += ")\"";
    postCommand(command, NoFlags, CB(handleQmlStackTrace));
}

void GdbEngine::handleQmlStackTrace(const DebuggerResponse &response)
{
    if (!response.data.isValid()) {
        showMessage(msgCannotLoadQmlStack(_("No result obtained.")), LogError);
        return;
    }
    // Prepend QML stack frames to existing C++ stack frames.
    QByteArray stackData = response.data["value"].data();
    const int index = stackData.indexOf("stack=");
    if (index == -1) {
        showMessage(msgCannotLoadQmlStack(_("Malformed result.")), LogError);
        return;
    }
    stackData.remove(0, index);
    stackData.replace("\\\"", "\"");
    GdbMi stackMi;
    stackMi.fromString(stackData);
    const int qmlFrameCount = stackMi.childCount();
    if (!qmlFrameCount) {
        showMessage(msgCannotLoadQmlStack(_("No stack frames obtained.")), LogError);
        return;
    }
    QList<StackFrame> qmlFrames;
    qmlFrames.reserve(qmlFrameCount);
    for (int i = 0; i < qmlFrameCount; ++i) {
        StackFrame frame = parseStackFrame(stackMi.childAt(i), i);
        frame.fixQmlFrame(startParameters());
        qmlFrames.append(frame);
    }
    stackHandler()->prependFrames(qmlFrames);
}

DebuggerCommand GdbEngine::stackCommand(int depth)
{
    DebuggerCommand cmd("stackListFrames");
    cmd.arg("limit", depth);
    cmd.arg("options", isNativeMixedActive() ? "nativemixed" : "");
    return cmd;
}

void GdbEngine::reloadStack()
{
    PENDING_DEBUG("RELOAD STACK");
    DebuggerCommand cmd = stackCommand(action(MaximalStackDepth)->value().toInt());
    cmd.flags = Discardable;
    cmd.callback = [this](const DebuggerResponse &r) { handleStackListFrames(r, false); };
    runCommand(cmd);
}

StackFrame GdbEngine::parseStackFrame(const GdbMi &frameMi, int level)
{
    //qDebug() << "HANDLING FRAME:" << frameMi.toString();
    StackFrame frame;
    frame.level = level;
    GdbMi fullName = frameMi["fullname"];
    if (fullName.isValid())
        frame.file = cleanupFullName(QFile::decodeName(fullName.data()));
    else
        frame.file = QFile::decodeName(frameMi["file"].data());
    frame.function = _(frameMi["func"].data());
    frame.from = _(frameMi["from"].data());
    frame.line = frameMi["line"].toInt();
    frame.address = frameMi["addr"].toAddress();
    GdbMi usable = frameMi["usable"];
    if (usable.isValid())
        frame.usable = usable.data().toInt();
    else
        frame.usable = QFileInfo(frame.file).isReadable();
    if (frameMi["language"].data() == "js"
            || frame.file.endsWith(QLatin1String(".js"))
            || frame.file.endsWith(QLatin1String(".qml"))) {
        frame.file = QFile::decodeName(frameMi["file"].data());
        frame.language = QmlLanguage;
        frame.fixQmlFrame(startParameters());
    }
    return frame;
}

void GdbEngine::handleStackListFrames(const DebuggerResponse &response, bool isFull)
{
    if (response.resultClass != ResultDone) {
        // That always happens on symbian gdb with
        // ^error,data={msg="Previous frame identical to this frame (corrupt stack?)"
        // logStreamOutput: "Previous frame identical to this frame (corrupt stack?)\n"
        //qDebug() << "LISTING STACK FAILED: " << response.toString();
        reloadRegisters();
        return;
    }

    QList<StackFrame> stackFrames;

    GdbMi stack = response.data["stack"]; // C++
    if (!stack.isValid() || stack.childCount() == 0) // Mixed.
        stack.fromStringMultiple(response.consoleStreamOutput);

    if (!stack.isValid()) {
        qDebug() << "FIXME: stack:" << stack.toString();
        return;
    }

    int targetFrame = -1;

    int n = stack.childCount();
    for (int i = 0; i != n; ++i) {
        stackFrames.append(parseStackFrame(stack.childAt(i), i));
        const StackFrame &frame = stackFrames.back();

        // Initialize top frame to the first valid frame.
        const bool isValid = frame.isUsable() && !frame.function.isEmpty();
        if (isValid && targetFrame == -1)
            targetFrame = i;
    }

    bool canExpand = !isFull && (n >= action(MaximalStackDepth)->value().toInt());
    action(ExpandStack)->setEnabled(canExpand);
    stackHandler()->setFrames(stackFrames, canExpand);

    // We can't jump to any file if we don't have any frames.
    if (stackFrames.isEmpty())
        return;

    // targetFrame contains the top most frame for which we have source
    // information. That's typically the frame we'd like to jump to, with
    // a few exceptions:

    // Always jump to frame #0 when stepping by instruction.
    if (boolSetting(OperateByInstruction))
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
    handler->setCurrentIndex(frameIndex);
    gotoLocation(stackHandler()->currentFrame());

    if (handler->frameAt(frameIndex).language != QmlLanguage) {
        // Assuming the command always succeeds this saves a roundtrip.
        // Otherwise the lines below would need to get triggered
        // after a response to this -stack-select-frame here.
        QByteArray cmd = "-stack-select-frame";
        //if (!m_currentThread.isEmpty())
        //    cmd += " --thread " + m_currentThread;
        cmd += ' ';
        cmd += QByteArray::number(frameIndex);
        postCommand(cmd, Discardable);
    }

    updateLocals();
    reloadRegisters();
}

void GdbEngine::handleThreadInfo(const DebuggerResponse &response)
{
    if (response.resultClass == ResultDone) {
        ThreadsHandler *handler = threadsHandler();
        handler->updateThreads(response.data);
        // This is necessary as the current thread might not be in the list.
        if (!handler->currentThread().isValid()) {
            ThreadId other = handler->threadAt(0);
            if (other.isValid())
                selectThread(other);
        }
        updateViews(); // Adjust Threads combobox.
        if (boolSetting(ShowThreadNames)) {
            postCommand("threadnames " +
                action(MaximalStackDepth)->value().toByteArray(),
                Discardable, CB(handleThreadNames));
        }
        reloadStack(); // Will trigger register reload.
    } else {
        // Fall back for older versions: Try to get at least a list
        // of running threads.
        postCommand("-thread-list-ids", Discardable, CB(handleThreadListIds));
    }
}

void GdbEngine::handleThreadListIds(const DebuggerResponse &response)
{
    // "72^done,{thread-ids={thread-id="2",thread-id="1"},number-of-threads="2"}
    // In gdb 7.1+ additionally: current-thread-id="1"
    ThreadsHandler *handler = threadsHandler();
    const std::vector<GdbMi> &items = response.data["thread-ids"].children();
    for (size_t index = 0, n = items.size(); index != n; ++index) {
        ThreadData thread;
        thread.id = ThreadId(items.at(index).toInt());
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
        foreach (const GdbMi &name, names.children()) {
            ThreadData thread;
            thread.id = ThreadId(name["id"].toInt());
            thread.name = decodeData(name["value"].data(),
                name["valueencoded"].toInt());
            handler->updateThread(thread);
        }
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
        // This must not be quoted, it doesn't work otherwise.
        postCommand("gcore " + fileName.toLocal8Bit(), NeedsStop|ConsoleCommand,
            [this, fileName](const DebuggerResponse &r) { handleMakeSnapshot(r, fileName); });
    } else {
        AsynchronousMessageBox::critical(tr("Snapshot Creation Error"),
            tr("Cannot create snapshot file."));
    }
}

void GdbEngine::handleMakeSnapshot(const DebuggerResponse &response, const QString &coreFile)
{
    if (response.resultClass == ResultDone) {
        DebuggerStartParameters sp = startParameters();
        sp.startMode = AttachCore;
        sp.coreFile = coreFile;
        //snapshot.setDate(QDateTime::currentDateTime());
        StackFrames frames = stackHandler()->frames();
        QString function = _("<unknown>");
        if (!frames.isEmpty()) {
            const StackFrame &frame = frames.at(0);
            function = frame.function + _(":") + QString::number(frame.line);
        }
        sp.displayName = function + _(": ") + QDateTime::currentDateTime().toString();
        sp.isSnapshot = true;
        DebuggerRunControlFactory::createAndScheduleRun(sp);
    } else {
        QByteArray msg = response.data["msg"].data();
        AsynchronousMessageBox::critical(tr("Snapshot Creation Error"),
            tr("Cannot create snapshot:") + QLatin1Char('\n') + QString::fromLocal8Bit(msg));
    }
}


//////////////////////////////////////////////////////////////////////
//
// Register specific stuff
//
//////////////////////////////////////////////////////////////////////

void GdbEngine::reloadRegisters()
{
    if (!Internal::isDockVisible(_(DOCKWIDGET_REGISTER)))
        return;

    if (state() != InferiorStopOk && state() != InferiorUnrunnable)
        return;

    if (true) {
        if (!m_registerNamesListed) {
            postCommand("-data-list-register-names", NoFlags, CB(handleRegisterListNames));
            m_registerNamesListed = true;
        }
        // Can cause i386-linux-nat.c:571: internal-error: Got request
        // for bad register number 41.\nA problem internal to GDB has been detected.
        postCommand("-data-list-register-values r",
                    Discardable, CB(handleRegisterListValues));
    } else {
        postCommand("maintenance print cooked-registers", NoFlags, CB(handleMaintPrintRegisters));
    }
}

static QByteArray readWord(const QByteArray &ba, int *pos)
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

    const QByteArray &ba = response.consoleStreamOutput;
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
        reg.value = readWord(ba, &pos);
        handler->updateRegister(reg);
    }
    handler->commitUpdates();
}
void GdbEngine::setRegisterValue(const QByteArray &name, const QString &value)
{
    postCommand("set $" + name  + "=" + value.toLatin1());
    reloadRegisters();
}

void GdbEngine::handleRegisterListNames(const DebuggerResponse &response)
{
    if (response.resultClass != ResultDone) {
        m_registerNamesListed = false;
        return;
    }

    GdbMi names = response.data["register-names"];
    m_registerNames.clear();
    int gdbRegisterNumber = 0;
    foreach (const GdbMi &item, names.children()) {
        if (!item.data().isEmpty())
            m_registerNames[gdbRegisterNumber] = item.data();
        ++gdbRegisterNumber;
    }
}

void GdbEngine::handleRegisterListValues(const DebuggerResponse &response)
{
    if (response.resultClass != ResultDone)
        return;

    RegisterHandler *handler = registerHandler();
    // 24^done,register-values=[{number="0",value="0xf423f"},...]
    const GdbMi values = response.data["register-values"];
    foreach (const GdbMi &item, values.children()) {
        Register reg;
        const int number = item["number"].toInt();
        reg.name = m_registerNames[number];
        QByteArray data = item["value"].data();
        if (data.startsWith("0x")) {
            reg.value = data;
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
            // Try to make sense of it using the int32 chunks:
            QByteArray result = "0x";
            const int pos1 = data.indexOf("_int32");
            const int pos2 = data.indexOf('{', pos1) + 1;
            const int pos3 = data.indexOf('}', pos2);
            QByteArray inner = data.mid(pos2, pos3 - pos2);
            QList<QByteArray> list = inner.split(',');
            for (int i = list.size(); --i >= 0; ) {
                QByteArray chunk = list.at(i);
                if (chunk.startsWith(' '))
                    chunk.remove(0, 1);
                if (chunk.startsWith("0x"))
                    chunk.remove(0, 2);
                QTC_ASSERT(chunk.size() == 8, continue);
                result.append(chunk);
            }
            reg.value = result;
        }
        handler->updateRegister(reg);
    }
    handler->commitUpdates();
}


//////////////////////////////////////////////////////////////////////
//
// Tooltip specific stuff
//
//////////////////////////////////////////////////////////////////////

bool GdbEngine::setToolTipExpression(const DebuggerToolTipContext &context)
{
    if (state() != InferiorStopOk || !context.isCppEditor)
        return false;

    UpdateParameters params;
    params.partialVariable = context.iname;
    doUpdateLocals(params);
    return true;
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

void GdbEngine::updateWatchItem(WatchItem *item)
{
    UpdateParameters params;
    params.partialVariable = item->iname;
    doUpdateLocals(params);
}

void GdbEngine::handleVarAssign(const DebuggerResponse &)
{
    // Everything might have changed, force re-evaluation.
    setTokenBarrier();
    updateLocals();
}

void GdbEngine::updateLocals()
{
    watchHandler()->resetValueCache();
    watchHandler()->notifyUpdateStarted();
    doUpdateLocals(UpdateParameters());
}

void GdbEngine::assignValueInDebugger(WatchItem *item,
    const QString &expression, const QVariant &value)
{
    DebuggerCommand cmd("assignValue");
    cmd.arg("type", item->type.toHex());
    cmd.arg("expr", expression.toLatin1().toHex());
    cmd.arg("value", value.toString().toLatin1().toHex());
    cmd.arg("simpleType", isIntOrFloatType(item->type));
    cmd.callback = CB(handleVarAssign);
    runCommand(cmd);
}

void GdbEngine::watchPoint(const QPoint &pnt)
{
    QByteArray x = QByteArray::number(pnt.x());
    QByteArray y = QByteArray::number(pnt.y());
    postCommand("print " + qtNamespace() + "QApplication::widgetAt("
            + x + ',' + y + ')',
        NeedsStop, CB(handleWatchPoint));
}

void GdbEngine::handleWatchPoint(const DebuggerResponse &response)
{
    if (response.resultClass == ResultDone) {
        // "$5 = (void *) 0xbfa7ebfc\n"
        const QByteArray ba = parsePlainConsoleStream(response);
        const int pos0x = ba.indexOf("0x");
        if (pos0x == -1) {
            showStatusMessage(tr("Cannot read widget data: %1").arg(_(ba)));
        } else {
            const QByteArray addr = ba.mid(pos0x);
            if (addr.toULongLong(0, 0)) { // Non-null pointer
                const QByteArray type = "::" + qtNamespace() + "QWidget";
                const QString exp = _("{%1}%2").arg(_(type)).arg(_(addr));
                watchHandler()->watchExpression(exp);
            } else {
                showStatusMessage(tr("Could not find a widget."));
            }
        }
    }
}

class MemoryAgentCookie
{
public:
    MemoryAgentCookie()
        : accumulator(0), pendingRequests(0), agent(0), token(0), base(0), offset(0), length(0)
    {}

public:
    QByteArray *accumulator; // Shared between split request. Last one cleans up.
    uint *pendingRequests; // Shared between split request. Last one cleans up.

    QPointer<MemoryAgent> agent;
    QPointer<QObject> token;
    quint64 base; // base address.
    uint offset; // offset to base, and in accumulator
    uint length; //
};


void GdbEngine::changeMemory(MemoryAgent *agent, QObject *token,
        quint64 addr, const QByteArray &data)
{
    QByteArray cmd = "-data-write-memory 0x" + QByteArray::number(addr, 16) + " d 1";
    foreach (unsigned char c, data) {
        cmd.append(' ');
        cmd.append(QByteArray::number(uint(c)));
    }
    MemoryAgentCookie ac;
    ac.agent = agent;
    ac.token = token;
    ac.base = addr;
    ac.length = data.size();
    postCommand(cmd, NeedsStop, CB(handleChangeMemory));
}

void GdbEngine::handleChangeMemory(const DebuggerResponse &response)
{
    Q_UNUSED(response);
}

void GdbEngine::fetchMemory(MemoryAgent *agent, QObject *token, quint64 addr,
                            quint64 length)
{
    MemoryAgentCookie ac;
    ac.accumulator = new QByteArray(length, char());
    ac.pendingRequests = new uint(1);
    ac.agent = agent;
    ac.token = token;
    ac.base = addr;
    ac.length = length;
    fetchMemoryHelper(ac);
}

void GdbEngine::fetchMemoryHelper(const MemoryAgentCookie &ac)
{
    postCommand("-data-read-memory 0x" + QByteArray::number(ac.base + ac.offset, 16) + " x 1 1 "
            + QByteArray::number(ac.length), NeedsStop,
                [this, ac](const DebuggerResponse &r) { handleFetchMemory(r, ac); });
}

void GdbEngine::handleFetchMemory(const DebuggerResponse &response, MemoryAgentCookie ac)
{
    // ^done,addr="0x08910c88",nr-bytes="16",total-bytes="16",
    // next-row="0x08910c98",prev-row="0x08910c78",next-page="0x08910c98",
    // prev-page="0x08910c78",memory=[{addr="0x08910c88",
    // data=["1","0","0","0","5","0","0","0","0","0","0","0","0","0","0","0"]}]
    --*ac.pendingRequests;
    showMessage(QString::fromLatin1("PENDING: %1").arg(*ac.pendingRequests));
    QTC_ASSERT(ac.agent, return);
    if (response.resultClass == ResultDone) {
        GdbMi memory = response.data["memory"];
        QTC_ASSERT(memory.children().size() <= 1, return);
        if (memory.children().empty())
            return;
        GdbMi memory0 = memory.children().at(0); // we asked for only one 'row'
        GdbMi data = memory0["data"];
        for (int i = 0, n = int(data.children().size()); i != n; ++i) {
            const GdbMi &child = data.children().at(i);
            bool ok = true;
            unsigned char c = '?';
            c = child.data().toUInt(&ok, 0);
            QTC_ASSERT(ok, return);
            (*ac.accumulator)[ac.offset + i] = c;
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
        ac.agent->addLazyData(ac.token, ac.base, *ac.accumulator);
        delete ac.pendingRequests;
        delete ac.accumulator;
    }
}

class DisassemblerAgentCookie
{
public:
    DisassemblerAgentCookie() : agent(0) {}
    DisassemblerAgentCookie(DisassemblerAgent *agent_) : agent(agent_) {}

public:
    QPointer<DisassemblerAgent> agent;
};

void GdbEngine::fetchDisassembler(DisassemblerAgent *agent)
{
    if (boolSetting(IntelFlavor))
        postCommand("set disassembly-flavor intel");
    else
        postCommand("set disassembly-flavor att");

    fetchDisassemblerByCliPointMixed(agent);
}

static inline QByteArray disassemblerCommand(const Location &location, bool mixed)
{
    QByteArray command = "disassemble /r";
    if (mixed)
        command += 'm';
    command += ' ';
    if (const quint64 address = location.address()) {
        command += "0x";
        command += QByteArray::number(address, 16);
    } else if (!location.functionName().isEmpty()) {
        command += location.functionName().toLatin1();
    } else {
        QTC_ASSERT(false, return QByteArray(); );
    }
    return command;
}

void GdbEngine::fetchDisassemblerByCliPointMixed(const DisassemblerAgentCookie &ac)
{
    QTC_ASSERT(ac.agent, return);
    postCommand(disassemblerCommand(ac.agent->location(), true), Discardable|ConsoleCommand,
        [this, ac](const DebuggerResponse &response) {
            if (response.resultClass == ResultDone)
                if (handleCliDisassemblerResult(response.consoleStreamOutput, ac.agent))
                    return;
            // 'point, plain' can take far too long.
            // Skip this feature and immediately fall back to the 'range' version:
            fetchDisassemblerByCliRangeMixed(ac);
        });
}

void GdbEngine::fetchDisassemblerByCliRangeMixed(const DisassemblerAgentCookie &ac)
{
    QTC_ASSERT(ac.agent, return);
    const quint64 address = ac.agent->address();
    QByteArray start = QByteArray::number(address - 20, 16);
    QByteArray end = QByteArray::number(address + 100, 16);
    QByteArray cmd = "disassemble /rm 0x" + start + ",0x" + end;
    postCommand(cmd, Discardable|ConsoleCommand,
        [this, ac](const DebuggerResponse &response) {
            if (response.resultClass == ResultDone)
                if (handleCliDisassemblerResult(response.consoleStreamOutput, ac.agent))
                    return;
            fetchDisassemblerByCliRangePlain(ac);
        });
}


void GdbEngine::fetchDisassemblerByCliRangePlain(const DisassemblerAgentCookie &ac0)
{
    DisassemblerAgentCookie ac = ac0;
    QTC_ASSERT(ac.agent, return);
    const quint64 address = ac.agent->address();
    QByteArray start = QByteArray::number(address - 20, 16);
    QByteArray end = QByteArray::number(address + 100, 16);
    QByteArray cmd = "disassemble /r 0x" + start + ",0x" + end;
    postCommand(cmd, Discardable,
        [this, ac](const DebuggerResponse &response) {
            if (response.resultClass == ResultDone)
                if (handleCliDisassemblerResult(response.consoleStreamOutput, ac.agent))
                    return;
            // Finally, give up.
            //76^error,msg="No function contains program counter for selected..."
            //76^error,msg="No function contains specified address."
            //>568^error,msg="Line number 0 out of range;
            QByteArray msg = response.data["msg"].data();
            showStatusMessage(tr("Disassembler failed: %1")
                .arg(QString::fromLocal8Bit(msg)), 5000);
        });
}

struct LineData
{
    LineData() {}
    LineData(int i, int f) : index(i), function(f) {}
    int index;
    int function;
};

bool GdbEngine::handleCliDisassemblerResult(const QByteArray &output, DisassemblerAgent *agent)
{
    QTC_ASSERT(agent, return true);
    // First line is something like
    // "Dump of assembler code from 0xb7ff598f to 0xb7ff5a07:"
    DisassemblerLines dlines;
    foreach (const QByteArray &line, output.split('\n'))
        dlines.appendUnparsed(_(line));

    QVector<DisassemblerLine> lines = dlines.data();

    typedef QMap<quint64, LineData> LineMap;
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

// Binary/configuration check logic.

static QString gdbBinary(const DebuggerStartParameters &sp)
{
    // 1) Environment.
    const QByteArray envBinary = qgetenv("QTC_DEBUGGER_PATH");
    if (!envBinary.isEmpty())
        return QString::fromLocal8Bit(envBinary);
    // 2) Command from profile.
    return sp.debuggerCommand;
}

static SourcePathMap mergeStartParametersSourcePathMap(const DebuggerStartParameters &sp,
                                                       const SourcePathMap &in)
{
    // Do not overwrite user settings.
    SourcePathMap rc = sp.sourcePathMap;
    for (auto it = in.constBegin(), end = in.constEnd(); it != end; ++it)
        rc.insert(it.key(), it.value());
    return rc;
}

//
// Starting up & shutting down
//

void GdbEngine::startGdb(const QStringList &args)
{
    const QByteArray tests = qgetenv("QTC_DEBUGGER_TESTS");
    foreach (const QByteArray &test, tests.split(','))
        m_testCases.insert(test.toInt());
    foreach (int test, m_testCases)
        showMessage(_("ENABLING TEST CASE: " + QByteArray::number(test)));

    m_gdbProc->disconnect(); // From any previous runs

    const DebuggerStartParameters &sp = startParameters();
    m_gdb = gdbBinary(sp);
    if (m_gdb.isEmpty()) {
        handleGdbStartFailed();
        handleAdapterStartFailed(
            msgNoGdbBinaryForToolChain(sp.toolChainAbi),
            Constants::DEBUGGER_COMMON_SETTINGS_ID);
        return;
    }
    QStringList gdbArgs;
    gdbArgs << _("-i");
    gdbArgs << _("mi");
    if (!boolSetting(LoadGdbInit))
        gdbArgs << _("-n");
    gdbArgs += args;

    connect(m_gdbProc, &GdbProcess::error, this, &GdbEngine::handleGdbError);
    connect(m_gdbProc, &GdbProcess::finished, this, &GdbEngine::handleGdbFinished);
    connect(m_gdbProc, &GdbProcess::readyReadStandardOutput, this, &GdbEngine::readGdbStandardOutput);
    connect(m_gdbProc, &GdbProcess::readyReadStandardError, this, &GdbEngine::readGdbStandardError);

    showMessage(_("STARTING ") + m_gdb + _(" ") + gdbArgs.join(QLatin1Char(' ')));
    m_gdbProc->start(m_gdb, gdbArgs);

    if (!m_gdbProc->waitForStarted()) {
        handleGdbStartFailed();
        const QString msg = errorMessage(QProcess::FailedToStart);
        handleAdapterStartFailed(msg);
        return;
    }

    showMessage(_("GDB STARTED, INITIALIZING IT"));
    postCommand("show version", NoFlags, CB(handleShowVersion));
    //postCommand("-list-features", CB(handleListFeatures));
    postCommand("show debug-file-directory", NoFlags, CB(handleDebugInfoLocation));

    //postCommand("-enable-timings");
    //postCommand("set print static-members off"); // Seemingly doesn't work.
    //postCommand("set debug infrun 1");
    //postCommand("define hook-stop\n-thread-list-ids\n-stack-list-frames\nend");
    //postCommand("define hook-stop\nprint 4\nend");
    //postCommand("define hookpost-stop\nprint 5\nend");
    //postCommand("define hook-call\nprint 6\nend");
    //postCommand("define hookpost-call\nprint 7\nend");
    postCommand("set print object on");
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
    //postCommand("set overload-resolution off");

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

    postCommand("set unwindonsignal on");
    postCommand("set width 0");
    postCommand("set height 0");

    // FIXME: Provide proper Gui settings for these:
    //postCommand("set breakpoint always-inserted on", ConsoleCommand);
    // displaced-stepping does not work in Thumb mode.
    //postCommand("set displaced-stepping on");
    //postCommand("set trust-readonly-sections on", ConsoleCommand);
    //postCommand("set remotecache on", ConsoleCommand);
    //postCommand("set non-stop on", ConsoleCommand);

    showStatusMessage(tr("Setting up inferior..."));

    // Addint executable to modules list.
    Module module;
    module.startAddress = 0;
    module.endAddress = 0;
    module.modulePath = sp.executable;
    module.moduleName = QLatin1String("<executable>");
    modulesHandler()->updateModule(module);

    // Apply source path mappings from global options.
    //showMessage(_("Assuming Qt is installed at %1").arg(qtInstallPath));
    const SourcePathMap sourcePathMap =
        DebuggerSourcePathMappingWidget::mergePlatformQtPath(sp,
                Internal::globalDebuggerOptions()->sourcePathMap);
    const SourcePathMap completeSourcePathMap =
            mergeStartParametersSourcePathMap(sp, sourcePathMap);
    for (auto it = completeSourcePathMap.constBegin(), cend = completeSourcePathMap.constEnd();
         it != cend;
         ++it) {
        postCommand("set substitute-path " + it.key().toLocal8Bit()
            + " " + it.value().toLocal8Bit());
    }

    // Spaces just will not work.
    foreach (const QString &src, sp.debugSourceLocation) {
        if (QDir(src).exists())
            postCommand("directory " + src.toLocal8Bit());
        else
            showMessage(_("# directory does not exist: ") + src, LogInput);
    }

    const QByteArray sysroot = sp.sysRoot.toLocal8Bit();
    if (!sysroot.isEmpty()) {
        postCommand("set sysroot " + sysroot);
        // sysroot is not enough to correctly locate the sources, so explicitly
        // relocate the most likely place for the debug source
        postCommand("set substitute-path /usr/src " + sysroot + "/usr/src");
    }

    //QByteArray ba = QFileInfo(sp.dumperLibrary).path().toLocal8Bit();
    //if (!ba.isEmpty())
    //    postCommand("set solib-search-path " + ba);
    if (attemptQuickStart()) {
        postCommand("set auto-solib-add off", ConsoleCommand);
    } else {
        m_fullStartDone = true;
        postCommand("set auto-solib-add on", ConsoleCommand);
    }

    if (boolSetting(MultiInferior)) {
        //postCommand("set follow-exec-mode new");
        postCommand("set detach-on-fork off");
    }

    // Finally, set up Python.
    // We need to guarantee a roundtrip before the adapter proceeds.
    // Make sure this stays the last command in startGdb().
    // Don't use ConsoleCommand, otherwise Mac won't markup the output.
    const QByteArray dumperSourcePath =
        ICore::resourcePath().toLocal8Bit() + "/debugger/";

    if (terminal()->isUsable())
        postCommand("set inferior-tty " + terminal()->slaveDevice());

    const QFileInfo gdbBinaryFile(m_gdb);
    const QByteArray uninstalledData = gdbBinaryFile.absolutePath().toLocal8Bit()
            + "/data-directory/python";

    const GdbCommandFlags flags = ConsoleCommand | Immediate;
    postCommand("python sys.path.insert(1, '" + dumperSourcePath + "')", flags);
    postCommand("python sys.path.append('" + uninstalledData + "')", flags);
    postCommand("python from gdbbridge import *", flags);

    const QString path = stringSetting(ExtraDumperFile);
    if (!path.isEmpty()) {
        DebuggerCommand cmd("addDumperModule");
        cmd.arg("path", path.toUtf8());
        runCommand(cmd);
    }

    const QString commands = stringSetting(ExtraDumperCommands);
    if (!commands.isEmpty())
        postCommand(commands.toLocal8Bit(), flags);

    runCommand(DebuggerCommand("loadDumpers", flags, CB(handlePythonSetup)));
}

void GdbEngine::handleGdbStartFailed()
{
}

void GdbEngine::loadInitScript()
{
    const QString script = startParameters().overrideStartScript;
    if (!script.isEmpty()) {
        if (QFileInfo(script).isReadable()) {
            postCommand("source " + script.toLocal8Bit());
        } else {
            AsynchronousMessageBox::warning(
            tr("Cannot find debugger initialization script"),
            tr("The debugger settings point to a script file at \"%1\" "
               "which is not accessible. If a script file is not needed, "
               "consider clearing that entry to avoid this warning. "
              ).arg(script));
        }
    } else {
        const QString commands = stringSetting(GdbStartupCommands);
        if (!commands.isEmpty())
            postCommand(commands.toLocal8Bit());
    }
}

void GdbEngine::reloadDebuggingHelpers()
{
    runCommand("reloadDumpers");
    reloadLocals();
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
        //m_gdbProc->kill();
        //notifyEngineIll();
        AsynchronousMessageBox::critical(tr("GDB I/O Error"), msg);
        break;
    }
}

void GdbEngine::handleGdbFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (m_commandTimer.isActive())
        m_commandTimer.stop();

    notifyDebuggerProcessFinished(exitCode, exitStatus, QLatin1String("GDB"));
}

void GdbEngine::abortDebugger()
{
    if (targetState() == DebuggerFinished) {
        // We already tried. Try harder.
        showMessage(_("ABORTING DEBUGGER. SECOND TIME."));
        QTC_ASSERT(m_gdbProc, return);
        m_gdbProc->kill();
    } else {
        // Be friendly the first time. This will change targetState().
        showMessage(_("ABORTING DEBUGGER. FIRST TIME."));
        quitDebugger();
    }
}

void GdbEngine::resetInferior()
{
    if (!startParameters().commandsForReset.isEmpty()) {
        QByteArray commands = globalMacroExpander()->expand(startParameters().commandsForReset);
        foreach (QByteArray command, commands.split('\n')) {
            command = command.trimmed();
            if (!command.isEmpty()) {
                if (state() == InferiorStopOk) {
                    postCommand(command, ConsoleCommand|Immediate);
                } else {
                    DebuggerCommand cmd(command);
                    cmd.flags = ConsoleCommand;
                    m_commandsToRunOnTemporaryBreak.append(cmd);
                }
            }
        }
    }
    requestInterruptInferior();
    runEngine();
}

void GdbEngine::handleAdapterStartFailed(const QString &msg, Id settingsIdHint)
{
    CHECK_STATE(EngineSetupOk);
    showMessage(_("ADAPTER START FAILED"));
    if (!msg.isEmpty()) {
        const QString title = tr("Adapter start failed");
        if (!settingsIdHint.isValid()) {
            ICore::showWarningWithOptions(title, msg);
        } else {
            ICore::showWarningWithOptions(title, msg, QString(), settingsIdHint);
        }
    }
    notifyEngineSetupFailed();
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
    const DebuggerStartParameters &sp = startParameters();

    CHECK_STATE(InferiorSetupRequested);

    if (!sp.commandsAfterConnect.isEmpty()) {
        QByteArray commands = globalMacroExpander()->expand(sp.commandsAfterConnect);
        foreach (QByteArray command, commands.split('\n')) {
            postCommand(command);
        }
    }

    //postCommand("set follow-exec-mode new");
    if (sp.breakOnMain) {
        QByteArray cmd = "tbreak ";
        cmd += sp.toolChainAbi.os() == Abi::WindowsOS ? "qMain" : "main";
        postCommand(cmd);
    }

    // Initial attempt to set breakpoints.
    if (sp.startMode != AttachCore) {
        showStatusMessage(tr("Setting breakpoints..."));
        showMessage(tr("Setting breakpoints..."));
        attemptBreakpointSynchronization();
    }

    if (m_commandForToken.isEmpty()) {
        finishInferiorSetup();
    } else {
        QTC_CHECK(m_commandsDoneCallback == 0);
        m_commandsDoneCallback = &GdbEngine::finishInferiorSetup;
    }
}

void GdbEngine::finishInferiorSetup()
{
    CHECK_STATE(InferiorSetupRequested);

    if (startParameters().startMode == AttachCore) {
        notifyInferiorSetupOk(); // No breakpoints in core files.
    } else {
        if (boolSetting(BreakOnAbort))
            postCommand("-break-insert -f abort");
        if (boolSetting(BreakOnWarning)) {
            postCommand("-break-insert -f '" + qtNamespace() + "qWarning'");
            postCommand("-break-insert -f '" + qtNamespace() + "QMessageLogger::warning'");
        }
        if (boolSetting(BreakOnFatal)) {
            postCommand("-break-insert -f '" + qtNamespace() + "qFatal'", NoFlags,
                        [this](const DebuggerResponse &r) { handleBreakOnQFatal(r, false); });
            postCommand("-break-insert -f '" + qtNamespace() + "QMessageLogger::fatal'", NoFlags,
                        [this](const DebuggerResponse &r) { handleBreakOnQFatal(r, true); });
        } else {
            notifyInferiorSetupOk();
        }
    }
}

void GdbEngine::handleDebugInfoLocation(const DebuggerResponse &response)
{
    if (response.resultClass == ResultDone) {
        const QByteArray debugInfoLocation = startParameters().debugInfoLocation.toLocal8Bit();
        if (QFile::exists(QString::fromLocal8Bit(debugInfoLocation))) {
            const QByteArray curDebugInfoLocations = response.consoleStreamOutput.split('"').value(1);
            if (curDebugInfoLocations.isEmpty()) {
                postCommand("set debug-file-directory " + debugInfoLocation);
            } else {
                postCommand("set debug-file-directory " + debugInfoLocation
                        + HostOsInfo::pathListSeparator().toLatin1()
                        + curDebugInfoLocations);
            }
        }
    }
}

void GdbEngine::handleBreakOnQFatal(const DebuggerResponse &response, bool continueSetup)
{
    if (response.resultClass == ResultDone) {
        GdbMi bkpt = response.data["bkpt"];
        GdbMi number = bkpt["number"];
        BreakpointResponseId rid(number.data());
        if (rid.isValid()) {
            m_qFatalBreakpointResponseId = rid;
            postCommand("-break-commands " + number.data() + " return");
        }
    }

    // Continue setup.
    if (continueSetup)
        notifyInferiorSetupOk();
}

void GdbEngine::notifyInferiorSetupFailed(const QString &msg)
{
    showStatusMessage(tr("Failed to start application:") + QLatin1Char(' ') + msg);
    if (state() == EngineSetupFailed) {
        showMessage(_("INFERIOR START FAILED, BUT ADAPTER DIED ALREADY"));
        return; // Adapter crashed meanwhile, so this notification is meaningless.
    }
    showMessage(_("INFERIOR START FAILED"));
    AsynchronousMessageBox::critical(tr("Failed to start application"), msg);
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
    if (state() == EngineSetupRequested)
        notifyEngineSetupFailed();
    else
        notifyEngineIll();

    // No point in being friendly here ...
    m_gdbProc->kill();

    if (!msg.isEmpty())
        AsynchronousMessageBox::critical(tr("Adapter crashed"), msg);
}

void GdbEngine::createFullBacktrace()
{
    postCommand("thread apply all bt full",
        NeedsStop|ConsoleCommand, CB(handleCreateFullBacktrace));
}

void GdbEngine::handleCreateFullBacktrace(const DebuggerResponse &response)
{
    if (response.resultClass == ResultDone) {
        Internal::openTextEditor(_("Backtrace $"),
            _(response.consoleStreamOutput + response.logStreamOutput));
    }
}

void GdbEngine::resetCommandQueue()
{
    m_commandTimer.stop();
    if (!m_commandForToken.isEmpty()) {
        QString msg;
        QTextStream ts(&msg);
        ts << "RESETING COMMAND QUEUE. LEFT OVER TOKENS: ";
        foreach (const DebuggerCommand &cmd, m_commandForToken)
            ts << "CMD:" << cmd.function;
        m_commandForToken.clear();
        showMessage(msg);
    }
}

bool GdbEngine::isQFatalBreakpoint(const BreakpointResponseId &id) const
{
    return id.isValid() && m_qFatalBreakpointResponseId == id;
}

bool GdbEngine::isHiddenBreakpoint(const BreakpointResponseId &id) const
{
    return isQFatalBreakpoint(id);
}

bool GdbEngine::usesExecInterrupt() const
{
    DebuggerStartMode mode = startParameters().startMode;
    return (mode == AttachToRemoteServer || mode == AttachToRemoteProcess)
        && boolSetting(TargetAsync);
}

void GdbEngine::scheduleTestResponse(int testCase, const QByteArray &response)
{
    if (!m_testCases.contains(testCase) && startParameters().testCase != testCase)
        return;

    int token = currentToken() + 1;
    showMessage(_("SCHEDULING TEST RESPONSE (CASE: %1, TOKEN: %2, RESPONSE: %3)")
        .arg(testCase).arg(token).arg(_(response)));
    m_scheduledTestResponses[token] = response;
}

void GdbEngine::requestDebugInformation(const DebugInfoTask &task)
{
    QProcess::startDetached(task.command);
}

bool GdbEngine::attemptQuickStart() const
{
    // Don't try if the user does not ask for it.
    if (!boolSetting(AttemptQuickStart))
        return false;

    // Don't try if there are breakpoints we might be able to handle.
    BreakHandler *handler = breakHandler();
    foreach (Breakpoint bp, handler->unclaimedBreakpoints()) {
        if (acceptsBreakpoint(bp))
            return false;
    }

    return true;
}

void GdbEngine::write(const QByteArray &data)
{
    m_gdbProc->write(data);
}

bool GdbEngine::prepareCommand()
{
    if (HostOsInfo::isWindowsHost()) {
        DebuggerStartParameters &sp = startParameters();
        QtcProcess::SplitError perr;
        sp.processArgs = QtcProcess::prepareArgs(sp.processArgs, &perr,
                                                 HostOsInfo::hostOs(),
                    &sp.environment, &sp.workingDirectory).toWindowsArgs();
        if (perr != QtcProcess::SplitOk) {
            // perr == BadQuoting is never returned on Windows
            // FIXME? QTCREATORBUG-2809
            handleAdapterStartFailed(QCoreApplication::translate("DebuggerEngine", // Same message in CdbEngine
                                                                 "Debugging complex command lines is currently not supported on Windows."), Id());
            return false;
        }
    }
    return true;
}

QString GdbEngine::msgGdbStopFailed(const QString &why)
{
    return tr("The gdb process could not be stopped:\n%1").arg(why);
}

QString GdbEngine::msgInferiorStopFailed(const QString &why)
{
    return tr("Application process could not be stopped:\n%1").arg(why);
}

QString GdbEngine::msgInferiorSetupOk()
{
    return tr("Application started");
}

QString GdbEngine::msgInferiorRunOk()
{
    return tr("Application running");
}

QString GdbEngine::msgAttachedToStoppedInferior()
{
    return tr("Attached to stopped application");
}

QString GdbEngine::msgConnectRemoteServerFailed(const QString &why)
{
    return tr("Connecting to remote server failed:\n%1").arg(why);
}

void GdbEngine::interruptLocalInferior(qint64 pid)
{
    QTC_ASSERT(state() == InferiorStopRequested, qDebug() << state(); return);
    if (pid <= 0) {
        showMessage(QLatin1String("TRYING TO INTERRUPT INFERIOR BEFORE PID WAS OBTAINED"), LogError);
        return;
    }
    QString errorMessage;
    if (interruptProcess(pid, GdbEngineType, &errorMessage)) {
        showMessage(QLatin1String("Interrupted ") + QString::number(pid));
    } else {
        showMessage(errorMessage, LogError);
        notifyInferiorStopFailed();
    }
}

QByteArray GdbEngine::dotEscape(QByteArray str)
{
    str.replace(' ', '.');
    str.replace('\\', '.');
    str.replace('/', '.');
    return str;
}

void GdbEngine::debugLastCommand()
{
    runCommand(m_lastDebuggableCommand);
}

//
// Factory
//

DebuggerEngine *createGdbEngine(const DebuggerStartParameters &sp)
{
    switch (sp.startMode) {
    case AttachCore:
        return new GdbCoreEngine(sp);
    case StartRemoteProcess:
    case AttachToRemoteServer:
        return new GdbRemoteServerEngine(sp);
    case AttachExternal:
        return new GdbAttachEngine(sp);
    default:
        if (sp.useTerminal)
            return new GdbTermEngine(sp);
        return new GdbPlainEngine(sp);
    }
}

void addGdbOptionPages(QList<IOptionsPage *> *opts)
{
    opts->push_back(new GdbOptionsPage());
    opts->push_back(new GdbOptionsPage2());
}

void GdbEngine::doUpdateLocals(const UpdateParameters &params)
{
    m_pendingBreakpointRequests = 0;

    DebuggerCommand cmd("showData");
    watchHandler()->appendFormatRequests(&cmd);

    cmd.arg("stringcutoff", action(MaximalStringLength)->value().toByteArray());
    cmd.arg("displaystringlimit", action(DisplayStringLimit)->value().toByteArray());

    // Re-create tooltip items that are not filters on existing local variables in
    // the tooltip model.
    cmd.beginList("watchers");
    DebuggerToolTipContexts toolTips = DebuggerToolTipManager::pendingTooltips(this);
    foreach (const DebuggerToolTipContext &p, toolTips) {
        cmd.beginGroup();
        cmd.arg("iname", p.iname);
        cmd.arg("exp", p.expression.toLatin1().toHex());
        cmd.endGroup();
    }

    QHashIterator<QByteArray, int> it(WatchHandler::watcherNames());
    while (it.hasNext()) {
        it.next();
        cmd.beginGroup();
        cmd.arg("iname", "watch." + QByteArray::number(it.value()));
        cmd.arg("exp", it.key().toHex());
        cmd.endGroup();
    }
    cmd.endList();

    const static bool alwaysVerbose = !qgetenv("QTC_DEBUGGER_PYTHON_VERBOSE").isEmpty();

    cmd.arg("passExceptions", alwaysVerbose);
    cmd.arg("fancy", boolSetting(UseDebuggingHelpers));
    cmd.arg("autoderef", boolSetting(AutoDerefPointers));
    cmd.arg("dyntype", boolSetting(UseDynamicType));
    cmd.arg("nativemixed", isNativeMixedActive());

    if (isNativeMixedActive()) {
        StackFrame frame = stackHandler()->currentFrame();
        if (frame.language == QmlLanguage)
            cmd.arg("qmlcontext", "0x" + QByteArray::number(frame.address, 16));
    }

    cmd.arg("resultvarname", m_resultVarName);
    cmd.arg("partialVariable", params.partialVariable);
    cmd.flags = Discardable;
    cmd.callback = [this, params](const DebuggerResponse &r) { handleStackFramePython(r); };
    runCommand(cmd);

    cmd.arg("passExceptions", true);
    m_lastDebuggableCommand = cmd;
}

void GdbEngine::handleStackFramePython(const DebuggerResponse &response)
{
    watchHandler()->notifyUpdateFinished();
    if (response.resultClass == ResultDone) {
        QByteArray out = response.consoleStreamOutput;
        while (out.endsWith(' ') || out.endsWith('\n'))
            out.chop(1);
        int pos = out.indexOf("data=");
        if (pos != 0) {
            showMessage(_("DISCARDING JUNK AT BEGIN OF RESPONSE: "
                + out.left(pos)));
            out = out.mid(pos);
        }
        GdbMi all;
        all.fromStringMultiple(out);

        updateLocalsView(all);

    } else {
        showMessage(_("DUMPER FAILED: " + response.toString()));
    }
}

QString GdbEngine::msgPtraceError(DebuggerStartMode sm)
{
    if (sm == StartInternal) {
        return QCoreApplication::translate("QtDumperHelper",
            "ptrace: Operation not permitted.\n\n"
            "Could not attach to the process. "
            "Make sure no other debugger traces this process.\n"
            "Check the settings of\n"
            "/proc/sys/kernel/yama/ptrace_scope\n"
            "For more details, see /etc/sysctl.d/10-ptrace.conf\n");
    }
    return QCoreApplication::translate("QtDumperHelper",
        "ptrace: Operation not permitted.\n\n"
        "Could not attach to the process. "
        "Make sure no other debugger traces this process.\n"
        "If your uid matches the uid\n"
        "of the target process, check the settings of\n"
        "/proc/sys/kernel/yama/ptrace_scope\n"
        "For more details, see /etc/sysctl.d/10-ptrace.conf\n");
}

} // namespace Internal
} // namespace Debugger

Q_DECLARE_METATYPE(Debugger::Internal::GdbMi)
