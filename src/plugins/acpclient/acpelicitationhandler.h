// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <acp/acpv2.h>

#include <QJsonObject>
#include <QJsonValue>
#include <QObject>

namespace AcpClient::Internal {

class AcpClientObject;

// A parsed elicitation/create request. The generated protocol types keep the
// mode-specific payload in additionalProperties, so this carries the fields
// the UI renders in typed form.
class ElicitationRequest
{
public:
    enum class Mode { Form, Url };

    QString message;
    Mode mode = Mode::Form;
    Acp::V2::ElicitationSchema requestedSchema; // form mode
    QString url;                                // url mode
    QString elicitationId;                      // url mode
};

// Handles agent-initiated elicitation requests. The wire shape is identical
// for protocol v1 and v2, so a single handler serves both and hands the UI
// v2-typed schemas.
class AcpElicitationHandler : public QObject
{
    Q_OBJECT

public:
    explicit AcpElicitationHandler(AcpClientObject *client, QObject *parent = nullptr);

    void sendElicitationAccepted(const QJsonValue &id, const QJsonObject &content);
    void sendElicitationDeclined(const QJsonValue &id);
    void sendElicitationCancelled(const QJsonValue &id);

signals:
    void elicitationRequested(const QJsonValue &id, const ElicitationRequest &request);
    void elicitationCancelledByAgent(const QJsonValue &id);
    // A URL-mode elicitation finished externally; id refers to the still
    // unanswered elicitation/create request it belongs to.
    void elicitationCompletedByAgent(const QJsonValue &id);

private:
    void handleRequest(const QJsonValue &id, const QString &method, const QJsonObject &params);
    void handleNotification(const QString &method, const QJsonObject &params);
    void handleRequestCancelled(const QJsonValue &id);
    bool removePending(const QJsonValue &id);

    AcpClientObject *m_client = nullptr;
    QList<QJsonValue> m_pendingIds;
    QHash<QString, QJsonValue> m_urlElicitationIds; // elicitationId -> request id
};

} // namespace AcpClient::Internal
