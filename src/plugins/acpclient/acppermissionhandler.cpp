// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "acppermissionhandler.h"
#include "acpclientobject.h"

#include <QLoggingCategory>

static Q_LOGGING_CATEGORY(logPermission, "qtc.acpclient.permission", QtWarningMsg);

using namespace Acp;

namespace AcpClient::Internal {

AcpPermissionHandler::AcpPermissionHandler(AcpClientObject *client, QObject *parent)
    : QObject(parent)
    , m_client(client)
{
    connect(client, &AcpClientObject::requestPermissionRequested,
            this, &AcpPermissionHandler::handleRequestPermission);
}

void AcpPermissionHandler::handleRequestPermission(const QJsonValue &id, const RequestPermissionRequest &request)
{
    qCDebug(logPermission) << "Permission request for tool call:" << request.toolCall().toolCallId();
    emit permissionRequested(id, request);
}

void AcpPermissionHandler::sendPermissionResponse(const QJsonValue &id, const QString &selectedOptionId)
{
    RequestPermissionResponse response;
    SelectedPermissionOutcome sel;
    sel.optionId(selectedOptionId);
    RequestPermissionOutcome outcome;
    outcome._value = sel;
    outcome._kind = QStringLiteral("selected");
    response.outcome(outcome);
    m_client->sendResponse(id, Acp::toJson(response));
}

void AcpPermissionHandler::sendPermissionCancelled(const QJsonValue &id)
{
    RequestPermissionResponse response;
    RequestPermissionOutcome outcome;
    outcome._value = std::monostate{};
    outcome._kind = QStringLiteral("cancelled");
    response.outcome(outcome);
    m_client->sendResponse(id, Acp::toJson(response));
}

} // namespace AcpClient::Internal
