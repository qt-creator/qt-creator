// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "acptransport.h"

#include "acpclienttr.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QLoggingCategory>

static Q_LOGGING_CATEGORY(logTransport, "qtc.acpclient.transport", QtWarningMsg);

namespace AcpClient::Internal {

AcpTransport::AcpTransport(QObject *parent)
    : QObject(parent)
{}

AcpTransport::~AcpTransport() = default;

void AcpTransport::send(const QJsonObject &message)
{
    const QByteArray content = QJsonDocument(message).toJson(QJsonDocument::Compact) + '\n';
    qCDebug(logTransport) << "Sending:" << content;
    sendData(content);
}

void AcpTransport::parseData(const QByteArray &data)
{
    m_buffer.append(data);

    // Process complete newline-delimited JSON messages
    int pos;
    while ((pos = m_buffer.indexOf('\n')) >= 0) {
        const QByteArray line = m_buffer.left(pos).trimmed();
        m_buffer = m_buffer.mid(pos + 1);

        if (line.isEmpty())
            continue;

        qCDebug(logTransport) << "Received:" << line;

        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(line, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            emit errorOccurred(Tr::tr("JSON parse error: %1").arg(parseError.errorString()));
            continue;
        }

        // ACP v2 explicitly allows JSON-RPC 2.0 batches on stdio: a line may
        // carry an array of requests, notifications, or responses.
        if (doc.isArray()) {
            const QJsonArray batch = doc.array();
            // JSON-RPC 2.0 calls an empty batch an Invalid Request; report it
            // so that every line either produces messages or an error.
            if (batch.isEmpty()) {
                emit errorOccurred(Tr::tr("Empty JSON-RPC batch."));
                continue;
            }
            for (const QJsonValue &entry : batch) {
                if (entry.isObject())
                    emit messageReceived(entry.toObject());
                else
                    emit errorOccurred(Tr::tr("Expected JSON object in batch."));
            }
            continue;
        }

        if (!doc.isObject()) {
            emit errorOccurred(Tr::tr("Expected JSON object."));
            continue;
        }

        emit messageReceived(doc.object());
    }
}

} // namespace AcpClient::Internal
