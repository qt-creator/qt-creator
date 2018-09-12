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

#include "languageserverprotocol_global.h"

#include <utils/algorithm.h>
#include <utils/mimetypes/mimetype.h>
#include <utils/optional.h>
#include <utils/qtcassert.h>
#include <utils/variant.h>

#include <QJsonArray>
#include <QJsonObject>

namespace LanguageServerProtocol {

template <typename T>
T fromJsonValue(const QJsonValue &value)
{
    QTC_ASSERT(value.isObject(), return T());
    return T(value.toObject());
}

template<>
LANGUAGESERVERPROTOCOL_EXPORT QString fromJsonValue<QString>(const QJsonValue &value);

template<>
LANGUAGESERVERPROTOCOL_EXPORT int fromJsonValue<int>(const QJsonValue &value);

template<>
LANGUAGESERVERPROTOCOL_EXPORT double fromJsonValue<double>(const QJsonValue &value);

template<>
LANGUAGESERVERPROTOCOL_EXPORT bool fromJsonValue<bool>(const QJsonValue &value);

template<>
LANGUAGESERVERPROTOCOL_EXPORT QJsonArray fromJsonValue<QJsonArray>(const QJsonValue &value);

template <typename T>
class LanguageClientArray : public Utils::variant<QList<T>, std::nullptr_t>
{
public:
    using Utils::variant<QList<T>, std::nullptr_t>::variant;
    using Utils::variant<QList<T>, std::nullptr_t>::operator=;

    LanguageClientArray(const QList<T> &list)
    { *this = list; }

    LanguageClientArray(const QJsonValue &value)
    {
        if (value.isArray()) {
            QList<T> values;
            values.reserve(value.toArray().count());
            for (auto arrayValue : value.toArray())
                values << fromJsonValue<T>(arrayValue);
            *this = values;
        } else {
            *this = nullptr;
        }
    }

    QJsonValue toJson() const
    {
        if (const auto list = Utils::get_if<QList<T>>(this)) {
            QJsonArray array;
            for (const T &value : *list)
                array.append(QJsonValue(value));
            return array;
        }
        return QJsonValue();
    }

    QList<T> toList() const
    {
        QTC_ASSERT(Utils::holds_alternative<QList<T>>(*this), return {});
        return Utils::get<QList<T>>(*this);
    }
    bool isNull() const { return Utils::holds_alternative<std::nullptr_t>(*this); }
};

template <typename T>
class LanguageClientValue : public Utils::variant<T, std::nullptr_t>
{
public:
    using Utils::variant<T, std::nullptr_t>::operator=;

    LanguageClientValue() : Utils::variant<T, std::nullptr_t>(nullptr) { }
    LanguageClientValue(const T &value) : Utils::variant<T, std::nullptr_t>(value) { }
    LanguageClientValue(const QJsonValue &value)
    {
        if (QTC_GUARD(value.isUndefined()) || value.isNull())
            *this = nullptr;
        else
            *this = fromJsonValue<T>(value);
    }

    operator const QJsonValue() const
    {
        if (auto val = Utils::get_if<T>(this))
            return QJsonValue(*val);
        return QJsonValue();
    }

    T value(const T &defaultValue = T()) const
    {
        QTC_ASSERT(Utils::holds_alternative<T>(*this), return defaultValue);
        return Utils::get<T>(*this);
    }

    template<typename Type>
    LanguageClientValue<Type> transform()
    {
        QTC_ASSERT(!Utils::holds_alternative<T>(*this), return LanguageClientValue<Type>());
        return Type(Utils::get<T>(*this));
    }

    bool isNull() const { return Utils::holds_alternative<std::nullptr_t>(*this); }
};

template <typename T>
QJsonArray enumArrayToJsonArray(const QList<T> &values)
{
    QJsonArray array;
    for (T value : values)
        array.append(static_cast<int>(value));
    return array;
}

} // namespace LanguageClient
