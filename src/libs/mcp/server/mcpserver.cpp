// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcpserver.h"

#include <utils/co_result.h>
#include <utils/overloaded.h>
#include <utils/result.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QTcpServer>
#include <QTimer>

#ifdef MCP_SERVER_HAS_QT_HTTP_SERVER
#include <QHttpServer>
#include <QHttpServerRequest>
#include <QHttpServerResponder>
#include <QHttpServerResponse>
#else
#include "minihttpserver.h"
// Bring fallback types into the global namespace so the rest of the file
// compiles unchanged whether or not Qt::HttpServer is present.
using QHttpServer = MiniHttp::HttpServer;
using QHttpServerRequest = MiniHttp::HttpRequest;
using QHttpServerResponder = MiniHttp::HttpResponder;
using QHttpServerResponse = MiniHttp::HttpResponse;
using QHttpHeaders = MiniHttp::HttpHeaders;
#endif

Q_LOGGING_CATEGORY(mcpServerLog, "mcp.server", QtWarningMsg)
Q_LOGGING_CATEGORY(mcpServerIOLog, "mcp.server.io", QtWarningMsg)

using namespace Utils;

namespace Mcp {

static constexpr int s_maxPageSize = 100;

enum ErrorCodes {
    // Defined by JSON RPC
    ParseError = -32700,
    InvalidRequest = -32600,
    MethodNotFound = -32601,
    InvalidParams = -32602,
    InternalError = -32603,
    serverErrorStart = -32099,
    serverErrorEnd = -32000,
    ServerNotInitialized = -32002,
    UnknownErrorCode = -32001,
};

class SseStream : public QObject
{
public:
    QHttpServerResponder responder;
    const QString sessionId;

    SseStream(QHttpHeaders headers, QHttpServerResponder &&_responder)
        : responder(std::move(_responder))
        , sessionId(QString::fromUtf8(headers.value("mcp-session-id")))
    {
        headers.append("Content-type", "text/event-stream");

        qCDebug(mcpServerLog) << "Starting SSE stream for session"
                              << headers.value("mcp-session-id");
        responder.writeBeginChunked(headers, QHttpServerResponder::StatusCode::Ok);
    }

    ~SseStream() { responder.writeEndChunked({"\n\n"}); }

    bool sendData(const QByteArray &data, const QString &sId)
    {
        if (responder.isResponseCanceled())
            return false;

        if (!sId.isEmpty() && sessionId != sId)
            return true; // Not for this stream

        QByteArray event = "data: " + data + "\n\n";
        responder.writeChunk(event);
        return true;
    }
};

static QJsonObject makeResponse(Schema::RequestId id, const Schema::ServerResult &result)
{
    return Schema::toJson(
        Schema::JSONRPCResultResponse().id(id).result(
            Schema::Result().additionalProperties(Schema::toJson(result))));
};

struct Responder
{
    std::function<void(QJsonDocument)> write;
    std::function<void(QHttpServerResponder::StatusCode)> writeStatus;
    std::function<void(const QByteArray &, const char *, QHttpServerResponse::StatusCode)> writeData;

    std::shared_ptr<QHttpServerResponder> httpResponder;
};

class ServerPrivate : public std::enable_shared_from_this<ServerPrivate>
{
    Schema::Implementation serverInfo;

public:
    ServerPrivate(Schema::Implementation serverInfo)
        : serverInfo(serverInfo)
    {}

    bool bind(QTcpServer *server) { return m_server.bind(server); }

    QHttpHeaders corsHeaders(const QString &sessionId) const
    {
        QHttpHeaders headers;

        if (enableCors) {
            headers.append("Access-Control-Allow-Origin", "*");
            headers.append("Access-Control-Allow-Methods", "GET, POST, OPTIONS, DELETE");
            headers.append(
                "access-control-expose-headers",
                "mcp-session-id, last-event-id, mcp-protocol-version");
            headers.append(
                "Access-Control-Allow-Headers",
                "Content-Type, mcp-session-id, last-event-id, mcp-protocol-version");
        }

        if (!sessionId.isNull())
            headers.append("mcp-session-id", sessionId.toUtf8());

        return headers;
    }

    Result<void> validateOrigin(const QHttpServerRequest &req)
    {
        if (!enableCors || !req.headers().contains("Origin"))
            return {};

        const auto originHeader = req.headers().value("Origin");
        if (originHeader.isEmpty())
            return ResultError("Empty origin header");

        const QUrl origin(QString::fromUtf8(originHeader));
        if (!origin.isValid())
            return ResultError(
                QString("Invalid Origin header: %1").arg(QString::fromUtf8(originHeader)));

        // Check origin is localhost.
        QHostAddress originHost(origin.host());
        if (origin.host() != "localhost" && !originHost.isLoopback())
            return ResultError(QString("Origin not allowed: %1").arg(origin.toString()));

        return {};
    }

    void sendNotification(const Schema::ServerNotification &notification, const QString &sessionId)
    {
        auto data = QJsonDocument(toJson(notification)).toJson(QJsonDocument::Compact);

        if (m_ioOutputHandler) {
            m_ioOutputHandler(data);
        }

        for (auto it = m_sseStreams.begin(); it != m_sseStreams.end();) {
            if (!(*it)->sendData(data, sessionId))
                it = m_sseStreams.erase(it);
            else
                ++it;
        }
    }

    void sendServerRequest(
        Schema::ServerRequest request,
        const QString &sessionId,
        std::function<void(Schema::JSONRPCResponse)> onResponse = {})
    {
        const int requestId = m_serverRequests.isEmpty() ? 1 : (m_serverRequests.lastKey() + 1);
        std::visit([requestId](auto &r) { r.id(requestId); }, request);

        m_serverRequests[requestId] = onResponse;

        auto data = QJsonDocument(toJson(request)).toJson(QJsonDocument::Compact);

        if (m_ioOutputHandler) {
            m_ioOutputHandler(data);
        }

        for (auto it = m_sseStreams.begin(); it != m_sseStreams.end();) {
            if (!(*it)->sendData(data, sessionId))
                it = m_sseStreams.erase(it);
            else
                ++it;
        }
    }

