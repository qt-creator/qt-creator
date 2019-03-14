/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "jsontreeitem.h"

#include <QJsonArray>
#include <QJsonObject>

Utils::JsonTreeItem::JsonTreeItem(const QString &displayName, const QJsonValue &value)
    : m_name(displayName)
    , m_value(value)
{ }

static QString typeName(QJsonValue::Type type)
{
    switch (type) {
    case QJsonValue::Null:
        return Utils::JsonTreeItem::tr("Null");
    case QJsonValue::Bool:
        return Utils::JsonTreeItem::tr("Bool");
    case QJsonValue::Double:
        return Utils::JsonTreeItem::tr("Double");
    case QJsonValue::String:
        return Utils::JsonTreeItem::tr("String");
    case QJsonValue::Array:
        return Utils::JsonTreeItem::tr("Array");
    case QJsonValue::Object:
        return Utils::JsonTreeItem::tr("Object");
    case QJsonValue::Undefined:
        return Utils::JsonTreeItem::tr("Undefined");
    }
    return {};
}

QVariant Utils::JsonTreeItem::data(int column, int role) const
{
    if (role != Qt::DisplayRole)
        return {};
    if (column == 0)
        return m_name;
    if (column == 2)
        return typeName(m_value.type());
    if (m_value.isObject())
        return QString('[' + tr("%n Items", nullptr, m_value.toObject().size()) + ']');
    if (m_value.isArray())
        return QString('[' + tr("%n Items", nullptr, m_value.toArray().size()) + ']');
    return m_value.toVariant();
}

bool Utils::JsonTreeItem::canFetchMore() const
{
    return canFetchObjectChildren() || canFetchArrayChildren();
}

void Utils::JsonTreeItem::fetchMore()
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

bool Utils::JsonTreeItem::canFetchObjectChildren() const
{
    return m_value.isObject() && m_value.toObject().size() > childCount();
}

bool Utils::JsonTreeItem::canFetchArrayChildren() const
{
    return m_value.isArray() && m_value.toArray().size() > childCount();
}
