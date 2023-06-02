// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "languageserverprotocol_global.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QJsonArray>
#include <QJsonObject>
#include <QLoggingCategory>

#include <optional>
#include <variant>

namespace LanguageServerProtocol {

LANGUAGESERVERPROTOCOL_EXPORT Q_DECLARE_LOGGING_CATEGORY(conversionLog)

template <typename T>
T fromJsonValue(const QJsonValue &value)
{
    if (conversionLog().isDebugEnabled() && !value.isObject())
        qCDebug(conversionLog) << "Expected Object in json value but got: " << value;
    T result(value.toObject());
    if (conversionLog().isDebugEnabled() && !result.isValid())
        qCDebug(conversionLog) << typeid(result).name() << " is not valid: " << result;
    return result;
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
LANGUAGESERVERPROTOCOL_EXPORT QJsonObject fromJsonValue<QJsonObject>(const QJsonValue &value);

template<>
LANGUAGESERVERPROTOCOL_EXPORT QJsonArray fromJsonValue<QJsonArray>(const QJsonValue &value);

template<>
LANGUAGESERVERPROTOCOL_EXPORT QJsonValue fromJsonValue<QJsonValue>(const QJsonValue &value);

template <typename T>
class LanguageClientArray : public std::variant<QList<T>, std::nullptr_t>
{
public:
    using std::variant<QList<T>, std::nullptr_t>::variant;
    using std::variant<QList<T>, std::nullptr_t>::operator=;

    LanguageClientArray() {}

    explicit LanguageClientArray(const QList<T> &list)
    { *this = list; }

    explicit LanguageClientArray(const QJsonValue &value)
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
        if (const auto list = std::get_if<QList<T>>(this)) {
            QJsonArray array;
            for (const T &value : *list)
                array.append(QJsonValue(value));
            return array;
        }
        return QJsonValue();
    }

    QList<T> toListOrEmpty() const
    {
        if (std::holds_alternative<QList<T>>(*this))
            return std::get<QList<T>>(*this);
        return {};
    }

    QList<T> toList() const
    {
        QTC_ASSERT(std::holds_alternative<QList<T>>(*this), return {});
        return std::get<QList<T>>(*this);
    }
    bool isNull() const { return std::holds_alternative<std::nullptr_t>(*this); }
};

template <typename T>
class LanguageClientValue : public std::variant<T, std::nullptr_t>
{
public:
    using std::variant<T, std::nullptr_t>::operator=;

    LanguageClientValue() : std::variant<T, std::nullptr_t>(nullptr) { }
    LanguageClientValue(const T &value) : std::variant<T, std::nullptr_t>(value) { }
    LanguageClientValue(const QJsonValue &value)
    {
        if (!QTC_GUARD(!value.isUndefined()) || value.isNull())
            *this = nullptr;
        else
            *this = fromJsonValue<T>(value);
    }

    operator const QJsonValue() const
    {
        if (auto val = std::get_if<T>(this))
            return QJsonValue(*val);
        return QJsonValue();
    }

    T value(const T &defaultValue = T()) const
    {
        QTC_ASSERT(std::holds_alternative<T>(*this), return defaultValue);
        return std::get<T>(*this);
    }

    template<typename Type>
    LanguageClientValue<Type> transform()
    {
        QTC_ASSERT(!std::holds_alternative<T>(*this), return LanguageClientValue<Type>());
        return Type(std::get<T>(*this));
    }

    bool isNull() const { return std::holds_alternative<std::nullptr_t>(*this); }
};

template <typename T>
QJsonArray enumArrayToJsonArray(const QList<T> &values)
{
    QJsonArray array;
    for (T value : values)
        array.append(static_cast<int>(value));
    return array;
}

template <typename T>
QList<T> jsonArrayToList(const QJsonArray &array)
{
    QList<T> list;
    for (const QJsonValue &val : array)
        list << fromJsonValue<T>(val);
    return list;
}

} // namespace LanguageServerProtocol
