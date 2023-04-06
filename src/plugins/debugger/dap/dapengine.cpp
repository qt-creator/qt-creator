// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "dapengine.h"

#include <debugger/breakhandler.h>
#include <debugger/debuggeractions.h>
#include <debugger/debuggercore.h>
#include <debugger/debuggerdialogs.h>
#include <debugger/debuggerplugin.h>
#include <debugger/debuggerprotocol.h>
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

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <coreplugin/idocument.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagebox.h>

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QTimer>
#include <QVariant>
#include <QJsonArray>
#include <QJsonDocument>
#include <QThread>

using namespace Core;
using namespace Utils;

namespace Debugger::Internal {

DapEngine::DapEngine()
{
    setObjectName("DapEngine");
    setDebuggerName("DAP");
}

void DapEngine::executeDebuggerCommand(const QString &/*command*/)
{
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
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
    qDebug() << msg;

    m_proc.writeRaw(msg);

    showMessage(QString::fromUtf8(msg), LogInput);
}

void DapEngine::runCommand(const DebuggerCommand &cmd)
{
    if (state() == EngineSetupRequested) { // cmd has been triggered too early
        showMessage("IGNORED COMMAND: " + cmd.function);
        return;
    }
    QTC_ASSERT(m_proc.isRunning(), notifyEngineIll());
//    postDirectCommand(cmd.args.toObject());
//    const QByteArray data = QJsonDocument(cmd.args.toObject()).toJson(QJsonDocument::Compact);
//    m_proc.writeRaw("Content-Length: " + QByteArray::number(data.size()) + "\r\n" + data + "\r\n");

//    showMessage(QString::fromUtf8(data), LogInput);
}

void DapEngine::shutdownInferior()
{
    QTC_ASSERT(state() == InferiorShutdownRequested, qDebug() << state());
    postDirectCommand({{"command", "terminate"},
                       {"type", "request"}});

    qDebug() << "DapEngine::shutdownInferior()";
    notifyInferiorShutdownFinished();
}

void DapEngine::shutdownEngine()
{
    QTC_ASSERT(state() == EngineShutdownRequested, qDebug() << state());

    qDebug() << "DapEngine::shutdownEngine()";
    m_proc.kill();
}

void DapEngine::setupEngine()
{
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());

    connect(&m_proc, &QtcProcess::started, this, &DapEngine::handleDabStarted);
    connect(&m_proc, &QtcProcess::done, this, &DapEngine::handleDapDone);
    connect(&m_proc, &QtcProcess::readyReadStandardOutput, this, &DapEngine::readDapStandardOutput);
    connect(&m_proc, &QtcProcess::readyReadStandardError, this, &DapEngine::readDapStandardError);

    const DebuggerRunParameters &rp = runParameters();
    const CommandLine cmd{rp.debugger.command.executable(), {"-i", "dap"}};
    showMessage("STARTING " + cmd.toUserOutput());
    m_proc.setProcessMode(ProcessMode::Writer);
    m_proc.setEnvironment(rp.debugger.environment);
    m_proc.setCommand(cmd);
    m_proc.start();
    notifyEngineRunAndInferiorRunOk();
}

// From the docs:
// The sequence of events/requests is as follows:
//   * adapters sends initialized event (after the initialize request has returned)
//   * client sends zero or more setBreakpoints requests
//   * client sends one setFunctionBreakpoints request
//     (if corresponding capability supportsFunctionBreakpoints is true)
//   * client sends a setExceptionBreakpoints request if one or more exceptionBreakpointFilters
//     have been defined (or if supportsConfigurationDoneRequest is not true)
//   * client sends other future configuration requests
//   * client sends one configurationDone request to indicate the end of the configuration.

void DapEngine::handleDabStarted()
{
    notifyEngineSetupOk();
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state());

//    CHECK_STATE(EngineRunRequested);

    postDirectCommand({
        {"command", "initialize"},
        {"type", "request"},
        {"arguments", QJsonObject {
            {"clientID",  "QtCreator"}, // The ID of the client using this adapter.
            {"clientName",  "QtCreator"} //  The human-readable name of the client using this adapter.
        }}
    });

    qDebug() << "handleDabStarted";
}

void DapEngine::handleDabConfigurationDone()
{
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state());

    //    CHECK_STATE(EngineRunRequested);

    postDirectCommand({{"command", "configurationDone"}, {"type", "request"}});

    qDebug() << "handleDabConfigurationDone";
}


void DapEngine::handleDabLaunch()
{
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state());

    //    CHECK_STATE(EngineRunRequested);

    postDirectCommand(
        {{"command", "launch"},
         {"type", "request"},
//         {"program", runParameters().inferior.command.executable().path()},
         {"arguments",
          QJsonObject{
              {"noDebug", false},
              {"program", runParameters().inferior.command.executable().path()},
              {"__restart", ""}
          }}});
    qDebug() << "handleDabLaunch";
}

