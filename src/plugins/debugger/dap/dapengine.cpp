// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "dapengine.h"

#include "cmakedapengine.h"
#include "dapclient.h"
#include "gdbdapengine.h"

#include <debugger/breakhandler.h>
#include <debugger/debuggeractions.h>
#include <debugger/debuggercore.h>
#include <debugger/debuggerdialogs.h>
#include <debugger/debuggerinternalconstants.h>
#include <debugger/debuggermainwindow.h>
#include <debugger/debuggerplugin.h>
#include <debugger/debuggerprotocol.h>
#include <debugger/debuggerruncontrol.h>
#include <debugger/debuggertooltipmanager.h>
#include <debugger/debuggertr.h>
#include <debugger/moduleshandler.h>
#include <debugger/procinterrupt.h>
#include <debugger/registerhandler.h>
#include <debugger/sourceutils.h>
#include <debugger/stackhandler.h>
#include <debugger/threaddata.h>
#include <debugger/watchhandler.h>
#include <debugger/watchutils.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/process.h>
#include <utils/qtcassert.h>
#include <utils/temporarydirectory.h>

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/messagebox.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projecttree.h>

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLocalSocket>
#include <QThread>
#include <QTimer>
#include <QVariant>

using namespace Core;
using namespace Utils;

static Q_LOGGING_CATEGORY(dapEngineLog, "qtc.dbg.dapengine", QtWarningMsg)

