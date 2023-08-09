// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "dapengine.h"
#include "cmakedapengine.h"
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
#include <projectexplorer/projecttree.h>
#include <projectexplorer/projectexplorerconstants.h>

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

DapEngine::DapEngine()
{
    m_rootWatchItem = new WatchItem();
    m_currentWatchItem = m_rootWatchItem;
}

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

void DapEngine::postDirectCommand(const QJsonObject &ob)
{
    static int seq = 1;
    QJsonObject obseq = ob;
    obseq.insert("seq", seq++);

    const QByteArray data = QJsonDocument(obseq).toJson(QJsonDocument::Compact);
    const QByteArray msg = "Content-Length: " + QByteArray::number(data.size()) + "\r\n\r\n" + data;
    qCDebug(dapEngineLog) << msg;

    m_dataGenerator->writeRaw(msg);

    showMessage(QString::fromUtf8(msg), LogInput);
}

void DapEngine::runCommand(const DebuggerCommand &cmd)
{
    if (state() == EngineSetupRequested) { // cmd has been triggered too early
        showMessage("IGNORED COMMAND: " + cmd.function);
        return;
    }
    QTC_ASSERT(m_dataGenerator->isRunning(), notifyEngineIll());
//    postDirectCommand(cmd.args.toObject());
//    const QByteArray data = QJsonDocument(cmd.args.toObject()).toJson(QJsonDocument::Compact);
//    m_proc.writeRaw("Content-Length: " + QByteArray::number(data.size()) + "\r\n" + data + "\r\n");

//    showMessage(QString::fromUtf8(data), LogInput);
}

void DapEngine::shutdownInferior()
{
    QTC_ASSERT(state() == InferiorShutdownRequested, qCDebug(dapEngineLog) << state());

    postDirectCommand({
        {"command", "disconnect"},
        {"type", "request"},
        {"arguments", QJsonObject{
            {"restart", false},
            {"terminateDebuggee", true}
        }}
    });

    qCDebug(dapEngineLog) << "DapEngine::shutdownInferior()";
    notifyInferiorShutdownFinished();
}

void DapEngine::shutdownEngine()
{
    QTC_ASSERT(state() == EngineShutdownRequested, qCDebug(dapEngineLog) << state());

    postDirectCommand({
        {"command", "terminate"},
        {"type", "request"},
        {"arguments", QJsonObject{
            {"restart", false}
        }}
    });

    qCDebug(dapEngineLog) << "DapEngine::shutdownEngine()";
    m_dataGenerator->kill();
}

void DapEngine::handleDapStarted()
{
    QTC_ASSERT(state() == EngineRunRequested, qCDebug(dapEngineLog) << state());

    postDirectCommand({
        {"command", "initialize"},
        {"type", "request"},
        {"arguments", QJsonObject {
            {"clientID",  "QtCreator"}, // The ID of the client using this adapter.
            {"clientName",  "QtCreator"} //  The human-readable name of the client using this adapter.
        }}
    });

    qCDebug(dapEngineLog) << "handleDapStarted";
}

void DapEngine::handleDapConfigurationDone()
{
    QTC_ASSERT(state() == EngineRunRequested, qCDebug(dapEngineLog) << state());

    postDirectCommand({
        {"command", "configurationDone"},
        {"type", "request"}
    });

    qCDebug(dapEngineLog) << "handleDapConfigurationDone";
}


void DapEngine::handleDapLaunch()
{
    QTC_ASSERT(state() == EngineRunRequested, qCDebug(dapEngineLog) << state());

    postDirectCommand({
        {"command", "launch"},
        {"type", "request"},
        {"arguments", QJsonObject{
            {"noDebug", false},
            {"program", runParameters().inferior.command.executable().path()},
            {"__restart", ""}
        }}
    });
    qCDebug(dapEngineLog) << "handleDapLaunch";
}

void DapEngine::interruptInferior()
{
    postDirectCommand({
        {"command", "pause"},
        {"type", "request"}
    });
}

void DapEngine::dapStackTrace()
{
    if (m_currentThreadId == -1)
        return;

    postDirectCommand({
        {"command", "stackTrace"},
        {"type", "request"},
        {"arguments", QJsonObject{
            {"threadId", m_currentThreadId},
            {"startFrame", 0},
            {"levels", 10}
        }}
    });
}

void DapEngine::dapScopes(int frameId)
{
    postDirectCommand({
        {"command", "scopes"},
        {"type", "request"},
        {"arguments", QJsonObject{
            {"frameId", frameId}
        }}
    });
}

