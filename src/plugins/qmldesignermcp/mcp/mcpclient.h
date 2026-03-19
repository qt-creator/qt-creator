// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <functional>
#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QProcess>

namespace QmlDesigner {

struct McpServerInfo
{
    QString name;
    QString version;
};

struct McpTool
{
    QString name;
    QString description;
    QJsonObject inputSchema;
};

struct McpResource
{
    QString name;
    QString description;
    QString uri;
    QString mimeType;
};

/*!
    \brief Abstract base for MCP client transports.

    Owns all JSON-RPC bookkeeping (request IDs, pending response dispatch,
    initialize handshake, and tool/resource helpers) and declares the full
    signal surface used by McpHost. Concrete subclasses provide the transport:

      - McpClientStdio — stdio via QProcess
      - McpClientHttp  — Streamable HTTP via QNetworkAccessManager

    Subclasses must implement isRunning(), stop(), and sendRpcToServer() to
    write one JSON-RPC object to the server. They must also call
    handleIncomingLine() for every complete newline-delimited JSON frame
    received from the server.
*/
class McpClient : public QObject
{
    Q_OBJECT

public:
    explicit McpClient(const QString &clientName, const QString &clientVersion, QObject *parent = nullptr);
    ~McpClient() override;

    virtual bool isRunning() const = 0;
    virtual void stop(int killTimeoutMs = 1500) = 0;

    qint64 initialize();
    qint64 listTools();
    qint64 callTool(const QString &toolName, const QJsonObject &arguments);
    qint64 listResources();
    qint64 readResource(const QString &uri);

    using ResponseHandler = std::function<void(qint64 requestId, const QJsonObject &result,
                                               const QJsonObject &error)>;

    qint64 sendRequest(
        const QString &method,
        const QJsonObject &params = QJsonObject(),
        ResponseHandler callback = nullptr);

signals:
    void started();
    void exited(int exitCode, QProcess::ExitStatus status);
    void errorOccurred(const QString &message);
    void connected(const QmlDesigner::McpServerInfo &info);

    void logMessage(const QString &line);

    void toolsListed(const QList<QmlDesigner::McpTool> &tools, qint64 requestId);
    void toolCallSucceeded(const QJsonObject &result, qint64 requestId);
    void toolCallFailed(const QString &errorMessage, const QJsonObject &errorObj, qint64 requestId);

    void resourcesListed(const QList<QmlDesigner::McpResource> &resources, qint64 requestId);
    void resourceReadSucceeded(const QJsonObject &result, qint64 requestId);
    void resourceReadFailed(
        const QString &errorMessage, const QJsonObject &errorObj, qint64 requestId);

    void notificationReceived(const QString &method, const QJsonObject &params);

protected:
    // Transport interface
    virtual void sendRpcToServer(const QJsonObject &obj) = 0;

    // Called by subclasses for each complete JSON line received from the server
    void handleIncomingLine(const QByteArray &line);

    // Resets all session state; call from subclass stop()
    void resetSession();

    QString m_clientName;
    QString m_clientVersion;
    QString m_negotiatedProtocolVersion;

private:
    struct PendingResponse
    {
        ResponseHandler callback;
        QString method;
    };

    void handleResponse(const QJsonObject &obj);
    void handleNotification(const QJsonObject &obj);
    void confirmClientInitialized();

    qint64 m_nextId = 1;
    QHash<qint64, PendingResponse> m_pendingResponses;
    bool m_initializedConfirmed = false;
};

} // namespace QmlDesigner
