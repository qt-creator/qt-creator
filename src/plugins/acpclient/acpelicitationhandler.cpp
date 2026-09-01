// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "acpelicitationhandler.h"

#include "acpclientobject.h"

#include <QLoggingCategory>

static Q_LOGGING_CATEGORY(logElicitation, "qtc.acpclient.elicitation", QtWarningMsg);

using namespace Acp;

namespace AcpClient::Internal {

AcpElicitationHandler::AcpElicitationHandler(AcpClientObject *client, QObject *parent)
    : QObject(parent)
    , m_client(client)
{
    connect(client, &AcpClientObject::requestReceived,
            this, &AcpElicitationHandler::handleRequest);
    connect(client, &AcpClientObject::notificationReceived,
            this, &AcpElicitationHandler::handleNotification);
    connect(client, &AcpClientObject::requestCancelled,
            this, &AcpElicitationHandler::handleRequestCancelled);
}

void AcpElicitationHandler::handleRequest(const QJsonValue &id, const QString &method,
                                          const QJsonObject &params)
{
    if (method != QLatin1String("elicitation/create"))
        return;

    ElicitationRequest request;
    request.message = params.value("message").toString();

    const QString mode = params.value("mode").toString();
    if (mode == QLatin1String("form")) {
        request.mode = ElicitationRequest::Mode::Form;
        const auto schema = V2::fromJson<V2::ElicitationSchema>(params.value("requestedSchema"));
        if (!schema) {
            m_client->sendErrorResponse(id, ErrorCode::Invalid_params,
                                        QStringLiteral("Invalid requestedSchema: %1")
                                            .arg(schema.error()));
            return;
        }
        request.requestedSchema = *schema;
    } else if (mode == QLatin1String("url")) {
        request.mode = ElicitationRequest::Mode::Url;
        request.url = params.value("url").toString();
        request.elicitationId = params.value("elicitationId").toString();
        if (request.url.isEmpty() || request.elicitationId.isEmpty()) {
            m_client->sendErrorResponse(id, ErrorCode::Invalid_params,
                                        QStringLiteral("Missing url or elicitationId"));
            return;
        }
        m_urlElicitationIds.insert(request.elicitationId, id);
    } else {
        // Unknown modes are reserved for future protocol variants; this client
        // must not render them as a known elicitation mode.
        qCWarning(logElicitation) << "Unsupported elicitation mode:" << mode;
        m_client->sendErrorResponse(id, ErrorCode::Invalid_params,
                                    QStringLiteral("Unsupported elicitation mode: %1").arg(mode));
        return;
    }

    m_pendingIds.append(id);
    emit elicitationRequested(id, request);
}

void AcpElicitationHandler::handleNotification(const QString &method, const QJsonObject &params)
{
    if (method != QLatin1String("elicitation/complete"))
        return;
    const QString elicitationId = params.value("elicitationId").toString();
    if (!m_urlElicitationIds.contains(elicitationId)) {
        qCDebug(logElicitation) << "Completion for unknown elicitation:" << elicitationId;
        return;
    }
    const QJsonValue id = m_urlElicitationIds.value(elicitationId);
    sendElicitationAccepted(id, {});
    emit elicitationCompletedByAgent(id);
}

void AcpElicitationHandler::handleRequestCancelled(const QJsonValue &id)
{
    if (!removePending(id))
        return;
    qCDebug(logElicitation) << "Elicitation cancelled by agent:" << id;
    emit elicitationCancelledByAgent(id);
}

// The request is answered at most once; the URL completion map must not
// outlive it either, or a later elicitation/complete would target a stale id.
bool AcpElicitationHandler::removePending(const QJsonValue &id)
{
    if (!m_pendingIds.removeOne(id))
        return false;
    m_urlElicitationIds.removeIf([&id](const auto &it) { return it.value() == id; });
    return true;
}

void AcpElicitationHandler::sendElicitationAccepted(const QJsonValue &id,
                                                    const QJsonObject &content)
{
    if (!removePending(id))
        return;
    QJsonObject response{{"action", QStringLiteral("accept")}};
    if (!content.isEmpty())
        response.insert("content", content);
    m_client->sendResponse(id, response);
}

void AcpElicitationHandler::sendElicitationDeclined(const QJsonValue &id)
{
    if (!removePending(id))
        return;
    m_client->sendResponse(id, QJsonObject{{"action", QStringLiteral("decline")}});
}

void AcpElicitationHandler::sendElicitationCancelled(const QJsonValue &id)
{
    if (!removePending(id))
        return;
    m_client->sendResponse(id, QJsonObject{{"action", QStringLiteral("cancel")}});
}

} // namespace AcpClient::Internal
