// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "acpprotocolv1adapter.h"

#include "acpclientobject.h"
#include "acpclienttr.h"

#include <QJsonArray>
#include <QLoggingCategory>

static Q_LOGGING_CATEGORY(logAdapter, "qtc.acpclient.v1adapter", QtWarningMsg);

using namespace Acp;

namespace AcpClient::Internal {

const char v1SessionModeConfigId[] = "_v1_session_mode";

// v1 diff: {type, path, oldText?, newText}. v2 diff: {type, changes: [...]}.
// oldText/newText travel on as additional properties of the change so the
// tool call widget can keep rendering the inline diff.
static QJsonObject convertDiff(const QJsonObject &diff)
{
    QJsonObject change;
    change["operation"] = diff.contains("oldText") && !diff.value("oldText").isNull()
                              ? QStringLiteral("modify") : QStringLiteral("add");
    change["path"] = diff.value("path");
    if (diff.contains("oldText"))
        change["oldText"] = diff.value("oldText");
    change["newText"] = diff.value("newText");

    QJsonObject result;
    result["type"] = QStringLiteral("diff");
    result["changes"] = QJsonArray{change};
    return result;
}

static QJsonArray convertToolCallContent(const QJsonArray &content)
{
    QJsonArray result;
    for (const QJsonValue &item : content) {
        const QJsonObject obj = item.toObject();
        if (obj.value("type").toString() == QLatin1String("diff"))
            result.append(convertDiff(obj));
        else
            result.append(item);
    }
    return result;
}

static QJsonObject convertToolCallUpdate(QJsonObject update)
{
    if (update.contains("content"))
        update["content"] = convertToolCallContent(update.value("content").toArray());
    return update;
}

// v1 config option JSON differs from v2 only in key names: the option id is
// "id" instead of "configId", select groups use "group" instead of "groupId".
static QJsonObject convertConfigOption(QJsonObject option)
{
    option.insert("configId", option.value("id"));
    option.remove("id");
    if (option.value("options").isArray()) {
        QJsonArray options;
        for (const QJsonValue &item : option.value("options").toArray()) {
            QJsonObject entry = item.toObject();
            if (entry.contains("group")) {
                entry.insert("groupId", entry.value("group"));
                entry.remove("group");
            }
            options.append(entry);
        }
        option["options"] = options;
    }
    return option;
}

AcpProtocolV1Adapter::AcpProtocolV1Adapter(AcpClientObject *client,
                                           const InitializeResponse &response,
                                           QObject *parent)
    : AcpProtocolAdapter(client, parent)
    , m_initializeResponse(response)
{
    connect(m_client, &AcpClientObject::notificationReceived,
            this, &AcpProtocolV1Adapter::handleNotification);
    connect(m_client, &AcpClientObject::requestPermissionRequested,
            this, &AcpProtocolV1Adapter::handlePermissionRequest);
}

QList<V2::AuthMethod> AcpProtocolV1Adapter::authMethods() const
{
    QList<V2::AuthMethod> result;
    const QList<AuthMethod> methods
        = m_initializeResponse.authMethods().value_or(QList<AuthMethod>{});
    for (const AuthMethod &method : methods) {
        // Terminal methods must not be answered with `authenticate`, and this
        // client does not advertise the capability that lets an agent offer
        // them, so there is nothing to present them as.
        if (!std::holds_alternative<AuthMethodAgent>(method)) {
            qCWarning(logAdapter) << "Ignoring unsupported auth method type:"
                                  << Acp::dispatchValue(method);
            continue;
        }
        QJsonObject obj = Acp::toJson(method);
        obj.insert("methodId", obj.value("id"));
        obj.remove("id");
        if (const auto converted = V2::fromJson<V2::AuthMethod>(obj))
            result.append(*converted);
        else
            qCWarning(logAdapter) << "Dropping unconvertible auth method:" << obj
                                  << converted.error();
    }
    return result;
}

bool AcpProtocolV1Adapter::supportsSessionList() const
{
    const auto &caps = m_initializeResponse.agentCapabilities();
    if (!caps)
        return false;
    const auto &sessionCaps = caps->sessionCapabilities();
    return sessionCaps.has_value() && sessionCaps->list().has_value();
}

bool AcpProtocolV1Adapter::supportsSessionDelete() const
{
    const auto &caps = m_initializeResponse.agentCapabilities();
    if (!caps)
        return false;
    const auto &sessionCaps = caps->sessionCapabilities();
    return sessionCaps.has_value() && sessionCaps->delete_().has_value();
}

bool AcpProtocolV1Adapter::supportsSessionClose() const
{
    const auto &caps = m_initializeResponse.agentCapabilities();
    if (!caps)
        return false;
    const auto &sessionCaps = caps->sessionCapabilities();
    return sessionCaps.has_value() && sessionCaps->close().has_value();
}

bool AcpProtocolV1Adapter::supportsImagePrompt() const
{
    const auto &caps = m_initializeResponse.agentCapabilities();
    if (!caps)
        return false;
    if (const auto &promptCaps = caps->promptCapabilities())
        return promptCaps->image().value_or(false);
    return false;
}

bool AcpProtocolV1Adapter::supportsEmbeddedContext() const
{
    const auto &caps = m_initializeResponse.agentCapabilities();
    if (!caps)
        return false;
    if (const auto &promptCaps = caps->promptCapabilities())
        return promptCaps->embeddedContext().value_or(false);
    return false;
}

QList<V2::SessionConfigOption> AcpProtocolV1Adapter::convertConfigOptions(
    const QJsonArray &options) const
{
    QList<V2::SessionConfigOption> result;
    for (const QJsonValue &item : options) {
        if (const auto opt = V2::fromJson<V2::SessionConfigOption>(convertConfigOption(item.toObject())))
            result.append(*opt);
        else
            qCWarning(logAdapter) << "Dropping unconvertible config option:" << item;
    }
    return result;
}

std::optional<V2::SessionConfigOption> AcpProtocolV1Adapter::synthesizedModeOption() const
{
    if (m_modes.isEmpty())
        return {};

    QJsonArray options;
    for (const QJsonObject &mode : m_modes) {
        QJsonObject option;
        option["value"] = mode.value("id");
        option["name"] = mode.value("name");
        if (mode.contains("description"))
            option["description"] = mode.value("description");
        options.append(option);
    }

    QJsonObject obj;
    obj["type"] = QStringLiteral("select");
    obj["configId"] = QLatin1String(v1SessionModeConfigId);
    obj["name"] = Tr::tr("Mode");
    obj["category"] = QStringLiteral("mode");
    obj["currentValue"] = m_currentModeId;
    obj["options"] = options;

    const auto option = V2::fromJson<V2::SessionConfigOption>(obj);
    if (!option) {
        qCWarning(logAdapter) << "Failed to synthesize mode option:" << option.error();
        return {};
    }
    return *option;
}

QList<V2::SessionConfigOption> AcpProtocolV1Adapter::fullConfigOptions() const
{
    QList<V2::SessionConfigOption> result = m_configOptions;
    if (const auto modeOption = synthesizedModeOption())
        result.append(*modeOption);
    return result;
}

QJsonArray AcpProtocolV1Adapter::fullConfigOptionsJson() const
{
    QJsonArray result;
    for (const V2::SessionConfigOption &option : fullConfigOptions())
        result.append(V2::toJson(option));
    return result;
}

void AcpProtocolV1Adapter::applySessionSetup(const QJsonObject &result)
{
    // This state belongs to one session, so a setup replaces it wholesale
    // instead of leaving the previous session's options in place.
    m_configOptions.clear();
    m_modes.clear();
    m_currentModeId.clear();
    if (result.value("configOptions").isArray())
        m_configOptions = convertConfigOptions(result.value("configOptions").toArray());
    if (const QJsonObject modes = result.value("modes").toObject(); !modes.isEmpty()) {
        for (const QJsonValue &mode : modes.value("availableModes").toArray())
            m_modes.append(mode.toObject());
        m_currentModeId = modes.value("currentModeId").toString();
    }
    emit configOptionsReceived(fullConfigOptions());
}

void AcpProtocolV1Adapter::newSession(const QString &cwd, const QJsonArray &mcpServers)
{
    QJsonObject params;
    params["cwd"] = cwd;
    params["mcpServers"] = mcpServers;
    m_client->sendRequest(QStringLiteral("session/new"), params,
                          [this](const QJsonObject &result, const std::optional<Error> &error) {
        if (error) {
            if (error->code() == ErrorCode::Authentication_required) {
                const QList<V2::AuthMethod> methods = authMethods();
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
        applySessionSetup(result);
        qCDebug(logAdapter) << "Session created:" << sessionId;
    });
}

void AcpProtocolV1Adapter::resumeSession(const QString &sessionId, const QString &cwd,
                                         const QJsonArray &mcpServers)
{
    QJsonObject params;
    params["sessionId"] = sessionId;
    params["cwd"] = cwd;
    params["mcpServers"] = mcpServers;
    m_client->sendRequest(QStringLiteral("session/load"), params,
                          [this, sessionId](const QJsonObject &result,
                                            const std::optional<Error> &error) {
        if (error) {
            emit errorOccurred(Tr::tr("Failed to load session: %1").arg(error->message()));
            return;
        }
        emit sessionResumed(sessionId);
        applySessionSetup(result);
    });
}

void AcpProtocolV1Adapter::listSessions(const std::optional<QString> &cursor)
{
    QJsonObject params;
    if (cursor)
        params["cursor"] = *cursor;
    m_client->sendRequest(QStringLiteral("session/list"), params,
                          [this](const QJsonObject &result, const std::optional<Error> &error) {
        if (error) {
            emit errorOccurred(Tr::tr("Failed to list sessions: %1").arg(error->message()));
            return;
        }
        if (!result.value("sessions").isArray()) {
            emit errorOccurred(Tr::tr("Failed to list sessions: invalid response."));
            return;
        }
        QList<V2::SessionInfo> sessions;
        for (const QJsonValue &item : result.value("sessions").toArray()) {
            const auto session = V2::fromJson<V2::SessionInfo>(item);
            if (session)
                sessions.append(*session);
            else
                qCWarning(logAdapter) << "Dropping unconvertible session:" << item
                                      << session.error();
        }
        std::optional<QString> nextCursor;
        if (result.value("nextCursor").isString())
            nextCursor = result.value("nextCursor").toString();
        emit sessionsListed(sessions, nextCursor);
    });
}

void AcpProtocolV1Adapter::deleteSession(const QString &sessionId)
{
    m_client->sendRequest(QStringLiteral("session/delete"), {{"sessionId", sessionId}},
                          [this, sessionId](const QJsonObject &,
                                            const std::optional<Error> &error) {
        if (error) {
            emit errorOccurred(Tr::tr("Failed to delete session: %1").arg(error->message()));
            return;
        }
        emit sessionDeleted(sessionId);
    });
}

void AcpProtocolV1Adapter::closeSession(const QString &sessionId)
{
    m_client->sendRequest(QStringLiteral("session/close"), {{"sessionId", sessionId}},
                          [this, sessionId](const QJsonObject &,
                                            const std::optional<Error> &error) {
        if (error) {
            emit errorOccurred(Tr::tr("Failed to close session: %1").arg(error->message()));
            return;
        }
        emit sessionClosed(sessionId);
    });
}

void AcpProtocolV1Adapter::prompt(const QString &sessionId, const QJsonArray &content)
{
    // Bumped again when the turn ends, so chunks that arrive between turns get
    // a messageId of their own instead of upserting into the finished message.
    ++m_turnCounter;
    QJsonObject params;
    params["sessionId"] = sessionId;
    params["prompt"] = content;
    m_client->sendRequest(QStringLiteral("session/prompt"), params,
                          [this](const QJsonObject &result, const std::optional<Error> &error) {
        ++m_turnCounter;
        if (error) {
            emit errorOccurred(Tr::tr("Prompt error: %1").arg(error->message()));
            emit promptFinished({});
            return;
        }
        std::optional<V2::StopReason> stopReason;
        if (const auto reason = V2::fromJson<V2::StopReason>(result.value("stopReason")))
            stopReason = *reason;
        if (stopReason)
            qCDebug(logAdapter) << "Prompt completed. Stop reason:" << V2::toString(*stopReason);
        emit promptFinished(stopReason);
    });
}

void AcpProtocolV1Adapter::cancel(const QString &sessionId)
{
    m_client->sendNotification(QStringLiteral("session/cancel"), {{"sessionId", sessionId}});
}

void AcpProtocolV1Adapter::authenticate(const QString &methodId)
{
    m_client->sendRequest(QStringLiteral("authenticate"), {{"methodId", methodId}},
                          [this](const QJsonObject &, const std::optional<Error> &error) {
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

void AcpProtocolV1Adapter::setConfigOption(const QString &sessionId, const QString &configId,
                                           const QJsonValue &value)
{
    if (configId == QLatin1String(v1SessionModeConfigId)) {
        const QString modeId = value.toString();
        QJsonObject params;
        params["sessionId"] = sessionId;
        params["modeId"] = modeId;
        m_client->sendRequest(QStringLiteral("session/set_mode"), params,
                              [this, modeId](const QJsonObject &,
                                             const std::optional<Error> &error) {
            if (error) {
                emit errorOccurred(Tr::tr("Failed to set mode: %1").arg(error->message()));
                return;
            }
            m_currentModeId = modeId;
            emit configOptionsReceived(fullConfigOptions());
        });
        return;
    }

    QJsonObject params;
    params["sessionId"] = sessionId;
    params["configId"] = configId;
    params["value"] = value;
    if (value.isBool())
        params["type"] = QStringLiteral("boolean");
    m_client->sendRequest(QStringLiteral("session/set_config_option"), params,
                          [this](const QJsonObject &result, const std::optional<Error> &error) {
        if (error) {
            emit errorOccurred(Tr::tr("Failed to set config option: %1").arg(error->message()));
            return;
        }
        if (result.value("configOptions").isArray()) {
            m_configOptions = convertConfigOptions(result.value("configOptions").toArray());
            emit configOptionsReceived(fullConfigOptions());
        }
    });
}

void AcpProtocolV1Adapter::handleNotification(const QString &method, const QJsonObject &params)
{
    if (method != QLatin1String("session/update"))
        return;

    const QString sessionId = params.value("sessionId").toString();
    QJsonObject update = params.value("update").toObject();
    const QString kind = update.value("sessionUpdate").toString();

    if (kind == QLatin1String("tool_call") || kind == QLatin1String("tool_call_update")) {
        // v2 has no separate tool_call kind: tool calls are created by the
        // first tool_call_update for an unseen id.
        update = convertToolCallUpdate(update);
        update["sessionUpdate"] = QStringLiteral("tool_call_update");
    } else if (kind == QLatin1String("plan")) {
        QJsonObject plan;
        plan["planId"] = QStringLiteral("v1");
        plan["entries"] = update.value("entries");
        update = QJsonObject{{"sessionUpdate", QStringLiteral("plan_update")}, {"plan", plan}};
    } else if (kind == QLatin1String("current_mode_update")) {
        m_currentModeId = update.value("currentModeId").toString();
        update = QJsonObject{{"sessionUpdate", QStringLiteral("config_option_update")},
                             {"configOptions", fullConfigOptionsJson()}};
    } else if (kind == QLatin1String("config_option_update")) {
        m_configOptions = convertConfigOptions(update.value("configOptions").toArray());
        update["configOptions"] = fullConfigOptionsJson();
    } else if (kind == QLatin1String("user_message_chunk")
               || kind == QLatin1String("agent_message_chunk")
               || kind == QLatin1String("agent_thought_chunk")) {
        if (!update.contains("messageId"))
            update["messageId"] = QStringLiteral("v1-%1-%2").arg(kind).arg(m_turnCounter);
    }

    const auto converted = V2::fromJson<V2::SessionUpdate>(update);
    if (!converted) {
        qCWarning(logAdapter) << "Invalid session update:" << update << converted.error();
        return;
    }
    emit sessionUpdate(sessionId, *converted);
}

void AcpProtocolV1Adapter::handlePermissionRequest(const QJsonValue &id,
                                                   const RequestPermissionRequest &request)
{
    QJsonObject toolCall = Acp::toJson(request.toolCall());
    toolCall = convertToolCallUpdate(toolCall);

    QJsonObject subject;
    subject["type"] = QStringLiteral("tool_call");
    subject["toolCall"] = toolCall;

    QJsonArray options;
    for (const PermissionOption &option : request.options())
        options.append(Acp::toJson(option));

    QJsonObject obj;
    obj["sessionId"] = request.sessionId();
    obj["title"] = request.toolCall().title().value_or(Tr::tr("Permission required"));
    obj["subject"] = subject;
    obj["options"] = options;

    const auto converted = V2::fromJson<V2::RequestPermissionRequest>(obj);
    if (!converted) {
        qCWarning(logAdapter) << "Invalid permission request:" << obj << converted.error();
        return;
    }
    emit permissionRequested(id, *converted);
}

} // namespace AcpClient::Internal
