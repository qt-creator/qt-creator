// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "propertytreeitem.h"

#include "squishtr.h"

#include <utils/qtcassert.h>

using namespace Utils;

namespace Squish {
namespace Internal {

// Squish IDE uses lower-case for "Is" - shall we too?
const QString Property::OPERATOR_IS = "Is";
const QString Property::OPERATOR_EQUALS = "Equals";
const QString Property::OPERATOR_REGEX = "RegEx";
const QString Property::OPERATOR_WILDCARD = "Wildcard";

/*************************************** Property *******************************************/

Property::Property() {}

Property::Property(const QByteArray &data)
{
    const int equalsPosition = data.indexOf('=');
    // no equals sign found or name is empty?
    if (equalsPosition <= 0)
        return;

    QByteArray namePart = data.left(equalsPosition).trimmed();
    QByteArray valuePart = data.mid(equalsPosition + 1).trimmed();

    if (!valuePart.startsWith('\'') || !valuePart.endsWith('\''))
        return;

    const int namePartSize = namePart.size();
    if (namePartSize > 1) {
        char lastChar = namePart.at(namePartSize - 1);

        if (lastChar == '~' || lastChar == '?') {
            namePart.chop(1);
            m_type = lastChar == '~' ? RegularExpression : Wildcard;
        }
        m_name = QLatin1String(namePart.trimmed());
    }

    m_value = QLatin1String(valuePart.mid(1, valuePart.size() - 2));
}

bool Property::set(const QString &propName, const QString &oper, const QString &propValue)
{
    if (oper == "=")
        m_type = Equals;
    else if (oper == "~=")
        m_type = RegularExpression;
    else if (oper == "?=")
        m_type = Wildcard;
    else
        return false;
    m_name = propName;
    m_value = propValue;
    return true;
}

const QStringList Property::toStringList() const
{
    QStringList result(m_name);
    switch (m_type) {
    case Equals:
        if (isContainer() || isRelativeWidget())
            result << OPERATOR_IS;
        else
            result << OPERATOR_EQUALS;
        break;
    case RegularExpression:
        result << OPERATOR_REGEX;
        break;
    case Wildcard:
        result << OPERATOR_WILDCARD;
        break;
    default:
        QTC_ASSERT(false, result << QString());
        break;
    }
    result << m_value;
    return result;
}

bool Property::isContainer() const
{
    static const char container[] = "container";
    static const char window[] = "window";

    return m_name == container || m_name == window;
}

bool Property::isRelativeWidget() const
{
    static const QStringList relatives({"buddy", "aboveWidget", "leftWidget", "parentWidget"});
    return relatives.contains(m_name);
}

PropertyType Property::typeFromString(const QString &typeString)
{
    if (typeString == OPERATOR_EQUALS || typeString == OPERATOR_IS)
        return Equals;
    if (typeString == OPERATOR_REGEX)
        return RegularExpression;
    if (typeString == OPERATOR_WILDCARD)
        return Wildcard;
    QTC_ASSERT(false, return Equals);
}

const QString Property::toString() const
{
    switch (m_type) {
    case Equals:
        return QString::fromLatin1("%1='%2'").arg(m_name, m_value);
    case RegularExpression:
        return QString::fromLatin1("%1~='%2'").arg(m_name, m_value);
    case Wildcard:
        return QString::fromLatin1("%1?='%2'").arg(m_name, m_value);
    }
    QTC_ASSERT(false, return QString());
}

/*********************************** PropertyTreeItem ***************************************/

enum ViewColumn { NameColumn, OperatorColumn, ValueColumn };

PropertyTreeItem::PropertyTreeItem(const Property &property, Qt::ItemFlags flags)
    : m_property(property)
    , m_flags(flags)
{}

QVariant PropertyTreeItem::data(int column, int role) const
{
    if (role == Qt::DisplayRole && column >= NameColumn && column <= ValueColumn)
        return m_property.toStringList().at(column);

    return TreeItem::data(column, role);
}

bool PropertyTreeItem::setData(int column, const QVariant &data, int /*role*/)
{
    // only accept untrimmed data for ValueColumn
    const QString value = column == ValueColumn ? data.toString() : data.toString().trimmed();
    if (value.isEmpty() && column != ValueColumn)
        return false;

    switch (column) {
    case NameColumn:
        m_property.m_name = value;
        return true;
    case OperatorColumn:
        m_property.m_type = Property::typeFromString(value);
        return true;
    case ValueColumn:
        m_property.m_value = value;
        return true;
    }
    return false;
}

Qt::ItemFlags PropertyTreeItem::flags(int column) const
{
    if (m_flags != Qt::NoItemFlags)
        return m_flags;
    return TreeItem::flags(column);
}

/*********************************** PropertiesModel ****************************************/

PropertiesModel::PropertiesModel(ObjectsMapTreeItem *parentItem)
    : TreeModel<PropertyTreeItem>(new PropertyTreeItem({}))
    , m_parentItem(parentItem)
{
    setHeader({Tr::tr("Name"), Tr::tr("Operator"), Tr::tr("Value")});
}

bool PropertiesModel::setData(const QModelIndex &idx, const QVariant &data, int role)
{
    // only editing is supported
    if (role != Qt::EditRole || !data.isValid())
        return false;

    const int column = idx.column();
    if (column < NameColumn || column > ValueColumn)
        return false;

    const QString old = idx.data().toString();
    bool result = TreeModel::setData(idx, data, role);
    if (result)
        emit propertyChanged(m_parentItem, old, data.toString(), idx.row(), idx.column());
    return result;
}

void PropertiesModel::addNewProperty(PropertyTreeItem *item)
{
    QTC_ASSERT(item, return );
    QTC_ASSERT(rootItem(), return );

    rootItem()->appendChild(item);
    emit propertyAdded(m_parentItem);
}

void PropertiesModel::removeProperty(PropertyTreeItem *item)
{
    QTC_ASSERT(item, return );

    Property property = item->property();
    delete takeItem(item);
    emit propertyRemoved(m_parentItem, property);
}

void PropertiesModel::modifySpecialProperty(const QString &oldValue, const QString &newValue)
{
    TreeItem *root = rootItem();
    QTC_ASSERT(root, return );

    TreeItem *itemToChange = root->findChildAtLevel(1, [oldValue](TreeItem *child) {
        auto propertyItem = static_cast<PropertyTreeItem *>(child);
        Property property = propertyItem->property();
        return (property.m_value == oldValue
                && (property.isContainer() || property.isRelativeWidget()));
    });

    if (!itemToChange)
        return;

    auto propertyItem = static_cast<PropertyTreeItem *>(itemToChange);
    propertyItem->setData(ValueColumn, newValue, Qt::EditRole);
    const QModelIndex idx = indexForItem(propertyItem);
    emit propertyChanged(m_parentItem, oldValue, newValue, idx.row(), idx.column());
}

QStringList PropertiesModel::allPropertyNames() const
{
    TreeItem *root = rootItem();
    if (!root)
        return {};

    QStringList result;
    result.reserve(root->childCount());
    root->forChildrenAtLevel(1, [&result](TreeItem *child) {
        result.append(child->data(NameColumn, Qt::DisplayRole).toString());
    });
    return result;
}

/********************************* PropertiesSortModel **************************************/

PropertiesSortModel::PropertiesSortModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{}

bool PropertiesSortModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    return left.data().toString() > right.data().toString();
}

} // namespace Internal
} // namespace Squish
