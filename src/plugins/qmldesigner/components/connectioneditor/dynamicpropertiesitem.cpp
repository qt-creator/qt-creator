// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dynamicpropertiesitem.h"
#include <scripteditorutils.h>

#include <abstractproperty.h>
#include <abstractview.h>
#include <bindingproperty.h>
#include <modelnode.h>
#include <variantproperty.h>
#include <qmldesignertr.h>
#include <qmlobjectnode.h>

namespace QmlDesigner {

QHash<int, QByteArray> DynamicPropertiesItem::roleNames()
{
    return {{TargetNameRole, "target"},
            {PropertyNameRole, "name"},
            {PropertyTypeRole, "type"},
            {PropertyValueRole, "value"},
            {InstancePropertyValueRole, "instanceValue"}};
}

QStringList DynamicPropertiesItem::headerLabels()
{
    return {Tr::tr("Item"), Tr::tr("Property"), Tr::tr("Property Type"), Tr::tr("Property Value")};
}

DynamicPropertiesItem::DynamicPropertiesItem(const AbstractProperty &property)
    : QStandardItem(property.parentModelNode().displayName())
{
    updateProperty(property);
}

int DynamicPropertiesItem::internalId() const
{
    return data(InternalIdRole).toInt();
}

PropertyName DynamicPropertiesItem::propertyName() const
{
    return data(PropertyNameRole).toString().toUtf8();
}

std::optional<const QmlObjectNode> parentIfNotDefaultState(const AbstractProperty &property)
{
    const QmlObjectNode objectNode = QmlObjectNode(property.parentModelNode());
    if (objectNode.isValid() && !QmlModelState::isBaseState(objectNode.view()->currentStateNode()))
        return objectNode;
    return std::nullopt;
}

void DynamicPropertiesItem::updateProperty(const AbstractProperty &property)
{
    setData(property.parentModelNode().internalId(), InternalIdRole);
    setData(property.parentModelNode().displayName(), TargetNameRole);
    setData(property.name().toByteArray(), PropertyNameRole);
    setData(property.dynamicTypeName(), PropertyTypeRole);

    const auto qmlObjectNode = QmlObjectNode(property.parentModelNode());
    setData(qmlObjectNode.instanceValue(property.name()), InstancePropertyValueRole);

    if (property.isVariantProperty()) {
        if (std::optional<const QmlObjectNode> nodeInState = parentIfNotDefaultState(property))
            setData(nodeInState->modelValue(property.name()), PropertyValueRole);
        else
            setData(property.toVariantProperty().value(), PropertyValueRole);
    } else if (property.isBindingProperty()) {
        if (std::optional<const QmlObjectNode> nodeInState = parentIfNotDefaultState(property))
            setData(nodeInState->expression(property.name()), PropertyValueRole);
        else
            setData(property.toBindingProperty().expression(), PropertyValueRole);
    }
}

} // End namespace QmlDesigner.
