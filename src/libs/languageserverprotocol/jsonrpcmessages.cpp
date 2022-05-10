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

QByteArray JsonRpcMessage::mimeType() const
{
    return jsonRpcMimeType();
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
    if (doc.isObject()) {
        m_jsonObject = doc.object();
    } else if (doc.isNull()) {
        m_parseError = tr("LanguageServerProtocol::JsonRpcMessage",
                          "Could not parse JSON message \"%1\".")
                           .arg(error.errorString());
    } else {
        m_parseError = tr("Expected a JSON object, but got a JSON \"%1\" value.")
                           .arg(docTypeName(doc));
    }
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