    void onRequest(
        Schema::RequestId id,
        const Schema::ClientRequest &request,
        const Responder &responder,
        QString sessionId)
    {
        qCDebug(mcpServerLog) << "Received JSONRPCRequest:" << Schema::dispatchValue(request);

        if (std::holds_alternative<Schema::InitializeRequest>(request)) {
            if (m_sessions.contains(sessionId)) {
                qCWarning(mcpServerLog)
                    << "Received initialize request with already assigned session ID" << sessionId
                    << ", rejecting";
                responder.writeStatus(QHttpServerResponder::StatusCode::BadRequest);
                return;
            }

            onInitialize(id, std::get<Schema::InitializeRequest>(request), responder, sessionId);
            return;
        }

        if (std::holds_alternative<Schema::CallToolRequest>(request)) {
            onToolCall(id, std::get<Schema::CallToolRequest>(request), responder, sessionId);
            return;
        } else if (std::holds_alternative<Schema::ListToolsRequest>(request)) {
            onToolsList(id, std::get<Schema::ListToolsRequest>(request), responder);
            return;
        } else if (std::holds_alternative<Schema::ListPromptsRequest>(request)) {
            onPromptsList(id, std::get<Schema::ListPromptsRequest>(request), responder);
            return;
        } else if (std::holds_alternative<Schema::GetPromptRequest>(request)) {
            onGetPrompt(id, std::get<Schema::GetPromptRequest>(request), responder);
            return;
        } else if (std::holds_alternative<Schema::ListResourcesRequest>(request)) {
            onResourcesList(id, std::get<Schema::ListResourcesRequest>(request), responder);
            return;
        } else if (std::holds_alternative<Schema::ReadResourceRequest>(request)) {
            onReadResource(id, std::get<Schema::ReadResourceRequest>(request), responder);
            return;
        } else if (std::holds_alternative<Schema::ListResourceTemplatesRequest>(request)) {
            onListResourceTemplates(
                id, std::get<Schema::ListResourceTemplatesRequest>(request), responder);
            return;
        } else if (std::holds_alternative<Schema::CompleteRequest>(request)) {
            onComplete(id, std::get<Schema::CompleteRequest>(request), responder);
            return;
        } else if (std::holds_alternative<Schema::GetTaskRequest>(request)) {
            onGetTask(id, std::get<Schema::GetTaskRequest>(request), responder);
            return;
        } else if (std::holds_alternative<Schema::ListTasksRequest>(request)) {
            onListTasks(id, std::get<Schema::ListTasksRequest>(request), responder);
            return;
        } else if (std::holds_alternative<Schema::CancelTaskRequest>(request)) {
            onCancelTask(id, std::get<Schema::CancelTaskRequest>(request), responder);
            return;
        } else if (std::holds_alternative<Schema::GetTaskPayloadRequest>(request)) {
            onGetTaskPayload(id, std::get<Schema::GetTaskPayloadRequest>(request), responder);
            return;
        } else if (std::holds_alternative<Schema::PingRequest>(request)) {
            onPing(id, std::get<Schema::PingRequest>(request), responder);
            return;
        }

        responder.write(QJsonDocument(
            Schema::toJson(
                Schema::JSONRPCErrorResponse()
                    .error(
                        Schema::Error()
                            .code(MethodNotFound)
                            .message(QString("Method \"%1\" not implemented")
                                         .arg(Schema::dispatchValue(request))))
                    .id(id))));

        return;
    }

    void onInitialize(
        Schema::RequestId id,
        const Schema::InitializeRequest &request,
        const Responder &responder,
        const QString &sessionId)
    {
        if (request.params().protocolVersion() != "2025-11-25") {
            auto errorResponse = Schema::JSONRPCErrorResponse().id(id).error(
                Schema::Error()
                    .code(InvalidRequest)
                    .message(QString("Unsupported protocol version: %1")
                                 .arg(request.params().protocolVersion())));

            responder.write(QJsonDocument(Schema::toJson(errorResponse)));
            return;
        }

        qCDebug(mcpServerLog).noquote()
            << "Client initialized with protocol version" << Schema::toJson(request.params());

        auto caps = Schema::ServerCapabilities()
                        .prompts(Schema::ServerCapabilities::Prompts{}.listChanged(true))
                        .tools(Schema::ServerCapabilities::Tools().listChanged(true))
                        .resources(Schema::ServerCapabilities::Resources{}.listChanged(true))
                        .tasks(
                            Schema::ServerCapabilities::Tasks()
                                .list(QJsonObject{})
                                .cancel(QJsonObject{})
                                .requests(
                                    Schema::ServerCapabilities::Tasks::Requests().tools(
                                        Schema::ServerCapabilities::Tasks::Requests::Tools().call(
                                            QJsonObject{}))));

        if (m_completionCallback)
            caps = caps.completions(QJsonObject());

        auto initResult = Schema::InitializeResult()
                              .protocolVersion(request.params().protocolVersion())
                              .serverInfo(serverInfo)
                              .capabilities(caps);

        qCDebug(mcpServerLog) << "Assigning session ID" << sessionId << "to new client";
        m_sessions.insert(
            sessionId, Client{request.params().capabilities(), request.params().clientInfo()});

        responder.write(QJsonDocument(makeResponse(id, initResult)));
    }

    void onPing(Schema::RequestId id, const Schema::PingRequest &request, const Responder &responder)
    {
        Q_UNUSED(request);
        responder.write(QJsonDocument(Schema::toJson(Schema::JSONRPCResultResponse().id(id))));
    }

    void onGetTaskPayload(
        Schema::RequestId id,
        const Schema::GetTaskPayloadRequest &request,
        const Responder &responder)
    {
        const auto it = m_tasks.find(request.params().taskId());
        if (it == m_tasks.end()) {
            responder.write(QJsonDocument(
                Schema::toJson(
                    Schema::JSONRPCErrorResponse()
                        .error(
                            Schema::Error()
                                .code(InvalidParams)
                                .message(QString("Task with ID \"%1\" not found")
                                             .arg(request.params().taskId())))
                        .id(id))));
            return;
        }

        Result<Schema::CallToolResult> r = it->second.callbacks.result();

        if (!r) {
            responder.write(QJsonDocument(
                Schema::toJson(
                    Schema::JSONRPCErrorResponse()
                        .error(
                            Schema::Error()
                                .code(InternalError)
                                .message(QString("Unknown Error: %1").arg(r.error())))
                        .id(id))));
            return;
        }

        Schema::CallToolResult result = *r;
        result.add_meta(
            "io.modelcontextprotocol/related-task",
            toJson(Schema::RelatedTaskMetadata().taskId(request.params().taskId())));

        responder.write(QJsonDocument(makeResponse(id, result)));
    }

    void onCancelTask(
        Schema::RequestId id, const Schema::CancelTaskRequest &request, const Responder &responder)
    {
        const auto it = m_tasks.find(request.params().taskId());
        if (it == m_tasks.end()) {
            responder.write(QJsonDocument(
                Schema::toJson(
                    Schema::JSONRPCErrorResponse()
                        .error(
                            Schema::Error()
                                .code(InvalidParams)
                                .message(QString("Task with ID \"%1\" not found")
                                             .arg(request.params().taskId())))
                        .id(id))));
            return;
        }

        if (!it->second.callbacks.cancelTask) {
            responder.write(QJsonDocument(
                Schema::toJson(
                    Schema::JSONRPCErrorResponse()
                        .error(
                            Schema::Error()
                                .code(InvalidParams)
                                .message(QString("Task with ID \"%1\" cannot be cancelled")
                                             .arg(request.params().taskId())))
                        .id(id))));
            return;
        }

        (*it->second.callbacks.cancelTask)();
        it->second.task.status(Schema::TaskStatus::cancelled);

        auto result = Mcp::Schema::CancelTaskResult()
                          .taskId(request.params().taskId())
                          .status(it->second.task.status())
                          .statusMessage(it->second.task.statusMessage())
                          .createdAt(it->second.task.createdAt())
                          .lastUpdatedAt(it->second.task.lastUpdatedAt())
                          .pollInterval(it->second.task.pollInterval())
                          .ttl(it->second.task.ttl());

        responder.write(QJsonDocument(makeResponse(id, result)));
    }

    void onListTasks(
        Schema::RequestId id, const Schema::ListTasksRequest &request, const Responder &responder)
    {
        Schema::ListTasksResult result;
        // Cursor
        auto it = m_tasks.begin();
        if (request.params() && request.params()->cursor())
            it = m_tasks.find(*request.params()->cursor());

        // Pagination
        int count = 0;
        for (; it != m_tasks.end() && count < s_maxPageSize; ++it, ++count) {
            result.addTask(it->second.task);
        }
        if (it != m_tasks.end())
            result.nextCursor(it->first);

        responder.write(QJsonDocument(makeResponse(id, result)));
    }

