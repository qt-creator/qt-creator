// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "mcpclient.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QObject>

namespace QmlDesigner {

struct McpServerConfig
{
    enum Transport { Stdio, Http };

    Transport transport = Stdio;
    QString name;

    // STDIO:
    QString command;
    QStringList args;
    QString workingDir;
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

    // HTTP:
    QString url;
    QString bearerToken;
};

/**
 * @brief Host-level manager for multiple McpClient instances (one per server).
 *
 * Responsibilities:
 *  - Create/start/stop/restart servers as McpClient connections.
 *  - Route tool calls to a specific server.
 *  - Aggregate logs, notifications, and errors with server context.
 *  - Enforce simple allow-list policy and optional consent hook for tool calls.
 *
 * Non-responsibilities:
 *  - UI; AI planning/routing; persistent configuration storage.
 */
class McpHost : public QObject
{
    Q_OBJECT
public:
    explicit McpHost(
        const QString &protocolVersion,
        const QString &hostName,
        const QString &hostVersion,
        QObject *parent = nullptr);

    // --- Configuration ---
    void addServer(const McpServerConfig &cfg);
    bool hasServer(const QString &serverName) const;
    void removeServer(const QString &serverName);
    QStringList servers() const;

    // Allow-list management (optional)
    void setAllowedTools(const QString &serverName, const QSet<QString> &tools);
    QSet<QString> allowedTools(const QString &serverName) const;

    // User consent hook (optional) - return false to block a call
    using ConsentHook = std::function<
        bool(const QString &serverName, const QString &toolName, const QJsonObject &args)>;
    void setConsentHook(ConsentHook hook);
    void setRequiredConsent(const QString &serverName, const QSet<QString> &tools);
    QSet<QString> requiredConsent(const QString &serverName) const;

    // --- Lifecycle ---
    void startAll();
    void stopAll();

    bool startServer(const QString &serverName);
    void stopServer(const QString &serverName);

    // --- High-level routing ---
    qint64 listTools(const QString &serverName);
    qint64 callTool(const QString &serverName, const QString &toolName, const QJsonObject &arguments);

    // Generic request routing
    qint64 sendRequest(
        const QString &serverName,
        const QString &method,
        const QJsonObject &params = QJsonObject(),
        McpClient::ResponseHandler callback = nullptr);

    QSharedPointer<McpClient> client(const QString &serverName) const;

signals:
    // Lifecycle / status
    void serverStarting(const QString &serverName);
    void serverStarted(const QString &serverName, const QmlDesigner::McpServerInfo &info);
    void serverExited(const QString &serverName, int exitCode, QProcess::ExitStatus status);

    // Aggregated logs/errors
    void errorOccurred(const QString &serverName, const QString &message);
    void logMessage(const QString &serverName, const QString &line);

    // Aggregated results
    void toolsListed(
        const QString &serverName, const QList<QmlDesigner::McpTool> &tools, qint64 requestId);
    void toolCallSucceeded(const QString &serverName, const QJsonObject &result, qint64 requestId);
    void toolCallFailed(
        const QString &serverName,
        const QString &message,
        const QJsonObject &errorObj,
        qint64 requestId);

    // Notifications (with server context)
    void notificationReceived(
        const QString &serverName, const QString &method, const QJsonObject &params);

private:
    void wireClientSignals(const QString &name, const QSharedPointer<McpClient> &client);
    void scheduleRestart(const QString &name);
    void cancelRestart(const QString &name);

    // Policy checks
    bool isToolAllowed(const QString &serverName, const QString &toolName) const;
    bool askConsent(
        const QString &serverName, const QString &toolName, const QJsonObject &args) const;

    QMap<QString, McpServerConfig> m_serverConfigs;     // key: server name
    QMap<QString, QSharedPointer<McpClient>> m_clients; // key: server name
    QMap<QString, QTimer *> m_restartTimers;            // key: server name
    QMap<QString, QSet<QString>> m_allowedTools;        // key: server name
    QMap<QString, QSet<QString>> m_toolsRequireConsent; // key: server name
    ConsentHook m_consentHook;

    const QString m_protocolVersion;
    const QString m_hostName;
    const QString m_hostVersion;
};

} // namespace QmlDesigner
