/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "variantproperty.h"
#include "internalproperty.h"
#include "internalvariantproperty.h"
#include "invalidmodelnodeexception.h"
#include "invalidpropertyexception.h"
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

VariantProperty::VariantProperty(const QString &propertyName, const Internal::InternalNodePointer &internalNode, Model* model,  AbstractView *view) :
        AbstractProperty(propertyName, internalNode, model, view)
{
}

void VariantProperty::setValue(const QVariant &value)
{
    Internal::WriteLocker locker(model());
    if (!isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

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

VariantProperty& VariantProperty::operator= (const QVariant &value)
{
    setValue(value);

    return *this;
}

void VariantProperty::setDynamicTypeNameAndValue(const QString &type, const QVariant &value)
{
    Internal::WriteLocker locker(model());
    if (!isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);


    if (type.isEmpty()) {
        throw InvalidArgumentException(__LINE__, __FUNCTION__, __FILE__, name());
    }

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

VariantProperty& VariantProperty::operator= (const QPair<QString, QVariant> &typeValuePair)
{
    setDynamicTypeNameAndValue(typeValuePair.first, typeValuePair.second);
    return *this;
}





QDebug operator<<(QDebug debug, const VariantProperty &VariantProperty)
{
    return debug.nospace() << "VariantProperty(" << VariantProperty.name() << ')';
}
QTextStream& operator<<(QTextStream &stream, const VariantProperty &property)
{
    stream << "VariantProperty(" << property.name() << ')';

    return stream;
}

} // namespace QmlDesigner