void DapEngine::threads()
{
    postDirectCommand({
        {"command", "threads"},
        {"type", "request"}
    });
}

void DapEngine::dapVariables(int variablesReference)
{
    postDirectCommand({
        {"command", "variables"},
        {"type", "request"},
        {"arguments", QJsonObject{
            {"variablesReference", variablesReference}
        }}
    });
}

void DapEngine::executeStepIn(bool)
{
    if (m_currentThreadId == -1)
        return;

    notifyInferiorRunRequested();

    postDirectCommand({
        {"command", "stepIn"},
        {"type", "request"},
        {"arguments", QJsonObject{
            {"threadId", m_currentThreadId},
        }}
    });

}

void DapEngine::executeStepOut()
{
    if (m_currentThreadId == -1)
        return;

    notifyInferiorRunRequested();

    postDirectCommand({
        {"command", "stepOut"},
        {"type", "request"},
        {"arguments", QJsonObject{
            {"threadId", m_currentThreadId},
        }}
    });

}

void DapEngine::executeStepOver(bool)
{
    if (m_currentThreadId == -1)
        return;

    notifyInferiorRunRequested();

    postDirectCommand({
        {"command", "next"},
        {"type", "request"},
        {"arguments", QJsonObject{
            {"threadId", m_currentThreadId},
        }}
    });

}

void DapEngine::continueInferior()
{
    notifyInferiorRunRequested();
    postDirectCommand({
        {"command", "continue"},
        {"type", "request"},
        {"arguments", QJsonObject{
            {"threadId", m_currentThreadId},
        }}
    });
}

void DapEngine::executeRunToLine(const ContextData &data)
{
    Q_UNUSED(data)
    QTC_CHECK("FIXME:  DapEngine::runToLineExec()" && false);
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

    const BreakpointParameters &params = bp->requestedParameters();
    bp->setResponseId(QString::number(m_nextBreakpointId++));

    QJsonArray breakpoints;
    for (const auto &breakpoint : breakHandler()->breakpoints()) {
        QJsonObject jsonBp = createBreakpoint(breakpoint);
        if (!jsonBp.isEmpty()
            && params.fileName.path() == jsonBp["source"].toObject()["path"].toString()) {
            breakpoints.append(jsonBp);
        }
    }

    postDirectCommand( {
        {"command", "setBreakpoints"},
        {"type", "request"},
        {"arguments", QJsonObject{
            {"source", QJsonObject{
                {"path", params.fileName.path()}
            }},
            {"breakpoints", breakpoints}
        }}
    });

    notifyBreakpointInsertOk(bp);
    qCDebug(dapEngineLog) << "insertBreakpoint" << bp->modelId() << bp->responseId();
}

void DapEngine::updateBreakpoint(const Breakpoint &bp)
{
    notifyBreakpointChangeProceeding(bp);
//    QTC_ASSERT(bp, return);
//    const BreakpointState state = bp->state();
//    if (QTC_GUARD(state == BreakpointUpdateRequested))
//    if (bp->responseId().isEmpty()) // FIXME postpone update somehow (QTimer::singleShot?)
//        return;

//    // FIXME figure out what needs to be changed (there might be more than enabled state)
//    const BreakpointParameters &requested = bp->requestedParameters();
//    if (requested.enabled != bp->isEnabled()) {
//        if (bp->isEnabled())
//            postDirectCommand("disable " + bp->responseId());
//        else
//            postDirectCommand("enable " + bp->responseId());
//        bp->setEnabled(!bp->isEnabled());
//    }
//    // Pretend it succeeds without waiting for response.
    notifyBreakpointChangeOk(bp);
}

