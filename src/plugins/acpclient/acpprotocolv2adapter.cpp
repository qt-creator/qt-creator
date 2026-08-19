// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "acpprotocolv2adapter.h"

#include "acpclientobject.h"
#include "acpclienttr.h"

#include <QJsonArray>
#include <QLoggingCategory>

static Q_LOGGING_CATEGORY(logAdapter, "qtc.acpclient.v2adapter", QtWarningMsg);

using namespace Acp::V2;

namespace AcpClient::Internal {

AcpProtocolV2Adapter::AcpProtocolV2Adapter(AcpClientObject *client,
                                           const InitializeResponse &response,
                                           QObject *parent)
    : AcpProtocolAdapter(client, parent)
    , m_initializeResponse(response)
{
    connect(m_client, &AcpClientObject::notificationReceived,
            this, &AcpProtocolV2Adapter::handleNotification);
    connect(m_client, &AcpClientObject::requestReceived,
            this, &AcpProtocolV2Adapter::handleRequest);
}

QList<AuthMethod> AcpProtocolV2Adapter::authMethods() const
{
    return m_initializeResponse.authMethods().value_or(QList<AuthMethod>{});
}

// Presence of capabilities.session commits the agent to the baseline session
// methods, including session/list and session/close.
bool AcpProtocolV2Adapter::supportsSessionList() const
{
    const auto &caps = m_initializeResponse.capabilities();
    return caps && caps->session().has_value();
}

bool AcpProtocolV2Adapter::supportsSessionClose() const
{
    return supportsSessionList();
}

bool AcpProtocolV2Adapter::supportsSessionDelete() const
{
    const auto &caps = m_initializeResponse.capabilities();
    return caps && caps->session().has_value() && caps->session()->delete_().has_value();
}

bool AcpProtocolV2Adapter::supportsImagePrompt() const
{
    const auto &caps = m_initializeResponse.capabilities();
    if (!caps || !caps->session().has_value())
        return false;
    const Patch<PromptCapabilities> &promptCaps = caps->session()->prompt();
    return promptCaps.has_value() && promptCaps->image().has_value();
}

bool AcpProtocolV2Adapter::supportsEmbeddedContext() const
{
    const auto &caps = m_initializeResponse.capabilities();
    if (!caps || !caps->session().has_value())
        return false;
    const Patch<PromptCapabilities> &promptCaps = caps->session()->prompt();
    return promptCaps.has_value() && promptCaps->embeddedContext().has_value();
}

void AcpProtocolV2Adapter::emitConfigOptions(const QJsonArray &options)
{
    QList<SessionConfigOption> result;
    for (const QJsonValue &item : options) {
        if (const auto option = fromJson<SessionConfigOption>(item))
            result.append(*option);
        else
            qCWarning(logAdapter) << "Dropping invalid config option:" << item;
    }
    emit configOptionsReceived(result);
}

void AcpProtocolV2Adapter::newSession(const QString &cwd, const QJsonArray &mcpServers)
{
    QJsonObject params;
    params["cwd"] = cwd;
    if (!mcpServers.isEmpty())
        params["mcpServers"] = mcpServers;
    m_client->sendRequest(QStringLiteral("session/new"), params,
                          [this](const QJsonObject &result,
                                 const std::optional<Acp::Error> &error) {
        if (error) {
            if (error->code() == ErrorCode::Authentication_required) {
                const QList<AuthMethod> methods = authMethods();
                if (!methods.isEmpty()) {
                    emit authenticationRequired(methods);
                    return;
                }
            }
            emit errorOccurred(Tr::tr("Session error: %1").arg(error->message()));
            return;
        }
        const QString sessionId = result.value("sessionId").toString();
        if (sessionId.isEmpty()) {
            emit errorOccurred(Tr::tr("Session error: invalid response."));
            return;
        }
        emit sessionCreated(sessionId);
        if (result.value("configOptions").isArray())
            emitConfigOptions(result.value("configOptions").toArray());
        qCDebug(logAdapter) << "Session created:" << sessionId;
    });
}

void AcpProtocolV2Adapter::resumeSession(const QString &sessionId, const QString &cwd,
                                         const QJsonArray &mcpServers)
{
    QJsonObject params;
    params["sessionId"] = sessionId;
    params["cwd"] = cwd;
    if (!mcpServers.isEmpty())
        params["mcpServers"] = mcpServers;
    params["replayFrom"] = QJsonObject{{"type", QStringLiteral("start")}};
    m_client->sendRequest(QStringLiteral("session/resume"), params,
                          [this, sessionId](const QJsonObject &result,
                                            const std::optional<Acp::Error> &error) {
        if (error) {
            emit errorOccurred(Tr::tr("Failed to resume session: %1").arg(error->message()));
            return;
        }
        emit sessionResumed(sessionId);
        if (result.value("configOptions").isArray())
            emitConfigOptions(result.value("configOptions").toArray());
    });
}

void AcpProtocolV2Adapter::listSessions(const std::optional<QString> &cursor)
{
    QJsonObject params;
    if (cursor)
        params["cursor"] = *cursor;
    m_client->sendRequest(QStringLiteral("session/list"), params,
                          [this](const QJsonObject &result,
                                 const std::optional<Acp::Error> &error) {
        if (error) {
            emit errorOccurred(Tr::tr("Failed to list sessions: %1").arg(error->message()));
            return;
        }
        if (!result.value("sessions").isArray()) {
            emit errorOccurred(Tr::tr("Failed to list sessions: invalid response."));
            return;
        }
        QList<SessionInfo> sessions;
        for (const QJsonValue &item : result.value("sessions").toArray()) {
            if (const auto session = fromJson<SessionInfo>(item))
                sessions.append(*session);
        }
        std::optional<QString> nextCursor;
        if (result.value("nextCursor").isString())
            nextCursor = result.value("nextCursor").toString();
        emit sessionsListed(sessions, nextCursor);
    });
}

void AcpProtocolV2Adapter::deleteSession(const QString &sessionId)
{
    m_client->sendRequest(QStringLiteral("session/delete"), {{"sessionId", sessionId}},
                          [this, sessionId](const QJsonObject &,
                                            const std::optional<Acp::Error> &error) {
        if (error) {
            emit errorOccurred(Tr::tr("Failed to delete session: %1").arg(error->message()));
            return;
        }
        emit sessionDeleted(sessionId);
    });
}

void AcpProtocolV2Adapter::closeSession(const QString &sessionId)
{
    m_client->sendRequest(QStringLiteral("session/close"), {{"sessionId", sessionId}},
                          [this, sessionId](const QJsonObject &,
                                            const std::optional<Acp::Error> &error) {
        if (error) {
            emit errorOccurred(Tr::tr("Failed to close session: %1").arg(error->message()));
            return;
        }
        emit sessionClosed(sessionId);
    });
}

// The response only acknowledges acceptance of the prompt; the turn ends with
// the idle state_update handled in handleNotification().
void AcpProtocolV2Adapter::prompt(const QString &sessionId, const QJsonArray &content)
{
    m_turnActive = true;
    QJsonObject params;
    params["sessionId"] = sessionId;
    params["prompt"] = content;
    m_client->sendRequest(QStringLiteral("session/prompt"), params,
                          [this](const QJsonObject &, const std::optional<Acp::Error> &error) {
        if (error) {
            emit errorOccurred(Tr::tr("Prompt error: %1").arg(error->message()));
            finishTurn({});
        }
    });
}

// The turn ends either on a failed acknowledgement or on the idle state that
// follows it, whichever arrives first.
void AcpProtocolV2Adapter::finishTurn(const std::optional<StopReason> &stopReason)
{
    if (!m_turnActive)
        return;
    m_turnActive = false;
    emit promptFinished(stopReason);
}

void AcpProtocolV2Adapter::cancel(const QString &sessionId)
{
    m_client->sendNotification(QStringLiteral("session/cancel"), {{"sessionId", sessionId}});
}

void AcpProtocolV2Adapter::authenticate(const QString &methodId)
{
    m_client->sendRequest(QStringLiteral("auth/login"), {{"methodId", methodId}},
                          [this](const QJsonObject &, const std::optional<Acp::Error> &error) {
        if (error) {
            QString errorMsg = error->message();
            if (const std::optional<QJsonValue> data = error->data(); data && data->isString()) {
                if (const QString dataString = data->toString(); !dataString.isEmpty())
                    errorMsg.append("\n" + dataString);
            }
            emit authenticationFailed(errorMsg);
            return;
        }
        emit authenticated();
    });
}

void AcpProtocolV2Adapter::setConfigOption(const QString &sessionId, const QString &configId,
                                           const QJsonValue &value)
{
    QJsonObject params;
    params["sessionId"] = sessionId;
    params["configId"] = configId;
    params["value"] = value;
    params["type"] = value.isBool() ? QStringLiteral("boolean") : QStringLiteral("id");
    m_client->sendRequest(QStringLiteral("session/set_config_option"), params,
                          [this](const QJsonObject &result,
                                 const std::optional<Acp::Error> &error) {
        if (error) {
            emit errorOccurred(Tr::tr("Failed to set config option: %1").arg(error->message()));
            return;
        }
        if (result.value("configOptions").isArray())
            emitConfigOptions(result.value("configOptions").toArray());
    });
}

void AcpProtocolV2Adapter::handleNotification(const QString &method, const QJsonObject &params)
{
    if (method != QLatin1String("session/update"))
        return;

    const QString sessionId = params.value("sessionId").toString();
    const auto update = fromJson<SessionUpdate>(params.value("update"));
    if (!update) {
        qCWarning(logAdapter) << "Invalid session update:" << params << update.error();
        return;
    }

    if (update->kind() == QLatin1String("state_update")) {
        if (const StateUpdate *state = update->get<StateUpdate>()) {
            if (const auto *idle = std::get_if<IdleStateUpdate>(state))
                finishTurn(idle->stopReason().asOptional());
        }
    }

    emit sessionUpdate(sessionId, *update);
}

void AcpProtocolV2Adapter::handleRequest(const QJsonValue &id, const QString &method,
                                         const QJsonObject &params)
{
    if (method == QLatin1String("session/request_permission")) {
        // A parse failure is answered with Invalid_params by AcpClientObject,
        // which fails both the v1 and the v2 parse in that case.
        const auto request = fromJson<RequestPermissionRequest>(QJsonValue(params));
        if (!request) {
            qCWarning(logAdapter) << "Invalid permission request:" << params << request.error();
            return;
        }
        emit permissionRequested(id, *request);
        return;
    }

    // v2 removed the client-side fs/* and terminal/* method surface.
    if (method.startsWith(QLatin1String("fs/")) || method.startsWith(QLatin1String("terminal/"))) {
        m_client->sendErrorResponse(id, ErrorCode::Method_not_found,
                                    Tr::tr("Method not available in ACP v2: %1").arg(method));
    }
}

} // namespace AcpClient::Internal
