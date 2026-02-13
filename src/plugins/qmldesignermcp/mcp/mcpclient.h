// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <functional>
#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QPointer>
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
    QJsonObject inputSchema; // JSON Schema object if provided
};

struct McpResource
{
    QString name;
    QString description;
    QString uri;
    QString mimeType;
};

/*!
 * \class McpClient
 * \brief Host-side connector that talks to a single MCP server via STDIO.
 *
 * McpClient spawns the MCP server with QProcess (STDIO transport) and exchanges
 * newline-delimited JSON-RPC 2.0 messages. After a successful `initialize` request,
 * it sends an `initialized` notification and is ready to list and call MCP tools.
 */

class McpClient : public QObject
{
    Q_OBJECT
public:
    explicit McpClient(const QString &clientName, const QString &clientVersion, QObject *parent = nullptr);
    ~McpClient() override;

    bool startProcess(
        const QString &command,
        const QStringList &args = {},
        const QString &workingDir = {},
        const QProcessEnvironment &env = QProcessEnvironment::systemEnvironment());

    // Send polite termination; kills if it doesn't exit fast.
    void stop(int killTimeoutMs = 1500);

    bool isRunning() const;

    qint64 initialize();

    qint64 listTools();
    qint64 callTool(const QString &toolName, const QJsonObject &arguments);

    qint64 listResources();
    qint64 readResource(const QString &uri);

    using ResponseHandler
        = std::function<void(qint64 requestId, const QJsonObject &result, const QJsonObject &error)>;

    qint64 sendRequest(
        const QString &method,
        const QJsonObject &params = QJsonObject(),
        ResponseHandler callback = nullptr);

signals:
    // Process & connection state
    void started();
    void exited(int exitCode, QProcess::ExitStatus status);
    void errorOccurred(const QString &message);
    void connected(const QmlDesigner::McpServerInfo &info);

    // Logging (server stderr or client warnings)
    void logMessage(const QString &line);

    // Core results
    void toolsListed(const QList<QmlDesigner::McpTool> &tools, qint64 requestId);
    void toolCallSucceeded(const QJsonObject &result, qint64 requestId);
    void toolCallFailed(const QString &errorMessage, const QJsonObject &errorObj, qint64 requestId);

    void resourcesListed(const QList<QmlDesigner::McpResource> &resources, qint64 requestId);
    void resourceReadSucceeded(const QJsonObject &result, qint64 requestId);
    void resourceReadFailed(
        const QString &errorMessage, const QJsonObject &errorObj, qint64 requestId);

    // Any JSON-RPC notification
    void notificationReceived(const QString &method, const QJsonObject &params);

private slots:
    void onStdoutReady();
    void onStderrReady();
    void onProcessStarted();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onProcessError(QProcess::ProcessError e);

private:
    struct PendingResponse
    {
        ResponseHandler callback;
        QString method;
    };

    void sendRpcToServer(const QJsonObject &obj);
    void handleIncomingLine(const QByteArray &line);
    void handleResponse(const QJsonObject &obj);
    void handleNotification(const QJsonObject &obj);

    void confirmClientInitialized();

    QPointer<QProcess> m_process;
    QByteArray m_stdoutBuffer;
    qint64 m_nextId = 1;
    QHash<qint64, PendingResponse> m_pendingResponses;
    bool m_initializedConfirmed = false;
    McpServerInfo m_serverInfo;
    QString m_clientName;
    QString m_clientVersion;
};

} // namespace QmlDesigner