void DapEngine::removeBreakpoint(const Breakpoint &bp)
{
    QTC_ASSERT(bp, return);
    QTC_CHECK(bp->state() == BreakpointRemoveRequested);
    notifyBreakpointRemoveProceeding(bp);

    const BreakpointParameters &params = bp->requestedParameters();

    QJsonArray breakpoints;
    for (const auto &breakpoint : breakHandler()->breakpoints())
        if (breakpoint->responseId() != bp->responseId()
            && params.fileName == breakpoint->requestedParameters().fileName) {
            QJsonObject jsonBp = createBreakpoint(breakpoint);
            breakpoints.append(jsonBp);
        }

    postDirectCommand({
        {"command", "setBreakpoints"},
        {"type", "request"},
        {"arguments", QJsonObject{
            {"source", QJsonObject{
                {"path", params.fileName.path()}
            }},
            {"breakpoints", breakpoints}
        }}
    });

    qCDebug(dapEngineLog) << "removeBreakpoint" << bp->modelId() << bp->responseId();
    notifyBreakpointRemoveOk(bp);
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
                .arg(m_dataGenerator->executable());
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
    if (m_dataGenerator->result() == ProcessResult::StartFailed) {
        notifyEngineSetupFailed();
        showMessage("ADAPTER START FAILED");
        ICore::showWarningWithOptions(Tr::tr("Adapter start failed"), m_dataGenerator->exitMessage());
        return;
    }

    const QProcess::ProcessError error = m_dataGenerator->error();
    if (error != QProcess::UnknownError) {
        showMessage("HANDLE DAP ERROR");
        if (error != QProcess::Crashed)
            AsynchronousMessageBox::critical(Tr::tr("DAP I/O Error"), errorMessage(error));
        if (error == QProcess::FailedToStart)
            return;
    }
    showMessage(QString("DAP PROCESS FINISHED, status %1, code %2")
                .arg(m_dataGenerator->exitStatus()).arg(m_dataGenerator->exitCode()));
    notifyEngineSpontaneousShutdown();
}

void DapEngine::readDapStandardError()
{
    QString err = m_dataGenerator->readAllStandardError();
    qCDebug(dapEngineLog) << "DAP STDERR:" << err;
    //qWarning() << "Unexpected DAP stderr:" << err;
    showMessage("Unexpected DAP stderr: " + err);
    //handleOutput(err);
}

void DapEngine::readDapStandardOutput()
{
    m_inbuffer.append(m_dataGenerator->readAllStandardOutput());

    qCDebug(dapEngineLog) << m_inbuffer;

    while (true) {
        // Something like
        //   Content-Length: 128\r\n
        //   {"type": "event", "event": "output", "body": {"category": "stdout", "output": "...\n"}, "seq": 1}\r\n
        // FIXME: There coud be more than one header line.
        int pos1 = m_inbuffer.indexOf("Content-Length:");
        if (pos1 == -1)
            break;
        pos1 += 15;

        int pos2 = m_inbuffer.indexOf('\n', pos1);
        if (pos2 == -1)
            break;

        const int len = m_inbuffer.mid(pos1, pos2 - pos1).trimmed().toInt();
        if (len < 4)
            break;

        pos2 += 3; // Skip \r\n\r

        if (pos2 + len > m_inbuffer.size())
            break;

        QJsonParseError error;
        const auto doc = QJsonDocument::fromJson(m_inbuffer.mid(pos2, len), &error);

        m_inbuffer = m_inbuffer.mid(pos2 + len);

        handleOutput(doc);
    }
}

void DapEngine::handleOutput(const QJsonDocument &data)
{
    QJsonObject ob = data.object();

    const QJsonValue t = ob.value("type");
    const QString type = t.toString();

    qCDebug(dapEngineLog) << "dap response" << ob;

    if (type == "response") {
        handleResponse(ob);
        return;
    }

    if (type == "event") {
        handleEvent(ob);
        return;
    }

    showMessage("UNKNOWN TYPE:" + type);
}

