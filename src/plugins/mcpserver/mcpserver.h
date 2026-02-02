// Copyright (C) 2025 David M. Cotter
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>

#include "httpparser.h"
#include "httpresponse.h"
#include "mcpcommands.h"

namespace Mcp::Internal {

class McpServer : public QObject
{
    Q_OBJECT

public:
    explicit McpServer(QObject *parent = nullptr);
    ~McpServer();

    bool start(quint16 port = 3001);
    void stop();
    bool isRunning() const;
    quint16 getPort() const;

    // Public method to call MCP methods directly
    using Callback = std::function<void(const QJsonObject &response)>;
    void callMCPMethod(
        const QString &method,
        const Callback &callback,
        const QJsonObject &params = {},
        const QJsonValue &id = {});

    using ToolHandler = std::function<void(const QJsonObject &params, const Callback &callback)>;
    void addTool(const QJsonObject &tool, ToolHandler handler);
    void initializeToolsForCommands();

private slots:
    void handleNewConnection();
    void handleClientData();
    void handleClientDisconnected();

private:
    QJsonObject createErrorResponse(
        int code, const QString &message, const QJsonValue &id = QJsonValue::Null);
    static QJsonObject createSuccessResponse(
        const QJsonValue &result, const QJsonValue &id = QJsonValue::Null);
    void sendResponse(QTcpSocket *client, const QJsonObject &response);

    // HTTP handling methods
    bool isHttpRequest(const QByteArray &data);
    void handleHttpRequest(QTcpSocket *client, const HttpParser::HttpRequest &request);
    static void sendHttpResponse(QTcpSocket *client, const QByteArray &httpResponse);
    void onHttpGet(QTcpSocket *client, const HttpParser::HttpRequest &request);
    void onHttpPost(QTcpSocket *client, const HttpParser::HttpRequest &request);
    void onHttpOptions(QTcpSocket *client, const HttpParser::HttpRequest &request);

    // SSE specific helpers
    void handleSseClient(QTcpSocket *client);
    void broadcastSseMessage(const QJsonObject &msg);
private:
    using MethodHandler = std::function<
        void(const QJsonObject &params, const QJsonValue &id, const Callback &callback)>;
    QHash<QString, MethodHandler> m_methods;

    QHash<QString, ToolHandler> m_toolHandlers;
    QJsonArray m_toolList;

    QTcpServer *m_tcpServerP;
    HttpParser *m_httpParserP;
    QList<QTcpSocket *> m_clients;
    QList<QTcpSocket *> m_sseClients;
    McpCommands m_commands;
    quint16 m_port;
    QHash<QTcpSocket*, QByteArray> m_partialData;
};

} // namespace Mcp::Internal
