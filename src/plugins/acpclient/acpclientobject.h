// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <acp/acp.h>

#include <QHash>
#include <QObject>

#include <functional>

namespace AcpClient::Internal {

class AcpInspector;
class AcpTransport;

class AcpClientObject : public QObject
{
    Q_OBJECT

public:
    enum class State {
        Disconnected,
        Connecting,
        InitializeRequested,
        Initialized,
        ShuttingDown,
    };

    using ResponseCallback = std::function<void(const QJsonObject &result, const std::optional<Acp::Error> &error)>;

    explicit AcpClientObject(AcpTransport *transport, QObject *parent = nullptr);
    ~AcpClientObject() override;

    void setInspector(AcpInspector *inspector, const QString &clientName);

    State state() const { return m_state; }

    // Outgoing requests (client → agent)
    void initialize(const Acp::InitializeRequest &request, ResponseCallback callback = {});
    void authenticate(const Acp::AuthenticateRequest &request, ResponseCallback callback = {});
    void newSession(const Acp::NewSessionRequest &request, ResponseCallback callback = {});
    void loadSession(const Acp::LoadSessionRequest &request, ResponseCallback callback = {});
    void prompt(const Acp::PromptRequest &request, ResponseCallback callback = {});
    void cancelSession(const Acp::CancelNotification &notification);
    void setSessionMode(const Acp::SetSessionModeRequest &request, ResponseCallback callback = {});
    void setSessionConfigOption(const Acp::SetSessionConfigOptionRequest &request, ResponseCallback callback = {});

    // Respond to incoming agent requests (QJsonValue carries the raw JSON-RPC id)
    void sendResponse(const QJsonValue &id, const QJsonObject &result);
    void sendErrorResponse(const QJsonValue &id, int code, const QString &message);

signals:
    void stateChanged(State state);
    void initializeResult(const Acp::InitializeResponse &response);

    // Session notifications
    void sessionUpdate(const QString &sessionId, const Acp::SessionUpdate &update);

    // Client-side method requests from agent (QJsonValue carries the raw JSON-RPC id)
    void createTerminalRequested(const QJsonValue &id, const Acp::CreateTerminalRequest &request);
    void terminalOutputRequested(const QJsonValue &id, const Acp::TerminalOutputRequest &request);
    void waitForTerminalExitRequested(const QJsonValue &id, const Acp::WaitForTerminalExitRequest &request);
    void killTerminalRequested(const QJsonValue &id, const Acp::KillTerminalCommandRequest &request);
    void releaseTerminalRequested(const QJsonValue &id, const Acp::ReleaseTerminalRequest &request);
    void readTextFileRequested(const QJsonValue &id, const Acp::ReadTextFileRequest &request);
    void writeTextFileRequested(const QJsonValue &id, const Acp::WriteTextFileRequest &request);
    void requestPermissionRequested(const QJsonValue &id, const Acp::RequestPermissionRequest &request);

    void errorOccurred(const QString &message);

private:
    void handleMessage(const QJsonObject &message);
    void handleRequest(const QJsonValue &id, const QString &method, const QJsonObject &params);
    void handleNotification(const QString &method, const QJsonObject &params);
    void handleResponse(const QJsonValue &id, const QJsonObject &message);
    qint64 sendRequest(const QString &method, const QJsonObject &params, ResponseCallback callback);
    void sendNotification(const QString &method, const QJsonObject &params);
    void setState(State state);

    AcpTransport *m_transport = nullptr;
    AcpInspector *m_inspector = nullptr;
    QString m_clientName;
    State m_state = State::Disconnected;
    qint64 m_nextId = 1;
    QHash<qint64, ResponseCallback> m_pendingRequests;
};

} // namespace AcpClient::Internal
