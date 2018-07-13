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
#include "lsputils.h"

#include <QCoreApplication>
#include <QDebug>
#include <QJsonObject>

namespace LanguageServerProtocol {

class LANGUAGESERVERPROTOCOL_EXPORT JsonObject
{
    Q_DECLARE_TR_FUNCTIONS(LanguageServerProtocol::JsonObject)

public:
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

    virtual bool isValid(QStringList * /*errorHierarchy*/) const { return true; }

protected:
    using iterator = QJsonObject::iterator;
    using const_iterator = QJsonObject::const_iterator;

    iterator insert(const QString &key, const JsonObject &value);
    iterator insert(const QString &key, const QJsonValue &value);

    // QJSonObject redirections
    QJsonValue value(const QString &key) const { return m_jsonObject.value(key); }
    bool contains(const QString &key) const { return m_jsonObject.contains(key); }
    iterator find(const QString &key) { return m_jsonObject.find(key); }
    const_iterator find(const QString &key) const { return m_jsonObject.find(key); }
    iterator end() { return m_jsonObject.end(); }
    const_iterator end() const { return m_jsonObject.end(); }
    void remove(const QString &key) { m_jsonObject.remove(key); }
    QStringList keys() const { return m_jsonObject.keys(); }

    // convenience value access
    template<typename T>
    T typedValue(const QString &key) const;
    template<typename T>
    Utils::optional<T> optionalValue(const QString &key) const;
    template<typename T>
    LanguageClientValue<T> clientValue(const QString &key) const;
    template<typename T>
    Utils::optional<LanguageClientValue<T>> optionalClientValue(const QString &key) const;
    template<typename T>
    QList<T> array(const QString &key) const;
    template<typename T>
    Utils::optional<QList<T>> optionalArray(const QString &key) const;
    template<typename T>
    LanguageClientArray<T> clientArray(const QString &key) const;
    template<typename T>
    Utils::optional<LanguageClientArray<T>> optionalClientArray(const QString &key) const;
    template<typename T>
    void insertArray(const QString &key, const QList<T> &array);
    template<typename>
    void insertArray(const QString &key, const QList<JsonObject> &array);

