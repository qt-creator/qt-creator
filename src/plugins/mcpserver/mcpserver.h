// Copyright (C) 2025 David M. Cotter
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "mcpcommands.h"

#include <QHttpServerResponse>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>
#include <QTimer>

class QHttpServer;
class QHttpServerRequest;
class QTcpServer;
class QTcpSocket;

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
    QJsonObject callMCPMethod(
        const QString &method, const QJsonObject &params = {}, const QJsonValue &id = {});

private slots:
    void handleNewConnection();
    void handleClientData();
    void handleClientDisconnected();

    QHttpServerResponse onHttpGet(const QHttpServerRequest &req);
    QHttpServerResponse onHttpPost(const QHttpServerRequest &req);
    QHttpServerResponse onHttpOptions(const QHttpServerRequest &req);

private:
    QJsonObject createErrorResponse(
        int code, const QString &message, const QJsonValue &id = QJsonValue::Null);
    QJsonObject createSuccessResponse(
        const QJsonValue &result, const QJsonValue &id = QJsonValue::Null);
    void sendResponse(QTcpSocket *client, const QJsonObject &response);

private:
    using MethodHandler
        = std::function<QJsonObject(const QJsonObject &params, const QJsonValue &id)>;
    QHash<QString, MethodHandler> m_methods;

    using ToolHandler = std::function<QJsonObject(const QJsonObject &params)>;
    QHash<QString, ToolHandler> m_toolHandlers;
    QJsonArray m_toolList;

    QTcpServer *m_tcpServerP;
    QTcpServer *m_httpTcpServerP;
    QHttpServer *m_httpServerP;
    QList<QTcpSocket *> m_clients;
    McpCommands *m_commandsP;
    quint16 m_port;
};

} // namespace Mcp::Internal
