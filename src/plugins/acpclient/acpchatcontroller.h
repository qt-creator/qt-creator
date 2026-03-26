// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <acp/acp.h>

#include <QJsonValue>
#include <QObject>

#include <utils/filepath.h>

namespace AcpClient::Internal {

class AcpClientObject;
class AcpFilesystemHandler;
class AcpInspector;
class AcpPermissionHandler;
class AcpTerminalHandler;
class AcpTransport;

class AcpChatController : public QObject
{
    Q_OBJECT

public:
    explicit AcpChatController(QObject *parent = nullptr);
    ~AcpChatController() override;

    void setInspector(AcpInspector *inspector);

    void connectToServer(const QString &serverId, const Utils::FilePath &workingDirectory);
    void disconnectFromServer();

    void createNewSession();
    void sendPrompt(const QString &text);
    void cancelPrompt();
    void authenticate(const QString &methodId);
    void setConfigOption(const QString &configId, const QString &value);
    void sendPermissionResponse(const QJsonValue &id, const QString &optionId);
    void sendPermissionCancelled(const QJsonValue &id);

    bool isConnected() const { return m_connected; }
    bool isInitialized() const { return m_initialized; }
    const QString &sessionId() const { return m_sessionId; }
    const QString &agentName() const { return m_agentName; }
    const QString &agentVersion() const { return m_agentVersion; }

signals:
    void connectionStateChanged(bool connected);
    void agentInfoReceived(const QString &name, const QString &version, const QString &iconUrl);
    void sessionCreated(const QString &sessionId);
    void configOptionsReceived(const QList<Acp::SessionConfigOption> &configOptions);
    void sessionUpdate(const QString &sessionId, const Acp::SessionUpdate &update);
    void authenticationRequired(const QList<Acp::AuthMethod> &methods);
    void authenticationFailed(const QString &error);
    void permissionRequested(const QJsonValue &id, const Acp::RequestPermissionRequest &request);
    void promptFinished();
    void errorOccurred(const QString &error);

private:
    void onStateChanged(int state);
    void onInitializeResult(const Acp::InitializeResponse &response);
    Acp::NewSessionRequest buildNewSessionRequest() const;

    AcpInspector *m_inspector = nullptr;
    AcpTransport *m_transport = nullptr;
    AcpClientObject *m_client = nullptr;
    AcpTerminalHandler *m_terminalHandler = nullptr;
    AcpFilesystemHandler *m_filesystemHandler = nullptr;
    AcpPermissionHandler *m_permissionHandler = nullptr;

    Utils::FilePath m_workingDirectory;
    QString m_sessionId;
    QString m_agentName;
    QString m_agentVersion;
    QString m_serverName;
    QString m_iconUrl;
    QList<Acp::AuthMethod> m_authMethods;
    bool m_connected = false;
    bool m_initialized = false;
};

} // namespace AcpClient::Internal
