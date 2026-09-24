// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "jdwpimpl.h"

#include "../breakpoint.h"
#include "../debuggerconstants.h"
#include "../debuggertr.h"

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/qtcassert.h>
#include <utils/shutdownguard.h>

#include <QRegularExpression>
#include <QTcpSocket>

#include <cstring>
#include <limits>

using namespace Utils;

namespace Debugger::Internal {

// Where a step does not stop, as jdb has it: the runtime's own code.
static const QStringList s_stepExcludes = {"java.*", "javax.*", "sun.*", "jdk.*", "com.sun.*"};

static constexpr qint32 s_classPreparedStatus = 2;

static GdbMi constMi(const QString &name, const QString &data)
{
    GdbMi mi;
    mi.m_type = GdbMi::Const;
    mi.m_name = name;
    mi.m_data = data;
    return mi;
}

// "Lpkg/Outer$Inner;" is pkg.Outer$Inner, "[I" is int[].
static QString typeName(const QString &signature)
{
    qsizetype dimensions = 0;
    while (dimensions < signature.size() && signature.at(dimensions) == '[')
        ++dimensions;
    const QString base = signature.mid(dimensions);
    QString name;
    if (base.startsWith('L') && base.endsWith(';')) {
        name = base.mid(1, base.size() - 2).replace('/', '.');
    } else if (base.size() == 1) {
        switch (base.at(0).toLatin1()) {
        case 'Z': name = "boolean"; break;
        case 'B': name = "byte"; break;
        case 'C': name = "char"; break;
        case 'S': name = "short"; break;
        case 'I': name = "int"; break;
        case 'J': name = "long"; break;
        case 'F': name = "float"; break;
        case 'D': name = "double"; break;
        case 'V': name = "void"; break;
        default: name = base; break;
        }
    } else {
        name = base;
    }
    for (qsizetype i = 0; i < dimensions; ++i)
        name += "[]";
    return name;
}

// The directory of the package a class signature names, "pkg/sub" for
// "Lpkg/sub/Name;".
static QString packagePath(const QString &signature)
{
    if (!signature.startsWith('L'))
        return {};
    const qsizetype slash = signature.lastIndexOf('/');
    return slash < 0 ? QString() : signature.mid(1, slash - 1);
}

// The name of the outermost class, "Outer" for "Lpkg/Outer$Inner;".
static QString outerClassName(const QString &signature)
{
    const qsizetype slash = signature.lastIndexOf('/');
    QString name = signature.mid(slash < 0 ? 1 : slash + 1);
    name.chop(1);
    return name.section('$', 0, 0);
}

static QString quoted(const QString &text)
{
    QString result = text;
    result.replace('\\', "\\\\").replace('"', "\\\"").replace('\n', "\\n")
        .replace('\r', "\\r").replace('\t', "\\t");
    return '"' + result + '"';
}

static QString primitiveText(const JdwpValue &value)
{
    switch (value.tag) {
    case Jdwp::BooleanValueTag:
        return value.bits ? QString("true") : QString("false");
    case Jdwp::ByteValueTag:
        return QString::number(qint8(value.bits));
    case Jdwp::ShortValueTag:
        return QString::number(qint16(value.bits));
    case Jdwp::IntValueTag:
        return QString::number(qint32(value.bits));
    case Jdwp::LongValueTag:
        return QString::number(qint64(value.bits));
    case Jdwp::CharValueTag: {
        const QChar c(char16_t(value.bits));
        if (c.isPrint())
            return QString("'%1'").arg(c);
        return QString("'\\u%1'").arg(uint(value.bits), 4, 16, QChar('0'));
    }
    case Jdwp::FloatValueTag: {
        const quint32 bits = quint32(value.bits);
        float f = 0;
        std::memcpy(&f, &bits, sizeof(f));
        return QString::number(f, 'g', 9);
    }
    case Jdwp::DoubleValueTag: {
        const quint64 bits = value.bits;
        double d = 0;
        std::memcpy(&d, &bits, sizeof(d));
        return QString::number(d, 'g', 17);
    }
    case Jdwp::VoidValueTag:
        return "void";
    }
    return {};
}

static QString threadStatusText(int status)
{
    switch (status) {
    case Jdwp::ThreadZombie: return Tr::tr("Terminated");
    case Jdwp::ThreadRunning: return Tr::tr("Running");
    case Jdwp::ThreadSleeping: return Tr::tr("Sleeping");
    case Jdwp::ThreadMonitor: return Tr::tr("Waiting for a monitor");
    case Jdwp::ThreadWait: return Tr::tr("Waiting");
    }
    return {};
}

static DebuggerEngineSetupData jdwpImplSetupData()
{
    DebuggerEngineSetupData data;
    data.capabilities = RunToLineCapability;
    data.extraCapabilities = DebuggerExtraCapability::Threads
                           | DebuggerExtraCapability::ThreadEvent;
    data.startModes = DebuggerStartModeFlag::Launch | DebuggerStartModeFlag::AttachToRemoteServer;
    data.toolTipHandling = ToolTipHandling::IfStoppedInferior;
    data.acceptsBreakpoint = [](const AcceptsBreakpointQuery &query) {
        return query.type == BreakpointByFileAndLine && query.fileName.suffix() == "java";
    };
    return data;
}

void JdwpImpl::Join::release()
{
    if (--m_pending > 0 || !m_done)
        return;
    const Done done = std::move(m_done);
    m_done = {};
    done();
}

JdwpImpl::JdwpImpl(const JdwpImplStartData &startData)
    : DebuggerEngineInterface(jdwpImplSetupData())
    , m_startData(startData)
    , m_process(std::make_unique<Process>())
{
    connect(&m_client, &JdwpClient::logMessage, this, [this](const QString &text, bool outgoing) {
        emit message(text, outgoing ? LogInput : LogOutput);
    });
    connect(&m_client, &JdwpClient::connected, this, &JdwpImpl::handleConnected);
    connect(&m_client, &JdwpClient::disconnected, this, &JdwpImpl::handleDisconnected);
    connect(&m_client, &JdwpClient::eventSetReceived, this, &JdwpImpl::handleEventSet);

    connect(&m_server, &QTcpServer::newConnection, this, [this] {
        QTcpSocket *socket = m_server.nextPendingConnection();
        m_server.close();
        if (socket)
            m_client.setSocket(socket);
    });

    connect(m_process.get(), &Process::started, this, [this] {
        emit inferiorPidKnown(ProcessHandle(m_process->processId()));
    });
    connect(m_process.get(), &Process::readyReadStandardOutput, this, [this] {
        emit message(m_process->readAllStandardOutput(), AppOutput);
    });
    connect(m_process.get(), &Process::readyReadStandardError, this, [this] {
        emit message(m_process->readAllStandardError(), AppError);
    });
    connect(m_process.get(), &Process::done, this, &JdwpImpl::handleProcessDone);
}

JdwpImpl::~JdwpImpl()
{
    m_client.disconnect(this);
    if (m_process) {
        m_process->disconnect(this);
        if (m_process->isRunning())
            m_process->kill();
    }
}

bool JdwpImpl::isLaunch() const
{
    return std::holds_alternative<ProcessRunData>(m_startData.inferiorStartData);
}

void JdwpImpl::start()
{
    if (isLaunch()) {
        launch();
        return;
    }
    if (std::holds_alternative<AttachToRemoteServerData>(m_startData.inferiorStartData)) {
        attach();
        return;
    }
    emit message(Tr::tr("A Java virtual machine can only be launched, or attached to at the "
                        "port its debug agent listens on."), LogError);
    emit inferiorEvent(InferiorEvent::EngineSetupFailed);
}

void JdwpImpl::launch()
{
    const auto &runData = std::get<ProcessRunData>(m_startData.inferiorStartData);
    if (!m_server.listen(QHostAddress::LocalHost)) {
        emit message(Tr::tr("Cannot open a port for the debug agent to connect to: %1")
                         .arg(m_server.errorString()), LogError);
        emit inferiorEvent(InferiorEvent::EngineSetupFailed);
        return;
    }

    // The agent connects to us rather than listening itself, so there is no
    // port to agree on and no output to wait for.
    const QString agent = QString("-agentlib:jdwp=transport=dt_socket,server=n,suspend=y,"
                                  "address=127.0.0.1:%1").arg(m_server.serverPort());
    CommandLine command = runData.command;
    Environment environment = runData.environment;
    const QString launcher = command.executable().baseName();
    if (launcher == "java" || launcher == "javaw") {
        command = CommandLine(runData.command.executable(), {agent});
        command.addArgs(runData.command.arguments(), CommandLine::Raw);
    } else {
        // Whatever starts the virtual machine in the end, a script for
        // instance, the virtual machine takes the agent from the environment.
        const QString previous = environment.value("JAVA_TOOL_OPTIONS");
        environment.set("JAVA_TOOL_OPTIONS", previous.isEmpty() ? agent : previous + ' ' + agent);
    }

    m_process->setCommand(command);
    m_process->setEnvironment(environment);
    if (runData.workingDirectory.isDir())
        m_process->setWorkingDirectory(runData.workingDirectory);
    emit message(command.toUserOutput(), LogInput);
    m_process->start();
}

void JdwpImpl::attach()
{
    const QString channel
        = std::get<AttachToRemoteServerData>(m_startData.inferiorStartData).channel;
    const qsizetype colon = channel.lastIndexOf(':');
    bool ok = false;
    const quint16 port = channel.mid(colon + 1).toUShort(&ok);
    if (!ok || port == 0) {
        emit message(Tr::tr("\"%1\" names no port to attach to.").arg(channel), LogError);
        emit inferiorEvent(InferiorEvent::EngineSetupFailed);
        return;
    }
    QString host = colon > 0 ? channel.left(colon) : QString("localhost");
    if (host.startsWith('[') && host.endsWith(']'))
        host = host.mid(1, host.size() - 2);
    emit message(QString("Attaching to %1:%2").arg(host).arg(port), LogInput);
    m_client.connectToHost(host, port);
}

void JdwpImpl::handleConnected()
{
    for (const quint8 kind : {Jdwp::ThreadStartEvent, Jdwp::ThreadDeathEvent}) {
        send(Jdwp::EventRequestSet, Jdwp::EventRequestSetCommand,
             writer().writeByte(kind).writeByte(Jdwp::SuspendNone).writeInt(0).data());
    }

    // The breakpoints the engine holds go out as it hears this, ahead of the
    // resume below, so the classes they are in are caught as they load.
    m_setupReported = true;
    emit inferiorEvent(InferiorEvent::EngineSetupOk);
    m_running = true;
    m_runReported = true;
    emit inferiorEvent(InferiorEvent::RunAndInferiorRunOk);
    if (std::exchange(m_suspendedForSetup, false))
        send(Jdwp::VirtualMachineSet, Jdwp::VmResume);
}

void JdwpImpl::handleDisconnected(const QString &reason)
{
    emit message(reason, LogMisc);
    if (!m_setupReported) {
        // A launch hears about the failure from the virtual machine exiting.
        if (!isLaunch()) {
            emit message(reason, LogError);
            emit inferiorEvent(InferiorEvent::EngineSetupFailed);
        }
        return;
    }
    if (m_shuttingDown) {
        if (!isLaunch())
            reportShutdownFinished();
        return;
    }
    if (!m_detaching && !isLaunch())
        reportInferiorDone({});
}

void JdwpImpl::handleProcessDone()
{
    if (m_shutdownReported)
        return;
    if (!m_setupReported) {
        m_server.close();
        emit message(Tr::tr("The Java virtual machine exited before its debug agent connected: "
                            "%1").arg(m_process->verboseExitMessage()), LogError);
        emit inferiorEvent(InferiorEvent::EngineSetupFailed);
        return;
    }
    m_client.close();
    if (m_shuttingDown) {
        reportShutdownFinished();
        return;
    }
    if (m_detaching)
        return;
    const bool crashed = m_process->exitStatus() == ProcessExitStatus::CrashExit;
    reportInferiorDone({m_process->exitCode(),
                        crashed ? InferiorExitStatus::Crash : InferiorExitStatus::Normal});
}

// The virtual machine outlives a detach, and with it the process that runs it.
void JdwpImpl::releaseProcess()
{
    if (!m_process || !m_process->isRunning())
        return;
    Process *process = m_process.release();
    process->disconnect(this);
    process->setParent(Utils::shutdownGuard());
    connect(process, &Process::readyReadStandardOutput, process,
            [process] { process->readAllRawStandardOutput(); });
    connect(process, &Process::readyReadStandardError, process,
            [process] { process->readAllRawStandardError(); });
    connect(process, &Process::done, process, &QObject::deleteLater);
}

void JdwpImpl::reportInferiorDone(const InferiorResultData &result)
{
    if (std::exchange(m_inferiorDoneReported, true))
        return;
    m_running = false;
    emit inferiorDone(result);
}

void JdwpImpl::reportShutdownFinished()
{
    if (std::exchange(m_shutdownReported, true))
        return;
    emit inferiorEvent(InferiorEvent::ShutdownFinished);
}

void JdwpImpl::reportUnsupported(const QString &what)
{
    emit message(Tr::tr("The Java debugger does not support %1.").arg(what), LogWarning);
}

void JdwpImpl::send(quint8 commandSet, quint8 command, const QByteArray &data,
                    const JdwpClient::ReplyHandler &handler)
{
    if (!m_client.isConnected()) {
        // There is nothing left to ask, and the handler hears that rather than
        // never running: everything here waits for one, and a chain that is
        // never released waits for as long as the session lasts.
        if (handler)
            handler({Jdwp::VmDeadError, {}});
        return;
    }
    m_client.send(commandSet, command, data, handler);
}

void JdwpImpl::shutdownInferior(ShutdownMode mode)
{
    m_shuttingDown = true;
    if (mode == ShutdownMode::Detach) {
        if (m_client.isConnected()) {
            send(Jdwp::VirtualMachineSet, Jdwp::VmDispose, {}, [this](const JdwpReply &) {
                releaseProcess();
                m_client.close();
                reportShutdownFinished();
            });
            return;
        }
        releaseProcess();
        reportShutdownFinished();
        return;
    }
    if (m_process && m_process->isRunning()) {
        m_process->kill();
        return;
    }
    if (!isLaunch() && m_client.isConnected()) {
        // The virtual machine may be gone before it answers, which ends the
        // shutdown the same way.
        send(Jdwp::VirtualMachineSet, Jdwp::VmExit, writer().writeInt(0).data(),
             [this](const JdwpReply &) {
            m_client.close();
            reportShutdownFinished();
        });
        return;
    }
    reportShutdownFinished();
}

void JdwpImpl::shutdownEngine()
{
    m_shutdownReported = true;
    m_client.close();
    m_server.close();
    if (m_process && m_process->isRunning())
        m_process->kill();
    emit inferiorEvent(InferiorEvent::EngineShutdownFinished);
}

void JdwpImpl::execute(const ExecutionRequest &request)
{
    if (!m_client.isConnected() || m_inferiorDoneReported) {
        if (request.command == ExecutionCommand::Continue)
            emit inferiorEvent(InferiorEvent::InferiorIll);
        return;
    }

    switch (request.command) {
    case ExecutionCommand::Continue:
        m_runRequestPending = true;
        emit inferiorEvent(InferiorEvent::RunRequested);
        resumeFromStop();
        return;
    case ExecutionCommand::Interrupt:
        if (!m_running) {
            emit inferiorEvent(InferiorEvent::StopOk);
            return;
        }
        m_interruptRequested = true;
        send(Jdwp::VirtualMachineSet, Jdwp::VmSuspend, {}, [this](const JdwpReply &reply) {
            if (!reply.ok()) {
                m_interruptRequested = false;
                emit inferiorEvent(InferiorEvent::StopFailed);
                return;
            }
            // A stop that came in the meantime is what the interrupt is
            // answered with, and the suspension is given back with it.
            if (m_running)
                stopAfterInterrupt();
            else
                ++m_extraSuspends;
        });
        return;
    case ExecutionCommand::StepIn:
        step(Jdwp::StepInto, request.flag);
        return;
    case ExecutionCommand::StepOver:
        step(Jdwp::StepOver, request.flag);
        return;
    case ExecutionCommand::StepOut:
        step(Jdwp::StepOut, false);
        return;
    case ExecutionCommand::RunToLine:
        runToLine(request.context);
        return;
    case ExecutionCommand::Detach:
        m_detaching = true;
        send(Jdwp::VirtualMachineSet, Jdwp::VmDispose, {}, [this](const JdwpReply &) {
            releaseProcess();
            m_client.close();
            reportInferiorDone({0, InferiorExitStatus::Detached});
        });
        return;
    case ExecutionCommand::Abort:
        if (m_process && m_process->isRunning())
            m_process->kill();
        else
            send(Jdwp::VirtualMachineSet, Jdwp::VmExit, writer().writeInt(1).data());
        return;
    case ExecutionCommand::RunToFunction:
        reportUnsupported(Tr::tr("running to a function"));
        return;
    case ExecutionCommand::JumpToLine:
        reportUnsupported(Tr::tr("jumping to a line"));
        return;
    case ExecutionCommand::Return:
        reportUnsupported(Tr::tr("returning from a function"));
        return;
    case ExecutionCommand::ResetInferior:
        reportUnsupported(Tr::tr("restarting the program"));
        return;
    case ExecutionCommand::RecordReverse:
        reportUnsupported(Tr::tr("reverse execution"));
        return;
    case ExecutionCommand::RepeatLastCommand:
        return;
    }
}

void JdwpImpl::resumeFromStop()
{
    ++m_stopGeneration;
    m_frames.clear();
    m_framesComplete = false;
    const int suspensions = 1 + std::exchange(m_extraSuspends, 0);

    if (!m_pendingStops.isEmpty()) {
        // The stop that is next holds a suspension of its own, so the virtual
        // machine stays where it is, and that stop is reported right away.
        for (int i = 0; i < suspensions; ++i)
            send(Jdwp::VirtualMachineSet, Jdwp::VmResume);
        const JdwpEvent next = m_pendingStops.takeFirst();
        QMetaObject::invokeMethod(this, [this, next] {
            reportStopAt(next.thread, next.location);
        }, Qt::QueuedConnection);
        return;
    }

    m_running = true;
    for (int i = 1; i < suspensions; ++i)
        send(Jdwp::VirtualMachineSet, Jdwp::VmResume);
    send(Jdwp::VirtualMachineSet, Jdwp::VmResume, {}, [this](const JdwpReply &reply) {
        if (reply.ok()) {
            reportRunOk();
        } else if (std::exchange(m_runRequestPending, false)) {
            m_running = false;
            emit inferiorEvent(InferiorEvent::RunFailed);
        }
    });
}

void JdwpImpl::reportRunOk()
{
    if (std::exchange(m_runRequestPending, false))
        emit inferiorEvent(InferiorEvent::RunOk);
}

void JdwpImpl::step(Jdwp::StepDepth depth, bool byInstruction)
{
    if (m_currentThread == 0) {
        reportUnsupported(Tr::tr("stepping without a current thread"));
        return;
    }
    m_runRequestPending = true;
    emit inferiorEvent(InferiorEvent::RunRequested);
    if (m_pendingStops.isEmpty()) {
        JdwpWriter request = writer();
        request.writeByte(Jdwp::SingleStepEvent).writeByte(Jdwp::SuspendAll)
            .writeInt(qint32(1 + s_stepExcludes.size()));
        request.writeByte(Jdwp::StepModifier).writeObjectId(m_currentThread)
            .writeInt(byInstruction ? Jdwp::StepMin : Jdwp::StepLine).writeInt(depth);
        for (const QString &pattern : s_stepExcludes)
            request.writeByte(Jdwp::ClassExcludeModifier).writeString(pattern);
        // The answer comes ahead of anything the resume below leads to.
        send(Jdwp::EventRequestSet, Jdwp::EventRequestSetCommand, request.data(),
             [this](const JdwpReply &reply) {
            if (reply.ok())
                m_stepRequests.append(reader(reply.data).readInt());
        });
    }
    resumeFromStop();
}

void JdwpImpl::runToLine(const ContextData &context)
{
    m_runRequestPending = true;
    emit inferiorEvent(InferiorEvent::RunRequested);

    BreakpointParameters params(BreakpointByFileAndLine);
    params.fileName = context.fileName;
    params.textPosition = context.textPosition;
    Breakpoint bp;
    bp.number = QString("run%1").arg(m_nextBreakpointNumber++);
    bp.internal = true;
    updateFromBreakpointRequest(bp, params);
    const QString number = bp.number;
    m_breakpoints.insert(number, bp);
    armBreakpoint(number, [this] { resumeFromStop(); });
}

void JdwpImpl::clearRequest(quint8 eventKind, qint32 requestId, const Done &done)
{
    send(Jdwp::EventRequestSet, Jdwp::EventRequestClear,
         writer().writeByte(eventKind).writeInt(requestId).data(),
         [done](const JdwpReply &) {
        if (done)
            done();
    });
}

// A step is over once anything stops the virtual machine, and so is a run to
// a line.
void JdwpImpl::clearTransientRequests()
{
    for (const qint32 id : std::exchange(m_stepRequests, {}))
        clearRequest(Jdwp::SingleStepEvent, id);
    const QStringList internal = Utils::filtered(m_breakpoints.keys(), [this](const QString &nr) {
        return m_breakpoints.value(nr).internal;
    });
    for (const QString &number : internal) {
        disarmBreakpoint(number, {});
        m_breakpoints.remove(number);
    }
}

void JdwpImpl::handleEventSet(const JdwpEventSet &set)
{
    const bool suspended = set.suspendPolicy != Jdwp::SuspendNone;
    QList<JdwpEvent> classPrepares;
    std::optional<JdwpEvent> stop;

    for (const JdwpEvent &event : set.events) {
        switch (event.kind) {
        case Jdwp::VmDeathEvent:
            emit message(Tr::tr("The Java virtual machine is shutting down."), LogMisc);
            break;
        case Jdwp::ThreadStartEvent:
        case Jdwp::ThreadDeathEvent: {
            GdbMi data;
            data.m_type = GdbMi::Tuple;
            data.addChild(constMi("id", QString::number(event.thread)));
            emit threadEvent(event.kind == Jdwp::ThreadStartEvent ? ThreadEvent::Created
                                                                  : ThreadEvent::Exited, data);
            break;
        }
        case Jdwp::ClassPrepareEvent:
            classPrepares.append(event);
            break;
        case Jdwp::BreakpointEvent:
        case Jdwp::SingleStepEvent:
            if (!stop)
                stop = event;
            break;
        default:
            break;
        }
    }

    if (stop) {
        // The stop holds the suspension the set came with, and gives it back
        // when it is over.
        const JdwpEvent event = *stop;
        handleClassPrepares(classPrepares, [this, event] {
            reportStop(event.thread, event.location);
        });
        return;
    }

    // However many events a set holds, it came with one suspension, and that
    // one is given back once, after everything in it has been dealt with. A
    // virtual machine that waits for us at its start is let go only when the
    // breakpoints are in, and then it is that resume which gives it back.
    // What holds it there is the suspension, not the start event in it: an
    // event that could not be read must not let the machine run off either.
    const bool waitsForSetup = suspended && !m_setupReported;
    if (waitsForSetup)
        m_suspendedForSetup = true;
    const bool resumes = suspended && !waitsForSetup;
    if (!classPrepares.isEmpty()) {
        handleClassPrepares(classPrepares, [this, resumes] {
            if (resumes)
                send(Jdwp::VirtualMachineSet, Jdwp::VmResume);
        });
        return;
    }
    // Nothing here asks for the machine to stay where it is.
    if (resumes)
        send(Jdwp::VirtualMachineSet, Jdwp::VmResume);
}

void JdwpImpl::reportStop(quint64 thread, const JdwpLocation &location)
{
    if (!m_running) {
        JdwpEvent event;
        event.thread = thread;
        event.location = location;
        m_pendingStops.append(event);
        return;
    }
    reportStopAt(thread, location);
}

void JdwpImpl::reportStopAt(quint64 thread, const JdwpLocation &location)
{
    m_running = false;
    ++m_stopGeneration;
    m_currentThread = thread;
    m_currentFrame = 0;
    m_frames.clear();
    m_framesComplete = false;
    // A step can end before the answer to its resume has been read.
    reportRunOk();
    clearTransientRequests();

    const bool requested = std::exchange(m_interruptRequested, false);
    const int generation = m_stopGeneration;
    resolveLocation(location, [this, requested, generation](const ResolvedLocation &resolved) {
        if (generation != m_stopGeneration)
            return;
        if (resolved.line > 0 && resolved.file.exists())
            emit locationChanged(resolved.file, resolved.line);
        emit inferiorEvent(requested ? InferiorEvent::StopOk : InferiorEvent::SpontaneousStop);
    });
}

// What an interrupt stops in is not said by the virtual machine, so the
// thread it is reported in is the one that was current before, or else main.
void JdwpImpl::stopAfterInterrupt()
{
    m_running = false;
    send(Jdwp::VirtualMachineSet, Jdwp::VmAllThreads, {}, [this](const JdwpReply &reply) {
        QList<quint64> threads;
        if (reply.ok()) {
            JdwpReader r = reader(reply.data);
            const qint32 count = r.readInt();
            for (qint32 i = 0; i < count && r.ok(); ++i)
                threads.append(r.readObjectId());
        }

        const auto stopIn = [this](quint64 thread) {
            if (thread == 0) {
                reportStopAt(0, {});
                return;
            }
            JdwpWriter request = writer();
            request.writeObjectId(thread).writeInt(0).writeInt(1);
            send(Jdwp::ThreadReferenceSet, Jdwp::ThreadFrames, request.data(),
                 [this, thread](const JdwpReply &reply) {
                JdwpLocation location;
                if (reply.ok()) {
                    JdwpReader r = reader(reply.data);
                    if (r.readInt() > 0) {
                        r.readFrameId();
                        location = r.readLocation();
                    }
                }
                reportStopAt(thread, location);
            });
        };

        if (threads.contains(m_currentThread)) {
            stopIn(m_currentThread);
            return;
        }
        auto names = std::make_shared<QHash<quint64, QString>>();
        const auto join = std::make_shared<Join>([threads, names, stopIn] {
            const quint64 main = Utils::findOrDefault(threads, [names](quint64 thread) {
                return names->value(thread) == "main";
            });
            stopIn(main != 0 ? main : threads.value(0));
        });
        join->add();
        for (const quint64 thread : std::as_const(threads)) {
            join->add();
            send(Jdwp::ThreadReferenceSet, Jdwp::ThreadName,
                 writer().writeObjectId(thread).data(),
                 [this, thread, names, join](const JdwpReply &reply) {
                if (reply.ok())
                    names->insert(thread, reader(reply.data).readString());
                join->release();
            });
        }
        join->release();
    });
}

void JdwpImpl::fetchOnce(const QString &key, const std::function<bool()> &known,
                         const std::function<void(const Done &finish)> &fetch, const Done &done)
{
    if (known()) {
        done();
        return;
    }
    QList<Done> &waiting = m_waiting[key];
    waiting.append(done);
    if (waiting.size() > 1)
        return;
    fetch([this, key] {
        const QList<Done> waiters = m_waiting.take(key);
        for (const Done &waiter : waiters)
            waiter();
    });
}

void JdwpImpl::withSignature(quint64 classId, const Done &done)
{
    fetchOnce(QString("signature:%1").arg(classId),
              [this, classId] {
        const auto it = m_classes.constFind(classId);
        return it != m_classes.cend() && it->signatureKnown;
    }, [this, classId](const Done &finish) {
        send(Jdwp::ReferenceTypeSet, Jdwp::RefTypeSignature,
             writer().writeReferenceTypeId(classId).data(),
             [this, classId, finish](const JdwpReply &reply) {
            ClassInfo &info = m_classes[classId];
            info.signatureKnown = true;
            if (reply.ok())
                info.signature = reader(reply.data).readString();
            finish();
        });
    }, done);
}

void JdwpImpl::withSourceFile(quint64 classId, const Done &done)
{
    fetchOnce(QString("source:%1").arg(classId),
              [this, classId] {
        const auto it = m_classes.constFind(classId);
        return it != m_classes.cend() && it->sourceFileKnown;
    }, [this, classId](const Done &finish) {
        send(Jdwp::ReferenceTypeSet, Jdwp::RefTypeSourceFile,
             writer().writeReferenceTypeId(classId).data(),
             [this, classId, finish](const JdwpReply &reply) {
            ClassInfo &info = m_classes[classId];
            info.sourceFileKnown = true;
            if (reply.ok())
                info.sourceFile = reader(reply.data).readString();
            finish();
        });
    }, done);
}

void JdwpImpl::withMethods(quint64 classId, const Done &done)
{
    fetchOnce(QString("methods:%1").arg(classId),
              [this, classId] {
        const auto it = m_classes.constFind(classId);
        return it != m_classes.cend() && it->methodsKnown;
    }, [this, classId](const Done &finish) {
        send(Jdwp::ReferenceTypeSet, Jdwp::RefTypeMethodsWithGeneric,
             writer().writeReferenceTypeId(classId).data(),
             [this, classId, finish](const JdwpReply &reply) {
            ClassInfo &info = m_classes[classId];
            info.methodsKnown = true;
            if (reply.ok()) {
                JdwpReader r = reader(reply.data);
                const qint32 count = r.readInt();
                for (qint32 i = 0; i < count && r.ok(); ++i) {
                    MethodInfo method;
                    method.id = r.readMethodId();
                    method.name = r.readString();
                    method.signature = r.readString();
                    r.readString(); // generic signature
                    method.modBits = r.readInt();
                    if (r.ok())
                        info.methods.append(method);
                }
            }
            finish();
        });
    }, done);
}

const JdwpImpl::MethodInfo *JdwpImpl::method(quint64 classId, quint64 methodId) const
{
    const auto it = m_classes.constFind(classId);
    if (it == m_classes.cend())
        return nullptr;
    for (const MethodInfo &method : it->methods) {
        if (method.id == methodId)
            return &method;
    }
    return nullptr;
}

void JdwpImpl::withLineTable(quint64 classId, quint64 methodId, const Done &done)
{
    withMethods(classId, [this, classId, methodId, done] {
        fetchOnce(QString("lines:%1:%2").arg(classId).arg(methodId),
                  [this, classId, methodId] {
            const MethodInfo *info = method(classId, methodId);
            return !info || info->lineTableKnown;
        }, [this, classId, methodId](const Done &finish) {
            send(Jdwp::MethodSet, Jdwp::MethodLineTable,
                 writer().writeReferenceTypeId(classId).writeMethodId(methodId).data(),
                 [this, classId, methodId, finish](const JdwpReply &reply) {
                for (MethodInfo &info : m_classes[classId].methods) {
                    if (info.id != methodId)
                        continue;
                    info.lineTableKnown = true;
                    if (!reply.ok())
                        break;
                    JdwpReader r = reader(reply.data);
                    r.readLong(); // start
                    r.readLong(); // end
                    const qint32 count = r.readInt();
                    for (qint32 i = 0; i < count && r.ok(); ++i) {
                        const quint64 index = quint64(r.readLong());
                        const int line = r.readInt();
                        if (r.ok())
                            info.lines.append({index, line});
                    }
                    std::sort(info.lines.begin(), info.lines.end());
                    break;
                }
                finish();
            });
        }, done);
    });
}

void JdwpImpl::withVariables(quint64 classId, quint64 methodId, const Done &done)
{
    withMethods(classId, [this, classId, methodId, done] {
        fetchOnce(QString("variables:%1:%2").arg(classId).arg(methodId),
                  [this, classId, methodId] {
            const MethodInfo *info = method(classId, methodId);
            return !info || info->variablesKnown;
        }, [this, classId, methodId](const Done &finish) {
            send(Jdwp::MethodSet, Jdwp::MethodVariableTableWithGeneric,
                 writer().writeReferenceTypeId(classId).writeMethodId(methodId).data(),
                 [this, classId, methodId, finish](const JdwpReply &reply) {
                for (MethodInfo &info : m_classes[classId].methods) {
                    if (info.id != methodId)
                        continue;
                    info.variablesKnown = true;
                    // Code compiled without -g has no table, and so no locals.
                    if (!reply.ok())
                        break;
                    JdwpReader r = reader(reply.data);
                    r.readInt(); // argument count
                    const qint32 count = r.readInt();
                    for (qint32 i = 0; i < count && r.ok(); ++i) {
                        MethodInfo::Variable variable;
                        variable.codeIndex = quint64(r.readLong());
                        variable.name = r.readString();
                        variable.signature = r.readString();
                        r.readString(); // generic signature
                        variable.length = r.readInt();
                        variable.slot = r.readInt();
                        if (r.ok())
                            info.variables.append(variable);
                    }
                    break;
                }
                finish();
            });
        }, done);
    });
}

void JdwpImpl::withFields(quint64 classId, const Done &done)
{
    fetchOnce(QString("fields:%1").arg(classId),
              [this, classId] {
        const auto it = m_classes.constFind(classId);
        return it != m_classes.cend() && it->fieldsKnown;
    }, [this, classId](const Done &finish) {
        send(Jdwp::ReferenceTypeSet, Jdwp::RefTypeFieldsWithGeneric,
             writer().writeReferenceTypeId(classId).data(),
             [this, classId, finish](const JdwpReply &reply) {
            QList<FieldInfo> declared;
            QList<FieldInfo> statics;
            if (reply.ok()) {
                JdwpReader r = reader(reply.data);
                const qint32 count = r.readInt();
                for (qint32 i = 0; i < count && r.ok(); ++i) {
                    FieldInfo field;
                    field.id = r.readFieldId();
                    field.name = r.readString();
                    field.signature = r.readString();
                    r.readString(); // generic signature
                    field.modBits = r.readInt();
                    if (!r.ok())
                        continue;
                    if (field.modBits & Jdwp::StaticModifierBit)
                        statics.append(field);
                    else
                        declared.append(field);
                }
            }
            // An interface has no superclass, and the question is refused.
            send(Jdwp::ClassTypeSet, Jdwp::ClassTypeSuperclass,
                 writer().writeReferenceTypeId(classId).data(),
                 [this, classId, declared, statics, finish](const JdwpReply &reply) {
                const quint64 superclass = reply.ok()
                                               ? reader(reply.data).readReferenceTypeId() : 0;
                const auto store = [this, classId, declared, statics, finish](
                                       const QList<FieldInfo> &inherited,
                                       const QList<FieldInfo> &inheritedStatics) {
                    ClassInfo &info = m_classes[classId];
                    info.fieldsKnown = true;
                    info.fields = inherited + declared;
                    // A static of a superclass goes by its plain name here as
                    // it does in Java, and the closest declaration wins.
                    info.staticFields = statics + inheritedStatics;
                    finish();
                };
                if (superclass == 0) {
                    store({}, {});
                    return;
                }
                withFields(superclass, [this, superclass, store] {
                    const ClassInfo info = m_classes.value(superclass);
                    store(info.fields, info.staticFields);
                });
            });
        });
    }, done);
}

void JdwpImpl::resolveLocation(const JdwpLocation &location,
                               const std::function<void(const ResolvedLocation &)> &done)
{
    if (!location.isValid()) {
        done({});
        return;
    }
    const quint64 classId = location.classId;
    withSignature(classId, [this, location, classId, done] {
        withSourceFile(classId, [this, location, classId, done] {
            withLineTable(classId, location.methodId, [this, location, classId, done] {
                const ClassInfo &info = m_classes[classId];
                ResolvedLocation resolved;
                const MethodInfo *where = method(classId, location.methodId);
                resolved.function = typeName(info.signature) + '.'
                                    + (where ? where->name : QString("?"));
                resolved.file = localFile(info.signature, info.sourceFile);
                if (where) {
                    for (const auto &[index, line] : where->lines) {
                        if (index > location.index)
                            break;
                        resolved.line = line;
                    }
                }
                done(resolved);
            });
        });
    });
}

FilePath JdwpImpl::localFile(const QString &signature, const QString &sourceFile) const
{
    if (sourceFile.isEmpty())
        return {};
    const QString package = packagePath(signature);
    const QString relative = package.isEmpty() ? sourceFile : package + '/' + sourceFile;
    if (const FilePath known = m_knownSources.value(relative); !known.isEmpty())
        return known;
    const FilePaths matches = Utils::filtered(m_startData.sourceFiles, [&](const FilePath &file) {
        return file.path().endsWith('/' + relative);
    });
    // A class in the default package names no directory, so a file of the
    // same name in any package matches as well.
    if (matches.size() == 1 || (!matches.isEmpty() && !package.isEmpty()))
        return matches.first();

    QList<FilePath> roots = m_startData.sourceSearchPaths;
    if (isLaunch())
        roots.append(std::get<ProcessRunData>(m_startData.inferiorStartData).workingDirectory);
    for (const FilePath &root : std::as_const(roots)) {
        if (root.isEmpty())
            continue;
        if (const FilePath candidate = root / relative; candidate.exists())
            return candidate;
        if (const FilePath candidate = root / sourceFile; candidate.exists())
            return candidate;
    }
    // A relative name would be resolved against the debugger's own location
    // further up, which is somewhere the file is not.
    return {};
}

JdwpImpl::Breakpoint *JdwpImpl::breakpoint(const QString &number)
{
    const auto it = m_breakpoints.find(number);
    return it == m_breakpoints.end() ? nullptr : &*it;
}

GdbMi JdwpImpl::breakpointData(const Breakpoint &bp) const
{
    GdbMi bkpt;
    bkpt.m_type = GdbMi::Tuple;
    bkpt.addChild(constMi("number", bp.number));
    bkpt.addChild(constMi("file", bp.file.path()));
    bkpt.addChild(constMi("fullname", bp.file.path()));
    bkpt.addChild(constMi("line", QString::number(bp.actualLine ? bp.actualLine : bp.line)));
    bkpt.addChild(constMi("enabled", bp.enabled ? "y" : "n"));
    bkpt.addChild(constMi("pending", bp.locations.isEmpty() ? "1" : "0"));
    return bkpt;
}

void JdwpImpl::updateFromBreakpointRequest(Breakpoint &bp, const BreakpointParameters &params)
{
    bp.file = params.fileName;
    bp.sourceName = params.fileName.fileName();
    bp.line = params.textPosition.line;
    bp.enabled = params.enabled;
    bp.packageKnown = false;
    bp.packagePath.clear();
    // The class signatures name the package, and the file says which one it
    // belongs to. A file that cannot be read matches its name in any package.
    if (const Result<QByteArray> contents = params.fileName.fileContents()) {
        static const QRegularExpression packageStatement(R"(^\s*package\s+([\w.]+)\s*;)",
                                                         QRegularExpression::MultilineOption);
        const QRegularExpressionMatch match
            = packageStatement.match(QString::fromUtf8(*contents));
        bp.packagePath = match.hasMatch() ? match.captured(1).replace('.', '/') : QString();
        bp.packageKnown = true;
        m_knownSources.insert(bp.packagePath.isEmpty() ? bp.sourceName
                                                       : bp.packagePath + '/' + bp.sourceName,
                              bp.file);
    }
}

void JdwpImpl::changeBreakpoint(const BreakpointChangeRequest &request)
{
    const quint64 requestId = request.requestId;
    switch (request.op) {
    case BreakpointOp::Insert: {
        if (request.params.type != BreakpointByFileAndLine) {
            emit breakpointEvent(requestId, BreakpointOp::Insert, false);
            return;
        }
        Breakpoint bp;
        bp.number = QString::number(m_nextBreakpointNumber++);
        updateFromBreakpointRequest(bp, request.params);
        const QString number = bp.number;
        m_breakpoints.insert(number, bp);
        armBreakpoint(number, [this, number, requestId] {
            const Breakpoint *bp = breakpoint(number);
            GdbMi data;
            data.m_type = GdbMi::List;
            if (bp)
                data.addChild(breakpointData(*bp));
            emit breakpointEvent(requestId, BreakpointOp::Insert, bp != nullptr, data);
        });
        return;
    }
    case BreakpointOp::Remove: {
        const QString number = request.responseId;
        if (!breakpoint(number)) {
            emit breakpointEvent(requestId, BreakpointOp::Remove, false);
            return;
        }
        disarmBreakpoint(number, [this, number, requestId] {
            m_breakpoints.remove(number);
            emit breakpointEvent(requestId, BreakpointOp::Remove, true);
        });
        return;
    }
    case BreakpointOp::Update: {
        const QString number = request.responseId;
        if (!breakpoint(number)) {
            emit breakpointEvent(requestId, BreakpointOp::Update, false);
            return;
        }
        const BreakpointParameters params = request.params;
        disarmBreakpoint(number, [this, number, requestId, params] {
            Breakpoint *bp = breakpoint(number);
            if (!bp) {
                emit breakpointEvent(requestId, BreakpointOp::Update, false);
                return;
            }
            // A change names the breakpoint and says little else about it.
            BreakpointParameters merged = params;
            if (merged.fileName.isEmpty())
                merged.fileName = bp->file;
            if (merged.textPosition.line <= 0)
                merged.textPosition.line = bp->line;
            updateFromBreakpointRequest(*bp, merged);
            armBreakpoint(number, [this, requestId] {
                emit breakpointEvent(requestId, BreakpointOp::Update, true);
            });
        });
        return;
    }
    case BreakpointOp::EnableSub:
        emit breakpointEvent(requestId, BreakpointOp::EnableSub, false);
        return;
    }
}

void JdwpImpl::armBreakpoint(const QString &number, const Done &done)
{
    const Breakpoint *bp = breakpoint(number);
    QTC_ASSERT(bp, done(); return);

    const auto join = std::make_shared<Join>([this, number, done] {
        if (Breakpoint *bp = breakpoint(number))
            bp->answered = true;
        done();
    });
    join->add();

    // Classes that load later are caught as they do. A virtual machine that
    // cannot filter by source file is asked for the whole package instead: a
    // file holds more than the class it is named after, and the source files
    // sort the answers out below.
    const QString baseName = bp->file.completeBaseName();
    JdwpWriter request = writer();
    request.writeByte(Jdwp::ClassPrepareEvent).writeByte(Jdwp::SuspendAll);
    const QString package = bp->packageKnown ? QString(bp->packagePath).replace('/', '.')
                                             : QString();
    if (m_client.hasCapability(Jdwp::CanUseSourceNameFilters)) {
        request.writeInt(1);
        request.writeByte(Jdwp::SourceNameMatchModifier).writeString(bp->sourceName);
    } else if (!package.isEmpty()) {
        request.writeInt(1);
        request.writeByte(Jdwp::ClassMatchModifier).writeString(package + ".*");
    } else {
        // A class match takes one wildcard, at one end, so no pattern covers
        // the class in any package and its nested ones at once. Every class
        // that loads is heard instead, but for those of the platform, and the
        // names sort them out below.
        static const QStringList platform = {"java.*", "javax.*", "jdk.*", "sun.*",
                                             "com.sun.*"};
        request.writeInt(platform.size());
        for (const QString &pattern : platform)
            request.writeByte(Jdwp::ClassExcludeModifier).writeString(pattern);
    }
    join->add();
    send(Jdwp::EventRequestSet, Jdwp::EventRequestSetCommand, request.data(),
         [this, number, join](const JdwpReply &reply) {
        if (reply.ok()) {
            const qint32 id = reader(reply.data).readInt();
            if (Breakpoint *bp = breakpoint(number))
                bp->classPrepareRequest = id;
            else
                clearRequest(Jdwp::ClassPrepareEvent, id);
        }
        join->release();
    });

    // And the classes that are loaded already.
    join->add();
    send(Jdwp::VirtualMachineSet, Jdwp::VmAllClasses, {},
         [this, number, baseName, join](const JdwpReply &reply) {
        const Breakpoint *bp = breakpoint(number);
        if (!bp || !reply.ok()) {
            join->release();
            return;
        }
        QList<quint64> candidates;
        JdwpReader r = reader(reply.data);
        const qint32 count = r.readInt();
        for (qint32 i = 0; i < count && r.ok(); ++i) {
            const quint8 tag = r.readByte();
            const quint64 id = r.readReferenceTypeId();
            const QString signature = r.readString();
            const qint32 status = r.readInt();
            if (!r.ok())
                break;
            ClassInfo &info = m_classes[id];
            info.tag = tag;
            if (!info.signatureKnown) {
                info.signatureKnown = true;
                info.signature = signature;
            }
            if (tag == Jdwp::ArrayTag || !(status & s_classPreparedStatus))
                continue;
            if (bp->packageKnown ? packagePath(signature) != bp->packagePath
                                 : outerClassName(signature) != baseName) {
                continue;
            }
            candidates.append(id);
        }
        const QString sourceName = bp->sourceName;
        for (const quint64 id : std::as_const(candidates)) {
            join->add();
            withSourceFile(id, [this, number, id, sourceName, join] {
                if (m_classes.value(id).sourceFile == sourceName)
                    resolveInClass(number, id, [join] { join->release(); });
                else
                    join->release();
            });
        }
        join->release();
    });

    join->release();
}

void JdwpImpl::disarmBreakpoint(const QString &number, const Done &done)
{
    Breakpoint *bp = breakpoint(number);
    if (!bp) {
        if (done)
            done();
        return;
    }
    const auto join = std::make_shared<Join>([done] {
        if (done)
            done();
    });
    join->add();
    if (bp->classPrepareRequest != 0) {
        join->add();
        clearRequest(Jdwp::ClassPrepareEvent, bp->classPrepareRequest, [join] { join->release(); });
    }
    for (const qint32 id : std::as_const(bp->requests)) {
        join->add();
        clearRequest(Jdwp::BreakpointEvent, id, [join] { join->release(); });
    }
    bp->classPrepareRequest = 0;
    bp->requests.clear();
    bp->classes.clear();
    bp->locations.clear();
    bp->actualLine = 0;
    join->release();
}

// Every method of the class whose code is on the line gets a location, or on
// the next line with code where the line itself has none, as gdb does it.
void JdwpImpl::resolveInClass(const QString &number, quint64 classId, const Done &done)
{
    Breakpoint *bp = breakpoint(number);
    if (!bp || bp->classes.contains(classId)) {
        done();
        return;
    }
    bp->classes.insert(classId);

    withMethods(classId, [this, number, classId, done] {
        const auto join = std::make_shared<Join>([this, number, classId, done] {
            Breakpoint *bp = breakpoint(number);
            if (!bp) {
                done();
                return;
            }
            const ClassInfo &info = m_classes[classId];
            QList<JdwpLocation> found;
            const auto collect = [&info, &found, classId](int line) {
                for (const MethodInfo &method : info.methods) {
                    quint64 best = std::numeric_limits<quint64>::max();
                    for (const auto &[index, entryLine] : method.lines) {
                        if (entryLine == line && index < best)
                            best = index;
                    }
                    if (best != std::numeric_limits<quint64>::max())
                        found.append({info.tag, classId, method.id, best});
                }
            };
            int line = bp->line;
            collect(line);
            if (found.isEmpty()) {
                int next = std::numeric_limits<int>::max();
                for (const MethodInfo &method : info.methods) {
                    if (method.lines.isEmpty())
                        continue;
                    int first = std::numeric_limits<int>::max();
                    int last = 0;
                    for (const auto &entry : method.lines) {
                        first = qMin(first, entry.second);
                        last = qMax(last, entry.second);
                    }
                    if (bp->line < first || bp->line > last)
                        continue;
                    for (const auto &entry : method.lines) {
                        if (entry.second > bp->line)
                            next = qMin(next, entry.second);
                    }
                }
                if (next != std::numeric_limits<int>::max()) {
                    line = next;
                    collect(line);
                }
            }
            if (found.isEmpty()) {
                done();
                return;
            }
            if (bp->actualLine == 0)
                bp->actualLine = line;
            bp->locations += found;
            if (!bp->enabled) {
                done();
                return;
            }

            const auto setJoin = std::make_shared<Join>(done);
            setJoin->add();
            for (const JdwpLocation &location : std::as_const(found)) {
                JdwpWriter request = writer();
                request.writeByte(Jdwp::BreakpointEvent).writeByte(Jdwp::SuspendAll).writeInt(1)
                    .writeByte(Jdwp::LocationOnlyModifier).writeLocation(location);
                setJoin->add();
                send(Jdwp::EventRequestSet, Jdwp::EventRequestSetCommand, request.data(),
                     [this, number, setJoin](const JdwpReply &reply) {
                    if (reply.ok()) {
                        const qint32 id = reader(reply.data).readInt();
                        // Taken back while the request was out, it must
                        // not stop anything.
                        if (Breakpoint *bp = breakpoint(number))
                            bp->requests.append(id);
                        else
                            clearRequest(Jdwp::BreakpointEvent, id);
                    }
                    setJoin->release();
                });
            }
            setJoin->release();
        });

        join->add();
        const QList<MethodInfo> methods = m_classes.value(classId).methods;
        for (const MethodInfo &method : methods) {
            join->add();
            withLineTable(classId, method.id, [join] { join->release(); });
        }
        join->release();
    });
}

void JdwpImpl::handleClassPrepares(const QList<JdwpEvent> &events, const Done &done)
{
    QStringList wasPending;
    for (const Breakpoint &bp : std::as_const(m_breakpoints)) {
        if (!bp.internal && bp.answered && bp.locations.isEmpty())
            wasPending.append(bp.number);
    }
    const auto join = std::make_shared<Join>([this, wasPending, done] {
        for (const QString &number : wasPending) {
            const Breakpoint *bp = breakpoint(number);
            if (!bp || bp->locations.isEmpty())
                continue;
            GdbMi data;
            data.m_type = GdbMi::List;
            data.addChild(breakpointData(*bp));
            emit breakpointModified(data);
        }
        done();
    });
    join->add();

    for (const JdwpEvent &event : events) {
        const quint64 classId = event.typeId;
        ClassInfo &info = m_classes[classId];
        info.tag = event.refTypeTag;
        info.signatureKnown = true;
        info.signature = event.signature;
        // By file rather than by request: a class can be prepared before the
        // answer that says which request is the breakpoint's has been read.
        for (const Breakpoint &bp : std::as_const(m_breakpoints)) {
            if (bp.packageKnown) {
                if (packagePath(event.signature) != bp.packagePath)
                    continue;
            } else if (!m_client.hasCapability(Jdwp::CanUseSourceNameFilters)
                       && outerClassName(event.signature) != bp.file.completeBaseName()) {
                // Unfiltered, so asking every class for its source file would
                // be a round trip per class that loads.
                continue;
            }
            const QString number = bp.number;
            join->add();
            withSourceFile(classId, [this, number, classId, join] {
                const Breakpoint *bp = breakpoint(number);
                if (bp && m_classes.value(classId).sourceFile == bp->sourceName)
                    resolveInClass(number, classId, [join] { join->release(); });
                else
                    join->release();
            });
        }
    }
    join->release();
}

void JdwpImpl::withFrames(int count, const Done &done)
{
    if (m_framesComplete || (count > 0 && m_frames.size() >= count)) {
        done();
        return;
    }
    const int generation = m_stopGeneration;
    const quint64 thread = m_currentThread;
    // Asking for more frames than there are is an error, so the count comes first.
    send(Jdwp::ThreadReferenceSet, Jdwp::ThreadFrameCount, writer().writeObjectId(thread).data(),
         [this, count, done, generation, thread](const JdwpReply &reply) {
        if (generation != m_stopGeneration) {
            // The frames belong to the stop that has since been left behind,
            // so nothing here is written down. The caller does hear back: it
            // asks about the generation itself, and what waits behind it
            // would wait for the rest of the session otherwise.
            done();
            return;
        }
        if (!reply.ok()) {
            // A thread that could not be asked says nothing about its stack,
            // and writing that down as the whole of it would keep every later
            // question from asking again.
            done();
            return;
        }
        const qint32 available = reader(reply.data).readInt();
        const qint32 wanted = count > 0 ? qMin(count, available) : available;
        if (wanted <= 0) {
            m_frames.clear();
            m_framesComplete = true;
            done();
            return;
        }
        JdwpWriter request = writer();
        request.writeObjectId(thread).writeInt(0).writeInt(wanted);
        send(Jdwp::ThreadReferenceSet, Jdwp::ThreadFrames, request.data(),
             [this, done, generation, available, wanted](const JdwpReply &reply) {
            if (generation != m_stopGeneration || !reply.ok()) {
                done();
                return;
            }
            m_frames.clear();
            JdwpReader r = reader(reply.data);
            const qint32 frameCount = r.readInt();
            for (qint32 i = 0; i < frameCount && r.ok(); ++i) {
                Frame frame;
                frame.id = r.readFrameId();
                frame.location = r.readLocation();
                if (r.ok())
                    m_frames.append(frame);
            }
            m_framesComplete = wanted >= available;
            done();
        });
    });
}

void JdwpImpl::reportStack(quint64 requestId, int depthLimit)
{
    const int generation = m_stopGeneration;
    const auto emitStack = [this, requestId](const GdbMi &frames) {
        GdbMi stack;
        stack.m_type = GdbMi::Tuple;
        stack.m_name = "stack";
        stack.addChild(frames);
        GdbMi all;
        all.m_type = GdbMi::Tuple;
        all.addChild(stack);
        emit refreshDataReceived(requestId, RefreshKind::FullStack, all);
    };
    if (m_running || m_currentThread == 0) {
        GdbMi frames;
        frames.m_type = GdbMi::List;
        frames.m_name = "frames";
        emitStack(frames);
        return;
    }

    withFrames(depthLimit, [this, generation, depthLimit, emitStack] {
        if (generation != m_stopGeneration)
            return;
        QList<Frame> frames = m_frames;
        if (depthLimit > 0 && frames.size() > depthLimit)
            frames = frames.mid(0, depthLimit);
        auto resolved = std::make_shared<QList<ResolvedLocation>>(frames.size());
        const auto join = std::make_shared<Join>([this, generation, resolved, emitStack] {
            if (generation != m_stopGeneration)
                return;
            GdbMi frameList;
            frameList.m_type = GdbMi::List;
            frameList.m_name = "frames";
            for (qsizetype level = 0; level < resolved->size(); ++level) {
                const ResolvedLocation &location = resolved->at(level);
                GdbMi frame;
                frame.m_type = GdbMi::Tuple;
                frame.addChild(constMi("level", QString::number(level)));
                frame.addChild(constMi("function", location.function));
                frame.addChild(constMi("file", location.file.path()));
                frame.addChild(constMi("fullname", location.file.path()));
                frame.addChild(constMi("line", QString::number(location.line)));
                frameList.addChild(frame);
            }
            emitStack(frameList);
        });
        join->add();
        for (qsizetype i = 0; i < frames.size(); ++i) {
            join->add();
            resolveLocation(frames.at(i).location,
                            [resolved, i, join](const ResolvedLocation &location) {
                (*resolved)[i] = location;
                join->release();
            });
        }
        join->release();
    });
}

void JdwpImpl::reportThreads(quint64 requestId)
{
    send(Jdwp::VirtualMachineSet, Jdwp::VmAllThreads, {},
         [this, requestId](const JdwpReply &reply) {
        QList<quint64> threads;
        if (reply.ok()) {
            JdwpReader r = reader(reply.data);
            const qint32 count = r.readInt();
            for (qint32 i = 0; i < count && r.ok(); ++i)
                threads.append(r.readObjectId());
        }
        auto names = std::make_shared<QHash<quint64, QString>>();
        auto states = std::make_shared<QHash<quint64, int>>();
        const auto join = std::make_shared<Join>([this, requestId, threads, names, states] {
            GdbMi list;
            list.m_type = GdbMi::List;
            list.m_name = "threads";
            for (const quint64 thread : threads) {
                GdbMi item;
                item.m_type = GdbMi::Tuple;
                item.addChild(constMi("id", QString::number(thread)));
                item.addChild(constMi("target-id", names->value(thread)));
                item.addChild(constMi("name", names->value(thread)));
                item.addChild(constMi("details", threadStatusText(states->value(thread, -1))));
                item.addChild(constMi("state", m_running ? "running" : "stopped"));
                list.addChild(item);
            }
            GdbMi all;
            all.m_type = GdbMi::Tuple;
            all.addChild(list);
            all.addChild(constMi("current-thread-id", QString::number(m_currentThread)));
            emit refreshDataReceived(requestId, RefreshKind::Threads, all);
        });
        join->add();
        for (const quint64 thread : std::as_const(threads)) {
            join->add();
            send(Jdwp::ThreadReferenceSet, Jdwp::ThreadName, writer().writeObjectId(thread).data(),
                 [this, thread, names, join](const JdwpReply &reply) {
                if (reply.ok())
                    names->insert(thread, reader(reply.data).readString());
                join->release();
            });
            join->add();
            send(Jdwp::ThreadReferenceSet, Jdwp::ThreadStatus,
                 writer().writeObjectId(thread).data(),
                 [this, thread, states, join](const JdwpReply &reply) {
                if (reply.ok())
                    states->insert(thread, reader(reply.data).readInt());
                join->release();
            });
        }
        join->release();
    });
}

void JdwpImpl::refresh(const RefreshRequest &request)
{
    switch (request.kind) {
    case RefreshKind::Locals:
        fetchLocals(request);
        return;
    case RefreshKind::FullStack:
        reportStack(request.requestId, request.stackDepthLimit);
        return;
    case RefreshKind::Threads:
        reportThreads(request.requestId);
        return;
    default:
        emit refreshDataReceived(request.requestId, request.kind, {});
        return;
    }
}

// The variables a frame can see where it stands, in the order they were declared.
QList<JdwpImpl::MethodInfo::Variable> JdwpImpl::visibleVariables(const JdwpLocation &location) const
{
    QList<MethodInfo::Variable> visible;
    const MethodInfo *where = method(location.classId, location.methodId);
    if (!where)
        return visible;
    for (const MethodInfo::Variable &variable : where->variables) {
        // javac lists it, too, and it is asked for on its own.
        if (variable.name == "this")
            continue;
        if (variable.codeIndex <= location.index
            && location.index < variable.codeIndex + quint64(variable.length)) {
            visible.append(variable);
        }
    }
    std::sort(visible.begin(), visible.end(), [](const auto &a, const auto &b) {
        return a.slot < b.slot;
    });
    return visible;
}

// Hands over the frame the views are showing, or nothing when the program is
// running or the stack does not reach that far.
void JdwpImpl::withCurrentFrame(const std::function<void(const Frame *)> &done)
{
    if (m_running || m_currentThread == 0 || !m_client.isConnected()) {
        done(nullptr);
        return;
    }
    const int frameIndex = m_currentFrame;
    const int stopGeneration = m_stopGeneration;
    withFrames(frameIndex + 1, [this, done, frameIndex, stopGeneration] {
        if (stopGeneration != m_stopGeneration || frameIndex >= m_frames.size()) {
            done(nullptr);
            return;
        }
        done(&m_frames.at(frameIndex));
    });
}

void JdwpImpl::evaluate(const JdwpExpression &expression, const Evaluation &done)
{
    const auto literal = [&done, &expression](quint8 tag, quint64 bits) {
        done({.value = {tag, bits}, .type = expression.typeName()});
    };
    switch (expression.kind) {
    case JdwpExpression::Kind::Int:
        literal(Jdwp::IntValueTag, quint64(expression.integer));
        return;
    case JdwpExpression::Kind::Long:
        literal(Jdwp::LongValueTag, quint64(expression.integer));
        return;
    case JdwpExpression::Kind::Boolean:
        literal(Jdwp::BooleanValueTag, quint64(expression.integer != 0));
        return;
    case JdwpExpression::Kind::Char:
        literal(Jdwp::CharValueTag, quint64(expression.integer));
        return;
    case JdwpExpression::Kind::Float: {
        const float value = float(expression.number);
        quint32 bits = 0;
        std::memcpy(&bits, &value, sizeof(value));
        literal(Jdwp::FloatValueTag, bits);
        return;
    }
    case JdwpExpression::Kind::Double: {
        const double value = expression.number;
        quint64 bits = 0;
        std::memcpy(&bits, &value, sizeof(value));
        literal(Jdwp::DoubleValueTag, bits);
        return;
    }
    case JdwpExpression::Kind::Null:
        literal(Jdwp::ObjectValueTag, 0);
        return;
    case JdwpExpression::Kind::String:
        // The machine holds the strings, so even a literal one has to be made there.
        send(Jdwp::VirtualMachineSet, Jdwp::VmCreateString,
             writer().writeString(expression.text).data(),
             [this, done](const JdwpReply &reply) {
            if (!reply.ok()) {
                done({.error = JdwpClient::errorString(reply.errorCode)});
                return;
            }
            done({.value = {Jdwp::StringValueTag, reader(reply.data).readObjectId()},
                  .type = "java.lang.String"});
        });
        return;
    case JdwpExpression::Kind::This:
        evaluateThis(done);
        return;
    case JdwpExpression::Kind::Name:
        evaluateName(expression.text, done);
        return;
    case JdwpExpression::Kind::Field: {
        const QString name = expression.text;
        evaluate(*expression.base, [this, name, done](const Evaluated &base) {
            if (!base.error.isEmpty()) {
                done(base);
                return;
            }
            fieldOf(base, name, done);
        });
        return;
    }
    case JdwpExpression::Kind::Index: {
        const JdwpExpression index = *expression.index;
        evaluate(*expression.base, [this, index, done](const Evaluated &base) {
            if (!base.error.isEmpty()) {
                done(base);
                return;
            }
            evaluate(index, [this, base, done](const Evaluated &position) {
                if (!position.error.isEmpty()) {
                    done(position);
                    return;
                }
                elementOf(base, position, done);
            });
        });
        return;
    }
    }
}

void JdwpImpl::evaluateThis(const Evaluation &done)
{
    withCurrentFrame([this, done](const Frame *frame) {
        if (!frame) {
            done({.error = Tr::tr("There is no frame to look in.")});
            return;
        }
        send(Jdwp::StackFrameSet, Jdwp::StackFrameThisObject,
             writer().writeObjectId(m_currentThread).writeFrameId(frame->id).data(),
             [this, done](const JdwpReply &reply) {
            if (!reply.ok()) {
                done({.error = JdwpClient::errorString(reply.errorCode)});
                return;
            }
            const JdwpValue self = reader(reply.data).readTaggedValue();
            if (!self.isObject() || self.isNull()) {
                done({.error = Tr::tr("The frame is of a static method, which has no \"this\".")});
                return;
            }
            done({.value = self});
        });
    });
}

// A name is a local variable of the frame first, then a field of its object,
// then a static field of its class, which is the order Java itself resolves.
void JdwpImpl::evaluateName(const QString &name, const Evaluation &done)
{
    withCurrentFrame([this, name, done](const Frame *frame) {
        if (!frame) {
            done({.error = Tr::tr("There is no frame to look in.")});
            return;
        }
        const JdwpLocation location = frame->location;
        const quint64 frameId = frame->id;
        const quint64 thread = m_currentThread;
        withVariables(location.classId, location.methodId,
                      [this, name, done, location, frameId, thread] {
            const QList<MethodInfo::Variable> visible = visibleVariables(location);
            const auto it = std::find_if(visible.cbegin(), visible.cend(),
                                         [&name](const MethodInfo::Variable &variable) {
                return variable.name == name;
            });
            if (it == visible.cend()) {
                // Not a local, so whatever holds the frame may have it as a field.
                const QString type = typeName(m_classes.value(location.classId).signature);
                evaluateThis([this, name, done, location, type](const Evaluated &self) {
                    if (self.error.isEmpty()) {
                        fieldOf(self, name, [this, name, done, location,
                                             type](const Evaluated &field) {
                            if (field.error.isEmpty()) {
                                done(field);
                                return;
                            }
                            staticFieldOf(location.classId, type, name, done);
                        });
                        return;
                    }
                    staticFieldOf(location.classId, type, name, done);
                });
                return;
            }
            const MethodInfo::Variable variable = *it;
            JdwpWriter request = writer();
            request.writeObjectId(thread).writeFrameId(frameId).writeInt(1)
                .writeInt(variable.slot)
                .writeByte(quint8(variable.signature.isEmpty()
                                      ? 'L' : variable.signature.at(0).toLatin1()));
            send(Jdwp::StackFrameSet, Jdwp::StackFrameGetValues, request.data(),
                 [this, done, variable](const JdwpReply &reply) {
                if (!reply.ok()) {
                    done({.error = JdwpClient::errorString(reply.errorCode)});
                    return;
                }
                JdwpReader r = reader(reply.data);
                if (r.readInt() < 1) {
                    done({.error = Tr::tr("The virtual machine returned no value.")});
                    return;
                }
                done({.value = r.readTaggedValue(), .type = typeName(variable.signature)});
            });
        });
    });
}

// Reads one static field of a class, which is where a name that is neither a
// local nor a field of "this" can still be.
void JdwpImpl::staticFieldOf(quint64 classId, const QString &className, const QString &name,
                             const Evaluation &done)
{
    withFields(classId, [this, classId, className, name, done] {
        const QList<FieldInfo> fields = m_classes.value(classId).staticFields;
        const auto it = std::find_if(fields.cbegin(), fields.cend(),
                                     [&name](const FieldInfo &field) {
            return field.name == name;
        });
        if (it == fields.cend()) {
            done({.error = Tr::tr("\"%1\" is not a variable here, and %2 has no field of that "
                                  "name.").arg(name, className)});
            return;
        }
        const FieldInfo field = *it;
        send(Jdwp::ReferenceTypeSet, Jdwp::RefTypeGetValues,
             writer().writeReferenceTypeId(classId).writeInt(1).writeFieldId(field.id).data(),
             [this, done, field](const JdwpReply &reply) {
            if (!reply.ok()) {
                done({.error = JdwpClient::errorString(reply.errorCode)});
                return;
            }
            JdwpReader r = reader(reply.data);
            if (r.readInt() < 1) {
                done({.error = Tr::tr("The virtual machine returned no value.")});
                return;
            }
            done({.value = r.readTaggedValue(), .type = typeName(field.signature)});
        });
    });
}

void JdwpImpl::fieldOf(const Evaluated &base, const QString &name, const Evaluation &done)
{
    if (!base.value.isObject()) {
        done({.error = Tr::tr("\"%1\" is not something with fields.")
                           .arg(primitiveText(base.value))});
        return;
    }
    if (base.value.isNull()) {
        done({.error = Tr::tr("The value is null.")});
        return;
    }
    const quint64 object = base.value.bits;
    if (base.value.tag == Jdwp::ArrayValueTag) {
        if (name != "length") {
            done({.error = Tr::tr("An array has no field \"%1\".").arg(name)});
            return;
        }
        send(Jdwp::ArrayReferenceSet, Jdwp::ArrayLength, writer().writeObjectId(object).data(),
             [this, done](const JdwpReply &reply) {
            if (!reply.ok()) {
                done({.error = JdwpClient::errorString(reply.errorCode)});
                return;
            }
            done({.value = {Jdwp::IntValueTag, quint64(reader(reply.data).readInt())},
                  .type = "int"});
        });
        return;
    }
    send(Jdwp::ObjectReferenceSet, Jdwp::ObjectReferenceType, writer().writeObjectId(object).data(),
         [this, object, name, done](const JdwpReply &reply) {
        if (!reply.ok()) {
            done({.error = JdwpClient::errorString(reply.errorCode)});
            return;
        }
        JdwpReader r = reader(reply.data);
        r.readByte();
        const quint64 typeId = r.readReferenceTypeId();
        withSignature(typeId, [this, object, name, done, typeId] {
            const QString className = typeName(m_classes.value(typeId).signature);
            withFields(typeId, [this, object, name, done, typeId, className] {
                // The superclasses come first, and a field hides theirs of its name.
                const QList<FieldInfo> fields = m_classes.value(typeId).fields;
                const auto it = std::find_if(fields.crbegin(), fields.crend(),
                                             [&name](const FieldInfo &field) {
                    return field.name == name;
                });
                if (it == fields.crend()) {
                    staticFieldOf(typeId, className, name, done);
                    return;
                }
                const FieldInfo field = *it;
                send(Jdwp::ObjectReferenceSet, Jdwp::ObjectGetValues,
                     writer().writeObjectId(object).writeInt(1).writeFieldId(field.id).data(),
                     [this, done, field](const JdwpReply &reply) {
                    if (!reply.ok()) {
                        done({.error = JdwpClient::errorString(reply.errorCode)});
                        return;
                    }
                    JdwpReader r = reader(reply.data);
                    if (r.readInt() < 1) {
                        done({.error = Tr::tr("The virtual machine returned no value.")});
                        return;
                    }
                    done({.value = r.readTaggedValue(), .type = typeName(field.signature)});
                });
            });
        });
    });
}

// The tags an array index can come in, which is the integral ones alone: a
// float indexes nothing in Java, and its bits are not a number here.
static bool isIntegralTag(quint8 tag)
{
    switch (tag) {
    case Jdwp::ByteValueTag:
    case Jdwp::CharValueTag:
    case Jdwp::ShortValueTag:
    case Jdwp::IntValueTag:
    case Jdwp::LongValueTag:
        return true;
    default:
        return false;
    }
}

void JdwpImpl::elementOf(const Evaluated &base, const Evaluated &index, const Evaluation &done)
{
    if (base.value.tag != Jdwp::ArrayValueTag) {
        done({.error = Tr::tr("The value is not an array.")});
        return;
    }
    if (base.value.isNull()) {
        done({.error = Tr::tr("The array is null.")});
        return;
    }
    if (!isIntegralTag(index.value.tag)) {
        done({.error = Tr::tr("The index is not a number.")});
        return;
    }
    const qint64 position = qint64(index.value.bits);
    const quint64 object = base.value.bits;
    send(Jdwp::ArrayReferenceSet, Jdwp::ArrayLength, writer().writeObjectId(object).data(),
         [this, object, position, done](const JdwpReply &reply) {
        if (!reply.ok()) {
            done({.error = JdwpClient::errorString(reply.errorCode)});
            return;
        }
        const qint32 length = reader(reply.data).readInt();
        if (position < 0 || position >= length) {
            done({.error = Tr::tr("The index %1 is outside the array, which holds %n item(s).",
                                  nullptr, length).arg(position)});
            return;
        }
        send(Jdwp::ArrayReferenceSet, Jdwp::ArrayGetValues,
             writer().writeObjectId(object).writeInt(qint32(position)).writeInt(1).data(),
             [this, done](const JdwpReply &reply) {
            if (!reply.ok()) {
                done({.error = JdwpClient::errorString(reply.errorCode)});
                return;
            }
            JdwpReader r = reader(reply.data);
            const quint8 elementTag = r.readByte();
            if (r.readInt() < 1) {
                done({.error = Tr::tr("The virtual machine returned no value.")});
                return;
            }
            const bool primitive = Jdwp::isPrimitiveTag(elementTag);
            const JdwpValue element = primitive ? r.readUntaggedValue(elementTag)
                                                : r.readTaggedValue();
            // Only a primitive tag names a type; the type of an object is its own,
            // and is read off the value.
            done({.value = element,
                  .type = primitive ? typeName(QChar::fromLatin1(char(elementTag)))
                                    : QString()});
        });
    });
}

void JdwpImpl::fetchLocals(const RefreshRequest &request)
{
    const int generation = ++m_localsGeneration;
    m_localsRequestId = request.requestId;
    m_expandedINames = request.expandedINames;
    m_expandedItems = request.expandedForDumpers();
    m_locals.clear();
    m_localRoots.clear();

    const auto join = std::make_shared<Join>([this, generation] {
        if (generation == m_localsGeneration)
            reportLocals();
    });
    join->add();
    if (m_running || m_currentThread == 0 || !m_client.isConnected()) {
        join->release();
        return;
    }
    addWatchers(join, request.watchers);

    const int frameIndex = m_currentFrame;
    const quint64 thread = m_currentThread;
    const int stopGeneration = m_stopGeneration;
    join->add();
    withFrames(frameIndex + 1, [this, join, generation, stopGeneration, frameIndex, thread] {
        if (generation != m_localsGeneration)
            return;
        if (stopGeneration != m_stopGeneration || frameIndex >= m_frames.size()) {
            join->release();
            return;
        }
        const Frame frame = m_frames.at(frameIndex);

        join->add();
        send(Jdwp::StackFrameSet, Jdwp::StackFrameThisObject,
             writer().writeObjectId(thread).writeFrameId(frame.id).data(),
             [this, join, generation](const JdwpReply &reply) {
            if (generation != m_localsGeneration)
                return;
            if (reply.ok()) {
                const JdwpValue self = reader(reply.data).readTaggedValue();
                if (self.isObject() && !self.isNull()) {
                    m_localRoots.prepend("local.this");
                    addValue(join, "local.this", "this", {}, self);
                }
            }
            join->release();
        });

        join->add();
        const JdwpLocation location = frame.location;
        withVariables(location.classId, location.methodId,
                      [this, join, generation, location, frame, thread] {
            if (generation != m_localsGeneration)
                return;
            const QList<MethodInfo::Variable> visible = visibleVariables(location);
            if (visible.isEmpty()) {
                join->release();
                return;
            }
            JdwpWriter request = writer();
            request.writeObjectId(thread).writeFrameId(frame.id).writeInt(qint32(visible.size()));
            for (const MethodInfo::Variable &variable : std::as_const(visible)) {
                request.writeInt(variable.slot)
                    .writeByte(quint8(variable.signature.isEmpty()
                                          ? 'L' : variable.signature.at(0).toLatin1()));
            }
            send(Jdwp::StackFrameSet, Jdwp::StackFrameGetValues, request.data(),
                 [this, join, generation, visible](const JdwpReply &reply) {
                if (generation != m_localsGeneration)
                    return;
                if (reply.ok()) {
                    JdwpReader r = reader(reply.data);
                    const qint32 count = r.readInt();
                    for (qint32 i = 0; i < count && i < visible.size() && r.ok(); ++i) {
                        const JdwpValue value = r.readTaggedValue();
                        const MethodInfo::Variable &variable = visible.at(i);
                        QString iname = "local." + variable.name;
                        if (m_locals.contains(iname))
                            iname += '#' + QString::number(variable.slot);
                        m_localRoots.append(iname);
                        addValue(join, iname, variable.name, typeName(variable.signature), value);
                    }
                }
                join->release();
            });
        });
        join->release();
    });
    join->release();
}

// The expressions the user typed into the Expressions view, each answered the
// way a local is, so that an object among them expands like one.
void JdwpImpl::addWatchers(const JoinPtr &join, const QJsonArray &watchers)
{
    const int generation = m_localsGeneration;
    for (const QJsonValue &watcherValue : watchers) {
        const QJsonObject watcher = watcherValue.toObject();
        const QString iname = watcher.value("iname").toString();
        const QString hexExpression = watcher.value("exp").toString();
        const QString expression
            = QString::fromUtf8(QByteArray::fromHex(hexExpression.toLatin1()));
        if (iname.isEmpty())
            continue;

        m_localRoots.append(iname);
        Local &local = m_locals[iname];
        local.name = expression;
        local.watcherName = hexExpression;

        const Result<JdwpExpression> parsed = parseJdwpExpression(expression);
        if (!parsed) {
            local.value = '<' + parsed.error() + '>';
            continue;
        }
        join->add();
        evaluate(*parsed, [this, join, generation, iname, expression](const Evaluated &result) {
            if (generation != m_localsGeneration)
                return;
            if (result.error.isEmpty())
                addValue(join, iname, expression, result.type, result.value);
            else
                m_locals[iname].value = '<' + result.error + '>';
            join->release();
        });
    }
}

void JdwpImpl::addValue(const JoinPtr &join, const QString &iname, const QString &name,
                        const QString &declaredType, const JdwpValue &value)
{
    Local &local = m_locals[iname];
    local.name = name;
    local.type = declaredType;
    if (!value.isObject()) {
        local.value = primitiveText(value);
        return;
    }
    if (value.isNull()) {
        local.value = "null";
        return;
    }
    if (value.tag == Jdwp::StringValueTag) {
        local.type = "java.lang.String";
        const int generation = m_localsGeneration;
        join->add();
        send(Jdwp::StringReferenceSet, Jdwp::StringValue, writer().writeObjectId(value.bits).data(),
             [this, join, generation, iname](const JdwpReply &reply) {
            if (generation != m_localsGeneration)
                return;
            m_locals[iname].value = reply.ok()
                                        ? quoted(reader(reply.data).readString())
                                        : '<' + JdwpClient::errorString(reply.errorCode) + '>';
            join->release();
        });
        return;
    }
    addObjectValue(join, iname, value);
}

void JdwpImpl::addObjectValue(const JoinPtr &join, const QString &iname, const JdwpValue &value)
{
    m_locals[iname].value = QString("@%1").arg(value.bits, 0, 16);
    const int generation = m_localsGeneration;
    const quint64 object = value.bits;
    join->add();
    send(Jdwp::ObjectReferenceSet, Jdwp::ObjectReferenceType, writer().writeObjectId(object).data(),
         [this, join, generation, iname, object](const JdwpReply &reply) {
        if (generation != m_localsGeneration)
            return;
        if (!reply.ok()) {
            join->release();
            return;
        }
        JdwpReader r = reader(reply.data);
        const quint8 tag = r.readByte();
        const quint64 typeId = r.readReferenceTypeId();
        withSignature(typeId, [this, join, generation, iname, object, tag, typeId] {
            if (generation != m_localsGeneration)
                return;
            const QString signature = m_classes.value(typeId).signature;
            m_locals[iname].type = typeName(signature);

            if (tag == Jdwp::ArrayTag) {
                send(Jdwp::ArrayReferenceSet, Jdwp::ArrayLength,
                     writer().writeObjectId(object).data(),
                     [this, join, generation, iname, object, signature](const JdwpReply &reply) {
                    if (generation != m_localsGeneration)
                        return;
                    const qint32 length = reply.ok() ? reader(reply.data).readInt() : 0;
                    Local &local = m_locals[iname];
                    local.value = Tr::tr("<%n items>", nullptr, length);
                    local.hasChildren = length > 0;
                    if (length == 0 || !m_expandedINames.contains(iname)) {
                        join->release();
                        return;
                    }
                    const qint32 count
                        = qMin(length, m_expandedItems.value(iname)
                                           .toInt(RefreshRequest::defaultArrayCount));
                    JdwpWriter request = writer();
                    request.writeObjectId(object).writeInt(0).writeInt(count);
                    send(Jdwp::ArrayReferenceSet, Jdwp::ArrayGetValues, request.data(),
                         [this, join, generation, iname, signature](const JdwpReply &reply) {
                        if (generation != m_localsGeneration)
                            return;
                        if (reply.ok()) {
                            const QString elementType = typeName(signature.mid(1));
                            JdwpReader r = reader(reply.data);
                            const quint8 elementTag = r.readByte();
                            const qint32 count = r.readInt();
                            for (qint32 i = 0; i < count && r.ok(); ++i) {
                                const JdwpValue element = Jdwp::isPrimitiveTag(elementTag)
                                                              ? r.readUntaggedValue(elementTag)
                                                              : r.readTaggedValue();
                                const QString child = iname + '.' + QString::number(i);
                                m_locals[iname].children.append(child);
                                addValue(join, child, QString("[%1]").arg(i), elementType, element);
                            }
                        }
                        join->release();
                    });
                });
                return;
            }

            m_locals[iname].hasChildren = true;
            if (!m_expandedINames.contains(iname)) {
                join->release();
                return;
            }
            withFields(typeId, [this, join, generation, iname, object, typeId] {
                if (generation != m_localsGeneration)
                    return;
                const QList<FieldInfo> fields = m_classes.value(typeId).fields;
                if (fields.isEmpty()) {
                    m_locals[iname].hasChildren = false;
                    join->release();
                    return;
                }
                JdwpWriter request = writer();
                request.writeObjectId(object).writeInt(qint32(fields.size()));
                for (const FieldInfo &field : fields)
                    request.writeFieldId(field.id);
                send(Jdwp::ObjectReferenceSet, Jdwp::ObjectGetValues, request.data(),
                     [this, join, generation, iname, fields](const JdwpReply &reply) {
                    if (generation != m_localsGeneration)
                        return;
                    if (reply.ok()) {
                        JdwpReader r = reader(reply.data);
                        const qint32 count = r.readInt();
                        for (qint32 i = 0; i < count && i < fields.size() && r.ok(); ++i) {
                            const JdwpValue fieldValue = r.readTaggedValue();
                            const FieldInfo &field = fields.at(i);
                            // A field a subclass hides is still there, and
                            // shown under a name of its own.
                            QString child = iname + '.' + field.name;
                            if (m_locals.contains(child))
                                child += '#' + QString::number(i);
                            m_locals[iname].children.append(child);
                            addValue(join, child, field.name, typeName(field.signature),
                                     fieldValue);
                        }
                    }
                    join->release();
                });
            });
        });
    });
}

GdbMi JdwpImpl::localsItem(const QString &iname) const
{
    const Local local = m_locals.value(iname);
    GdbMi item;
    item.m_type = GdbMi::Tuple;
    item.addChild(constMi("iname", iname));
    item.addChild(constMi("name", local.name));
    if (!local.watcherName.isEmpty())
        item.addChild(constMi("wname", local.watcherName));
    item.addChild(constMi("type", local.type));
    item.addChild(constMi("value", local.value));
    item.addChild(constMi("numchild", local.hasChildren ? "1" : "0"));
    if (!local.children.isEmpty()) {
        GdbMi children;
        children.m_type = GdbMi::List;
        children.m_name = "children";
        for (const QString &child : local.children)
            children.addChild(localsItem(child));
        item.addChild(children);
    }
    return item;
}

void JdwpImpl::reportLocals()
{
    GdbMi data;
    data.m_type = GdbMi::List;
    data.m_name = "data";
    for (const QString &iname : std::as_const(m_localRoots))
        data.addChild(localsItem(iname));
    GdbMi all;
    all.m_type = GdbMi::Tuple;
    all.addChild(data);
    emit refreshDataReceived(m_localsRequestId, RefreshKind::Locals, all);
}

void JdwpImpl::selectThread(const QString &threadId)
{
    const quint64 thread = threadId.toULongLong();
    if (thread == m_currentThread)
        return;
    // Frames of the thread left behind can still be on their way.
    ++m_stopGeneration;
    m_currentThread = thread;
    m_currentFrame = 0;
    m_frames.clear();
    m_framesComplete = false;
}

void JdwpImpl::activateFrame(int index)
{
    m_currentFrame = qMax(0, index);
}

void JdwpImpl::setRegisterValue(const QString &, const QString &)
{
    reportUnsupported(Tr::tr("registers"));
}

void JdwpImpl::accessMemory(MemoryOp, quint64, quint64, quint64, const QByteArray &)
{
    reportUnsupported(Tr::tr("memory access"));
}

void JdwpImpl::fetchDisassembly(quint64 requestId, quint64, const QString &)
{
    reportUnsupported(Tr::tr("disassembly"));
    emit disassemblyReceived(requestId, {});
}

void JdwpImpl::setPeripheralRegisterValue(quint64, quint64)
{
    reportUnsupported(Tr::tr("peripheral registers"));
}

void JdwpImpl::watchPoint(quint64 requestId, const QPoint &)
{
    reportUnsupported(Tr::tr("watch points"));
    emit watchPointResolved(requestId, 0, {});
}

void JdwpImpl::createSnapshot(quint64 requestId)
{
    reportUnsupported(Tr::tr("snapshots"));
    emit snapshotCreated(requestId, false, {});
}

void JdwpImpl::assignValueInDebugger(const WatchItemData &, const QString &, const QString &)
{
    reportUnsupported(Tr::tr("changing values"));
}

void JdwpImpl::executeDebuggerCommand(const QString &, const WatchItemData &)
{
    reportUnsupported(Tr::tr("debugger commands"));
}

} // namespace Debugger::Internal
