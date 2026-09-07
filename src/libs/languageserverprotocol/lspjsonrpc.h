// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "languageserverprotocol_global.h"
#include "lspbasemessage.h"
#include "lspmessages.h"

#include <utils/result.h>

#include <QJsonObject>
#include <QString>

#include <optional>
#include <variant>

namespace LanguageServerProtocol {

class LANGUAGESERVERPROTOCOL_EXPORT MessageId
{
public:
    MessageId() = default;
    explicit MessageId(int id) : m_value(id) {}
    explicit MessageId(const QString &id) : m_value(id) {}
    explicit MessageId(const QJsonValue &value);

    QJsonValue toJsonValue() const;
    QString toString() const;
    bool isValid() const;

    friend bool operator==(const MessageId &first, const MessageId &second)
    { return first.m_value == second.m_value; }

private:
    std::variant<int, QString> m_value = QString();
};

LANGUAGESERVERPROTOCOL_EXPORT size_t qHash(const MessageId &id);

struct LANGUAGESERVERPROTOCOL_EXPORT ResponseError
{
    int code = 0;
    QString message;
    std::optional<QJsonValue> data;

    static ResponseError fromJsonObject(const QJsonObject &error);
    QJsonObject toJsonObject() const;
};

/// The payload of a message that carries none.
inline constexpr bool isEmptyPayload(const std::monostate &) { return true; }

namespace Internal {

template<typename T>
QJsonValue payloadToJson(const T &payload)
{
    if constexpr (std::is_same_v<T, std::monostate>)
        return QJsonValue(QJsonValue::Null);
    else if constexpr (std::is_constructible_v<QJsonValue, T>)
        return QJsonValue(payload);
    else if constexpr (requires { toJsonValue(payload); })
        return toJsonValue(payload);
    else
        return toJson(payload);
}

template<typename T>
Utils::Result<T> payloadFromJson(const QJsonValue &value)
{
    if constexpr (std::is_same_v<T, std::monostate>)
        return std::monostate{};
    else
        return fromJson<T>(value);
}

} // namespace Internal

/// The JSON-RPC object for a request of the message type \a M.
template<typename M>
QJsonObject requestObject(const MessageId &id, const typename M::Params &params)
{
    QJsonObject object{{"jsonrpc", "2.0"}, {"id", id.toJsonValue()}, {"method", M::method}};
    if constexpr (!std::is_same_v<typename M::Params, std::monostate>)
        object.insert("params", Internal::payloadToJson(params));
    return object;
}

template<typename M>
QJsonObject requestObject(const MessageId &id)
{
    static_assert(std::is_same_v<typename M::Params, std::monostate>,
                  "this message takes parameters");
    return QJsonObject{{"jsonrpc", "2.0"}, {"id", id.toJsonValue()}, {"method", M::method}};
}

/// The JSON-RPC object for a notification of the message type \a M.
template<typename M>
QJsonObject notificationObject(const typename M::Params &params)
{
    QJsonObject object{{"jsonrpc", "2.0"}, {"method", M::method}};
    if constexpr (!std::is_same_v<typename M::Params, std::monostate>)
        object.insert("params", Internal::payloadToJson(params));
    return object;
}

template<typename M>
QJsonObject notificationObject()
{
    static_assert(std::is_same_v<typename M::Params, std::monostate>,
                  "this message takes parameters");
    return QJsonObject{{"jsonrpc", "2.0"}, {"method", M::method}};
}

/// The JSON-RPC object answering the request \a id with \a result.
template<typename M>
QJsonObject responseObject(const MessageId &id, const typename M::Result &result)
{
    return QJsonObject{{"jsonrpc", "2.0"}, {"id", id.toJsonValue()},
                       {"result", Internal::payloadToJson(result)}};
}

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject errorResponseObject(const MessageId &id,
                                                               const ResponseError &error);

/// The parameters of \a message, which must be of the message type \a M.
template<typename M>
Utils::Result<typename M::Params> params(const QJsonObject &message)
{
    return Internal::payloadFromJson<typename M::Params>(message.value("params"));
}

/// The result of \a response, or the error the peer reported instead.
template<typename M>
Utils::Result<typename M::Result> result(const QJsonObject &response)
{
    if (response.contains("error"))
        return Utils::ResultError(ResponseError::fromJsonObject(response.value("error").toObject()).message);
    return Internal::payloadFromJson<typename M::Result>(response.value("result"));
}

/// The method of \a message, empty for a response.
LANGUAGESERVERPROTOCOL_EXPORT QString messageMethod(const QJsonObject &message);

/// The id of \a message, invalid for a notification.
LANGUAGESERVERPROTOCOL_EXPORT MessageId messageId(const QJsonObject &message);

/// \a message as it goes over the wire.
LANGUAGESERVERPROTOCOL_EXPORT BaseMessage toBaseMessage(const QJsonObject &message);

/// The JSON-RPC object \a message carries, or why it does not carry one.
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<QJsonObject> fromBaseMessage(
    const BaseMessage &message);

} // namespace LanguageServerProtocol
