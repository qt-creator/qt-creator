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
#include <QTimer>

#include <chrono>
#include <cmath>
#include <cstring>
#include <limits>

using namespace Utils;

namespace Debugger::Internal {

// Where a step does not stop, as jdb has it: the runtime's own code. An
// exception breakpoint does not exclude these by where the throw happens. What
// a user wants to be stopped by is mostly thrown from inside the runtime - an
// index outside a list, a number that will not parse - and such a filter would
// drop exactly those. Only a throw the runtime also catches itself is left out.
// A device runs a runtime of its own, and its framework throws and catches
// within itself as much as a desktop one does; the Kotlin library stands for
// the same part of a Kotlin program that java.lang does of a Java one.
static const QStringList s_stepExcludes = {
    "java.*", "javax.*", "sun.*", "jdk.*", "com.sun.*",
    "android.*", "com.android.internal.*", "dalvik.*", "libcore.*",
    "org.apache.harmony.*", "kotlin.*"};

// Whether a class name is matched by one of those patterns, which are the ones
// the protocol takes: a name, or one with a "*" at either end of it.
static bool isRuntimeClass(const QString &name)
{
    return Utils::anyOf(s_stepExcludes, [&name](const QString &pattern) {
        if (pattern.startsWith('*'))
            return name.endsWith(QStringView(pattern).mid(1));
        if (pattern.endsWith('*'))
            return name.startsWith(QStringView(pattern).chopped(1));
        return name == pattern;
    });
}

static constexpr qint32 s_classPreparedStatus = 2;

// The row the statics of the class the frame stands in are shown under.
// "static" is a keyword, so no local of that frame can be standing there.
static const char s_staticIName[] = "local.@static";

// How long a toString() is given before the views go on without it. A call
// that does not return holds a thread the virtual machine will not suspend
// again, which is bad; waiting for it for the rest of the session is worse.
static constexpr std::chrono::seconds s_invokeTimeout{5};

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

QString packageDirectoryOf(const QString &source)
{
    // Kotlin ends the statement with the line rather than with a semicolon.
    static const QRegularExpression packageStatement(R"(^\s*package\s+([\w.]+)\s*;?)",
                                                     QRegularExpression::MultilineOption);
    const QRegularExpressionMatch match = packageStatement.match(source);
    return match.hasMatch() ? match.captured(1).replace('.', '/') : QString();
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

// A name without package matches in any package. A nested class can be
// written Outer.Inner or Outer$Inner, the signature tells which dot is which.
static bool namesClass(const QString &className, const QString &signature)
{
    const QString full = typeName(signature);
    const QString wanted = QString(className).replace('$', '.');
    if (QString(full).replace('$', '.') == wanted)
        return true;
    const QString nested = full.section('.', -1).replace('$', '.');
    return nested == wanted || nested.endsWith('.' + wanted);
}

// What a value reads as in a view that gives it one line and one cell, which
// is no place for the characters that would break out of either.
static QString escaped(const QString &text)
{
    QString result = text;
    result.replace('\\', "\\\\").replace('"', "\\\"").replace('\n', "\\n")
        .replace('\r', "\\r").replace('\t', "\\t");
    return result;
}

static QString quoted(const QString &text)
{
    return '"' + escaped(text) + '"';
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

// The kind of event a breakpoint of this type is registered under, which is
// what it takes to clear the request again and to tell a hit of one kind from
// a request of another that carries the same number.
static quint8 eventKindOf(BreakpointType type)
{
    switch (type) {
    case BreakpointAtThrow:
        return Jdwp::ExceptionEvent;
    // Writes only, as gdb's plain watchpoint.
    case WatchpointAtExpression:
        return Jdwp::FieldModificationEvent;
    default:
        return Jdwp::BreakpointEvent;
    }
}

static bool namesAClass(BreakpointType type)
{
    return type == BreakpointByFunction || type == WatchpointAtExpression;
}

static QString classOfMember(const QString &spec)
{
    return jvmMemberSpec(spec).section('.', 0, -2);
}

static QString memberOfMember(const QString &spec)
{
    return jvmMemberSpec(spec).section('.', -1);
}

static DebuggerEngineSetupData jdwpImplSetupData()
{
    DebuggerEngineSetupData data;
    // No WatchpointByExpressionCapability: the Locals view would hand over a
    // plain field name or a local, which the machine cannot watch.
    data.capabilities = RunToLineCapability | AddWatcherCapability
                      | BreakConditionCapability | BreakOnThrowAndCatchCapability
                      | BreakIndividualLocationsCapability | TracePointCapability;
    data.extraCapabilities = DebuggerExtraCapability::Threads
                           | DebuggerExtraCapability::ThreadEvent;
    data.startModes = DebuggerStartModeFlag::Launch | DebuggerStartModeFlag::AttachToRemoteServer;
    data.toolTipHandling = ToolTipHandling::IfStoppedInferior;
    data.acceptsBreakpoint = [](const AcceptsBreakpointQuery &query) {
        switch (query.type) {
        case BreakpointAtThrow:
            return true;
        case BreakpointByFileAndLine:
            return isJvmSource(query.fileName);
        case BreakpointByFunction:
            return isJvmMember(query.functionName);
        case WatchpointAtExpression:
            return isJvmMember(query.expression);
        default:
            return false;
        }
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
    m_since.start();
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
            // A hit whose condition is still being worked out holds the
            // machine already, and that stop is what answers the interrupt.
            if (m_decidingOnHit)
                m_interruptRequested = true;
            else
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

void JdwpImpl::pin(const JdwpValue &value, const Stop &stop)
{
    // The stop it was made for is over, and with it the pins.
    if (stop.runGeneration != m_runGeneration || !value.isObject() || value.isNull()
        || m_pinned.contains(value.bits)) {
        return;
    }
    m_pinned.append(value.bits);
    send(Jdwp::ObjectReferenceSet, Jdwp::ObjectDisableCollection,
         writer().writeObjectId(value.bits).data());
}

QString JdwpImpl::whyNoCallAt(const Stop &stop) const
{
    if (m_running || stop.thread == 0)
        return Tr::tr("A method can only be called while the program is stopped.");
    if (stop.runGeneration != m_runGeneration)
        return Tr::tr("The program went on before the call could be made.");
    if (stop.thread != m_currentThread)
        return Tr::tr("Another thread was selected before the call could be made.");
    // The machine refuses a call on a thread that is still off running one
    // that was given up on.
    if (m_callsOut.value(stop.thread) > 0)
        return Tr::tr("The thread is still running an earlier call.");
    return {};
}

void JdwpImpl::resumeFromStop()
{
    for (const quint64 object : std::exchange(m_pinned, {})) {
        send(Jdwp::ObjectReferenceSet, Jdwp::ObjectEnableCollection,
             writer().writeObjectId(object).data());
    }
    for (const QString &command : std::exchange(m_pendingCommands, {})) {
        emit message(Tr::tr("The program went on before \"%1\" was worked out.").arg(command),
                     ConsoleOutput);
    }
    ++m_stopGeneration;
    ++m_runGeneration;
    m_frames.clear();
    m_framesComplete = false;
    const int suspensions = 1 + std::exchange(m_extraSuspends, 0);

    if (!m_pendingStops.isEmpty()) {
        // The stop that is next holds a suspension of its own, so the virtual
        // machine stays where it is, and that stop is reported right away.
        for (int i = 0; i < suspensions; ++i)
            send(Jdwp::VirtualMachineSet, Jdwp::VmResume);
        const QList<JdwpEvent> next = m_pendingStops.takeFirst();
        QMetaObject::invokeMethod(this, [this, next] { reportStopAt(next); },
                                  Qt::QueuedConnection);
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
    m_givenUpRequests.insert({eventKind, requestId});
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
    // Everything that stops at one place comes in one set, and it takes only
    // one of them to mean it.
    QList<JdwpEvent> stops;

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
        case Jdwp::ExceptionEvent:
        case Jdwp::FieldModificationEvent:
            stops.append(event);
            break;
        default:
            break;
        }
    }

    if (!stops.isEmpty()) {
        // The stop holds the suspension the set came with, and gives it back
        // when it is over.
        handleClassPrepares(classPrepares, [this, stops] { reportStop(stops); });
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

void JdwpImpl::reportStop(const QList<JdwpEvent> &hits)
{
    if (!m_running) {
        // A call the views made is out on the thread that ran into this.
        // Reporting it would leave the program standing in a call nobody
        // asked for, and the call could not come back, since the thread it
        // needs is the one this suspends. The suspension it came with is
        // given back instead, which lets the call run on. Any other thread
        // ran into something of the user's, and waits its turn below.
        if (m_callsOut.value(hits.first().thread) > 0) {
            if (!std::exchange(m_callStopMentioned, true)) {
                emit message(Tr::tr("The program stopped inside a call made to show a value, "
                                    "and is run past it: the call has to come back before "
                                    "the program can be left standing anywhere."), LogWarning);
            }
            send(Jdwp::VirtualMachineSet, Jdwp::VmResume);
            return;
        }
        m_pendingStops.append(hits);
        return;
    }
    reportStopAt(hits);
}

// What was thrown, and whether anything is going to catch it. The message the
// exception carries is not read: that would mean calling getMessage() in the
// virtual machine, and a stop is reported before anything is allowed to run.
void JdwpImpl::reportException(const JdwpEvent &event)
{
    if (!event.exception.isObject() || event.exception.isNull())
        return;
    const bool caught = event.catchLocation.isValid();
    // Let go of again before the name came back, which would put the line
    // among what a later stop, or the running program, is saying.
    const int generation = m_runGeneration;
    send(Jdwp::ObjectReferenceSet, Jdwp::ObjectReferenceType,
         writer().writeObjectId(event.exception.bits).data(),
         [this, caught, generation](const JdwpReply &reply) {
        if (!reply.ok() || generation != m_runGeneration)
            return;
        JdwpReader r = reader(reply.data);
        r.readByte();
        const quint64 typeId = r.readReferenceTypeId();
        if (!r.ok())
            return;
        withSignature(typeId, [this, typeId, caught, generation] {
            if (generation != m_runGeneration)
                return;
            const QString name = typeName(m_classes.value(typeId).signature);
            emit message(caught
                             ? Tr::tr("%1 was thrown, and is caught further up.").arg(name)
                             : Tr::tr("%1 was thrown, and nothing catches it.").arg(name),
                         LogMisc);
        });
    });
}

void JdwpImpl::valueText(const JdwpValue &value, const std::function<void(const QString &)> &done)
{
    if (!value.isObject()) {
        done(primitiveText(value));
        return;
    }
    if (value.isNull()) {
        done("null");
        return;
    }
    const quint64 object = value.bits;
    if (value.tag != Jdwp::StringValueTag) {
        done(QString("@%1").arg(object, 0, 16));
        return;
    }
    send(Jdwp::StringReferenceSet, Jdwp::StringValue, writer().writeObjectId(object).data(),
         [this, object, done](const JdwpReply &reply) {
        done(reply.ok() ? quoted(reader(reply.data).readString())
                        : QString("@%1").arg(object, 0, 16));
    });
}

// Reports the value about to be written.
void JdwpImpl::reportFieldWrite(const Hit &hit)
{
    const Breakpoint *bp = breakpoint(hit.number);
    if (!bp)
        return;
    const QString name = bp->className + '.' + bp->memberName;
    // Dropped if the program ran on in the meantime.
    const int generation = m_runGeneration;
    valueText(hit.event.valueToBe, [this, name, generation](const QString &text) {
        if (generation == m_runGeneration)
            emit message(Tr::tr("%1 is about to be set to %2.").arg(name, text), LogMisc);
    });
}

void JdwpImpl::reportTracepoint(const QString &number, const JdwpEvent &event, const Done &done)
{
    const Breakpoint *bp = breakpoint(number);
    if (!bp) {
        done();
        return;
    }
    const QString text = bp->message;
    const QList<TracepointCapture> captures = parseTracepointCaptures(text);
    const auto filled = std::make_shared<QStringList>(captures.size(), QString());
    const int generation = m_runGeneration;
    const auto calling = std::make_shared<QList<CallingCapture>>();
    const auto say = [this, text, captures, filled, generation, done] {
        if (generation != m_runGeneration) {
            done();
            return;
        }
        QString formatted = text;
        // Back to front: a replacement moves everything behind it.
        for (qsizetype i = captures.size() - 1; i >= 0; --i) {
            const TracepointCapture &capture = captures.at(i);
            formatted.replace(capture.start, capture.end - capture.start, filled->at(i));
        }
        emit message(formatted, LogMisc);
        done();
    };
    const auto join = std::make_shared<Join>([this, filled, calling, generation, say] {
        answerTracepointCalls(filled, calling, generation, say);
    });
    join->add();

    const auto withResolvedFrame = [this, join, filled](qsizetype index, int frame,
                                                        const std::function<QString(
                                                            const ResolvedLocation &)> &text) {
        join->add();
        withFrames(frame + 1, [this, join, filled, index, frame, text] {
            if (frame >= m_frames.size()) {
                join->release();
                return;
            }
            resolveLocation(m_frames.at(frame).location,
                            [join, filled, index, text](const ResolvedLocation &resolved) {
                (*filled)[index] = text(resolved);
                join->release();
            });
        });
    };

    for (qsizetype i = 0; i < captures.size(); ++i) {
        const TracepointCapture &capture = captures.at(i);
        switch (capture.type) {
        case TracepointCaptureType::Expression: {
            const Result<JdwpExpression> parsed = parseJdwpExpression(capture.expression);
            if (!parsed) {
                (*filled)[i] = parsed.error();
                break;
            }
            // One that calls lets the thread run, and the frames the captures
            // beside it are read off go with it, so it waits until they are in.
            if (parsed->makesTheProgramRun()) {
                calling->append({i, *parsed});
                break;
            }
            join->add();
            evaluate(*parsed, [this, filled, i, join](const Evaluated &result) {
                if (!result.error.isEmpty()) {
                    (*filled)[i] = result.error;
                    join->release();
                    return;
                }
                valueText(result.value, [filled, i, join](const QString &text) {
                    (*filled)[i] = text;
                    join->release();
                });
            });
            break;
        }
        case TracepointCaptureType::Function:
            withResolvedFrame(i, 0, [](const ResolvedLocation &r) { return r.function; });
            break;
        case TracepointCaptureType::FilePos:
            withResolvedFrame(i, 0, [](const ResolvedLocation &r) {
                return QString("%1:%2").arg(r.file.fileName()).arg(r.line);
            });
            break;
        case TracepointCaptureType::Caller:
            withResolvedFrame(i, 1, [](const ResolvedLocation &r) { return r.function; });
            break;
        case TracepointCaptureType::Callstack: {
            join->add();
            withFrames(0, [this, join, filled, i] {
                const auto names = std::make_shared<QStringList>(m_frames.size(),
                                                                 QString());
                const auto stackJoin = std::make_shared<Join>([join, filled, i, names] {
                    (*filled)[i] = names->join(" <- ");
                    join->release();
                });
                stackJoin->add();
                for (qsizetype frame = 0; frame < m_frames.size(); ++frame) {
                    stackJoin->add();
                    resolveLocation(m_frames.at(frame).location,
                                    [stackJoin, names, frame](const ResolvedLocation &resolved) {
                        (*names)[frame] = resolved.function;
                        stackJoin->release();
                    });
                }
                stackJoin->release();
            });
            break;
        }
        // The code index stands in for an address.
        case TracepointCaptureType::Address:
            (*filled)[i] = QString("0x%1").arg(event.location.index, 0, 16);
            break;
        case TracepointCaptureType::Tid:
            (*filled)[i] = QString::number(event.thread);
            break;
        case TracepointCaptureType::ThreadName:
            join->add();
            send(Jdwp::ThreadReferenceSet, Jdwp::ThreadName,
                 writer().writeObjectId(event.thread).data(),
                 [this, filled, i, join](const JdwpReply &reply) {
                if (reply.ok())
                    (*filled)[i] = reader(reply.data).readString();
                join->release();
            });
            break;
        case TracepointCaptureType::Tick:
            (*filled)[i] = QString::number(m_since.elapsed());
            break;
        // Unknown when attached.
        case TracepointCaptureType::Pid:
            (*filled)[i] = QString::number(m_process ? m_process->processId() : 0);
            break;
        case TracepointCaptureType::ProcessName:
            if (m_process)
                (*filled)[i] = m_process->commandLine().executable().fileName();
            break;
        }
    }
    join->release();
}

void JdwpImpl::reportTracepoints(QList<Hit> hits, const Done &done)
{
    if (hits.isEmpty()) {
        done();
        return;
    }
    const Hit hit = hits.takeFirst();
    reportTracepoint(hit.number, hit.event, [this, hits, done] { reportTracepoints(hits, done); });
}

// One at a time, since the machine takes no second call on a thread that is
// off running the first, and after everything else in the message: what the
// frames say is gone as soon as the first of these runs.
void JdwpImpl::answerTracepointCalls(const std::shared_ptr<QStringList> &filled,
                                     const std::shared_ptr<QList<CallingCapture>> &calling,
                                     int generation, const Done &done)
{
    if (calling->isEmpty() || generation != m_runGeneration) {
        done();
        return;
    }
    const CallingCapture capture = calling->takeFirst();
    const auto next = [this, filled, calling, generation, done] {
        answerTracepointCalls(filled, calling, generation, done);
    };
    evaluate(capture.parsed, [this, filled, capture, next](const Evaluated &result) {
        if (!result.error.isEmpty()) {
            (*filled)[capture.index] = result.error;
            next();
            return;
        }
        valueText(result.value, [filled, capture, next](const QString &text) {
            (*filled)[capture.index] = text;
            next();
        });
    });
}

// The protocol numbers a request within its kind, so the request a step was
// registered under can carry the same number as a breakpoint's.
// For a hit that came before the answer naming its request: matched by its
// location, or for an exception breakpoint, by being the one being armed.
JdwpImpl::Breakpoint *JdwpImpl::breakpointBeingArmedFor(const JdwpEvent &event)
{
    for (Breakpoint &bp : m_breakpoints) {
        if (!bp.enabled || eventKindOf(bp.type) != event.kind)
            continue;
        if (bp.type == BreakpointAtThrow) {
            if (bp.requests.isEmpty())
                return &bp;
            continue;
        }
        const bool stands = Utils::anyOf(bp.locations, [&event](const Breakpoint::Location &l) {
            if (l.request != 0 || !l.enabled)
                return false;
            if (event.kind == Jdwp::FieldModificationEvent)
                return l.fieldClass == event.typeId && l.fieldId == event.fieldId;
            return l.where == event.location;
        });
        if (stands)
            return &bp;
    }
    return nullptr;
}

JdwpImpl::Breakpoint *JdwpImpl::breakpointOfHit(const JdwpEvent &event)
{
    if (Breakpoint *bp = breakpointOfRequest(event.kind, event.requestId))
        return bp;
    if (m_givenUpRequests.contains({event.kind, event.requestId}))
        return nullptr;
    return breakpointBeingArmedFor(event);
}

JdwpImpl::Breakpoint *JdwpImpl::breakpointOfRequest(quint8 eventKind, qint32 requestId)
{
    if (requestId == 0)
        return nullptr;
    for (Breakpoint &bp : m_breakpoints) {
        if (eventKindOf(bp.type) != eventKind)
            continue;
        if (bp.requests.contains(requestId))
            return &bp;
        const bool atOne = Utils::anyOf(bp.locations, [requestId](const Breakpoint::Location &l) {
            return l.request == requestId;
        });
        if (atOne)
            return &bp;
    }
    return nullptr;
}

// An exception the runtime throws and catches within itself is its own
// business: loading a class, looking up a character set or reaching for
// something by reflection throw them by the dozen, and none of them is what a
// breakpoint at a throw is set for. One that is caught outside the runtime, or
// caught nowhere at all, is the program's, wherever it was thrown.
void JdwpImpl::isRuntimesOwn(const JdwpEvent &event, const std::function<void(bool)> &done)
{
    if (!event.catchLocation.isValid()) {
        done(false);
        return;
    }
    const quint64 thrownIn = event.location.classId;
    const quint64 caughtIn = event.catchLocation.classId;
    withSignature(thrownIn, [this, thrownIn, caughtIn, done] {
        if (!isRuntimeClass(typeName(m_classes.value(thrownIn).signature))) {
            done(false);
            return;
        }
        withSignature(caughtIn, [this, caughtIn, done] {
            done(isRuntimeClass(typeName(m_classes.value(caughtIn).signature)));
        });
    });
}

// A condition the virtual machine cannot be asked about is worked out here,
// with the thread suspended where it stopped, and the stop given up if it
// does not hold. A condition that cannot be read stops, since going on past
// it silently would be worse.
void JdwpImpl::decideOnHit(const JdwpEvent &event, const std::function<void(bool)> &done)
{
    Breakpoint *bp = breakpointOfHit(event);
    if (!bp) {
        // A hit of a request given back was already on its way and is
        // nobody's. A request still being made is live.
        done(!m_givenUpRequests.contains({event.kind, event.requestId}));
        return;
    }
    const QString number = bp->number;
    const QString condition = bp->condition;
    const bool atThrow = bp->type == BreakpointAtThrow;
    const auto counted = [this, number] {
        Breakpoint *bp = breakpoint(number);
        if (!bp)
            return true;
        ++bp->hits;
        if (!bp->internal) {
            GdbMi data;
            data.m_type = GdbMi::List;
            data.addChild(breakpointData(*bp));
            emit breakpointModified(data);
        }
        return bp->hits - bp->ignoredSince > bp->ignoreCount;
    };

    // What the runtime throws and catches itself never reaches the condition
    // or the count either: it is not a hit of this breakpoint at all.
    const auto decide = [this, number, condition, counted, done] {
        if (condition.isEmpty()) {
            done(counted());
            return;
        }
        const Result<JdwpExpression> parsed = parseJdwpExpression(condition);
        if (!parsed) {
            emit message(Tr::tr("The condition of breakpoint %1 cannot be read: %2")
                             .arg(number, parsed.error()), LogWarning);
            done(true);
            return;
        }
        // The decision is made while the program stands at the hit, and a
        // call would run it on from there at every hit.
        if (parsed->makesTheProgramRun()) {
            emit message(Tr::tr("The condition of breakpoint %1 calls a method, which would "
                                "make the program run while it is being decided.").arg(number),
                         LogWarning);
            done(true);
            return;
        }
        evaluate(*parsed, [this, number, counted, done](const Evaluated &result) {
            if (!result.error.isEmpty()) {
                emit message(Tr::tr("The condition of breakpoint %1 could not be worked out: %2")
                                 .arg(number, result.error), LogWarning);
                done(true);
                return;
            }
            if (result.value.tag != Jdwp::BooleanValueTag) {
                emit message(Tr::tr("The condition of breakpoint %1 is not a boolean.").arg(number),
                             LogWarning);
                done(true);
                return;
            }
            done(result.value.bits != 0 && counted());
        });
    };

    if (!atThrow) {
        decide();
        return;
    }
    isRuntimesOwn(event, [decide, done](bool own) {
        if (own)
            done(false);
        else
            decide();
    });
}

// Every breakpoint hit counts its hit, even where another one already stops.
void JdwpImpl::decideOnHits(QList<JdwpEvent> hits, QList<JdwpEvent> taken,
                            const std::function<void(const QList<JdwpEvent> &)> &done)
{
    if (hits.isEmpty()) {
        done(taken);
        return;
    }
    const JdwpEvent hit = hits.takeFirst();
    decideOnHit(hit, [this, hit, hits, taken, done](bool yes) {
        QList<JdwpEvent> all = taken;
        if (yes)
            all.append(hit);
        decideOnHits(hits, all, done);
    });
}

// The first of the hits says where the stop is. None at all is a stop that
// no request asked for, which is one to stop at.
void JdwpImpl::reportStopAt(const QList<JdwpEvent> &hits)
{
    m_running = false;
    ++m_stopGeneration;
    const JdwpEvent where = hits.value(0);
    m_currentThread = where.thread;
    m_currentFrame = 0;
    m_frames.clear();
    m_framesComplete = false;

    // Only a resume gives up on this stop. A thread selected in the meantime
    // moves on the other generation, and the suspension this holds would be
    // lost for good if that ended the stop here.
    const int generation = m_runGeneration;
    m_decidingOnHit = true;
    decideOnHits(hits, {}, [this, hits, generation](const QList<JdwpEvent> &accepted) {
        if (generation != m_runGeneration) {
            m_decidingOnHit = false;
            return;
        }
        // An interrupt is answered by wherever the machine is, so a hit that
        // is given up on is still where it stops, and the flag is spent here
        // either way: a later stop of its own is not what was asked for.
        const bool requested = std::exchange(m_interruptRequested, false);
        const bool stop = hits.isEmpty() || !accepted.isEmpty();
        if (!stop && !requested) {
            m_decidingOnHit = false;
            resumeFromStop();
            return;
        }
        QList<Hit> taken;
        for (const JdwpEvent &hit : accepted) {
            const Breakpoint *bp = breakpointOfHit(hit);
            taken.append({bp ? bp->number : QString(), hit});
        }
        QList<Hit> tracepoints;
        for (const Hit &hit : std::as_const(taken)) {
            Breakpoint *bp = breakpoint(hit.number);
            if (!bp)
                continue;
            if (bp->tracepoint)
                tracepoints.append(hit);
            // Disabled rather than removed, so that it can be re-enabled.
            if (bp->oneShot && bp->enabled) {
                const QString number = bp->number;
                bp->enabled = false;
                clearRequestsOf(number, [this, number] {
                    if (const Breakpoint *bp = breakpoint(number))
                        emit breakpointModified(breakpointReport(*bp));
                });
            }
        }
        // Tracepoints let the program go on, unless a finished step, an
        // interrupt or another hit wants it stopped. A step still on its way
        // is carried on by that.
        if (!tracepoints.isEmpty()) {
            const bool goesOn = tracepoints.size() == taken.size() && !requested;
            reportTracepoints(tracepoints, [this, hits, taken, generation, goesOn, requested] {
                m_decidingOnHit = false;
                if (generation != m_runGeneration)
                    return;
                const bool late = std::exchange(m_interruptRequested, false);
                if (goesOn && !late) {
                    resumeFromStop();
                    return;
                }
                reportRunOk();
                clearTransientRequests();
                reportStopReached(hits, taken, requested || late, generation);
            });
            return;
        }
        m_decidingOnHit = false;
        // A step can end before the answer to its resume has been read.
        reportRunOk();
        clearTransientRequests();
        reportStopReached(hits, taken, requested, generation);
    });
}

void JdwpImpl::reportStopReached(const QList<JdwpEvent> &hits, const QList<Hit> &taken,
                                 bool requested, int generation)
{
    for (const Hit &hit : taken) {
        if (hit.event.kind == Jdwp::ExceptionEvent)
            reportException(hit.event);
        else if (hit.event.kind == Jdwp::FieldModificationEvent)
            reportFieldWrite(hit);
    }

    const int stopGeneration = m_stopGeneration;
    resolveLocation(hits.value(0).location, [this, requested, generation,
                                             stopGeneration](const ResolvedLocation &resolved) {
        if (generation != m_runGeneration)
            return;
        if (stopGeneration == m_stopGeneration && resolved.line > 0 && resolved.file.exists())
            emit locationChanged(resolved.file, resolved.line);
        emit inferiorEvent(requested ? InferiorEvent::StopOk
                                     : InferiorEvent::SpontaneousStop);
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
                reportStopAt({});
                return;
            }
            JdwpWriter request = writer();
            request.writeObjectId(thread).writeInt(0).writeInt(1);
            send(Jdwp::ThreadReferenceSet, Jdwp::ThreadFrames, request.data(),
                 [this, thread](const JdwpReply &reply) {
                JdwpEvent where;
                where.thread = thread;
                if (reply.ok()) {
                    JdwpReader r = reader(reply.data);
                    if (r.readInt() > 0) {
                        r.readFrameId();
                        where.location = r.readLocation();
                    }
                }
                reportStopAt({where});
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
                    method.genericSignature = r.readString();
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
                    if (!reply.ok()) {
                        info.variablesAbsent = reply.errorCode == Jdwp::AbsentInformationError;
                        break;
                    }
                    JdwpReader r = reader(reply.data);
                    r.readInt(); // argument count
                    const qint32 count = r.readInt();
                    for (qint32 i = 0; i < count && r.ok(); ++i) {
                        MethodInfo::Variable variable;
                        variable.codeIndex = quint64(r.readLong());
                        variable.name = r.readString();
                        variable.signature = r.readString();
                        variable.genericSignature = r.readString();
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
                    field.declaringClass = classId;
                    field.name = r.readString();
                    field.signature = r.readString();
                    field.genericSignature = r.readString();
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

// Which toString() an object of this class answers with, looked for in the
// class itself and then up its superclasses. The one java.lang.Object declares
// is not taken: what it returns is the type and the identity, and those are
// shown without making the program run at all.
void JdwpImpl::withToString(quint64 classId, const Done &done)
{
    fetchOnce(QString("tostring:%1").arg(classId),
              [this, classId] {
        const auto it = m_classes.constFind(classId);
        return it != m_classes.cend() && it->toStringKnown;
    }, [this, classId](const Done &finish) {
        withSignature(classId, [this, classId, finish] {
            if (m_classes.value(classId).signature == "Ljava/lang/Object;") {
                m_classes[classId].toStringKnown = true;
                finish();
                return;
            }
            withMethods(classId, [this, classId, finish] {
                const QList<MethodInfo> methods = m_classes.value(classId).methods;
                const auto it = std::find_if(methods.cbegin(), methods.cend(),
                                             [](const MethodInfo &method) {
                    return method.name == "toString" && method.signature == "()Ljava/lang/String;"
                           && !(method.modBits & (Jdwp::StaticModifierBit
                                                  | Jdwp::AbstractModifierBit));
                });
                if (it != methods.cend()) {
                    ClassInfo &info = m_classes[classId];
                    info.toStringKnown = true;
                    info.toStringClass = classId;
                    info.toStringMethod = it->id;
                    finish();
                    return;
                }
                // An interface has no superclass, and the question is refused.
                send(Jdwp::ClassTypeSet, Jdwp::ClassTypeSuperclass,
                     writer().writeReferenceTypeId(classId).data(),
                     [this, classId, finish](const JdwpReply &reply) {
                    const quint64 superclass = reply.ok()
                                                   ? reader(reply.data).readReferenceTypeId() : 0;
                    if (superclass == 0) {
                        m_classes[classId].toStringKnown = true;
                        finish();
                        return;
                    }
                    withToString(superclass, [this, classId, superclass, finish] {
                        const ClassInfo above = m_classes.value(superclass);
                        ClassInfo &info = m_classes[classId];
                        info.toStringKnown = true;
                        info.toStringClass = above.toStringClass;
                        info.toStringMethod = above.toStringMethod;
                        finish();
                    });
                });
            });
        });
    }, done);
}

// Runs the method and hands over what it returned, or nothing where it threw,
// returned something other than a string, or did not come back in time. The
// call cannot be taken back once it is out: a thread that does not return is
// one the virtual machine keeps running, so the views are let go of it rather
// than waiting for the rest of the session.
void JdwpImpl::invokeToString(const Stop &stop, quint64 object, quint64 declaringClass,
                              quint64 methodId, const Called &done)
{
    invoke(stop, object, declaringClass, methodId, Dispatch::OnObject, {},
           [this, done](const Invoked &result) {
        if (result.gaveUp) {
            done({}, true);
            return;
        }
        // Anything but a string it answered leaves the object shown by its
        // identity, which is what it had before the call.
        if (!result.error.isEmpty() || !result.thrown.isNull()
            || result.value.tag != Jdwp::StringValueTag || result.value.isNull()) {
            done({}, false);
            return;
        }
        send(Jdwp::StringReferenceSet, Jdwp::StringValue,
             writer().writeObjectId(result.value.bits).data(),
             [this, done](const JdwpReply &reply) {
            done(reply.ok() ? std::optional<QString>(escaped(reader(reply.data).readString()))
                            : std::optional<QString>(), false);
        });
    });
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

JdwpImpl::Breakpoint::Location *JdwpImpl::breakpointLocation(const QString &number)
{
    Breakpoint *bp = breakpoint(number.section('.', 0, 0));
    if (!bp)
        return nullptr;
    for (Breakpoint::Location &location : bp->locations) {
        if (location.number == number)
            return &location;
    }
    return nullptr;
}

GdbMi JdwpImpl::locationData(const Breakpoint &bp, const Breakpoint::Location &location) const
{
    GdbMi item;
    item.m_type = GdbMi::Tuple;
    item.addChild(constMi("number", location.number));
    item.addChild(constMi("func", location.function));
    const Utils::FilePath file = location.file.isEmpty() ? bp.file : location.file;
    if (!file.isEmpty() && location.line > 0) {
        item.addChild(constMi("file", file.path()));
        item.addChild(constMi("fullname", file.path()));
        item.addChild(constMi("line", QString::number(location.line)));
    }
    item.addChild(constMi("enabled", location.enabled ? "y" : "n"));
    return item;
}

GdbMi JdwpImpl::breakpointData(const Breakpoint &bp) const
{
    GdbMi bkpt;
    bkpt.m_type = GdbMi::Tuple;
    bkpt.addChild(constMi("number", bp.number));
    // An exception breakpoint stands at no line of any file, so it says
    // nothing about one: what the model reads there it also marks with.
    if (bp.type == BreakpointByFileAndLine || bp.type == BreakpointByFunction) {
        if (!bp.file.isEmpty()) {
            bkpt.addChild(constMi("file", bp.file.path()));
            bkpt.addChild(constMi("fullname", bp.file.path()));
        }
        if (const int line = bp.actualLine ? bp.actualLine : bp.line; line > 0)
            bkpt.addChild(constMi("line", QString::number(line)));
    }
    if (bp.type == BreakpointByFunction)
        bkpt.addChild(constMi("func", bp.className + '.' + bp.memberName));
    bkpt.addChild(constMi("enabled", bp.enabled ? "y" : "n"));
    // Also -1, as the model keeps a field that is not reported.
    bkpt.addChild(constMi("thread", QString::number(bp.threadSpec)));
    // An exception breakpoint waits for no class to load: it is in as soon as
    // its request is, and a disabled one asks for no request at all. The field
    // is written only where it holds, since the model reads the presence of it
    // rather than what it says, as gdb, which only writes it then, allows. It
    // says nothing: what it says is read as the location of a breakpoint that
    // named no file, and a breakpoint at a throw names none.
    const bool pending = bp.type == BreakpointAtThrow ? bp.enabled && bp.requests.isEmpty()
                                                      : bp.locations.isEmpty();
    if (pending)
        bkpt.addChild(constMi("pending", {}));
    bkpt.addChild(constMi("times", QString::number(bp.hits)));
    if (bp.locations.size() > 1) {
        GdbMi locations;
        locations.m_type = GdbMi::List;
        locations.m_name = "locations";
        for (const Breakpoint::Location &location : bp.locations)
            locations.addChild(locationData(bp, location));
        bkpt.addChild(locations);
    }
    return bkpt;
}

GdbMi JdwpImpl::breakpointReport(const Breakpoint &bp) const
{
    GdbMi data;
    data.m_type = GdbMi::List;
    data.addChild(breakpointData(bp));
    if (bp.locations.size() > 1) {
        for (const Breakpoint::Location &location : bp.locations)
            data.addChild(locationData(bp, location));
    }
    return data;
}

void JdwpImpl::updateFromBreakpointRequest(Breakpoint &bp, const BreakpointParameters &params)
{
    bp.enabled = params.enabled;
    bp.condition = params.condition;
    // As in gdb, a count ignores the hits from the time it is set on.
    if (params.ignoreCount != bp.ignoreCount)
        bp.ignoredSince = bp.hits;
    bp.ignoreCount = params.ignoreCount;
    bp.threadSpec = params.threadSpec;
    bp.tracepoint = params.tracepoint;
    bp.message = params.message;
    bp.oneShot = params.oneShot;
    if (namesAClass(bp.type)) {
        const QString spec = bp.type == BreakpointByFunction ? params.functionName
                                                             : params.expression;
        bp.className = classOfMember(spec);
        bp.memberName = memberOfMember(spec);
        return;
    }
    if (bp.type != BreakpointByFileAndLine)
        return;
    bp.file = params.fileName;
    bp.sourceName = params.fileName.fileName();
    bp.line = params.textPosition.line;
    bp.packageKnown = false;
    bp.packagePath.clear();
    // The class signatures name the package, and the file says which one it
    // belongs to. A file that cannot be read matches its name in any package.
    if (const Result<QByteArray> contents = params.fileName.fileContents()) {
        bp.packagePath = packageDirectoryOf(QString::fromUtf8(*contents));
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
        const BreakpointType type = request.params.type;
        if (type != BreakpointByFileAndLine && type != BreakpointAtThrow
            && type != BreakpointByFunction && type != WatchpointAtExpression) {
            emit breakpointEvent(requestId, BreakpointOp::Insert, false);
            return;
        }
        Breakpoint bp;
        bp.number = QString::number(m_nextBreakpointNumber++);
        bp.type = type;
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
            const QString spec = bp->className + '.' + bp->memberName;
            if (merged.functionName.isEmpty())
                merged.functionName = spec;
            if (merged.expression.isEmpty())
                merged.expression = spec;
            updateFromBreakpointRequest(*bp, merged);
            armBreakpoint(number, [this, number, requestId] {
                const Breakpoint *bp = breakpoint(number);
                GdbMi data;
                data.m_type = GdbMi::List;
                if (bp)
                    data.addChild(breakpointData(*bp));
                emit breakpointEvent(requestId, BreakpointOp::Update, bp != nullptr, data);
            });
        });
        return;
    }
    case BreakpointOp::EnableSub: {
        const QString number = request.subResponseId;
        if (!breakpointLocation(number)) {
            emit breakpointEvent(requestId, BreakpointOp::EnableSub, false);
            return;
        }
        enableLocation(number, request.enabled, [this, requestId] {
            emit breakpointEvent(requestId, BreakpointOp::EnableSub, true);
        });
        return;
    }
    }
}

JdwpImpl::Modifiers JdwpImpl::commonModifiers(const Breakpoint &bp) const
{
    Modifiers modifiers;
    // No CountModifier for a one-shot: a hit turned down by the condition or
    // the ignore count would spend it.
    if (bp.threadSpec >= 0) {
        modifiers.add(writer().writeByte(Jdwp::ThreadOnlyModifier)
                          .writeObjectId(quint64(bp.threadSpec)).data());
    }
    return modifiers;
}

QString JdwpImpl::Breakpoint::Location::key() const
{
    if (where.isValid())
        return QString("%1/%2/%3").arg(where.classId).arg(where.methodId).arg(where.index);
    return QString("%1#%2").arg(fieldClass).arg(fieldId);
}

QString JdwpImpl::addLocation(Breakpoint &bp, const Breakpoint::Location &location)
{
    Breakpoint::Location added = location;
    added.number = QString("%1.%2").arg(bp.number).arg(bp.nextLocationNumber++);
    added.enabled = !bp.disabledLocations.contains(added.key());
    bp.locations.append(added);
    return added.number;
}

void JdwpImpl::armLocation(const QString &number, const QString &locationNumber, const Done &done)
{
    const Breakpoint *bp = breakpoint(number);
    const Breakpoint::Location *location = breakpointLocation(locationNumber);
    if (!bp || !location || location->request != 0 || !bp->enabled || !location->enabled) {
        done();
        return;
    }
    Modifiers modifiers = commonModifiers(*bp);
    if (location->where.isValid()) {
        modifiers.add(writer().writeByte(Jdwp::LocationOnlyModifier)
                          .writeLocation(location->where).data());
    } else {
        modifiers.add(writer().writeByte(Jdwp::FieldOnlyModifier)
                          .writeReferenceTypeId(location->fieldClass)
                          .writeFieldId(location->fieldId).data());
    }
    const quint8 kind = eventKindOf(bp->type);
    const QByteArray request = writer().writeByte(kind).writeByte(Jdwp::SuspendAll)
                                   .writeInt(modifiers.count).data() + modifiers.data;
    send(Jdwp::EventRequestSet, Jdwp::EventRequestSetCommand, request,
         [this, locationNumber, kind, done](const JdwpReply &reply) {
        if (reply.ok()) {
            const qint32 id = reader(reply.data).readInt();
            // Taken back, switched off or armed again while the request was
            // out, it must not stop anything.
            m_givenUpRequests.remove({kind, id});
            const Breakpoint *bp = breakpoint(locationNumber.section('.', 0, 0));
            Breakpoint::Location *location = breakpointLocation(locationNumber);
            if (bp && location && bp->enabled && location->enabled && location->request == 0)
                location->request = id;
            else
                clearRequest(kind, id);
        } else {
            dropLocation(locationNumber, JdwpClient::errorString(reply.errorCode));
        }
        done();
    });
}

void JdwpImpl::dropLocation(const QString &locationNumber, const QString &reason)
{
    Breakpoint *bp = breakpoint(locationNumber.section('.', 0, 0));
    if (!bp)
        return;
    const int at = Utils::indexOf(bp->locations,
                                  [&locationNumber](const Breakpoint::Location &location) {
        return location.number == locationNumber;
    });
    if (at < 0)
        return;
    const QString where = bp->locations.at(at).function;
    bp->locations.removeAt(at);
    if (bp->internal)
        return;
    emit message(Tr::tr("The virtual machine will not stop at %1: %2")
                     .arg(where.isEmpty() ? bp->number : where, reason), LogWarning);
}

void JdwpImpl::enableLocation(const QString &locationNumber, bool enabled, const Done &done)
{
    const QString number = locationNumber.section('.', 0, 0);
    Breakpoint *bp = breakpoint(number);
    Breakpoint::Location *location = breakpointLocation(locationNumber);
    if (!bp || !location) {
        done();
        return;
    }
    location->enabled = enabled;
    if (enabled) {
        bp->disabledLocations.remove(location->key());
        armLocation(number, locationNumber, done);
        return;
    }
    bp->disabledLocations.insert(location->key());
    const qint32 request = std::exchange(location->request, 0);
    if (request == 0) {
        done();
        return;
    }
    clearRequest(eventKindOf(bp->type), request, done);
}

void JdwpImpl::armBreakpoint(const QString &number, const Done &done)
{
    const Breakpoint *bp = breakpoint(number);
    QTC_ASSERT(bp, done(); return);

    const auto answer = [this, number, done] {
        if (Breakpoint *bp = breakpoint(number))
            bp->answered = true;
        done();
    };

    // An exception is thrown by code that is running already, so there is
    // nothing to wait for: one request covers the whole virtual machine.
    if (bp->type == BreakpointAtThrow) {
        if (!bp->enabled) {
            answer();
            return;
        }
        Modifiers modifiers = commonModifiers(*bp);
        // A null class stands for every exception there is, and both flags for
        // one whether or not a handler is waiting for it.
        modifiers.add(writer().writeByte(Jdwp::ExceptionOnlyModifier).writeReferenceTypeId(0)
                          .writeBool(true).writeBool(true).data());
        const QByteArray request = writer().writeByte(Jdwp::ExceptionEvent)
                                       .writeByte(Jdwp::SuspendAll)
                                       .writeInt(modifiers.count).data() + modifiers.data;
        send(Jdwp::EventRequestSet, Jdwp::EventRequestSetCommand, request,
             [this, number, answer](const JdwpReply &reply) {
            if (reply.ok()) {
                const qint32 id = reader(reply.data).readInt();
                // Taken back or switched off while the request was out, or
                // armed a second time over, it must not stop anything.
                m_givenUpRequests.remove({Jdwp::ExceptionEvent, id});
                Breakpoint *bp = breakpoint(number);
                if (bp && bp->enabled && bp->requests.isEmpty())
                    bp->requests.append(id);
                else
                    clearRequest(Jdwp::ExceptionEvent, id);
            }
            answer();
        });
        return;
    }

    const auto join = std::make_shared<Join>(answer);
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
    if (namesAClass(bp->type)) {
        // A class match takes one wildcard, at one end. A full name would need
        // the machine's spelling of nested classes, so "*Name" is asked for and
        // the names sort the answers out below.
        const QString simpleName = bp->className.section('.', -1).section('$', -1);
        request.writeInt(1);
        request.writeByte(Jdwp::ClassMatchModifier).writeString('*' + simpleName);
    } else if (m_client.hasCapability(Jdwp::CanUseSourceNameFilters)) {
        request.writeInt(1);
        request.writeByte(Jdwp::SourceNameMatchModifier).writeString(bp->sourceName);
    } else if (!package.isEmpty()) {
        request.writeInt(1);
        request.writeByte(Jdwp::ClassMatchModifier).writeString(package + ".*");
    } else {
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
        const bool byClass = namesAClass(bp->type);
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
            if (byClass) {
                if (!namesClass(bp->className, signature))
                    continue;
            } else if (bp->packageKnown ? packagePath(signature) != bp->packagePath
                                        : outerClassName(signature) != baseName) {
                continue;
            }
            candidates.append(id);
        }
        const QString sourceName = bp->sourceName;
        for (const quint64 id : std::as_const(candidates)) {
            join->add();
            if (byClass) {
                resolveInClass(number, id, [join] { join->release(); });
                continue;
            }
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

void JdwpImpl::clearRequestsOf(const QString &number, const Done &done)
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
    const quint8 kind = eventKindOf(bp->type);
    for (const qint32 id : std::exchange(bp->requests, {})) {
        join->add();
        clearRequest(kind, id, [join] { join->release(); });
    }
    for (Breakpoint::Location &location : bp->locations) {
        const qint32 id = std::exchange(location.request, 0);
        if (id == 0)
            continue;
        join->add();
        clearRequest(kind, id, [join] { join->release(); });
    }
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
    join->add();
    clearRequestsOf(number, [join] { join->release(); });
    bp->classPrepareRequest = 0;
    bp->classes.clear();
    bp->locations.clear();
    bp->nextLocationNumber = 1;
    bp->actualLine = 0;
    join->release();
}

void JdwpImpl::resolveInClass(const QString &number, quint64 classId, const Done &done)
{
    Breakpoint *bp = breakpoint(number);
    if (!bp || bp->classes.contains(classId)) {
        done();
        return;
    }
    bp->classes.insert(classId);

    switch (bp->type) {
    case BreakpointByFunction:
        resolveMethodInClass(number, classId, done);
        return;
    case WatchpointAtExpression:
        resolveFieldInClass(number, classId, done);
        return;
    default:
        resolveLineInClass(number, classId, done);
        return;
    }
}

// Every method of the class whose code is on the line gets a location, or on
// the next line with code where the line itself has none, as gdb does it.
void JdwpImpl::resolveLineInClass(const QString &number, quint64 classId, const Done &done)
{
    withMethods(classId, [this, number, classId, done] {
        const auto join = std::make_shared<Join>([this, number, classId, done] {
            Breakpoint *bp = breakpoint(number);
            if (!bp) {
                done();
                return;
            }
            const ClassInfo &info = m_classes[classId];
            const QString className = typeName(info.signature);
            QList<Breakpoint::Location> found;
            const auto collect = [&info, &found, &className, classId](int line) {
                for (const MethodInfo &method : info.methods) {
                    quint64 best = std::numeric_limits<quint64>::max();
                    for (const auto &[index, entryLine] : method.lines) {
                        if (entryLine == line && index < best)
                            best = index;
                    }
                    if (best == std::numeric_limits<quint64>::max())
                        continue;
                    Breakpoint::Location location;
                    location.where = {info.tag, classId, method.id, best};
                    location.function = className + '.' + method.name;
                    location.line = line;
                    found.append(location);
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

            QStringList added;
            for (const Breakpoint::Location &location : std::as_const(found))
                added.append(addLocation(*bp, location));

            const auto setJoin = std::make_shared<Join>(done);
            setJoin->add();
            for (const QString &locationNumber : std::as_const(added)) {
                setJoin->add();
                armLocation(number, locationNumber, [setJoin] { setJoin->release(); });
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

// One location per overload, at the first line of the method rather than its
// first instruction, so that the arguments are in place.
void JdwpImpl::resolveMethodInClass(const QString &number, quint64 classId, const Done &done)
{
    withMethods(classId, [this, number, classId, done] {
        const Breakpoint *bp = breakpoint(number);
        if (!bp) {
            done();
            return;
        }
        const QString member = bp->memberName;
        QList<quint64> wanted;
        const QList<MethodInfo> methods = m_classes.value(classId).methods;
        for (const MethodInfo &method : methods) {
            if (method.name == member)
                wanted.append(method.id);
        }
        if (wanted.isEmpty()) {
            done();
            return;
        }

        const auto join = std::make_shared<Join>([this, number, classId, wanted, done] {
            Breakpoint *bp = breakpoint(number);
            if (!bp) {
                done();
                return;
            }
            const ClassInfo &info = m_classes[classId];
            const QString className = typeName(info.signature);
            const Utils::FilePath source = localFile(info.signature, info.sourceFile);
            QStringList added;
            for (const quint64 methodId : wanted) {
                const MethodInfo *found = method(classId, methodId);
                // Abstract or native.
                if (!found || found->lines.isEmpty())
                    continue;
                QPair<quint64, int> first = found->lines.first();
                for (const auto &entry : found->lines) {
                    if (entry.first < first.first)
                        first = entry;
                }
                Breakpoint::Location location;
                location.where = {info.tag, classId, methodId, first.first};
                location.function = className + '.' + found->name;
                location.file = source;
                location.line = first.second;
                added.append(addLocation(*bp, location));
                if (bp->actualLine == 0) {
                    bp->actualLine = first.second;
                    bp->file = source;
                }
            }
            if (added.isEmpty()) {
                done();
                return;
            }
            const auto setJoin = std::make_shared<Join>(done);
            setJoin->add();
            for (const QString &locationNumber : std::as_const(added)) {
                setJoin->add();
                armLocation(number, locationNumber, [setJoin] { setJoin->release(); });
            }
            setJoin->release();
        });

        join->add();
        for (const quint64 methodId : std::as_const(wanted)) {
            join->add();
            withLineTable(classId, methodId, [join] { join->release(); });
        }
        join->add();
        withSourceFile(classId, [join] { join->release(); });
        join->release();
    });
}

void JdwpImpl::resolveFieldInClass(const QString &number, quint64 classId, const Done &done)
{
    withFields(classId, [this, number, classId, done] {
        Breakpoint *bp = breakpoint(number);
        if (!bp) {
            done();
            return;
        }
        const ClassInfo &info = m_classes[classId];
        const auto named = [bp](const FieldInfo &field) { return field.name == bp->memberName; };
        // The nearest declaration wins, static or not. The superclasses come
        // first among the instance fields, so this class is looked at first.
        const auto declaredHere = [classId, &named](const FieldInfo &field) {
            return field.declaringClass == classId && named(field);
        };
        const FieldInfo *found = nullptr;
        for (const QList<FieldInfo> *fields : {&info.fields, &info.staticFields}) {
            const auto it = std::find_if(fields->cbegin(), fields->cend(), declaredHere);
            if (it != fields->cend()) {
                found = &*it;
                break;
            }
        }
        if (!found) {
            const auto inherited = std::find_if(info.fields.crbegin(), info.fields.crend(), named);
            if (inherited != info.fields.crend()) {
                found = &*inherited;
            } else {
                const auto shared = std::find_if(info.staticFields.cbegin(),
                                                 info.staticFields.cend(), named);
                if (shared != info.staticFields.cend())
                    found = &*shared;
            }
        }
        if (!found) {
            done();
            return;
        }
        // Two classes that inherit the same field are one field to watch.
        const quint64 fieldClass = found->declaringClass;
        const quint64 fieldId = found->id;
        const bool known = Utils::anyOf(bp->locations,
                                        [fieldClass, fieldId](const Breakpoint::Location &other) {
            return other.fieldClass == fieldClass && other.fieldId == fieldId;
        });
        if (known) {
            done();
            return;
        }
        Breakpoint::Location location;
        location.fieldClass = fieldClass;
        location.fieldId = fieldId;
        location.function = typeName(info.signature) + '.' + bp->memberName;
        armLocation(number, addLocation(*bp, location), done);
    });
}

void JdwpImpl::handleClassPrepares(const QList<JdwpEvent> &events, const Done &done)
{
    QHash<QString, qsizetype> before;
    for (const Breakpoint &bp : std::as_const(m_breakpoints)) {
        if (!bp.internal && bp.answered && bp.type != BreakpointAtThrow)
            before.insert(bp.number, bp.locations.size());
    }
    const auto join = std::make_shared<Join>([this, before, done] {
        for (auto it = before.cbegin(); it != before.cend(); ++it) {
            const Breakpoint *bp = breakpoint(it.key());
            if (bp && bp->locations.size() != it.value())
                emit breakpointModified(breakpointReport(*bp));
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
        // By name rather than by request: a class can be prepared before the
        // answer that says which request is the breakpoint's has been read.
        QStringList byName;
        QStringList bySource;
        for (const Breakpoint &bp : std::as_const(m_breakpoints)) {
            if (namesAClass(bp.type)) {
                if (namesClass(bp.className, event.signature))
                    byName.append(bp.number);
                continue;
            }
            if (bp.type != BreakpointByFileAndLine)
                continue;
            if (bp.packageKnown) {
                if (packagePath(event.signature) != bp.packagePath)
                    continue;
            } else if (!m_client.hasCapability(Jdwp::CanUseSourceNameFilters)
                       && outerClassName(event.signature) != bp.file.completeBaseName()) {
                // Unfiltered, so asking every class for its source file would
                // be a round trip per class that loads.
                continue;
            }
            bySource.append(bp.number);
        }
        for (const QString &number : std::as_const(byName)) {
            join->add();
            resolveInClass(number, classId, [join] { join->release(); });
        }
        for (const QString &number : std::as_const(bySource)) {
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
    // Read before the last call into the virtual machine, and so no longer
    // anything the machine would recognize.
    if (m_framesFrom != m_callGeneration) {
        m_frames.clear();
        m_framesComplete = false;
    }
    if (m_framesComplete || (count > 0 && m_frames.size() >= count)) {
        done();
        return;
    }
    const int generation = m_stopGeneration;
    const int callGeneration = m_callGeneration;
    const quint64 thread = m_currentThread;
    // Asking for more frames than there are is an error, so the count comes first.
    send(Jdwp::ThreadReferenceSet, Jdwp::ThreadFrameCount, writer().writeObjectId(thread).data(),
         [this, count, done, generation, callGeneration, thread](const JdwpReply &reply) {
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
            m_framesFrom = callGeneration;
            done();
            return;
        }
        JdwpWriter request = writer();
        request.writeObjectId(thread).writeInt(0).writeInt(wanted);
        send(Jdwp::ThreadReferenceSet, Jdwp::ThreadFrames, request.data(),
             [this, done, generation, callGeneration, available, wanted](const JdwpReply &reply) {
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
            m_framesFrom = callGeneration;
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

// A virtual machine is made of no libraries: what it was started with is the
// class path, which is where its code comes from and the nearest thing it has.
void JdwpImpl::reportModules(quint64 requestId)
{
    send(Jdwp::VirtualMachineSet, Jdwp::VmClassPaths, {},
         [this, requestId](const JdwpReply &reply) {
        GdbMi list;
        list.m_type = GdbMi::List;
        if (reply.ok()) {
            JdwpReader r = reader(reply.data);
            // Where the machine was started, which the entries behind it may
            // be written relative to.
            const QString base = r.readString();
            // The class path of the program, and then the one the runtime
            // brings with it.
            for (int which = 0; which < 2 && r.ok(); ++which) {
                const qint32 count = r.readInt();
                for (qint32 i = 0; i < count && r.ok(); ++i) {
                    const QString entry = r.readString();
                    if (entry.isEmpty())
                        continue;
                    FilePath path = FilePath::fromUserInput(entry);
                    if (path.isRelativePath() && !base.isEmpty())
                        path = FilePath::fromUserInput(base).resolvePath(entry);
                    GdbMi item;
                    item.m_type = GdbMi::Tuple;
                    item.addChild(constMi("modulepath", path.toUserOutput()));
                    list.addChild(item);
                }
            }
        }
        emit refreshDataReceived(requestId, RefreshKind::Modules, list);
    });
}

// The sources of the classes that are the program's. The runtime's own are
// thousands, and there is no source on this computer for any of them.
void JdwpImpl::reportSourceFiles(quint64 requestId)
{
    withAllClasses([this, requestId] {
        QList<quint64> wanted;
        for (auto it = m_classes.cbegin(); it != m_classes.cend(); ++it) {
            if (it->tag == Jdwp::ArrayTag || !it->signatureKnown)
                continue;
            const QString name = typeName(it->signature);
            if (name.isEmpty() || isRuntimeClass(name))
                continue;
            wanted.append(it.key());
        }
        const auto join = std::make_shared<Join>([this, requestId, wanted] {
            GdbMi list;
            list.m_type = GdbMi::List;
            QSet<QString> seen;
            for (const quint64 classId : wanted) {
                const ClassInfo info = m_classes.value(classId);
                if (info.sourceFile.isEmpty())
                    continue;
                // The classes of one file are many, and the file is one.
                const QString package = packagePath(info.signature);
                const QString relative = package.isEmpty() ? info.sourceFile
                                                           : package + '/' + info.sourceFile;
                if (seen.contains(relative))
                    continue;
                seen.insert(relative);
                GdbMi item;
                item.m_type = GdbMi::Tuple;
                item.addChild(constMi("file", relative));
                const FilePath local = localFile(info.signature, info.sourceFile);
                if (!local.isEmpty())
                    item.addChild(constMi("fullname", local.path()));
                list.addChild(item);
            }
            emit refreshDataReceived(requestId, RefreshKind::SourceFiles, list);
        });
        join->add();
        for (const quint64 classId : std::as_const(wanted)) {
            join->add();
            withSourceFile(classId, [join] { join->release(); });
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
    case RefreshKind::Modules:
        reportModules(request.requestId);
        return;
    case RefreshKind::SourceFiles:
        reportSourceFiles(request.requestId);
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

static bool isFloating(quint8 tag)
{
    return tag == Jdwp::FloatValueTag || tag == Jdwp::DoubleValueTag;
}

static bool isNumeric(quint8 tag)
{
    return Jdwp::isPrimitiveTag(tag) && tag != Jdwp::BooleanValueTag;
}

static double asDouble(const JdwpValue &value)
{
    if (value.tag == Jdwp::FloatValueTag) {
        const quint32 bits = quint32(value.bits);
        float f = 0;
        std::memcpy(&f, &bits, sizeof(f));
        return f;
    }
    if (value.tag == Jdwp::DoubleValueTag) {
        const quint64 bits = value.bits;
        double d = 0;
        std::memcpy(&d, &bits, sizeof(d));
        return d;
    }
    if (value.tag == Jdwp::CharValueTag)
        return double(quint16(value.bits));
    return double(qint64(value.bits));
}

// What a Java cast of a floating value to an integral type of this width
// gives: a value that does not fit stops at the end of the range, where the
// conversion would be undefined here.
static qint64 saturated(double number, bool wide)
{
    if (std::isnan(number))
        return 0;
    const qint64 high = wide ? std::numeric_limits<qint64>::max()
                             : std::numeric_limits<qint32>::max();
    const qint64 low = wide ? std::numeric_limits<qint64>::min()
                            : std::numeric_limits<qint32>::min();
    if (number >= double(high))
        return high;
    return number <= double(low) ? low : qint64(number);
}

static qint64 asInteger(const JdwpValue &value)
{
    if (isFloating(value.tag))
        return saturated(asDouble(value), true);
    if (value.tag == Jdwp::CharValueTag)
        return qint64(quint16(value.bits));
    return qint64(value.bits);
}

// Java wraps where signed arithmetic overflows, which C++ leaves undefined.
static qint64 wrapped(quint64 bits)
{
    return qint64(bits);
}

static JdwpValue madeOf(double number)
{
    quint64 bits = 0;
    std::memcpy(&bits, &number, sizeof(number));
    return {Jdwp::DoubleValueTag, bits};
}

static JdwpValue madeOf(float number)
{
    quint32 bits = 0;
    std::memcpy(&bits, &number, sizeof(number));
    return {Jdwp::FloatValueTag, bits};
}

static JdwpValue madeOf(qint64 number, bool wide)
{
    return {quint8(wide ? Jdwp::LongValueTag : Jdwp::IntValueTag),
            wide ? quint64(number) : quint64(qint32(number))};
}

static JdwpValue madeOf(bool yes)
{
    return {Jdwp::BooleanValueTag, quint64(yes ? 1 : 0)};
}

// The signatures of what a method takes, "(ILjava/lang/String;)V" giving "I"
// and "Ljava/lang/String;".
static QStringList parameterSignatures(const QString &signature)
{
    QStringList parameters;
    qsizetype pos = signature.indexOf('(');
    if (pos < 0)
        return parameters;
    ++pos;
    while (pos < signature.size() && signature.at(pos) != ')') {
        const qsizetype start = pos;
        while (pos < signature.size() && signature.at(pos) == '[')
            ++pos;
        if (pos >= signature.size())
            break;
        if (signature.at(pos) == 'L') {
            const qsizetype end = signature.indexOf(';', pos);
            if (end < 0)
                break;
            pos = end + 1;
        } else {
            ++pos;
        }
        parameters.append(signature.mid(start, pos - start));
    }
    return parameters;
}

static QString returnSignature(const QString &signature)
{
    const qsizetype close = signature.lastIndexOf(')');
    return close < 0 ? QString() : signature.mid(close + 1);
}

static bool isPrimitiveSignature(const QString &signature)
{
    return signature.size() == 1 && signature != "V";
}

// A type variable, or an array of one, which the erased signature hides.
static bool isTypeVariable(const QString &genericSignature)
{
    qsizetype pos = 0;
    while (pos < genericSignature.size() && genericSignature.at(pos) == '[')
        ++pos;
    return pos < genericSignature.size() && genericSignature.at(pos) == 'T';
}

// The static type a declaration gives what it holds, or nothing where that
// is a type variable.
static QString staticSignature(const QString &signature, const QString &genericSignature)
{
    return isTypeVariable(genericSignature) ? QString() : signature;
}

static QString signatureOf(const JdwpValue &value)
{
    return Jdwp::isPrimitiveTag(value.tag) ? QString(QChar::fromLatin1(char(value.tag)))
                                           : QString();
}

// Java widens a primitive silently in this order and in no other, and
// nothing widens to a char (JLS 5.1.2).
static bool widens(QChar from, QChar to)
{
    if (from == to)
        return true;
    switch (from.toLatin1()) {
    case 'B':
        return QString("SIJFD").contains(to);
    case 'S':
    case 'C':
        return QString("IJFD").contains(to);
    case 'I':
        return QString("JFD").contains(to);
    case 'J':
        return QString("FD").contains(to);
    case 'F':
        return to == 'D';
    }
    return false;
}

static const QList<std::pair<QChar, QString>> &wrappers()
{
    static const QList<std::pair<QChar, QString>> list = {
        {'Z', "Ljava/lang/Boolean;"}, {'B', "Ljava/lang/Byte;"},
        {'C', "Ljava/lang/Character;"}, {'S', "Ljava/lang/Short;"},
        {'I', "Ljava/lang/Integer;"}, {'J', "Ljava/lang/Long;"},
        {'F', "Ljava/lang/Float;"}, {'D', "Ljava/lang/Double;"}};
    return list;
}

static QString boxed(QChar primitive)
{
    for (const auto &[kind, wrapper] : wrappers()) {
        if (kind == primitive)
            return wrapper;
    }
    return {};
}

static QChar unboxed(const QString &signature)
{
    for (const auto &[kind, wrapper] : wrappers()) {
        if (wrapper == signature)
            return kind;
    }
    return {};
}

using Subtypes = QHash<QString, QSet<QString>>;

// Whether a value of the one type goes to the other with no more than a
// widening (JLS 4.10): the same type, a primitive that widens, or a subtype.
// An empty signature is the null type.
static bool isSubtype(const QString &sub, const QString &super, const Subtypes &subtypes)
{
    if (sub == super)
        return true;
    if (sub.isEmpty())
        return !isPrimitiveSignature(super);
    if (isPrimitiveSignature(sub) || isPrimitiveSignature(super)) {
        return isPrimitiveSignature(sub) && isPrimitiveSignature(super)
               && widens(sub.at(0), super.at(0));
    }
    if (super == "Ljava/lang/Object;")
        return true;
    if (sub.startsWith('[')) {
        if (super == "Ljava/lang/Cloneable;" || super == "Ljava/io/Serializable;")
            return true;
        // An array of references is one of every supertype of what it holds.
        return super.startsWith('[') && !isPrimitiveSignature(sub.mid(1))
               && !isPrimitiveSignature(super.mid(1))
               && isSubtype(sub.mid(1), super.mid(1), subtypes);
    }
    return subtypes.value(sub).contains(super);
}

// The parameters the arguments go to. In a call of variable arity, the type
// of the elements of the last one stands in for every argument from there.
static QStringList parameterTypesFor(const QStringList &parameters, bool varArgs,
                                     qsizetype count)
{
    if (!varArgs || parameters.isEmpty())
        return parameters;
    QStringList types = parameters.first(parameters.size() - 1);
    const QString element = parameters.last().mid(1);
    while (types.size() < count)
        types.append(element);
    return types;
}

// Whether the argument goes to the parameter in the phase: by subtyping and
// widening alone, or also by boxing and unboxing (JLS 5.3).
static bool converts(const QString &argument, const QString &parameter, bool loose,
                     const Subtypes &subtypes)
{
    if (isSubtype(argument, parameter, subtypes))
        return true;
    if (!loose || argument.isEmpty())
        return false;
    if (isPrimitiveSignature(argument)) {
        return !isPrimitiveSignature(parameter)
               && isSubtype(boxed(argument.at(0)), parameter, subtypes);
    }
    const QChar primitive = unboxed(argument);
    return !primitive.isNull() && isPrimitiveSignature(parameter)
           && widens(primitive, parameter.at(0));
}

// The three phases of JLS 15.12.2, each tried only where the one before
// found nothing: strict, loose, and then by variable arity.
enum class Phase { Strict, Loose, VariableArity };

static bool isApplicable(const QStringList &parameters, bool varArgs,
                         const QStringList &arguments, Phase phase, const Subtypes &subtypes)
{
    const bool byArity = phase == Phase::VariableArity;
    if (byArity ? !varArgs || arguments.size() < parameters.size() - 1
                : arguments.size() != parameters.size()) {
        return false;
    }
    const QStringList types = parameterTypesFor(parameters, byArity, arguments.size());
    for (qsizetype i = 0; i < arguments.size(); ++i) {
        if (!converts(arguments.at(i), types.at(i), phase != Phase::Strict, subtypes))
            return false;
    }
    return true;
}

// Whether the one method is at least as specific as the other for a call
// with that many arguments (JLS 15.12.2.5).
static bool isMoreSpecific(const QStringList &one, const QStringList &other, bool byArity,
                           qsizetype count, const Subtypes &subtypes)
{
    const QStringList ones = parameterTypesFor(one, byArity, count);
    const QStringList others = parameterTypesFor(other, byArity, count);
    for (qsizetype i = 0; i < count; ++i) {
        if (!isSubtype(ones.at(i), others.at(i), subtypes))
            return false;
    }
    if (byArity && other.size() == count + 1) {
        return isSubtype(parameterTypesFor(one, true, count + 1).at(count),
                         other.last().mid(1), subtypes);
    }
    return true;
}

// The one that is more specific than all the others, or -1 where none is,
// which makes the call ambiguous. Two with the same parameters cannot both
// be there, since a declaration hides those it overrides.
static qsizetype mostSpecific(const QList<QStringList> &applicable, bool byArity,
                              qsizetype count, const Subtypes &subtypes)
{
    for (qsizetype i = 0; i < applicable.size(); ++i) {
        bool beatsAll = true;
        for (qsizetype j = 0; j < applicable.size() && beatsAll; ++j) {
            beatsAll = i == j
                       || isMoreSpecific(applicable.at(i), applicable.at(j), byArity, count,
                                         subtypes);
        }
        if (beatsAll)
            return i;
    }
    return -1;
}

// The primitive as a parameter of a wider type takes it.
static JdwpValue widenedTo(const JdwpValue &value, QChar kind)
{
    const quint8 want = quint8(kind.toLatin1());
    switch (want) {
    case Jdwp::ShortValueTag:
        return JdwpValue{want, quint64(qint16(asInteger(value)))};
    case Jdwp::IntValueTag:
        return JdwpValue{want, quint64(qint32(asInteger(value)))};
    case Jdwp::LongValueTag:
        return JdwpValue{want, quint64(asInteger(value))};
    case Jdwp::FloatValueTag: {
        const float number = isFloating(value.tag) ? float(asDouble(value))
                                                   : float(asInteger(value));
        quint32 bits = 0;
        std::memcpy(&bits, &number, sizeof(number));
        return JdwpValue{want, bits};
    }
    case Jdwp::DoubleValueTag: {
        const double number = isFloating(value.tag) ? asDouble(value)
                                                    : double(asInteger(value));
        quint64 bits = 0;
        std::memcpy(&bits, &number, sizeof(number));
        return JdwpValue{want, bits};
    }
    }
    return value;
}

// What Java makes of two values and an operator, as far as it needs no code to
// run in the virtual machine. The promotions are Java's: a double operand
// makes the result double, else a float one float, else a long one long, and
// anything narrower is an int.
static Utils::Result<JdwpValue> combined(JdwpExpression::Operator op, const JdwpValue &left,
                                         const JdwpValue &right)
{
    using Op = JdwpExpression::Operator;
    const bool comparison = op == Op::Equal || op == Op::NotEqual;
    if (left.isObject() || right.isObject()) {
        if (!comparison) {
            return ResultError(Tr::tr("An object is not something to do arithmetic on, and "
                                      "a method cannot be called here."));
        }
        if (!left.isObject() || !right.isObject())
            return ResultError(Tr::tr("An object and a number cannot be compared."));
        // Java compares the references, and so does this.
        return madeOf((left.bits == right.bits) == (op == Op::Equal));
    }
    if (left.tag == Jdwp::BooleanValueTag || right.tag == Jdwp::BooleanValueTag) {
        if (left.tag != right.tag || !comparison)
            return ResultError(Tr::tr("A boolean can only be compared with a boolean."));
        return madeOf(((left.bits != 0) == (right.bits != 0)) == (op == Op::Equal));
    }
    if (!isNumeric(left.tag) || !isNumeric(right.tag))
        return ResultError(Tr::tr("These are not values to combine."));

    const bool real = isFloating(left.tag) || isFloating(right.tag);
    const bool wide = left.tag == Jdwp::LongValueTag || right.tag == Jdwp::LongValueTag;
    if (real) {
        // Float arithmetic done in double and rounded once is exact, while the
        // operands have to be rounded to float first.
        const bool single = left.tag != Jdwp::DoubleValueTag
                            && right.tag != Jdwp::DoubleValueTag;
        const auto operand = [single](const JdwpValue &value) -> double {
            if (!single || value.tag == Jdwp::FloatValueTag)
                return asDouble(value);
            return isFloating(value.tag) ? float(asDouble(value)) : float(asInteger(value));
        };
        const auto number = [single](double result) {
            return single ? madeOf(float(result)) : madeOf(result);
        };
        const double a = operand(left);
        const double b = operand(right);
        switch (op) {
        case Op::Add: return number(a + b);
        case Op::Subtract: return number(a - b);
        case Op::Multiply: return number(a * b);
        case Op::Divide: return number(a / b);
        case Op::Remainder: return number(std::fmod(a, b));
        case Op::Less: return madeOf(a < b);
        case Op::LessEqual: return madeOf(a <= b);
        case Op::Greater: return madeOf(a > b);
        case Op::GreaterEqual: return madeOf(a >= b);
        case Op::Equal: return madeOf(a == b);
        case Op::NotEqual: return madeOf(a != b);
        default: break;
        }
        return ResultError(Tr::tr("This operator does not work on numbers."));
    }
    const qint64 a = asInteger(left);
    const qint64 b = asInteger(right);
    switch (op) {
    case Op::Add: return madeOf(wrapped(quint64(a) + quint64(b)), wide);
    case Op::Subtract: return madeOf(wrapped(quint64(a) - quint64(b)), wide);
    case Op::Multiply: return madeOf(wrapped(quint64(a) * quint64(b)), wide);
    case Op::Divide:
    case Op::Remainder:
        if (b == 0)
            return ResultError(Tr::tr("The virtual machine would divide by zero."));
        // The one division that overflows traps here but wraps in Java.
        if (a == std::numeric_limits<qint64>::min() && b == -1)
            return madeOf(op == Op::Divide ? a : 0, wide);
        return madeOf(op == Op::Divide ? a / b : a % b, wide);
    case Op::Less: return madeOf(a < b);
    case Op::LessEqual: return madeOf(a <= b);
    case Op::Greater: return madeOf(a > b);
    case Op::GreaterEqual: return madeOf(a >= b);
    case Op::Equal: return madeOf(a == b);
    case Op::NotEqual: return madeOf(a != b);
    default: break;
    }
    return ResultError(Tr::tr("This operator does not work on numbers."));
}

static Utils::Result<JdwpValue> negatedOrFlipped(JdwpExpression::Operator op,
                                                 const JdwpValue &value)
{
    if (op == JdwpExpression::Operator::Not) {
        if (value.tag != Jdwp::BooleanValueTag)
            return ResultError(Tr::tr("Only a boolean can be negated with \"!\"."));
        return madeOf(value.bits == 0);
    }
    if (!isNumeric(value.tag))
        return ResultError(Tr::tr("Only a number can be negated with \"-\"."));
    if (value.tag == Jdwp::FloatValueTag)
        return madeOf(-float(asDouble(value)));
    if (isFloating(value.tag))
        return madeOf(-asDouble(value));
    return madeOf(wrapped(0 - quint64(asInteger(value))), value.tag == Jdwp::LongValueTag);
}

// The type a value that came out of an operator is shown with.
static QString typeOf(const JdwpValue &value)
{
    return Jdwp::isPrimitiveTag(value.tag) ? typeName(QChar::fromLatin1(char(value.tag)))
                                           : QString();
}

void JdwpImpl::withAllClasses(const Done &done)
{
    // Only a stopped machine keeps the classes it has. One that runs is asked
    // again every time.
    const int generation = m_runGeneration;
    fetchOnce(QString("allclasses:%1:%2").arg(generation).arg(m_running),
              [this, generation] { return !m_running && m_allClassesFrom == generation; },
              [this, generation, stopped = !m_running](const Done &finish) {
        send(Jdwp::VirtualMachineSet, Jdwp::VmAllClasses, {},
             [this, generation, stopped, finish](const JdwpReply &reply) {
            if (reply.ok()) {
                JdwpReader r = reader(reply.data);
                const qint32 count = r.readInt();
                for (qint32 i = 0; i < count && r.ok(); ++i) {
                    const quint8 tag = r.readByte();
                    const quint64 id = r.readReferenceTypeId();
                    const QString signature = r.readString();
                    r.readInt(); // status
                    if (!r.ok())
                        break;
                    ClassInfo &info = m_classes[id];
                    info.tag = tag;
                    if (!info.signatureKnown) {
                        info.signatureKnown = true;
                        info.signature = signature;
                    }
                }
                if (stopped && !m_running && generation == m_runGeneration)
                    m_allClassesFrom = generation;
            }
            finish();
        });
    }, done);
}

void JdwpImpl::classNamed(const QString &name, const std::function<void(quint64)> &done)
{
    if (name.isEmpty()) {
        done(0);
        return;
    }
    if (m_classesNamedGeneration != m_stopGeneration) {
        m_classesNamed.clear();
        m_classesNamedGeneration = m_stopGeneration;
    }
    // The few names it can be are asked for one by one, which is cheaper than
    // the list of every class of the machine, repeated at every stop.
    const auto pick = [this, name, done](quint64 frameClass, const QStringList &candidates) {
        const QString key = QString::number(frameClass) + ' ' + name;
        // A call can load the class a miss was about.
        const auto known = m_classesNamed.constFind(key);
        if (known != m_classesNamed.cend()
            && (known->first != 0 || known->second == m_callGeneration)) {
            done(known->first);
            return;
        }
        classNamedAmong(candidates, 0,
                        [this, key, done, generation = m_stopGeneration,
                         callGeneration = m_callGeneration](quint64 id) {
            if (generation == m_stopGeneration)
                m_classesNamed.insert(key, {id, callGeneration});
            done(id);
        });
    };
    // A nested class is Outer$Inner in the machine, so a dotted name is also
    // tried with its last dots read as nesting, from the right.
    const auto spellings = [](const QString &dotted) {
        QStringList result{dotted};
        QString spelled = dotted;
        for (qsizetype dot = spelled.lastIndexOf('.'); dot > 0;
             dot = spelled.lastIndexOf('.', dot - 1)) {
            spelled[dot] = '$';
            result.append(spelled);
        }
        return result;
    };
    // A plain name reads the way a source that imports nothing of its own
    // would read it: the classes enclosing the frame and its package first,
    // then java.lang.
    const auto candidatesFor = [this, name, spellings](quint64 frameClass) {
        QString package;
        QStringList candidates;
        if (frameClass) {
            const QString signature = m_classes.value(frameClass).signature;
            package = packagePath(signature);
            if (!package.isEmpty())
                package = QString(package).replace('/', '.') + '.';
            if (!name.contains('.') && signature.startsWith('L')) {
                QString enclosing = QString(signature.mid(1, signature.size() - 2))
                                        .replace('/', '.');
                for (; !enclosing.isEmpty(); enclosing.truncate(enclosing.lastIndexOf('$')))
                    candidates.append(enclosing + '$' + name);
            }
        }
        // Only the dots of the name can be nesting, never those of the package.
        if (!package.isEmpty()) {
            for (const QString &spelled : spellings(name))
                candidates.append(package + spelled);
        }
        candidates.append(spellings(name));
        if (!name.contains('.'))
            candidates.append("java.lang." + name);
        return candidates;
    };
    withCurrentFrame([this, pick, candidatesFor](const Frame *frame) {
        if (!frame) {
            pick(0, candidatesFor(0));
            return;
        }
        const quint64 frameClass = frame->location.classId;
        withSignature(frameClass, [pick, candidatesFor, frameClass] {
            pick(frameClass, candidatesFor(frameClass));
        });
    });
}

void JdwpImpl::classNamedAmong(const QStringList &candidates, qsizetype index,
                               const std::function<void(quint64)> &done)
{
    if (index >= candidates.size()) {
        done(0);
        return;
    }
    const QString signature = 'L' + QString(candidates.at(index)).replace('.', '/') + ';';
    classOfSignature(signature, [this, candidates, index, done](quint64 id) {
        if (id != 0)
            done(id);
        else
            classNamedAmong(candidates, index + 1, done);
    });
}

void JdwpImpl::classOfSignature(const QString &signature,
                                const std::function<void(quint64)> &done)
{
    send(Jdwp::VirtualMachineSet, Jdwp::VmClassesBySignature,
         writer().writeString(signature).data(),
         [this, signature, done](const JdwpReply &reply) {
        if (reply.ok()) {
            JdwpReader r = reader(reply.data);
            if (r.readInt() > 0) {
                const quint8 tag = r.readByte();
                const quint64 id = r.readReferenceTypeId();
                if (r.ok() && id != 0) {
                    ClassInfo &info = m_classes[id];
                    info.tag = tag;
                    if (!info.signatureKnown) {
                        info.signatureKnown = true;
                        info.signature = signature;
                    }
                    done(id);
                    return;
                }
            }
        }
        done(0);
    });
}

void JdwpImpl::withObjectClass(const std::function<void(quint64)> &done)
{
    if (m_objectClass != 0) {
        done(m_objectClass);
        return;
    }
    classOfSignature("Ljava/lang/Object;", [this, done](quint64 id) {
        m_objectClass = id;
        done(id);
    });
}

void JdwpImpl::classNameOf(const JdwpValue &object,
                           const std::function<void(const QString &)> &done)
{
    if (!object.isObject() || object.isNull()) {
        done({});
        return;
    }
    send(Jdwp::ObjectReferenceSet, Jdwp::ObjectReferenceType,
         writer().writeObjectId(object.bits).data(), [this, done](const JdwpReply &reply) {
        if (!reply.ok()) {
            done({});
            return;
        }
        JdwpReader r = reader(reply.data);
        r.readByte();
        const quint64 typeId = r.readReferenceTypeId();
        if (!r.ok()) {
            done({});
            return;
        }
        withSignature(typeId, [this, typeId, done] {
            done(typeName(m_classes.value(typeId).signature));
        });
    });
}

void JdwpImpl::invoke(const Stop &stop, quint64 object, quint64 classId, quint64 methodId,
                      Dispatch dispatch, const QList<JdwpValue> &arguments, const Returned &done)
{
    // The lookups that got here took round trips, in which the program can
    // have moved on, even to a stop on the same thread.
    if (const QString why = whyNoCallAt(stop); !why.isEmpty()) {
        done({.error = why});
        return;
    }
    const auto answered = std::make_shared<bool>(false);
    const auto answer = [answered, done](const Invoked &result) {
        if (!std::exchange(*answered, true))
            done(result);
    };

    // Whatever was read off a frame up to here is all there is to read: the
    // thread runs while the call does, and the machine forgets the handles of
    // the frames it was standing in.
    ++m_callGeneration;

    // The call is out until the machine answers it, which is not the same as
    // until it is waited for: one that is given up on still has its thread,
    // and one made on that thread afterwards is refused while it runs. They
    // are counted by thread, so that the answer to the one does not take the
    // other for answered, and so that a stop of any other thread is reported
    // the way it would be with no call out at all.
    const quint64 thread = stop.thread;
    if (m_callsOut[thread]++ == 0)
        m_callStopMentioned = false;

    // The two commands spell the same call out in a different order.
    JdwpWriter request = writer();
    if (dispatch == Dispatch::Static) {
        request.writeReferenceTypeId(classId).writeObjectId(thread).writeMethodId(methodId);
    } else {
        request.writeObjectId(object).writeObjectId(thread)
            .writeReferenceTypeId(classId).writeMethodId(methodId);
    }
    request.writeInt(qint32(arguments.size()));
    for (const JdwpValue &argument : arguments)
        request.writeTaggedValue(argument);
    // Single-threaded, so that the rest of the machine stays where it is and
    // the call cannot let the program get anywhere the user did not ask it to.
    request.writeInt(Jdwp::InvokeSingleThreaded);
    // The two commands are numbered within their own sets, so neither their
    // types nor their values have anything to do with one another.
    // A static method of an interface is not one of a class, and the machine
    // refuses it there.
    const bool ofAnInterface = m_classes.value(classId).tag == Jdwp::InterfaceTag;
    const bool onObject = dispatch == Dispatch::OnObject;
    const quint8 commandSet = onObject ? quint8(Jdwp::ObjectReferenceSet)
                              : ofAnInterface ? quint8(Jdwp::InterfaceTypeSet)
                                              : quint8(Jdwp::ClassTypeSet);
    const quint8 command = onObject ? quint8(Jdwp::ObjectInvokeMethod)
                           : ofAnInterface ? quint8(Jdwp::InterfaceTypeInvokeMethod)
                                           : quint8(Jdwp::ClassTypeInvokeMethod);
    send(commandSet, command, request.data(),
         [this, thread, answer](const JdwpReply &reply) {
        const auto out = m_callsOut.find(thread);
        if (out != m_callsOut.end() && --out.value() <= 0)
            m_callsOut.erase(out);
        if (!reply.ok()) {
            answer({.error = JdwpClient::errorString(reply.errorCode)});
            return;
        }
        JdwpReader r = reader(reply.data);
        const JdwpValue value = r.readTaggedValue();
        const JdwpValue thrown = r.readTaggedValue();
        if (!r.ok()) {
            answer({.error = Tr::tr("The virtual machine returned no value.")});
            return;
        }
        answer({.value = value, .thrown = thrown});
    });

    // A call that never comes back cannot be taken back, so the views go on
    // without it rather than waiting for the rest of the session.
    QTimer::singleShot(s_invokeTimeout, this, [answer] { answer({.gaveUp = true}); });
}

class JdwpImpl::SupertypeWalk
{
public:
    QList<quint64> order;
    QList<quint64> pending;
    QHash<quint64, QList<quint64>> direct;
};

// Each type ahead of all it inherits from, and otherwise in the order of the
// walk, which keeps the classes ahead of the interfaces.
static QList<quint64> subtypesFirst(QList<quint64> left,
                                    const QHash<quint64, QList<quint64>> &direct)
{
    QHash<quint64, int> subtypes;
    for (const QList<quint64> &supertypes : direct) {
        for (const quint64 supertype : supertypes)
            ++subtypes[supertype];
    }
    QList<quint64> sorted;
    while (!left.isEmpty()) {
        const auto next = std::find_if(left.begin(), left.end(), [&subtypes](quint64 type) {
            return subtypes.value(type) == 0;
        });
        QTC_ASSERT(next != left.end(), return sorted + left);
        const quint64 type = *next;
        left.erase(next);
        sorted.append(type);
        for (const quint64 supertype : direct.value(type))
            --subtypes[supertype];
    }
    return sorted;
}

// What a loaded class inherits from cannot change, so the walk is made once.
void JdwpImpl::withSupertypes(quint64 typeId, const Types &done)
{
    fetchOnce(QString("supertypes:%1").arg(typeId),
              [this, typeId] { return m_classes.value(typeId).supertypesKnown; },
              [this, typeId](const Done &finish) {
        const auto walk = std::make_shared<SupertypeWalk>();
        walk->pending.append(typeId);
        expandSupertypes(walk, [this, typeId, walk, finish](const QList<quint64> &order) {
            ClassInfo &info = m_classes[typeId];
            info.supertypes = subtypesFirst(order, walk->direct);
            info.supertypesKnown = true;
            finish();
        });
    }, [this, typeId, done] { done(m_classes.value(typeId).supertypes); });
}

void JdwpImpl::expandSupertypes(const std::shared_ptr<SupertypeWalk> &walk, const Types &done)
{
    while (!walk->pending.isEmpty()
           && (walk->pending.first() == 0 || walk->order.contains(walk->pending.first()))) {
        walk->pending.removeFirst();
    }
    if (walk->pending.isEmpty()) {
        done(walk->order);
        return;
    }
    const quint64 typeId = walk->pending.takeFirst();
    walk->order.append(typeId);
    // An interface has no superclass, and the question is refused. The
    // superclass goes in front of the interfaces still waiting, so that every
    // class is met before any interface.
    send(Jdwp::ClassTypeSet, Jdwp::ClassTypeSuperclass,
         writer().writeReferenceTypeId(typeId).data(),
         [this, walk, typeId, done](const JdwpReply &reply) {
        if (reply.ok()) {
            const quint64 superclass = reader(reply.data).readReferenceTypeId();
            if (superclass != 0) {
                m_classes[superclass].tag = Jdwp::ClassTag;
                walk->pending.prepend(superclass);
                walk->direct[typeId].append(superclass);
            }
        }
        send(Jdwp::ReferenceTypeSet, Jdwp::RefTypeInterfaces,
             writer().writeReferenceTypeId(typeId).data(),
             [this, walk, typeId, done](const JdwpReply &reply) {
            if (reply.ok()) {
                JdwpReader r = reader(reply.data);
                const qint32 count = r.readInt();
                for (qint32 i = 0; i < count && r.ok(); ++i) {
                    const quint64 implemented = r.readReferenceTypeId();
                    m_classes[implemented].tag = Jdwp::InterfaceTag;
                    walk->pending.append(implemented);
                    walk->direct[typeId].append(implemented);
                }
            }
            expandSupertypes(walk, done);
        });
    });
}

void JdwpImpl::withSignatures(const QList<quint64> &typeIds, qsizetype index, const Done &done)
{
    if (index >= typeIds.size()) {
        done();
        return;
    }
    withSignature(typeIds.at(index), [this, typeIds, index, done] {
        withSignatures(typeIds, index + 1, done);
    });
}

void JdwpImpl::withClassOf(quint64 object, const std::function<void(quint64)> &done)
{
    send(Jdwp::ObjectReferenceSet, Jdwp::ObjectReferenceType,
         writer().writeObjectId(object).data(), [this, done](const JdwpReply &reply) {
        JdwpReader r = reader(reply.data);
        const quint8 tag = r.readByte();
        const quint64 typeId = r.readReferenceTypeId();
        if (!reply.ok() || !r.ok() || typeId == 0) {
            done(0);
            return;
        }
        m_classes[typeId].tag = tag;
        withSignature(typeId, [done, typeId] { done(typeId); });
    });
}

// What each class is a subtype of. One the machine has not loaded has no
// object of its own yet, and is taken for a subtype of Object alone.
void JdwpImpl::withSupertypeSignatures(const QStringList &signatures, qsizetype index,
                                       const std::shared_ptr<Subtyping> &subtyping,
                                       const Done &done)
{
    if (index >= signatures.size()) {
        done();
        return;
    }
    const QString signature = signatures.at(index);
    const auto next = [this, signatures, index, subtyping, done] {
        withSupertypeSignatures(signatures, index + 1, subtyping, done);
    };
    if (!signature.startsWith('L') || subtyping->contains(signature)) {
        next();
        return;
    }
    subtyping->insert(signature, {signature, "Ljava/lang/Object;"});
    classOfSignature(signature, [this, signature, subtyping, next](quint64 classId) {
        if (classId == 0) {
            next();
            return;
        }
        withSupertypes(classId, [this, signature, subtyping, next](const QList<quint64> &types) {
            withSignatures(types, 0, [this, signature, subtyping, types, next] {
                QSet<QString> &supertypes = (*subtyping)[signature];
                for (const quint64 type : types)
                    supertypes.insert(m_classes.value(type).signature);
                next();
            });
        });
    });
}

void JdwpImpl::withArgumentSignatures(const QList<Evaluated> &arguments, qsizetype index,
                                      const std::shared_ptr<QStringList> &signatures,
                                      const Done &done)
{
    if (index >= arguments.size()) {
        done();
        return;
    }
    const Evaluated &argument = arguments.at(index);
    const auto next = [this, arguments, index, signatures, done](const QString &signature) {
        signatures->append(signature);
        withArgumentSignatures(arguments, index + 1, signatures, done);
    };
    if (!argument.value.isObject()) {
        next(signatureOf(argument.value));
        return;
    }
    if (!argument.signature.isEmpty() || argument.value.isNull()) {
        next(argument.signature);
        return;
    }
    withClassOf(argument.value.bits, [this, next](quint64 typeId) {
        next(typeId ? m_classes.value(typeId).signature : QString("Ljava/lang/Object;"));
    });
}

void JdwpImpl::gatherCallables(const QList<quint64> &types, qsizetype index, const QString &name,
                               const std::shared_ptr<QList<Callable>> &found,
                               const std::function<void(const QList<Callable> &)> &done)
{
    if (index >= types.size()) {
        done(*found);
        return;
    }
    const quint64 classId = types.at(index);
    withMethods(classId, [this, types, index, classId, name, found, done] {
        const QList<MethodInfo> methods = m_classes.value(classId).methods;
        const bool ofAnInterface = m_classes.value(classId).tag == Jdwp::InterfaceTag;
        for (const MethodInfo &method : methods) {
            if (method.name != name)
                continue;
            const quint32 modifiers = quint32(method.modBits);
            // None of these is a method the source declared.
            if (modifiers & (Jdwp::BridgeModifierBit | Jdwp::SyntheticModifierBit
                             | Jdwp::JdwpSyntheticModifierBits)) {
                continue;
            }
            const bool isStatic = modifiers & Jdwp::StaticModifierBit;
            // Neither a private method nor a static one of an interface is
            // inherited (JLS 8.4.8, 9.4.1).
            const bool inherited = !(modifiers & Jdwp::PrivateModifierBit)
                                   && !(isStatic && ofAnInterface);
            if (index > 0 && !inherited)
                continue;
            Callable callable;
            callable.declaringClass = classId;
            callable.methodId = method.id;
            callable.parameters = parameterSignatures(method.signature);
            callable.returns = returnSignature(method.signature);
            callable.returnsTypeVariable = isTypeVariable(returnSignature(method.genericSignature));
            callable.isStatic = isStatic;
            callable.isVarArgs = modifiers & Jdwp::VarArgsModifierBit;
            // A declaration overrides or hides what it inherits with the same
            // parameters, and comes first.
            const bool hidden = Utils::anyOf(*found, [&callable](const Callable &known) {
                return known.parameters == callable.parameters;
            });
            if (!hidden)
                found->append(callable);
        }
        gatherCallables(types, index + 1, name, found, done);
    });
}

void JdwpImpl::withCallables(quint64 classId, const QString &name,
                             const std::function<void(const QList<Callable> &)> &done)
{
    withSupertypes(classId, [this, classId, name, done](const QList<quint64> &types) {
        const auto gather = [this, name, done](const QList<quint64> &searched) {
            gatherCallables(searched, 0, name, std::make_shared<QList<Callable>>(), done);
        };
        // An interface has the methods of Object as well (JLS 9.2).
        if (m_classes.value(classId).tag != Jdwp::InterfaceTag) {
            gather(types);
            return;
        }
        withObjectClass([types, gather](quint64 objectClass) {
            gather(objectClass ? types + QList<quint64>{objectClass} : types);
        });
    });
}

void JdwpImpl::evaluateEach(const Arguments &expressions, qsizetype index,
                            const std::shared_ptr<QList<Evaluated>> &values,
                            const std::function<void(const QString &error)> &done)
{
    if (index >= expressions.size()) {
        done({});
        return;
    }
    const std::shared_ptr<JdwpExpression> expression = expressions.at(index);
    if (!expression) {
        done(Tr::tr("An argument is missing."));
        return;
    }
    evaluate(*expression, [this, expressions, index, values, done](const Evaluated &value) {
        if (!value.error.isEmpty()) {
            done(value.error);
            return;
        }
        if (value.value.tag == Jdwp::VoidValueTag) {
            done(Tr::tr("A method that returns nothing cannot be an argument."));
            return;
        }
        values->append(value);
        evaluateEach(expressions, index + 1, values, done);
    });
}

void JdwpImpl::converted(const Stop &stop, const Evaluated &argument, const QString &parameter,
                         const std::function<void(const JdwpValue &, const QString &)> &done)
{
    const JdwpValue value = argument.value;
    if (isPrimitiveSignature(parameter)) {
        if (Jdwp::isPrimitiveTag(value.tag)) {
            done(widenedTo(value, parameter.at(0)), {});
            return;
        }
        if (value.isNull()) {
            done({}, Tr::tr("The value is null."));
            return;
        }
        // Unboxed by what the wrapper holds, which needs no call.
        withClassOf(value.bits, [this, value, parameter, done](quint64 typeId) {
            withFields(typeId, [this, value, parameter, done, typeId] {
                const QList<FieldInfo> fields = m_classes.value(typeId).fields;
                const auto held = std::find_if(fields.cbegin(), fields.cend(),
                                               [](const FieldInfo &field) {
                    return field.name == "value";
                });
                if (held == fields.cend()) {
                    done({}, Tr::tr("What the wrapper holds could not be read."));
                    return;
                }
                send(Jdwp::ObjectReferenceSet, Jdwp::ObjectGetValues,
                     writer().writeObjectId(value.bits).writeInt(1).writeFieldId(held->id).data(),
                     [this, parameter, done](const JdwpReply &reply) {
                    if (!reply.ok()) {
                        done({}, JdwpClient::errorString(reply.errorCode));
                        return;
                    }
                    JdwpReader r = reader(reply.data);
                    if (r.readInt() < 1) {
                        done({}, Tr::tr("The virtual machine returned no value."));
                        return;
                    }
                    done(widenedTo(r.readTaggedValue(), parameter.at(0)), {});
                });
            });
        });
        return;
    }
    if (!Jdwp::isPrimitiveTag(value.tag)) {
        done(value, {});
        return;
    }
    // Boxed the way Java boxes, by valueOf() of the wrapper.
    const QChar primitive = QChar::fromLatin1(char(value.tag));
    const QString wrapper = boxed(primitive);
    classOfSignature(wrapper, [this, stop, value, primitive, wrapper, done](quint64 classId) {
        withMethods(classId, [this, stop, value, primitive, wrapper, done, classId] {
            const QString signature = QString("(%1)%2").arg(primitive, wrapper);
            const QList<MethodInfo> methods = m_classes.value(classId).methods;
            const auto valueOf = std::find_if(methods.cbegin(), methods.cend(),
                                              [&signature](const MethodInfo &method) {
                return method.name == "valueOf" && method.signature == signature;
            });
            if (valueOf == methods.cend()) {
                done({}, Tr::tr("%1 cannot be boxed here.").arg(typeName(primitive)));
                return;
            }
            invoke(stop, 0, classId, valueOf->id, Dispatch::Static, {value},
                   [this, stop, primitive, done](const Invoked &result) {
                if (!result.error.isEmpty() || result.gaveUp || !result.thrown.isNull()) {
                    done({}, result.error.isEmpty()
                                 ? Tr::tr("%1 could not be boxed.").arg(typeName(primitive))
                                 : result.error);
                    return;
                }
                pin(result.value, stop);
                done(result.value, {});
            });
        });
    });
}

void JdwpImpl::convertedEach(const Stop &stop, const QList<Evaluated> &arguments,
                             const QStringList &parameters, qsizetype index,
                             const std::shared_ptr<QList<JdwpValue>> &values,
                             const std::function<void(const QString &)> &done)
{
    if (index >= arguments.size()) {
        done({});
        return;
    }
    converted(stop, arguments.at(index), parameters.at(index),
              [this, stop, arguments, parameters, index, values, done](const JdwpValue &value,
                                                                       const QString &error) {
        if (!error.isEmpty()) {
            done(error);
            return;
        }
        values->append(value);
        convertedEach(stop, arguments, parameters, index + 1, values, done);
    });
}

void JdwpImpl::newArray(const Stop &stop, const QString &signature,
                        const QList<JdwpValue> &elements,
                        const std::function<void(const JdwpValue &, const QString &)> &done)
{
    // The machine makes an array only of a class it has loaded, which it does
    // when the program first makes one of the kind.
    classOfSignature(signature, [this, stop, signature, elements, done](quint64 arrayType) {
        if (arrayType == 0) {
            done({}, Tr::tr("The program has no %1 yet to pass the arguments in.")
                         .arg(typeName(signature)));
            return;
        }
        send(Jdwp::ArrayTypeSet, Jdwp::ArrayTypeNewInstance,
             writer().writeReferenceTypeId(arrayType).writeInt(qint32(elements.size())).data(),
             [this, stop, elements, done](const JdwpReply &reply) {
            if (!reply.ok()) {
                done({}, JdwpClient::errorString(reply.errorCode));
                return;
            }
            JdwpReader r = reader(reply.data);
            const JdwpValue array = r.readTaggedValue();
            if (!r.ok() || array.isNull()) {
                done({}, Tr::tr("The virtual machine returned no value."));
                return;
            }
            pin(array, stop);
            if (elements.isEmpty()) {
                done(array, {});
                return;
            }
            JdwpWriter request = writer();
            request.writeObjectId(array.bits).writeInt(0).writeInt(qint32(elements.size()));
            for (const JdwpValue &element : elements)
                request.writeUntaggedValue(element);
            send(Jdwp::ArrayReferenceSet, Jdwp::ArraySetValues, request.data(),
                 [array, done](const JdwpReply &reply) {
                if (!reply.ok()) {
                    done({}, JdwpClient::errorString(reply.errorCode));
                    return;
                }
                done(array, {});
            });
        });
    });
}

void JdwpImpl::callOn(const Stop &stop, const Receiver &receiver, const QString &name,
                      const QList<Evaluated> &arguments, const Evaluation &done)
{
    if (const QString why = whyNoCallAt(stop); !why.isEmpty()) {
        done({.error = why});
        return;
    }
    const auto call = [this, stop, receiver, name, done](const Callable &chosen,
                                                         const QList<JdwpValue> &values) {
        invoke(stop, receiver.object, chosen.declaringClass, chosen.methodId,
               chosen.isStatic ? Dispatch::Static : Dispatch::OnObject, values,
               [this, stop, name, chosen, done](const Invoked &result) {
            if (!result.error.isEmpty()) {
                done({.error = result.error});
                return;
            }
            if (result.gaveUp) {
                done({.error = Tr::tr("\"%1\" did not come back.").arg(name)});
                return;
            }
            if (!result.thrown.isNull()) {
                // By its class: reading the message it carries would be
                // another call, and the first one has the thread.
                classNameOf(result.thrown, [done, name](const QString &className) {
                    done({.error = Tr::tr("\"%1\" threw %2.").arg(name, className)});
                });
                return;
            }
            pin(result.value, stop);
            done({.value = result.value, .type = typeName(chosen.returns),
                  .signature = chosen.returnsTypeVariable ? QString() : chosen.returns});
        });
    };

    const auto choose = [this, stop, receiver, name, arguments, done, call]
                        (const QList<Callable> &candidates, const QStringList &signatures,
                         const Subtyping &subtyping) {
        const qsizetype count = arguments.size();
        const Callable *chosen = nullptr;
        bool byArity = false;
        for (const Phase phase : {Phase::Strict, Phase::Loose, Phase::VariableArity}) {
            QList<const Callable *> applicable;
            QList<QStringList> parameters;
            for (const Callable &candidate : candidates) {
                if (isApplicable(candidate.parameters, candidate.isVarArgs, signatures, phase,
                                 subtyping)) {
                    applicable.append(&candidate);
                    parameters.append(candidate.parameters);
                }
            }
            if (applicable.isEmpty())
                continue;
            byArity = phase == Phase::VariableArity;
            const qsizetype best = mostSpecific(parameters, byArity, count, subtyping);
            if (best < 0) {
                done({.error = Tr::tr("The call to \"%1\" is ambiguous.").arg(name)});
                return;
            }
            chosen = applicable.at(best);
            break;
        }
        if (!chosen) {
            const bool arityFits = Utils::anyOf(candidates, [count](const Callable &candidate) {
                return candidate.parameters.size() == count
                       || (candidate.isVarArgs && count >= candidate.parameters.size() - 1);
            });
            done({.error = arityFits
                               ? Tr::tr("No method \"%1\" here takes what was written for it.")
                                     .arg(name)
                               : Tr::tr("There is no method \"%1\" here that takes %n "
                                        "argument(s).", nullptr, int(count)).arg(name)});
            return;
        }
        // Chosen first and refused afterwards, the way JLS 15.12.3 has it.
        if (!chosen->isStatic && receiver.object == 0) {
            done({.error = receiver.isNull
                               ? Tr::tr("The value is null.")
                               : Tr::tr("\"%1\" is not static, and there is no object here to "
                                        "call it on.").arg(name)});
            return;
        }
        QList<Evaluated> typed = arguments;
        for (qsizetype i = 0; i < count; ++i)
            typed[i].signature = signatures.at(i);
        const QStringList parameters = parameterTypesFor(chosen->parameters, byArity, count);
        const auto values = std::make_shared<QList<JdwpValue>>();
        convertedEach(stop, typed, parameters, 0, values,
                      [this, stop, chosen = *chosen, byArity, values, done, call]
                      (const QString &error) {
            if (!error.isEmpty()) {
                done({.error = error});
                return;
            }
            if (!byArity) {
                call(chosen, *values);
                return;
            }
            // The trailing arguments go in an array of the last parameter.
            const qsizetype fixed = chosen.parameters.size() - 1;
            newArray(stop, chosen.parameters.last(), values->mid(fixed),
                     [chosen, values, fixed, done, call](const JdwpValue &array,
                                                         const QString &error) {
                if (!error.isEmpty()) {
                    done({.error = error});
                    return;
                }
                call(chosen, values->first(fixed) << array);
            });
        });
    };

    withCallables(receiver.searchType, name,
                  [this, name, arguments, done, choose](const QList<Callable> &candidates) {
        if (candidates.isEmpty()) {
            done({.error = Tr::tr("There is no method \"%1\" here.").arg(name)});
            return;
        }
        const auto signatures = std::make_shared<QStringList>();
        withArgumentSignatures(arguments, 0, signatures,
                               [this, candidates, signatures, choose] {
            // Everything the choice compares, without the arrays around it.
            QStringList types;
            for (const QString &signature : std::as_const(*signatures)) {
                types.append(signature);
                if (isPrimitiveSignature(signature))
                    types.append(boxed(signature.at(0)));
            }
            for (const Callable &candidate : candidates)
                types += candidate.parameters;
            for (QString &type : types) {
                while (type.startsWith('['))
                    type.remove(0, 1);
            }
            types.removeDuplicates();
            const auto subtyping = std::make_shared<Subtyping>();
            withSupertypeSignatures(types, 0, subtyping,
                                    [candidates, signatures, subtyping, choose] {
                choose(candidates, *signatures, *subtyping);
            });
        });
    });
}

void JdwpImpl::enclosingReceiver(quint64 classId, quint64 object, const QString &name,
                                 const std::function<void(const Receiver &)> &done)
{
    withCallables(classId, name,
                  [this, classId, object, name, done](const QList<Callable> &found) {
        withSignature(classId, [this, classId, object, name, done, found] {
            const QString signature = m_classes.value(classId).signature;
            const qsizetype cut = signature.lastIndexOf('$');
            if (!found.isEmpty() || cut < 0) {
                done({.searchType = classId, .object = object});
                return;
            }
            const QString outer = signature.left(cut) + ';';
            classOfSignature(outer, [this, classId, object, name, done, outer](quint64 outerId) {
                if (outerId == 0) {
                    done({.searchType = classId, .object = object});
                    return;
                }
                const auto onwards = [this, outerId, name, done](quint64 outerObject) {
                    enclosingReceiver(outerId, outerObject, name, done);
                };
                if (object == 0) {
                    onwards(0);
                    return;
                }
                // An inner class keeps the object it was made in.
                withFields(classId, [this, classId, object, outer, onwards] {
                    const QList<FieldInfo> fields = m_classes.value(classId).fields;
                    const auto it = std::find_if(fields.cbegin(), fields.cend(),
                                                 [&outer](const FieldInfo &field) {
                        return field.name.startsWith("this$") && field.signature == outer;
                    });
                    if (it == fields.cend()) {
                        onwards(0);
                        return;
                    }
                    send(Jdwp::ObjectReferenceSet, Jdwp::ObjectGetValues,
                         writer().writeObjectId(object).writeInt(1).writeFieldId(it->id).data(),
                         [this, onwards](const JdwpReply &reply) {
                        JdwpReader r = reader(reply.data);
                        if (!reply.ok() || r.readInt() < 1) {
                            onwards(0);
                            return;
                        }
                        const JdwpValue enclosing = r.readTaggedValue();
                        onwards(enclosing.isNull() ? 0 : enclosing.bits);
                    });
                });
            });
        });
    });
}

void JdwpImpl::callOf(const JdwpExpression &expression, const Evaluation &done)
{
    const Stop stop = currentStop();
    if (const QString why = whyNoCallAt(stop); !why.isEmpty()) {
        done({.error = why});
        return;
    }
    const QString name = expression.text;
    const Arguments arguments = expression.arguments;
    const auto withReceiver = [this, stop, name, arguments, done](const Receiver &receiver) {
        const auto values = std::make_shared<QList<Evaluated>>();
        evaluateEach(arguments, 0, values,
                     [this, stop, name, receiver, values, done](const QString &error) {
            if (!error.isEmpty()) {
                done({.error = error});
                return;
            }
            callOn(stop, receiver, name, *values, done);
        });
    };

    if (!expression.base) {
        // A call with nothing in front of it is one of the class the frame is
        // in, or of a class around it (JLS 15.12.1).
        withCurrentFrame([this, name, withReceiver, done](const Frame *frame) {
            if (!frame) {
                done({.error = Tr::tr("There is no frame to look in.")});
                return;
            }
            const quint64 frameClass = frame->location.classId;
            // The call goes out as an interface's or a class's by this tag.
            m_classes[frameClass].tag = frame->location.typeTag;
            evaluateThis([this, frameClass, name, withReceiver](const Evaluated &self) {
                enclosingReceiver(frameClass, self.error.isEmpty() ? self.value.bits : 0, name,
                                  withReceiver);
            });
        });
        return;
    }

    const JdwpExpression base = *expression.base;
    evaluate(base, [this, base, name, withReceiver, done](const Evaluated &target) {
        if (target.error.isEmpty()) {
            if (!target.value.isObject()) {
                done({.error = Tr::tr("\"%1\" has no methods.")
                                   .arg(primitiveText(target.value))});
                return;
            }
            const bool isNull = target.value.isNull();
            if (isNull && target.signature.isEmpty()) {
                done({.error = Tr::tr("The value is null.")});
                return;
            }
            const quint64 object = isNull ? 0 : target.value.bits;
            const auto searchIn = [withReceiver, name, object, isNull, done](quint64 searchType) {
                if (searchType == 0) {
                    done({.error = Tr::tr("The class to look up \"%1\" in is not known.")
                                       .arg(name)});
                    return;
                }
                withReceiver({.searchType = searchType, .object = object, .isNull = isNull});
            };
            // The methods of an array are those of Object.
            const QString signature = target.signature;
            if (signature.startsWith('[')
                || (signature.isEmpty() && target.value.tag == Jdwp::ArrayValueTag)) {
                withObjectClass(searchIn);
                return;
            }
            // The method is chosen by the type the expression is declared
            // with, and the machine's dispatch picks the override.
            if (signature.isEmpty()) {
                withClassOf(object, searchIn);
                return;
            }
            classOfSignature(signature, [this, object, searchIn](quint64 classId) {
                if (classId != 0 || object == 0)
                    searchIn(classId);
                else
                    withClassOf(object, searchIn);
            });
            return;
        }
        // Nothing goes by that name here, but a class may, and then this is a
        // static method of it.
        const QString dotted = base.dottedName();
        if (dotted.isEmpty()) {
            done(target);
            return;
        }
        classNamed(dotted, [target, withReceiver, done](quint64 classId) {
            if (classId == 0) {
                done(target);
                return;
            }
            withReceiver({.searchType = classId});
        });
    });
}

void JdwpImpl::evaluate(const JdwpExpression &expression, const Evaluation &done)
{
    const auto literal = [&done, &expression](quint8 tag, quint64 bits) {
        const JdwpValue value{tag, bits};
        done({.value = value, .type = expression.typeName(), .signature = signatureOf(value)});
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
             [this, done, stop = currentStop()](const JdwpReply &reply) {
            if (!reply.ok()) {
                done({.error = JdwpClient::errorString(reply.errorCode)});
                return;
            }
            const JdwpValue string = {Jdwp::StringValueTag, reader(reply.data).readObjectId()};
            pin(string, stop);
            done({.value = string, .type = "java.lang.String",
                  .signature = "Ljava/lang/String;"});
        });
        return;
    case JdwpExpression::Kind::This:
        evaluateThis(done);
        return;
    case JdwpExpression::Kind::Name:
        evaluateName(expression.text, done);
        return;
    case JdwpExpression::Kind::Call:
        callOf(expression, done);
        return;
    case JdwpExpression::Kind::Field: {
        const QString name = expression.text;
        const JdwpExpression base = *expression.base;
        evaluate(base, [this, name, base, done](const Evaluated &value) {
            if (value.error.isEmpty()) {
                fieldOf(value, name, done);
                return;
            }
            // Nothing goes by that name here; a class may, and then this is a
            // static field of it.
            const QString dotted = base.dottedName();
            if (dotted.isEmpty()) {
                done(value);
                return;
            }
            classNamed(dotted, [this, name, dotted, value, done](quint64 classId) {
                if (classId == 0) {
                    done(value);
                    return;
                }
                staticFieldOf(classId, dotted, name, done);
            });
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
    case JdwpExpression::Kind::Unary: {
        const JdwpExpression::Operator op = expression.op;
        evaluate(*expression.base, [op, done](const Evaluated &operand) {
            if (!operand.error.isEmpty()) {
                done(operand);
                return;
            }
            const Result<JdwpValue> result = negatedOrFlipped(op, operand.value);
            if (!result) {
                done({.error = result.error()});
                return;
            }
            done({.value = *result, .type = typeOf(*result), .signature = signatureOf(*result)});
        });
        return;
    }
    case JdwpExpression::Kind::Binary: {
        const JdwpExpression::Operator op = expression.op;
        const JdwpExpression right = *expression.right;
        if (op == JdwpExpression::Operator::Equal || op == JdwpExpression::Operator::NotEqual) {
            // A literal is interned, which makes it the very string it spells in
            // the program. The one made in the machine to stand for it is not,
            // so a literal is compared by what it spells instead.
            const bool leftLiteral = expression.base->kind == JdwpExpression::Kind::String;
            if (leftLiteral || right.kind == JdwpExpression::Kind::String) {
                compareWithLiteral(leftLiteral ? expression.base->text : right.text,
                                   leftLiteral ? right : *expression.base,
                                   op == JdwpExpression::Operator::Equal, done);
                return;
            }
        }
        evaluate(*expression.base, [this, op, right, done](const Evaluated &left) {
            if (!left.error.isEmpty()) {
                done(left);
                return;
            }
            // Java decides "&&" and "||" on the left alone where it can, and
            // then the right side is never even read.
            const bool logical = op == JdwpExpression::Operator::And
                                 || op == JdwpExpression::Operator::Or;
            if (logical) {
                if (left.value.tag != Jdwp::BooleanValueTag) {
                    done({.error = Tr::tr("\"%1\" needs a boolean on both sides.")
                                       .arg(op == JdwpExpression::Operator::And ? "&&" : "||")});
                    return;
                }
                const bool yes = left.value.bits != 0;
                if (yes == (op == JdwpExpression::Operator::Or)) {
                    done({.value = madeOf(yes), .type = "boolean", .signature = "Z"});
                    return;
                }
            }
            evaluate(right, [op, left, logical, done](const Evaluated &other) {
                if (!other.error.isEmpty()) {
                    done(other);
                    return;
                }
                if (logical) {
                    if (other.value.tag != Jdwp::BooleanValueTag) {
                        done({.error = Tr::tr("\"%1\" needs a boolean on both sides.")
                                           .arg(op == JdwpExpression::Operator::And
                                                    ? "&&" : "||")});
                        return;
                    }
                    done({.value = madeOf(other.value.bits != 0), .type = "boolean",
                          .signature = "Z"});
                    return;
                }
                const Result<JdwpValue> result = combined(op, left.value, other.value);
                if (!result) {
                    done({.error = result.error()});
                    return;
                }
                done({.value = *result, .type = typeOf(*result),
                      .signature = signatureOf(*result)});
            });
        });
        return;
    }
    }
}

void JdwpImpl::compareWithLiteral(const QString &literal, const JdwpExpression &other,
                                  bool equal, const Evaluation &done)
{
    if (other.kind == JdwpExpression::Kind::String) {
        done({.value = madeOf((other.text == literal) == equal), .type = "boolean",
              .signature = "Z"});
        return;
    }
    evaluate(other, [this, literal, equal, done](const Evaluated &result) {
        if (!result.error.isEmpty()) {
            done(result);
            return;
        }
        if (!result.value.isObject()) {
            done({.error = Tr::tr("An object and a number cannot be compared.")});
            return;
        }
        if (result.value.isNull() || result.value.tag != Jdwp::StringValueTag) {
            done({.value = madeOf(!equal), .type = "boolean", .signature = "Z"});
            return;
        }
        send(Jdwp::StringReferenceSet, Jdwp::StringValue,
             writer().writeObjectId(result.value.bits).data(),
             [this, literal, equal, done](const JdwpReply &reply) {
            if (!reply.ok()) {
                done({.error = JdwpClient::errorString(reply.errorCode)});
                return;
            }
            const bool same = reader(reply.data).readString() == literal;
            done({.value = madeOf(same == equal), .type = "boolean", .signature = "Z"});
        });
    });
}

void JdwpImpl::evaluateThis(const Evaluation &done)
{
    withCurrentFrame([this, done](const Frame *frame) {
        if (!frame) {
            done({.error = Tr::tr("There is no frame to look in.")});
            return;
        }
        const quint64 classId = frame->location.classId;
        send(Jdwp::StackFrameSet, Jdwp::StackFrameThisObject,
             writer().writeObjectId(m_currentThread).writeFrameId(frame->id).data(),
             [this, done, classId](const JdwpReply &reply) {
            if (!reply.ok()) {
                done({.error = JdwpClient::errorString(reply.errorCode)});
                return;
            }
            const JdwpValue self = reader(reply.data).readTaggedValue();
            if (!self.isObject() || self.isNull()) {
                done({.error = Tr::tr("The frame is of a static method, which has no \"this\".")});
                return;
            }
            // "this" is declared as the class the frame is in.
            withSignature(classId, [this, done, classId, self] {
                const QString signature = m_classes.value(classId).signature;
                done({.value = self, .type = typeName(signature), .signature = signature});
            });
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
                done({.value = r.readTaggedValue(), .type = typeName(variable.signature),
                      .signature = staticSignature(variable.signature,
                                                   variable.genericSignature)});
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
            done({.value = r.readTaggedValue(), .type = typeName(field.signature),
                  .signature = staticSignature(field.signature, field.genericSignature)});
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
                  .type = "int", .signature = "I"});
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
                    done({.value = r.readTaggedValue(), .type = typeName(field.signature),
                          .signature = staticSignature(field.signature,
                                                       field.genericSignature)});
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
         [this, object, position, done, arrayType = base.signature](const JdwpReply &reply) {
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
             [this, done, arrayType](const JdwpReply &reply) {
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
            const QString elementType = primitive ? QString(QChar::fromLatin1(char(elementTag)))
                                        : arrayType.startsWith('[') ? arrayType.mid(1)
                                                                    : QString();
            done({.value = element, .type = primitive ? typeName(elementType) : QString(),
                  .signature = elementType});
        });
    });
}

void JdwpImpl::fetchLocals(const RefreshRequest &request)
{
    const int generation = ++m_localsGeneration;
    m_lastLocalsRequest = request;
    m_localsRequestId = request.requestId;
    m_expandedINames = request.expandedINames;
    m_expandedItems = request.expandedForDumpers();
    m_locals.clear();
    m_localRoots.clear();
    m_toStringCalls.clear();
    m_deferredWatchers.clear();

    // The calls are made on the thread that is stopped, so the batch belongs
    // to this stop of it and to no other.
    const int stopGeneration = m_stopGeneration;
    const auto join = std::make_shared<Join>([this, generation, stop = currentStop()] {
        if (generation == m_localsGeneration)
            answerCalls(generation, stop);
    });
    join->add();
    if (m_running || m_currentThread == 0 || !m_client.isConnected()) {
        join->release();
        return;
    }
    addWatchers(join, request.watchers);
    addStaticFields(join);

    const int frameIndex = m_currentFrame;
    const quint64 thread = m_currentThread;
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
                mentionAbsentVariables(location);
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
        // One that calls cannot be worked out here: the call lets the thread
        // run, and the machine forgets the frames the rest of this reads off.
        // It waits until the reading is over.
        if (parsed->makesTheProgramRun()) {
            if (!m_lastLocalsRequest.allowInferiorCalls) {
                local.value = '<' + Tr::tr("calling would make the program run, which the views "
                                           "are not allowed to do") + '>';
                continue;
            }
            m_deferredWatchers.append({iname, expression, *parsed});
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

void JdwpImpl::mentionAbsentVariables(const JdwpLocation &location)
{
    const MethodInfo *info = method(location.classId, location.methodId);
    if (!info || !info->variablesAbsent)
        return;
    const QString key = QString("%1:%2").arg(location.classId).arg(location.methodId);
    if (!m_absentMentioned.contains(key)) {
        m_absentMentioned.insert(key);
        emit message(Tr::tr("%1.%2 was compiled without the table that names its local "
                            "variables, so there are none to show.")
                         .arg(typeName(m_classes.value(location.classId).signature), info->name),
                     LogWarning);
    }
}

void JdwpImpl::addStaticFields(const JoinPtr &join)
{
    const int generation = m_localsGeneration;
    join->add();
    withCurrentFrame([this, join, generation](const Frame *frame) {
        if (generation != m_localsGeneration)
            return;
        if (!frame) {
            join->release();
            return;
        }
        const quint64 classId = frame->location.classId;
        withSignature(classId, [this, join, generation, classId] {
            if (generation != m_localsGeneration)
                return;
            withFields(classId, [this, join, generation, classId] {
                if (generation != m_localsGeneration)
                    return;
                const QList<FieldInfo> fields = m_classes.value(classId).staticFields;
                if (fields.isEmpty()) {
                    join->release();
                    return;
                }
                const QString iname = s_staticIName;
                m_localRoots.append(iname);
                Local &local = m_locals[iname];
                local.name = "static";
                local.type = typeName(m_classes.value(classId).signature);
                local.value = Tr::tr("<%n items>", nullptr, int(fields.size()));
                local.hasChildren = true;
                if (!m_expandedINames.contains(iname)) {
                    join->release();
                    return;
                }
                JdwpWriter request = writer();
                request.writeReferenceTypeId(classId).writeInt(qint32(fields.size()));
                QList<quint64> declaring;
                for (const FieldInfo &field : fields) {
                    request.writeFieldId(field.id);
                    if (!declaring.contains(field.declaringClass))
                        declaring.append(field.declaringClass);
                }
                // Each is named after its declaring class, since the name alone
                // would be taken for a local of the same name.
                withSignatures(declaring, 0, [this, join, generation, iname, fields,
                                              request = request.data()] {
                    if (generation != m_localsGeneration)
                        return;
                    send(Jdwp::ReferenceTypeSet, Jdwp::RefTypeGetValues, request,
                         [this, join, generation, iname, fields](const JdwpReply &reply) {
                        if (generation != m_localsGeneration)
                            return;
                        if (reply.ok()) {
                            JdwpReader r = reader(reply.data);
                            const qint32 count = r.readInt();
                            for (qint32 i = 0; i < count && i < fields.size() && r.ok(); ++i) {
                                const JdwpValue value = r.readTaggedValue();
                                const FieldInfo &field = fields.at(i);
                                // A static a superclass declares is shown under
                                // its plain name here, and one of them can hide
                                // another exactly as an instance field does.
                                QString child = iname + '.' + field.name;
                                if (m_locals.contains(child))
                                    child += '#' + QString::number(i);
                                m_locals[iname].children.append(child);
                                m_locals[child].exp
                                    = typeName(m_classes.value(field.declaringClass).signature)
                                      + '.' + field.name;
                                addValue(join, child, field.name, typeName(field.signature),
                                         value);
                            }
                        }
                        join->release();
                    });
                });
            });
        });
    });
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
                                m_locals[child].exp
                                    = QString("(%1)[%2]").arg(expressionOf(iname)).arg(i);
                                addValue(join, child, QString("[%1]").arg(i), elementType, element);
                            }
                        }
                        join->release();
                    });
                });
                return;
            }

            m_locals[iname].hasChildren = true;
            if (m_lastLocalsRequest.allowInferiorCalls)
                showAsToString(join, iname, object, typeId);
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
                            m_locals[child].exp
                                = QString("(%1).%2").arg(expressionOf(iname), field.name);
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

// What an assignment to the row is given, which for a child has to say
// whose child it is.
QString JdwpImpl::expressionOf(const QString &iname) const
{
    const Local local = m_locals.value(iname);
    return local.exp.isEmpty() ? local.name : local.exp;
}

// Notes that the object is one to show by what its own toString() answers.
// The call is not made here: it lets the thread run, and everything read off a
// frame has to be in before that happens, so the calls all come afterwards.
void JdwpImpl::showAsToString(const JoinPtr &join, const QString &iname, quint64 object,
                              quint64 classId)
{
    const int generation = m_localsGeneration;
    join->add();
    withToString(classId, [this, join, generation, iname, object, classId] {
        if (generation != m_localsGeneration)
            return;
        const ClassInfo info = m_classes.value(classId);
        if (info.toStringMethod != 0)
            m_toStringCalls.append({iname, object, info.toStringClass, info.toStringMethod});
        join->release();
    });
}

// Calls what was gathered, one call at a time: a call runs the thread it is
// made on, and the virtual machine refuses the next one for as long as that
// thread has not come back and been suspended again. An object whose call
// answers nothing keeps the identity it reads as, which is what
// java.lang.Object would have said anyway.
void JdwpImpl::answerCalls(int generation, const Stop &stop)
{
    if (generation != m_localsGeneration)
        return;
    // Let go of again while the values were being read, or another thread
    // selected, which leaves the thread the batch was gathered on where the
    // calls cannot be made, or a call given up on is still out on it. What is
    // there is what the views get.
    if (const QString why = whyNoCallAt(stop); !why.isEmpty()) {
        for (const DeferredWatcher &watcher : std::as_const(m_deferredWatchers))
            m_locals[watcher.iname].value = '<' + why + '>';
        m_deferredWatchers.clear();
        m_toStringCalls.clear();
        reportLocals();
        return;
    }
    // The watchers that call come first, since what they answer is shown by
    // its toString() as any other value is, and those calls join the batch.
    if (!m_deferredWatchers.isEmpty()) {
        const DeferredWatcher watcher = m_deferredWatchers.takeFirst();
        const auto join = std::make_shared<Join>([this, generation, stop] {
            answerCalls(generation, stop);
        });
        join->add();
        evaluate(watcher.parsed, [this, join, generation, watcher](const Evaluated &result) {
            if (generation != m_localsGeneration)
                return;
            if (result.error.isEmpty())
                addValue(join, watcher.iname, watcher.expression, result.type, result.value);
            else
                m_locals[watcher.iname].value = '<' + result.error + '>';
            join->release();
        });
        return;
    }
    if (m_toStringCalls.isEmpty()) {
        reportLocals();
        return;
    }
    const ToStringCall call = m_toStringCalls.takeFirst();
    invokeToString(stop, call.object, call.declaringClass, call.methodId,
                   [this, call, generation, stop](const std::optional<QString> &text,
                                                  bool gaveUp) {
        if (generation != m_localsGeneration)
            return;
        if (text)
            m_locals[call.iname].value = *text;
        // The thread is off running the call that did not come back, so there
        // is nothing left to make the calls behind it on either.
        if (gaveUp)
            m_toStringCalls.clear();
        answerCalls(generation, stop);
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
    if (!local.exp.isEmpty())
        item.addChild(constMi("exp", local.exp));
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
    // The statics come after the locals however the two were read, which is
    // in whatever order the machine answered about them.
    for (const QString &iname : std::as_const(m_localRoots)) {
        if (iname != s_staticIName)
            data.addChild(localsItem(iname));
    }
    if (m_localRoots.contains(s_staticIName))
        data.addChild(localsItem(s_staticIName));
    GdbMi all;
    all.m_type = GdbMi::Tuple;
    all.addChild(data);
    emit refreshDataReceived(m_localsRequestId, RefreshKind::Locals, all);
    m_reportedLocalsGeneration = m_localsGeneration;
    runPendingCommands();
}

// One at a time, since each may make calls on the thread.
void JdwpImpl::runPendingCommands()
{
    while (!m_pendingCommands.isEmpty() && m_reportedLocalsGeneration == m_localsGeneration
           && m_commandRunGeneration != m_runGeneration) {
        executeDebuggerCommand(m_pendingCommands.takeFirst(), {});
    }
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

// Where a name stands for a value that can be written: a local of the frame,
// a field of the object it runs on, or a static field of its class.
void JdwpImpl::placeOfName(const QString &name, const Placement &done)
{
    withCurrentFrame([this, name, done](const Frame *frame) {
        if (!frame) {
            done({.error = Tr::tr("There is no frame to look in.")});
            return;
        }
        const JdwpLocation location = frame->location;
        const quint64 frameId = frame->id;
        withVariables(location.classId, location.methodId, [this, name, done, location, frameId] {
            const QList<MethodInfo::Variable> visible = visibleVariables(location);
            const auto it = std::find_if(visible.cbegin(), visible.cend(),
                                         [&name](const MethodInfo::Variable &variable) {
                return variable.name == name;
            });
            if (it != visible.cend()) {
                done({.kind = Place::Kind::Local, .owner = frameId, .position = it->slot,
                      .signature = it->signature});
                return;
            }
            const quint64 classId = location.classId;
            evaluateThis([this, name, done, classId](const Evaluated &self) {
                if (self.error.isEmpty()) {
                    placeOfFieldIn(self.value.bits, name,
                                   [this, name, done, classId](const Place &place) {
                        if (place.error.isEmpty()) {
                            done(place);
                            return;
                        }
                        placeOfStaticField(classId, name, done);
                    });
                    return;
                }
                placeOfStaticField(classId, name, done);
            });
        });
    });
}

void JdwpImpl::placeOfStaticField(quint64 classId, const QString &name, const Placement &done)
{
    withFields(classId, [this, classId, name, done] {
        const QList<FieldInfo> fields = m_classes.value(classId).staticFields;
        const auto it = std::find_if(fields.cbegin(), fields.cend(),
                                     [&name](const FieldInfo &field) {
            return field.name == name;
        });
        if (it == fields.cend()) {
            done({.error = Tr::tr("There is nothing called \"%1\" here.").arg(name)});
            return;
        }
        done({.kind = Place::Kind::StaticField, .owner = classId, .fieldId = it->id,
              .signature = it->signature});
    });
}

void JdwpImpl::placeOfFieldIn(quint64 object, const QString &name, const Placement &done)
{
    send(Jdwp::ObjectReferenceSet, Jdwp::ObjectReferenceType, writer().writeObjectId(object).data(),
         [this, object, name, done](const JdwpReply &reply) {
        if (!reply.ok()) {
            done({.error = JdwpClient::errorString(reply.errorCode)});
            return;
        }
        JdwpReader r = reader(reply.data);
        r.readByte();
        const quint64 typeId = r.readReferenceTypeId();
        withFields(typeId, [this, object, name, done, typeId] {
            // The superclasses come first, and a field hides theirs of its name.
            const QList<FieldInfo> fields = m_classes.value(typeId).fields;
            const auto it = std::find_if(fields.crbegin(), fields.crend(),
                                         [&name](const FieldInfo &field) {
                return field.name == name;
            });
            if (it == fields.crend()) {
                placeOfStaticField(typeId, name, done);
                return;
            }
            done({.kind = Place::Kind::Field, .owner = object, .fieldId = it->id,
                  .signature = it->signature});
        });
    });
}

void JdwpImpl::placeOf(const JdwpExpression &expression, const Placement &done)
{
    switch (expression.kind) {
    case JdwpExpression::Kind::Name:
        placeOfName(expression.text, done);
        return;
    case JdwpExpression::Kind::Field: {
        const QString name = expression.text;
        const JdwpExpression baseExpression = *expression.base;
        evaluate(baseExpression, [this, name, baseExpression, done](const Evaluated &base) {
            if (!base.error.isEmpty()) {
                // Nothing goes by that name here; a class may, and then this
                // is a static field of it.
                const QString dotted = baseExpression.dottedName();
                if (dotted.isEmpty()) {
                    done({.error = base.error});
                    return;
                }
                classNamed(dotted, [this, name, base, done](quint64 classId) {
                    if (classId == 0)
                        done({.error = base.error});
                    else
                        placeOfStaticField(classId, name, done);
                });
                return;
            }
            if (!base.value.isObject() || base.value.isNull()) {
                done({.error = Tr::tr("The value has no field to write to.")});
                return;
            }
            placeOfFieldIn(base.value.bits, name, done);
        });
        return;
    }
    case JdwpExpression::Kind::Index: {
        const JdwpExpression index = *expression.index;
        evaluate(*expression.base, [this, index, done](const Evaluated &base) {
            if (!base.error.isEmpty()) {
                done({.error = base.error});
                return;
            }
            if (base.value.tag != Jdwp::ArrayValueTag || base.value.isNull()) {
                done({.error = Tr::tr("The value is not an array.")});
                return;
            }
            const quint64 array = base.value.bits;
            evaluate(index, [this, array, done](const Evaluated &position) {
                if (!position.error.isEmpty()) {
                    done({.error = position.error});
                    return;
                }
                if (!isIntegralTag(position.value.tag)) {
                    done({.error = Tr::tr("The index is not a number.")});
                    return;
                }
                // The type of an element is the type of the array without its
                // leading bracket, which is what its signature says.
                send(Jdwp::ObjectReferenceSet, Jdwp::ObjectReferenceType,
                     writer().writeObjectId(array).data(),
                     [this, array, position, done](const JdwpReply &reply) {
                    if (!reply.ok()) {
                        done({.error = JdwpClient::errorString(reply.errorCode)});
                        return;
                    }
                    JdwpReader r = reader(reply.data);
                    r.readByte();
                    const quint64 typeId = r.readReferenceTypeId();
                    withSignature(typeId, [this, array, position, done, typeId] {
                        const QString signature = m_classes.value(typeId).signature;
                        done({.kind = Place::Kind::Element, .owner = array,
                              .position = qint32(position.value.bits),
                              .signature = signature.mid(1)});
                    });
                });
            });
        });
        return;
    }
    default:
        break;
    }
    done({.error = Tr::tr("This is not something a value can be written to.")});
}

// Makes a value fit the type of the place it goes to, as far as Java would
// widen it by itself. A value that does not fit says so rather than being
// written as something else.
static Utils::Result<JdwpValue> coerced(const JdwpValue &value, const QString &signature)
{
    if (signature.isEmpty())
        return value;
    const char wanted = signature.at(0).toLatin1();
    if (!Jdwp::isPrimitiveTag(quint8(wanted))) {
        if (value.isObject())
            return value;
        return ResultError(Tr::tr("An object cannot hold %1.").arg(primitiveText(value)));
    }
    if (value.isObject()) {
        return ResultError(Tr::tr("A %1 cannot hold an object.")
                               .arg(typeName(signature)));
    }

    // A Java cast to anything narrower than a long goes through an int, so a
    // floating value stops at the ends of that range before it is cut to width.
    const auto asNarrow = [&value] {
        return isFloating(value.tag) ? saturated(asDouble(value), false) : asInteger(value);
    };

    JdwpValue result;
    result.tag = quint8(wanted);
    switch (wanted) {
    case Jdwp::BooleanValueTag:
        if (value.tag != Jdwp::BooleanValueTag)
            return ResultError(Tr::tr("A boolean can only hold true or false."));
        result.bits = value.bits ? 1 : 0;
        return result;
    case Jdwp::ByteValueTag:
        result.bits = quint64(qint8(asNarrow()));
        return result;
    case Jdwp::ShortValueTag:
        result.bits = quint64(qint16(asNarrow()));
        return result;
    case Jdwp::CharValueTag:
        result.bits = quint64(quint16(asNarrow()));
        return result;
    case Jdwp::IntValueTag:
        result.bits = quint64(qint32(asNarrow()));
        return result;
    case Jdwp::LongValueTag:
        result.bits = quint64(asInteger(value));
        return result;
    case Jdwp::FloatValueTag: {
        const float f = float(asDouble(value));
        quint32 bits = 0;
        std::memcpy(&bits, &f, sizeof(f));
        result.bits = bits;
        return result;
    }
    case Jdwp::DoubleValueTag: {
        const double d = asDouble(value);
        quint64 bits = 0;
        std::memcpy(&bits, &d, sizeof(d));
        result.bits = bits;
        return result;
    }
    default:
        break;
    }
    return ResultError(Tr::tr("A %1 cannot be written.").arg(typeName(signature)));
}

void JdwpImpl::writeTo(const Place &place, const JdwpValue &value, const QString &shown)
{
    const auto report = [this, shown](const JdwpReply &reply) {
        if (reply.ok()) {
            // The view read its values before the write, and the engine asked
            // for them again before it had happened, so it is asked once more.
            if (m_lastLocalsRequest.requestId != 0)
                fetchLocals(m_lastLocalsRequest);
            return;
        }
        emit message(Tr::tr("Cannot set %1: %2")
                         .arg(shown, JdwpClient::errorString(reply.errorCode)), LogError);
    };

    switch (place.kind) {
    case Place::Kind::Local: {
        JdwpWriter request = writer();
        request.writeObjectId(m_currentThread).writeFrameId(place.owner).writeInt(1)
            .writeInt(place.position).writeTaggedValue(value);
        send(Jdwp::StackFrameSet, Jdwp::StackFrameSetValues, request.data(), report);
        return;
    }
    case Place::Kind::Field: {
        JdwpWriter request = writer();
        request.writeObjectId(place.owner).writeInt(1).writeFieldId(place.fieldId)
            .writeUntaggedValue(value);
        send(Jdwp::ObjectReferenceSet, Jdwp::ObjectSetValues, request.data(), report);
        return;
    }
    case Place::Kind::StaticField: {
        JdwpWriter request = writer();
        request.writeReferenceTypeId(place.owner).writeInt(1).writeFieldId(place.fieldId)
            .writeUntaggedValue(value);
        send(Jdwp::ClassTypeSet, Jdwp::ClassTypeSetValues, request.data(), report);
        return;
    }
    case Place::Kind::Element: {
        JdwpWriter request = writer();
        request.writeObjectId(place.owner).writeInt(place.position).writeInt(1)
            .writeUntaggedValue(value);
        send(Jdwp::ArrayReferenceSet, Jdwp::ArraySetValues, request.data(), report);
        return;
    }
    }
}

void JdwpImpl::assignValueInDebugger(const WatchItemData &, const QString &expr,
                                     const QString &value)
{
    const Result<JdwpExpression> target = parseJdwpExpression(expr);
    if (!target) {
        emit message(target.error(), LogError);
        return;
    }
    const Result<JdwpExpression> assigned = parseJdwpExpression(value);
    if (!assigned) {
        emit message(assigned.error(), LogError);
        return;
    }
    using Kind = JdwpExpression::Kind;
    if (target->kind != Kind::Name && target->kind != Kind::Field && target->kind != Kind::Index) {
        emit message(Tr::tr("Cannot set %1: %2").arg(expr,
                         Tr::tr("This is not something a value can be written to.")), LogError);
        return;
    }
    const auto write = [this, expr](const Evaluated &result, const Place &place) {
        if (!place.error.isEmpty()) {
            emit message(Tr::tr("Cannot set %1: %2").arg(expr, place.error), LogError);
            return;
        }
        const Result<JdwpValue> fitted = coerced(result.value, place.signature);
        if (!fitted) {
            emit message(Tr::tr("Cannot set %1: %2").arg(expr, fitted.error()), LogError);
            return;
        }
        writeTo(place, *fitted, expr);
    };
    // A call in the value lets the thread run, and the frame id read before
    // that is gone after, so there the value is worked out before the place is
    // looked up. Otherwise a target that cannot be written to is found first.
    if (assigned->makesTheProgramRun()) {
        evaluate(*assigned, [this, target, expr, write](const Evaluated &result) {
            if (!result.error.isEmpty()) {
                emit message(Tr::tr("Cannot set %1: %2").arg(expr, result.error), LogError);
                return;
            }
            placeOf(*target, [result, write](const Place &place) { write(result, place); });
        });
        return;
    }
    placeOf(*target, [this, assigned, expr, write](const Place &place) {
        if (!place.error.isEmpty()) {
            emit message(Tr::tr("Cannot set %1: %2").arg(expr, place.error), LogError);
            return;
        }
        evaluate(*assigned, [this, place, expr, write](const Evaluated &result) {
            if (!result.error.isEmpty()) {
                emit message(Tr::tr("Cannot set %1: %2").arg(expr, result.error), LogError);
                return;
            }
            write(result, place);
        });
    });
}

// There is no command language on the wire, so what can be typed here is what
// a watcher takes: an expression, worked out where the program stands.
void JdwpImpl::executeDebuggerCommand(const QString &command, const WatchItemData &)
{
    const QString text = command.trimmed();
    if (text.isEmpty())
        return;
    if (m_running || m_decidingOnHit || m_currentThread == 0) {
        emit message(Tr::tr("The program has to be stopped for an expression to be worked out."),
                     ConsoleOutput);
        return;
    }
    // The views' calls run on the same thread, one after the other, and would
    // collide with the ones of the expression, as would those of another one.
    if (m_reportedLocalsGeneration != m_localsGeneration
        || m_commandRunGeneration == m_runGeneration) {
        m_pendingCommands.append(text);
        return;
    }
    const Result<JdwpExpression> parsed = parseJdwpExpression(text);
    if (!parsed) {
        emit message(parsed.error(), ConsoleOutput);
        return;
    }
    const int callGeneration = m_callGeneration;
    m_commandRunGeneration = m_runGeneration;
    const auto wentOn = [this, text, runGeneration = m_runGeneration] {
        if (runGeneration == m_runGeneration)
            return false;
        emit message(Tr::tr("The program went on before \"%1\" was worked out.").arg(text),
                     ConsoleOutput);
        return true;
    };
    evaluate(*parsed, [this, text, callGeneration, wentOn](const Evaluated &result) {
        if (wentOn())
            return;
        // What the views show was read before the call, and may be what it
        // changed. Reading them while the thread is still off in a call fails.
        if (m_callGeneration != callGeneration && m_callsOut.value(m_currentThread) == 0
            && m_lastLocalsRequest.requestId != 0) {
            fetchLocals(m_lastLocalsRequest);
        }
        if (!result.error.isEmpty()) {
            m_commandRunGeneration = -1;
            emit message(result.error, ConsoleOutput);
            runPendingCommands();
            return;
        }
        // The next one waits for this line, which may take a round trip.
        valueText(result.value, [this, text, wentOn](const QString &value) {
            if (wentOn())
                return;
            m_commandRunGeneration = -1;
            emit message(text + " = " + value, ConsoleOutput);
            runPendingCommands();
        });
    });
}

} // namespace Debugger::Internal
