// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "mcpclient.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>

namespace QmlDesigner {

/*!
    \brief Streamable HTTP transport for the Model Context Protocol.

    Implements the MCP 2025-11-25 "Streamable HTTP" transport:
    https://modelcontextprotocol.io/specification/2025-11-25/basic/transports

    Every outgoing JSON-RPC message is a discrete HTTP POST to the MCP endpoint.
    The server may reply with either:

      - application/json     — a single JSON-RPC response object.
      - text/event-stream    — an SSE stream carrying one or more JSON-RPC
                               objects, used for streaming results or
                               server-initiated notifications.

    Session management via MCP-Session-Id is supported. Stream resumption
    via Last-Event-ID is not implemented.
*/
class McpClientHttp : public McpClient
{
    Q_OBJECT

public:
    explicit McpClientHttp(const QString &clientName,
                           const QString &clientVersion,
                           QObject *parent = nullptr);
    ~McpClientHttp() override;

    // Connect to a Streamable HTTP MCP server.
    // Returns false immediately if the URL is syntactically invalid.
    // The "started" signal is emitted on success; the first real network
    // error will surface through "errorOccurred".
    bool connectToServer(const QString &url, const QString &bearerToken = {});

    bool isRunning() const override;
    void stop(int killTimeoutMs = 3000) override;

protected:
    void sendRpcToServer(const QJsonObject &obj) override;

private slots:
    void onFinished();

private:
    // All state for a single in-flight HTTP request.
    struct PendingRequest
    {
        QNetworkReply *reply = nullptr;
        QByteArray buffer;
    };

    void parseSseBuffer(const QByteArray &buffer);

    // Network helpers
    QNetworkRequest buildRequest() const;

    // Pending request lookup — returns -1 if not found.
    qsizetype indexOf(QNetworkReply *reply) const;

    // Aborts and deletes all pending requests without emitting any signals.
    void abortAll();

    QNetworkAccessManager m_nam;
    QUrl m_url;
    QString m_bearerToken;
    QString m_sessionId;   // MCP-Session-Id assigned by server, empty until received
    bool m_running = false;

    QList<PendingRequest> m_pending;
};

} // namespace QmlDesigner
