/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "variantproperty.h"
#include "internalproperty.h"
#include "invalidmodelnodeexception.h"
#include "invalidargumentexception.h"
#include "internalnode_p.h"
#include "model.h"
#include "model_p.h"



namespace QmlDesigner {

VariantProperty::VariantProperty()
{}

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
    Internal::WriteLocker locker(model());
    if (!isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    if (isDynamic())
        qWarning() << "Calling VariantProperty::setValue on dynamic property.";

    if (value.isNull())
        throw InvalidArgumentException(__LINE__, __FUNCTION__, __FILE__, name());

    if (internalNode()->hasProperty(name())) { //check if oldValue != value
        Internal::InternalProperty::Pointer internalProperty = internalNode()->property(name());
        if (internalProperty->isVariantProperty()
            && internalProperty->toVariantProperty()->value() == value
            && dynamicTypeName().isEmpty())

            return;
    }

    if (internalNode()->hasProperty(name()) && !internalNode()->property(name())->isVariantProperty())
        model()->d->removeProperty(internalNode()->property(name()));

    model()->d->setVariantProperty(internalNode(), name(), value);
}

QVariant VariantProperty::value() const
{
    if (internalNode()->hasProperty(name())
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
    Internal::WriteLocker locker(model());
    if (!isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);


    if (type.isEmpty())
        throw InvalidArgumentException(__LINE__, __FUNCTION__, __FILE__, name());

    if (internalNode()->hasProperty(name())) { //check if oldValue != value
        Internal::InternalProperty::Pointer internalProperty = internalNode()->property(name());
        if (internalProperty->isVariantProperty()
            && internalProperty->toVariantProperty()->value() == value
            && internalProperty->toVariantProperty()->dynamicTypeName() == type)

            return;
    }

    if (internalNode()->hasProperty(name()) && !internalNode()->property(name())->isVariantProperty())
        model()->d->removeProperty(internalNode()->property(name()));

    model()->d->setDynamicVariantProperty(internalNode(), name(), type, value);
}

void VariantProperty::setDynamicTypeNameAndEnumeration(const TypeName &type, const EnumerationName &enumerationName)
{
    setDynamicTypeNameAndValue(type, QVariant::fromValue(Enumeration(enumerationName)));
}

QDebug operator<<(QDebug debug, const VariantProperty &VariantProperty)
{
    return debug.nospace() << "VariantProperty(" << VariantProperty.name() << ')';
}
QTextStream& operator<<(QTextStream &stream, const VariantProperty &property)
{
    stream << "VariantProperty(" << property.name() << ',' << ' ' << property.value().toString() << ' ' << property.value().typeName() << ')';

    return stream;
}

} // namespace QmlDesigner
