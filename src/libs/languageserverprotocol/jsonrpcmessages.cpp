// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "jsonrpcmessages.h"

#include "initializemessages.h"
#include "languageserverprotocoltr.h"
#include "lsputils.h"

#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QObject>
#include <QJsonDocument>
#include <QTextCodec>

namespace LanguageServerProtocol {
Q_LOGGING_CATEGORY(timingLog, "qtc.languageserverprotocol.timing", QtWarningMsg)

constexpr const char CancelRequest::methodName[];

QByteArray JsonRpcMessage::toRawData() const
{
    return QJsonDocument(m_jsonObject).toJson(QJsonDocument::Compact);
}

bool JsonRpcMessage::isValid(QString *errorMessage) const
{
    if (!m_parseError.isEmpty()) {
        if (errorMessage)
            *errorMessage = m_parseError;
        return false;
    }
    return m_jsonObject[jsonRpcVersionKey] == "2.0";
}

const QJsonObject &JsonRpcMessage::toJsonObject() const
{
    return m_jsonObject;
}

JsonRpcMessage::JsonRpcMessage()
{
    // The language server protocol always uses “2.0” as the jsonrpc version
    m_jsonObject[jsonRpcVersionKey] = "2.0";
}

constexpr int utf8mib = 106;

static QString docType(const QJsonDocument &doc)
{
    if (doc.isArray())
        return QString("array");
    if (doc.isEmpty())
        return QString("empty");
    if (doc.isNull())
        return QString("null");
    if (doc.isObject())
        return QString("object");
    return {};
}

JsonRpcMessage::JsonRpcMessage(const BaseMessage &message)
{
    if (message.content.isEmpty())
        return;
    QByteArray content;
    if (message.codec && message.codec->mibEnum() != utf8mib) {
        QTextCodec *utf8 = QTextCodec::codecForMib(utf8mib);
        if (utf8)
            content = utf8->fromUnicode(message.codec->toUnicode(message.content));
    }
    if (content.isEmpty())
        content = message.content;
    QJsonParseError error = {0, QJsonParseError::NoError};
    const QJsonDocument doc = QJsonDocument::fromJson(content, &error);
    if (doc.isObject())
        m_jsonObject = doc.object();
    else if (doc.isNull())
        m_parseError = Tr::tr("Could not parse JSON message: \"%1\".").arg(error.errorString());
    else
        m_parseError =
            Tr::tr("Expected a JSON object, but got a JSON \"%1\" value.").arg(docType(doc));
}

JsonRpcMessage::JsonRpcMessage(const QJsonObject &jsonObject)
    : m_jsonObject(jsonObject)
{ }

JsonRpcMessage::JsonRpcMessage(QJsonObject &&jsonObject)
    : m_jsonObject(std::move(jsonObject))
{ }

QByteArray JsonRpcMessage::jsonRpcMimeType()
{
    return "application/vscode-jsonrpc";
}

CancelRequest::CancelRequest(const CancelParameter &params)
    : Notification(methodName, params)
{ }

void logElapsedTime(const QString &method, const QElapsedTimer &t)
{
    qCDebug(timingLog) << "received server reply to" << method
                       << "after" << t.elapsed() << "ms";
}

} // namespace LanguageServerProtocol