void DapEngine::handleResponse(const QJsonObject &response)
{
    const QString command = response.value("command").toString();

    if (command == "configurationDone") {
        showMessage("configurationDone", LogDebug);
        qCDebug(dapEngineLog) << "configurationDone success";
        notifyEngineRunAndInferiorRunOk();
        return;
    }

    if (command == "continue") {
        showMessage("continue", LogDebug);
        qCDebug(dapEngineLog) << "continue success";
        notifyInferiorRunOk();
        return;
    }

    if (command == "stackTrace") {
        handleStackTraceResponse(response);
        return;
    }

    if (command == "scopes") {
        handleScopesResponse(response);
        return;
    }

    if (command == "variables") {
        auto variables = response.value("body").toObject().value("variables").toArray();
        refreshLocals(variables);
        return;
    }

    if (command == "stepIn" || command == "stepOut" || command == "next") {
        if (response.value("success").toBool()) {
            showMessage(command, LogDebug);
            notifyInferiorRunOk();
        } else {
            notifyInferiorRunFailed();
        }
        return;
    }

    if (command == "threads") {
        handleThreadsResponse(response);
        return;
    }

    showMessage("UNKNOWN RESPONSE:" + command);
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
    dapScopes(stackFrame.value("id").toInt());
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
            m_rootWatchItem = new WatchItem();
            m_currentWatchItem = m_rootWatchItem;
            watchHandler()->removeAllData();
            watchHandler()->notifyUpdateStarted();
            dapVariables(variablesReference);
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

void DapEngine::handleEvent(const QJsonObject &event)
{
    const QString eventType = event.value("event").toString();
    const QJsonObject body = event.value("body").toObject();
    showMessage(eventType, LogDebug);

    if (eventType == "exited") {
        notifyInferiorExited();
        return;
    }

    if (eventType == "output") {
        const QString category = body.value("category").toString();
        const QString output = body.value("output").toString();
        if (category == "stdout")
            showMessage(output, AppOutput);
        else if (category == "stderr")
            showMessage(output, AppError);
        else
            showMessage(output, LogDebug);
        return;
    }

    if (eventType == "initialized") {
        qCDebug(dapEngineLog) << "initialize success";
        handleDapLaunch();
        handleDapConfigurationDone();
        return;
    }

    if (eventType == "stopped") {
        handleStoppedEvent(event);
        return;
    }

    if (eventType == "thread") {
        threads();
        if (body.value("reason").toString() == "started" && body.value("threadId").toInt() == 1)
            claimInitialBreakpoints();
        return;
    }

    if (eventType == "breakpoint") {
        handleBreakpointEvent(event);
        return;
    }

    showMessage("UNKNOWN EVENT:" + eventType);
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
        }
    }

    if (state() == InferiorStopRequested)
        notifyInferiorStopOk();
    else
        notifyInferiorSpontaneousStop();

    dapStackTrace();
    threads();
}

void DapEngine::handleBreakpointEvent(const QJsonObject &event)
{
    const QJsonObject body = event.value("body").toObject();
    QJsonObject breakpoint = body.value("breakpoint").toObject();

    Breakpoint bp = breakHandler()->findBreakpointByResponseId(
        QString::number(breakpoint.value("id").toInt()));
    qCDebug(dapEngineLog) << "breakpoint id :" << breakpoint.value("id").toInt();

    if (body.value("reason").toString() == "new") {
        if (breakpoint.value("verified").toBool()) {
            notifyBreakpointInsertOk(bp);
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
        m_currentWatchItem->appendChild(item);

        const int variablesReference = variable.toObject().value("variablesReference").toInt();
        if (variablesReference > 0)
            m_variablesReferenceQueue.push({variablesReference, item});
    }

    if (m_variablesReferenceQueue.empty()) {
        for (auto item = m_rootWatchItem->begin(); item != m_rootWatchItem->end(); ++item)
            watchHandler()->insertItem(*item);
        watchHandler()->notifyUpdateFinished();
        return;
    }

    const auto front = m_variablesReferenceQueue.front();
    m_variablesReferenceQueue.pop();

    dapVariables(front.first);
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

void DapEngine::updateAll()
{
    runCommand({"stackListFrames"});
    updateLocals();
}

void DapEngine::updateLocals()
{
}

bool DapEngine::hasCapability(unsigned cap) const
{
    return cap & (ReloadModuleCapability
                  | BreakConditionCapability
                  | ShowModuleSymbolsCapability);
}

void DapEngine::claimInitialBreakpoints()
{
    BreakpointManager::claimBreakpointsForEngine(this);
    qCDebug(dapEngineLog) << "claimInitialBreakpoints";
}

void DapEngine::connectDataGeneratorSignals()
{
    if (!m_dataGenerator)
        return;

    connect(m_dataGenerator.get(), &IDataProvider::started, this, &DapEngine::handleDapStarted);
    connect(m_dataGenerator.get(), &IDataProvider::done, this, &DapEngine::handleDapDone);
    connect(m_dataGenerator.get(),
            &IDataProvider::readyReadStandardOutput,
            this,
            &DapEngine::readDapStandardOutput);
    connect(m_dataGenerator.get(),
            &IDataProvider::readyReadStandardError,
            this,
            &DapEngine::readDapStandardError);
}

DebuggerEngine *createDapEngine(Utils::Id runMode)
{
    if (runMode == ProjectExplorer::Constants::CMAKE_DEBUG_RUN_MODE)
        return new CMakeDapEngine;

    return new GdbDapEngine;
}

} // Debugger::Internal
