// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "acptestserver.h"

#include <acp/acp.h>
#include <acp/acpv2.h>

#include <QDir>
#include <QJsonDocument>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

using namespace Acp;

namespace AcpTestServer {

static SessionModeState makeModeState(const QString &currentModeId)
{
    return SessionModeState()
        .currentModeId(currentModeId)
        .addAvailableMode(SessionMode().id("ask").name("Ask"))
        .addAvailableMode(SessionMode().id("code").name("Code"));
}

Server::Server(const ServerScenario &scenario)
    : m_scenario(scenario)
{
    for (int i = 1; i <= m_scenario.seededSessions; ++i)
        m_sessions.append(QString("session-%1").arg(i));
    m_nextSessionNumber = m_scenario.seededSessions + 1;
}

int Server::run()
{
    noise("startup");
    std::string line;
    while (!m_quitRequested && std::getline(std::cin, line)) {
        QJsonParseError parseError;
        const QJsonDocument doc
            = QJsonDocument::fromJson(QByteArray::fromStdString(line), &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            fprintf(stderr, "acptestserver: ignoring malformed input line\n");
            fflush(stderr);
            continue;
        }
        dispatch(doc.object());
    }
    return 0;
}

void Server::dispatch(const QJsonObject &message)
{
    const QJsonValue id = message.value("id");
    const QString method = message.value("method").toString();
    const QJsonObject params = message.value("params").toObject();

    if (!method.isEmpty() && !id.isUndefined())
        handleRequest(id, method, params);
    else if (!method.isEmpty())
        handleNotification(method, params);
    // else: a response to one of the server's own requests that no one is
    // waiting for anymore (e.g. after a cancelled permission request) - ignore.
}

void Server::handleRequest(const QJsonValue &id, const QString &method, const QJsonObject &params)
{
    if (m_scenario.protocolVersion == 2) {
        if (method == "initialize")
            handleInitializeV2(id, params);
        else if (method == "auth/login")
            handleLogin(id, params);
        else if (method == "auth/logout")
            sendResult(id, QJsonObject());
        else if (method == "session/new")
            handleNewSessionV2(id, params);
        else if (method == "session/list")
            handleListSessions(id, params);
        else if (method == "session/resume")
            handleResumeSession(id, params);
        else if (method == "session/delete")
            handleDeleteSession(id, params);
        else if (method == "session/close")
            handleCloseSession(id, params);
        else if (method == "session/prompt")
            handlePromptV2(id, params);
        else if (method == "session/set_config_option")
            handleSetConfigOption(id, params);
        else if (method == "test/quit") {
            sendResult(id, QJsonObject());
            m_quitRequested = true;
        } else
            sendError(id, ErrorCode::Method_not_found, QString("Unknown method: %1").arg(method));
        return;
    }

    if (method == "initialize")
        handleInitialize(id, params);
    else if (method == "authenticate")
        handleAuthenticate(id, params);
    else if (method == "session/new")
        handleNewSession(id, params);
    else if (method == "session/list")
        handleListSessions(id, params);
    else if (method == "session/load")
        handleLoadSession(id, params);
    else if (method == "session/delete")
        handleDeleteSession(id, params);
    else if (method == "session/close")
        handleCloseSession(id, params);
    else if (method == "session/prompt")
        handlePrompt(id, params);
    else if (method == "session/set_mode")
        handleSetMode(id, params);
    else if (method == "session/set_config_option")
        handleSetConfigOption(id, params);
    else if (method == "test/quit") {
        sendResult(id, QJsonObject());
        m_quitRequested = true;
    } else
        sendError(id, ErrorCode::Method_not_found, QString("Unknown method: %1").arg(method));
}

void Server::handleNotification(const QString &method, const QJsonObject &params)
{
    Q_UNUSED(method)
    Q_UNUSED(params)
    // session/cancel outside of a prompt and unknown notifications are ignored.
}

void Server::handleInitialize(const QJsonValue &id, const QJsonObject &params)
{
    const auto request = fromJson<InitializeRequest>(QJsonValue(params));
    if (!request) {
        sendError(id, ErrorCode::Invalid_params, request.error());
        return;
    }

    InitializeResponse response;
    // A v1-only agent answers with its own latest version when the client
    // requests a newer one.
    response.protocolVersion(m_scenario.protocolVersion >= 0
                                 ? m_scenario.protocolVersion
                                 : std::min(request->protocolVersion(), 1));
    response.agentInfo(Implementation().name("acptestserver").version("1.0"));

    if (m_scenario.requireAuth) {
        response.addAuthMethod(AuthMethodAgent()
                                   .id("test-login")
                                   .name("Test Login")
                                   .description("Authentication method of acptestserver."));
    }

    AgentCapabilities capabilities;
    if (m_scenario.seededSessions > 0) {
        capabilities.loadSession(true);
        capabilities.sessionCapabilities(SessionCapabilities()
                                             .list(SessionListCapabilities())
                                             .delete_(SessionDeleteCapabilities())
                                             .close(SessionCloseCapabilities()));
    }
    response.agentCapabilities(capabilities);

    QJsonObject responseObject = toJson(response);
    if (m_scenario.omitProtocolVersion)
        responseObject.remove("protocolVersion");
    sendResult(id, responseObject);
}

void Server::handleAuthenticate(const QJsonValue &id, const QJsonObject &params)
{
    const auto request = fromJson<AuthenticateRequest>(QJsonValue(params));
    if (!request) {
        sendError(id, ErrorCode::Invalid_params, request.error());
        return;
    }
    if (request->methodId() != QLatin1String("test-login")) {
        sendError(id, ErrorCode::Invalid_params,
                  QString("Unknown authentication method: %1").arg(request->methodId()));
        return;
    }
    m_authenticated = true;
    sendResult(id, toJson(AuthenticateResponse()));
}

void Server::handleNewSession(const QJsonValue &id, const QJsonObject &params)
{
    const auto request = fromJson<NewSessionRequest>(QJsonValue(params));
    if (!request) {
        sendError(id, ErrorCode::Invalid_params, request.error());
        return;
    }
    if (m_scenario.requireAuth && !m_authenticated) {
        sendError(id, ErrorCode::Authentication_required, "Authentication required");
        return;
    }

    const QString sessionId = QString("session-%1").arg(m_nextSessionNumber++);
    m_sessions.append(sessionId);

    NewSessionResponse response;
    response.sessionId(sessionId);
    if (m_scenario.configOptions)
        response.configOptions(configOptionsJson());
    if (m_scenario.modes)
        response.modes(makeModeState(m_currentModeId));
    sendResult(id, toJson(response));
}

void Server::handleListSessions(const QJsonValue &id, const QJsonObject &params)
{
    const auto request = fromJson<ListSessionsRequest>(QJsonValue(params));
    if (!request) {
        sendError(id, ErrorCode::Invalid_params, request.error());
        return;
    }

    const int pageSize = 2;
    int start = 0;
    if (const std::optional<QString> &cursor = request->cursor())
        start = cursor->toInt();

    ListSessionsResponse response;
    const QString cwd = QDir::toNativeSeparators(QDir::currentPath());
    for (int i = start; i < m_sessions.size() && i < start + pageSize; ++i) {
        response.addSession(SessionInfo()
                                .sessionId(m_sessions.at(i))
                                .cwd(cwd)
                                .title(QString("Test session %1").arg(m_sessions.at(i))));
    }
    if (start + pageSize < m_sessions.size())
        response.nextCursor(QString::number(start + pageSize));
    sendResult(id, toJson(response));
}

void Server::handleLoadSession(const QJsonValue &id, const QJsonObject &params)
{
    const auto request = fromJson<LoadSessionRequest>(QJsonValue(params));
    if (!request) {
        sendError(id, ErrorCode::Invalid_params, request.error());
        return;
    }
    if (!m_sessions.contains(request->sessionId())) {
        sendError(id, ErrorCode::Resource_not_found,
                  QString("Unknown session: %1").arg(request->sessionId()));
        return;
    }

    sendAgentMessageChunk(request->sessionId(), "replayed history");

    LoadSessionResponse response;
    if (m_scenario.configOptions)
        response.configOptions(configOptionsJson());
    if (m_scenario.modes)
        response.modes(makeModeState(m_currentModeId));
    sendResult(id, toJson(response));
}

void Server::handleDeleteSession(const QJsonValue &id, const QJsonObject &params)
{
    const auto request = fromJson<DeleteSessionRequest>(QJsonValue(params));
    if (!request) {
        sendError(id, ErrorCode::Invalid_params, request.error());
        return;
    }
    if (!m_sessions.removeOne(request->sessionId())) {
        sendError(id, ErrorCode::Resource_not_found,
                  QString("Unknown session: %1").arg(request->sessionId()));
        return;
    }
    sendResult(id, toJson(DeleteSessionResponse()));
}

void Server::handleCloseSession(const QJsonValue &id, const QJsonObject &params)
{
    const auto request = fromJson<CloseSessionRequest>(QJsonValue(params));
    if (!request) {
        sendError(id, ErrorCode::Invalid_params, request.error());
        return;
    }
    sendResult(id, toJson(CloseSessionResponse()));
}

// The same wire shape serves protocol v1 and v2.
static QJsonObject elicitationParams(const QString &sessionId, const QString &mode)
{
    if (mode == QLatin1String("url")) {
        return QJsonObject{{"sessionId", sessionId},
                           {"message", "Log in externally"},
                           {"mode", "url"},
                           {"url", "https://example.com/login"},
                           {"elicitationId", "elic-1"}};
    }
    if (mode == QLatin1String("unknown")) {
        return QJsonObject{{"sessionId", sessionId},
                           {"message", "Future input mode"},
                           {"mode", "_test_future_mode"}};
    }
    return QJsonObject{
        {"sessionId", sessionId},
        {"message", "Please enter your name"},
        {"mode", "form"},
        {"requestedSchema",
         QJsonObject{{"type", "object"},
                     {"properties",
                      QJsonObject{{"name", QJsonObject{{"type", "string"},
                                                       {"title", "Your name"}}}}},
                     {"required", QJsonArray{"name"}}}}};
}

// The word the agent echoes back for a given elicitation answer, so tests can
// observe which response the client sent.
static QString elicitationOutcome(const QJsonObject &answer)
{
    if (answer.contains("error"))
        return "error";
    const QJsonObject result = answer.value("result").toObject();
    const QString action = result.value("action").toString();
    if (action == QLatin1String("accept")) {
        const QJsonObject content = result.value("content").toObject();
        return content.contains("name") ? content.value("name").toString()
                                        : QString("accepted");
    }
    return action;
}

void Server::handlePrompt(const QJsonValue &id, const QJsonObject &params)
{
    if (m_scenario.crashOnPrompt) {
        fprintf(stderr, "acptestserver: simulated crash\n");
        fflush(stderr);
        std::exit(1);
    }

    const auto request = fromJson<PromptRequest>(QJsonValue(params));
    if (!request) {
        sendError(id, ErrorCode::Invalid_params, request.error());
        return;
    }
    const QString sessionId = request->sessionId();

    if (m_scenario.invalidResponse) {
        writeRawLine("this is not json");
        sendResult(id, toJson(PromptResponse().stopReason(StopReason::end_turn)));
        return;
    }

    QString promptText;
    for (const ContentBlock &block : request->prompt()) {
        if (const auto *text = std::get_if<TextContent>(&block)) {
            promptText = text->text();
            break;
        }
    }

    if (m_scenario.allUpdateKinds) {
        const auto sendUpdate = [&](const SessionUpdate::Variant &value, const QString &kind) {
            SessionUpdate update;
            update._value = value;
            update._kind = kind;
            sendNotification("session/update",
                             toJson(SessionNotification().sessionId(sessionId).update(update)));
        };
        sendUpdate(Plan().addEntry(PlanEntry()
                                       .content("first step")
                                       .priority(PlanEntryPriority::high)
                                       .status(PlanEntryStatus::pending)),
                   "plan");
        sendUpdate(ToolCall()
                       .toolCallId("tool-1")
                       .title("Reading a file")
                       .kind(ToolKind::read)
                       .status(ToolCallStatus::in_progress),
                   "tool_call");
        sendUpdate(ToolCallUpdate().toolCallId("tool-1").status(ToolCallStatus::completed),
                   "tool_call_update");
        sendUpdate(ContentChunk().content(TextContent().text("all kinds")),
                   "agent_message_chunk");
        sendUpdate(AvailableCommandsUpdate().addAvailableCommand(
                       AvailableCommand().name("test_command").description("A test command.")),
                   "available_commands_update");
        sendUpdate(UsageUpdate().used(100).size(1000), "usage_update");
        sendResult(id, toJson(PromptResponse().stopReason(StopReason::end_turn)));
        return;
    }

    sendAgentMessageChunk(sessionId, QString("chunk-1:%1").arg(promptText));

    if (m_scenario.permission) {
        const qint64 permissionId = m_nextOutgoingId++;
        QJsonObject requestMessage;
        requestMessage["jsonrpc"] = "2.0";
        requestMessage["id"] = static_cast<double>(permissionId);
        requestMessage["method"] = "session/request_permission";
        requestMessage["params"] = toJson(
            RequestPermissionRequest()
                .sessionId(sessionId)
                .toolCall(ToolCallUpdate().toolCallId("tool-perm").title("Dangerous operation"))
                .addOption(PermissionOption()
                               .optionId("allow")
                               .name("Allow")
                               .kind(PermissionOptionKind::allow_once))
                .addOption(PermissionOption()
                               .optionId("reject")
                               .name("Reject")
                               .kind(PermissionOptionKind::reject_once)));
        writeLine(requestMessage);

        const auto answer = readUntil([permissionId](const QJsonObject &message) {
            if (message.value("method").toString() == QLatin1String("session/cancel"))
                return true;
            return !message.contains("method")
                   && message.value("id").toDouble() == permissionId;
        });
        if (!answer)
            return; // stdin closed

        if (answer->value("method").toString() == QLatin1String("session/cancel")) {
            // The user cancelled the whole prompt. Withdraw the pending
            // permission request, then finish the turn as cancelled.
            sendNotification("$/cancel_request",
                             toJson(CancelRequestNotification().requestId(
                                 RequestId(static_cast<int>(permissionId)))));
            sendResult(id, toJson(PromptResponse().stopReason(StopReason::cancelled)));
            return;
        }

        const auto response = fromJson<RequestPermissionResponse>(answer->value("result"));
        const bool selected = response && response->outcome().kind() == QLatin1String("selected");
        if (selected) {
            sendAgentMessageChunk(sessionId, QString("chunk-2:%1").arg(promptText));
            sendResult(id, toJson(PromptResponse().stopReason(StopReason::end_turn)));
        } else {
            sendResult(id, toJson(PromptResponse().stopReason(StopReason::cancelled)));
        }
        return;
    }

    if (!m_scenario.elicitation.isEmpty()) {
        const qint64 elicitationId = m_nextOutgoingId++;
        QJsonObject requestMessage;
        requestMessage["jsonrpc"] = "2.0";
        requestMessage["id"] = static_cast<double>(elicitationId);
        requestMessage["method"] = "elicitation/create";
        requestMessage["params"] = elicitationParams(sessionId, m_scenario.elicitation);
        writeLine(requestMessage);
        if (m_scenario.elicitation == QLatin1String("url")) {
            sendNotification("elicitation/complete",
                             QJsonObject{{"elicitationId", "elic-1"}});
        }

        const auto answer = readUntil([elicitationId](const QJsonObject &message) {
            if (message.value("method").toString() == QLatin1String("session/cancel"))
                return true;
            return !message.contains("method")
                   && message.value("id").toDouble() == elicitationId;
        });
        if (!answer)
            return; // stdin closed

        if (answer->value("method").toString() == QLatin1String("session/cancel")) {
            sendNotification("$/cancel_request",
                             toJson(CancelRequestNotification().requestId(
                                 RequestId(static_cast<int>(elicitationId)))));
            sendResult(id, toJson(PromptResponse().stopReason(StopReason::cancelled)));
            return;
        }

        sendAgentMessageChunk(sessionId,
                              QString("elicited:%1").arg(elicitationOutcome(*answer)));
        sendResult(id, toJson(PromptResponse().stopReason(StopReason::end_turn)));
        return;
    }

    if (m_scenario.waitForCancel) {
        const auto cancel = readUntil([](const QJsonObject &message) {
            return message.value("method").toString() == QLatin1String("session/cancel");
        });
        if (!cancel)
            return; // stdin closed
        sendResult(id, toJson(PromptResponse().stopReason(StopReason::cancelled)));
        return;
    }

    for (int i = 2; i <= m_scenario.chunks; ++i)
        sendAgentMessageChunk(sessionId, QString("chunk-%1:%2").arg(i).arg(promptText));
    sendResult(id, toJson(PromptResponse().stopReason(StopReason::end_turn)));
}

void Server::handleSetMode(const QJsonValue &id, const QJsonObject &params)
{
    const auto request = fromJson<SetSessionModeRequest>(QJsonValue(params));
    if (!request) {
        sendError(id, ErrorCode::Invalid_params, request.error());
        return;
    }
    if (request->modeId() != QLatin1String("ask") && request->modeId() != QLatin1String("code")) {
        sendError(id, ErrorCode::Invalid_params,
                  QString("Unknown mode: %1").arg(request->modeId()));
        return;
    }
    m_currentModeId = request->modeId();
    sendResult(id, toJson(SetSessionModeResponse()));

    SessionUpdate update;
    update._value = CurrentModeUpdate().currentModeId(m_currentModeId);
    update._kind = "current_mode_update";
    sendNotification("session/update",
                     toJson(SessionNotification().sessionId(request->sessionId()).update(update)));
}

void Server::handleSetConfigOption(const QJsonValue &id, const QJsonObject &params)
{
    const auto request = fromJson<SetSessionConfigOptionRequest>(QJsonValue(params));
    if (!request) {
        sendError(id, ErrorCode::Invalid_params, request.error());
        return;
    }
    const QJsonValue value = request->additionalProperties().value("value");
    if (request->configId() == QLatin1String("test.autoApprove")) {
        m_autoApprove = value.toBool();
    } else if (request->configId() == QLatin1String("test.model")) {
        m_model = value.toString();
    } else {
        sendError(id, ErrorCode::Invalid_params,
                  QString("Unknown config option: %1").arg(request->configId()));
        return;
    }

    const QJsonArray options = m_scenario.protocolVersion == 2 ? configOptionsJsonV2()
                                                               : configOptionsJson();
    QJsonObject result;
    result["configOptions"] = options;
    sendResult(id, result);

    if (m_scenario.protocolVersion == 2) {
        sendSessionUpdateV2(request->sessionId(),
                            QJsonObject{{"sessionUpdate", "config_option_update"},
                                        {"configOptions", options}});
        return;
    }

    SessionUpdate update;
    ConfigOptionUpdate configUpdate;
    for (const QJsonValue &option : options) {
        if (const auto parsed = fromJson<SessionConfigOption>(option))
            configUpdate.addConfigOption(*parsed);
    }
    update._value = configUpdate;
    update._kind = "config_option_update";
    sendNotification("session/update",
                     toJson(SessionNotification().sessionId(request->sessionId()).update(update)));
}

// --- v2 mode ------------------------------------------------------------------

void Server::handleInitializeV2(const QJsonValue &id, const QJsonObject &params)
{
    const auto request = V2::fromJson<V2::InitializeRequest>(QJsonValue(params));
    if (!request) {
        sendError(id, ErrorCode::Invalid_params, request.error());
        return;
    }

    QJsonObject session;
    session["prompt"] = QJsonObject{{"image", QJsonObject()},
                                    {"embeddedContext", QJsonObject()}};
    if (m_scenario.seededSessions > 0)
        session["delete"] = QJsonObject();

    QJsonObject result;
    result["protocolVersion"] = 2;
    result["info"] = QJsonObject{{"name", "acptestserver"}, {"version", "1.0"}};
    result["capabilities"] = QJsonObject{{"session", session}};
    if (m_scenario.requireAuth) {
        result["authMethods"] = QJsonArray{QJsonObject{{"type", "agent"},
                                                       {"methodId", "test-login"},
                                                       {"name", "Test Login"}}};
    }
    sendResult(id, result);
}

void Server::handleLogin(const QJsonValue &id, const QJsonObject &params)
{
    const QString methodId = params.value("methodId").toString();
    if (methodId != QLatin1String("test-login")) {
        sendError(id, ErrorCode::Invalid_params,
                  QString("Unknown authentication method: %1").arg(methodId));
        return;
    }
    m_authenticated = true;
    sendResult(id, QJsonObject());
}

void Server::handleNewSessionV2(const QJsonValue &id, const QJsonObject &params)
{
    const auto request = V2::fromJson<V2::NewSessionRequest>(QJsonValue(params));
    if (!request) {
        sendError(id, ErrorCode::Invalid_params, request.error());
        return;
    }
    if (m_scenario.requireAuth && !m_authenticated) {
        sendError(id, ErrorCode::Authentication_required, "Authentication required");
        return;
    }

    const QString sessionId = QString("session-%1").arg(m_nextSessionNumber++);
    m_sessions.append(sessionId);

    QJsonObject result;
    result["sessionId"] = sessionId;
    if (m_scenario.configOptions)
        result["configOptions"] = configOptionsJsonV2();
    sendResult(id, result);
}

void Server::handleResumeSession(const QJsonValue &id, const QJsonObject &params)
{
    const auto request = V2::fromJson<V2::ResumeSessionRequest>(QJsonValue(params));
    if (!request) {
        sendError(id, ErrorCode::Invalid_params, request.error());
        return;
    }
    if (!m_sessions.contains(request->sessionId())) {
        sendError(id, ErrorCode::Resource_not_found,
                  QString("Unknown session: %1").arg(request->sessionId()));
        return;
    }

    // With a replay cursor, history is replayed as ordinary session updates
    // before the response.
    if (request->replayFrom().has_value()) {
        sendSessionUpdateV2(request->sessionId(),
                            QJsonObject{{"sessionUpdate", "user_message"},
                                        {"messageId", "replay-user-1"},
                                        {"content",
                                         QJsonArray{QJsonObject{{"type", "text"},
                                                                {"text", "replayed prompt"}}}}});
        sendAgentMessageChunkV2(request->sessionId(), "replay-agent-1", "replayed history");
    }

    QJsonObject result;
    if (m_scenario.configOptions)
        result["configOptions"] = configOptionsJsonV2();
    sendResult(id, result);
}

void Server::handlePromptV2(const QJsonValue &id, const QJsonObject &params)
{
    const auto request = V2::fromJson<V2::PromptRequest>(QJsonValue(params));
    if (!request) {
        sendError(id, ErrorCode::Invalid_params, request.error());
        return;
    }
    const QString sessionId = request->sessionId();

    QString promptText;
    for (const V2::ContentBlock &block : request->prompt()) {
        if (const auto *text = std::get_if<V2::TextContent>(&block)) {
            promptText = text->text();
            break;
        }
    }

    // The response only acknowledges acceptance; the turn runs on afterwards.
    sendResult(id, QJsonObject());
    sendStateUpdate(sessionId, "running");

    const QString messageId = QString("msg-%1").arg(++m_messageCounter);

    if (m_scenario.permission) {
        sendAgentMessageChunkV2(sessionId, messageId, QString("chunk-1:%1").arg(promptText));

        const qint64 permissionId = m_nextOutgoingId++;
        QJsonObject requestMessage;
        requestMessage["jsonrpc"] = "2.0";
        requestMessage["id"] = static_cast<double>(permissionId);
        requestMessage["method"] = "session/request_permission";
        requestMessage["params"] = QJsonObject{
            {"sessionId", sessionId},
            {"title", "Allow test tool?"},
            {"subject", QJsonObject{{"type", "tool_call"},
                                    {"toolCall", QJsonObject{{"toolCallId", "tool-perm"},
                                                             {"title", "Dangerous operation"}}}}},
            {"options", QJsonArray{QJsonObject{{"optionId", "allow"},
                                               {"name", "Allow"},
                                               {"kind", "allow_once"}},
                                   QJsonObject{{"optionId", "reject"},
                                               {"name", "Reject"},
                                               {"kind", "reject_once"}}}}};
        writeLine(requestMessage);

        const auto answer = readUntil([permissionId](const QJsonObject &message) {
            if (message.value("method").toString() == QLatin1String("session/cancel"))
                return true;
            return !message.contains("method")
                   && message.value("id").toDouble() == permissionId;
        });
        if (!answer)
            return; // stdin closed

        if (answer->value("method").toString() == QLatin1String("session/cancel")) {
            sendNotification("$/cancel_request",
                             toJson(CancelRequestNotification().requestId(
                                 RequestId(static_cast<int>(permissionId)))));
            sendStateUpdate(sessionId, "idle", "cancelled");
            return;
        }

        const QJsonObject outcome
            = answer->value("result").toObject().value("outcome").toObject();
        if (outcome.value("outcome").toString() == QLatin1String("selected")) {
            sendSessionUpdateV2(sessionId,
                                QJsonObject{{"sessionUpdate", "tool_call_update"},
                                            {"toolCallId", "tool-perm"},
                                            {"status", "completed"}});
            sendAgentMessageChunkV2(sessionId, messageId, QString("chunk-2:%1").arg(promptText));
            sendStateUpdate(sessionId, "idle", "end_turn");
        } else {
            sendStateUpdate(sessionId, "idle", "cancelled");
        }
        return;
    }

    if (!m_scenario.elicitation.isEmpty()) {
        const qint64 elicitationId = m_nextOutgoingId++;
        QJsonObject requestMessage;
        requestMessage["jsonrpc"] = "2.0";
        requestMessage["id"] = static_cast<double>(elicitationId);
        requestMessage["method"] = "elicitation/create";
        requestMessage["params"] = elicitationParams(sessionId, m_scenario.elicitation);
        writeLine(requestMessage);
        if (m_scenario.elicitation == QLatin1String("url")) {
            sendNotification("elicitation/complete",
                             QJsonObject{{"elicitationId", "elic-1"}});
        }

        const auto answer = readUntil([elicitationId](const QJsonObject &message) {
            if (message.value("method").toString() == QLatin1String("session/cancel"))
                return true;
            return !message.contains("method")
                   && message.value("id").toDouble() == elicitationId;
        });
        if (!answer)
            return; // stdin closed

        if (answer->value("method").toString() == QLatin1String("session/cancel")) {
            sendNotification("$/cancel_request",
                             toJson(CancelRequestNotification().requestId(
                                 RequestId(static_cast<int>(elicitationId)))));
            sendStateUpdate(sessionId, "idle", "cancelled");
            return;
        }

        sendAgentMessageChunkV2(sessionId, messageId,
                                QString("elicited:%1").arg(elicitationOutcome(*answer)));
        sendStateUpdate(sessionId, "idle", "end_turn");
        return;
    }

    if (m_scenario.waitForCancel) {
        sendAgentMessageChunkV2(sessionId, messageId, QString("chunk-1:%1").arg(promptText));
        const auto cancel = readUntil([](const QJsonObject &message) {
            return message.value("method").toString() == QLatin1String("session/cancel");
        });
        if (!cancel)
            return; // stdin closed
        sendStateUpdate(sessionId, "idle", "cancelled");
        return;
    }

    for (int i = 1; i <= m_scenario.chunks; ++i)
        sendAgentMessageChunkV2(sessionId, messageId, QString("chunk-%1:%2").arg(i).arg(promptText));

    sendSessionUpdateV2(sessionId, QJsonObject{{"sessionUpdate", "tool_call_update"},
                                               {"toolCallId", "tool-1"},
                                               {"title", "Reading a file"},
                                               {"kind", "read"},
                                               {"status", "in_progress"}});
    sendSessionUpdateV2(sessionId, QJsonObject{{"sessionUpdate", "tool_call_update"},
                                               {"toolCallId", "tool-1"},
                                               {"status", "completed"}});
    sendSessionUpdateV2(sessionId,
                        QJsonObject{{"sessionUpdate", "plan_update"},
                                    {"plan", QJsonObject{{"planId", "plan-1"},
                                                         {"entries",
                                                          QJsonArray{QJsonObject{
                                                              {"content", "first step"},
                                                              {"priority", "high"},
                                                              {"status", "pending"}}}}}}});
    if (m_scenario.emitUnknownUpdate)
        sendSessionUpdateV2(sessionId, QJsonObject{{"sessionUpdate", "_test_future_kind"},
                                                   {"x", 1}});
    sendStateUpdate(sessionId, "idle", "end_turn");
}

QJsonArray Server::configOptionsJson() const
{
    const QJsonObject autoApprove
        = toJson(SessionConfigOption()
                     .id("test.autoApprove")
                     .name("Auto approve")
                     .additionalProperties("type", "boolean")
                     .additionalProperties("currentValue", m_autoApprove));
    const QJsonObject model
        = toJson(SessionConfigOption()
                     .id("test.model")
                     .name("Model")
                     .category(SessionConfigOptionCategory::model)
                     .additionalProperties("type", "select")
                     .additionalProperties("currentValue", m_model)
                     .additionalProperties(
                         "options",
                         QJsonArray{toJson(SessionConfigSelectOption().value("small").name("Small")),
                                    toJson(SessionConfigSelectOption().value("big").name("Big"))}));
    return QJsonArray{autoApprove, model};
}

QJsonArray Server::configOptionsJsonV2() const
{
    const QJsonObject autoApprove
        = V2::toJson(V2::SessionConfigOption()
                         .configId("test.autoApprove")
                         .name("Auto approve")
                         .additionalProperties("type", "boolean")
                         .additionalProperties("currentValue", m_autoApprove));
    const QJsonObject model
        = V2::toJson(V2::SessionConfigOption()
                         .configId("test.model")
                         .name("Model")
                         .category(V2::SessionConfigOptionCategory::model)
                         .additionalProperties("type", "select")
                         .additionalProperties("currentValue", m_model)
                         .additionalProperties(
                             "options",
                             QJsonArray{QJsonObject{{"value", "small"}, {"name", "Small"}},
                                        QJsonObject{{"value", "big"}, {"name", "Big"}}}));
    return QJsonArray{autoApprove, model};
}

void Server::sendSessionUpdateV2(const QString &sessionId, const QJsonObject &update)
{
    sendNotification("session/update",
                     QJsonObject{{"sessionId", sessionId}, {"update", update}});
}

void Server::sendAgentMessageChunkV2(const QString &sessionId, const QString &messageId,
                                     const QString &text)
{
    sendSessionUpdateV2(sessionId,
                        QJsonObject{{"sessionUpdate", "agent_message_chunk"},
                                    {"messageId", messageId},
                                    {"content", QJsonObject{{"type", "text"}, {"text", text}}}});
}

void Server::sendStateUpdate(const QString &sessionId, const QString &state,
                             const QString &stopReason)
{
    QJsonObject update{{"sessionUpdate", "state_update"}, {"state", state}};
    if (!stopReason.isEmpty())
        update["stopReason"] = stopReason;
    sendSessionUpdateV2(sessionId, update);
}

void Server::sendResult(const QJsonValue &id, const QJsonObject &result)
{
    QJsonObject message;
    message["jsonrpc"] = "2.0";
    message["id"] = id;
    message["result"] = result;
    noise("response");
    writeLine(message);
}

void Server::sendError(const QJsonValue &id, int code, const QString &errorMessage)
{
    QJsonObject message;
    message["jsonrpc"] = "2.0";
    message["id"] = id;
    message["error"] = toJson(Error().code(code).message(errorMessage));
    noise("error response");
    writeLine(message);
}

void Server::sendNotification(const QString &method, const QJsonObject &params)
{
    QJsonObject message;
    message["jsonrpc"] = "2.0";
    message["method"] = method;
    message["params"] = params;
    writeLine(message);
}

void Server::sendAgentMessageChunk(const QString &sessionId, const QString &text)
{
    SessionUpdate update;
    update._value = ContentChunk().content(TextContent().text(text));
    update._kind = "agent_message_chunk";
    sendNotification("session/update",
                     toJson(SessionNotification().sessionId(sessionId).update(update)));
}

void Server::writeLine(const QJsonObject &message)
{
    writeRawLine(QJsonDocument(message).toJson(QJsonDocument::Compact));
}

void Server::writeRawLine(const QByteArray &line)
{
    std::cout << line.toStdString() << '\n' << std::flush;
}

void Server::noise(const char *context)
{
    if (!m_scenario.stderrNoise)
        return;
    fprintf(stderr, "acptestserver noise: %s\n", context);
    fflush(stderr);
}

std::optional<QJsonObject> Server::readUntil(const std::function<bool(const QJsonObject &)> &pred)
{
    std::string line;
    while (std::getline(std::cin, line)) {
        QJsonParseError parseError;
        const QJsonDocument doc
            = QJsonDocument::fromJson(QByteArray::fromStdString(line), &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject())
            continue;
        const QJsonObject message = doc.object();
        if (pred(message))
            return message;
        dispatch(message);
    }
    return {};
}

} // namespace AcpTestServer
