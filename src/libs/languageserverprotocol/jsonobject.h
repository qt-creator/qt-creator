// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "languageserverprotocol_global.h"
#include "lsputils.h"

#include <QCoreApplication>
#include <QDebug>
#include <QJsonObject>

namespace LanguageServerProtocol {

class LANGUAGESERVERPROTOCOL_EXPORT JsonObject
{
public:
    using iterator = QJsonObject::iterator;
    using const_iterator = QJsonObject::const_iterator;

    JsonObject() = default;

    explicit JsonObject(const QJsonObject &object) : m_jsonObject(object) { }
    explicit JsonObject(QJsonObject &&object) : m_jsonObject(std::move(object)) { }

    JsonObject(const JsonObject &object) : m_jsonObject(object.m_jsonObject) { }
    JsonObject(JsonObject &&object) : m_jsonObject(std::move(object.m_jsonObject)) { }

    explicit JsonObject(const QJsonValue &value) : m_jsonObject(value.toObject()) { }

    virtual ~JsonObject() = default;

    JsonObject &operator=(const JsonObject &other);
    JsonObject &operator=(JsonObject &&other);

    bool operator==(const JsonObject &other) const { return m_jsonObject == other.m_jsonObject; }

    operator const QJsonObject&() const { return m_jsonObject; }

    virtual bool isValid() const { return true; }

    iterator end() { return m_jsonObject.end(); }
    const_iterator end() const { return m_jsonObject.end(); }

protected:
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
    using Key = QLatin1StringView;
#else
    using Key = QLatin1String;
#endif
    iterator insert(const Key key, const JsonObject &value);
    iterator insert(const Key key, const QJsonValue &value);

    template <typename T, typename V>
    iterator insertVariant(const Key key, const V &variant);
    template <typename T1, typename T2, typename... Args, typename V>
    iterator insertVariant(const Key key, const V &variant);

    // QJSonObject redirections
    QJsonValue value(const QString &key) const { return m_jsonObject.value(key); }
    QJsonValue value(const Key key) const { return m_jsonObject.value(key); }
    bool contains(const Key key) const { return m_jsonObject.contains(key); }
    iterator find(const Key key) { return m_jsonObject.find(key); }
    const_iterator find(const Key key) const { return m_jsonObject.find(key); }
    void remove(const Key key) { m_jsonObject.remove(key); }
    QStringList keys() const { return m_jsonObject.keys(); }

    // convenience value access
    template<typename T>
    T typedValue(const Key key) const;
    template<typename T>
    std::optional<T> optionalValue(const Key key) const;
    template<typename T>
    LanguageClientValue<T> clientValue(const Key key) const;
    template<typename T>
    std::optional<LanguageClientValue<T>> optionalClientValue(const Key key) const;
    template<typename T>
    QList<T> array(const Key key) const;
    template<typename T>
    std::optional<QList<T>> optionalArray(const Key key) const;
    template<typename T>
    LanguageClientArray<T> clientArray(const Key key) const;
    template<typename T>
    std::optional<LanguageClientArray<T>> optionalClientArray(const Key key) const;
    template<typename T>
    void insertArray(const Key key, const QList<T> &array);
    template<typename>
    void insertArray(const Key key, const QList<JsonObject> &array);

private:
    QJsonObject m_jsonObject;
};

template<typename T, typename V>
JsonObject::iterator JsonObject::insertVariant(const Key key, const V &variant)
{
    return std::holds_alternative<T>(variant) ? insert(key, std::get<T>(variant)) : end();
}

template<typename T1, typename T2, typename... Args, typename V>
JsonObject::iterator JsonObject::insertVariant(const Key key, const V &variant)
{
    auto result = insertVariant<T1>(key, variant);
    return result != end() ? result : insertVariant<T2, Args...>(key, variant);
}

template<typename T>
T JsonObject::typedValue(const Key key) const
{
    return fromJsonValue<T>(value(key));
}

template<typename T>
std::optional<T> JsonObject::optionalValue(const Key key) const
{
    const QJsonValue &val = value(key);
    return val.isUndefined() ? std::nullopt : std::make_optional(fromJsonValue<T>(val));
}

template<typename T>
LanguageClientValue<T> JsonObject::clientValue(const Key key) const
{
    return LanguageClientValue<T>(value(key));
}

template<typename T>
std::optional<LanguageClientValue<T>> JsonObject::optionalClientValue(const Key key) const
{
    return contains(key) ? std::make_optional(clientValue<T>(key)) : std::nullopt;
}

template<typename T>
QList<T> JsonObject::array(const Key key) const
{
    if (const std::optional<QList<T>> &array = optionalArray<T>(key))
        return *array;
    qCDebug(conversionLog) << QString("Expected array under %1 in:").arg(key) << *this;
    return {};
}

template<typename T>
std::optional<QList<T>> JsonObject::optionalArray(const Key key) const
{
    const QJsonValue &jsonValue = value(key);
    if (jsonValue.isUndefined())
        return std::nullopt;
    return Utils::transform<QList<T>>(jsonValue.toArray(), &fromJsonValue<T>);
}

template<typename T>
LanguageClientArray<T> JsonObject::clientArray(const Key key) const
{
    return LanguageClientArray<T>(value(key));
}

template<typename T>
std::optional<LanguageClientArray<T>> JsonObject::optionalClientArray(const Key key) const
{
    const QJsonValue &val = value(key);
    return !val.isUndefined() ? std::make_optional(LanguageClientArray<T>(value(key)))
                              : std::nullopt;
}

template<typename T>
void JsonObject::insertArray(const Key key, const QList<T> &array)
{
    QJsonArray jsonArray;
    for (const T &item : array)
        jsonArray.append(QJsonValue(item));
    insert(key, jsonArray);
}

template<typename >
void JsonObject::insertArray(const Key key, const QList<JsonObject> &array)
{
    QJsonArray jsonArray;
    for (const JsonObject &item : array)
        jsonArray.append(item.m_jsonObject);
    insert(key, jsonArray);
}

} // namespace LanguageServerProtocol