    void onComplete(
        Schema::RequestId id, const Schema::CompleteRequest &request, const Responder &responder)
    {
        if (!m_completionCallback) {
            responder.write(QJsonDocument(
                Schema::toJson(
                    Schema::JSONRPCErrorResponse()
                        .error(
                            Schema::Error().code(MethodNotFound).message("Completion not supported"))
                        .id(id))));
            return;
        }

        const auto onResult = [responder, id](Result<Schema::CompleteResult> result) mutable {
            if (result) {
                responder.write(QJsonDocument(makeResponse(id, *result)));
                return;
            }
            responder.write(QJsonDocument(
                Schema::toJson(
                    Schema::JSONRPCErrorResponse()
                        .error(
                            Schema::Error()
                                .code(InternalError)
                                .message(QString("Unknown Error: %1").arg(result.error())))
                        .id(id))));
        };

        m_completionCallback(request.params(), onResult);
    }

    void onGetTask(
        Schema::RequestId id, const Schema::GetTaskRequest &request, const Responder &responder)
    {
        auto it = m_tasks.find(request.params().taskId());
        if (it == m_tasks.end()) {
            responder.write(QJsonDocument(
                Schema::toJson(
                    Schema::JSONRPCErrorResponse()
                        .error(
                            Schema::Error()
                                .code(InvalidParams)
                                .message(QString("Task with ID \"%1\" not found")
                                             .arg(request.params().taskId())))
                        .id(id))));
            return;
        }

        // Update task information
        auto newTask = it->second.callbacks.updateTask(it->second.task);
        it->second.update(newTask, shared_from_this());

        auto result = Mcp::Schema::GetTaskResult()
                          .taskId(request.params().taskId())
                          .status(it->second.task.status())
                          .createdAt(it->second.task.createdAt())
                          .lastUpdatedAt(it->second.task.lastUpdatedAt())
                          .ttl(it->second.task.ttl())
                          .statusMessage(it->second.task.statusMessage())
                          .pollInterval(it->second.task.pollInterval());

        responder.write(QJsonDocument(makeResponse(id, result)));
    }

    void onSimpleToolCall(
        Schema::RequestId id,
        const Schema::CallToolRequest &request,
        const Responder &responder,
        const Server::ToolCallback &cb)
    {
        if (request.params().task().has_value()) {
            qCWarning(mcpServerLog) << "Received call for tool" << request.params().name()
                                    << "with task parameters, but tool does not support tasks";
            responder.write(QJsonDocument(toJson(
                Schema::JSONRPCErrorResponse()
                    .error(
                        Schema::Error()
                            .code(InvalidParams)
                            .message("Tool does not support tasks: " + request.params().name()))
                    .id(id))));
            return;
        }

        Result<Schema::CallToolResult> r = cb(request.params());

        if (!r) {
            responder.write(QJsonDocument(makeResponse(
                id,
                Schema::CallToolResult().isError(true).content(
                    {Schema::TextContent().text(r.error())}))));
            return;
        }

        responder.write(QJsonDocument(makeResponse(id, *r)));
    }

    void onToolInterfaceCall(
        Schema::RequestId id,
        const QString &sessionId,
        const Schema::CallToolRequest &request,
        const Responder &responder,
        const Server::ToolInterfaceCallback &cb)
    {
        Schema::ClientCapabilities clientCapabilities = m_sessions.value(sessionId).capabilities;

        ToolInterface toolInterface(
            shared_from_this(), clientCapabilities, request.id(), sessionId, responder);

        Result<> r = cb(request.params(), toolInterface);
        if (!r) {
            responder.write(QJsonDocument(makeResponse(
                id,
                Schema::CallToolResult().isError(true).content(
                    {Schema::TextContent().text(r.error())}))));
        }
    }

    void onToolCall(
        Schema::RequestId id,
        const Schema::CallToolRequest &request,
        const Responder &responder,
        const QString sessionId)
    {
        auto toolIt = m_tools.find(request.params().name());

        if (toolIt == m_tools.end()) {
            qCWarning(mcpServerLog) << "Received call for unknown tool:" << request.params().name();

            responder.write(QJsonDocument(toJson(
                Schema::JSONRPCErrorResponse()
                    .error(
                        Schema::Error()
                            .code(MethodNotFound)
                            .message("Invalid Tool:" + request.params().name()))
                    .id(id))));

            return;
        }

        qCDebug(mcpServerLog) << "Running tool" << toolIt.key();

        std::visit(
            overloaded{
                [&](const Server::ToolCallback &cb) {
                    onSimpleToolCall(id, request, responder, cb);
                },
                [&](const Server::ToolInterfaceCallback &cb) {
                    onToolInterfaceCall(id, sessionId, request, responder, cb);
                },
            },
            toolIt->callback);
    }

    void onToolsList(
        Schema::RequestId id, const Schema::ListToolsRequest &request, const Responder &responder)
    {
        auto it = m_tools.begin();
        if (request.params() && request.params()->cursor())
            it = m_tools.find(*request.params()->cursor());

        Schema::ListToolsResult result;
        int count = 0;
        for (; it != m_tools.end() && count < s_maxPageSize; ++it, ++count) {
            result.addTool(it.value().tool);
        }
        if (it != m_tools.end())
            result.nextCursor(it.key());

        responder.write(QJsonDocument(makeResponse(id, result)));
    }

    void onPromptsList(
        Schema::RequestId id, const Schema::ListPromptsRequest &request, const Responder &responder)
    {
        auto it = m_prompts.begin();
        if (request.params() && request.params()->cursor())
            it = m_prompts.find(*request.params()->cursor());

        Schema::ListPromptsResult result;
        int count = 0;
        for (; it != m_prompts.end() && count < s_maxPageSize; ++it, ++count) {
            result.addPrompt(it->prompt);
        }
        if (it != m_prompts.end())
            result.nextCursor(it.key());

        responder.write(QJsonDocument(makeResponse(id, result)));
    }

    void onResourcesList(
        Schema::RequestId id,
        const Schema::ListResourcesRequest &request,
        const Responder &responder)
    {
        auto it = m_resources.begin();
        if (request.params() && request.params()->cursor())
            it = m_resources.find(*request.params()->cursor());

        Schema::ListResourcesResult result;
        int count = 0;
        for (; it != m_resources.end() && count < s_maxPageSize; ++it, ++count) {
            result.addResource(it.value().resource);
        }
        if (it != m_resources.end())
            result.nextCursor(it.key());

        responder.write(QJsonDocument(makeResponse(id, result)));
    }

    void onReadResource(
        Schema::RequestId id, const Schema::ReadResourceRequest &request, const Responder &responder)
    {
        auto it = m_resources.find(request.params().uri());
        if (it == m_resources.end()) {
            if (m_resourceFallbackCallback) {
                Result<Schema::ReadResourceResult> r = m_resourceFallbackCallback(request.params());

                if (r) {
                    responder.write(QJsonDocument(makeResponse(id, *r)));
                    return;
                }
            }

            responder.write(QJsonDocument(
                Schema::toJson(
                    Schema::JSONRPCErrorResponse()
                        .error(
                            Schema::Error()
                                .code(InvalidParams)
                                .message(
                                    QString("Resource \"%1\" not found").arg(request.params().uri())))
                        .id(id))));
            return;
        }

        Result<Schema::ReadResourceResult> r = it->callback(request.params());
        if (!r) {
            responder.write(QJsonDocument(
                Schema::toJson(
                    Schema::JSONRPCErrorResponse()
                        .error(Schema::Error().code(InternalError).message(r.error()))
                        .id(id))));
            return;
        }

        responder.write(QJsonDocument(makeResponse(id, *r)));
    }

