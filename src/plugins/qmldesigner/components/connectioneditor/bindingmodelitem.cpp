// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "bindingmodelitem.h"
#include <scripteditorutils.h>

#include <modelnode.h>
#include <qmldesignertr.h>

namespace QmlDesigner {

QHash<int, QByteArray> BindingModelItem::roleNames()
{
    return {{TargetNameRole, "target"},
            {TargetPropertyNameRole, "targetProperty"},
            {SourceNameRole, "source"},
            {SourcePropertyNameRole, "sourceProperty"}};
}

QStringList BindingModelItem::headerLabels()
{
    return {Tr::tr("Item"), Tr::tr("Property"), Tr::tr("Source Item"), Tr::tr("Source Property")};
}

BindingModelItem::BindingModelItem(const BindingProperty &property)
    : QStandardItem(property.parentModelNode().displayName())
{
    updateProperty(property);
}

int BindingModelItem::internalId() const
{
    return data(InternalIdRole).toInt();
}

PropertyName BindingModelItem::targetPropertyName() const
{
    return data(TargetPropertyNameRole).toString().toUtf8();
}

void BindingModelItem::updateProperty(const BindingProperty &property)
{
    setData(property.parentModelNode().internalId(), InternalIdRole);
    setData(property.parentModelNode().displayName(), TargetNameRole);
    setData(property.name().toByteArray(), TargetPropertyNameRole);

    // TODO: Make this safe when the new codemodel allows it.
    if (auto expression = property.expression(); !expression.isEmpty()) {
        auto [nodeName, propertyName] = splitExpression(expression);
        setData(nodeName, SourceNameRole);
        setData(propertyName, SourcePropertyNameRole);
    }
}

} // End namespace QmlDesigner.
