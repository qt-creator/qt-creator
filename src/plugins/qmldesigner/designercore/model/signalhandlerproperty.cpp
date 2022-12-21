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
    : AbstractProperty(property.name(), property.internalNode(), property.model(), view)
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

    if (internalNode()->hasProperty(name())) { //check if oldValue != value
        Internal::InternalProperty::Pointer internalProperty = internalNode()->property(name());
        if (internalProperty->isSignalHandlerProperty()
            && internalProperty->toSignalHandlerProperty()->source() == source)

            return;
    }

    if (internalNode()->hasProperty(name()) && !internalNode()->property(name())->isSignalHandlerProperty())
        privateModel()->removeProperty(internalNode()->property(name()));

    privateModel()->setSignalHandlerProperty(internalNode(), name(), source);
}

QString SignalHandlerProperty::source() const
{
    if (internalNode()->hasProperty(name())
        && internalNode()->property(name())->isSignalHandlerProperty())
        return internalNode()->signalHandlerProperty(name())->source();

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
    : AbstractProperty(property.name(), property.internalNode(), property.model(), view)
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

    if (internalNode()->hasProperty(name())) { //check if oldValue != value
        Internal::InternalProperty::Pointer internalProperty = internalNode()->property(name());
        if (internalProperty->isSignalDeclarationProperty()
            && internalProperty->toSignalDeclarationProperty()->signature() == signature)

            return;
    }

    if (internalNode()->hasProperty(name()) && !internalNode()->property(name())->isSignalDeclarationProperty())
        privateModel()->removeProperty(internalNode()->property(name()));

    privateModel()->setSignalDeclarationProperty(internalNode(), name(), signature);
}

QString SignalDeclarationProperty::signature() const
{
    if (internalNode() && internalNode()->hasProperty(name())
        && internalNode()->property(name())->isSignalDeclarationProperty())
        return internalNode()->signalDeclarationProperty(name())->signature();

    return QString();
}

} // namespace QmlDesigner