    void onListResourceTemplates(
        Schema::RequestId id,
        const Schema::ListResourceTemplatesRequest &request,
        const Responder &responder)
    {
        auto it = m_resourceTemplates.begin();
        if (request.params() && request.params()->cursor())
            it = m_resourceTemplates.find(*request.params()->cursor());

        Schema::ListResourceTemplatesResult result;
        int count = 0;
        for (; it != m_resourceTemplates.end() && count < s_maxPageSize; ++it, ++count) {
            result.addResourceTemplate(it.value());
        }
        if (it != m_resourceTemplates.end())
            result.nextCursor(it.key());

        responder.write(QJsonDocument(makeResponse(id, result)));
    }

    void onGetPrompt(
        Schema::RequestId id, const Schema::GetPromptRequest &request, const Responder &responder)
    {
        auto it = m_prompts.find(request.params().name());
        if (it == m_prompts.end()) {
            responder.write(QJsonDocument(
                Schema::toJson(
                    Schema::JSONRPCErrorResponse()
                        .error(
                            Schema::Error()
                                .code(InvalidParams)
                                .message(
                                    QString("Prompt \"%1\" not found").arg(request.params().name())))
                        .id(id))));
            return;
        }

        QList<Schema::PromptMessage> messages = it->callback(
            request.params().arguments().value_or(QMap<QString, QString>{}));

        auto result
            = Schema::GetPromptResult().description(it->prompt.description()).messages(messages);
        responder.write(QJsonDocument(makeResponse(id, result)));
    }

    void onTaskStatusNotification(
        const Schema::TaskStatusNotification &notification, QString sessionId)
    {
        SessionIdAndTaskId key{sessionId, notification.params().taskId()};
        auto it = m_clientTasks.find(key);
        if (it == m_clientTasks.end()) {
            qCWarning(mcpServerLog)
                << "Received status notification for unknown task ID"
                << notification.params().taskId() << "and session ID" << sessionId;
            return;
        }
        if (!*it)
            return;

        auto task = (*it)->task;
        task.status(notification.params().status());
        task.statusMessage(notification.params().statusMessage());
        task.lastUpdatedAt(notification.params().lastUpdatedAt());
        task.pollInterval(notification.params().pollInterval());

        (*it)->onTaskUpdate(task);
    }

    void onData(const QByteArray &data, const Responder &responder, QString sessionId)
    {
        QJsonParseError parseError;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &parseError);
        if (parseError.error != QJsonParseError::NoError || !jsonDoc.isObject()) {
            responder.writeData(
                "Invalid JSON body", "text/plain", QHttpServerResponse::StatusCode::BadRequest);
            return;
        }

        const auto request = Schema::fromJson<Schema::JSONRPCRequest>(jsonDoc.object());
        const auto clientRequest = Schema::fromJson<Schema::ClientRequest>(jsonDoc.object());
        if (request && clientRequest) {
            onRequest(request->_id, *clientRequest, responder, sessionId);
            return;
        }

        const auto clientNotification = Schema::fromJson<Schema::ClientNotification>(
            jsonDoc.object());
        if (clientNotification) {
            qCDebug(mcpServerLog) << "Received JSONRPCNotification:"
                                  << Schema::dispatchValue(clientNotification.value());

            if (std::holds_alternative<Schema::TaskStatusNotification>(*clientNotification)) {
                const auto &notification = std::get<Schema::TaskStatusNotification>(
                    *clientNotification);
                onTaskStatusNotification(notification, sessionId);
            }

            responder.writeStatus(QHttpServerResponse::StatusCode::Accepted);
            return;
        }

        const auto clientResponse = Schema::fromJson<Schema::JSONRPCResponse>(jsonDoc.object());
        if (clientResponse) {
            auto requestId = std::visit(
                [](const auto &resp) -> std::optional<Schema::RequestId> { return resp.id(); },
                *clientResponse);

            if (!requestId) {
                qCWarning(mcpServerLog) << "Received JSONRPC response without ID, rejecting";
                responder.writeStatus(QHttpServerResponse::StatusCode::BadRequest);
                return;
            }

            if (std::holds_alternative<QString>(*requestId)) {
                qCWarning(mcpServerLog)
                    << "Received JSONRPC response with non-integer ID, rejecting";
                responder.writeStatus(QHttpServerResponse::StatusCode::BadRequest);
                return;
            }

            int id = std::get<int>(*requestId);
            auto it = m_serverRequests.find(id);
            if (it != m_serverRequests.end()) {
                auto callback = it.value();
                m_serverRequests.erase(it);
                callback(*clientResponse);
                return;
            }
        }

        responder.writeStatus(QHttpServerResponse::StatusCode::BadRequest);
        return;
    }

    bool validateSession(QString sessionId)
    {
        if (sessionId.isEmpty())
            return false;

        auto it = m_sessions.find(sessionId);
        if (it == m_sessions.end()) {
            return false;
        }
        it->lastSeen = QDateTime::currentDateTime();
        return true;
    }

    struct SessionIdAndTaskId
    {
        QString sessionId;
        QString taskId;
        bool operator<(const SessionIdAndTaskId &other) const
        {
            return std::tie(sessionId, taskId) < std::tie(other.sessionId, other.taskId);
        }
    };

    struct RunningClientTask : public QObject
    {
        using Callback = std::function<void(const Utils::Result<Schema::GetTaskPayloadResult> &)>;
        std::weak_ptr<ServerPrivate> weak;
        QString sessionId;
        Schema::Task task;
        QTimer pollTimer;
        Callback callback;

        RunningClientTask(
            ServerPrivate *server, const QString &sId, const Schema::Task &t, const Callback &cb)
            : weak(server->shared_from_this())
            , sessionId(sId)
            , task(t)
            , callback(cb)
        {
            pollTimer.setSingleShot(false);
            QObject::connect(&pollTimer, &QTimer::timeout, this, &RunningClientTask::poll);

            if (task.pollInterval()) {
                pollTimer.setInterval(std::chrono::milliseconds(*task.pollInterval()));
                pollTimer.start();
            }
        }

        void poll()
        {
            if (auto d = weak.lock()) {
                d->sendServerRequest(
                    Schema::GetTaskRequest().params(
                        Schema::GetTaskRequest::Params().taskId(task.taskId())),
                    sessionId,
                    [this](const Utils::Result<Schema::JSONRPCResponse> &result) {
                        if (!result) {
                            finish(ResultError(
                                QString("Failed to get task status: %1").arg(result.error())));
                        } else {
                            if (std::holds_alternative<Schema::JSONRPCErrorResponse>(*result)) {
                                const auto &errorResponse = std::get<Schema::JSONRPCErrorResponse>(
                                    *result);
                                finish(ResultError(QString("Failed to get task status: %1")
                                                       .arg(errorResponse.error().message())));
                            } else if (std::holds_alternative<Schema::JSONRPCResultResponse>(
                                           *result)) {
                                const auto &resultResponse
                                    = std::get<Schema::JSONRPCResultResponse>(*result);
                                auto taskResult = Schema::fromJson<Schema::GetTaskResult>(
                                    resultResponse.result().additionalProperties());
                                if (!taskResult) {
                                    finish(ResultError(
                                        QString("Failed to parse task status result: %1")
                                            .arg(taskResult.error())));
                                    return;
                                }

                                onTaskUpdate(
                                    Schema::Task()
                                        .taskId(taskResult->taskId())
                                        .status(taskResult->status())
                                        .statusMessage(taskResult->statusMessage())
                                        .createdAt(taskResult->createdAt())
                                        .lastUpdatedAt(taskResult->lastUpdatedAt())
                                        .pollInterval(taskResult->pollInterval())
                                        .ttl(taskResult->ttl()));
                            }
                        }
                    });
            }
        }