void DapEngine::interruptInferior()
{
    postDirectCommand({{"command", "pause"},
                       {"type", "request"}});
}

void DapEngine::executeStepIn(bool)
{
    notifyInferiorRunRequested();

//    postDirectCommand({{"command", "stepIn"},
//                       {"type", "request"},
//                       {"arguments",
//                        QJsonObject{
//                            {"threadId", 1}, // The ID of the client using this adapter.
//                        }}});

    notifyInferiorRunOk();
}

void DapEngine::executeStepOut()
{
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
//    postDirectCommand("return");
}

void DapEngine::executeStepOver(bool)
{
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
//    postDirectCommand("next");
}

void DapEngine::continueInferior()
{
    notifyInferiorRunRequested();
    postDirectCommand({{"command", "continue"},
                       {"type", "request"},
                       {"arguments",
                        QJsonObject{
                            {"threadId", 1}, // The ID of the client using this adapter.
                        }}});
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
    bp["line"] = params.lineNumber;
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

    postDirectCommand(
        {{"command", "setBreakpoints"},
         {"type", "request"},
         {"arguments",
          QJsonObject{{"source", QJsonObject{{"path", params.fileName.path()}}},
                      {"breakpoints", breakpoints}

          }}});

    qDebug() << "insertBreakpoint" << bp->modelId() << bp->responseId();
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

    postDirectCommand({{"command", "setBreakpoints"},
                       {"type", "request"},
                       {"arguments",
                        QJsonObject{{"source", QJsonObject{{"path", params.fileName.path()}}},
                                    {"breakpoints", breakpoints}}}});

    qDebug() << "removeBreakpoint" << bp->modelId() << bp->responseId();
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
                .arg(m_proc.commandLine().executable().toUserOutput());
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
    if (m_proc.result() == ProcessResult::StartFailed) {
        notifyEngineSetupFailed();
        showMessage("ADAPTER START FAILED");
        ICore::showWarningWithOptions(Tr::tr("Adapter start failed"), m_proc.exitMessage());
        return;
    }

    const QProcess::ProcessError error = m_proc.error();
    if (error != QProcess::UnknownError) {
        showMessage("HANDLE DAP ERROR");
        if (error != QProcess::Crashed)
            AsynchronousMessageBox::critical(Tr::tr("DAP I/O Error"), errorMessage(error));
        if (error == QProcess::FailedToStart)
            return;
    }
    showMessage(QString("DAP PROCESS FINISHED, status %1, code %2")
                .arg(m_proc.exitStatus()).arg(m_proc.exitCode()));
    notifyEngineSpontaneousShutdown();
}

void DapEngine::readDapStandardError()
{
    QString err = m_proc.readAllStandardError();
    //qWarning() << "Unexpected DAP stderr:" << err;
    showMessage("Unexpected DAP stderr: " + err);
    //handleOutput(err);
}