    // value checking
    bool checkKey(QStringList *errorHierarchy, const QString &key,
                  const std::function<bool(const QJsonValue &val)> &predicate) const;
    static QString valueTypeString(QJsonValue::Type type);
    static QString errorString(QJsonValue::Type expected, QJsonValue::Type type2);
    static bool checkType(QJsonValue::Type type,
                          QJsonValue::Type expectedType,
                          QStringList *errorHierarchy);
    template <typename T>
    static bool checkVal(QStringList *errorHierarchy, const QJsonValue &val);
    template <typename T1, typename T2, typename... Args>
    bool check(QStringList *errorHierarchy, const QString &key) const;
    template <typename T>
    bool check(QStringList *errorHierarchy, const QString &key) const;
    template <typename T>
    bool checkArray(QStringList *errorHierarchy, const QString &key) const;
    template <typename T1, typename T2, typename... Args>
    bool checkOptional(QStringList *errorHierarchy, const QString &key) const;
    template <typename T>
    bool checkOptional(QStringList *errorHierarchy, const QString &key) const;
    template <typename T>
    bool checkOptionalArray(QStringList *errorHierarchy, const QString &key) const;

private:
    QJsonObject m_jsonObject;
};

template<typename T>
T JsonObject::typedValue(const QString &key) const
{
    return fromJsonValue<T>(value(key));
}

template<typename T>
Utils::optional<T> JsonObject::optionalValue(const QString &key) const
{
    const QJsonValue &val = value(key);
    return val.isUndefined() ? Utils::nullopt : Utils::make_optional(fromJsonValue<T>(val));
}

template<typename T>
LanguageClientValue<T> JsonObject::clientValue(const QString &key) const
{
    return LanguageClientValue<T>(value(key));
}

template<typename T>
Utils::optional<LanguageClientValue<T>> JsonObject::optionalClientValue(const QString &key) const
{
    return contains(key) ? Utils::make_optional(clientValue<T>(key)) : Utils::nullopt;
}

template<typename T>
QList<T> JsonObject::array(const QString &key) const
{
    return LanguageClientArray<T>(value(key)).toList();
}

template<typename T>
Utils::optional<QList<T>> JsonObject::optionalArray(const QString &key) const
{
    using Result = Utils::optional<QList<T>>;
    return contains(key) ? Result(LanguageClientArray<T>(value(key)).toList())
                         : Result(Utils::nullopt);
}

template<typename T>
LanguageClientArray<T> JsonObject::clientArray(const QString &key) const
{
    return LanguageClientArray<T>(value(key));
}

template<typename T>
Utils::optional<LanguageClientArray<T>> JsonObject::optionalClientArray(const QString &key) const
{
    const QJsonValue &val = value(key);
    return !val.isUndefined() ? Utils::make_optional(LanguageClientArray<T>(value(key)))
                              : Utils::nullopt;
}

template<typename T>
void JsonObject::insertArray(const QString &key, const QList<T> &array)
{
    QJsonArray jsonArray;
    for (const T &item : array)
        jsonArray.append(QJsonValue(item));
    insert(key, jsonArray);
}

template<typename >
void JsonObject::insertArray(const QString &key, const QList<JsonObject> &array)
{
    QJsonArray jsonArray;
    for (const JsonObject &item : array)
        jsonArray.append(item.m_jsonObject);
    insert(key, jsonArray);
}

template <typename T>
bool JsonObject::checkVal(QStringList *errorHierarchy, const QJsonValue &val)
{
    return checkType(val.type(), QJsonValue::Object, errorHierarchy)
            && T(val).isValid(errorHierarchy);
}

template <typename T1, typename T2, typename... Args>
bool JsonObject::check(QStringList *errorHierarchy, const QString &key) const
{
    const QStringList errorBackUp = errorHierarchy ? *errorHierarchy : QStringList();
    if (check<T1>(errorHierarchy, key))
        return true;

    const bool ret = check<T2, Args...>(errorHierarchy, key);
    if (ret && errorHierarchy)
        *errorHierarchy = errorBackUp;
    return ret;
}

template <typename T>
bool JsonObject::check(QStringList *errorHierarchy, const QString &key) const
{
    return checkKey(errorHierarchy, key, [errorHierarchy](const QJsonValue &val){
        return checkVal<T>(errorHierarchy, val);
    });
}

template <typename T>
bool JsonObject::checkArray(QStringList *errorHierarchy, const QString &key) const
{
    return checkKey(errorHierarchy, key, [errorHierarchy](const QJsonValue &val){
        return val.isArray() && Utils::allOf(val.toArray(), [&errorHierarchy](const QJsonValue &value){
            return checkVal<T>(errorHierarchy, value);
        });
    });
}

template <typename T1, typename T2, typename... Args>
bool JsonObject::checkOptional(QStringList *errorHierarchy, const QString &key) const
{
    const QStringList errorBackUp = errorHierarchy ? *errorHierarchy : QStringList();
    if (checkOptional<T1>(errorHierarchy, key))
        return true;

    const bool ret = checkOptional<T2, Args...>(errorHierarchy, key);
    if (ret && errorHierarchy)
        *errorHierarchy = errorBackUp;
    return ret;
}

template <typename T>
bool JsonObject::checkOptional(QStringList *errorHierarchy, const QString &key) const
{
    if (contains(key))
        return check<T>(errorHierarchy, key);
    return true;
}

template <typename T>
bool JsonObject::checkOptionalArray(QStringList *errorHierarchy, const QString &key) const
{
    if (contains(key))
        return checkArray<T>(errorHierarchy, key);
    return true;
}

template <>
LANGUAGESERVERPROTOCOL_EXPORT bool JsonObject::checkVal<QString>(QStringList *errorHierarchy,
                                                                 const QJsonValue &val);

template <>
LANGUAGESERVERPROTOCOL_EXPORT bool JsonObject::checkVal<int>(QStringList *errorHierarchy,
                                                             const QJsonValue &val);

template <>
LANGUAGESERVERPROTOCOL_EXPORT bool JsonObject::checkVal<double>(QStringList *errorHierarchy,
                                                                const QJsonValue &val);

template <>
LANGUAGESERVERPROTOCOL_EXPORT bool JsonObject::checkVal<bool>(QStringList *errorHierarchy,
                                                              const QJsonValue &val);

template <>
LANGUAGESERVERPROTOCOL_EXPORT bool JsonObject::checkVal<std::nullptr_t>(QStringList *errorHierarchy,
                                                                        const QJsonValue &val);

template<>
LANGUAGESERVERPROTOCOL_EXPORT bool JsonObject::checkVal<QJsonArray>(QStringList *errorHierarchy,
                                                                    const QJsonValue &val);

template<>
LANGUAGESERVERPROTOCOL_EXPORT bool JsonObject::checkVal<QJsonValue>(QStringList *errorHierarchy,
                                                                    const QJsonValue &val);

} // namespace LanguageServerProtocol