        void finish(const Utils::Result<Schema::GetTaskPayloadResult> &payloadResult)
        {
            pollTimer.stop();

            callback(payloadResult);

            if (auto d = weak.lock()) {
                d->removeClientTask(sessionId, task.taskId());
            }
        }

        void onTaskUpdate(const Schema::Task &newTask)
        {
            if (newTask.pollInterval() != task.pollInterval()) {
                if (newTask.pollInterval()) {
                    pollTimer.setInterval(std::chrono::milliseconds(*newTask.pollInterval()));
                    pollTimer.start();
                } else {
                    pollTimer.stop();
                }
            }

            task = newTask;

            if (newTask.status() == Schema::TaskStatus::completed) {
                pollTimer.stop();

                if (auto d = weak.lock()) {
                    d->sendServerRequest(
                        Schema::GetTaskPayloadRequest().params(
                            Schema::GetTaskPayloadRequest::Params().taskId(task.taskId())),
                        sessionId,
                        [this](const Utils::Result<Schema::JSONRPCResponse> &result) {
                            if (!result) {
                                finish(ResultError(
                                    QString("Failed to get task payload: %1").arg(result.error())));
                            } else {
                                if (std::holds_alternative<Schema::JSONRPCErrorResponse>(*result)) {
                                    const auto &errorResponse
                                        = std::get<Schema::JSONRPCErrorResponse>(*result);
                                    finish(ResultError(QString("Failed to get task payload: %1")
                                                           .arg(errorResponse.error().message())));
                                } else if (std::holds_alternative<Schema::JSONRPCResultResponse>(
                                               *result)) {
                                    const auto &resultResponse
                                        = std::get<Schema::JSONRPCResultResponse>(*result);
                                    auto payloadResult
                                        = Schema::fromJson<Schema::GetTaskPayloadResult>(
                                            resultResponse.result().additionalProperties());

                                    finish(payloadResult);
                                }
                            }
                        });
                }
            }
        }
    };

    QMap<SessionIdAndTaskId, RunningClientTask *> m_clientTasks;

    void addClientTask(
        const QString &sessionId,
        const Schema::Task &task,
        const RunningClientTask::Callback &callback)
    {
        std::weak_ptr<ServerPrivate> weak = shared_from_this();

        m_clientTasks.insert(
            SessionIdAndTaskId{sessionId, task.taskId()},
            new RunningClientTask(this, sessionId, task, callback));
    }

    void removeClientTask(const QString &sessionId, const QString &taskId)
    {
        auto it = m_clientTasks.find(SessionIdAndTaskId{sessionId, taskId});
        if (it != m_clientTasks.end()) {
            it.value()->deleteLater();
            m_clientTasks.erase(it);
        }
    }

    struct ToolAndCallback
    {
        Schema::Tool tool;
        std::variant<Server::ToolInterfaceCallback, Server::ToolCallback> callback;
    };
    QMap<QString, ToolAndCallback> m_tools;

    struct PromptAndCallback
    {
        Schema::Prompt prompt;
        Server::PromptCallback callback;
    };
    QMap<QString, PromptAndCallback> m_prompts;

    struct ResourceAndCallback
    {
        Schema::Resource resource;
        Server::ResourceCallback callback;
    };
    QMap<QString, ResourceAndCallback> m_resources;
    Server::ResourceCallback m_resourceFallbackCallback;
    QMap<QString, Schema::ResourceTemplate> m_resourceTemplates;

    QHttpServer m_server;
    std::vector<std::unique_ptr<SseStream>> m_sseStreams;
    std::function<void(QByteArray)> m_ioOutputHandler;

    Server::CompletionCallback m_completionCallback;

    using UpdateTaskCallback = std::function<Schema::Task(Schema::Task)>;
    using TaskResultCallback = std::function<Utils::Result<Schema::CallToolResult>()>;
    using CancelTaskCallback = std::function<void()>;

    struct TaskCallbacks
    {
        UpdateTaskCallback updateTask;
        TaskResultCallback result;
        std::optional<CancelTaskCallback> cancelTask;
        int pollingIntervalMs{1000};
    };

    using UniqueDeleteLaterTimer = std::unique_ptr<QTimer, QScopedPointerObjectDeleteLater<QTimer>>;

    struct TaskAndCallbacks
    {
        TaskAndCallbacks(
            Schema::Task task, TaskCallbacks callbacks, const std::weak_ptr<ServerPrivate> &server)
            : task(std::move(task))
            , callbacks(std::move(callbacks))
        {
            createTimer(server);
        }

        Schema::Task task;
        TaskCallbacks callbacks;
        UniqueDeleteLaterTimer timer;

        void createTimer(const std::weak_ptr<ServerPrivate> &server)
        {
            if (auto ttl = task.ttl()) {
                timer.reset(new QTimer());
                timer->setSingleShot(true);

                const QDateTime createdAt = QDateTime::fromString(task.createdAt(), Qt::ISODate);
                const QDateTime now = QDateTime::currentDateTime();
                const std::chrono::milliseconds age = now - createdAt;
                const std::chrono::milliseconds remainingTtl = std::chrono::milliseconds(*ttl)
                                                               - age;
                timer->setInterval(remainingTtl);
                QObject::connect(timer.get(), &QTimer::timeout, [server, taskId = task.taskId()]() {
                    if (auto d = server.lock()) {
                        auto it = d->m_tasks.find(taskId);
                        if (it == d->m_tasks.end())
                            return;
                        d->m_tasks.erase(it);
                    }
                });
                timer->start();
            } else {
                timer.reset();
            }
        }

        void update(const Schema::Task &newTask, const std::weak_ptr<ServerPrivate> &server)
        {
            task.lastUpdatedAt(QDateTime::currentDateTime().toString(Qt::ISODate));
            task.status(newTask.status());
            task.statusMessage(newTask.statusMessage());
            task.pollInterval(newTask.pollInterval());
            if (task.ttl() != newTask.ttl()) {
                task.ttl(newTask.ttl());
                createTimer(server);
            }
        }
    };

    std::map<QString, TaskAndCallbacks> m_tasks;

    struct Client
    {
        Schema::ClientCapabilities capabilities;
        Schema::Implementation info;
        QDateTime lastSeen = QDateTime::currentDateTime();
    };

    QMap<QString, Client> m_sessions;

    QMap<int, std::function<void(Schema::JSONRPCResponse)>> m_serverRequests;

    bool enableCors = false;
};

