// Copyright (C) 2025 David M. Cotter
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <QHttpServerResponse>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>
#include <QTimer>

#include "mcpcommands.h"

class QHttpServer;
class QHttpServerRequest;
class QTcpServer;
class QTcpSocket;

namespace MCP::Internal {

class MCPServer : public QObject
{
    Q_OBJECT

public:
    explicit MCPServer(QObject *parent = nullptr);
    ~MCPServer();

    bool start(quint16 port = 3001);
    void stop();
    bool isRunning() const;
    quint16 getPort() const;

    // Public method to call MCP methods directly
    QJsonObject callMCPMethod(const QString &method, const QJsonValue &params = QJsonValue());

private slots:
    void handleNewConnection();
    void handleClientData();
    void handleClientDisconnected();

    QHttpServerResponse onHttpGet(const QHttpServerRequest &req);
    QHttpServerResponse onHttpPost(const QHttpServerRequest &req);
    QHttpServerResponse onHttpOptions(const QHttpServerRequest &req);

private:
    QJsonObject processRequest(const QJsonObject &request);
    QJsonObject createErrorResponse(
        int code, const QString &message, const QJsonValue &id = QJsonValue::Null);
    QJsonObject createSuccessResponse(
        const QJsonValue &result, const QJsonValue &id = QJsonValue::Null);
    void sendResponse(QTcpSocket *client, const QJsonObject &response);

private:
    QTcpServer *m_tcpServerP;
    QTcpServer *m_httpTcpServerP;
    QHttpServer *m_httpServerP;
    QList<QTcpSocket *> m_clients;
    MCPCommands *m_commandsP;
    quint16 m_port;
};

} // namespace MCP::Internal
