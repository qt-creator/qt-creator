// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "acpclientobject.h"
#include "acpinspector.h"
#include "acptransport.h"

#include <QJsonDocument>
#include <QLoggingCategory>

static Q_LOGGING_CATEGORY(logClient, "qtc.acpclient.client", QtWarningMsg);

using namespace Acp;

namespace AcpClient::Internal {

AcpClientObject::AcpClientObject(AcpTransport *transport, QObject *parent)
    : QObject(parent)
    , m_transport(transport)
{
    connect(m_transport, &AcpTransport::messageReceived,
            this, &AcpClientObject::handleMessage);
    connect(m_transport, &AcpTransport::started, this, [this] {
        setState(State::Connecting);
    });
    connect(m_transport, &AcpTransport::finished, this, [this] {
        m_pendingRequests.clear();
        setState(State::Disconnected);
    });
    connect(m_transport, &AcpTransport::errorOccurred, this, &AcpClientObject::errorOccurred);
}

AcpClientObject::~AcpClientObject() = default;

void AcpClientObject::setInspector(AcpInspector *inspector, const QString &clientName)
{
    m_inspector = inspector;
    m_clientName = clientName;
}

void AcpClientObject::initialize(const InitializeRequest &request, ResponseCallback callback)
{
    sendRequest(QStringLiteral("initialize"), Acp::toJson(request), [this, callback](const QJsonObject &result, const std::optional<Error> &error) {
        if (!error) {
            setState(State::Initialized);
            auto resp = fromJson<InitializeResponse>(QJsonValue(result));
            if (resp)
                emit initializeResult(*resp);
        }
        if (callback)
            callback(result, error);
    });
    setState(State::InitializeRequested);
}

void AcpClientObject::authenticate(const AuthenticateRequest &request, ResponseCallback callback)
{
    sendRequest(QStringLiteral("authenticate"), Acp::toJson(request), callback);
}

void AcpClientObject::newSession(const NewSessionRequest &request, ResponseCallback callback)
{
    sendRequest(QStringLiteral("session/new"), Acp::toJson(request), callback);
}

void AcpClientObject::loadSession(const LoadSessionRequest &request, ResponseCallback callback)
{
    sendRequest(QStringLiteral("session/load"), Acp::toJson(request), callback);
}

void AcpClientObject::prompt(const PromptRequest &request, ResponseCallback callback)
{
    sendRequest(QStringLiteral("session/prompt"), Acp::toJson(request), callback);
}

void AcpClientObject::cancelSession(const CancelNotification &notification)
{
    sendNotification(QStringLiteral("session/cancel"), Acp::toJson(notification));
}

void AcpClientObject::setSessionMode(const SetSessionModeRequest &request, ResponseCallback callback)
{
    sendRequest(QStringLiteral("session/set_mode"), Acp::toJson(request), callback);
}

void AcpClientObject::setSessionConfigOption(const SetSessionConfigOptionRequest &request, ResponseCallback callback)
{
    sendRequest(QStringLiteral("session/set_config_option"), Acp::toJson(request), callback);
}

void AcpClientObject::sendResponse(const QJsonValue &id, const QJsonObject &result)
{
    QJsonObject message;
    message[QLatin1String("jsonrpc")] = QStringLiteral("2.0");
    message[QLatin1String("id")] = id;
    message[QLatin1String("result")] = result;
    if (m_inspector)
        m_inspector->log(AcpLogMessage::ClientMessage, m_clientName, message);
    m_transport->send(message);
}

void AcpClientObject::sendErrorResponse(const QJsonValue &id, int code, const QString &message)
{
    Acp::Error error;
    error.code(code);
    error.message(message);

    QJsonObject msg;
    msg[QLatin1String("jsonrpc")] = QStringLiteral("2.0");
    msg[QLatin1String("id")] = id;
    msg[QLatin1String("error")] = Acp::toJson(error);
    if (m_inspector)
        m_inspector->log(AcpLogMessage::ClientMessage, m_clientName, msg);
    m_transport->send(msg);
}

void AcpClientObject::handleMessage(const QJsonObject &message)
{
    if (m_inspector)
        m_inspector->log(AcpLogMessage::ServerMessage, m_clientName, message);

    const bool hasMethod = message.contains(QLatin1String("method"));
    const bool hasId = message.contains(QLatin1String("id"));

    if (hasMethod && hasId) {
        // Request from agent
        const QJsonValue id = message.value(QLatin1String("id"));
        const QString method = message.value(QLatin1String("method")).toString();
        const QJsonObject params = message.value(QLatin1String("params")).toObject();
        handleRequest(id, method, params);
    } else if (hasMethod) {
        // Notification from agent
        const QString method = message.value(QLatin1String("method")).toString();
        const QJsonObject params = message.value(QLatin1String("params")).toObject();
        handleNotification(method, params);
    } else if (hasId) {
        // Response to our request
        const QJsonValue id = message.value(QLatin1String("id"));
        handleResponse(id, message);
    } else {
        qCWarning(logClient) << "Unknown message format:" << message;
    }
}

void AcpClientObject::handleRequest(const QJsonValue &id, const QString &method, const QJsonObject &params)
{
    qCDebug(logClient) << "Agent request:" << method;

    const QJsonValue paramsVal(params);

    auto dispatch = [&]<typename T>(auto signal) {
        auto req = fromJson<T>(paramsVal);
        if (req)
            emit (this->*signal)(id, *req);
        else
            sendErrorResponse(id, ErrorCode::Invalid_params, req.error());
    };

    if (method == QLatin1String("terminal/create")) {
        dispatch.template operator()<CreateTerminalRequest>(&AcpClientObject::createTerminalRequested);
    } else if (method == QLatin1String("terminal/output")) {
        dispatch.template operator()<TerminalOutputRequest>(&AcpClientObject::terminalOutputRequested);
    } else if (method == QLatin1String("terminal/wait_for_exit")) {
        dispatch.template operator()<WaitForTerminalExitRequest>(&AcpClientObject::waitForTerminalExitRequested);
    } else if (method == QLatin1String("terminal/kill")) {
        dispatch.template operator()<KillTerminalCommandRequest>(&AcpClientObject::killTerminalRequested);
    } else if (method == QLatin1String("terminal/release")) {
        dispatch.template operator()<ReleaseTerminalRequest>(&AcpClientObject::releaseTerminalRequested);
    } else if (method == QLatin1String("fs/read_text_file")) {
        dispatch.template operator()<ReadTextFileRequest>(&AcpClientObject::readTextFileRequested);
    } else if (method == QLatin1String("fs/write_text_file")) {
        dispatch.template operator()<WriteTextFileRequest>(&AcpClientObject::writeTextFileRequested);
    } else if (method == QLatin1String("session/request_permission")) {
        dispatch.template operator()<RequestPermissionRequest>(&AcpClientObject::requestPermissionRequested);
    } else {
        qCWarning(logClient) << "Unknown agent request method:" << method;
        sendErrorResponse(id, ErrorCode::Method_not_found,
                          QStringLiteral("Unknown method: %1").arg(method));
    }
}

void AcpClientObject::handleNotification(const QString &method, const QJsonObject &params)
{
    qCDebug(logClient) << "Agent notification:" << method;

    if (method == QLatin1String("session/update")) {
        auto notif = fromJson<SessionNotification>(QJsonValue(params));
        if (notif)
            emit sessionUpdate(notif->sessionId(), notif->update());
        else
            qCWarning(logClient) << "Invalid session notification:" << params;
    } else {
        qCWarning(logClient) << "Unknown notification method:" << method;
    }
}

void AcpClientObject::handleResponse(const QJsonValue &id, const QJsonObject &message)
{
    if (!id.isDouble()) {
        qCWarning(logClient) << "Non-integer response id not supported";
        return;
    }

    const qint64 intId = static_cast<qint64>(id.toDouble());
    auto it = m_pendingRequests.find(intId);
    if (it == m_pendingRequests.end()) {
        qCWarning(logClient) << "Response for unknown request id:" << intId;
        return;
    }

    const ResponseCallback callback = it.value();
    m_pendingRequests.erase(it);

    if (message.contains(QLatin1String("error"))) {
        const Acp::Error error = fromJson<Acp::Error>(message.value(QLatin1String("error"))).value_or(Acp::Error{});
        qCDebug(logClient) << "Error response:" << error.message();
        if (callback)
            callback({}, error);
    } else {
        const QJsonObject result = message.value(QLatin1String("result")).toObject();
        if (callback)
            callback(result, std::nullopt);
    }
}

qint64 AcpClientObject::sendRequest(const QString &method, const QJsonObject &params, ResponseCallback callback)
{
    const qint64 id = m_nextId++;

    QJsonObject message;
    message[QLatin1String("jsonrpc")] = QStringLiteral("2.0");
    message[QLatin1String("id")] = id;
    message[QLatin1String("method")] = method;
    message[QLatin1String("params")] = params;

    if (callback)
        m_pendingRequests.insert(id, callback);

    if (m_inspector)
        m_inspector->log(AcpLogMessage::ClientMessage, m_clientName, message);

    qCDebug(logClient) << "Sending request:" << method << "id:" << id;
    m_transport->send(message);
    return id;
}

void AcpClientObject::sendNotification(const QString &method, const QJsonObject &params)
{
    QJsonObject message;
    message[QLatin1String("jsonrpc")] = QStringLiteral("2.0");
    message[QLatin1String("method")] = method;
    message[QLatin1String("params")] = params;

    if (m_inspector)
        m_inspector->log(AcpLogMessage::ClientMessage, m_clientName, message);

    qCDebug(logClient) << "Sending notification:" << method;
    m_transport->send(message);
}

void AcpClientObject::setState(State state)
{
    if (m_state != state) {
        m_state = state;
        emit stateChanged(m_state);
    }
}

} // namespace AcpClient::Internal