Server::Server(Schema::Implementation serverInfo, bool enableSSETestRoute)
    : d(std::make_shared<ServerPrivate>(serverInfo))
{
    d->m_server.setMissingHandler(
        new QObject(), [](const QHttpServerRequest &request, QHttpServerResponder &responder) {
            qCDebug(mcpServerIOLog) << request.url() << request.method() << "not found";
            qCDebug(mcpServerIOLog) << request.headers();
            responder.write(QHttpServerResponse::StatusCode::NotFound);
        });

    d->m_server.route(
        "/",
        QHttpServerRequest::Method::Options,
        [this](const QHttpServerRequest &req, QHttpServerResponder &responder) {
            Q_UNUSED(req);
            auto headers = d->corsHeaders({});
            responder.write(headers, QHttpServerResponse::StatusCode::Ok);
        });

    d->m_server.route(
        "/",
        QHttpServerRequest::Method::Delete,
        [this](const QHttpServerRequest &req, QHttpServerResponder &responder) {
            if (!req.headers().contains("mcp-session-id")) {
                qCWarning(mcpServerLog)
                    << "Received request to delete session without session ID, rejecting";
                responder.write(d->corsHeaders({}), QHttpServerResponse::StatusCode::BadRequest);
                return;
            }

            QString sessionId = QString::fromUtf8(req.headers().value("mcp-session-id"));
            if (!d->validateSession(sessionId)) {
                qCWarning(mcpServerLog)
                    << "Received request to delete session with invalid session ID,"
                       "rejecting";
                responder.write(d->corsHeaders({}), QHttpServerResponse::StatusCode::BadRequest);
                return;
            }

            qCDebug(mcpServerLog) << "Deleting session" << sessionId;
            d->m_sessions.remove(sessionId);
            responder.write(d->corsHeaders(sessionId), QHttpServerResponse::StatusCode::Ok);
        });

    d->m_server.route(
        "/",
        QHttpServerRequest::Method::Get,
        [this](const QHttpServerRequest &req, QHttpServerResponder &responder) {
            if (req.headers().value("accept") == "text/event-stream") {
                if (req.headers().contains("mcp-session-id")) {
                    qCDebug(mcpServerLog) << "Received SSE connection with session ID:"
                                          << req.headers().value("mcp-session-id");
                    if (!d->validateSession(
                            QString::fromUtf8(req.headers().value("mcp-session-id")))) {
                        qCWarning(mcpServerLog) << "Received SSE connection with invalid session "
                                                   "ID, closing connection";
                        responder.write(QHttpServerResponse::StatusCode::NotFound);
                        return;
                    }
                } else {
                    qCWarning(mcpServerLog)
                        << "Received SSE connection without session ID, closing connection";
                    responder.write(d->corsHeaders({}), QHttpServerResponse::StatusCode::NotFound);
                    return;
                }

                d->m_sseStreams.emplace_back(
                    std::make_unique<SseStream>(
                        d->corsHeaders(QString::fromUtf8(req.headers().value("mcp-session-id"))),
                        std::move(responder)));
                return;
            }

            responder.write(QHttpServerResponse::StatusCode::NotFound);
        });

    if (enableSSETestRoute) {
        d->m_server.route("/ssetest", QHttpServerRequest::Method::Get, []() {
            auto html = R"(
                <html>
                    <body>
                        <h1>SSE Test</h1>
                        <div id="events"></div>
                        <script>
                            const eventSource = new EventSource("/");
                            eventSource.onmessage = function(event) {
                                const newElement = document.createElement("div");
                                newElement.textContent = "Received event: " + event.data;
                                document.getElementById("events").appendChild(newElement);
                            };
                            eventSource.onerror = function() {
                                const newElement = document.createElement("div");
                                newElement.textContent = "Error occurred, connection closed.";
                                document.getElementById("events").appendChild(newElement);
                                eventSource.close();
                            };
                        </script>
                    </body>
                </html>
            )";
            return QHttpServerResponse(html, QHttpServerResponse::StatusCode::Ok);
        });
    }

    d->m_server.route(
        "/",
        QHttpServerRequest::Method::Post,
        [this](const QHttpServerRequest &req, QHttpServerResponder &responder) -> void {
            auto errorHeaders = d->corsHeaders({});
            errorHeaders.append("content-type", "text/plain");

            Result<void> originValid = d->validateOrigin(req);
            if (!originValid) {
                qCWarning(mcpServerLog) << "Rejected request with invalid Origin header:"
                                        << req.headers().value("Origin") << originValid.error();
                responder.write(
                    QString("Invalid origin header: %s").arg(originValid.error()).toUtf8(),
                    errorHeaders,
                    QHttpServerResponse::StatusCode::BadRequest);
                return;
            }

            // Check header contains "Accept" with only "application/json" and "text/event-stream"
            if (!req.headers().contains("Accept")) {
                responder.write(
                    "Missing Accept header",
                    errorHeaders,
                    QHttpServerResponse::StatusCode::BadRequest);
                return;
            }

            if (req.headers().contains("mcp-protocol-version")
                && req.headers().value("mcp-protocol-version") != "2025-11-25") {
                responder.write(
                    "Unsupported Mcp protocol version",
                    errorHeaders,
                    QHttpServerResponse::StatusCode::BadRequest);
                return;
            }

            const QString sessionId = req.headers().contains("mcp-session-id")
                                          ? QString::fromUtf8(req.headers().value("mcp-session-id"))
                                          : QUuid::createUuid().toString();

            if (req.headers().contains("mcp-session-id")) {
                bool validSessionId = !sessionId.isNull();
                if (!validSessionId || !d->validateSession(sessionId)) {
                    qCInfo(mcpServerLog) << "Received request with invalid session ID:"
                                         << req.headers().value("mcp-session-id");

                    responder.write(
                        "Invalid session ID",
                        errorHeaders,
                        QHttpServerResponse::StatusCode::NotFound);
                    return;
                }
            }

            qCDebug(mcpServerIOLog).noquote() << "Received request with headers:\n"
                                              << req.headers() << "\nand body:\n"
                                              << req.body() << "\nEnd of body";

            QStringList acceptValues = QString::fromUtf8(req.headers().value("Accept")).split(",");
            for (QString &value : acceptValues)
                value = value.trimmed();
            acceptValues.sort();

            const bool streamMode = acceptValues
                                    == QStringList{"application/json", "text/event-stream"};

            if (!streamMode) {
                responder.write(
                    "Invalid Accept header",
                    errorHeaders,
                    QHttpServerResponse::StatusCode::BadRequest);
                return;
            }

            auto corsHeaders = d->corsHeaders(sessionId);
            Responder r;
            r.httpResponder = std::make_shared<QHttpServerResponder>(std::move(responder));
            r.write = [corsHeaders, http = r.httpResponder](QJsonDocument json) {
                const QByteArray jsonData = json.toJson(QJsonDocument::Compact);
                qCDebug(mcpServerIOLog).noquote() << "Writing response:" << jsonData;

                auto headers = corsHeaders;
                headers.append("content-type", "application/json");
                http->write(jsonData, headers, QHttpServerResponse::StatusCode::Ok);
            };
            r.writeStatus = [corsHeaders,
                             http = r.httpResponder](QHttpServerResponder::StatusCode status) {
                auto headers = corsHeaders;
                http->write(headers, status);
            };
            r.writeData = [corsHeaders, http = r.httpResponder](
                              const QByteArray &data,
                              const char *contentType,
                              QHttpServerResponse::StatusCode status) {
                auto headers = corsHeaders;
                headers.append("content-type", contentType);
                http->write(data, headers, status);
            };

            d->onData(req.body(), r, sessionId);
        });
}

Server::~Server() = default;

bool Server::bind(QTcpServer *server)
{
    return d->bind(server);
}

QList<QTcpServer *> Server::boundTcpServers() const
{
    return d->m_server.servers();
}

void Server::addTool(const Schema::Tool &tool, const ToolInterfaceCallback &callback)
{
    d->m_tools.insert(tool.name(), ServerPrivate::ToolAndCallback{tool, callback});
    sendNotification(Schema::ToolListChangedNotification{});
}

void Server::addTool(const Schema::Tool &tool, const ToolCallback &callback)
{
    d->m_tools.insert(tool.name(), ServerPrivate::ToolAndCallback{tool, callback});
    sendNotification(Schema::ToolListChangedNotification{});
}

void Server::sendNotification(
    const Schema::ServerNotification &notification, const QString &sessionId)
{
    d->sendNotification(notification, sessionId);
}

