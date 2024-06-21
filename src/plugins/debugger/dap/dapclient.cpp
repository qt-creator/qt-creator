// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "dapclient.h"
#include "qjsonarray.h"

#include <utils/qtcprocess.h>

#include <QDebug>
#include <QJsonDocument>
#include <QLoggingCategory>

using namespace Core;
using namespace Utils;

namespace Debugger::Internal {

DapClient::DapClient(IDataProvider *dataProvider, QObject *parent)
    : QObject(parent)
    , m_dataProvider(dataProvider)
{
    connect(m_dataProvider,
            &IDataProvider::readyReadStandardOutput,
            this,
            &DapClient::readOutput);
    connect(m_dataProvider,
            &IDataProvider::readyReadStandardError,
            this,
            &DapClient::readyReadStandardError);

    connect(m_dataProvider, &IDataProvider::done, this, &DapClient::done);
    connect(m_dataProvider, &IDataProvider::started, this, &DapClient::started);
}

DapClient::~DapClient() = default;


void DapClient::postRequest(const QString &command, const QJsonObject &arguments)
{
    static int seq = 1;

    QJsonObject ob = {
        {"command", command},
        {"type", "request"},
        {"seq", seq++},
        {"arguments", arguments}
    };

    const QByteArray data = QJsonDocument(ob).toJson(QJsonDocument::Compact);
    const QByteArray msg = "Content-Length: " + QByteArray::number(data.size()) + "\r\n\r\n" + data;
    qCDebug(logCategory()) << msg;

    m_dataProvider->writeRaw(msg);
}

void DapClient::sendInitialize()
{
    postRequest("initialize", QJsonObject{{"clientID", "QtCreator"}, {"clientName", "QtCreator"}});
}

void DapClient::sendLaunch(const Utils::CommandLine &command)
{
    postRequest("launch",
                QJsonObject{{"noDebug", false},
                            {"program", command.executable().path()},
                            {"args", command.arguments()},
                            {"__restart", ""}});
}

void DapClient::sendAttach()
{
    postRequest("attach",
                QJsonObject{{"__restart", ""}});
}

void DapClient::sendConfigurationDone()
{
    postRequest("configurationDone");
}

void DapClient::sendDisconnect()
{
    postRequest("disconnect", QJsonObject{{"restart", false}, {"terminateDebuggee", true}});
}

void DapClient::sendTerminate()
{
    postRequest("terminate", QJsonObject{{"restart", false}});
}

void DapClient::sendContinue(int threadId)
{
    QTC_ASSERT(threadId != -1, return);
    postRequest("continue", QJsonObject{{"threadId", threadId}});
}

void DapClient::sendPause()
{
    postRequest("pause");
}

void DapClient::sendStepIn(int threadId)
{
    QTC_ASSERT(threadId != -1, return);
    postRequest("stepIn", QJsonObject{{"threadId", threadId}});
}

void DapClient::sendStepOut(int threadId)
{
    QTC_ASSERT(threadId != -1, return);
    postRequest("stepOut", QJsonObject{{"threadId", threadId}});
}

void DapClient::sendStepOver(int threadId)
{
    QTC_ASSERT(threadId != -1, return);
    postRequest("next", QJsonObject{{"threadId", threadId}});
}

void DapClient::evaluateVariable(const QString &expression, int frameId)
{
    postRequest("evaluate",
                QJsonObject{{"expression", expression},
                            {"frameId", frameId},
                            {"context", "variables"}});
}

void DapClient::stackTrace(int threadId)
{
    QTC_ASSERT(threadId != -1, return);
    postRequest("stackTrace",
                QJsonObject{{"threadId", threadId}, {"startFrame", 0}, {"levels", 10}});
}

void DapClient::scopes(int frameId)
{
    postRequest("scopes", QJsonObject{{"frameId", frameId}});
}

void DapClient::threads()
{
    postRequest("threads");
}

void DapClient::variables(int variablesReference)
{
    postRequest("variables", QJsonObject{{"variablesReference", variablesReference}});
}

void DapClient::setBreakpoints(const QJsonArray &breakpoints, const FilePath &fileName)
{
    postRequest("setBreakpoints",
                QJsonObject{{"source", QJsonObject{{"path", fileName.path()}}},
                            {"breakpoints", breakpoints}});
}

void DapClient::setFunctionBreakpoints(const QJsonArray &breakpoints)
{
    postRequest("setFunctionBreakpoints", QJsonObject{{"breakpoints", breakpoints}});
}

void DapClient::readOutput()
{
    m_inbuffer.append(m_dataProvider->readAllStandardOutput());

    qCDebug(logCategory()) << m_inbuffer;

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
        const QJsonDocument doc = QJsonDocument::fromJson(m_inbuffer.mid(pos2, len), &error);

        m_inbuffer = m_inbuffer.mid(pos2 + len);

        emitSignals(doc);
    }
}

