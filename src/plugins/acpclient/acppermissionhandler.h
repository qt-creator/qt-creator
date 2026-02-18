// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <acp/acp.h>

#include <QJsonValue>
#include <QObject>

namespace AcpClient::Internal {

class AcpClientObject;

class AcpPermissionHandler : public QObject
{
    Q_OBJECT

public:
    explicit AcpPermissionHandler(AcpClientObject *client, QObject *parent = nullptr);

    void sendPermissionResponse(const QJsonValue &id, const QString &selectedOptionId);
    void sendPermissionCancelled(const QJsonValue &id);

signals:
    void permissionRequested(const QJsonValue &id, const Acp::RequestPermissionRequest &request);

private:
    void handleRequestPermission(const QJsonValue &id, const Acp::RequestPermissionRequest &request);

    AcpClientObject *m_client;
};

} // namespace AcpClient::Internal
