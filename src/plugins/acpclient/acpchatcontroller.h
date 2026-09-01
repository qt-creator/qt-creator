// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "acpclientobject.h"
#include "acpelicitationhandler.h"
#include "acpsettings.h"
#include "chatpanel.h"

#include <acp/acp.h>
#include <acp/acpv2.h>

#include <QJsonValue>
#include <QObject>

#include <utils/filepath.h>

namespace AcpClient::Internal {

class AcpClientObject;
class AcpFilesystemHandler;
class AcpInspector;
class AcpPermissionHandler;
class AcpProtocolAdapter;
class AcpTerminalHandler;
class AcpTransport;

class AcpChatController : public QObject
{
    Q_OBJECT

public:
    explicit AcpChatController(QObject *parent = nullptr);
    ~AcpChatController() override;

    void setInspector(AcpInspector *inspector);
    void showInspector();

    void connectToServer(const QString &serverId);
    void connectToServer(const AcpSettings::ServerInfo &serverInfo);
    void disconnectFromServer();

    void createNewSession(const Utils::FilePath &workingDirectory = {});
    void listSessions(const std::optional<QString> &cursor = {});
    void loadSession(const QString &sessionId, const Utils::FilePath &workingDirectory);
    void sendPrompt(const QString &text, const QList<Utils::FilePath> &additionalFiles = {},
                    bool includeCurrentEditor = true,
                    const QList<TextContext> &textContexts = {},
                    const QList<ImageContext> &imageContexts = {});
    void cancelPrompt();
    void authenticate(const QString &methodId);
    void setConfigOption(const QString &configId, const QJsonValue &value);
    void sendPermissionResponse(const QJsonValue &id, const QString &optionId);
    void sendPermissionCancelled(const QJsonValue &id);
    void sendElicitationAccepted(const QJsonValue &id, const QJsonObject &content);
    void sendElicitationDeclined(const QJsonValue &id);
    void sendElicitationCancelled(const QJsonValue &id);
    void deleteSession(const QString &sessionId);
    void closeSession();

    bool isInitialized() const { return m_initialized; }
    bool supportsSessionList() const;
    bool supportsSessionDelete() const;
    bool supportsSessionClose() const;
    bool supportsImagePrompt() const;
    const QString &sessionId() const { return m_sessionId; }
    QString displayName() const;
    const QString &agentName() const { return m_agentName; }
    const QString &agentVersion() const { return m_agentVersion; }

signals:
    void connectionStateChanged(AcpClientObject::State state);
    void agentInfoReceived(const QString &name, const QString &version, const QString &iconUrl);
    void sessionSelectionRequired();
    void sessionCreated(const QString &sessionId);
    void sessionLoaded(const QString &sessionId);
    void sessionsListed(const QList<Acp::V2::SessionInfo> &sessions,
                        const std::optional<QString> &nextCursor);
    void sessionDeleted(const QString &sessionId);
    void sessionClosed(const QString &sessionId);
    void configOptionsReceived(const QList<Acp::V2::SessionConfigOption> &configOptions);
    void sessionUpdate(const QString &sessionId, const Acp::V2::SessionUpdate &update);
    void authenticationRequired(const QList<Acp::V2::AuthMethod> &methods);
    void authenticationFailed(const QString &error);
    void permissionRequested(const QJsonValue &id, const Acp::V2::RequestPermissionRequest &request);
    void permissionCancelledByAgent(const QJsonValue &id);
    void elicitationRequested(const QJsonValue &id, const ElicitationRequest &request);
    void elicitationCancelledByAgent(const QJsonValue &id);
    void elicitationCompletedByAgent(const QJsonValue &id);
    void promptFinished();
    void errorOccurred(const QString &error);

public:
    // The hybrid v1+v2 initialize request parameters. Exposed for testing.
    static QJsonObject buildInitializeParams();

private:
    void onInitializeResult(const QJsonObject &result);
    void setUpV1Adapter(const Acp::InitializeResponse &response);
    void setUpV2Adapter(const Acp::V2::InitializeResponse &response,
                        const QJsonObject &rawCapabilities);
    void connectAdapter();
    QJsonArray buildMcpServersJson() const;

    AcpInspector *m_inspector = nullptr;
    AcpTransport *m_transport = nullptr;
    AcpClientObject *m_client = nullptr;
    AcpProtocolAdapter *m_adapter = nullptr;
    AcpTerminalHandler *m_terminalHandler = nullptr;
    AcpFilesystemHandler *m_filesystemHandler = nullptr;
    AcpPermissionHandler *m_permissionHandler = nullptr;
    AcpElicitationHandler *m_elicitationHandler = nullptr;

    Utils::FilePath m_workingDirectory;
    QString m_sessionId;
    QString m_agentName;
    QString m_agentVersion;
    QString m_serverName;
    QString m_iconUrl;
    bool m_initialized = false;
};

} // namespace AcpClient::Internal
