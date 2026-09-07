// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "lspjsonrpc.h"

#include "languageserverprotocoltr.h"

#include <QHash>
#include <QJsonDocument>

namespace LanguageServerProtocol {

MessageId::MessageId(const QJsonValue &value)
{
    if (value.isDouble())
        m_value = value.toInt();
    else
        m_value = value.toString();
}

QJsonValue MessageId::toJsonValue() const
{
    if (const int *id = std::get_if<int>(&m_value))
        return *id;
    return std::get<QString>(m_value);
}

QString MessageId::toString() const
{
    if (const QString *id = std::get_if<QString>(&m_value))
        return '"' + *id + '"';
    return QString::number(std::get<int>(m_value));
}

bool MessageId::isValid() const
{
    const QString *id = std::get_if<QString>(&m_value);
    return !id || !id->isEmpty();
}

size_t qHash(const MessageId &id)
{
    return qHash(id.toString());
}

ResponseError ResponseError::fromJsonObject(const QJsonObject &error)
{
    ResponseError result;
    result.code = error.value("code").toInt();
    result.message = error.value("message").toString();
    if (error.contains("data"))
        result.data = error.value("data");
    return result;
}

QJsonObject ResponseError::toJsonObject() const
{
    QJsonObject object{{"code", code}, {"message", message}};
    if (data)
        object.insert("data", *data);
    return object;
}

QJsonObject errorResponseObject(const MessageId &id, const ResponseError &error)
{
    return QJsonObject{{"jsonrpc", "2.0"}, {"id", id.toJsonValue()},
                       {"error", error.toJsonObject()}};
}

QString messageMethod(const QJsonObject &message)
{
    return message.value("method").toString();
}

MessageId messageId(const QJsonObject &message)
{
    return MessageId(message.value("id"));
}

BaseMessage toBaseMessage(const QJsonObject &message)
{
    return BaseMessage(BaseMessage::jsonRpcMimeType,
                       QJsonDocument(message).toJson(QJsonDocument::Compact));
}

Utils::Result<QJsonObject> fromBaseMessage(const BaseMessage &message)
{
    if (message.mimeType != QByteArray(BaseMessage::jsonRpcMimeType)) {
        return Utils::ResultError(Tr::tr("Cannot handle MIME type \"%1\" of message.")
                                      .arg(QString::fromUtf8(message.mimeType)));
    }
    QByteArray content;
    if (message.encoding.isValid() && !message.encoding.isUtf8())
        content = message.encoding.decode(message.content).toUtf8();
    if (content.isEmpty())
        content = message.content;
    QJsonParseError error = {0, QJsonParseError::NoError};
    const QJsonDocument document = QJsonDocument::fromJson(content, &error);
    if (!document.isObject()) {
        return Utils::ResultError(
            Tr::tr("Could not parse JSON message: \"%1\".").arg(error.errorString()));
    }
    return document.object();
}

} // namespace LanguageServerProtocol
