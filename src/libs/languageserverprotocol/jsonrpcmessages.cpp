/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "jsonrpcmessages.h"

#include "lsputils.h"
#include "initializemessages.h"

#include <utils/mimetypes/mimedatabase.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QObject>
#include <QJsonDocument>
#include <QTextCodec>

namespace LanguageServerProtocol {

constexpr const char CancelRequest::methodName[];

QHash<QString, JsonRpcMessageHandler::MessageProvider> JsonRpcMessageHandler::m_messageProvider;

QByteArray JsonRpcMessage::toRawData() const
{
    return QJsonDocument(m_jsonObject).toJson(QJsonDocument::Compact);
}

QByteArray JsonRpcMessage::mimeType() const
{
    return JsonRpcMessageHandler::jsonRpcMimeType();
}

bool JsonRpcMessage::isValid(QString * /*errorMessage*/) const
{
    return m_jsonObject[jsonRpcVersionKey] == "2.0";
}

JsonRpcMessage::JsonRpcMessage()
{
    // The language server protocol always uses “2.0” as the jsonrpc version
    m_jsonObject[jsonRpcVersionKey] = "2.0";
}

JsonRpcMessage::JsonRpcMessage(const QJsonObject &jsonObject)
    : m_jsonObject(jsonObject)
{ }

JsonRpcMessage::JsonRpcMessage(QJsonObject &&jsonObject)
    : m_jsonObject(std::move(jsonObject))
{ }

QByteArray JsonRpcMessageHandler::jsonRpcMimeType()
{
    return "application/vscode-jsonrpc";
}

void JsonRpcMessageHandler::registerMessageProvider(
        const QString &method, JsonRpcMessageHandler::MessageProvider provider)
{
    m_messageProvider.insert(method, provider);
}

void JsonRpcMessageHandler::parseContent(const QByteArray &content,
                                         QTextCodec *codec,
                                         QString &parseError,
                                         ResponseHandlers responseHandlers,
                                         MethodHandler methodHandler)
{
    const QJsonObject &jsonObject = toJsonObject(content, codec, parseError);
    if (jsonObject.isEmpty())
        return;

    const MessageId id(jsonObject.value(idKey));
    const QString &method = jsonObject.value(methodKey).toString();
    if (!method.isEmpty()) {
        if (auto provider = m_messageProvider[method]) {
            methodHandler(method, id, provider(jsonObject));
            return;
        }
    }

    responseHandlers(id, content, codec);
}

constexpr int utf8mib = 106;

static QString docTypeName(const QJsonDocument &doc)
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

QJsonObject JsonRpcMessageHandler::toJsonObject(const QByteArray &_content,
                                                QTextCodec *codec,
                                                QString &parseError)
{
    if (_content.isEmpty())
        return QJsonObject();
    QByteArray content;
    if (codec && codec->mibEnum() != utf8mib) {
        QTextCodec *utf8 = QTextCodec::codecForMib(utf8mib);
        if (utf8)
            content = utf8->fromUnicode(codec->toUnicode(_content));
    }
    if (content.isEmpty())
        content = _content;
    QJsonParseError error = {0 , QJsonParseError::NoError};
    const QJsonDocument doc = QJsonDocument::fromJson(content, &error);
    if (doc.isObject())
        return doc.object();
    if (doc.isNull())
        parseError = tr("Could not parse JSON message \"%1\".").arg(error.errorString());
    else
        parseError = tr("Expected a JSON object, but got a JSON \"%1\".").arg(docTypeName(doc));
    return QJsonObject();
}

CancelRequest::CancelRequest(const CancelParameter &params)
    : Notification(methodName, params)
{ }

} // namespace LanguageServerProtocol
