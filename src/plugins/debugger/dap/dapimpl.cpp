// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dapimpl.h"

#include "dapclient.h"
#include "dapdataproviders.h"

#include "../debuggerprotocol.h"
#include "../debuggertr.h"
#include "../disassemblerlines.h"
#include "../genericdebuggerengine.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QJsonArray>
#include <QJsonDocument>

using namespace Utils;

namespace Debugger::Internal {

namespace {

class DapImplClient final : public DapClient
{
public:
    using DapClient::DapClient;

private:
    const QLoggingCategory &logCategory() final
    {
        static const QLoggingCategory category("qtc.dbg.dapimpl", QtWarningMsg);
        return category;
    }
};

} // namespace

static GdbMi constMi(const QString &name, const QString &data)
{
    GdbMi mi;
    mi.m_name = name;
    mi.m_data = data;
    mi.m_type = GdbMi::Const;
    return mi;
}

static DebuggerEngineSetupData dapImplSetupData()
{
    DebuggerEngineSetupData data;
    // Only what the protocol itself defines. Memory and disassembly are
    // optional in DAP, so they are offered here and refused per session if the
    // adapter turns out not to have them.
    data.capabilities = BreakConditionCapability | ShowMemoryCapability
                      | DisassemblerCapability | OperateByInstructionCapability;
    data.extraCapabilities = DebuggerExtraCapability::Detach
                           | DebuggerExtraCapability::Threads;
    data.startModes = DebuggerStartModeFlag::Launch | DebuggerStartModeFlag::AttachToProcess;
    data.toolTipHandling = ToolTipHandling::IfStoppedInferior;
    data.acceptsBreakpoint = [](const AcceptsBreakpointQuery &query) {
        if (query.startMode == AttachToCore)
            return false;
        return query.type == BreakpointByFileAndLine || query.type == BreakpointByFunction;
    };
    return data;
}

DapImpl::DapImpl(const DapStartData &startData)
    : DebuggerEngineInterface(dapImplSetupData())
    , m_startData(startData)
{}

DapImpl::~DapImpl()
{
    reportRunning(false);
    if (m_startData.channel)
        m_startData.channel->send = {};
}

// Once either way: whoever asked for the session is waiting to hear that it is
// up, and would wait forever if the adapter never got that far.
void DapImpl::reportRunning(bool running)
{
    if (m_runningReported || !m_startData.channel || !m_startData.channel->reportRunning)
        return;
    m_runningReported = true;
    m_startData.channel->reportRunning(running);
}

// The state machine takes a run request before the run itself, and an adapter
// that resumed on its own never made one.
void DapImpl::reportRunRequested()
{
    if (m_runRequestPending || !m_runReported)
        return;
    m_runRequestPending = true;
    emit inferiorEvent(InferiorEvent::RunRequested);
}

// A resume is both announced as an event and answered as a request, in that
// order, so the outcome is only reported for the first of the two to arrive.
void DapImpl::reportRunResult(bool ok)
{
    if (!m_runRequestPending)
        return;
    m_runRequestPending = false;
    emit inferiorEvent(ok ? InferiorEvent::RunOk : InferiorEvent::RunFailed);
}

void DapImpl::sendCustomRequest(const QString &command, const QJsonObject &arguments,
                                const DapSessionChannel::Answer &answer)
{
    QTC_ASSERT(m_client, answer(ResultError(Tr::tr("The debug adapter is not running."))); return);
    const int sequence = m_client->postRequest(command, arguments);
    if (answer)
        m_customRequests.insert(sequence, answer);
}

void DapImpl::start()
{
    const DapAdapterDescriptor &adapter = m_startData.adapter;
    IDataProvider *provider = nullptr;
    switch (adapter.kind) {
    case DapAdapterDescriptor::Kind::Executable:
        provider = new ProcessDataProvider(adapter.runData, adapter.command, this);
        break;
    case DapAdapterDescriptor::Kind::Server:
        provider = new TcpDataProvider(adapter.host, adapter.port, this);
        break;
    case DapAdapterDescriptor::Kind::Pipe:
        provider = new LocalSocketDataProvider(adapter.pipePath, this);
        break;
    }
    QTC_ASSERT(provider, emit inferiorEvent(InferiorEvent::EngineSetupFailed); return);

    m_client = new DapImplClient(provider, this);

    connect(m_client, &DapClient::requestSent,
            this, &DapImpl::logRequest);
    connect(m_client, &DapClient::started,
            this, &DapImpl::handleStarted);
    connect(m_client, &DapClient::done,
            this, &DapImpl::handleFinished);
    connect(m_client, &DapClient::readyReadStandardError,
            this, &DapImpl::handleStandardError);
    connect(m_client, &DapClient::responseReady,
            this, &DapImpl::handleResponse);
    connect(m_client, &DapClient::eventReady,
            this, &DapImpl::handleEvent);

    if (m_startData.channel) {
        m_startData.channel->send
            = [this](const QString &command, const QJsonObject &arguments,
                     const DapSessionChannel::Answer &answer) {
                  sendCustomRequest(command, arguments, answer);
              };
    }

    emit message(provider->executable(), LogInput);
    provider->start();
}

void DapImpl::handleStarted()
{
    emit inferiorEvent(InferiorEvent::EngineSetupOk);
    postRequest("initialize",
                QJsonObject{{"clientID", "QtCreator"},
                            {"clientName", "QtCreator"},
                            {"adapterID", m_startData.adapterId},
                            {"pathFormat", "path"},
                            {"linesStartAt1", true},
                            {"columnsStartAt1", true},
                            {"supportsVariableType", true},
                            {"supportsMemoryReferences", true}});
}

void DapImpl::handleFinished()
{
    if (m_client->dataProvider()->result() == ProcessResult::StartFailed)
        emit inferiorEvent(InferiorEvent::EngineSetupFailed);
    else if (!m_runReported)
        emit inferiorEvent(InferiorEvent::EngineRunFailed);
    auto provider = qobject_cast<ProcessDataProvider *>(m_client->dataProvider());
    emit engineProcessFinished(provider ? provider->resultData() : ProcessResultData());
}

void DapImpl::handleStandardError()
{
    const QString error = m_client->dataProvider()->readAllStandardError();
    if (!error.isEmpty())
        emit message(error, LogError);
}

void DapImpl::reportUnsupported(const QString &what)
{
    emit message(Tr::tr("\"%1\" is not part of the Debug Adapter Protocol, so the adapter "
                        "cannot be asked for it.").arg(what), LogWarning);
}

int DapImpl::postRequest(const QString &command, const QJsonObject &arguments)
{
    QTC_ASSERT(m_client, return -1);
    return m_client->postRequest(command, arguments);
}

void DapImpl::logRequest(int seq, const QString &command, const QJsonObject &arguments)
{
    emit message(QString::number(seq) + command + '('
                     + QString::fromUtf8(QJsonDocument(arguments).toJson(QJsonDocument::Compact))
                     + ')',
                 LogInput);
}

void DapImpl::postLaunchOrAttach()
{
    // The configuration is the adapter's own schema, so it travels as it came.
    postRequest(m_startData.attach ? "attach" : "launch", m_startData.configuration);
}

void DapImpl::shutdownInferior(ShutdownMode mode)
{
    if (!m_client) {
        emit inferiorEvent(InferiorEvent::ShutdownFinished);
        return;
    }
    m_shuttingDown = true;
    if (mode == ShutdownMode::Detach) {
        sendDetach();
    } else if (m_client->capabilities().supportsTerminateRequest) {
        postRequest("terminate", QJsonObject{{"restart", false}});
    } else {
        postRequest("disconnect", QJsonObject{{"restart", false},
                                              {"terminateDebuggee", true}});
    }
}

void DapImpl::shutdownEngine()
{
    if (m_client) {
        // DapClient::sendDisconnect() takes the debuggee with it, which is the
        // one thing a session that detached must not do.
        if (!m_detaching)
            m_client->sendDisconnect();
        m_client->dataProvider()->kill();
    }
    emit inferiorEvent(InferiorEvent::EngineShutdownFinished);
}

void DapImpl::execute(const ExecutionRequest &request)
{
    QTC_ASSERT(m_client, return);

    const auto stepArguments = [this](bool byInstruction) {
        QJsonObject args{{"threadId", m_currentThreadId}};
        if (byInstruction)
            args.insert("granularity", "instruction");
        return args;
    };

    switch (request.command) {
    case ExecutionCommand::Continue:
        // A resume the engine asked for before it heard that the debuggee had
        // ended. There is nothing left for the adapter to continue.
        if (m_inferiorDoneReported) {
            emit inferiorEvent(InferiorEvent::InferiorIll);
            return;
        }
        m_stopRequested = false;
        reportRunRequested();
        m_client->sendContinue(m_currentThreadId);
        return;
    case ExecutionCommand::Interrupt:
        if (!m_inferiorRunning) {
            emit inferiorEvent(InferiorEvent::StopOk);
            return;
        }
        m_stopRequested = true;
        m_client->sendPause();
        return;
    case ExecutionCommand::StepIn:
        reportRunRequested();
        postRequest("stepIn", stepArguments(request.flag));
        return;
    case ExecutionCommand::StepOver:
        reportRunRequested();
        postRequest("next", stepArguments(request.flag));
        return;
    case ExecutionCommand::StepOut:
        reportRunRequested();
        m_client->sendStepOut(m_currentThreadId);
        return;
    case ExecutionCommand::Detach:
        sendDetach();
        return;
    case ExecutionCommand::Abort:
        m_client->sendTerminate();
        return;
    case ExecutionCommand::JumpToLine:
        if (!m_client->capabilities().supportsGotoTargetsRequest) {
            reportUnsupported(Tr::tr("Jump to Line"));
            return;
        }
        postRequest("gotoTargets",
                    QJsonObject{{"source",
                                 QJsonObject{{"path", request.context.fileName.path()}}},
                                {"line", request.context.textPosition.line}});
        return;
    case ExecutionCommand::RunToLine:
        reportUnsupported(Tr::tr("Run to Line"));
        return;
    case ExecutionCommand::RunToFunction:
        reportUnsupported(Tr::tr("Run to Function"));
        return;
    case ExecutionCommand::RepeatLastCommand:
        if (m_lastLocalsRequest)
            refresh(*m_lastLocalsRequest);
        return;
    case ExecutionCommand::Return:
    case ExecutionCommand::ResetInferior:
    case ExecutionCommand::RecordReverse:
        reportUnsupported(Tr::tr("this command"));
        return;
    }
}

void DapImpl::sendDetach()
{
    m_detaching = true;
    postRequest("disconnect", QJsonObject{{"restart", false},
                                          {"terminateDebuggee", false}});
}

void DapImpl::sendBreakpointsFor(const FilePath &file)
{
    QTC_ASSERT(m_client, return);
    QJsonArray breakpoints;
    for (const Breakpoint &breakpoint : m_sourceBreakpoints.value(file)) {
        if (!breakpoint.enabled)
            continue;
        const BreakpointParameters &params = breakpoint.params;
        QJsonObject item{{"line", params.textPosition.line}};
        if (params.textPosition.column > 0)
            item.insert("column", params.textPosition.column);
        if (!params.condition.isEmpty())
            item.insert("condition", params.condition);
        if (params.ignoreCount > 0)
            item.insert("hitCondition", QString::number(params.ignoreCount));
        if (params.tracepoint && !params.message.isEmpty())
            item.insert("logMessage", params.message);
        breakpoints.append(item);
    }
    const int seq = m_client->postRequest(
        "setBreakpoints",
        QJsonObject{{"source", QJsonObject{{"path", file.path()},
                                           {"name", file.fileName()}}},
                    {"breakpoints", breakpoints},
                    {"sourceModified", false}});
    if (seq >= 0)
        m_breakpointRequests.insert(seq, file);
}

void DapImpl::sendFunctionBreakpoints()
{
    QTC_ASSERT(m_client, return);
    if (!m_client->capabilities().supportsFunctionBreakpoints) {
        reportUnsupported(Tr::tr("breakpoints by function name"));
        return;
    }
    QJsonArray breakpoints;
    for (const Breakpoint &breakpoint : m_functionBreakpoints) {
        if (!breakpoint.enabled)
            continue;
        QJsonObject item{{"name", breakpoint.params.functionName}};
        if (!breakpoint.params.condition.isEmpty())
            item.insert("condition", breakpoint.params.condition);
        breakpoints.append(item);
    }
    const int seq = m_client->postRequest("setFunctionBreakpoints",
                                          QJsonObject{{"breakpoints", breakpoints}});
    if (seq >= 0)
        m_functionBreakpointRequests.insert(seq);
}

void DapImpl::changeBreakpoint(const BreakpointChangeRequest &request)
{
    QTC_ASSERT(m_client, return);

    if (request.op == BreakpointOp::EnableSub) {
        // A breakpoint reported over DAP has no locations of its own to enable.
        reportUnsupported(Tr::tr("enabling a single breakpoint location"));
        emit breakpointEvent(request.requestId, BreakpointOp::EnableSub, false);
        return;
    }

    const BreakpointParameters &params = request.params;
    bool byFunction = params.type == BreakpointByFunction;
    FilePath file = params.fileName;
    bool inArray = false;

    if (request.op == BreakpointOp::Insert) {
        QList<Breakpoint> &list = byFunction ? m_functionBreakpoints
                                             : m_sourceBreakpoints[file];
        list.append({request.requestId, request.op, request.modelId, {}, params,
                     params.enabled});
        inArray = params.enabled;
    } else {
        // A change names the breakpoint by what the adapter called it and
        // brings no location along, so which array has to go out again is
        // looked up rather than taken from the request.
        const auto named = [&request](const Breakpoint &breakpoint) {
            if (!request.responseId.isEmpty())
                return breakpoint.responseId == request.responseId;
            return !request.params.fileName.isEmpty()
                   && breakpoint.modelId == request.modelId;
        };
        QList<Breakpoint> *list = nullptr;
        if (Utils::contains(m_functionBreakpoints, named)) {
            byFunction = true;
            list = &m_functionBreakpoints;
        } else {
            for (auto it = m_sourceBreakpoints.begin(); it != m_sourceBreakpoints.end(); ++it) {
                if (Utils::contains(*it, named)) {
                    byFunction = false;
                    file = it.key();
                    list = &*it;
                    break;
                }
            }
        }
        if (!list) {
            emit breakpointEvent(request.requestId, request.op, false);
            return;
        }
        const auto it = std::find_if(list->begin(), list->end(), named);
        if (request.op == BreakpointOp::Remove) {
            list->erase(it);
        } else {
            if (!params.fileName.isEmpty())
                it->params = params;
            it->enabled = params.enabled;
            it->requestId = request.requestId;
            it->op = request.op;
            inArray = it->enabled;
        }
    }

    // Until the adapter says it is ready for configuration, the set is only
    // remembered: DAP takes breakpoints between "initialized" and
    // "configurationDone", not before.
    if (m_configured) {
        if (byFunction)
            sendFunctionBreakpoints();
        else
            sendBreakpointsFor(file);
    }

    // The answer to a setBreakpoints lists what was sent, so only what is in
    // the array gets a reply of its own. A removal is not, and neither is a
    // disabled breakpoint - DAP expresses that by leaving it out.
    if (!inArray)
        emit breakpointEvent(request.requestId, request.op, true);
}

void DapImpl::handleBreakpointsSet(const QJsonObject &response)
{
    const int seq = response.value("request_seq").toInt();
    const bool byFunction = m_functionBreakpointRequests.remove(seq);
    const FilePath file = m_breakpointRequests.take(seq);
    QList<Breakpoint> &known = byFunction ? m_functionBreakpoints : m_sourceBreakpoints[file];
    const QJsonArray reported = response.value("body").toObject()
                                    .value("breakpoints").toArray();

    // The answer is positional: it lists the breakpoints that were sent, in the
    // order they were sent, so only the enabled ones line up with it.
    QList<Breakpoint> sent;
    for (Breakpoint &breakpoint : known) {
        if (!breakpoint.enabled)
            continue;
        // What the adapter calls it is how a later change to it will be named.
        const QJsonObject item = reported.at(sent.size()).toObject();
        if (item.contains("id"))
            breakpoint.responseId = QString::number(item.value("id").toInt());
        sent.append(breakpoint);
    }

    for (int i = 0; i < sent.size(); ++i) {
        const QJsonObject item = reported.at(i).toObject();
        const bool verified = item.value("verified").toBool();
        GdbMi bkpt;
        bkpt.m_type = GdbMi::Tuple;
        bkpt.addChild(constMi("number", QString::number(item.value("id").toInt())));
        bkpt.addChild(constMi("line", QString::number(item.value("line").toInt())));
        bkpt.addChild(constMi("file", item.value("source").toObject()
                                          .value("path").toString()));
        bkpt.addChild(constMi("pending", verified ? "0" : "1"));
        GdbMi data;
        data.m_type = GdbMi::List;
        data.addChild(bkpt);
        // An unverified breakpoint is one the adapter has taken but not bound
        // yet, which is what a breakpoint set before the program is loaded
        // always is. Only "failed" says it was refused - as does an answer that
        // does not list it at all, which a refused request lists nothing in.
        const bool taken = i < reported.size()
                           && (verified || item.value("reason").toString() != "failed");
        emit breakpointEvent(sent.at(i).requestId, sent.at(i).op, taken, data);
    }
}

const DapImpl::Breakpoint *DapImpl::breakpointForResponseId(const QString &responseId) const
{
    for (const QList<Breakpoint> &known : m_sourceBreakpoints) {
        for (const Breakpoint &breakpoint : known) {
            if (breakpoint.responseId == responseId)
                return &breakpoint;
        }
    }
    for (const Breakpoint &breakpoint : m_functionBreakpoints) {
        if (breakpoint.responseId == responseId)
            return &breakpoint;
    }
    return nullptr;
}

void DapImpl::handleBreakpointChanged(const QJsonObject &event)
{
    const QJsonObject body = event.value("body").toObject();
    // "new" and "removed" are about breakpoints of the adapter's own making,
    // which nothing here has a request to answer for.
    if (body.value("reason").toString() != "changed")
        return;

    const QJsonObject item = body.value("breakpoint").toObject();
    const QString responseId = QString::number(item.value("id").toInt());
    GdbMi bkpt;
    bkpt.m_type = GdbMi::Tuple;
    bkpt.addChild(constMi("number", responseId));
    if (item.contains("line"))
        bkpt.addChild(constMi("line", QString::number(item.value("line").toInt())));
    const QString path = item.value("source").toObject().value("path").toString();
    if (!path.isEmpty()) {
        bkpt.addChild(constMi("file", path));
        bkpt.addChild(constMi("fullname", path));
    }
    const QString address = item.value("instructionReference").toString();
    if (address.startsWith("0x"))
        bkpt.addChild(constMi("addr", address));
    // A field the update does not carry counts as the default rather than as
    // unchanged, so what the event leaves out is filled in from the request.
    if (const Breakpoint *known = breakpointForResponseId(responseId)) {
        bkpt.addChild(constMi("enabled", known->enabled ? "y" : "n"));
        if (!known->params.condition.isEmpty())
            bkpt.addChild(constMi("cond", known->params.condition));
    }

    GdbMi data;
    data.m_type = GdbMi::List;
    data.addChild(bkpt);
    emit breakpointModified(data);
}

void DapImpl::refresh(const RefreshRequest &request)
{
    QTC_ASSERT(m_client, return);

    switch (request.kind) {
    case RefreshKind::Locals:
        m_lastLocalsRequest = request;
        m_localsRequestId = request.requestId;
        m_expandedINames = request.expandedINames;
        m_locals.clear();
        m_localRoots.clear();
        m_pendingVariables.clear();
        m_variableRequests.clear();
        if (m_currentFrameId < 0) {
            reportLocals();
            return;
        }
        m_scopesSeq = m_client->scopes(m_currentFrameId);
        return;
    case RefreshKind::FullStack:
        if (const int seq = m_client->stackTrace(m_currentThreadId); seq >= 0)
            m_stackTraceRequests.insert(seq, {false, request.requestId});
        return;
    case RefreshKind::Threads:
        if (const int seq = m_client->postRequest("threads"); seq >= 0)
            m_threadRequests.insert(seq, request.requestId);
        return;
    case RefreshKind::AllSymbols:
        // Nothing loads symbols over the protocol; what the caller is after is
        // what the adapter reads them for.
        refresh({request.requestId, RefreshKind::FullStack});
        refresh({request.requestId, RefreshKind::Locals});
        return;
    case RefreshKind::DebuggingHelpers:
        // There are no dumpers behind a stock adapter, so reloading them is
        // asking it for the values again.
        refresh({request.requestId, RefreshKind::Locals});
        return;
    default:
        // Modules, registers, symbols and snapshots have no counterpart the
        // protocol defines, so the view is answered with nothing rather than
        // being left waiting.
        emit refreshDataReceived(request.requestId, request.kind, {});
        return;
    }
}

void DapImpl::handleResponse(DapResponseType type, const QJsonObject &response)
{
    const QString command = response.value("command").toString();
    const bool success = response.value("success").toBool();

    if (const DapSessionChannel::Answer answer
        = m_customRequests.take(response.value("request_seq").toInt())) {
        if (success)
            answer(response.value("body").toObject());
        else
            answer(ResultError(response.value("message").toString()));
        return;
    }

    // The handlers below read a body an unsuccessful answer does not have, so
    // the reason it gives is passed on here or nowhere.
    if (!success && !command.isEmpty())
        emit message(command + ": " + response.value("message").toString(), LogError);

    switch (type) {
    case DapResponseType::Initialize:
        return;
    case DapResponseType::ConfigurationDone:
        postLaunchOrAttach();
        reportRunning(true);
        emit inferiorEvent(InferiorEvent::RunAndInferiorRunOk);
        m_inferiorRunning = true;
        m_runReported = true;
        // Whatever became of the debuggee while this was in flight is only
        // reportable now that the run itself has been.
        if (m_pendingResult) {
            const InferiorResultData result = *m_pendingResult;
            m_pendingResult.reset();
            m_inferiorDoneReported = true;
            emit inferiorDone(result);
        }
        return;
    case DapResponseType::Continue:
        reportRunResult(success);
        m_inferiorRunning = success;
        return;
    case DapResponseType::StepIn:
    case DapResponseType::StepOut:
    case DapResponseType::StepOver:
        reportRunResult(success);
        return;
    case DapResponseType::StackTrace:
        handleStackTrace(response);
        return;
    case DapResponseType::Scopes:
        handleScopes(response);
        return;
    case DapResponseType::Variables:
        handleVariables(response);
        return;
    case DapResponseType::Pause:
        if (!success)
            emit inferiorEvent(InferiorEvent::StopFailed);
        return;
    case DapResponseType::SetBreakpoints:
    case DapResponseType::SetFunctionBreakpoints:
        handleBreakpointsSet(response);
        return;
    case DapResponseType::Launch:
    case DapResponseType::Attach:
        if (!success)
            emit inferiorEvent(InferiorEvent::EngineRunFailed);
        return;
    case DapResponseType::Evaluate:
        emit message(response.value("body").toObject().value("result").toString(),
                     LogMisc);
        return;
    default:
        break;
    }

    if (command == "threads") {
        const quint64 requestId = m_threadRequests.take(response.value("request_seq").toInt());
        GdbMi threads;
        threads.m_type = GdbMi::List;
        threads.m_name = "threads";
        for (const QJsonValue &value : response.value("body").toObject()
                                           .value("threads").toArray()) {
            const QJsonObject item = value.toObject();
            GdbMi thread;
            thread.m_type = GdbMi::Tuple;
            thread.addChild(constMi("id", QString::number(item.value("id").toInt())));
            thread.addChild(constMi("target-id", item.value("name").toString()));
            thread.addChild(constMi("state", m_inferiorRunning ? "running" : "stopped"));
            threads.addChild(thread);
        }
        GdbMi all;
        all.m_type = GdbMi::Tuple;
        all.addChild(threads);
        all.addChild(constMi("current-thread-id", QString::number(m_currentThreadId)));
        emit refreshDataReceived(requestId, RefreshKind::Threads, all);
        return;
    }
    if (command == "readMemory") {
        handleReadMemory(response);
        return;
    }
    if (command == "disassemble") {
        if (success)
            handleDisassemble(response);
        return;
    }
    if (m_shuttingDown && (command == "terminate" || command == "disconnect")) {
        // The shutdown the engine asked for is over once the adapter has
        // answered for it, whether or not it could do what was asked.
        emit inferiorEvent(InferiorEvent::ShutdownFinished);
        return;
    }
    if (command == "disconnect" && m_detaching) {
        reportInferiorDone({0, InferiorExitStatus::Detached});
        return;
    }
    if (command == "gotoTargets" && success) {
        const QJsonArray targets = response.value("body").toObject()
                                       .value("targets").toArray();
        if (!targets.isEmpty()) {
            postRequest("goto", QJsonObject{{"threadId", m_currentThreadId},
                                            {"targetId", targets.first().toObject()
                                                             .value("id").toInt()}});
        }
        return;
    }
}

void DapImpl::handleEvent(DapEventType type, const QJsonObject &event)
{
    switch (type) {
    case DapEventType::Initialized:
        // The one window in which DAP accepts breakpoints, so whatever was
        // collected while the adapter started goes out now.
        m_configured = true;
        for (auto it = m_sourceBreakpoints.cbegin(); it != m_sourceBreakpoints.cend(); ++it)
            sendBreakpointsFor(it.key());
        if (!m_functionBreakpoints.isEmpty())
            sendFunctionBreakpoints();
        m_client->sendConfigurationDone();
        return;
    case DapEventType::Stopped:
        handleStopped(event);
        return;
    case DapEventType::DapBreakpoint:
        handleBreakpointChanged(event);
        return;
    case DapEventType::Exited: {
        InferiorResultData result;
        result.exitCode = event.value("body").toObject().value("exitCode").toInt();
        reportInferiorDone(result);
        return;
    }
    case DapEventType::Output: {
        const QJsonObject body = event.value("body").toObject();
        const QString category = body.value("category").toString();
        emit message(body.value("output").toString(),
                     category == "stderr" ? AppError : AppOutput);
        return;
    }
    default:
        break;
    }

    const QString name = event.value("event").toString();
    if (name == "continued") {
        // The adapter resumed on its own - cortex-debug does once it has the
        // target reset, and a launch that runs to an entry point does too -
        // and until that is passed on, the views keep showing the stop it
        // reported before and a Continue goes out to something already
        // running, which the adapter can only refuse.
        const bool wasRunning = m_inferiorRunning;
        m_inferiorRunning = true;
        if (!wasRunning) {
            reportRunRequested();
            reportRunResult(true);
        }
    } else if (name == "terminated") {
        reportInferiorDone({});
    } else if (name == "process") {
        const qint64 pid = event.value("body").toObject().value("systemProcessId").toInteger();
        if (pid != 0)
            emit inferiorPidKnown(ProcessHandle(pid));
    }
}

void DapImpl::handleStopped(const QJsonObject &event)
{
    const QJsonObject body = event.value("body").toObject();
    m_currentThreadId = body.value("threadId").toInt();
    m_inferiorRunning = false;

    const QString reason = body.value("reason").toString();
    if (reason == "exception" || reason == "signal") {
        // The protocol names no signals. "text" is where an adapter says what
        // it was, if it says anything at all.
        emit signalReceived(body.value("text").toString(),
                            body.value("description").toString());
    }

    // Report the stop only once the location is known, as the other backends do.
    const int seq = m_client->stackTrace(m_currentThreadId);
    if (seq < 0) {
        reportStop();
        return;
    }
    m_stackTraceRequests.insert(seq, {true, 0});
}

// An adapter can be done before the engine has been told the run started, and
// the engine only accepts the two in that order.
void DapImpl::reportInferiorDone(InferiorResultData result)
{
    if (m_inferiorDoneReported)
        return;
    m_inferiorRunning = false;
    // The engine asked for this end. It hears about it as the answer to its own
    // request, and an exit reported on top of that is one it would act on.
    if (m_shuttingDown)
        return;
    if (m_detaching)
        result.exitStatus = InferiorExitStatus::Detached;
    if (!m_runReported) {
        m_pendingResult = result;
        return;
    }
    m_inferiorDoneReported = true;
    emit inferiorDone(result);
}

void DapImpl::reportStop()
{
    emit inferiorEvent(m_stopRequested ? InferiorEvent::StopOk
                                       : InferiorEvent::SpontaneousStop);
    m_stopRequested = false;
}

void DapImpl::handleStackTrace(const QJsonObject &response)
{
    const StackTraceRequest request
        = m_stackTraceRequests.take(response.value("request_seq").toInt());
    const QJsonArray frames = response.value("body").toObject()
                                  .value("stackFrames").toArray();

    m_frameIds.clear();
    for (const QJsonValue &value : frames)
        m_frameIds.append(value.toObject().value("id").toInt());
    m_currentFrameId = m_frameIds.isEmpty() ? -1 : m_frameIds.first();

    if (request.reportsStop) {
        const QJsonObject top = frames.isEmpty() ? QJsonObject() : frames.first().toObject();
        const int lineNumber = top.value("line").toInt();
        const FilePath fileName
            = FilePath::fromUserInput(top.value("source").toObject().value("path").toString());
        if (lineNumber != 0 && fileName.exists())
            emit locationChanged(fileName, lineNumber);
        reportStop();
        return;
    }

    GdbMi frameList;
    frameList.m_name = "frames";
    frameList.m_type = GdbMi::List;
    int level = 0;
    for (const QJsonValue &value : frames) {
        const QJsonObject item = value.toObject();
        GdbMi frame;
        frame.m_type = GdbMi::Tuple;
        frame.addChild(constMi("level", QString::number(level++)));
        frame.addChild(constMi("function", item.value("name").toString()));
        const QString path = item.value("source").toObject().value("path").toString();
        frame.addChild(constMi("file", path));
        frame.addChild(constMi("fullname", path));
        frame.addChild(constMi("line", QString::number(item.value("line").toInt())));
        frame.addChild(constMi("address",
                               item.value("instructionPointerReference").toString()));
        frameList.addChild(frame);
    }
    GdbMi stack;
    stack.m_type = GdbMi::Tuple;
    stack.m_name = "stack";
    stack.addChild(frameList);
    GdbMi all;
    all.m_type = GdbMi::Tuple;
    all.addChild(stack);

    emit refreshDataReceived(request.refreshRequestId, RefreshKind::FullStack, all);
}

void DapImpl::selectThread(const QString &threadId)
{
    m_currentThreadId = threadId.toInt();
}

void DapImpl::activateFrame(int index)
{
    if (index >= 0 && index < m_frameIds.size())
        m_currentFrameId = m_frameIds.at(index);
}

void DapImpl::queueVariables(const QString &iname, int reference)
{
    m_pendingVariables.enqueue({iname, reference});
}

void DapImpl::continueLocalsWalk()
{
    while (!m_pendingVariables.isEmpty()) {
        const QPair<QString, int> next = m_pendingVariables.dequeue();
        const int seq = m_client->postRequest("variables",
                                              QJsonObject{{"variablesReference", next.second}});
        if (seq >= 0) {
            m_variableRequests.insert(seq, next.first);
            return;
        }
    }
    reportLocals();
}

void DapImpl::handleScopes(const QJsonObject &response)
{
    // A stop can be reported more than once - cortex-debug reports each of
    // them twice - and each report asks for the locals again. Answers to a
    // walk that has been started over would otherwise be added to the one
    // running now, listing everything twice.
    if (response.value("request_seq").toInt() != m_scopesSeq)
        return;

    // The adapter offers more than the Locals view is about: registers have a
    // view of their own, and neither globals nor file statics are locals.
    static const QSet<QString> notLocalHints = {"registers", "globals"};
    // Not every adapter sets the hint - cortex-debug names its scopes "Local",
    // "Global", "Static: <file>" and "Registers" and hints none of them - so
    // the name has to say what the hint does not.
    static const QStringList notLocalNames = {"register", "global", "static"};
    for (const QJsonValue &value : response.value("body").toObject()
                                       .value("scopes").toArray()) {
        const QJsonObject scope = value.toObject();
        const QString hint = scope.value("presentationHint").toString();
        if (!hint.isEmpty()) {
            if (notLocalHints.contains(hint))
                continue;
        } else {
            const QString name = scope.value("name").toString().toLower();
            if (Utils::anyOf(notLocalNames, [&name](const QString &prefix) {
                    return name.startsWith(prefix);
                })) {
                continue;
            }
        }
        const int reference = scope.value("variablesReference").toInt();
        if (reference != 0)
            queueVariables("local", reference);
    }
    continueLocalsWalk();
}

void DapImpl::handleVariables(const QJsonObject &response)
{
    const QString parent = m_variableRequests.take(response.value("request_seq").toInt());
    const QJsonArray variables = response.value("body").toObject()
                                     .value("variables").toArray();

    // Where in the parent this level continues. More than one scope can be
    // locals - arguments are their own scope for some adapters - so the root
    // count carries across the answers rather than restarting per scope.
    int index = parent == "local" ? m_localRoots.size()
                                  : m_locals.value(parent).childINames.size();
    for (const QJsonValue &value : variables) {
        const QJsonObject item = value.toObject();
        Local local;
        // The name is whatever the adapter chose and can hold anything, so the
        // path uses the position instead and keeps the name for display only.
        local.iname = parent + '.' + QString::number(index++);
        local.name = item.value("name").toString();
        // An adapter is free to put more than a type name here - cortex-debug
        // sends a whole hover text - and the type column is one line.
        local.type = item.value("type").toString().section('\n', 0, 0);
        local.value = item.value("value").toString();
        local.reference = item.value("variablesReference").toInt();
        local.hasChildren = local.reference != 0;
        local.address = item.value("memoryReference").toString().toULongLong(nullptr, 0);

        if (parent == "local")
            m_localRoots.append(local.iname);
        else
            m_locals[parent].childINames.append(local.iname);
        m_locals.insert(local.iname, local);

        if (local.hasChildren && m_expandedINames.contains(local.iname))
            queueVariables(local.iname, local.reference);
    }
    continueLocalsWalk();
}

GdbMi DapImpl::localsItem(const QString &iname) const
{
    const Local local = m_locals.value(iname);
    GdbMi item;
    item.m_type = GdbMi::Tuple;
    item.addChild(constMi("iname", local.iname));
    item.addChild(constMi("name", local.name));
    item.addChild(constMi("type", local.type));
    item.addChild(constMi("value", local.value));
    item.addChild(constMi("numchild", local.hasChildren ? "1" : "0"));
    if (local.address != 0)
        item.addChild(constMi("address", QString::number(local.address)));

    if (!local.childINames.isEmpty()) {
        GdbMi children;
        children.m_type = GdbMi::List;
        children.m_name = "children";
        for (const QString &child : local.childINames)
            children.addChild(localsItem(child));
        item.addChild(children);
    }
    return item;
}

void DapImpl::reportLocals()
{
    GdbMi data;
    data.m_type = GdbMi::List;
    data.m_name = "data";
    for (const QString &iname : m_localRoots)
        data.addChild(localsItem(iname));

    GdbMi all;
    all.m_type = GdbMi::Tuple;
    all.addChild(data);
    emit refreshDataReceived(m_localsRequestId, RefreshKind::Locals, all);
}

void DapImpl::accessMemory(MemoryOp op, quint64 requestId, quint64 addr, quint64 lengthOrSize,
                           const QByteArray &data)
{
    QTC_ASSERT(m_client, return);
    const QString reference = "0x" + QString::number(addr, 16);

    if (op == MemoryOp::Fetch) {
        if (!m_client->capabilities().supportsReadMemoryRequest) {
            reportUnsupported(Tr::tr("reading memory"));
            return;
        }
        const int seq = m_client->postRequest("readMemory",
                                              QJsonObject{{"memoryReference", reference},
                                                          {"count", qint64(lengthOrSize)}});
        if (seq >= 0)
            m_memoryRequests.insert(seq, {requestId, addr, lengthOrSize});
        return;
    }
    if (!m_client->capabilities().supportsWriteMemoryRequest) {
        reportUnsupported(Tr::tr("writing memory"));
        return;
    }
    postRequest("writeMemory",
                QJsonObject{{"memoryReference", reference},
                            {"data", QString::fromUtf8(data.toBase64())}});
}

void DapImpl::handleReadMemory(const QJsonObject &response)
{
    const MemoryRequest request = m_memoryRequests.take(response.value("request_seq").toInt());
    if (request.length == 0)
        return;
    QByteArray data = QByteArray::fromBase64(
        response.value("body").toObject().value("data").toString().toUtf8());
    // A read the adapter refused, or answered only in part, is still an answer:
    // what could not be read reads as zero, as it does in the other backends.
    data.truncate(qsizetype(request.length));
    data.append(QByteArray(qsizetype(request.length) - data.size(), char(0)));
    emit memoryDataReceived(request.requestId, request.address, data);
}

void DapImpl::fetchDisassembly(quint64 requestId, quint64 address, const QString &functionName)
{
    QTC_ASSERT(m_client, return);
    if (!m_client->capabilities().supportsDisassembleRequest) {
        reportUnsupported(Tr::tr("disassembly"));
        return;
    }
    if (address == 0) {
        emit message(Tr::tr("Disassembling \"%1\" needs its address, which the protocol "
                            "does not offer a way to ask for.").arg(functionName), LogWarning);
        return;
    }
    const QString reference = "0x" + QString::number(address, 16);
    const int seq = m_client->postRequest(
        "disassemble", QJsonObject{{"memoryReference", reference},
                                   {"instructionCount", 100}});
    if (seq >= 0)
        m_disassemblyRequests.insert(seq, {requestId, address});
}

void DapImpl::handleDisassemble(const QJsonObject &response)
{
    const DisassemblyRequest request
        = m_disassemblyRequests.take(response.value("request_seq").toInt());
    DisassemblerLines lines;
    QString function;
    quint64 functionAddress = 0;
    QString sourceFile;
    int sourceLine = 0;
    int bytesLength = 0;
    for (const QJsonValue &value : response.value("body").toObject()
                                       .value("instructions").toArray()) {
        const QJsonObject item = value.toObject();
        DisassemblerLine line;
        line.address = item.value("address").toString().toULongLong(nullptr, 0);
        line.data = item.value("instruction").toString();
        line.bytes = item.value("instructionBytes").toString();
        bytesLength = qMax(bytesLength, int(line.bytes.size()));

        // Only the instruction a function starts at is named, so the ones after
        // it belong to the last name seen.
        const QString symbol = item.value("symbol").toString();
        if (!symbol.isEmpty() && symbol != function) {
            function = symbol;
            functionAddress = line.address;
            DisassemblerLine header;
            header.data = "Function: " + symbol;
            lines.appendLine(header);
        }
        line.function = function;
        if (functionAddress != 0 && line.address >= functionAddress)
            line.offset = uint(line.address - functionAddress);

        const QString file = item.value("location").toObject().value("path").toString();
        const int number = item.value("line").toInt();
        if (!file.isEmpty() && number != 0 && (file != sourceFile || number != sourceLine)) {
            sourceFile = file;
            sourceLine = number;
            lines.appendSourceLine(file, number);
        }
        lines.appendLine(line);
    }
    lines.setBytesLength(bytesLength);
    emit disassemblyReceived(request.requestId, lines);
}

void DapImpl::assignValueInDebugger(const WatchItemData &item, const QString &expr,
                                    const QString &value)
{
    Q_UNUSED(item)
    QTC_ASSERT(m_client, return);
    if (!m_client->capabilities().supportsSetExpression) {
        reportUnsupported(Tr::tr("assigning a value"));
        return;
    }
    postRequest("setExpression", QJsonObject{{"expression", expr},
                                             {"value", value},
                                             {"frameId", m_currentFrameId}});
}

void DapImpl::executeDebuggerCommand(const QString &command, const WatchItemData &inspectorItem)
{
    Q_UNUSED(inspectorItem)
    QTC_ASSERT(m_client, return);
    postRequest("evaluate", QJsonObject{{"expression", command},
                                        {"frameId", m_currentFrameId},
                                        {"context", "repl"}});
}

void DapImpl::setRegisterValue(const QString &name, const QString &value)
{
    Q_UNUSED(name)
    Q_UNUSED(value)
    reportUnsupported(Tr::tr("setting a register"));
}

void DapImpl::setPeripheralRegisterValue(quint64 address, quint64 value)
{
    Q_UNUSED(address)
    Q_UNUSED(value)
    reportUnsupported(Tr::tr("setting a peripheral register"));
}

void DapImpl::watchPoint(quint64 requestId, const QPoint &pnt)
{
    Q_UNUSED(pnt)
    reportUnsupported(Tr::tr("watching a widget"));
    emit watchPointResolved(requestId, 0, {});
}

void DapImpl::createSnapshot(quint64 requestId)
{
    reportUnsupported(Tr::tr("snapshots"));
    emit snapshotCreated(requestId, false, {});
}

DebuggerEngine *createDapAdapterEngine(const DapStartData &data)
{
    return new GenericDebuggerEngine("DAP", new DapImpl(data));
}

} // namespace Debugger::Internal