Result<std::function<void(QByteArray)>> Server::bindIO(std::function<void(QByteArray)> outputHandler)
{
    if (d->m_ioOutputHandler)
        return ResultError("IO already bound");
    if (!outputHandler)
        return ResultError("Output handler cannot be null");
    d->m_ioOutputHandler = std::move(outputHandler);

    Responder r;
    r.write = [this](QJsonDocument json) {
        if (d->m_ioOutputHandler)
            d->m_ioOutputHandler(json.toJson(QJsonDocument::Compact));
    };
    r.writeStatus = [](QHttpServerResponder::StatusCode status) {
        Q_UNUSED(status);
        // We do not use HTTP status codes in IO mode, so ignore this
    };
    r.writeData = [this](
                      const QByteArray &data,
                      const char *contentType,
                      QHttpServerResponse::StatusCode status) {
        Q_UNUSED(contentType);
        Q_UNUSED(status);
        Q_ASSERT(
            data.contains('\n')
            == false); // We use newlines to separate messages, so data cannot contain newlines
        if (d->m_ioOutputHandler)
            d->m_ioOutputHandler(data);
    };

    return [this, r = std::move(r)](QByteArray data) mutable {
        d->onData(data, r, {});
    };
}

void Server::removeTool(const QString &toolName)
{
    if (d->m_tools.remove(toolName) > 0)
        sendNotification(Schema::ToolListChangedNotification{});
}

void Server::addPrompt(const Schema::Prompt &prompt, const PromptCallback &callback)
{
    d->m_prompts.insert(prompt.name(), {prompt, callback});
    sendNotification(Schema::PromptListChangedNotification{});
}

void Server::removePrompt(const QString &promptName)
{
    if (d->m_prompts.remove(promptName) > 0)
        sendNotification(Schema::PromptListChangedNotification{});
}

void Server::addResource(const Schema::Resource &resource, const ResourceCallback &callback)
{
    d->m_resources.insert(resource.uri(), {resource, callback});
    sendNotification(Schema::ResourceListChangedNotification{});
}

void Server::removeResource(const QString &uri)
{
    if (d->m_resources.remove(uri) > 0)
        sendNotification(Schema::ResourceListChangedNotification{});
}

void Server::addResourceTemplate(const Schema::ResourceTemplate &resourceTemplate)
{
    d->m_resourceTemplates.insert(resourceTemplate.name(), resourceTemplate);
    sendNotification(Schema::ResourceListChangedNotification{});
}

void Server::removeResourceTemplate(const QString &name)
{
    if (d->m_resourceTemplates.remove(name) > 0)
        sendNotification(Schema::ResourceListChangedNotification{});
}

void Server::setCompletionCallback(const CompletionCallback &callback)
{
    d->m_completionCallback = callback;
}

void Server::setResourceFallbackCallback(const ResourceCallback &callback)
{
    d->m_resourceFallbackCallback = callback;
}

void Server::setCorsEnabled(bool enabled)
{
    d->enableCors = enabled;
}

struct ToolInterfacePrivate
{
    Schema::ClientCapabilities _clientCapabilities;
    std::weak_ptr<ServerPrivate> _server;
    Schema::RequestId _initialRequestId;
    QString _sessionId;
    Responder _responder;

    bool _isFinished = false;
    bool _isTask = false;
    QString _taskId;

    bool isFinished() const
    {
        if (_isFinished) {
            return true;
        }

        // Check if the task exists / its status == completed.
        if (_isTask) {
            if (auto serverPrivate = _server.lock()) {
                auto it = serverPrivate->m_tasks.find(_taskId);
                if (it == serverPrivate->m_tasks.end()
                    || it->second.task.status() == Schema::TaskStatus::completed
                    || it->second.task.status() == Schema::TaskStatus::cancelled
                    || it->second.task.status() == Schema::TaskStatus::failed) {
                    return true;
                }
            }
        }

        return false;
    }
};

ToolInterface::ToolInterface(
    std::weak_ptr<ServerPrivate> serverPrivate,
    const Schema::ClientCapabilities &clientCaps,
    const Schema::RequestId &id,
    const QString &sessionId,
    const Responder &responder)
    : d(std::make_shared<ToolInterfacePrivate>(clientCaps, serverPrivate, id, sessionId, responder))
{}

ToolInterface::~ToolInterface() {}

const Schema::ClientCapabilities &ToolInterface::clientCapabilities() const
{
    return d->_clientCapabilities;
}

void ToolInterface::elicit(
    const Schema::ElicitRequestParams &params, const ElicitResultCallback &cb) const
{
    if (d->isFinished()) {
        qCWarning(mcpServerLog) << "A finished tool should not ask for elicitation.";
        cb(Utils::ResultError("Tool is already finished"));
        return;
    }

    const bool wantsTask
        = std::visit([](const auto &p) -> bool { return p.task().has_value(); }, params);

    if (!d->_clientCapabilities.elicitation()) {
        qCWarning(mcpServerLog) << "Caller attempted to elicit, but client does not "
                                   "support elicitation";
        cb(Utils::ResultError("Client does not support elicitation"));
        return;
    }

    const bool hasElicitSupport = d->_clientCapabilities.elicitation().has_value();
    const bool hasFormSupportObject = hasElicitSupport
                                      && d->_clientCapabilities.elicitation()->form().has_value();
    const bool hasUrlSupportObject = hasElicitSupport
                                     && d->_clientCapabilities.elicitation()->url().has_value();
    const bool hasFormOrUrlSupport = hasFormSupportObject || hasUrlSupportObject;
    const bool hasFormSupport
        = hasFormSupportObject
          || !hasFormOrUrlSupport; // Fallback if no support is declared, but the elicitation capability is present

    if (std::holds_alternative<Schema::ElicitRequestFormParams>(params) && !hasFormSupport) {
        qCWarning(mcpServerLog) << "Caller attempted to elicit with form parameters, "
                                   "but client does not support elicitation forms";
        cb(Utils::ResultError("Client does not support elicitation forms"));
        return;
    } else if (std::holds_alternative<Schema::ElicitRequestURLParams>(params) && !hasUrlSupportObject) {
        qCWarning(mcpServerLog) << "Caller attempted to elicit with URL parameters, "
                                   "but client does not support elicitation URLs";
        cb(Utils::ResultError("Client does not support elicitation URLs"));
        return;
    }

    if (wantsTask
        && (!d->_clientCapabilities.tasks() || !d->_clientCapabilities.tasks()->requests()
            || !d->_clientCapabilities.tasks()->requests()->elicitation()
            || !d->_clientCapabilities.tasks()->requests()->elicitation()->create())) {
        qCWarning(mcpServerLog) << "Caller attempted to elicit with task parameters, "
                                   "but client does not support task elicitation";
        cb(Utils::ResultError("Client does not support task elicitation"));
        return;
    }

    if (auto serverPrivate = d->_server.lock()) {
        serverPrivate->sendServerRequest(
            Schema::ElicitRequest().params(params),
            d->_sessionId,
            [d = this->d, cb, wantsTask](const Schema::JSONRPCResponse &response) {
                Utils::Result<Schema::ElicitResult> r;

                if (std::holds_alternative<Schema::JSONRPCResultResponse>(response)) {
                    const auto &jsonRpcResult = std::get<Schema::JSONRPCResultResponse>(response);

                    auto elicitResult = Schema::fromJson<Schema::ElicitResult>(
                        jsonRpcResult.result().additionalProperties());

                    if (elicitResult) {
                        cb(Utils::Result<Schema::ElicitResult>(elicitResult));
                        return;
                    }

                    if (wantsTask) {
                        auto elicitTaskResult = Schema::fromJson<Schema::CreateTaskResult>(
                            jsonRpcResult.result().additionalProperties());

                        if (!elicitTaskResult) {
                            qCWarning(mcpServerLog)
                                << "Failed to parse elicit task result from client:"
                                << elicitTaskResult.error();
                            cb(Utils::ResultError(
                                "Failed to parse elicit task result from client: "
                                + elicitTaskResult.error()));
                            return;
                        }

                        if (auto serverPrivate = d->_server.lock()) {
                            serverPrivate->addClientTask(
                                d->_sessionId,
                                elicitTaskResult->task(),
                                [cb](const Utils::Result<Schema::GetTaskPayloadResult> &taskResult) {
                                    cb(Schema::fromJson<Schema::ElicitResult>(
                                        Schema::toJson(*taskResult)));
                                });
                        }

                        return;
                    }

                    qCWarning(mcpServerLog)
                        << "Failed to parse elicit result from client:" << elicitResult.error();
                    cb(Utils::ResultError(
                        "Failed to parse elicit result from client: " + elicitResult.error()));
                    return;
                }

                const auto &error = std::get<Schema::JSONRPCErrorResponse>(response);
                qCWarning(mcpServerLog)
                    << "Received elicit error from client:" << Schema::toJson(error.error());
                r = Utils::ResultError("Client error: " + error.error().message());
                cb(Utils::ResultError("Client error: " + error.error().message()));
            });
    }
}

