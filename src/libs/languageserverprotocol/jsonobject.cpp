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

#include "jsonobject.h"

#include <QCoreApplication>

namespace LanguageServerProtocol {

template <>
bool JsonObject::checkVal<QString>(QStringList *errorHierarchy, const QJsonValue &val)
{ return checkType(val.type(), QJsonValue::String, errorHierarchy); }

template <>
bool JsonObject::checkVal<int>(QStringList *errorHierarchy, const QJsonValue &val)
{ return checkType(val.type(), QJsonValue::Double, errorHierarchy); }

template <>
bool JsonObject::checkVal<double>(QStringList *errorHierarchy, const QJsonValue &val)
{ return checkType(val.type(), QJsonValue::Double, errorHierarchy); }

template <>
bool JsonObject::checkVal<bool>(QStringList *errorHierarchy, const QJsonValue &val)
{ return checkType(val.type(), QJsonValue::Bool, errorHierarchy); }

template <>
bool JsonObject::checkVal<std::nullptr_t>(QStringList *errorHierarchy, const QJsonValue &val)
{ return checkType(val.type(), QJsonValue::Null, errorHierarchy); }

template<>
bool JsonObject::checkVal<QJsonArray>(QStringList *errorHierarchy, const QJsonValue &val)
{ return checkType(val.type(), QJsonValue::Array, errorHierarchy); }

template<>
bool JsonObject::checkVal<QJsonValue>(QStringList * /*errorHierarchy*/, const QJsonValue &/*val*/)
{ return true; }

JsonObject &JsonObject::operator=(const JsonObject &other) = default;

JsonObject &JsonObject::operator=(JsonObject &&other)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    m_jsonObject.swap(other.m_jsonObject);
#else
    m_jsonObject = other.m_jsonObject; // NOTE use QJsonObject::swap when minimum required Qt version >= 5.10
#endif
    return *this;
}

QJsonObject::iterator JsonObject::insert(const QString &key, const JsonObject &object)
{
    return m_jsonObject.insert(key, object.m_jsonObject);
}

QJsonObject::iterator JsonObject::insert(const QString &key, const QJsonValue &value)
{
    return m_jsonObject.insert(key, value);
}

bool JsonObject::checkKey(QStringList *errorHierarchy, const QString &key,
                          const std::function<bool (const QJsonValue &)> &predicate) const
{
    const bool valid = predicate(m_jsonObject.value(key));
    if (!valid && errorHierarchy)
        errorHierarchy->append(key);
    return valid;
}

QString JsonObject::valueTypeString(QJsonValue::Type type)
{
    switch (type) {
    case QJsonValue::Null: return QString("Null");
    case QJsonValue::Bool: return QString("Bool");
    case QJsonValue::Double: return QString("Double");
    case QJsonValue::String: return QString("String");
    case QJsonValue::Array: return QString("Array");
    case QJsonValue::Object: return QString("Object");
    case QJsonValue::Undefined: return QString("Undefined");
    }
    return QString();
}

QString JsonObject::errorString(QJsonValue::Type expected, QJsonValue::Type actual)
{
    return tr("Expected type %1 but value contained %2")
            .arg(valueTypeString(expected), valueTypeString(actual));
}

bool JsonObject::checkType(QJsonValue::Type type,
                           QJsonValue::Type expectedType,
                           QStringList *errorHierarchy)
{
    const bool ret = type == expectedType;
    if (!ret && errorHierarchy)
        errorHierarchy->append(errorString(expectedType, type));
    return ret;
}

} // namespace LanguageServerProtocol
