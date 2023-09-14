// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "signalhandlerproperty.h"
#include "internalproperty.h"
#include "internalnode_p.h"
#include "model.h"
#include "model_p.h"
namespace QmlDesigner {

SignalHandlerProperty::SignalHandlerProperty() = default;

SignalHandlerProperty::SignalHandlerProperty(const SignalHandlerProperty &property, AbstractView *view)
    : AbstractProperty(property.name(), property.internalNodeSharedPointer(), property.model(), view)
{
}

SignalHandlerProperty::SignalHandlerProperty(const PropertyName &propertyName, const Internal::InternalNodePointer &internalNode, Model* model, AbstractView *view)
    : AbstractProperty(propertyName, internalNode, model, view)
{
}

void SignalHandlerProperty::setSource(const QString &source)
{
    Internal::WriteLocker locker(model());
    if (!isValid())
        return;

    if (name() == "id")
        return;

    if (source.isEmpty())
        return;

    if (auto internalProperty = internalNode()->property(name())) {
        auto signalHandlerProperty = internalProperty->to<PropertyType::SignalHandler>();
        //check if oldValue != value
        if (signalHandlerProperty && signalHandlerProperty->source() == source)
            return;

        if (!signalHandlerProperty)
            privateModel()->removePropertyAndRelatedResources(internalProperty);
    }

    privateModel()->setSignalHandlerProperty(internalNodeSharedPointer(), name(), source);
}

QString SignalHandlerProperty::source() const
{
    if (!isValid())
        return {};

    if (auto property = internalNode()->signalHandlerProperty(name()))
        return property->source();

    return QString();
}

PropertyName SignalHandlerProperty::prefixAdded(const PropertyName &propertyName)
{
    QString nameAsString = QString::fromUtf8(propertyName);
    if (nameAsString.startsWith("on"))
        return propertyName;

    QChar firstChar = nameAsString.at(0).toUpper();
    nameAsString[0] = firstChar;
    nameAsString.prepend("on");

    return nameAsString.toLatin1();
}

PropertyName SignalHandlerProperty::prefixRemoved(const PropertyName &propertyName)
{
    QString nameAsString = QString::fromUtf8(propertyName);
    if (!nameAsString.startsWith("on"))
        return propertyName;

    nameAsString.remove(0, 2);
    QChar firstChar = nameAsString.at(0).toLower();
    nameAsString[0] = firstChar;

    return nameAsString.toLatin1();
}

SignalDeclarationProperty::SignalDeclarationProperty() = default;

SignalDeclarationProperty::SignalDeclarationProperty(const SignalDeclarationProperty &property,
                                                     AbstractView *view)
    : AbstractProperty(property.name(), property.internalNodeSharedPointer(), property.model(), view)
{}

SignalDeclarationProperty::SignalDeclarationProperty(
    const PropertyName &propertyName,
    const Internal::InternalNodePointer &internalNode,
    Model *model,
    AbstractView *view)
    : AbstractProperty(propertyName, internalNode, model, view)
{
}

void SignalDeclarationProperty::setSignature(const QString &signature)
{
    Internal::WriteLocker locker(model());
    if (!isValid())
        return;

    if (name() == "id")
        return;

    if (signature.isEmpty())
        return;

    if (auto internalProperty = internalNode()->property(name())) {
        auto signalDeclarationProperty = internalProperty->to<PropertyType::SignalDeclaration>();
        //check if oldValue != value
        if (signalDeclarationProperty && signalDeclarationProperty->signature() == signature)
            return;

        if (!signalDeclarationProperty)
            privateModel()->removePropertyAndRelatedResources(internalProperty);
    }

    privateModel()->setSignalDeclarationProperty(internalNodeSharedPointer(), name(), signature);
}

QString SignalDeclarationProperty::signature() const
{
    if (!isValid())
        return {};

    if (auto property = internalNode()->signalDeclarationProperty(name()))
        return property->signature();

    return QString();
}

} // namespace QmlDesigner
