// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "jsontreeitem.h"

#include "utilstr.h"

#include <QJsonArray>
#include <QJsonObject>

namespace Utils {

JsonTreeItem::JsonTreeItem(const QString &displayName, const QJsonValue &value)
    : m_name(displayName)
    , m_value(value)
{ }

static QString typeName(QJsonValue::Type type)
{
    switch (type) {
    case QJsonValue::Null:
        return Tr::tr("Null");
    case QJsonValue::Bool:
        return Tr::tr("Bool");
    case QJsonValue::Double:
        return Tr::tr("Double");
    case QJsonValue::String:
        return Tr::tr("String");
    case QJsonValue::Array:
        return Tr::tr("Array");
    case QJsonValue::Object:
        return Tr::tr("Object");
    case QJsonValue::Undefined:
        return Tr::tr("Undefined");
    }
    return {};
}

QVariant JsonTreeItem::data(int column, int role) const
{
    if (role != Qt::DisplayRole)
        return {};
    if (column == 0)
        return m_name;
    if (column == 2)
        return typeName(m_value.type());
    if (m_value.isObject())
        return QString('[' + Tr::tr("%n Items", nullptr, m_value.toObject().size()) + ']');
    if (m_value.isArray())
        return QString('[' + Tr::tr("%n Items", nullptr, m_value.toArray().size()) + ']');
    return m_value.toVariant();
}

bool JsonTreeItem::canFetchMore() const
{
    return canFetchObjectChildren() || canFetchArrayChildren();
}

void JsonTreeItem::fetchMore()
{
    if (canFetchObjectChildren()) {
        const QJsonObject &object = m_value.toObject();
        for (const QString &key : object.keys())
            appendChild(new JsonTreeItem(key, object.value(key)));
    } else if (canFetchArrayChildren()) {
        int index = 0;
        const QJsonArray &array = m_value.toArray();
        for (const QJsonValue &val : array)
            appendChild(new JsonTreeItem(QString::number(index++), val));
    }
}

bool JsonTreeItem::canFetchObjectChildren() const
{
    return m_value.isObject() && m_value.toObject().size() > childCount();
}

bool JsonTreeItem::canFetchArrayChildren() const
{
    return m_value.isArray() && m_value.toArray().size() > childCount();
}

} // namespace Utils
