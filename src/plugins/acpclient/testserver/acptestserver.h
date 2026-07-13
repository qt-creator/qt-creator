// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QStringList>

#include <functional>
#include <optional>

namespace AcpTestServer {

// Behavior switches, selected via command line arguments. See main.cpp.
class ServerScenario
{
public:
    bool requireAuth = false;      // --require-auth
    int seededSessions = 0;        // --sessions <N>
    bool permission = false;       // --permission
    bool waitForCancel = false;    // --cancel
    bool crashOnPrompt = false;    // --crash-on-prompt
    bool configOptions = false;    // --config-options
    bool modes = false;            // --modes
    bool allUpdateKinds = false;   // --updates-all
    bool stderrNoise = false;      // --stderr-noise
    bool invalidResponse = false;  // --invalid-response-on-prompt
    int chunks = 3;                // --chunks <N>
};

// A minimal, deterministic ACP agent speaking newline-delimited JSON-RPC 2.0
// on stdio. All reads are blocking; everything the server owes is written and
// flushed before it blocks on the next read.
class Server
{
public:
    explicit Server(const ServerScenario &scenario);

    int run();

private:
    void dispatch(const QJsonObject &message);
    void handleRequest(const QJsonValue &id, const QString &method, const QJsonObject &params);
    void handleNotification(const QString &method, const QJsonObject &params);

    void handleInitialize(const QJsonValue &id, const QJsonObject &params);
    void handleAuthenticate(const QJsonValue &id, const QJsonObject &params);
    void handleNewSession(const QJsonValue &id, const QJsonObject &params);
    void handleListSessions(const QJsonValue &id, const QJsonObject &params);
    void handleLoadSession(const QJsonValue &id, const QJsonObject &params);
    void handleDeleteSession(const QJsonValue &id, const QJsonObject &params);
    void handleCloseSession(const QJsonValue &id, const QJsonObject &params);
    void handlePrompt(const QJsonValue &id, const QJsonObject &params);
    void handleSetMode(const QJsonValue &id, const QJsonObject &params);
    void handleSetConfigOption(const QJsonValue &id, const QJsonObject &params);

    void sendResult(const QJsonValue &id, const QJsonObject &result);
    void sendError(const QJsonValue &id, int code, const QString &message);
    void sendNotification(const QString &method, const QJsonObject &params);
    void sendAgentMessageChunk(const QString &sessionId, const QString &text);
    void writeLine(const QJsonObject &message);
    void writeRawLine(const QByteArray &line);
    void noise(const char *context);

    QJsonArray configOptionsJson() const;

    // Blocks reading stdin, dispatching messages that do not match the
    // predicate, until a matching message arrives or stdin is closed.
    std::optional<QJsonObject> readUntil(const std::function<bool(const QJsonObject &)> &pred);

    ServerScenario m_scenario;
    bool m_authenticated = false;
    bool m_quitRequested = false;
    bool m_autoApprove = false;
    QString m_model = "small";
    QString m_currentModeId = "ask";
    QStringList m_sessions;
    int m_nextSessionNumber = 1;
    // Ids for server -> client requests. Distinct range from typical client
    // ids to keep protocol logs unambiguous.
    qint64 m_nextOutgoingId = 1000;
};

} // namespace AcpTestServer
