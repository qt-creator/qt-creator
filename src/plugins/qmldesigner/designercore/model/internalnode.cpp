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

#include "internalnode_p.h"
#include "internalproperty.h"
#include "internalvariantproperty.h"
#include "internalnodeproperty.h"
#include "internalnodelistproperty.h"


#include <QDebug>

namespace QmlDesigner {
namespace Internal {

InternalNodeAbstractProperty::Pointer InternalNode::parentProperty() const
{
    return m_parentProperty;
}
void InternalNode::setParentProperty(const InternalNodeAbstractProperty::Pointer &parent)
{
    InternalNodeAbstractProperty::Pointer parentProperty = m_parentProperty.toStrongRef();
    if (parentProperty)
        parentProperty->remove(shared_from_this());

    Q_ASSERT(parent && parent->isValid());
    m_parentProperty = parent;

    parent->add(shared_from_this());
}

void InternalNode::resetParentProperty()
{
    InternalNodeAbstractProperty::Pointer parentProperty = m_parentProperty.toStrongRef();
    if (parentProperty)
        parentProperty->remove(shared_from_this());

    m_parentProperty.clear();
}

QVariant InternalNode::auxiliaryData(const PropertyName &name) const
{
    return m_auxiliaryDataHash.value(name);
}

void InternalNode::setAuxiliaryData(const PropertyName &name, const QVariant &data)
{
    m_auxiliaryDataHash.insert(name, data);
}

void InternalNode::removeAuxiliaryData(const PropertyName &name)
{
    m_auxiliaryDataHash.remove(name);
}

bool InternalNode::hasAuxiliaryData(const PropertyName &name) const
{
    return m_auxiliaryDataHash.contains(name);
}

const QHash<PropertyName, QVariant> &InternalNode::auxiliaryData() const
{
    return m_auxiliaryDataHash;
}

InternalProperty::Pointer InternalNode::property(const PropertyName &name) const
{
    return m_namePropertyHash.value(name);
}

InternalBindingProperty::Pointer InternalNode::bindingProperty(const PropertyName &name) const
{
    InternalProperty::Pointer property =  m_namePropertyHash.value(name);
    if (property->isBindingProperty())
        return property.staticCast<InternalBindingProperty>();

    return InternalBindingProperty::Pointer();
}

InternalSignalHandlerProperty::Pointer InternalNode::signalHandlerProperty(const PropertyName &name) const
{
    InternalProperty::Pointer property =  m_namePropertyHash.value(name);
    if (property->isSignalHandlerProperty())
        return property.staticCast<InternalSignalHandlerProperty>();

    return InternalSignalHandlerProperty::Pointer();
}

InternalSignalDeclarationProperty::Pointer InternalNode::signalDeclarationProperty(const PropertyName &name) const
{
    InternalProperty::Pointer property =  m_namePropertyHash.value(name);
    if (property->isSignalDeclarationProperty())
        return property.staticCast<InternalSignalDeclarationProperty>();

    return InternalSignalDeclarationProperty::Pointer();
}

InternalVariantProperty::Pointer InternalNode::variantProperty(const PropertyName &name) const
{
    InternalProperty::Pointer property =  m_namePropertyHash.value(name);
    if (property->isVariantProperty())
        return property.staticCast<InternalVariantProperty>();

    return InternalVariantProperty::Pointer();
}

void InternalNode::addBindingProperty(const PropertyName &name)
{
    InternalProperty::Pointer newProperty(InternalBindingProperty::create(name, shared_from_this()));
    m_namePropertyHash.insert(name, newProperty);
}

void InternalNode::addSignalHandlerProperty(const PropertyName &name)
{
    InternalProperty::Pointer newProperty(
        InternalSignalHandlerProperty::create(name, shared_from_this()));
    m_namePropertyHash.insert(name, newProperty);
}

void InternalNode::addSignalDeclarationProperty(const PropertyName &name)
{
    InternalProperty::Pointer newProperty(
        InternalSignalDeclarationProperty::create(name, shared_from_this()));
    m_namePropertyHash.insert(name, newProperty);
}

InternalNodeListProperty::Pointer InternalNode::nodeListProperty(const PropertyName &name) const
{
    InternalProperty::Pointer property =  m_namePropertyHash.value(name);
    if (property && property->isNodeListProperty())
        return property.staticCast<InternalNodeListProperty>();

    return InternalNodeListProperty::Pointer();
}

InternalNodeAbstractProperty::Pointer InternalNode::nodeAbstractProperty(const PropertyName &name) const
{
    InternalProperty::Pointer property =  m_namePropertyHash.value(name);
    if (property && property->isNodeAbstractProperty())
        return property.staticCast<InternalNodeAbstractProperty>();

    return InternalNodeProperty::Pointer();
}

InternalNodeProperty::Pointer InternalNode::nodeProperty(const PropertyName &name) const
{
    InternalProperty::Pointer property =  m_namePropertyHash.value(name);
    if (property->isNodeProperty())
        return property.staticCast<InternalNodeProperty>();

    return InternalNodeProperty::Pointer();
}

void InternalNode::addVariantProperty(const PropertyName &name)
{
    InternalProperty::Pointer newProperty(InternalVariantProperty::create(name, shared_from_this()));
    m_namePropertyHash.insert(name, newProperty);
}

void InternalNode::addNodeProperty(const PropertyName &name, const TypeName &dynamicTypeName)
{
    InternalNodeProperty::Pointer newProperty(InternalNodeProperty::create(name, shared_from_this()));
    newProperty->setDynamicTypeName(dynamicTypeName);
    m_namePropertyHash.insert(name, newProperty);
}

void InternalNode::addNodeListProperty(const PropertyName &name)
{
    InternalProperty::Pointer newProperty(InternalNodeListProperty::create(name, shared_from_this()));
    m_namePropertyHash.insert(name, newProperty);
}

void InternalNode::removeProperty(const PropertyName &name)
{
    InternalProperty::Pointer property = m_namePropertyHash.take(name);
    Q_ASSERT(!property.isNull());
}

PropertyNameList InternalNode::propertyNameList() const
{
    return m_namePropertyHash.keys();
}

bool InternalNode::hasProperties() const
{
    return !m_namePropertyHash.isEmpty();
}

bool InternalNode::hasProperty(const PropertyName &name) const
{
    return m_namePropertyHash.contains(name);
}

QList<InternalProperty::Pointer> InternalNode::propertyList() const
{
    return m_namePropertyHash.values();
}

QList<InternalNodeAbstractProperty::Pointer> InternalNode::nodeAbstractPropertyList() const
{
    QList<InternalNodeAbstractProperty::Pointer> abstractPropertyList;
    const QList<InternalProperty::Pointer> properties = propertyList();
    for (const InternalProperty::Pointer &property : properties) {
        if (property->isNodeAbstractProperty())
            abstractPropertyList.append(property->toNodeAbstractProperty());
    }

    return abstractPropertyList;
}


QList<InternalNode::Pointer> InternalNode::allSubNodes() const
{
    QList<InternalNode::Pointer> nodeList;
    const QList<InternalNodeAbstractProperty::Pointer> properties = nodeAbstractPropertyList();
    for (const InternalNodeAbstractProperty::Pointer &property : properties) {
        nodeList.append(property->allSubNodes());
    }

    return nodeList;
}

QList<InternalNode::Pointer> InternalNode::allDirectSubNodes() const
{
    QList<InternalNode::Pointer> nodeList;
    const QList<InternalNodeAbstractProperty::Pointer> properties = nodeAbstractPropertyList();
    for (const InternalNodeAbstractProperty::Pointer &property : properties) {
        nodeList.append(property->directSubNodes());
    }

    return nodeList;
}

} // namespace Internal
}