namespace Debugger::Internal {

void DapEngine::executeDebuggerCommand(const QString &/*command*/)
{
    QTC_ASSERT(state() == InferiorStopOk, qCDebug(dapEngineLog) << state());
//    if (state() == DebuggerNotReady) {
//        showMessage("DAP PROCESS NOT RUNNING, PLAIN CMD IGNORED: " + command);
//        return;
//    }
//    QTC_ASSERT(m_proc.isRunning(), notifyEngineIll());
//    postDirectCommand(command);
}

void DapEngine::runCommand(const DebuggerCommand &cmd)
{
    if (state() == EngineSetupRequested) { // cmd has been triggered too early
        showMessage("IGNORED COMMAND: " + cmd.function);
        return;
    }
    QTC_ASSERT(m_dapClient->dataProvider()->isRunning(), notifyEngineIll());
//    postDirectCommand(cmd.args.toObject());
//    const QByteArray data = QJsonDocument(cmd.args.toObject()).toJson(QJsonDocument::Compact);
//    m_proc.writeRaw("Content-Length: " + QByteArray::number(data.size()) + "\r\n" + data + "\r\n");

//    showMessage(QString::fromUtf8(data), LogInput);
}

void DapEngine::shutdownInferior()
{
    QTC_ASSERT(state() == InferiorShutdownRequested, qCDebug(dapEngineLog) << state());

    m_dapClient->sendDisconnect();

    qCDebug(dapEngineLog) << "DapEngine::shutdownInferior()";
    notifyInferiorShutdownFinished();
}

void DapEngine::shutdownEngine()
{
    QTC_ASSERT(state() == EngineShutdownRequested, qCDebug(dapEngineLog) << state());

    m_dapClient->sendTerminate();

    qCDebug(dapEngineLog) << "DapEngine::shutdownEngine()";
    m_dapClient->dataProvider()->kill();
}

void DapEngine::handleDapStarted()
{
    QTC_ASSERT(state() == EngineRunRequested, qCDebug(dapEngineLog) << state());

    m_dapClient->sendInitialize();

    qCDebug(dapEngineLog) << "handleDapStarted";
}

void DapEngine::handleDapConfigurationDone()
{
    QTC_ASSERT(state() == EngineRunRequested, qCDebug(dapEngineLog) << state());

    m_dapClient->sendConfigurationDone();

    qCDebug(dapEngineLog) << "handleDapConfigurationDone";
}


void DapEngine::handleDapLaunch()
{
    QTC_ASSERT(state() == EngineRunRequested, qCDebug(dapEngineLog) << state());

    m_dapClient->sendLaunch(runParameters().inferior.command.executable());

    qCDebug(dapEngineLog) << "handleDapLaunch";
}

void DapEngine::interruptInferior()
{
    m_dapClient->sendPause();
}

void DapEngine::executeStepIn(bool)
{
    if (m_currentThreadId == -1)
        return;

    notifyInferiorRunRequested();
    m_dapClient->sendStepIn(m_currentThreadId);
}

void DapEngine::executeStepOut()
{
    if (m_currentThreadId == -1)
        return;

    notifyInferiorRunRequested();
    m_dapClient->sendStepOut(m_currentThreadId);
}

void DapEngine::executeStepOver(bool)
{
    if (m_currentThreadId == -1)
        return;

    notifyInferiorRunRequested();

    m_dapClient->sendStepOver(m_currentThreadId);
}

void DapEngine::continueInferior()
{
    notifyInferiorRunRequested();
    m_dapClient->sendContinue(m_currentThreadId);
}

void DapEngine::executeRunToLine(const ContextData &data)
{
    // Add one-shot breakpoint
    BreakpointParameters bp;
    bp.oneShot = true;
    if (data.address) {
        bp.type = BreakpointByAddress;
        bp.address = data.address;
    } else {
        bp.type = BreakpointByFileAndLine;
        bp.fileName = data.fileName;
        bp.textPosition = data.textPosition;
    }

    BreakpointManager::createBreakpointForEngine(bp, this);
}

void DapEngine::executeRunToFunction(const QString &functionName)
{
    Q_UNUSED(functionName)
    QTC_CHECK("FIXME:  DapEngine::runToFunctionExec()" && false);
}

void DapEngine::executeJumpToLine(const ContextData &data)
{
    Q_UNUSED(data)
    QTC_CHECK("FIXME:  DapEngine::jumpToLineExec()" && false);
}

void DapEngine::activateFrame(int frameIndex)
{
    if (state() != InferiorStopOk && state() != InferiorUnrunnable)
        return;

    StackHandler *handler = stackHandler();
    QTC_ASSERT(frameIndex < handler->stackSize(), return);
    handler->setCurrentIndex(frameIndex);
    gotoLocation(handler->currentFrame());
    updateLocals();
}

void DapEngine::selectThread(const Thread &thread)
{
    Q_UNUSED(thread)
    m_currentThreadId = thread->id().toInt();
    threadsHandler()->setCurrentThread(thread);
    updateLocals();
}

bool DapEngine::acceptsBreakpoint(const BreakpointParameters &) const
{
    return true; // FIXME: Too bold.
}

static QJsonObject createBreakpoint(const Breakpoint &breakpoint)
{
    const BreakpointParameters &params = breakpoint->requestedParameters();

    if (params.fileName.isEmpty())
        return QJsonObject();

    QJsonObject bp;
    bp["line"] = params.textPosition.line;
    bp["source"] = QJsonObject{{"name", params.fileName.fileName()},
                               {"path", params.fileName.path()}};
    return bp;
}


void DapEngine::insertBreakpoint(const Breakpoint &bp)
{
    QTC_ASSERT(bp, return);
    QTC_CHECK(bp->state() == BreakpointInsertionRequested);
    notifyBreakpointInsertProceeding(bp);

    dapInsertBreakpoint(bp);
}

void DapEngine::dapInsertBreakpoint(const Breakpoint &bp)
{
    bp->setResponseId(QString::number(m_nextBreakpointId++));
    const BreakpointParameters &params = bp->requestedParameters();

    QJsonArray breakpoints;
    for (const auto &breakpoint : breakHandler()->breakpoints()) {
        QJsonObject jsonBp = createBreakpoint(breakpoint);
        if (!jsonBp.isEmpty()
            && params.fileName.path() == jsonBp["source"].toObject()["path"].toString()) {
            breakpoints.append(jsonBp);
        }
    }

    m_dapClient->setBreakpoints(breakpoints, params.fileName);

    qCDebug(dapEngineLog) << "insertBreakpoint" << bp->modelId() << bp->responseId();
}

void DapEngine::updateBreakpoint(const Breakpoint &bp)
{
    BreakpointParameters parameters = bp->requestedParameters();
    notifyBreakpointChangeProceeding(bp);
    qDebug() << "updateBreakpoint";

    if (parameters.enabled != bp->isEnabled()) {
        if (bp->isEnabled())
            dapRemoveBreakpoint(bp);
        else
            dapInsertBreakpoint(bp);
    }
}

void DapEngine::removeBreakpoint(const Breakpoint &bp)
{
    QTC_ASSERT(bp, return);
    QTC_CHECK(bp->state() == BreakpointRemoveRequested);
    notifyBreakpointRemoveProceeding(bp);

    dapRemoveBreakpoint(bp);
}

void DapEngine::dapRemoveBreakpoint(const Breakpoint &bp)
{
    const BreakpointParameters &params = bp->requestedParameters();

    QJsonArray breakpoints;
    for (const auto &breakpoint : breakHandler()->breakpoints()) {
        if (breakpoint->responseId() != bp->responseId()
            && params.fileName == breakpoint->requestedParameters().fileName) {
            QJsonObject jsonBp = createBreakpoint(breakpoint);
            breakpoints.append(jsonBp);
        }
    }

    m_dapClient->setBreakpoints(breakpoints, params.fileName);

    qCDebug(dapEngineLog) << "removeBreakpoint" << bp->modelId() << bp->responseId();
}

void DapEngine::loadSymbols(const Utils::FilePath  &/*moduleName*/)
{
}

void DapEngine::loadAllSymbols()
{
}

void DapEngine::reloadModules()
{
    runCommand({"listModules"});
}

void DapEngine::refreshModules(const GdbMi &modules)
{
    ModulesHandler *handler = modulesHandler();
    handler->beginUpdateAll();
    for (const GdbMi &item : modules) {
        Module module;
        module.moduleName = item["name"].data();
        QString path = item["value"].data();
        int pos = path.indexOf("' from '");
        if (pos != -1) {
            path = path.mid(pos + 8);
            if (path.size() >= 2)
                path.chop(2);
        } else if (path.startsWith("<module '")
                   && path.endsWith("' (built-in)>")) {
            path = "(builtin)";
        }
        module.modulePath = FilePath::fromString(path);
        handler->updateModule(module);
    }
    handler->endUpdateAll();
}

void DapEngine::requestModuleSymbols(const Utils::FilePath &/*moduleName*/)
{
//    DebuggerCommand cmd("listSymbols");
//    cmd.arg("module", moduleName);
//    runCommand(cmd);
}

void DapEngine::refreshState(const GdbMi &reportedState)
{
    QString newState = reportedState.data();
    if (newState == "stopped") {
        notifyInferiorSpontaneousStop();
        updateAll();
    } else if (newState == "inferiorexited") {
        notifyInferiorExited();
    }
}

void DapEngine::refreshLocation(const GdbMi &reportedLocation)
{
    StackFrame frame;
    frame.file = FilePath::fromString(reportedLocation["file"].data());
    frame.line = reportedLocation["line"].toInt();
    frame.usable = frame.file.isReadableFile();
    if (state() == InferiorRunOk) {
        showMessage(QString("STOPPED AT: %1:%2").arg(frame.file.toUserOutput()).arg(frame.line));
        gotoLocation(frame);
        notifyInferiorSpontaneousStop();
        updateAll();
    }
}

void DapEngine::refreshSymbols(const GdbMi &symbols)
{
    QString moduleName = symbols["module"].data();
    Symbols syms;
    for (const GdbMi &item : symbols["symbols"]) {
        Symbol symbol;
        symbol.name = item["name"].data();
        syms.append(symbol);
    }
    showModuleSymbols(FilePath::fromString(moduleName), syms);
}

bool DapEngine::canHandleToolTip(const DebuggerToolTipContext &) const
{
    return state() == InferiorStopOk;
}

void DapEngine::assignValueInDebugger(WatchItem *, const QString &/*expression*/, const QVariant &/*value*/)
{
    //DebuggerCommand cmd("assignValue");
    //cmd.arg("expression", expression);
    //cmd.arg("value", value.toString());
    //runCommand(cmd);
    //    postDirectCommand("global " + expression + ';' + expression + "=" + value.toString());
    updateLocals();
}

void DapEngine::updateItem(const QString &iname)
{
    Q_UNUSED(iname)
    updateAll();
}

QString DapEngine::errorMessage(QProcess::ProcessError error) const
{
    switch (error) {
        case QProcess::FailedToStart:
            return Tr::tr("The DAP process failed to start. Either the "
                "invoked program \"%1\" is missing, or you may have insufficient "
                "permissions to invoke the program.")
                .arg(m_dapClient->dataProvider()->executable());
        case QProcess::Crashed:
            return Tr::tr("The DAP process crashed some time after starting "
                "successfully.");
        case QProcess::Timedout:
            return Tr::tr("The last waitFor...() function timed out. "
                "The state of QProcess is unchanged, and you can try calling "
                "waitFor...() again.");
        case QProcess::WriteError:
            return Tr::tr("An error occurred when attempting to write "
                "to the DAP process. For example, the process may not be running, "
                "or it may have closed its input channel.");
        case QProcess::ReadError:
            return Tr::tr("An error occurred when attempting to read from "
                "the DAP process. For example, the process may not be running.");
        default:
            return Tr::tr("An unknown error in the DAP process occurred.") + ' ';
    }
}

void DapEngine::handleDapDone()
{
    if (m_dapClient->dataProvider()->result() == ProcessResult::StartFailed) {
        notifyEngineSetupFailed();
        showMessage("ADAPTER START FAILED");
        ICore::showWarningWithOptions(Tr::tr("Adapter start failed"),
                                      m_dapClient->dataProvider()->exitMessage());
        return;
    }

    const QProcess::ProcessError error = m_dapClient->dataProvider()->error();
    if (error != QProcess::UnknownError) {
        showMessage("HANDLE DAP ERROR");
        if (error != QProcess::Crashed)
            AsynchronousMessageBox::critical(Tr::tr("DAP I/O Error"), errorMessage(error));
        if (error == QProcess::FailedToStart)
            return;
    }
    showMessage(QString("DAP PROCESS FINISHED, status %1, code %2")
                .arg(m_dapClient->dataProvider()->exitStatus()).arg(m_dapClient->dataProvider()->exitCode()));
    notifyEngineSpontaneousShutdown();
}

void DapEngine::readDapStandardError()
{
    QString err = m_dapClient->dataProvider()->readAllStandardError();
    qCDebug(dapEngineLog) << "DAP STDERR:" << err;
    //qWarning() << "Unexpected DAP stderr:" << err;
    showMessage("Unexpected DAP stderr: " + err);
    //handleOutput(err);
}

void DapEngine::handleResponse(DapResponseType type, const QJsonObject &response)
{
    const QString command = response.value("command").toString();

    switch (type) {
    case DapResponseType::ConfigurationDone:
        showMessage("configurationDone", LogDebug);
        qCDebug(dapEngineLog) << "configurationDone success";
        notifyEngineRunAndInferiorRunOk();
        break;
    case DapResponseType::Continue:
        showMessage("continue", LogDebug);
        qCDebug(dapEngineLog) << "continue success";
        notifyInferiorRunOk();
        break;
    case DapResponseType::StackTrace:
        handleStackTraceResponse(response);
        break;
    case DapResponseType::Scopes:
        handleScopesResponse(response);
        break;
    case DapResponseType::Variables: {
        auto variables = response.value("body").toObject().value("variables").toArray();
        refreshLocals(variables);
        break;
    }
    case DapResponseType::StepIn:
    case DapResponseType::StepOut:
    case DapResponseType::StepOver:
        if (response.value("success").toBool()) {
            showMessage(command, LogDebug);
            notifyInferiorRunOk();
        } else {
            notifyInferiorRunFailed();
        }
        break;
    case DapResponseType::DapThreads:
        handleThreadsResponse(response);
        break;

    default:
        showMessage("UNKNOWN RESPONSE:" + command);
    };
}

void DapEngine::handleStackTraceResponse(const QJsonObject &response)
{
    QJsonArray stackFrames = response.value("body").toObject().value("stackFrames").toArray();
    if (stackFrames.isEmpty())
        return;

    QJsonObject stackFrame = stackFrames[0].toObject();
    const FilePath file = FilePath::fromString(
        stackFrame.value("source").toObject().value("path").toString());
    const int line = stackFrame.value("line").toInt();
    qCDebug(dapEngineLog) << "stackTrace success" << file << line;
    gotoLocation(Location(file, line));

    refreshStack(stackFrames);
    m_dapClient->scopes(stackFrame.value("id").toInt());
}

void DapEngine::handleScopesResponse(const QJsonObject &response)
{
    if (!response.value("success").toBool())
        return;

    QJsonArray scopes = response.value("body").toObject().value("scopes").toArray();
    for (const QJsonValueRef &scope : scopes) {
        const QString name = scope.toObject().value("name").toString();
        const int variablesReference = scope.toObject().value("variablesReference").toInt();
        qCDebug(dapEngineLog) << "scoped success" << name << variablesReference;
        if (name == "Locals") { // Fix for several scopes
            watchHandler()->removeAllData();
            watchHandler()->notifyUpdateStarted();
            m_dapClient->variables(variablesReference);
        }
    }
}

void DapEngine::handleThreadsResponse(const QJsonObject &response)
{
    QJsonArray threads = response.value("body").toObject().value("threads").toArray();

    if (threads.isEmpty())
        return;

    ThreadsHandler *handler = threadsHandler();
    for (const QJsonValueRef &thread : threads) {
        ThreadData threadData;
        threadData.id = QString::number(thread.toObject().value("id").toInt());
        threadData.name = thread.toObject().value("name").toString();
        handler->updateThread(threadData);
    }

    if (m_currentThreadId)
        handler->setCurrentThread(threadsHandler()->threadForId(QString::number(m_currentThreadId)));
}

void DapEngine::handleEvent(DapEventType type, const QJsonObject &event)
{
    const QString eventType = event.value("event").toString();
    const QJsonObject body = event.value("body").toObject();
    showMessage(eventType, LogDebug);

    switch (type) {
    case DapEventType::Initialized:
        qCDebug(dapEngineLog) << "initialize success";
        handleDapLaunch();
        handleDapConfigurationDone();
        break;
    case DapEventType::Stopped:
        handleStoppedEvent(event);
        break;
    case DapEventType::Exited:
        notifyInferiorExited();
        break;
    case DapEventType::DapThread:
        m_dapClient->threads();
        if (body.value("reason").toString() == "started" && body.value("threadId").toInt() == 1)
            claimInitialBreakpoints();
        break;
    case DapEventType::DapBreakpoint:
        handleBreakpointEvent(event);
        break;
    case DapEventType::Output: {
        const QString category = body.value("category").toString();
        const QString output = body.value("output").toString();
        if (category == "stdout")
            showMessage(output, AppOutput);
        else if (category == "stderr")
            showMessage(output, AppError);
        else
            showMessage(output, LogDebug);
        break;
    }
    default:
        showMessage("UNKNOWN EVENT:" + eventType);
    };
}

void DapEngine::handleStoppedEvent(const QJsonObject &event)
{
    const QJsonObject body = event.value("body").toObject();
    m_currentThreadId = body.value("threadId").toInt();

    if (body.value("reason").toString() == "breakpoint") {
        QString id = QString::number(body.value("hitBreakpointIds").toArray().first().toInteger());

        Breakpoint bp = breakHandler()->findBreakpointByResponseId(id);
        if (bp) {
            const BreakpointParameters &params = bp->requestedParameters();
            gotoLocation(Location(params.fileName, params.textPosition));
            if (params.oneShot)
                removeBreakpoint(bp);
        }
    }

    if (state() == InferiorStopRequested)
        notifyInferiorStopOk();
    else
        notifyInferiorSpontaneousStop();

    m_dapClient->stackTrace(m_currentThreadId);
    m_dapClient->threads();
}

void DapEngine::handleBreakpointEvent(const QJsonObject &event)
{
    const QJsonObject body = event.value("body").toObject();
    QJsonObject breakpoint = body.value("breakpoint").toObject();

    Breakpoint bp = breakHandler()->findBreakpointByResponseId(
        QString::number(breakpoint.value("id").toInt()));
    qCDebug(dapEngineLog) << "breakpoint id :" << breakpoint.value("id").toInt();

    if (bp) {
        BreakpointParameters parameters = bp->requestedParameters();
        if (parameters.enabled != bp->isEnabled()) {
            parameters.pending = false;
            bp->setParameters(parameters);
            notifyBreakpointChangeOk(bp);
            return;
        }
    }

    if (body.value("reason").toString() == "new") {
        if (breakpoint.value("verified").toBool()) {
            notifyBreakpointInsertOk(bp);
            const BreakpointParameters &params = bp->requestedParameters();
            if (params.oneShot)
                continueInferior();
            qCDebug(dapEngineLog) << "breakpoint inserted";
        } else {
            notifyBreakpointInsertFailed(bp);
            qCDebug(dapEngineLog) << "breakpoint insertion failed";
        }
        return;
    }

    if (body.value("reason").toString() == "removed") {
        if (breakpoint.value("verified").toBool()) {
            notifyBreakpointRemoveOk(bp);
            qCDebug(dapEngineLog) << "breakpoint removed";
        } else {
            notifyBreakpointRemoveFailed(bp);
            qCDebug(dapEngineLog) << "breakpoint remove failed";
        }
        return;
    }
}

void DapEngine::refreshLocals(const QJsonArray &variables)
{
    bool isFirstLayer = m_watchItems.isEmpty();
    for (auto variable : variables) {
        WatchItem *item = new WatchItem;
        const QString name = variable.toObject().value("name").toString();
        item->iname = "local." + name;
        item->name = name;
        item->type = variable.toObject().value("type").toString();
        item->value = variable.toObject().value("value").toString();
        item->address = variable.toObject().value("address").toInt();
        item->type = variable.toObject().value("type").toString();

        qCDebug(dapEngineLog) << "variable" << name << item->hexAddress();
        if (isFirstLayer)
            m_watchItems.append(item);
        else
            m_currentWatchItem->appendChild(item);

        const int variablesReference = variable.toObject().value("variablesReference").toInt();
        if (variablesReference > 0)
            m_variablesReferenceQueue.push({variablesReference, item});
    }

    if (m_variablesReferenceQueue.empty()) {
        for (auto item : m_watchItems)
            watchHandler()->insertItem(item);
        m_watchItems.clear();

        watchHandler()->notifyUpdateFinished();
        return;
    }

    const auto front = m_variablesReferenceQueue.front();
    m_variablesReferenceQueue.pop();

    m_dapClient->variables(front.first);
    m_currentWatchItem = front.second;
}

void DapEngine::refreshStack(const QJsonArray &stackFrames)
{
    StackHandler *handler = stackHandler();
    StackFrames frames;
    for (const auto &value : stackFrames) {
        StackFrame frame;
        QJsonObject item = value.toObject();
        frame.level = item.value("id").toString();
        frame.function = item.value("name").toString();
        frame.line = item.value("line").toInt();
        QJsonObject source = item.value("source").toObject();
        frame.file = FilePath::fromString(source.value("path").toString());
        frame.address = item.value("instructionPointerReference").toInt();
        frame.usable = frame.file.isReadableFile();
        frames.append(frame);
    }
    handler->setFrames(frames, false);

    int index = stackHandler()->firstUsableIndex();
    handler->setCurrentIndex(index);
    if (index >= 0 && index < handler->stackSize())
        gotoLocation(handler->frameAt(index));
}

void DapEngine::reloadFullStack()
{
    updateAll();
}

void DapEngine::updateAll()
{
    runCommand({"stackListFrames"});
    updateLocals();
}

void DapEngine::updateLocals()
{
    m_dapClient->stackTrace(m_currentThreadId);
}

bool DapEngine::hasCapability(unsigned cap) const
{
    return cap & (ReloadModuleCapability
                  | BreakConditionCapability
                  | ShowModuleSymbolsCapability
                  | RunToLineCapability);
}

void DapEngine::claimInitialBreakpoints()
{
    BreakpointManager::claimBreakpointsForEngine(this);
    qCDebug(dapEngineLog) << "claimInitialBreakpoints";
}

void DapEngine::connectDataGeneratorSignals()
{
    if (!m_dapClient)
        return;

    connect(m_dapClient, &DapClient::started, this, &DapEngine::handleDapStarted);
    connect(m_dapClient, &DapClient::done, this, &DapEngine::handleDapDone);
    connect(m_dapClient,
            &DapClient::readyReadStandardError,
            this,
            &DapEngine::readDapStandardError);

    connect(m_dapClient, &DapClient::responseReady, this, &DapEngine::handleResponse);
    connect(m_dapClient, &DapClient::eventReady, this, &DapEngine::handleEvent);
}

DebuggerEngine *createDapEngine(Utils::Id runMode)
{
    if (runMode == ProjectExplorer::Constants::CMAKE_DEBUG_RUN_MODE)
        return new CMakeDapEngine;

    return new GdbDapEngine;
}

} // Debugger::Internal
