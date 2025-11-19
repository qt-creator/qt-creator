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

private:
    QJsonObject processRequest(const QJsonObject &request);
    QJsonObject createErrorResponse(
        int code, const QString &message, const QJsonValue &id = QJsonValue::Null);
    QJsonObject createSuccessResponse(
        const QJsonValue &result, const QJsonValue &id = QJsonValue::Null);
    void sendResponse(QTcpSocket *client, const QJsonObject &response);

    // HTTP handling methods
    bool isHttpRequest(const QByteArray &data);
    void handleHttpRequest(QTcpSocket *client, const HttpParser::HttpRequest &request);
    void sendHttpResponse(QTcpSocket *client, const QByteArray &httpResponse);

private:
    QTcpServer *m_tcpServerP;
    HttpParser *m_httpParserP;
    QList<QTcpSocket *> m_clients;
    MCPCommands *m_commandsP;
    quint16 m_port;
};

} // namespace MCP::Internal
