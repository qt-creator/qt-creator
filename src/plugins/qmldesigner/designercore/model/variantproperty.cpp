// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "variantproperty.h"
#include "internalproperty.h"
#include "internalnode_p.h"
#include "model.h"
#include "model_p.h"



namespace QmlDesigner {

VariantProperty::VariantProperty() = default;

VariantProperty::VariantProperty(const VariantProperty &property, AbstractView *view)
    : AbstractProperty(property.name(), property.internalNode(), property.model(), view)
{

}

VariantProperty::VariantProperty(const PropertyName &propertyName, const Internal::InternalNodePointer &internalNode, Model* model,  AbstractView *view) :
        AbstractProperty(propertyName, internalNode, model, view)
{
}

void VariantProperty::setValue(const QVariant &value)
{
    if (!isValid())
        return;

    if (!value.isValid())
        return;

    Internal::WriteLocker locker(model());
    if (isDynamic())
        qWarning() << "Calling VariantProperty::setValue on dynamic property.";


    if (internalNode()->hasProperty(name())) { //check if oldValue != value
        Internal::InternalProperty::Pointer internalProperty = internalNode()->property(name());
        if (internalProperty->isVariantProperty()
            && internalProperty->toVariantProperty()->value() == value
            && dynamicTypeName().isEmpty())

            return;
    }

    if (internalNode()->hasProperty(name()) && !internalNode()->property(name())->isVariantProperty())
        privateModel()->removeProperty(internalNode()->property(name()));

    privateModel()->setVariantProperty(internalNode(), name(), value);
}

QVariant VariantProperty::value() const
{
    if (isValid() && internalNode()->hasProperty(name())
        && internalNode()->property(name())->isVariantProperty())
        return internalNode()->variantProperty(name())->value();

    return QVariant();
}

void VariantProperty::setEnumeration(const EnumerationName &enumerationName)
{
    setValue(QVariant::fromValue(Enumeration(enumerationName)));
}

Enumeration VariantProperty::enumeration() const
{
    return value().value<Enumeration>();
}

bool VariantProperty::holdsEnumeration() const
{
    return value().canConvert<Enumeration>();
}

void VariantProperty::setDynamicTypeNameAndValue(const TypeName &type, const QVariant &value)
{
    if (!isValid())
        return;

    if (type.isEmpty())
        return;

    Internal::WriteLocker locker(model());

    if (internalNode()->hasProperty(name())) { //check if oldValue != value
        Internal::InternalProperty::Pointer internalProperty = internalNode()->property(name());
        if (internalProperty->isVariantProperty()
            && internalProperty->toVariantProperty()->value() == value
            && internalProperty->toVariantProperty()->dynamicTypeName() == type)

            return;
    }

    if (internalNode()->hasProperty(name()) && !internalNode()->property(name())->isVariantProperty())
        privateModel()->removeProperty(internalNode()->property(name()));

   privateModel()->setDynamicVariantProperty(internalNode(), name(), type, value);
}

void VariantProperty::setDynamicTypeNameAndEnumeration(const TypeName &type, const EnumerationName &enumerationName)
{
    setDynamicTypeNameAndValue(type, QVariant::fromValue(Enumeration(enumerationName)));
}

QDebug operator<<(QDebug debug, const VariantProperty &property)
{
    return debug.nospace() << "VariantProperty(" << property.name() << ',' << ' ' << property.value().toString() << ' ' << property.value().typeName() << property.parentModelNode() << ')';
}

QTextStream& operator<<(QTextStream &stream, const VariantProperty &property)
{
    stream << "VariantProperty(" << property.name() << ',' << ' ' << property.value().toString() << ' ' << property.value().typeName() << property.parentModelNode() << ')';

    return stream;
}

} // namespace QmlDesigner
