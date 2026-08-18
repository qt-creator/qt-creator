// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <acp/acpv2.h>

#include <QJsonArray>
#include <QJsonValue>
#include <QObject>

#include <optional>

namespace AcpClient::Internal {

class AcpClientObject;

// Seam between the chat controller/UI and the wire protocol. The controller
// and UI speak the Acp::V2 vocabulary; adapters translate it to the protocol
// version negotiated for the connection.
class AcpProtocolAdapter : public QObject
{
    Q_OBJECT

public:
    explicit AcpProtocolAdapter(AcpClientObject *client, QObject *parent = nullptr);

    virtual int protocolVersion() const = 0;

    virtual void newSession(const QString &cwd, const QJsonArray &mcpServers) = 0;
    virtual void resumeSession(const QString &sessionId, const QString &cwd,
                               const QJsonArray &mcpServers) = 0;
    virtual void listSessions(const std::optional<QString> &cursor) = 0;
    virtual void deleteSession(const QString &sessionId) = 0;
    virtual void closeSession(const QString &sessionId) = 0;
    virtual void prompt(const QString &sessionId, const QJsonArray &content) = 0;
    virtual void cancel(const QString &sessionId) = 0;
    virtual void authenticate(const QString &methodId) = 0;
    virtual void setConfigOption(const QString &sessionId, const QString &configId,
                                 const QJsonValue &value) = 0;

    virtual bool supportsSessionList() const = 0;
    virtual bool supportsSessionDelete() const = 0;
    virtual bool supportsSessionClose() const = 0;
    virtual bool supportsImagePrompt() const = 0;
    virtual bool supportsEmbeddedContext() const = 0;
    virtual QList<Acp::V2::AuthMethod> authMethods() const = 0;

signals:
    void sessionCreated(const QString &sessionId);
    void sessionResumed(const QString &sessionId);
    void sessionsListed(const QList<Acp::V2::SessionInfo> &sessions,
                        const std::optional<QString> &nextCursor);
    void sessionDeleted(const QString &sessionId);
    void sessionClosed(const QString &sessionId);
    void configOptionsReceived(const QList<Acp::V2::SessionConfigOption> &configOptions);
    void sessionUpdate(const QString &sessionId, const Acp::V2::SessionUpdate &update);
    void permissionRequested(const QJsonValue &id,
                             const Acp::V2::RequestPermissionRequest &request);
    void authenticationRequired(const QList<Acp::V2::AuthMethod> &methods);
    void authenticationFailed(const QString &error);
    void authenticated();
    void promptFinished(const std::optional<Acp::V2::StopReason> &stopReason);
    void errorOccurred(const QString &error);

protected:
    AcpClientObject *m_client = nullptr;
};

} // namespace AcpClient::Internal
