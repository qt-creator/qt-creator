// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "mcpclienthttp.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>

namespace QmlDesigner {

// ------------------------------------------------------------------------------------------------
// Construction / destruction
// ------------------------------------------------------------------------------------------------

McpClientHttp::McpClientHttp(const QString &clientName,
                             const QString &clientVersion,
                             QObject *parent)
    : McpClient(clientName, clientVersion, parent)
{}

McpClientHttp::~McpClientHttp()
{
    // abortAll() must run before m_nam is destroyed so that aborting a reply
    // cannot fire signals into a partially-destroyed object.
    abortAll();
}

// ------------------------------------------------------------------------------------------------
// Public API
// ------------------------------------------------------------------------------------------------

bool McpClientHttp::connectToServer(const QString &url, const QString &bearerToken)
{
    const QUrl parsed(url);
    if (!parsed.isValid() || parsed.isEmpty()) {
        emit errorOccurred(tr("Invalid MCP server URL: %1").arg(url));
        return false;
    }

    m_url = parsed;
    m_bearerToken = bearerToken;
    m_sessionId.clear();
    m_running = true;

    // Note: no TCP connection is made here — HTTP is per-request.
    // The first real connectivity error surfaces through errorOccurred().
    emit started();
    return true;
}

bool McpClientHttp::isRunning() const
{
    return m_running;
}

void McpClientHttp::stop(int /*killTimeoutMs*/)
{
    m_running = false;
    abortAll();
    resetSession();
    emit exited(0, QProcess::NormalExit);
}

// ------------------------------------------------------------------------------------------------
// Transport — sending
// ------------------------------------------------------------------------------------------------

// Every JSON-RPC message is a discrete HTTP POST, as required by the spec:
//   "Every JSON-RPC message sent from the client MUST be a new HTTP POST request."
//
// We tell the server we can accept either a plain JSON reply or an SSE stream:
//   "The client MUST include an Accept header, listing both application/json and
//    text/event-stream as supported content types."
void McpClientHttp::sendRpcToServer(const QJsonObject &obj)
{
    const QByteArray body = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    QNetworkRequest req = buildRequest();
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Accept", "application/json, text/event-stream");

    QNetworkReply *reply = m_nam.post(req, body);
    m_pending.append(PendingRequest{reply});
    connect(reply, &QNetworkReply::finished, this, &McpClientHttp::onFinished);
}

// ------------------------------------------------------------------------------------------------
// Transport — receiving
// ------------------------------------------------------------------------------------------------

void McpClientHttp::onFinished()
{
    auto *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) return;
    reply->deleteLater();

    const qsizetype idx = indexOf(reply);
    if (idx < 0) return;

    PendingRequest req = std::move(m_pending[idx]);
    m_pending.remove(idx);
    req.reply = nullptr;

    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred(tr("Network error: %1").arg(reply->errorString()));
        return;
    }

    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (status == 404 && !m_sessionId.isEmpty()) {
        m_sessionId.clear();
        emit errorOccurred(tr("MCP session expired (HTTP 404). A new session will be started."));
        return;
    }
    if (status < 200 || status >= 300) {
        emit errorOccurred(tr("HTTP error %1").arg(status));
        return;
    }

    const QByteArray newSessionId = reply->rawHeader("mcp-Session-Id");
    if (!newSessionId.isEmpty())
        m_sessionId = QString::fromLatin1(newSessionId);

    // read everything here, once
    req.buffer = reply->readAll();

    const QByteArray contentType =
        reply->header(QNetworkRequest::ContentTypeHeader).toByteArray().toLower();

    if (contentType.contains("text/event-stream"))
        parseSseBuffer(req.buffer);
    else if (!req.buffer.trimmed().isEmpty())
        handleIncomingLine(req.buffer.trimmed());
}

void McpClientHttp::parseSseBuffer(const QByteArray &buffer)
{
    QByteArray data;
    for (QByteArray line : buffer.split('\n')) {
        if (line.endsWith('\r'))
            line.chop(1);

        if (line.isEmpty()) {
            // Blank line signals end of an SSE event.
            if (!data.isEmpty()) {
                if (data.endsWith('\n'))
                    data.chop(1);
                handleIncomingLine(data.trimmed());
                data.clear();
            }
        } else if (line.startsWith("data:")) {
            QByteArray value = line.mid(5);
            if (value.startsWith(' '))
                value.remove(0, 1);
            data.append(value).append('\n');
        }
        // Spec fields event:, id:, retry: and comment lines (`:`) are not needed for basic
        // MCP JSON-RPC and are intentionally ignored.
    }
}

// ------------------------------------------------------------------------------------------------
// Helpers
// ------------------------------------------------------------------------------------------------

QNetworkRequest McpClientHttp::buildRequest() const
{
    QNetworkRequest req(m_url);
    // Spec: "The client MUST include MCP-Protocol-Version on all requests."
    req.setRawHeader("MCP-Protocol-Version", m_negotiatedProtocolVersion.toLatin1());

    // Spec: "If an MCP-Session-Id was returned during initialization, the client
    //        MUST include it in MCP-Session-Id on all subsequent requests."
    if (!m_sessionId.isEmpty())
        req.setRawHeader("MCP-Session-Id", m_sessionId.toLatin1());

    if (!m_bearerToken.isEmpty())
        req.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(m_bearerToken).toUtf8());

    return req;
}

qsizetype McpClientHttp::indexOf(QNetworkReply *reply) const
{
    for (qsizetype i = 0; i < m_pending.size(); ++i) {
        if (m_pending[i].reply == reply)
            return i;
    }
    return -1;
}

// Cancels all in-flight requests without emitting signals.
// Signals are disconnected first so that abort() does not trigger onFinished()
// while we are iterating — avoiding both re-entrance and use-after-free against m_nam.
void McpClientHttp::abortAll()
{
    for (PendingRequest &req : m_pending) {
        if (req.reply) {
            req.reply->disconnect(this);
            req.reply->abort();
            delete req.reply;
            req.reply = nullptr;
        }
    }
    m_pending.clear();
}

} // namespace QmlDesigner
