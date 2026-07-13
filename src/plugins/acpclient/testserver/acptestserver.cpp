// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "acptestserver.h"

#include <acp/acp.h>

#include <QDir>
#include <QJsonDocument>

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
    response.protocolVersion(request->protocolVersion());
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

    sendResult(id, toJson(response));
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

    QJsonObject result;
    result["configOptions"] = configOptionsJson();
    sendResult(id, result);

    SessionUpdate update;
    ConfigOptionUpdate configUpdate;
    const QJsonArray options = configOptionsJson();
    for (const QJsonValue option : options) {
        if (const auto parsed = fromJson<SessionConfigOption>(option))
            configUpdate.addConfigOption(*parsed);
    }
    update._value = configUpdate;
    update._kind = "config_option_update";
    sendNotification("session/update",
                     toJson(SessionNotification().sessionId(request->sessionId()).update(update)));
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