void DapEngine::readDapStandardOutput()
{
    m_inbuffer.append(m_proc.readAllStandardOutput().toUtf8());
//    qDebug() << m_inbuffer;

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
    qDebug() << "response" << ob;

    if (type == "response") {
        const QString command = ob.value("command").toString();
        if (command == "configurationDone") {
            showMessage("configurationDone", LogDebug);
            qDebug() << "configurationDone success";
            notifyInferiorRunOk();
            return;
        }

        if (command == "continue") {
            showMessage("continue", LogDebug);
            qDebug() << "continue success";
            notifyInferiorRunOk();
            return;
        }

    }

    if (type == "event") {
        const QString event = ob.value("event").toString();
        const QJsonObject body = ob.value("body").toObject();

        if (event == "output") {
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
        qDebug() << data;

        if (event == "initialized") {
            showMessage(event, LogDebug);
            qDebug() << "initialize success";
            handleDabLaunch();
            handleDabConfigurationDone();
            return;
        }

        if (event == "initialized") {
            showMessage(event, LogDebug);
            return;
        }

        if (event == "stopped") {
            showMessage(event, LogDebug);
            if (body.value("reason").toString() == "breakpoint") {
                QString id = QString::number(body.value("hitBreakpointIds").toArray().first().toInteger());
                const BreakpointParameters &params
                    = breakHandler()->findBreakpointByResponseId(id)->requestedParameters();
                gotoLocation(Location(params.fileName, params.lineNumber));
            }

            if (state() == InferiorStopRequested)
                notifyInferiorStopOk();
            else
                notifyInferiorSpontaneousStop();
            return;
        }

        if (event == "thread") {
            showMessage(event, LogDebug);
            if (body.value("reason").toString() == "started" && body.value("threadId").toInt() == 1)
                claimInitialBreakpoints();
            return;
        }

        if (event == "breakpoint") {
            showMessage(event, LogDebug);
            QJsonObject breakpoint = body.value("breakpoint").toObject();
            Breakpoint bp = breakHandler()->findBreakpointByResponseId(
                QString::number(breakpoint.value("id").toInt()));
            qDebug() << "breakpoint id :" << breakpoint.value("id").toInt();

            if (body.value("reason").toString() == "new") {
                if (breakpoint.value("verified").toBool()) {
//                    bp->setPending(false);
                    notifyBreakpointInsertOk(bp);
                    qDebug() << "breakpoint inserted";
                } else {
                    notifyBreakpointInsertFailed(bp);
                    qDebug() << "breakpoint insertion failed";
                }
                return;
            }

            if (body.value("reason").toString() == "removed") {
                if (breakpoint.value("verified").toBool()) {
                    notifyBreakpointRemoveOk(bp);
                    qDebug() << "breakpoint removed";
                } else {
                    notifyBreakpointRemoveFailed(bp);
                    qDebug() << "breakpoint remove failed";
                }
                return;
            }
            return;
        }


        showMessage("UNKNOWN EVENT:" + event);
        return;
    }

    showMessage("UNKNOWN TYPE:" + type);
}

void DapEngine::refreshLocals(const GdbMi &vars)
{
    WatchHandler *handler = watchHandler();
    handler->resetValueCache();
    handler->insertItems(vars);
    handler->notifyUpdateFinished();

    updateToolTips();
}

void DapEngine::refreshStack(const GdbMi &stack)
{
    StackHandler *handler = stackHandler();
    StackFrames frames;
    for (const GdbMi &item : stack["frames"]) {
        StackFrame frame;
        frame.level = item["level"].data();
        frame.file = FilePath::fromString(item["file"].data());
        frame.function = item["function"].data();
        frame.module = item["function"].data();
        frame.line = item["line"].toInt();
        frame.address = item["address"].toAddress();
        GdbMi usable = item["usable"];
        if (usable.isValid())
            frame.usable = usable.data().toInt();
        else
            frame.usable = frame.file.isReadableFile();
        frames.append(frame);
    }
    bool canExpand = stack["hasmore"].toInt();
    //action(ExpandStack)->setEnabled(canExpand);
    handler->setFrames(frames, canExpand);

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
//    DebuggerCommand cmd("updateData");
//    cmd.arg("nativeMixed", isNativeMixedActive());
//    watchHandler()->appendFormatRequests(&cmd);
//    watchHandler()->appendWatchersAndTooltipRequests(&cmd);

//    const bool alwaysVerbose = qtcEnvironmentVariableIsSet("QTC_DEBUGGER_PYTHON_VERBOSE");
//    cmd.arg("passexceptions", alwaysVerbose);
//    cmd.arg("fancy", debuggerSettings()->useDebuggingHelpers.value());

//    //cmd.arg("resultvarname", m_resultVarName);
//    //m_lastDebuggableCommand = cmd;
//    //m_lastDebuggableCommand.args.replace("\"passexceptions\":0", "\"passexceptions\":1");
//    cmd.arg("frame", stackHandler()->currentIndex());

//    watchHandler()->notifyUpdateStarted();
//    runCommand(cmd);
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
    qDebug() << "claimInitialBreakpoints";
//    const Breakpoints bps = breakHandler()->breakpoints();
//    for (const Breakpoint &bp : bps)
//        qDebug() << "breakpoit: " << bp->fileName() << bp->lineNumber();
//    qDebug() << "claimInitialBreakpoints end";

//    const DebuggerRunParameters &rp = runParameters();
//    if (rp.startMode != AttachToCore) {
//        showStatusMessage(Tr::tr("Setting breakpoints..."));
//        showMessage(Tr::tr("Setting breakpoints..."));
//        BreakpointManager::claimBreakpointsForEngine(this);

//        const DebuggerSettings &s = *debuggerSettings();
//        const bool onAbort = s.breakOnAbort.value();
//        const bool onWarning = s.breakOnWarning.value();
//        const bool onFatal = s.breakOnFatal.value();
//        if (onAbort || onWarning || onFatal) {
//            DebuggerCommand cmd("createSpecialBreakpoints");
//            cmd.arg("breakonabort", onAbort);
//            cmd.arg("breakonwarning", onWarning);
//            cmd.arg("breakonfatal", onFatal);
//            runCommand(cmd);
//        }
//    }

//    // It is ok to cut corners here and not wait for createSpecialBreakpoints()'s
//    // response, as the command is synchronous from Creator's point of view,
//    // and even if it fails (e.g. due to stripped binaries), continuing with
//    // the start up is the best we can do.

//    if (!rp.commandsAfterConnect.isEmpty()) {
//        const QString commands = expand(rp.commandsAfterConnect);
//        for (const QString &command : commands.split('\n'))
//            runCommand({command, NativeCommand});
//    }
}

DebuggerEngine *createDapEngine()
{
    return new DapEngine;
}

} // Debugger::Internal