void ToolInterface::sample(
    const Schema::CreateMessageRequestParams &params, const SampleResultCallback &cb) const
{
    if (d->isFinished()) {
        qCWarning(mcpServerLog) << "A finished tool should not ask for sampling.";
        cb(Utils::ResultError("Tool is already finished"));
        return;
    }

    const bool wantsTask = params.task().has_value();
    if (wantsTask) {
        qCWarning(mcpServerLog) << "Caller attempted to elicit with task parameters, "
                                   "which is not yet supported";
        cb(Utils::ResultError("Elicit does not support tasks"));
        return;
    }
    if (!d->_clientCapabilities.sampling()) {
        qCWarning(mcpServerLog) << "Caller attempted to sample, but client does not "
                                   "support sampling";
        cb(Utils::ResultError("Client does not support sampling"));
        return;
    }

    if (auto serverPrivate = d->_server.lock()) {
        serverPrivate->sendServerRequest(
            Schema::CreateMessageRequest().params(params),
            d->_sessionId, // sessionId is not needed for samples as they are one-off and not associated with a task
            [cb](const Schema::JSONRPCResponse &response) {
                Utils::Result<Schema::CreateMessageResult> r;

                if (std::holds_alternative<Schema::JSONRPCResultResponse>(response)) {
                    const auto &jsonRpcResult = std::get<Schema::JSONRPCResultResponse>(response);

                    auto createMessageResult = Schema::fromJson<Schema::CreateMessageResult>(
                        jsonRpcResult.result().additionalProperties());

                    if (!createMessageResult) {
                        qCWarning(mcpServerLog) << "Failed to parse sample result from client:"
                                                << createMessageResult.error();
                        cb(Utils::ResultError(
                            "Failed to parse sample result from client: "
                            + createMessageResult.error()));
                        return;
                    }

                    qCDebug(mcpServerLog) << "Received sample result from client:"
                                          << Schema::toJson(jsonRpcResult.result());

                    cb(Utils::Result<Schema::CreateMessageResult>(createMessageResult));
                    return;
                }

                const auto &error = std::get<Schema::JSONRPCErrorResponse>(response);
                qCWarning(mcpServerLog)
                    << "Received sample error from client:" << Schema::toJson(error.error());
                r = Utils::ResultError("Client error: " + error.error().message());
                cb(Utils::ResultError("Client error: " + error.error().message()));
            });
    }
}

void ToolInterface::notify(const Schema::ServerNotification &notification) const
{
    if (d->isFinished()) {
        qCWarning(mcpServerLog) << "A finished tool should not send notifications";
        return;
    }

    if (auto serverPrivate = d->_server.lock()) {
        serverPrivate->sendNotification(notification, d->_sessionId);
    }
}

void ToolInterface::finish(const Utils::Result<Schema::CallToolResult> &result) const
{
    if (d->isFinished() || d->_isTask) {
        qCWarning(mcpServerLog)
            << "Attempted to finish a tool that is already finished or started a task";
        return;
    }

    d->_isFinished = true;

    if (!result) {
        d->_responder.write(QJsonDocument(makeResponse(
            d->_initialRequestId,
            Schema::CallToolResult().isError(true).content(
                {Schema::TextContent().text(result.error())}))));
        return;
    }

    d->_responder.write(QJsonDocument(makeResponse(d->_initialRequestId, *result)));
}

Utils::Result<ToolInterface::TaskProgressNotify> ToolInterface::startTask(
    std::optional<int> pollingIntervalMs,
    const UpdateTaskCallback &onUpdateTask,
    const TaskResultCallback &onResultCallback,
    const std::optional<CancelTaskCallback> &onCancelTaskCallback,
    std::optional<int> ttlMs) const
{
    if (d->isFinished() || d->_isTask) {
        qCWarning(mcpServerLog)
            << "Attempted to finish a tool that is already finished or started a task";
        return Utils::ResultError(
            "Attempted to start a task for a tool that is already finished or started a "
            "task");
    }

    d->_isTask = true;

    if (auto serverPrivate = d->_server.lock()) {
        auto taskId = QUuid::createUuid().toString();
        auto task = Schema::Task()
                        .taskId(taskId)
                        .ttl(ttlMs)
                        .status(Schema::TaskStatus::working)
                        .pollInterval(pollingIntervalMs)
                        .createdAt(QDateTime::currentDateTime().toString(Qt::ISODate))
                        .lastUpdatedAt(QDateTime::currentDateTime().toString(Qt::ISODate));

        const auto callbacks
            = ServerPrivate::TaskCallbacks{onUpdateTask, onResultCallback, onCancelTaskCallback};
        serverPrivate->m_tasks.insert(
            std::make_pair(taskId, ServerPrivate::TaskAndCallbacks(task, callbacks, d->_server)));

        QJsonObject json = Schema::toJson(
            Schema::JSONRPCResultResponse()
                .id(d->_initialRequestId)
                .result(
                    Schema::Result().additionalProperties(
                        Schema::toJson(Schema::CreateTaskResult().task(task)))));

        d->_responder.write(QJsonDocument(json));

        auto notifyTaskUpdate =
            [weak = d->_server, taskId, sessionId = d->_sessionId](
                const Schema::TaskStatus &status,
                const std::optional<QString> &statusMessage,
                const std::optional<int> &ttl) {
                if (auto d = weak.lock()) {
                    auto it = d->m_tasks.find(taskId);
                    if (it == d->m_tasks.end()) {
                        qCWarning(mcpServerLog)
                            << "Attempted to update non-existent task with ID" << taskId;
                        return;
                    }
                    auto task = it->second.task;
                    it->second
                        .update(task.status(status).statusMessage(statusMessage).ttl(ttl), weak);

                    auto params = Schema::TaskStatusNotificationParams()
                                      .taskId(taskId)
                                      .status(it->second.task.status())
                                      .createdAt(it->second.task.createdAt())
                                      .lastUpdatedAt(it->second.task.lastUpdatedAt())
                                      .pollInterval(*it->second.task.pollInterval())
                                      .statusMessage(*it->second.task.statusMessage())
                                      .ttl(*it->second.task.ttl());

                    d->sendNotification(Schema::TaskStatusNotification().params(params), sessionId);
                }
            };

        return notifyTaskUpdate;
    }

    return Utils::ResultError("Failed to start task: Server instance no longer exists");
}

} // namespace Mcp
