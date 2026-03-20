// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "mcpclient.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QTimer>

namespace {

inline constexpr char mcpProtocolVersionFallback[] = "2025-11-25";

QString jsonToCompact(const QJsonObject &o)
{
    return QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact));
}

} // namespace

namespace QmlDesigner {

McpClient::McpClient(const QString &clientName, const QString &clientVersion, QObject *parent)
    : QObject(parent)
    , m_clientName(clientName)
    , m_clientVersion(clientVersion)
    , m_negotiatedProtocolVersion(QString::fromLatin1(mcpProtocolVersionFallback))
{}

McpClient::~McpClient() = default;

void McpClient::resetSession()
{
    m_pendingResponses.clear();
    m_nextId = 1;
    m_initializedConfirmed = false;
    m_negotiatedProtocolVersion = QString::fromLatin1(mcpProtocolVersionFallback);
}

QString McpClient::clientName() const
{
    return m_clientName;
}

qint64 McpClient::initialize()
{
    QJsonObject params{
        {"protocolVersion", mcpProtocolVersionFallback},
        {"capabilities", QJsonObject{
            {"roots", QJsonObject{{"listChanged", true}}},
            {"sampling", QJsonObject{}},
        }},
        {"clientInfo", QJsonObject{
            {"name", m_clientName},
            {"version", m_clientVersion},
        }},
    };

    return sendRequest(
        QStringLiteral("initialize"),
        params,
        [this](qint64, const QJsonObject &result, const QJsonObject &error) {
            if (!error.isEmpty()) {
                emit errorOccurred(
                    tr("initialize failed: %1").arg(error.value("message").toString()));
                return;
            }

            // Adopt whatever version the server agreed to use.
            const QString negotiated = result.value("protocolVersion").toString();
            if (!negotiated.isEmpty())
                m_negotiatedProtocolVersion = negotiated;

            McpServerInfo info{
                .name = result.value("serverInfo").toObject().value("name").toString(),
                .version = result.value("serverInfo").toObject().value("version").toString(),
            };

            emit connected(info);
            QTimer::singleShot(0, this, &McpClient::confirmClientInitialized);
        });
}

qint64 McpClient::listTools()
{
    return sendRequest(
        QStringLiteral("tools/list"),
        QJsonObject(),
        [this](qint64 requestId, const QJsonObject &result, const QJsonObject &error) {
            if (!error.isEmpty()) {
                emit errorOccurred(
                    tr("tools/list failed: %1").arg(error.value("message").toString()));
                return;
            }
            QList<McpTool> mcpTools;
            const QJsonArray toolsArray = result.value("tools").toArray();
            mcpTools.reserve(toolsArray.size());
            for (const QJsonValue &v : toolsArray) {
                const QJsonObject obj = v.toObject();
                mcpTools.append({
                    .name = obj.value("name").toString(),
                    .description = obj.value("description").toString(),
                    .inputSchema = obj.value("inputSchema").toObject(),
                });
            }
            emit toolsListed(mcpTools, requestId);
        });
}

qint64 McpClient::callTool(const QString &toolName, const QJsonObject &arguments)
{
    QJsonObject params{{"name", toolName}, {"arguments", arguments}};
    return sendRequest(
        QStringLiteral("tools/call"),
        params,
        [this](qint64 requestId, const QJsonObject &result, const QJsonObject &error) {
            if (error.isEmpty())
                emit toolCallSucceeded(result, requestId);
            else
                emit toolCallFailed(error.value("message").toString(), error, requestId);
        });
}

qint64 McpClient::listResources()
{
    return sendRequest(
        QStringLiteral("resources/list"),
        QJsonObject(),
        [this](qint64 requestId, const QJsonObject &result, const QJsonObject &error) {
            if (!error.isEmpty()) {
                emit errorOccurred(
                    tr("resources/list failed: %1").arg(error.value("message").toString()));
                return;
            }
            QList<McpResource> resources;
            const QJsonArray arr = result.value("resources").toArray();
            resources.reserve(arr.size());
            for (const QJsonValue &v : arr) {
                const QJsonObject obj = v.toObject();
                resources.append({
                    .name = obj.value("name").toString(),
                    .description = obj.value("description").toString(),
                    .uri = obj.value("uri").toString(),
                    .mimeType = obj.value("mimeType").toString(),
                });
            }
            emit resourcesListed(resources, requestId);
        });
}

qint64 McpClient::readResource(const QString &uri)
{
    QJsonObject params{{"uri", uri}};
    return sendRequest(
        QStringLiteral("resources/read"),
        params,
        [this](qint64 requestId, const QJsonObject &result, const QJsonObject &error) {
            if (error.isEmpty())
                emit resourceReadSucceeded(result, requestId);
            else
                emit resourceReadFailed(error.value("message").toString(), error, requestId);
        });
}

qint64 McpClient::sendRequest(const QString &method, const QJsonObject &params, ResponseHandler callback)
{
    if (!isRunning()) {
        emit errorOccurred(tr("sendRequest called but transport is not running"));
        return -1;
    }

    const qint64 id = m_nextId++;
    QJsonObject req{{"jsonrpc", "2.0"}, {"id", id}, {"method", method}, {"params", params}};
    sendRpcToServer(req);

    PendingResponse pending;
    pending.method = method;
    if (callback)
        pending.callback = std::move(callback);
    m_pendingResponses.insert(id, std::move(pending));
    return id;
}

void McpClient::handleIncomingLine(const QByteArray &line)
{
    QJsonParseError perr{};
    const QJsonDocument doc = QJsonDocument::fromJson(line, &perr);
    if (perr.error != QJsonParseError::NoError || !doc.isObject()) {
        emit errorOccurred(tr("Invalid JSON from server: %1").arg(QString::fromUtf8(line)));
        return;
    }
    const QJsonObject obj = doc.object();

    if (obj.contains(QStringLiteral("id")))
        handleResponse(obj);
    else if (obj.contains(QStringLiteral("method")))
        handleNotification(obj);
    else
        emit errorOccurred(tr("Unknown MCP frame: %1").arg(jsonToCompact(obj)));
}

void McpClient::handleResponse(const QJsonObject &obj)
{
    const qint64 id = obj.value("id").toInteger();
    const auto it = m_pendingResponses.find(id);
    const QJsonObject result = obj.value("result").toObject();
    const QJsonObject error = obj.value("error").toObject();

    if (it != m_pendingResponses.end()) {
        auto callback = std::move(it->callback);
        m_pendingResponses.erase(it);
        if (callback)
            callback(id, result, error);
    } else if (!error.isEmpty()) {
        emit errorOccurred(
            tr("Untracked response id=%1: %2").arg(id).arg(error.value("message").toString()));
    }
}

void McpClient::handleNotification(const QJsonObject &obj)
{
    const QString method = obj.value("method").toString();
    const QJsonObject params = obj.value("params").toObject();
    emit notificationReceived(method, params);
}

void McpClient::confirmClientInitialized()
{
    if (m_initializedConfirmed)
        return;
    m_initializedConfirmed = true;

    QJsonObject notif{
        {"jsonrpc", "2.0"},
        {"method", "notifications/initialized"},
    };
    sendRpcToServer(notif);
}

} // namespace QmlDesigner