void DapClient::emitSignals(const QJsonDocument &doc)
{
    QJsonObject ob = doc.object();
    const QJsonValue t = ob.value("type");
    const QString type = t.toString();

    qCDebug(logCategory()) << "dap response" << ob;

    if (type == "response") {
        DapResponseType type = DapResponseType::Unknown;
        const QString command = ob.value("command").toString();
        if (command == "initialize") {
            type = DapResponseType::Initialize;
            fillCapabilities(ob);
        } else if (command == "configurationDone") {
            type = DapResponseType::ConfigurationDone;
        } else if (command == "continue") {
            type = DapResponseType::Continue;
        } else if (command == "stackTrace") {
            type = DapResponseType::StackTrace;
        } else if (command == "scopes") {
            type = DapResponseType::Scopes;
        } else if (command == "variables") {
            type = DapResponseType::Variables;
        } else if (command == "stepIn") {
            type = DapResponseType::StepIn;
        } else if (command == "stepOut") {
            type = DapResponseType::StepOut;
        } else if (command == "next") {
            type = DapResponseType::StepOver;
        } else if (command == "threads") {
            type = DapResponseType::DapThreads;
        } else if (command == "pause") {
            type = DapResponseType::Pause;
        } else if (command == "evaluate") {
            type = DapResponseType::Evaluate;
        } else if (command == "setBreakpoints") {
            type = DapResponseType::SetBreakpoints;
        } else if (command == "setFunctionBreakpoints") {
            type = DapResponseType::SetFunctionBreakpoints;
        } else if (command == "attach") {
            type = DapResponseType::Attach;
        } else if (command == "launch") {
            type = DapResponseType::Launch;
        }
        emit responseReady(type, ob);
        return;
    }

    if (type == "event") {
        const QString event = ob.value("event").toString();

        DapEventType type = DapEventType::Unknown;
        if (event == "initialized") {
            type = DapEventType::Initialized;
        } else if (event == "stopped") {
            type = DapEventType::Stopped;
        } else if (event == "thread") {
            type = DapEventType::DapThread;
        } else if (event == "output") {
            type = DapEventType::Output;
        } else if (event == "breakpoint") {
            type = DapEventType::DapBreakpoint;
        } else if (event == "exited") {
            type = DapEventType::Exited;
        }
        emit eventReady(type, ob);
    }
}

void DapClient::fillCapabilities(const QJsonObject &response)
{
    QJsonObject body = response.value("body").toObject();

    m_capabilities.supportsConfigurationDoneRequest
        = body.value("supportsConfigurationDoneRequest").toBool();
    m_capabilities.supportsFunctionBreakpoints = body.value("supportsFunctionBreakpoints").toBool();
    m_capabilities.supportsConditionalBreakpoints
        = body.value("supportsConditionalBreakpoints").toBool();
    m_capabilities.supportsHitConditionalBreakpoints
        = body.value("supportsHitConditionalBreakpoints").toBool();
    m_capabilities.supportsEvaluateForHovers = body.value("supportsEvaluateForHovers").toBool();
    m_capabilities.supportsStepBack = body.value("supportsStepBack").toBool();
    m_capabilities.supportsSetVariable = body.value("supportsSetVariable").toBool();
    m_capabilities.supportsRestartFrame = body.value("supportsRestartFrame").toBool();
    m_capabilities.supportsGotoTargetsRequest = body.value("supportsGotoTargetsRequest").toBool();
    m_capabilities.supportsStepInTargetsRequest
        = body.value("supportsStepInTargetsRequest").toBool();
    m_capabilities.supportsCompletionsRequest = body.value("supportsCompletionsRequest").toBool();
    m_capabilities.supportsModulesRequest = body.value("supportsModulesRequest").toBool();
    m_capabilities.supportsRestartRequest = body.value("supportsRestartRequest").toBool();
    m_capabilities.supportsExceptionOptions = body.value("supportsExceptionOptions").toBool();
    m_capabilities.supportsValueFormattingOptions
        = body.value("supportsValueFormattingOptions").toBool();
    m_capabilities.supportsExceptionInfoRequest
        = body.value("supportsExceptionInfoRequest").toBool();
    m_capabilities.supportTerminateDebuggee = body.value("supportTerminateDebuggee").toBool();
    m_capabilities.supportSuspendDebuggee = body.value("supportSuspendDebuggee").toBool();
    m_capabilities.supportsDelayedStackTraceLoading
        = body.value("supportsDelayedStackTraceLoading").toBool();
    m_capabilities.supportsLoadedSourcesRequest
        = body.value("supportsLoadedSourcesRequest").toBool();
    m_capabilities.supportsLogPoints = body.value("supportsLogPoints").toBool();
    m_capabilities.supportsTerminateThreadsRequest
        = body.value("supportsTerminateThreadsRequest").toBool();
    m_capabilities.supportsSetExpression = body.value("supportsSetExpression").toBool();
    m_capabilities.supportsTerminateRequest = body.value("supportsTerminateRequest").toBool();
    m_capabilities.supportsDataBreakpoints = body.value("supportsDataBreakpoints").toBool();
    m_capabilities.supportsReadMemoryRequest = body.value("supportsReadMemoryRequest").toBool();
    m_capabilities.supportsWriteMemoryRequest = body.value("supportsWriteMemoryRequest").toBool();
    m_capabilities.supportsDisassembleRequest = body.value("supportsDisassembleRequest").toBool();
    m_capabilities.supportsCancelRequest = body.value("supportsCancelRequest").toBool();
    m_capabilities.supportsBreakpointLocationsRequest
        = body.value("supportsBreakpointLocationsRequest").toBool();
    m_capabilities.supportsClipboardContext = body.value("supportsClipboardContext").toBool();
    m_capabilities.supportsSteppingGranularity = body.value("supportsSteppingGranularity").toBool();
    m_capabilities.supportsInstructionBreakpoints
        = body.value("supportsInstructionBreakpoints").toBool();
    m_capabilities.supportsExceptionFilterOptions
        = body.value("supportsExceptionFilterOptions").toBool();
    m_capabilities.supportsSingleThreadExecutionRequests
        = body.value("supportsSingleThreadExecutionRequests").toBool();
}

} // namespace Debugger::Internal

