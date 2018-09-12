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

#pragma once

#include "basemessage.h"

#include <utils/mimetypes/mimetype.h>
#include <utils/qtcassert.h>
#include <utils/variant.h>

#include <QByteArray>
#include <QDebug>
#include <QHash>
#include <QJsonValue>

#include <functional>

namespace LanguageServerProtocol {

class IContent;

class LANGUAGESERVERPROTOCOL_EXPORT MessageId : public Utils::variant<int, QString>
{
public:
    MessageId() = default;
    MessageId(int id) : variant(id) {}
    MessageId(QString id) : variant(id) {}
    explicit MessageId(const QJsonValue &value)
    {
        if (value.isUndefined())
            return;
        QTC_CHECK(value.isDouble() || value.isString());
        if (value.isDouble())
            *this = value.toInt();
        else if (value.isString())
            *this = value.toString();
    }

    QJsonValue toJson() const
    {
        QTC_CHECK(Utils::holds_alternative<int>(*this) || Utils::holds_alternative<QString>(*this));
        if (auto id = Utils::get_if<int>(this))
            return *id;
        if (auto id = Utils::get_if<QString>(this))
            return *id;
        return QJsonValue();
    }

    bool isValid(QStringList *error = nullptr) const
    {
        if (Utils::holds_alternative<int>(*this) || Utils::holds_alternative<QString>(*this))
            return true;
        if (error)
            error->append("Expected int or string as MessageId");
        return false;
    }

    QString toString() const
    {
        if (auto id = Utils::get_if<QString>(this))
            return *id;
        if (auto id = Utils::get_if<int>(this))
            return QString::number(*id);
        return {};
    }
};

using ResponseHandler = std::function<void(const QByteArray &, QTextCodec *)>;
using ResponseHandlers = std::function<void(MessageId, const QByteArray &, QTextCodec *)>;
using MethodHandler = std::function<void(const QString, MessageId, const IContent *)>;

inline LANGUAGESERVERPROTOCOL_EXPORT uint qHash(const LanguageServerProtocol::MessageId &id)
{
    if (Utils::holds_alternative<int>(id))
        return QT_PREPEND_NAMESPACE(qHash(Utils::get<int>(id)));
    if (Utils::holds_alternative<QString>(id))
        return QT_PREPEND_NAMESPACE(qHash(Utils::get<QString>(id)));
    return QT_PREPEND_NAMESPACE(qHash(0));
}

template <typename Error>
inline LANGUAGESERVERPROTOCOL_EXPORT QDebug operator<<(QDebug stream,
                                                       const LanguageServerProtocol::MessageId &id)
{
    if (Utils::holds_alternative<int>(id))
        stream << Utils::get<int>(id);
    else
        stream << Utils::get<QString>(id);
    return stream;
}

class LANGUAGESERVERPROTOCOL_EXPORT IContent
{
public:
    virtual ~IContent() = default;

    virtual QByteArray toRawData() const = 0;
    virtual QByteArray mimeType() const = 0;
    virtual bool isValid(QString *errorMessage) const = 0;

    virtual void registerResponseHandler(QHash<MessageId, ResponseHandler> *) const { }

    BaseMessage toBaseMessage() const
    { return BaseMessage(mimeType(), toRawData()); }
};

} // namespace LanguageClient
